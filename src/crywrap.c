/* -*- mode: c; c-file-style: "gnu" -*-
 * crywrap.c -- CryWrap
 * Copyright (C) 2003, 2004 Gergely Nagy <algernon@bonehunter.rulez.org>
 *
 * This file is part of CryWrap.
 *
 * CryWrap is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CryWrap is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/** @file crywrap.c
 * CryWrap itself.
 */

#include <system.h>
#include "compat/compat.h"

#ifdef HAVE_ARGP_H
#include <argp.h>
#endif
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <grp.h>
#include <idna.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stringprep.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

#include "crywrap.h"
#include "primes.h"

/** @defgroup globals Global variables.
 * @{
 */
/** Status flag to toggle on SIGCHLD.
 */
static sig_atomic_t sigchld = 0;
/** An array of pids.
 * This array holds the PIDs of all of our children, indexed by the
 * socket the associated client connected to us.
 */
static pid_t crywrap_children[_CRYWRAP_MAXCONN + 2];
static pid_t main_pid = -1; /**< Pid of the main process */
static char *pidfile = _CRYWRAP_PIDFILE; /**< File to log our PID
					    into. */
/** GNUTLS server credentials.
 */
static gnutls_certificate_server_credentials cred;
static gnutls_dh_params dh_params; /**< GNUTLS DH parameters. */
static gnutls_rsa_params rsa_params; /**< GNUTLS RSA parameters. */
static const int dh_bits = 1024; /**< GNUTLS DH bits. */

/** Bugreport address.
 * Used by the argp suite.
 */
const char *argp_program_bug_address = "<algernon@bonehunter.rulez.org>";
/** Porgram version.
 * Used by the argp suite.
 */
const char *argp_program_version = __CRYWRAP__ " " _CRYWRAP_VERSION;

/** The options CryWrap takes.
 * Used by the argp suite.
 */
static const struct argp_option _crywrap_options[] = {
	{NULL, 0, NULL, 0, "Mandatory options:", 1},
	{"destination", 'd', "IP/PORT", 0, "IP and port to connect to", 1},
	{NULL, 0, NULL, 0, "TLS certificates:", 2},
	{"pem", 'p', "TYPE=PATH", 0, "Server key and certificate", 2},
	{"anon", 'a', NULL, 0, "Enable Anon-DH (don't use a certificate)", 2},
	{"verify", 'v', "LEVEL", OPTION_ARG_OPTIONAL,
	 "Verify clients certificate", 2},
	{NULL, 0, NULL, 0, "Other options:", 3},
	{"user", 'u', "UID", 0, "User ID to run as", 3},
	{"pidfile", 'P', "PATH", 0, "File to log the PID into", 3},
	{"inetd", 'i', NULL, 0, "Enable inetd mode", 3},
	{"listen", 'l', "IP/PORT", 0, "IP and port to listen on", 3},
	{0, 0, 0, 0, NULL, 0}
};

static error_t _crywrap_config_parse_opt(int key, char *arg,
					 struct argp_state *state);
/** The main argp structure for Crywrap.
 */
static const struct argp _crywrap_argp =
    { _crywrap_options, _crywrap_config_parse_opt, 0,
	__CRYWRAP__ " -- Security for the masses\v"
	    "The --destination option is mandatory, as is --listen if --inetd "
	    "was not used.",
	NULL, NULL, NULL
};

#ifndef __DOXYGEN__
enum {
	CRYWRAP_P_SUBOPT_CERT,
	CRYWRAP_P_SUBOPT_KEY,
	CRYWRAP_P_SUBOPT_END
};
#endif

/** Helper structure for parsing --pem subopts.
 */
static char *_crywrap_p_subopts[] = {
	[CRYWRAP_P_SUBOPT_CERT] = "cert",
	[CRYWRAP_P_SUBOPT_KEY] = "key",
	[CRYWRAP_P_SUBOPT_END] = NULL
};

