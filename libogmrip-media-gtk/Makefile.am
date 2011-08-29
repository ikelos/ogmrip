lib_LTLIBRARIES = \
  libogmrip-media-gtk.la

libogmrip_media_gtk_la_SOURCES = \
  ogmrip-chapter-store.c         \
  ogmrip-title-chooser.c         \
  ogmrip-title-chooser-widget.c

libogmrip_media_gtk_ladir = \
  $(includedir)/ogmrip-media

libogmrip_media_gtk_la_HEADERS = \
  ogmrip-chapter-store.h         \
  ogmrip-title-chooser.h         \
  ogmrip-title-chooser-widget.h  \
  ogmrip-media-gtk.h

libogmrip_media_gtk_la_LDFLAGS = \
  -version-info $(OGMDVD_GTK_LT_VERSION)

libogmrip_media_gtk_la_LIBADD = \
  $(top_builddir)/libogmrip-media/libogmrip-media.la

INCLUDES = \
  -I$(top_srcdir)/libogmrip-media

