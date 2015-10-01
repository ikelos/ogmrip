lib_LTLIBRARIES = \
  libogmrip-media-gtk.la

libogmrip_media_gtk_la_SOURCES =   \
  ogmrip-chapter-store.c           \
  ogmrip-chapter-view.c            \
  ogmrip-error-dialog.c            \
  ogmrip-language-chooser-widget.c \
  ogmrip-language-store.c          \
  ogmrip-media-chooser.c           \
  ogmrip-media-chooser-dialog.c    \
  ogmrip-media-chooser-widget.c    \
  ogmrip-source-chooser.c          \
  ogmrip-title-chooser.c           \
  ogmrip-title-chooser-widget.c

resourcefiles = \
  ogmrip-media.gresource.xml

ogmrip_RESOURCES = \
  $(resourcefiles:.gresource.xml=-gresource.c)

libogmrip_media_gtk_la_SOURCES += \
  $(ogmrip_RESOURCES)

libogmrip_media_gtk_ladir = \
  $(includedir)/ogmrip-media

libogmrip_media_gtk_la_HEADERS =   \
  ogmrip-chapter-store.h           \
  ogmrip-chapter-view.h            \
  ogmrip-error-dialog.h            \
  ogmrip-language-chooser-widget.h \
  ogmrip-language-store.h          \
  ogmrip-media-chooser.h           \
  ogmrip-media-chooser-dialog.h    \
  ogmrip-media-chooser-widget.h    \
  ogmrip-source-chooser.h          \
  ogmrip-title-chooser.h           \
  ogmrip-title-chooser-widget.h    \
  ogmrip-media-gtk.h

libogmrip_media_gtk_la_CPPFLAGS = \
  -I$(top_srcdir)/libogmrip-base \
  -I$(top_srcdir)/libogmrip-media

libogmrip_media_gtk_la_CFLAGS = \
  $(WARN_CFLAGS)

libogmrip_media_gtk_la_LDFLAGS = \
  -version-info $(OGMRIP_MEDIA_LT_VERSION) \
  $(WARN_LDFLAGS)

libogmrip_media_gtk_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-media/libogmrip-media.la

BUILT_SOURCES = \
  $(ogmrip_RESOURCES)

ui_FILES = \
  ogmrip-media-chooser-dialog.ui

ogmrip-media-gresource.c: ogmrip-media.gresource.xml $(ui_FILES) $(GLIB_COMPILE_RESOURCES)
	$(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/libogmrip-media-gtk --generate-source --target=$@ $<

CLEANFILES = \
  $(BUILT_SOURCES)

MAINTAINERCLEANFILES = \
  Makefile.in

EXTRA_DIST =  \
  $(ui_FILES) \
  $(resourcefiles)

