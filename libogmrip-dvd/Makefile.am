lib_LTLIBRARIES = \
  libogmrip-dvd.la

libogmrip_dvd_la_SOURCES = \
  ogmdvd-audio.c   \
  ogmdvd-contrib.c \
  ogmdvd-disc.c    \
  ogmdvd-parser.c  \
  ogmdvd-priv.c    \
  ogmdvd-reader.c  \
  ogmdvd-subp.c    \
  ogmdvd-title.c

libogmrip_dvd_la_HEADERS = \
  ogmdvd-audio.h  \
  ogmdvd-disc.h   \
  ogmdvd-parser.h \
  ogmdvd-reader.h \
  ogmdvd-subp.h   \
  ogmdvd-title.h  \
  ogmrip-dvd.h

libogmrip_dvd_la_LDFLAGS = \
  -version-info $(OGMRIP_MEDIA_LT_VERSION)

libogmrip_dvd_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la   \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la

libogmrip_dvd_ladir = \
  $(includedir)/ogmrip-dvd

EXTRA_DIST = \
  ogmdvd-contrib.h   \
  ogmdvd-priv.h

INCLUDES = \
  -I$(top_srcdir)/libogmrip-base  \
  -I$(top_srcdir)/libogmrip-media \
  -I$(top_srcdir)/libogmrip-job

if MAINTAINER_MODE
INCLUDES += \
  -DG_ENABLE_DEBUG
endif

