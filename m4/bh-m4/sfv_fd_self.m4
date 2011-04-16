# arch-tag: 8f8683bf-e264-44c6-9610-c0266826ccb7

AC_DEFUN([BHM4_SFV_FD_SELF_CHECK], [

AC_CHECK_DECL([SFV_FD_SELF],
[AC_DEFINE(HAVE_DECL_SFV_FD_SELF, 1,
[Define this if you have SFV_FD_SELF])], [],
[#if HAVE_SYS_SENDFILE_H
#include <sys/sendfile.h>
#endif
])

])
