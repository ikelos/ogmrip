lib_LTLIBRARIES = \
  libogmrip-file-gtk.la

libogmrip_file_gtk_la_SOURCES = \
  ogmrip-audio-file-chooser-dialog.c \
  ogmrip-file-chooser-dialog.c       \
  ogmrip-source-chooser-widget.c     \
  ogmrip-subp-file-chooser-dialog.c

libogmrip_file_gtk_ladir = \
  $(includedir)/ogmrip-file

libogmrip_file_gtk_la_HEADERS = \
  ogmrip-audio-file-chooser-dialog.h \
  ogmrip-file-chooser-dialog.h       \
  ogmrip-source-chooser-widget.h     \
  ogmrip-subp-file-chooser-dialog.h  \
  ogmrip-file-gtk.h

libogmrip_file_gtk_la_LDFLAGS = \
  -version-info $(OGMRIP_MEDIA_LT_VERSION)

libogmrip_file_gtk_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-file/libogmrip-file.la \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-media-gtk/libogmrip-media-gtk.la

INCLUDES = \
  -I$(top_srcdir)/libogmrip-base      \
  -I$(top_srcdir)/libogmrip-media     \
  -I$(top_srcdir)/libogmrip-media-gtk \
  -I$(top_srcdir)/libogmrip-file      \
  -DOGMRIP_LIB_DIR=\""$(libdir)"\"    \
  -DOGMRIP_DATA_DIR=\""$(datadir)"\"

CLEANFILES = \
  $(BUILT_SOURCES)

