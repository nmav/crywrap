# arch-tag: 9e49a1d9-1005-4ae9-8bf1-04a7b9b80285
dnl configure.ac for CryWrap
dnl Copyright (C) 2003, 2004 Gergely Nagy <algernon@bonehunter.rulez.org>
dnl
dnl Process this file with autoconf to produce a configure script.
dnl
dnl This file is part of CryWrap.
dnl
dnl CryWrap is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl CryWrap is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

dnl **********************
dnl * Boilerplate
dnl **********************
AC_PREREQ(2.53)
AC_INIT([crywrap], [0.2-stable], [algernon@bonehunter.rulez.org])
AC_CONFIG_SRCDIR(src/crywrap.c)

AC_CONFIG_HEADERS(system.h)

AC_CANONICAL_HOST
AC_SUBST(host_os)

dnl **********************
dnl * Programs
dnl **********************
BHM4_CC_CHECK

AC_PROG_INSTALL

BHMK_MAKE_CHECK

dnl **********************
dnl * Headers
dnl **********************
AC_HEADER_SYS_WAIT

AC_CHECK_HEADERS([arpa/inet.h netinet/in.h sys/select.h sys/types.h])

dnl **********************
dnl * Typedefs & co
dnl **********************
AC_STRUCT_TM
AC_TYPE_UID_T
AC_TYPE_PID_T

BHM4_TCP_CORK_CHECK
BHM4_SOCKADDR_STORAGE_CHECK

dnl **********************
dnl * Library functions
dnl **********************
AC_FUNC_FORK
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_LSTAT
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_STRFTIME
AC_FUNC_MEMCMP
AC_FUNC_VPRINTF
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_MMAP
AC_CHECK_FUNCS([alarm atexit dup2 epoll_create kqueue memchr memset munmap \
		putenv regcomp scandir select socket strcasecmp strchr \
		strdup strerror strncasecmp strrchr strstr strtoul uname])
AC_CHECK_FUNC([sendfile], [], [AC_CHECK_LIB(sendfile, sendfile)])

dnl **********************
dnl * BH compat suite
dnl **********************

BHCOMPAT_CHECK

dnl **********************
dnl * Rest
dnl **********************

PKG_CHECK_MODULES([LIBGNUTLS], [gnutls])

AC_CHECK_HEADER(idna.h,
	AC_CHECK_LIB(idn, stringprep_check_version,
		[libidn=yes; AC_SUBST(SHISHI_LIBS, -lidn)], libidn=no))

if test "$libidn" != "no" ; then
	AC_DEFINE(USE_LIBIDN, 1, [Define to 1 if you want Libidn.])
else
	AC_MSG_ERROR([You need Libidn to compile CryWrap])
fi

dnl **********************
dnl * Features
dnl **********************
dnl * Fork
AC_ARG_ENABLE(fork,
	AC_HELP_STRING([--disable-fork], [disable forking to background]),
	[f_fork="$enableval"], [f_fork="yes"])
if test "x$f_fork" != "xno"; then
	AC_DEFINE_UNQUOTED(CRYWRAP_OPTION_FORK, 1,
			   Define this to enable forking to background)
else
	AC_DEFINE_UNQUOTED(CRYWRAP_OPTION_NOFORK, 1,
			   Define this to disable forking to background)
fi

dnl **********************
dnl * Output
dnl **********************
AC_CONFIG_FILES([
Makefile
Mk/Rules.mk
Mk/Makefile
config.mk
doc/Makefile
etc/Makefile
history/Makefile
lib/Makefile
lib/compat/Makefile
m4/Makefile
m4/bh-m4/Makefile
src/Makefile
utils/Makefile
])

AC_OUTPUT
