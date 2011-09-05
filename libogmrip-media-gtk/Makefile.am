lib_LTLIBRARIES = \
  libogmrip-media-gtk.la

libogmrip_media_gtk_la_SOURCES =   \
  ogmrip-chapter-store.c           \
  ogmrip-chapter-view.c            \
  ogmrip-error-dialog.c            \
  ogmrip-language-chooser-widget.c \
  ogmrip-language-store.c          \
  ogmrip-source-chooser.c          \
  ogmrip-title-chooser.c           \
  ogmrip-title-chooser-widget.c

libogmrip_media_gtk_ladir = \
  $(includedir)/ogmrip-media

libogmrip_media_gtk_la_HEADERS =   \
  ogmrip-chapter-store.h           \
  ogmrip-chapter-view.h            \
  ogmrip-error-dialog.h            \
  ogmrip-language-chooser-widget.h \
  ogmrip-language-store.h          \
  ogmrip-source-chooser.h          \
  ogmrip-title-chooser.h           \
  ogmrip-title-chooser-widget.h    \
  ogmrip-media-gtk.h

libogmrip_media_gtk_la_LDFLAGS = \
  -version-info $(OGMDVD_GTK_LT_VERSION)

libogmrip_media_gtk_la_LIBADD = \
  $(top_builddir)/libogmrip-media/libogmrip-media.la

INCLUDES = \
  -I$(top_srcdir)/libogmrip-media

