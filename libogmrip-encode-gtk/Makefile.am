lib_LTLIBRARIES = \
  libogmrip-encode-gtk.la

libogmrip_encode_gtk_la_SOURCES =    \
  ogmrip-audio-chooser-widget.c      \
  ogmrip-audio-options.c             \
  ogmrip-audio-options-dialog.c      \
  ogmrip-encoding-manager-dialog.c   \
  ogmrip-list-item.c                 \
  ogmrip-options-editable.c          \
  ogmrip-profile-chooser-widget.c    \
  ogmrip-profile-editor-dialog.c     \
  ogmrip-profile-manager-dialog.c    \
  ogmrip-profile-store.c             \
  ogmrip-profile-view.c              \
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
  ogmrip-profile-chooser-widget.h    \
  ogmrip-profile-editor-dialog.h     \
  ogmrip-profile-manager-dialog.h    \
  ogmrip-profile-store.h             \
  ogmrip-profile-view.h              \
  ogmrip-progress-dialog.h           \
  ogmrip-subp-chooser-widget.h       \
  ogmrip-subp-options.h              \
  ogmrip-subp-options-dialog.h       \
  ogmrip-type-store.h                \
  ogmrip-encode-gtk.h

libogmrip_encode_gtk_la_CPPFLAGS = \
  -I$(top_srcdir)/libogmrip-base      \
  -I$(top_srcdir)/libogmrip-job       \
  -I$(top_srcdir)/libogmrip-media     \
  -I$(top_srcdir)/libogmrip-media-gtk \
  -I$(top_srcdir)/libogmrip-file      \
  -I$(top_srcdir)/libogmrip-file-gtk  \
  -I$(top_srcdir)/libogmrip-dvd       \
  -I$(top_srcdir)/libogmrip-encode    \
  -I$(top_builddir)/libogmrip-encode  \
  -DOGMRIP_LIB_DIR=\""$(libdir)"\"    \
  -DOGMRIP_DATA_DIR=\""$(datadir)"\"

libogmrip_encode_gtk_la_CFLAGS = \
  $(WARN_CFLAGS)

libogmrip_encode_gtk_la_LDFLAGS = \
  -version-info $(OGMRIP_ENCODE_LT_VERSION) \
  $(WARN_LDFLAGS)

libogmrip_encode_gtk_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la           \
  $(top_builddir)/libogmrip-media/libogmrip-media.la         \
  $(top_builddir)/libogmrip-media-gtk/libogmrip-media-gtk.la \
  $(top_builddir)/libogmrip-file/libogmrip-file.la           \
  $(top_builddir)/libogmrip-file-gtk/libogmrip-file-gtk.la   \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la             \
  $(top_builddir)/libogmrip-job/libogmrip-job.la             \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la

resourcefiles = \
  ogmrip-encode.gresource.xml

ogmrip_RESOURCES = \
  $(resourcefiles:.gresource.xml=-gresource.c)

libogmrip_encode_gtk_la_SOURCES += \
  $(ogmrip_RESOURCES)

BUILT_SOURCES = \
  $(ogmrip_RESOURCES)

ui_FILES = \
  ogmrip-profile-editor-dialog.ui   \
  ogmrip-profile-manager-dialog.ui  \
  ogmrip-profile-manager-menu.ui    \
  ogmrip-progress-dialog.ui         \
  ogmrip-progress-menu.ui           \
  ogmrip-encoding-manager-dialog.ui

ogmrip-encode-gresource.c: ogmrip-encode.gresource.xml $(ui_FILES) $(GLIB_COMPILE_RESOURCES)
	$(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/libogmrip-encode-gtk --generate-source --target=$@ $<

CLEANFILES = \
  $(BUILT_SOURCES)

EXTRA_DIST =  \
  $(ui_FILES) \
  $(resourcefiles) \
  ogmrip-type-chooser-widget.h

MAINTAINERCLEANFILES = \
  Makefile.in
