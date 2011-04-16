# arch-tag: f22024e7-e564-4dd9-a659-fab78821f91e

AC_DEFUN([BHCOMPAT_DEBUG], [

dnl * Debug
AC_ARG_ENABLE(debug,
	AC_HELP_STRING([--enable-debug], [enable debug mode]),
	[f_debug="$enableval"], [f_debug="no"])
if test "x$f_debug" != "xno"; then
	AC_DEFINE_UNQUOTED(BHC_OPTION_DEBUG, 1,
			   Define this to enable debug mode)
else
	AC_DEFINE_UNQUOTED(BHC_OPTION_NODEBUG, 1,
			   Define this to disable debug mode)
fi

])

AC_DEFUN([BHCOMPAT_LOGGING], [

dnl * all-logging
AC_ARG_ENABLE(all-logging,
	AC_HELP_STRING([--disable-all-logging],
		       [disable all kinds of logging]),
	[f_allog="$enableval"], [f_allog="yes"])
if test "x$f_allog" != "xno"; then
	AC_DEFINE_UNQUOTED(BHC_OPTION_ALL_LOGGING, 1,
			   Define this to enable logging support)
else
	AC_DEFINE_UNQUOTED(BHC_OPTION_NOALL_LOGGING, 1,
			   Define this to disable all logging support)
fi

])

AC_DEFUN([BHCOMPAT_CHECK], [

BHCOMPAT_DEBUG
BHCOMPAT_LOGGING

AC_HEADER_DIRENT
AC_HEADER_STDC

AC_CHECK_HEADERS([argp.h errno.h fcntl.h getopt.h limits.h netdb.h \
		  paths.h stddef.h strings.h sys/socket.h sys/types.h \
		  syslog.h])

AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

AC_CHECK_SIZEOF(size_t)
AC_CHECK_TYPE(error_t, int)

AC_FUNC_MALLOC
m4_ifdef([AC_FUNC_REALLOC], [AC_FUNC_REALLOC])

AC_CHECK_FUNCS([alphasort argp_parse asprintf basename canonicalize_file_name confstr daemon dirname getcwd getdelim getline getopt_long getsubopt mempcpy setenv setproctitle strndup strptime vasprintf vsnprintf vsyslog])

AC_CHECK_LIB(nsl, inet_ntoa)

AC_CHECK_FUNC([getopt_long], [], [AC_CHECK_LIB(gnugetopt, getopt_long)])
AC_CHECK_LIB(compat, daemon, [AC_DEFINE(HAVE_DAEMON, 1)
LIBS="$LIBS -lcompat"])
AC_CHECK_LIB(socket, socket)

AC_CACHE_CHECK([for suboptarg], [ac_cv_suboptarg],
	       [ac_cv_suboptarg=no
		AC_RUN_IFELSE(AC_LANG_PROGRAM(
			[[#include <stdio.h>
			  #include <stdlib.h>
			  #include <unistd.h>

			  static char **empty_subopts[[]] = { NULL };
			  extern char *suboptarg;
			]], [[  char *subopts = "something";
				char *value;

				getsubopt (&subopts, empty_subopts, &value);
				exit (!suboptarg);]]),
		[ac_cv_suboptarg="yes"], [ac_cv_suboptarg="no"],
		[ac_cv_suboptarg="no"])])
case ${ac_cv_suboptarg} in
	yes)
		AC_DEFINE_UNQUOTED(HAVE_SUBOPTARG, 1,
			Define this if you have the suboptarg variable)
		;;
esac

AC_CACHE_CHECK([whether setproctitle() works], [ac_cv_setproctitle_ok],
	       [ac_cv_setproctitle_ok=yes
		AC_RUN_IFELSE(AC_LANG_PROGRAM(
			[[#include <sys/types.h>
			  #include <stdlib.h>
			  #include <unistd.h>
			]], [[  setproctitle (NULL);
				exit (0);]]),
	       [ac_cv_setproctitle_ok="yes"], [ac_cv_setproctitle_ok="no"],
	       [ac_cv_setproctitle_ok="yes"])])
case ${ac_cv_setproctitle_ok} in
	yes)
		AC_DEFINE_UNQUOTED(HAVE_WORKING_SETPROCTITLE, 1,
			Define this if setproctitle(NULL) works)
		;;
esac

AC_CACHE_CHECK([whether getsubopt() is broken], [ac_cv_getsubopt_broken],
	       [ac_cv_getsubopt_broken=no
		AC_RUN_IFELSE(AC_LANG_PROGRAM(
			[[#include <unistd.h>
			  #include <stdio.h>
			  #include <stdlib.h>
			  #include <string.h>

			  static char *empty_subopts[[]] = { NULL };
			]], [[  char *optionp;
				char *value;
				char *tmp;

				optionp = strdup ("foo=bar");
				tmp = optionp;
				getsubopt (&optionp, empty_subopts, &value);

				if (!value)
				  exit (1);

				if (!strcmp (value, tmp))
				  exit (0);
				else
				  exit (1);]]),
		[ac_cv_getsubopt_broken="no"], [ac_cv_getsubopt_broken="yes"],
		[ac_cv_getsubopt_broken="no"])])
case ${ac_cv_getsubopt_broken} in
	yes)
		AC_DEFINE_UNQUOTED(HAVE_BROKEN_GETSUBOPT, 1,
			Define this if your getsubopt() is broken)
		;;
esac

])

AC_DEFUN([BHCOMPAT_REGEX_CHECK], [

AC_CHECK_HEADERS([regex.h])
AC_CACHE_CHECK([whether <regex.h> works],
	       [ac_cv_regex_h_ok], [ac_cv_regex_h_ok=no
		AC_COMPILE_IFELSE(AC_LANG_PROGRAM(
			[#include <sys/types.h>
			 #include <regex.h>
			], []), [ac_cv_regex_h_ok=yes],
			[ac_cv_regex_h_ok=no])])
case ${ac_cv_regex_h_ok} in
	yes)
		AC_DEFINE(HAVE_WORKING_REGEX_H, 1,
			[Define if your <regex.h> is usable])
		;;
esac

])
