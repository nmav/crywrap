# arch-tag: 8ad01d06-02d4-4a37-ac1f-781c7a279376

AC_DEFUN([BHM4_SOCKADDR_STORAGE_CHECK], [

AC_CACHE_CHECK([for struct sockaddr_storage],
	       [ac_cv_struct_sockaddr_storage],
	       [ac_cv_struct_sockaddr_storage=no
		AC_COMPILE_IFELSE(AC_LANG_PROGRAM(
			[#include <sys/types.h>
			 #include <sys/socket.h>
			],
			[[  struct sockaddr_storage x;]]),
			[ac_cv_struct_sockaddr_storage=yes],
			[ac_cv_struct_sockaddr_storage=no])])
case ${ac_cv_struct_sockaddr_storage} in
	yes)
		AC_DEFINE(HAVE_SOCKADDR_STORAGE, 1,
			  [Define if you have struct sockaddr_storage])
		;;
esac

])
