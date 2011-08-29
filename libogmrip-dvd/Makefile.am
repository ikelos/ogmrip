lib_LTLIBRARIES = \
  libogmrip-dvd.la

libogmrip_dvd_la_SOURCES = \
  ogmdvd-audio.c     \
  ogmdvd-contrib.c   \
  ogmdvd-device.c    \
  ogmdvd-disc.c      \
  ogmdvd-drive.c     \
  ogmdvd-monitor.c   \
  ogmdvd-parser.c    \
  ogmdvd-priv.c      \
  ogmdvd-reader.c    \
  ogmdvd-subp.c      \
  ogmdvd-title.c     \
  ogmdvd-transport.c

libogmrip_dvd_la_HEADERS = \
  ogmdvd-audio.h   \
  ogmdvd-disc.h    \
  ogmdvd-drive.h   \
  ogmdvd-monitor.h \
  ogmdvd-parser.h  \
  ogmdvd-reader.h  \
  ogmdvd-subp.h    \
  ogmdvd-title.h   \
  ogmrip-dvd.h

libogmrip_dvd_la_LDFLAGS = \
  -version-info $(OGMDVD_LT_VERSION)

libogmrip_dvd_la_LIBADD = \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la

libogmrip_dvd_ladir = \
  $(includedir)/ogmrip-dvd

EXTRA_DIST = \
  ogmdvd-contrib.h   \
  ogmdvd-device.h    \
  ogmdvd-transport.h \
  ogmdvd-priv.h

INCLUDES = \
  -I$(top_srcdir)/libogmrip-media \
  -I$(top_srcdir)/libogmrip-job