/** Helper variable to set if a certificate was explictly set.
 */
static int cert_seen = 0;

/** @} */

/* Forward declaration */
static int _crywrap_dh_params_generate(void);
static int _crywrap_rsa_params_generate(void);

/** @defgroup signal Signal handlers & co.
 * @{
 */

/** SIGCHLD handler
 */
static void _crywrap_sigchld_handler(int sig)
{
	sigchld = 1;
	signal(sig, _crywrap_sigchld_handler);
}

/** SIGHUP handler.
 * Regenerates DH and RSA paramaters. Takes a bit long...
 */
static void _crywrap_sighup_handler(int sig)
{
	_crywrap_dh_params_generate();
	_crywrap_rsa_params_generate();

	gnutls_certificate_set_dh_params(cred, dh_params);
	gnutls_certificate_set_rsa_params(cred, rsa_params);

	signal(sig, _crywrap_sighup_handler);
}

/** Generic signal handler.
 * This one removes the #pidfile, if necessary.
 */
static void _crywrap_sighandler(int sig)
{
	if (getpid() == main_pid) {
		bhc_log("Exiting on signal %d", sig);
		if (pidfile && *pidfile)
			unlink(pidfile);
		closelog();
		exit(0);
	}
}

/** @} */

/** @defgroup parsing Option parsing
 * @{
 */

/** Service resolver.
 * Resolves a service - be it a name or a number.
 *
 * @param serv is the port to resolve.
 *
 * @returns The purt number, or -1 on error.
 */
static int _crywrap_port_get(const char *serv)
{
	int port;
	struct servent *se;

	if (!serv)
		return -1;

	se = getservbyname(serv, "tcp");
	if (!se)
		port = atoi(serv);
	else
		port = ntohs(se->s_port);

	return port;
}

/** Address resolver.
 * Resolves an address - be it numeric or a hostname, IPv4 or IPv6.
 *
 * @param hostname is the host to resolve.
 * @param addr is the structure to put the result into.
 *
 * @returns Zero on success, -1 on error.
 */
static int
_crywrap_addr_get(const char *hostname, struct sockaddr_storage **addr)
{
	struct addrinfo *res;
	struct addrinfo hints;
	ssize_t len;
	char *lz = NULL;

	if (idna_to_ascii_lz(hostname, &lz, 0) != IDNA_SUCCESS)
		return -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_IP;
	*addr = bhc_calloc(1, sizeof(struct sockaddr_storage));

	if (getaddrinfo(lz, NULL, &hints, &res) != 0) {
		free(lz);
		return -1;
	}

	free(lz);

	switch (res->ai_addr->sa_family) {
	case AF_INET:
		len = sizeof(struct sockaddr_in);
		break;
	case AF_INET6:
		len = sizeof(struct sockaddr_in6);
		break;
	default:
		freeaddrinfo(res);
		return -1;
	}

	if (len < (ssize_t) res->ai_addrlen) {
		freeaddrinfo(res);
		return -1;
	}

	memcpy(*addr, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);

	return 0;
}

/** Parse a HOST/IP pair.
 * Splits up a given HOST/IP pair, and converts them into structures
 * directly usable by libc routines.
 *
 * @param ip is the HOST/IP pair to parse.
 * @param port is a pointer to an integer where the port number should
 * go.
 * @param addr is the destination of the resolved and parsed IP.
 *
 * @returns Zero on success, -1 on error.
 */
static int
_crywrap_parse_ip(const char *ip, in_port_t * port,
		  struct sockaddr_storage **addr, char **host)
{
	char *s_ip;
	char *tmp;

	tmp = strchr(ip, '/');

	if (!tmp)
		return -1;

	if (tmp == ip) {
		s_ip = bhc_strdup("0.0.0.0");
		*port = (in_port_t) _crywrap_port_get(&ip[1]);
	} else {
		*port = (in_port_t) _crywrap_port_get(&tmp[1]);
		s_ip = bhc_strndup(ip, tmp - ip);
	}

