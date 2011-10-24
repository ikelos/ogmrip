bin_PROGRAMS = \
  ogmrip

ogmrip_SOURCES = \
  ogmrip-application.c    \
  ogmrip-crop-dialog.c    \
  ogmrip-helper.c         \
  ogmrip-main.c           \
  ogmrip-options-dialog.c \
  ogmrip-pref-dialog.c    \
  ogmrip-spell-dialog.c

EXTRA_DIST = \
  ogmrip-application.h    \
  ogmrip-crop-dialog.h    \
  ogmrip-helper.h         \
  ogmrip-options-dialog.h \
  ogmrip-pref-dialog.h    \
  ogmrip-settings.h       \
  ogmrip-spell-dialog.h

ogmrip_LDADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la               \
  $(top_builddir)/libogmrip-media/libogmrip-media.la           \
  $(top_builddir)/libogmrip-media-gtk/libogmrip-media-gtk.la   \
  $(top_builddir)/libogmrip-file/libogmrip-file.la             \
  $(top_builddir)/libogmrip-file-gtk/libogmrip-file-gtk.la     \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la               \
  $(top_builddir)/libogmrip-drive/libogmrip-drive.la           \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la         \
  $(top_builddir)/libogmrip-encode-gtk/libogmrip-encode-gtk.la \
  $(top_builddir)/libogmrip-module/libogmrip-module.la

INCLUDES = \
  -I$(top_srcdir)/libogmrip-job        \
  -I$(top_srcdir)/libogmrip-media      \
  -I$(top_srcdir)/libogmrip-media-gtk  \
  -I$(top_srcdir)/libogmrip-file       \
  -I$(top_srcdir)/libogmrip-file-gtk   \
  -I$(top_srcdir)/libogmrip-dvd        \
  -I$(top_srcdir)/libogmrip-drive      \
  -I$(top_srcdir)/libogmrip-encode     \
  -I$(top_srcdir)/libogmrip-encode-gtk \
  -I$(top_srcdir)/libogmrip-module     \
  -I$(top_builddir)/libogmrip-encode   \
  -DOGMRIP_DATA_DIR=\""$(datadir)"\"   \
  -DOGMRIP_LIB_DIR=\""$(libdir)"\"

if MAINTAINER_MODE
INCLUDES += \
  -DG_ENABLE_DEBUG
endif

CLEANFILES = \
  $(BUILT_SOURCES) *~ *.bak

