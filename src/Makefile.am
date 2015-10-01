bin_PROGRAMS = \
  ogmrip

ogmrip_SOURCES = \
  ogmrip-application.c \
  ogmrip-cli.c   \
  ogmrip-main.c

resourcefiles =

if HAVE_GTK_SUPPORT
ogmrip_SOURCES += \
  ogmrip-crop-dialog.c    \
  ogmrip-gui.c            \
  ogmrip-helper.c         \
  ogmrip-main-window.c    \
  ogmrip-options-dialog.c \
  ogmrip-pref-dialog.c

if HAVE_ENCHANT_SUPPORT
ogmrip_SOURCES += \
  ogmrip-spell-dialog.c
endif

resourcefiles += \
  ogmrip.gresource.xml
endif

ogmrip_RESOURCES = \
  $(resourcefiles:.gresource.xml=-gresource.c)

ogmrip_SOURCES += \
  $(ogmrip_RESOURCES)

BUILT_SOURCES = \
  $(ogmrip_RESOURCES)

ui_FILES = \
  ogmrip-crop-dialog.ui    \
  ogmrip-main-window.ui    \
  ogmrip-menu.ui           \
  ogmrip-options-dialog.ui \
  ogmrip-pref-dialog.ui    \
  ogmrip-spell-dialog.ui

ogmrip-gresource.c: ogmrip.gresource.xml $(ui_FILES) $(GLIB_COMPILE_RESOURCES)
	$(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/src --generate-source --target=$@ $<

ogmrip_CPPFLAGS = \
  -I$(top_srcdir)/libogmrip-base       \
  -I$(top_srcdir)/libogmrip-job        \
  -I$(top_srcdir)/libogmrip-media      \
  -I$(top_srcdir)/libogmrip-file       \
  -I$(top_srcdir)/libogmrip-dvd        \
  -I$(top_srcdir)/libogmrip-bluray     \
  -I$(top_srcdir)/libogmrip-encode     \
  -I$(top_srcdir)/libogmrip-module     \
  -I$(top_srcdir)/libogmrip-mplayer    \
  -I$(top_builddir)/libogmrip-encode   \
  -DOGMRIP_DATA_DIR=\""$(datadir)"\"   \
  -DOGMRIP_LIB_DIR=\""$(libdir)"\"

if HAVE_GTK_SUPPORT
ogmrip_CPPFLAGS += \
  -I$(top_srcdir)/libogmrip-media-gtk  \
  -I$(top_srcdir)/libogmrip-file-gtk   \
  -I$(top_srcdir)/libogmrip-encode-gtk
endif

ogmrip_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_LDFLAGS = \
  $(WARN_LDFLAGS)

ogmrip_LDADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la             \
  $(top_builddir)/libogmrip-job/libogmrip-job.la               \
  $(top_builddir)/libogmrip-media/libogmrip-media.la           \
  $(top_builddir)/libogmrip-file/libogmrip-file.la             \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la               \
  $(top_builddir)/libogmrip-bluray/libogmrip-bluray.la         \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la         \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la       \
  $(top_builddir)/libogmrip-module/libogmrip-module.la

if HAVE_GTK_SUPPORT
ogmrip_LDADD += \
  $(top_builddir)/libogmrip-media-gtk/libogmrip-media-gtk.la   \
  $(top_builddir)/libogmrip-file-gtk/libogmrip-file-gtk.la     \
  $(top_builddir)/libogmrip-encode-gtk/libogmrip-encode-gtk.la
endif

EXTRA_DIST = \
  $(ui_FILES)             \
  $(resourcefiles)        \
  ogmrip-application.h    \
  ogmrip-cli.h            \
  ogmrip-crop-dialog.h    \
  ogmrip-gui.h            \
  ogmrip-helper.h         \
  ogmrip-main-window.h    \
  ogmrip-options-dialog.h \
  ogmrip-pref-dialog.h    \
  ogmrip-settings.h       \
  ogmrip-spell-dialog.h

CLEANFILES = \
  $(ogmrip_RESOURCES) \
  *~ *.bak

MAINTAINERCLEANFILES = \
  Makefile.in
