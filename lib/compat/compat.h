/* -*- mode: c; c-file-style: "gnu" -*-
 * compat.h -- Prototypes for glibc-specific functions
 * Copyright (C) 2002, 2003 Gergely Nagy <algernon@bonehunter.rulez.org>
 *
 * This file is part of The BoneHunter compat suite.
 *
 * The BoneHunter compat suite is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * The BoneHunter compat suite is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/** @file compat.h
 * Prototypes for glibc-specific functions.
 */

#ifndef _BHC_COMPAT_H
#define _BHC_COMPAT_H 1 /**< compat.h multi-inclusion guard. */

#include "system.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <dirent.h>
#include <errno.h>
#include <netdb.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>

/** C99-style enum initialiser wrapper.
 * This macro is used to allow non-C99 compilers (hi, AIX!) to compile
 * BoneHunter software, why allowing C99-enabled ones to... well, to
 * do stuff.
 *
 * However, there is no C99 check in configure at the moment, so this
 * macro is a no-op for now.
 */
#define C99_INIT(a)

/** Convert a string to long, with error checking.
 * @param str is the string to convert.
 * @param result is where the result will go.
 *
 * @returns Zero on success, -1 otherwise. If there was an error,
 * result will be left untouched.
 */
int bhc_atoi (const char *str, long *result);

/** Get the current working directory.
 * @returns A newly allocated string, containing the current working
 * directory.
 */
char *bhc_getcwd (void);

/** Initialiser for bhc_setproctitle().
 * This saves the start and the end of argv, for later use by
 * bhc_setproctitle(). Provided of course, that there is no native
 * setproctitle().
 *
 * @param argc is the argument count.
 * @param argv is the argument vector.
 * @param envp is the environment vector.
 */
void bhc_setproctitle_init (int argc, char **argv, char **envp);

/** Sets the process title.
 * Change the name of the process - as appears in ps(1).
 *
 * @param name is the new title of the process.
 */
void bhc_setproctitle (const char *name);

/** Convenience wrapper around bhc_realloc().
 */
#define XSREALLOC(ptr, type, nmemb) \
	ptr = (type*) bhc_realloc (ptr, (nmemb) * sizeof (type))

void *bhc_malloc (size_t size);
void *bhc_realloc (void *ptr, size_t size);
void *bhc_calloc (size_t nmemb, size_t size);
char *bhc_strdup (const char *src);
char *bhc_strndup (const char *src, size_t n);

void bhc_exit (int code);

/** getsubopt() replacement.
 * This function is either a wrapper around getsubopt() or a complete
 * reimplementation.
 *
 * @param optionp is a pointer to the string to process.
 * @param tokens references an array of strings containing the known
 * suboptions.
 * @param valuep will be used to return the value of the suboption.
 *
 * @returns Zero on success, -1 otherwise.
 *
 * @note Modifies #valuep in-place.
 */
int bhc_getsubopt (char **optionp, char *const *tokens,
		   char **valuep);

#ifndef __DOXYGEN__

#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif

#ifndef socklen_t
#define socklen_t int
#endif

#ifndef error_t
#define error_t int
#endif

#ifndef HAVE_SOCKADDR_STORAGE
struct sockaddr_storage
{
  uchar ss_len;  /* address length */
  uchar ss_family; /* address family */
  unsigned char fill[126];
};
#endif

#ifndef AI_NUMERICHOST
#define AI_NUMERICHOST 0 /* If it is not defined - it is not
			    supported, so define it to zero. */
#endif

#if !defined(HAVE_BASENAME) || defined (_AIX)
char *basename (const char *path);
#endif

#if !defined(HAVE_DIRNAME) || defined (_AIX)
char *dirname (char *path);
#endif

#ifndef HAVE_VASPRINTF
int vasprintf (char **ptr, const char *fmt, va_list a);
#endif

#ifndef HAVE_ASPRINTF
int asprintf (char **ptr, const char *fmt, ...);
#endif

#ifndef HAVE_VSNPRINTF
int vsnprintf (char *str, size_t size, const char *format, va_list ap);
#endif

#ifndef HAVE_DAEMON
int daemon (int nochdir, int noclose);
#endif

#ifndef HAVE_CONFSTR
#define _CS_PATH 1
size_t confstr (int name, char *buf, size_t len);
#endif

#ifndef HAVE_MEMPCPY
void *mempcpy (void *TO, const void *FROM, size_t SIZE);
#endif
#ifndef HAVE_CANONICALIZE_FILE_NAME
char *canonicalize_file_name (const char *fn);
#endif
#ifndef HAVE_STRNDUP
char *strndup (const char *s, size_t size);
#endif
#ifndef HAVE_STRPTIME
char *strptime (const char *s, const char *format, struct tm *tm);
#endif

#ifndef HAVE_GETLINE
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#endif
#ifndef HAVE_GETDELIM
ssize_t
getdelim (char **lineptr, size_t *n, int delim, FILE *stream);
#endif

