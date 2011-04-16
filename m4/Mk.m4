# arch-tag: ace75c56-7f84-4e88-b2d4-31fada4e6122

AC_DEFUN([BHMK_MAKE_CHECK],[

AC_PROG_MAKE_SET
AC_MSG_CHECKING([whether ${MAKE-make} supports per-target variable appending])
AC_CACHE_VAL(ac_cv_prog_make_target_var_append,
[cat >conftest.make <<\_ACEOF
foo = bar
all: foo += baz
all:
	@echo 'ac_maketemp="${foo}"'
_ACEOF
# GNU make sometimes prints "make[1]: Entering...", which would confuse us.
eval `${MAKE-make} -f conftest.make 2>/dev/null | grep temp=`
if test -n "$ac_maketemp"; then
  eval ac_cv_prog_make_target_var_append=yes
else
  eval ac_cv_prog_make_target_var_append=no
fi
rm -f conftest.make])
if eval "test \"`echo '$ac_cv_prog_make_target_var_append'`\" = yes"; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi
MAKE_TARGET_VAR_APPEND=$ac_cv_prog_make_target_var_append
AC_SUBST([MAKE_TARGET_VAR_APPEND])

])
