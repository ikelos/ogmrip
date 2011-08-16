bin_PROGRAMS = \
  ogmrip

ogmrip_SOURCES = \
  ogmrip-crop-dialog.c     \
  ogmrip-settings.c        \
  ogmrip-main.c            \
  ogmrip-options-dialog.c  \
  ogmrip-pref-dialog.c     \
  ogmrip-profiles.c        \
  ogmrip-progress-dialog.c \
  ogmrip-spell-dialog.c    \
  ogmrip-update-dialog.c

EXTRA_DIST = \
  ogmrip-crop-dialog.h     \
  ogmrip-settings.h        \
  ogmrip-options-dialog.h  \
  ogmrip-pref-dialog.h     \
  ogmrip-profiles.h        \
  ogmrip-progress-dialog.h \
  ogmrip-spell-dialog.h    \
  ogmrip-update-dialog.h

ogmrip_LDADD = \
  $(top_builddir)/libogmdvd/libogmdvd.la         \
  $(top_builddir)/libogmdvd-gtk/libogmdvd-gtk.la \
  $(top_builddir)/libogmjob/libogmjob.la         \
  $(top_builddir)/libogmrip/libogmrip.la         \
  $(top_builddir)/libogmrip-gtk/libogmrip-gtk.la

INCLUDES = \
  -I$(top_srcdir)/libogmdvd     \
  -I$(top_srcdir)/libogmdvd-gtk \
  -I$(top_srcdir)/libogmjob     \
  -I$(top_srcdir)/libogmrip     \
  -I$(top_builddir)/libogmrip   \
  -I$(top_srcdir)/libogmrip-gtk \
  -DOGMRIP_DATA_DIR=\""$(datadir)"\"

if MAINTAINER_MODE
INCLUDES += \
  -DG_ENABLE_DEBUG
endif

CLEANFILES = \
  $(BUILT_SOURCES) *~ *.bak

