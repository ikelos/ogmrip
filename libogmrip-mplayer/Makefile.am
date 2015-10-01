lib_LTLIBRARIES =   \
  libogmrip-mplayer.la

libogmrip_mplayer_la_SOURCES = \
  ogmrip-mplayer-commands.c \
  ogmrip-mplayer-lavc.c     \
  ogmrip-mplayer-version.c
  
libogmrip_mplayer_la_HEADERS = \
  ogmrip-mplayer-commands.h \
  ogmrip-mplayer-lavc.h     \
  ogmrip-mplayer-version.h  \
  ogmrip-mplayer.h

libogmrip_mplayer_ladir = \
  $(includedir)/ogmrip-mplayer

libogmrip_mplayer_la_CPPFLAGS = \
  -I$(top_srcdir)/libogmrip-base   \
  -I$(top_srcdir)/libogmrip-job    \
  -I$(top_srcdir)/libogmrip-media  \
  -I$(top_srcdir)/libogmrip-file   \
  -I$(top_srcdir)/libogmrip-encode \
  -I$(top_builddir)/libogmrip-encode

libogmrip_mplayer_la_CFLAGS = \
  $(WARN_CFLAGS)

libogmrip_mplayer_la_LDFLAGS = \
  -version-info $(OGMRIP_MPLAYER_LT_VERSION) \
  $(WARN_LDFLAGS)

libogmrip_mplayer_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la   \
  $(top_builddir)/libogmrip-job/libogmrip-job.la     \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la

MAINTAINERCLEANFILES = \
  Makefile.in
