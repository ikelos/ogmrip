bin_PROGRAMS = \
  ogmrip

ogmrip_SOURCES = \
  ogmrip-crop-dialog.c     \
  ogmrip-settings.c        \
  ogmrip-main.c            \
  ogmrip-options-dialog.c  \
  ogmrip-pref-dialog.c     \
  ogmrip-profiles.c        \
  ogmrip-spell-dialog.c    \
  ogmrip-update-dialog.c

EXTRA_DIST = \
  ogmrip-crop-dialog.h     \
  ogmrip-settings.h        \
  ogmrip-options-dialog.h  \
  ogmrip-pref-dialog.h     \
  ogmrip-profiles.h        \
  ogmrip-spell-dialog.h    \
  ogmrip-update-dialog.h

ogmrip_LDADD = \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la         \
  $(top_builddir)/libogmrip-dvd-gtk/libogmrip-dvd-gtk.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la         \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la   \
  $(top_builddir)/libogmrip-encode-gtk/libogmrip-encode-gtk.la

INCLUDES = \
  -I$(top_srcdir)/libogmrip-dvd        \
  -I$(top_srcdir)/libogmrip-dvd-gtk    \
  -I$(top_srcdir)/libogmrip-job        \
  -I$(top_srcdir)/libogmrip-encode     \
  -I$(top_srcdir)/libogmrip-encode-gtk \
  -I$(top_builddir)/libogmrip-encode   \
  -DOGMRIP_DATA_DIR=\""$(datadir)"\"

if MAINTAINER_MODE
INCLUDES += \
  -DG_ENABLE_DEBUG
endif

CLEANFILES = \
  $(BUILT_SOURCES) *~ *.bak

