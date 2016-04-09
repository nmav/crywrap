## toplevel.mk -- Rules for a top-level makefile	-*- Makefile -*-
## arch-tag: a2fd49a3-e42f-4f04-b9e7-b1d8d67c19e6

DISTCLEANFILES	+= config.cache config.log autom4te.cache config.h \
		Makefile config.status config.mk Mk/Rules.mk
MAINTCLEANFILES	+= ${top_srcdir}/configure ${top_srcdir}/config.h.in \
		${top_srcdir}/autom4te.cache ${top_srcdir}/aclocal.m4
EXTRA_DIST	+= build-aux/config.guess build-aux/config.sub config.mk.in configure \
		configure.ac build-aux/install-sh config.h.in

doxy: WARNUNDOC=NO
doxy: DOT=NO
doxy:
	${quiet} ${P_SCRIPT} Doxygen docs
	${quiet} cd ${top_srcdir} && \
		(cat ${PACKAGE}.doxy | grep -v "OUTPUT_DIRECTORY"; \
		 echo PROJECT_NUMBER="${VERSION} ${EXTRA_VERSION}"; \
		 echo OUTPUT_DIRECTORY="${CURDIR}/doxygen"; \
		 echo WARN_IF_UNDOCUMENTED="${WARNUNDOC}"; \
		 echo HAVE_DOT="${DOT}";) | doxygen -

ifeq (${VERBOSE},2)
dist: _target=(dist)
endif
dist: info dist-pre distdir dist-post
dist-pre:
	${quiet} test -z "${distdir}" || rm -rf "${distdir}"
dist-post:
	${quiet} ${P_TARBALL} ${PACKAGE}-${VERSION}.tar.gz
	${quiet} ${TAR} ${TAR_OPTIONS} -cf - ${PACKAGE}-${VERSION} | \
		${GZIP} ${GZIP_ENV} >${PACKAGE}-${VERSION}.tar.gz
	${quiet} test -z "${distdir}" || rm -rf "${distdir}"

distcheck: dist
	${quiet} ${ECHO} "Checking distribution"
	${quiet} ${GZIP} -dfc ${PACKAGE}-${VERSION}.tar.gz | tar ${TAR_OPTIONS} -xf -
	${quiet} cd ${PACKAGE}-${VERSION} && ./configure
	${quiet} ${MAKE} -C ${PACKAGE}-${VERSION} DEFDESCEND=1 \
		all install dist DESTDIR=$(shell pwd)/${PACKAGE}-${VERSION}
	${quiet} rm -rf ${PACKAGE}-${VERSION}
	@banner="${PACKAGE}-${VERSION}.tar.gz is ready for distribution." ;\
	dashes=`echo "$$banner" | sed -e s/./=/g`; \
	echo "$$dashes" ;\
	echo "$$banner" ;\
	echo "$$dashes"

distsig: dist
	${quiet} ${P_SIG} ${PACKAGE}-${VERSION}.tar.gz
	${quiet} md5sum ${PACKAGE}-${VERSION}.tar.gz | gpg --clearsign \
	       >${PACKAGE}-${VERSION}.tar.gz.sig

.PHONY: help
help:
	@echo 'Cleaning targets:'
	@echo '  clean            - remove most generated files but keep the config'
	@echo '  distclean        - remove even more generated files, even the config'
	@echo '  maintainer-clean - remove all generated files and the config'
	@echo ''
	@echo 'Other targets:'
	@echo '  all              - Build all default targets'
	@echo "  install          - Install ${PACKAGE}"
	@echo "  uninstall        - Uninstall ${PACKAGE}"
	@echo ''
	@echo 'Developer targets:'
	@echo '  dir/             - Build all files in dir and below'
	@echo '  dir/file         - Build specified target only'
	@echo '  TAGS             - Generate tags file for editors'
	@echo  ''
	@echo  '  make VERBOSE=0|1 [targets] 0 => quiet build (default), 1 => verbose build'
	@echo  ''
	@echo  'Execute "make" or "make all" to build all targets.'
