lib_LTLIBRARIES = \
  libogmrip-bluray.la

libogmrip_bluray_la_SOURCES = \
  ogmrip-bluray-audio.c   \
  ogmrip-bluray-disc.c    \
  ogmrip-bluray-makemkv.c \
  ogmrip-bluray-subp.c    \
  ogmrip-bluray-title.c

libogmrip_bluray_la_HEADERS = \
  ogmrip-bluray-audio.h  \
  ogmrip-bluray-disc.h   \
  ogmrip-bluray-subp.h   \
  ogmrip-bluray-title.h  \
  ogmrip-bluray.h

libogmrip_bluray_la_LDFLAGS = \
  -version-info $(OGMRIP_MEDIA_LT_VERSION)

libogmrip_bluray_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la   \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la

libogmrip_bluray_ladir = \
  $(includedir)/ogmrip-bluray

EXTRA_DIST = \
  ogmrip-bluray-makemkv.h \
  ogmrip-bluray-priv.h

INCLUDES = \
  -I$(top_srcdir)/libogmrip-base  \
  -I$(top_srcdir)/libogmrip-media \
  -I$(top_srcdir)/libogmrip-job

