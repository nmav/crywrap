/* -*- mode: c; c-file-style: "gnu" -*-
 * crywrap.h -- Global definitions for CryWrap
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

/** @file crywrap.h
 * Global variables and declarations for CryWrap.
 *
 * All of the global types, structures and whatnot are declared in
 * this file. Not variables, though. Those are in crywrap.c.
 */

#ifndef _CRYWRAP_H
#define _CRYWRAP_H 1 /**< crywrap.h multi-inclusion guard. */

/** @defgroup defaults Built-in defaults.
 * @{
 */
#define __CRYWRAP__ "crywrap" /**< Software name. */
/** Software version.
 */
#define _CRYWRAP_VERSION "0.2." PATCHLEVEL EXTRAVERSION
/** Configuration directory.
 */
#define _CRYWRAP_CONFDIR SYSCONFDIR "/crywrap"
#define _CRYWRAP_UID 65534 /**< Default UID to run as. */
/** Default PID file.
 */
#define _CRYWRAP_PIDFILE "/var/run/crywrap.pid"
/** Maximum number of clients supported.
 */
#define _CRYWRAP_MAXCONN 1024
/** Maximum I/O buffer size.
 */
#define _CRYWRAP_MAXBUF 64 * 1024
/** Default server certificate and key.
 */
#define _CRYWRAP_PEMFILE _CRYWRAP_CONFDIR "/server.pem"
/** @} */

/** Configuration structure.
 * Most of the CryWrap configuration - those options that are settable
 * via the command-line are stored in a variable of this type.
 */
typedef struct {
  /** Properties of the listening socket.
   */
	struct {
		in_port_t port;
		struct sockaddr_storage *addr;
	} listen;

  /** Properties of the destination socket.
   */
	struct {
		in_port_t port;
		char *host;
		struct sockaddr_storage *addr;
	} dest;

	char *pidfile;
		 /**< File to store our PID in. */
	uid_t uid;
	     /**< User ID to run as. */
	int inetd;
	     /**< InetD-mode toggle. */
	int anon;
	    /**< Anon-DH toggle. */
	int verify;
	      /**< Client certificate verify level. */
} crywrap_config_t;

/** @defgroup options Options.
 * These are the compile-time options.
 * @{
 */
/** If this option is set, CryWrap will fork into the background.
 */
#ifndef CRYWRAP_OPTION_FORK
#define CRYWRAP_OPTION_FORK 1
#endif

#if CRYWRAP_OPTION_NOFORK
#undef CRYWRAP_OPTION_FORK
#endif

	 /** @} *//* End of the Options group */

#endif				/* !_CRYWRAP_H */

/* arch-tag: ebfe1550-0fec-4c0d-8833-23e48292e75d */
