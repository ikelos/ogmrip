lib_LTLIBRARIES =   \
  libogmrip-mplayer.la

libogmrip_mplayer_la_SOURCES = \
  ogmrip-mplayer-commands.c \
  ogmrip-mplayer-lavc.c     \
  ogmrip-mplayer-player.c   \
  ogmrip-mplayer-version.c
  
libogmrip_mplayer_la_LDFLAGS = \
  -version-info $(OGMRIP_MPLAYER_LT_VERSION)

libogmrip_mplayer_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la   \
  $(top_builddir)/libogmrip-job/libogmrip-job.la     \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la

libogmrip_mplayer_ladir = \
  $(includedir)/ogmrip-mplayer

libogmrip_mplayer_la_HEADERS = \
  ogmrip-mplayer-commands.h \
  ogmrip-mplayer-lavc.h     \
  ogmrip-mplayer-player.h   \
  ogmrip-mplayer-version.h  \
  ogmrip-mplayer.h

INCLUDES = \
  -I$(top_srcdir)/libogmrip-base   \
  -I$(top_srcdir)/libogmrip-job    \
  -I$(top_srcdir)/libogmrip-media  \
  -I$(top_srcdir)/libogmrip-file   \
  -I$(top_srcdir)/libogmrip-encode \
  -I$(top_builddir)/libogmrip-encode

if MAINTAINER_MODE
INCLUDES += \
  -DG_ENABLE_DEBUG
endif

