lib_LTLIBRARIES = \
  libogmrip-file.la

libogmrip_file_la_SOURCES = \
  ogmrip-audio-file.c  \
  ogmrip-file-object.c \
  ogmrip-file-priv.c   \
  ogmrip-media-file.c  \
  ogmrip-media-info.c  \
  ogmrip-subp-file.c   \
  ogmrip-video-file.c

libogmrip_file_la_HEADERS = \
  ogmrip-audio-file.h  \
  ogmrip-file-object.h \
  ogmrip-media-file.h  \
  ogmrip-media-info.h  \
  ogmrip-subp-file.h   \
  ogmrip-video-file.h  \
  ogmrip-file.h

libogmrip_file_la_CPPFLAGS = \
  -I$(top_srcdir)/libogmrip-base \
  -I$(top_srcdir)/libogmrip-media

libogmrip_file_la_CFLAGS = \
  $(WARN_CFLAGS)

libogmrip_file_la_LDFLAGS = \
  -version-info $(OGMRIP_MEDIA_LT_VERSION) \
  $(WARN_LDFLAGS)

libogmrip_file_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-media/libogmrip-media.la

libogmrip_file_ladir = \
  $(includedir)/ogmrip-file

EXTRA_DIST = \
  ogmrip-file-priv.h

MAINTAINERCLEANFILES = \
  Makefile.in
