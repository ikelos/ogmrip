lib_LTLIBRARIES = \
  libogmrip-base.la

libogmrip_base_la_SOURCES = \
  ogmrip-fs.c    \
  ogmrip-lang.c  \
  ogmrip-log.c   \
  ogmrip-type.c  \
  ogmrip-utils.c \
  ogmrip-xml.c

libogmrip_base_ladir = \
  $(includedir)/ogmrip-base

libogmrip_base_la_HEADERS = \
  ogmrip-fs.h    \
  ogmrip-lang.h  \
  ogmrip-log.h   \
  ogmrip-type.h  \
  ogmrip-utils.h \
  ogmrip-xml.h   \
  ogmrip-base.h

libogmrip_base_la_CFLAGS = \
  $(WARN_CFLAGS)

libogmrip_base_la_LDFLAGS = \
  -version-info $(OGMRIP_BASE_LT_VERSION) \
  $(WARN_LDFLAGS)

MAINTAINERCLEANFILES = \
  Makefile.in
