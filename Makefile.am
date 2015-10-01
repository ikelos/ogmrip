ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = \
  libogmrip-base    \
  libogmrip-job     \
  libogmrip-media   \
  libogmrip-module  \
  libogmrip-dvd     \
  libogmrip-file    \
  libogmrip-encode  \
  libogmrip-mplayer \
  avibox \
  dvdcpy

if HAVE_BLURAY_SUPPORT
SUBDIRS += \
  libogmrip-bluray
endif

if HAVE_GTK_SUPPORT
SUBDIRS += \
  libogmrip-media-gtk  \
  libogmrip-file-gtk   \
  libogmrip-encode-gtk
endif

if HAVE_THEORA_SUPPORT
SUBDIRS += \
  theoraenc
endif

if HAVE_SRT_SUPPORT
SUBDIRS += \
  subrip
endif

SUBDIRS += \
  plugins \
  data \
  src \
  po

EXTRA_DIST= \
  intltool-update.in  \
  intltool-extract.in \
  intltool-merge.in   \
  po/Makefile.in.in   \
  po/POTFILES.in

DISTCLEANFILES = \
  intltool-update  \
  intltool-extract \
  intltool-merge

MAINTAINERCLEANFILES = \
  aclocal.m4 \
  compile \
  config.guess \
  config.h.in \
  config.h.in~ \
  config.rpath \
  config.sub \
  configure \
  depcomp \
  gtk-doc.make \
  install-sh \
  intltool-extract.in \
  intltool-merge.in \
  intltool-update.in \
  ltmain.sh \
  missing \
  Makefile.in

DISTCHECK_CONFIGURE_FLAGS = \
  --enable-gtk-doc=yes
