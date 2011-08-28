lib_LTLIBRARIES = \
  libogmrip-encode-gtk.la

libogmrip_encode_gtk_la_SOURCES =    \
  ogmrip-audio-chooser-widget.c      \
  ogmrip-audio-file-chooser-dialog.c \
  ogmrip-audio-options.c             \
  ogmrip-audio-options-dialog.c      \
  ogmrip-chapter-list.c              \
  ogmrip-encoding-manager-dialog.c   \
  ogmrip-file-chooser-dialog.c       \
  ogmrip-helper.c                    \
  ogmrip-list-item.c                 \
  ogmrip-options-plugin.c            \
  ogmrip-profile-editor-dialog.c     \
  ogmrip-profile-manager-dialog.c    \
  ogmrip-progress-dialog.c           \
  ogmrip-source-chooser.c            \
  ogmrip-source-chooser-widget.c     \
  ogmrip-subp-chooser-widget.c       \
  ogmrip-subp-file-chooser-dialog.c  \
  ogmrip-subp-options.c              \
  ogmrip-subp-options-dialog.c

libogmrip_encode_gtk_ladir = \
  $(includedir)/ogmrip-encode

libogmrip_encode_gtk_la_HEADERS =    \
  ogmrip-audio-chooser-widget.h      \
  ogmrip-audio-file-chooser-dialog.h \
  ogmrip-audio-options.h             \
  ogmrip-audio-options-dialog.h      \
  ogmrip-chapter-list.h              \
  ogmrip-encoding-manager-dialog.h   \
  ogmrip-file-chooser-dialog.h       \
  ogmrip-helper.h                    \
  ogmrip-list-item.h                 \
  ogmrip-options-plugin.h            \
  ogmrip-profile-editor-dialog.h     \
  ogmrip-profile-manager-dialog.h    \
  ogmrip-progress-dialog.h           \
  ogmrip-source-chooser.h            \
  ogmrip-source-chooser-widget.h     \
  ogmrip-subp-chooser-widget.h       \
  ogmrip-subp-file-chooser-dialog.h  \
  ogmrip-subp-options.h              \
  ogmrip-subp-options-dialog.h       \
  ogmrip-gtk.h

libogmrip_encode_gtk_la_LDFLAGS = \
  -version-info $(OGMRIP_GTK_LT_VERSION)

libogmrip_encode_gtk_la_LIBADD = \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la \
  $(top_builddir)/libogmrip-dvd-gtk/libogmrip-dvd-gtk.la  \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la

INCLUDES = \
  -I$(top_srcdir)/libogmrip-media    \
  -I$(top_srcdir)/libogmrip-dvd      \
  -I$(top_srcdir)/libogmrip-dvd-gtk  \
  -I$(top_srcdir)/libogmrip-job      \
  -I$(top_srcdir)/libogmrip-encode   \
  -DOGMRIP_LIB_DIR=\""$(libdir)"\"   \
  -DOGMRIP_DATA_DIR=\""$(datadir)"\"

CLEANFILES = \
  $(BUILT_SOURCES)

