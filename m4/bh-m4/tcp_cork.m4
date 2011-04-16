# arch-tag: 3416a800-dad6-4fad-8c25-b599962200e7

AC_DEFUN([BHM4_TCP_CORK_CHECK],[

AC_CHECK_DECL([TCP_CORK],
[AC_DEFINE(HAVE_DECL_TCP_CORK, 1,
[Define this if you have TCP_CORK])], [],
[#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])

])
