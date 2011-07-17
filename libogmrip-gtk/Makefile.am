lib_LTLIBRARIES = \
  libogmrip-gtk.la

libogmrip_gtk_la_SOURCES =       \
  ogmrip-chapter-list.c          \
  ogmrip-chooser-list.c          \
  ogmrip-helper.c                \
  ogmrip-options-plugin.c        \
  ogmrip-source-chooser.c        \
  ogmrip-source-chooser-widget.c

libogmrip_gtk_ladir = \
  $(includedir)/ogmrip

libogmrip_gtk_la_HEADERS =       \
  ogmrip-chapter-list.h          \
  ogmrip-chooser-list.h          \
  ogmrip-helper.h                \
  ogmrip-options-plugin.h        \
  ogmrip-source-chooser.h        \
  ogmrip-source-chooser-widget.h \
  ogmrip-gtk.h

libogmrip_gtk_la_LDFLAGS = \
  -version-info $(OGMRIP_GTK_LT_VERSION)

libogmrip_gtk_la_LIBADD = \
  $(top_builddir)/libogmdvd/libogmdvd.la \
  $(top_builddir)/libogmdvd-gtk/libogmdvd-gtk.la  \
  $(top_builddir)/libogmjob/libogmjob.la \
  $(top_builddir)/libogmrip/libogmrip.la

#
# Misc
#

INCLUDES = \
  -I$(top_srcdir)/libogmdvd      \
  -I$(top_srcdir)/libogmdvd-gtk  \
  -I$(top_srcdir)/libogmjob      \
  -I$(top_srcdir)/libogmrip      \
  -DOGMRIP_LIB_DIR=\""$(libdir)"\"   \
  -DOGMRIP_DATA_DIR=\""$(datadir)"\"

CLEANFILES = \
  $(BUILT_SOURCES)

