lib_LTLIBRARIES = \
  libogmrip-media.la

libogmrip_media_la_SOURCES = \
  ogmrip-media-audio.c    \
  ogmrip-media-backend.c  \
  ogmrip-media-chapters.c \
  ogmrip-media-labels.c   \
  ogmrip-media-object.c   \
  ogmrip-media-stream.c   \
  ogmrip-media-subp.c     \
  ogmrip-media-time.c     \
  ogmrip-media-title.c    \
  ogmrip-media-video.c

libogmrip_media_la_HEADERS = \
  ogmrip-media-audio.h    \
  ogmrip-media-backend.h  \
  ogmrip-media-chapters.h \
  ogmrip-media-enums.h    \
  ogmrip-media-labels.h   \
  ogmrip-media-object.h   \
  ogmrip-media-stream.h   \
  ogmrip-media-subp.h     \
  ogmrip-media-time.h     \
  ogmrip-media-title.h    \
  ogmrip-media-types.h    \
  ogmrip-media-video.h    \
  ogmrip-media.h

libogmrip_media_la_LDFLAGS = \
  -version-info $(OGMRIP_MEDIA_LT_VERSION)

libogmrip_media_ladir = \
  $(includedir)/ogmrip-media

libogmrip_media_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la

INCLUDES = \
  -I$(top_srcdir)/libogmrip-base \
  -I$(top_srcdir)/libogmrip-job