#ifndef HAVE_ARGP_PARSE
struct argp_option
{
  char *name;
  int key;
  const char *arg;
  int flags;
  char *doc;
  int group;
};
struct argp;
struct argp_state;
typedef error_t (*argp_parser_t) (int key, char *arg,
				  struct argp_state *state);
#define ARGP_ERR_UNKNOWN E2BIG
#define ARGP_KEY_END 0x1000001
#define ARGP_KEY_ARG 0

#define OPTION_ARG_OPTIONAL 0x1
#define OPTION_HIDDEN 0x2
struct argp
{
  const struct argp_option *options;
  argp_parser_t parser;
  const char *args_doc;
  const char *doc;
  /* The rest is ignored */
  const void *children;
  char *(*help_filter) (int key, const char *text, void *input);
  const char *domain;
};

struct argp_state
{
  /* This is deliberately not compatible with glibc's one. We only use
     ->input anyway. And argv[0]... */
  void *input;
  char *argv0;
};

extern const char *argp_program_version;
extern const char *argp_program_bug_address;
extern void (*argp_program_version_hook) (FILE *stream,
					  struct argp_state *state);

error_t argp_parse (const struct argp *argps, int argc, char **argv,
		    unsigned flags, int *arg_index, void *input);
error_t argp_error (const struct argp_state *state, char *fmt, ...);
#endif

#ifndef HAVE_SCANDIR
int scandir (const char *dir, struct dirent ***namelist,
	     int (*sd_select)(const struct dirent *),
	     int (*sd_compar)(const struct dirent **,
			      const struct dirent **));
#endif
#ifndef HAVE_ALPHASORT
int alphasort (const struct dirent **a, const struct dirent **b);
#endif
#ifndef HAVE_SETENV
int setenv(const char *name, const char *value, int overwrite);
#endif

#ifndef HAVE_VSYSLOG
void vsyslog(int priority, const char *format, va_list ap);
#endif

#if !defined(va_copy) && !defined(__va_copy)
#define __va_copy(dst,src) memcpy (&dst, &src, sizeof (va_list))
#endif

#if !defined(va_copy)
#define va_copy __va_copy
#endif

#ifdef _AIX
char *strndup (const char *s, size_t size);
#endif

#ifdef __dietlibc__
int strncmp (const char *s1, const char *s2, size_t n);
int strcasecmp (const char *s1, const char *s2);
#endif

#endif /* !__DOXYGEN__ */

/* Note: The code below is ugly like hell. Do not even try to look at
   it. You have been warned. */
#ifndef __DOXYGEN__
#ifndef BHC_OPTION_DEBUG
#define BHC_OPTION_DEBUG 0
#endif

#if BHC_OPTION_NODEBUG
#undef BHC_OPTION_DEBUG
#endif

#if HAVE___VA_ARGS__
#  if BHC_OPTION_ALL_LOGGING
#    define bhc_error(fmt,...) syslog (LOG_WARNING, fmt, __VA_ARGS__)
#    define bhc_log(fmt,...) syslog (LOG_NOTICE, fmt, __VA_ARGS__)
#    if BHC_OPTION_DEBUG
#      define bhc_debug(fmt,...) syslog (LOG_DEBUG, fmt, __VA_ARGS__)
#    else
#      define bhc_debug(fmt,...)
#    endif
#  else
#    define bhc_error(fmt,...)
#    define bhc_log(fmt,...)
#    define bhc_debug(fmt,...)
#  endif
#else /* HAVE___VA_ARGS__ */
#  ifdef HAVE_VARIADIC_MACROS
#    if BHC_OPTION_ALL_LOGGING
#      define bhc_error(format,args...) \
		syslog (LOG_WARNING, format , ## args)
#      define bhc_log(format,args...) \
		syslog (LOG_NOTICE, format , ## args)
#      if BHC_OPTION_DEBUG
#	 define bhc_debug(format,args...) \
		  syslog (LOG_DEBUG, format , ## args)
#      else
#	 define bhc_debug(format,args...)
#      endif
#    else
#      define bhc_error(format,args...)
#      define bhc_log(format,args...)
#      define bhc_debug(format,args...)
#    endif
#  else
     void bhc_error (char *fmt, ...);
     void bhc_log (char *fmt, ...);
     void bhc_debug (char *fmt, ...);
#  endif /* !HAVE_VARIADIC_MACROS */
#endif /* !HAVE___VA_ARGS__ */
#else /* __DOXYGEN__ */
/** Log an error condition.
 * This is a wrapper around syslog(), with priority set to
 * LOG_WARNING. After called, the caller should return an error
 * condition.
 */
#define bhc_error(fmt,...)
/** Log various stuff.
 * This wrapper is around to log miscellaneous events. Such as served
 * requests and startup information.
 */
#define bhc_log(fmt,...)
/** Wrapper for logging debug information.
 * Every function that wants to emit information only useful
 * debugging, should use this function. It will not be compiled in if
 * BHC_OPTION_DEBUG is unset.
 */
#define bhc_debug(fmt,...)
#endif

#endif /* !_BHC_COMPAT_H */

/* arch-tag: c8c8c7bc-b88b-4e0a-9c18-51f0fd3c8ba4 */