	if (!*port)
		return -1;

	if (host)
		*host = strdup(s_ip);

	return _crywrap_addr_get(s_ip, addr);
}

/** Argument parsing routine.
 * Used by the argp suite.
 */
static error_t
_crywrap_config_parse_opt(int key, char *arg, struct argp_state *state)
{
	crywrap_config_t *cfg = (crywrap_config_t *) state->input;
	char *pem_cert, *pem_key, *subopts, *value;

	switch (key) {
	case 'd':
		if (_crywrap_parse_ip(arg, &cfg->dest.port, &cfg->dest.addr,
				      &cfg->dest.host) < 0)
			argp_error(state, "Could not resolve address: `%s'",
				   arg);
		break;
	case 'l':
		if (_crywrap_parse_ip(arg, &cfg->listen.port,
				      &cfg->listen.addr, NULL) < 0)
			argp_error(state, "Could not resolve address: `%s'",
				   arg);
		break;
	case 'u':
		cfg->uid = atoi(arg);
		break;
	case 'P':
		if (arg && *arg)
			cfg->pidfile = bhc_strdup(arg);
		else
			cfg->pidfile = NULL;
		break;
	case 'p':
		subopts = optarg;
		pem_cert = NULL;
		pem_key = NULL;
		while (*subopts != '\0')
			switch (bhc_getsubopt
				(&subopts, _crywrap_p_subopts, &value)) {
			case CRYWRAP_P_SUBOPT_CERT:
				pem_cert = bhc_strdup(value);
				break;
			case CRYWRAP_P_SUBOPT_KEY:
				pem_key = bhc_strdup(value);
				break;
			default:
				pem_cert = bhc_strdup(value);
				break;
			}
		if (!pem_key)
			pem_key = bhc_strdup(pem_cert);
		if (!pem_cert)
			pem_cert = bhc_strdup(pem_key);
		if (gnutls_certificate_set_x509_key_file
		    (cred, pem_cert, pem_key, GNUTLS_X509_FMT_PEM) < 0)
			argp_error(state,
				   "error reading X.509 key or certificate file.");
		cert_seen = 1;
		break;
	case 'i':
		cfg->inetd = 1;
		break;
	case 'a':
		cfg->anon = 1;
		break;
	case 'v':
		cfg->verify = (arg) ? atoi(arg) : 1;
		break;
	case ARGP_KEY_END:
		if (!cfg->inetd) {
			if (!cfg->listen.addr || !cfg->dest.addr)
				argp_error
				    (state,
				     "a listening and a destination address must be set!");
		} else if (!cfg->dest.addr)
			argp_error(state, "a destination address must be set!");
		if (cert_seen)
			break;
		if (cfg->anon)
			break;
		if (gnutls_certificate_set_x509_key_file(cred, _CRYWRAP_PEMFILE,
							 _CRYWRAP_PEMFILE,
							 GNUTLS_X509_FMT_PEM) <
		    0)
			argp_error(state,
				   "error reading X.509 key or certificate file.");
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/** Configuration parsing.
 * Sets up the default values, and parses the command-line options
 * using argp.
 *
 * @note Does not return if an error occurred.
 */
static crywrap_config_t *_crywrap_config_parse(int argc, char **argv)
{
	crywrap_config_t *config =
	    (crywrap_config_t *) bhc_malloc(sizeof(crywrap_config_t));

	config->listen.port = 0;
	config->listen.addr = NULL;
	config->dest.port = 0;
	config->dest.addr = NULL;

	config->uid = _CRYWRAP_UID;
	config->pidfile = _CRYWRAP_PIDFILE;
	config->inetd = 0;
	config->anon = 0;
	config->verify = 1;

	argp_parse(&_crywrap_argp, argc, argv, 0, 0, config);

	return config;
}

/** @} */

/** @defgroup tls Lower-level TLS routines.
 * @{
 */

/** Callback function.
 * This function gets called by GnuTLS to determine which certificate
 * to use.
 *
 * It checks if the client indicated which host it is connecting to,
 * and if yes, tries to find an appropriate cert.
 *
 * @returns The most appropriate certificate's index on success, zero
 * (the first cert) if no other certificate is appropriate.
 */
static int
_crywrap_session_cert_select(gnutls_session session,
			     gnutls_datum * server_certs, int ncerts)
{
	int idx, data_length = 0, name_type, i;
	char *name;

	/* If we only have one cert, return that. */
	if (ncerts < 2)
		return 0;

	/* Get the length of the data. Note that we only care about the
	   first hostname sent. At least, for now. */
	data_length = 0;
	i = gnutls_server_name_get(session, NULL, &data_length, &name_type, 0);

	/* And get the data itself */
	if (i == GNUTLS_E_SHORT_MEMORY_BUFFER) {
		data_length += 2;
		name = (char *)bhc_malloc(data_length);
		gnutls_server_name_get(session, name, &data_length, &name_type,
				       0);
		if (name_type != GNUTLS_NAME_DNS) {
			free(name);
			return 0;
		}
	} else
		return 0;

	/* Iterate through the certs and select the appropriate one. */
	for (idx = 0; idx < ncerts; idx++) {
		gnutls_x509_crt *crt = (gnutls_x509_crt *) & server_certs[idx];

		if (gnutls_x509_crt_check_hostname(*crt, name) != 0) {
			free(name);
			return idx;
		}
	}

	free(name);
	return 0;
}

/** Create a GNUTLS session.
 * Initialises the cyphers and the session database for a new TLS
 * session.
 *
 * @returns The newly created TLS session.
 */
static gnutls_session
_crywrap_tls_session_create(const crywrap_config_t * config)
{
	gnutls_session session;
	const int comp_prio[] = { GNUTLS_COMP_ZLIB, GNUTLS_COMP_LZO,
		GNUTLS_COMP_NULL, 0
	};
	const int mac_prio[] = { GNUTLS_MAC_SHA, GNUTLS_MAC_MD5, 0 };
	const int kx_prio[] = { GNUTLS_KX_DHE_DSS, GNUTLS_KX_RSA,
		GNUTLS_KX_DHE_RSA, GNUTLS_KX_RSA_EXPORT,
		GNUTLS_KX_ANON_DH, 0
	};
	const int cipher_prio[] = { GNUTLS_CIPHER_RIJNDAEL_128_CBC,
		GNUTLS_CIPHER_3DES_CBC,
		GNUTLS_CIPHER_ARCFOUR_128,
		GNUTLS_CIPHER_ARCFOUR_40, 0
	};
	const int protocol_prio[] = { GNUTLS_TLS1, GNUTLS_SSL3, 0 };

	gnutls_init(&session, GNUTLS_SERVER);

	gnutls_cipher_set_priority(session, cipher_prio);
	gnutls_compression_set_priority(session, comp_prio);
	gnutls_kx_set_priority(session, kx_prio);
	gnutls_protocol_set_priority(session, protocol_prio);
	gnutls_mac_set_priority(session, mac_prio);

	if (config->anon)
		gnutls_credentials_set(session, GNUTLS_CRD_ANON, cred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, cred);

	gnutls_dh_set_prime_bits(session, dh_bits);

	gnutls_handshake_set_private_extensions(session, 1);

	gnutls_certificate_server_set_select_function
	    (session, (gnutls_certificate_server_select_function *)
	     _crywrap_session_cert_select);

	if (config->verify)
		gnutls_certificate_server_set_request(session,
						      GNUTLS_CERT_REQUEST);

	return session;
}

/** (Re)Initialise Diffie Hellman parameters.
 * @returns Zero.
 */
static int _crywrap_dh_params_generate(void)
{
	if (gnutls_dh_params_init(&dh_params) < 0) {
		bhc_error("%s", "Error in dh parameter initialisation.");
		bhc_exit(3);
	}

	if (gnutls_dh_params_generate2(dh_params, dh_bits) < 0) {
		bhc_error("%s", "Error in prime generation.");
		bhc_exit(3);
	}

	gnutls_certificate_set_dh_params(cred, dh_params);

	return 0;
}

/** (Re)Initialise RSA parameters.
 * @returns Zero.
 */
static int _crywrap_rsa_params_generate(void)
{
	if (gnutls_rsa_params_init(&rsa_params) < 0) {
		bhc_error("%s", "Error in RSA parameter initialisation.");
		bhc_exit(3);
	}

	if (gnutls_rsa_params_generate2(rsa_params, 512) < 0) {
		bhc_error("%s", "Error in RSA parameter generation.");
		bhc_exit(3);
	}

	gnutls_certificate_set_rsa_params(cred, rsa_params);

	return 0;
}

/** Generate initial DH and RSA params.
 * Loads the pre-generated DH primes, and generates RSA params.
 */
static void _crywrap_tls_init(void)
{
	gnutls_datum dh_prime_bits, dh_generator;

	/* DH */
	dh_prime_bits.data = _crywrap_prime_dh_1024;
	dh_prime_bits.size = sizeof(_crywrap_prime_dh_1024);

	dh_generator.data = _crywrap_generator_dh;
	dh_generator.size = sizeof(_crywrap_generator_dh);

	gnutls_dh_params_init(&dh_params);
	gnutls_dh_params_import_raw(dh_params, &dh_prime_bits, &dh_generator);

	gnutls_certificate_set_dh_params(cred, dh_params);

	/* RSA */
	_crywrap_rsa_params_generate();
	gnutls_certificate_set_rsa_params(cred, rsa_params);
}

/** @} */

/** @defgroup networking Networking
 * @{
 */

/** Bind to an address.
 * This one binds to an address, handles errors and anything that may
 * arise.
 *
 * @param ai is the address information.
 * @param listen_port is the port to bind to, and listen on.
 *
 * @returns The bound filedescriptor, or -1 on error.
 */
static int _crywrap_bind(const struct addrinfo *ai, int listen_port)
{
	int ret;
	const int one = 1;
	int listenfd;
	char sock_name[NI_MAXHOST];

	listenfd = socket(ai->ai_family, SOCK_STREAM, IPPROTO_IP);
	if (listenfd == -1) {
		bhc_error("socket: %s", strerror(errno));
		return -1;
	}

	memset(sock_name, 0, sizeof(sock_name));
	getnameinfo((struct sockaddr *)ai->ai_addr, ai->ai_addrlen, sock_name,
		    sizeof(sock_name), NULL, 0, NI_NUMERICHOST);

	switch (ai->ai_family) {
	case AF_INET6:
		((struct sockaddr_in6 *)(ai->ai_addr))->sin6_port = listen_port;
		break;
	case AF_INET:
		((struct sockaddr_in *)(ai->ai_addr))->sin_port = listen_port;
		break;
	}

	ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if (ret != 0) {
		bhc_error("setsockopt: %s (%s)", strerror(errno), sock_name);
		return -1;
	}

	ret = bind(listenfd, ai->ai_addr, ai->ai_addrlen);
	if (ret != 0) {
		bhc_error("bind to %s failed: %s", sock_name, strerror(errno));
		return -1;
	}

	if (listen(listenfd, _CRYWRAP_MAXCONN) != 0) {
		bhc_error("listen on %s failed: %s", sock_name,
			  strerror(errno));
		return -1;
	}

	bhc_log("Socket bound to port %d on %s.", ntohs(listen_port),
		sock_name);

	return listenfd;
}

/** Set up a listening socket.
 * Sets up a listening socket on all the required addresses.
 *
 * @param config holds the CryWrap configuration, from where the
 * listen address and port will be extracted.
 *
 * @returns The listening FD on success, -1 on error.
 */
static int _crywrap_listen(const crywrap_config_t * config)
{
	struct addrinfo *cur;
	int ret;

	cur = bhc_calloc(1, sizeof(struct addrinfo));
	cur->ai_family = config->listen.addr->ss_family;

	switch (cur->ai_family) {
	case AF_INET6:
		cur->ai_addrlen = sizeof(struct sockaddr_in6);
		break;
	case AF_INET:
		cur->ai_addrlen = sizeof(struct sockaddr_in);
		break;
	}

	cur->ai_addr = bhc_malloc(cur->ai_addrlen);
	memcpy(cur->ai_addr, config->listen.addr, cur->ai_addrlen);

	ret = _crywrap_bind(cur, htons(config->listen.port));
	free(cur->ai_addr);
	free(cur);

	return ret;
}

/** Connect to a remote server.
 * Estabilishes a connection to a remote server, and handles all
 * errors and anything that may arise during this process.
 *
 * @param addr is the address of the remote server.
 * @param port is the port to connect to.
 *
 * @returns the connected socket on success, otherwise it exits.
 */
static int
_crywrap_remote_connect(const struct sockaddr_storage *addr, int port)
{
	struct addrinfo *cur;
	int sock;

	cur = bhc_calloc(1, sizeof(struct addrinfo));
	cur->ai_family = addr->ss_family;

	switch (cur->ai_family) {
	case AF_INET6:
		cur->ai_addrlen = sizeof(struct sockaddr_in6);
		break;
	case AF_INET:
		cur->ai_addrlen = sizeof(struct sockaddr_in);
		break;
	}

	cur->ai_addr = bhc_malloc(cur->ai_addrlen);
	memcpy(cur->ai_addr, addr, cur->ai_addrlen);

	switch (cur->ai_family) {
	case AF_INET6:
		((struct sockaddr_in6 *)(cur->ai_addr))->sin6_port = port;
		break;
	case AF_INET:
		((struct sockaddr_in *)(cur->ai_addr))->sin_port = port;
		break;
	}

	sock = socket(cur->ai_family, SOCK_STREAM, IPPROTO_IP);
	if (sock < 0) {
		bhc_error("socket(): %s", strerror(errno));
		bhc_exit(1);
	}

	if (connect(sock, cur->ai_addr, cur->ai_addrlen) < 0) {
		bhc_error("connect(): %s", strerror(errno));
		bhc_exit(1);
	}

	free(cur->ai_addr);
	free(cur);

	return sock;
}

/** @} */

/** @defgroup crywrap Main CryWrap code.
 * @{
 */

/** Drop privileges.
 * Drop privileges, if running as root.
 * Upon failure, it will make CryWrap exit.
 */
static void _crywrap_privs_drop(const crywrap_config_t * config)
{
	struct passwd *pwd;

	if (getuid() != 0) {
		bhc_log("%s", "Not running as root, not dropping privileges.");
		return;
	}

	if ((pwd = getpwuid(config->uid)) == NULL) {
		bhc_error("getpwuid(): %s", strerror(errno));
		bhc_exit(1);
	}

	if (initgroups(pwd->pw_name, pwd->pw_gid) == -1) {
		bhc_error("initgroups(): %s", strerror(errno));
		bhc_exit(1);
	}

	if (setgid(pwd->pw_gid) == -1) {
		bhc_error("setgid(): %s", strerror(errno));
		bhc_exit(1);
	}

	if (setuid(config->uid)) {
		bhc_error("setuid(): %s", strerror(errno));
		bhc_exit(1);
	}
}

/** Set up the PID file.
 * Checks if a #pidfile already exists, and create one - containing the
 * current PID - if one does not.
 *
 * @note Exits upon error.
 */
static void _crywrap_setup_pidfile(const crywrap_config_t * config)
{
	char mypid[128];
	int pidfilefd;

	if (!config->pidfile || !*(config->pidfile))
		return;

	if (!access(config->pidfile, F_OK)) {
		bhc_error("Pidfile (%s) already exists. Exiting.",
			  config->pidfile);
		exit(1);
	}
	if ((pidfilefd = open(config->pidfile,
			      O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
		bhc_error("Cannot create pidfile (%s): %s.\n", config->pidfile,
			  strerror(errno));
		exit(1);
	}
	fchown(pidfilefd, config->uid, (gid_t) - 1);

	main_pid = getpid();
	snprintf(mypid, sizeof(mypid), "%d\n", main_pid);
	write(pidfilefd, mypid, strlen(mypid));
	close(pidfilefd);
	pidfile = config->pidfile;
}

/** Child cleanup routine.
 * Called after a SIGCHLD is received. Walks through #crywrap_children
 * and closes the socket of the one that exited.
 */
static void _crywrap_reap_children(void)
{
	pid_t child;
	int status, i;

	while ((child = waitpid(-1, &status, WNOHANG)) > (pid_t) 0) {
		for (i = 0; i < _CRYWRAP_MAXCONN; i++) {
			if (!crywrap_children[i])
				continue;
			if (child == crywrap_children[i]) {
				shutdown(i, SHUT_RDWR);
				close(i);
				crywrap_children[i] = 0;
			}
		}
	}
	sigchld = 0;
}

/** Handles one client.
 * This one connects to the remote server, and proxies every traffic
 * between our client and the server.
 *
 * @param config is the main CryWrap configuration structure.
 * @param insock is the socket through which the client sends input.
 * @param outsock is the socket through which we send output.
 *
 * @note Exits on error.
 */
static int
_crywrap_do_one(const crywrap_config_t * config, int insock, int outsock)
{
	int sock, ret;
	gnutls_session session;
	char buffer[_CRYWRAP_MAXBUF + 2];
	fd_set fdset;
	struct sockaddr_storage faddr;
	socklen_t socklen = sizeof(struct sockaddr_storage);
	char peer_name[NI_MAXHOST];

	/* Log the connection */
	if (getpeername(insock, (struct sockaddr *)&faddr, &socklen) != 0)
		bhc_error("getpeername(): %s", strerror(errno));
	else {
		getnameinfo((struct sockaddr *)&faddr,
			    sizeof(struct sockaddr_storage), peer_name,
			    sizeof(peer_name), NULL, 0, NI_NUMERICHOST);
		bhc_log("Accepted connection from %s on %d to %s/%d",
			peer_name, insock, config->dest.host,
			config->dest.port);
	}

	/* Do the handshake with our peer */
	session = _crywrap_tls_session_create(config);
	gnutls_transport_set_ptr2(session,
				  (gnutls_transport_ptr) insock,
				  (gnutls_transport_ptr) outsock);
	if ((ret = gnutls_handshake(session)) < 0) {
		bhc_error("Handshake failed: %s", gnutls_strerror(ret));
		gnutls_bye(session, GNUTLS_SHUT_RDWR);
		close(insock);
		close(outsock);
		return 1;
	}

	/* Verify the client's certificate, if any. */
	if (config->verify) {
		ret = gnutls_certificate_verify_peers(session);
		if (ret < 0)
			bhc_log("Error getting certificate from client: %s",
				gnutls_strerror(ret));
		else if (ret != 0)
			switch (ret) {
			case GNUTLS_CERT_INVALID:
				bhc_log("%s",
					"Client certificate not trusted or invalid");
				break;
			default:
				bhc_log("%s",
					"Unknown error while getting the certificate");
				break;
			}

		if (config->verify > 1 && ret != 0)
			return 1;
	}

	/* Connect to the remote host */
	sock = _crywrap_remote_connect(config->dest.addr,
				       htons(config->dest.port));

	for (;;) {
		FD_ZERO(&fdset);
		FD_SET(insock, &fdset);
		FD_SET(sock, &fdset);

		bzero(buffer, _CRYWRAP_MAXBUF + 1);
		select(sock + 1, &fdset, NULL, NULL, NULL);

		/* TLS client */
		if (FD_ISSET(insock, &fdset)) {
			ret =
			    gnutls_record_recv(session, buffer,
					       _CRYWRAP_MAXBUF);
			if (ret == 0) {
				bhc_log("%s",
					"Peer has closed the GNUTLS connection");
				break;
			} else if (ret < 0) {
				bhc_log("Received corrupted data: %s.",
					gnutls_strerror(ret));
				break;
			} else
				send(sock, buffer, ret, 0);
		}

		/* Remote server */
		if (FD_ISSET(sock, &fdset)) {
			ret = recv(sock, buffer, _CRYWRAP_MAXBUF, 0);
			if (ret == 0) {
				bhc_log("%s",
					"Server has closed the connection");
				break;
			} else if (ret < 0) {
				bhc_log("Received corrupted data: %s.",
					strerror(errno));
				break;
			} else {
				int r, o = 0;

				do {
					r = gnutls_record_send(session,
							       &buffer[o],
							       ret - o);
					o += r;
				} while (r > 0 && ret > o);

				if (r < 0)
					bhc_log("Received corrupted data: %s",
						gnutls_strerror(r));
			}
		}
	}
	gnutls_bye(session, GNUTLS_SHUT_WR);
	gnutls_deinit(session);
	close(insock);
	close(outsock);

	return (ret == 0) ? 0 : 1;
}

/** CryWrap entry point.
 * This is the main entry point - controls the whole program and so
 * on...
 */
int main(int argc, char **argv, char **envp)
{
	crywrap_config_t *config;
	int server_socket;

	openlog(__CRYWRAP__, LOG_PID, LOG_DAEMON);

	if (gnutls_global_init() < 0) {
		bhc_error("%s", "Global TLS state initialisation failed.");
		bhc_exit(1);
	}
	if (gnutls_certificate_allocate_credentials(&cred) < 0) {
		bhc_error("%s", "Couldn't allocate credentials.");
		bhc_exit(1);
	}

	stringprep_locale_charset();

	bhc_setproctitle_init(argc, argv, envp);
	config = _crywrap_config_parse(argc, argv);
	bhc_setproctitle(__CRYWRAP__);

	_crywrap_tls_init();

	if (config->inetd) {
		_crywrap_privs_drop(config);
		exit(_crywrap_do_one(config, 0, 1));
	}
#if CRYWRAP_OPTION_FORK
	if (daemon(0, 0)) {
		bhc_error("daemon: %s", strerror(errno));
		exit(1);
	}
#endif

	bhc_log("%s", "Crywrap starting...");

	server_socket = _crywrap_listen(config);
	if (server_socket < 0)
		exit(1);

	_crywrap_setup_pidfile(config);
	_crywrap_privs_drop(config);

	signal(SIGTERM, _crywrap_sighandler);
	signal(SIGQUIT, _crywrap_sighandler);
	signal(SIGSEGV, _crywrap_sighandler);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, _crywrap_sighup_handler);

	bhc_log("%s", "Accepting connections");

	memset(crywrap_children, 0, sizeof(crywrap_children));
	signal(SIGCHLD, _crywrap_sigchld_handler);

	for (;;) {
		int csock;
#if !BHC_OPTION_DEBUG
		int child;
#endif

		if (sigchld)
			_crywrap_reap_children();

		csock = accept(server_socket, NULL, NULL);
		if (csock < 0)
			continue;

#if !BHC_OPTION_DEBUG
		child = fork();
		switch (child) {
		case 0:
			exit(_crywrap_do_one(config, csock, csock));
			break;
		case -1:
			bhc_error("%s", "Forking error.");
			bhc_exit(1);
			break;
		default:
			crywrap_children[csock] = child;
			break;
		}
#else
		_crywrap_do_one(config, csock, csock);
#endif
	}

	return 0;
}

/** @} */

/** arch-tag: 3d45d946-5d09-493d-ae6e-26effb71eb6b */
