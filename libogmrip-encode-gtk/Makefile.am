lib_LTLIBRARIES = \
  libogmrip-encode-gtk.la

libogmrip_encode_gtk_la_SOURCES =    \
  ogmrip-audio-chooser-widget.c      \
  ogmrip-audio-options.c             \
  ogmrip-audio-options-dialog.c      \
  ogmrip-encoding-manager-dialog.c   \
  ogmrip-list-item.c                 \
  ogmrip-options-editable.c          \
  ogmrip-profile-editor-dialog.c     \
  ogmrip-profile-manager-dialog.c    \
  ogmrip-profile-store.c             \
  ogmrip-progress-dialog.c           \
  ogmrip-subp-chooser-widget.c       \
  ogmrip-subp-options.c              \
  ogmrip-subp-options-dialog.c       \
  ogmrip-type-chooser-widget.c       \
  ogmrip-type-store.c

libogmrip_encode_gtk_ladir = \
  $(includedir)/ogmrip-encode

libogmrip_encode_gtk_la_HEADERS =    \
  ogmrip-audio-chooser-widget.h      \
  ogmrip-audio-options.h             \
  ogmrip-audio-options-dialog.h      \
  ogmrip-encoding-manager-dialog.h   \
  ogmrip-list-item.h                 \
  ogmrip-options-editable.h          \
  ogmrip-profile-editor-dialog.h     \
  ogmrip-profile-manager-dialog.h    \
  ogmrip-profile-store.h             \
  ogmrip-progress-dialog.h           \
  ogmrip-subp-chooser-widget.h       \
  ogmrip-subp-options.h              \
  ogmrip-subp-options-dialog.h       \
  ogmrip-type-store.h                \
  ogmrip-encode-gtk.h

libogmrip_encode_gtk_la_LDFLAGS = \
  -version-info $(OGMRIP_ENCODE_LT_VERSION)

libogmrip_encode_gtk_la_LIBADD = \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-media-gtk/libogmrip-media-gtk.la \
  $(top_builddir)/libogmrip-file/libogmrip-file.la \
  $(top_builddir)/libogmrip-file-gtk/libogmrip-file-gtk.la \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la

INCLUDES = \
  -I$(top_srcdir)/libogmrip-job       \
  -I$(top_srcdir)/libogmrip-module    \
  -I$(top_srcdir)/libogmrip-media     \
  -I$(top_srcdir)/libogmrip-media-gtk \
  -I$(top_srcdir)/libogmrip-file      \
  -I$(top_srcdir)/libogmrip-file-gtk  \
  -I$(top_srcdir)/libogmrip-dvd       \
  -I$(top_srcdir)/libogmrip-encode    \
  -DOGMRIP_LIB_DIR=\""$(libdir)"\"    \
  -DOGMRIP_DATA_DIR=\""$(datadir)"\"

EXTRA_DIST = \
  ogmrip-type-chooser-widget.h

CLEANFILES = \
  $(BUILT_SOURCES)

