lib_LTLIBRARIES = \
  libogmrip-gtk.la

libogmrip_gtk_la_SOURCES =           \
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

libogmrip_gtk_ladir = \
  $(includedir)/ogmrip

libogmrip_gtk_la_HEADERS =           \
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

libogmrip_gtk_la_LDFLAGS = \
  -version-info $(OGMRIP_GTK_LT_VERSION)

libogmrip_gtk_la_LIBADD = \
  $(top_builddir)/libogmdvd/libogmdvd.la \
  $(top_builddir)/libogmdvd-gtk/libogmdvd-gtk.la  \
  $(top_builddir)/libogmjob/libogmjob.la \
  $(top_builddir)/libogmrip/libogmrip.la

INCLUDES = \
  -I$(top_srcdir)/libogmdvd      \
  -I$(top_srcdir)/libogmdvd-gtk  \
  -I$(top_srcdir)/libogmjob      \
  -I$(top_srcdir)/libogmrip      \
  -DOGMRIP_LIB_DIR=\""$(libdir)"\"   \
  -DOGMRIP_DATA_DIR=\""$(datadir)"\"

CLEANFILES = \
  $(BUILT_SOURCES)

