lib_LTLIBRARIES = \
  libogmrip-bluray.la

libogmrip_bluray_la_SOURCES = \
  ogmrip-bluray-audio.c   \
  ogmrip-bluray-disc.c    \
  ogmrip-bluray-subp.c    \
  ogmrip-bluray-title.c   \
  ogmrip-bluray-makemkv.c

libogmrip_bluray_la_HEADERS = \
  ogmrip-bluray-audio.h   \
  ogmrip-bluray-disc.h    \
  ogmrip-bluray-subp.h    \
  ogmrip-bluray-title.h   \
  ogmrip-bluray-makemkv.h \
  ogmrip-bluray.h

libogmrip_bluray_la_CFLAGS = \
  $(WARN_CFLAGS)

libogmrip_bluray_la_LDFLAGS = \
  -version-info $(OGMRIP_MEDIA_LT_VERSION) \
  $(WARN_LDFLAGS)

libogmrip_bluray_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la   \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la

libogmrip_bluray_ladir = \
  $(includedir)/ogmrip-bluray

EXTRA_DIST = \
  ogmrip-bluray-priv.h

libogmrip_bluray_la_CPPFLAGS = \
  -I$(top_srcdir)/libogmrip-base  \
  -I$(top_srcdir)/libogmrip-media \
  -I$(top_srcdir)/libogmrip-job

MAINTAINERCLEANFILES = \
  Makefile.in
