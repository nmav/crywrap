# arch-tag: 5b59cf27-9403-4203-9a20-b3e57ee97b45

AC_DEFUN([BHM4_CC_CHECK],[

AC_CHECK_TOOL(CC, cc)
AC_PROG_CC

WFLAGS=""
WFLAGS_3X=""
WFLAGS_29X=""
if test "x$ac_compiler_gnu" = "xyes"; then
	WFLAGS='${WFLAGS_GCC}'
	AC_MSG_CHECKING(whether we are using GCC >= 2.9x)
	GCCVER=$(${CC} -dumpversion 2>/dev/null)
	case ${GCCVER} in
		2.9*)
			WFLAGS_29X='${WFLAGS_29X}'
			AC_MSG_RESULT([yes, ${GCCVER}])
			;;
		3.*)
			WFLAGS_29X='${WFLAGS_29X}'
			WFLAGS_3X='${WFLAGS_3X}'
			AC_MSG_RESULT([yes, ${GCCVER}])
			;;
		*)
			AC_MSG_RESULT([no, ${GCCVER}])
			;;
	esac
	AC_MSG_CHECKING(whether GCC supports -no-cpp-precomp)
	case $(${CC} -no-cpp-precomp 2>&1) in
		*-no-cpp-precomp*)
			AC_MSG_RESULT(no)
			;;
		*)
			CFLAGS="${CFLAGS} -no-cpp-precomp"
			AC_MSG_RESULT(yes)
			;;
	esac
else
	AC_MSG_CHECKING(whether we are using the Intel C compiler)
	if ${CC} -V 2>&1 | head -n 1 | grep -q "Intel(R)"; then
		AC_MSG_RESULT(yes)
		WFLAGS='${WFLAGS_ICC}'
		ac_compiler_intel="yes"
	else
		AC_MSG_RESULT(no)
		ac_compiler_intel="no"
	fi
fi
AC_SUBST(WFLAGS)
AC_SUBST(WFLAGS_29X)
AC_SUBST(WFLAGS_3X)

AC_PROG_CPP
AC_PROG_GCC_TRADITIONAL

AC_CACHE_CHECK([for __VA_ARGS__], [ac_cv_va_args],
	       [ac_cv_va_args=no
		AC_COMPILE_IFELSE(AC_LANG_PROGRAM(
			[#include <stdarg.h>
			 #include <stdlib.h>
			 #include <syslog.h>

			 #define vaarg_test(fmt,...) syslog (LOG_DEBUG, fmt, __VA_ARGS__)
			], [vaarg_test ("%d", atoi ("42"));]), [ac_cv_va_args=yes],
			[ac_cv_va_args=no])])
case ${ac_cv_va_args} in
	yes)
		AC_DEFINE(HAVE___VA_ARGS__, 1,
			  [Define if you have __VA_ARGS__])
		;;
esac

AC_CACHE_CHECK([whether the compiler supports gcc-style variadic macros],
	       [ac_cv_variadic_macros], [ac_cv_variadic_macros=no
		AC_COMPILE_IFELSE(AC_LANG_PROGRAM(
		[[#include <stdarg.h>
		  #include <stdlib.h>
		  #include <syslog.h>

		  #define gcc_vaarg_test(fmt,args...) syslog (LOG_DEBUG, fmt , ## args)
		]], [[ gcc_vaarg_test ("%d", atoi ("42")); ]]), [ac_cv_variadic_macros="yes"],
		[ac_cv_variadic_macros="no"])])
case ${ac_cv_variadic_macros} in
	yes)
		AC_DEFINE_UNQUOTED(HAVE_VARIADIC_MACROS, 1,
			Define this if you have GCC-style variadic macros)
		;;
esac

])
