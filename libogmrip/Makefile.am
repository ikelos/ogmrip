lib_LTLIBRARIES =   \
  libogmrip.la \
  libogmrip-mplayer.la \
  libogmrip-lavc.la

BUILT_SOURCES = \
  ogmrip-marshal.c \
  ogmrip-marshal.h

libogmrip_la_SOURCES =      \
  ogmrip-audio-codec.c      \
  ogmrip-chapters.c         \
  ogmrip-codec.c            \
  ogmrip-container.c        \
  ogmrip-dvdcpy.c           \
  ogmrip-encoding.c         \
  ogmrip-encoding-manager.c \
  ogmrip-file.c             \
  ogmrip-fs.c               \
  ogmrip-hardsub.c          \
  ogmrip-marshal.c          \
  ogmrip-novideo.c          \
  ogmrip-player.c           \
  ogmrip-plugin.c           \
  ogmrip-profile.c          \
  ogmrip-profile-engine.c   \
  ogmrip-subp-codec.c       \
  ogmrip-version.c          \
  ogmrip-video-codec.c      \
  ogmrip-xml.c

libogmrip_ladir = \
  $(includedir)/ogmrip

libogmrip_la_HEADERS =      \
  ogmrip-audio-codec.h      \
  ogmrip-chapters.h         \
  ogmrip-codec.h            \
  ogmrip-container.h        \
  ogmrip-dvdcpy.h           \
  ogmrip-encoding.h         \
  ogmrip-encoding-manager.h \
  ogmrip-enums.h            \
  ogmrip-file.h             \
  ogmrip-fs.h               \
  ogmrip-hardsub.h          \
  ogmrip-marshal.h          \
  ogmrip-novideo.h          \
  ogmrip-player.h           \
  ogmrip-plugin.h           \
  ogmrip-profile.h          \
  ogmrip-profile-engine.h   \
  ogmrip-profile-keys.h     \
  ogmrip-subp-codec.h       \
  ogmrip-version.h          \
  ogmrip-video-codec.h      \
  ogmrip-xml.h              \
  ogmrip.h

libogmrip_la_LDFLAGS = \
  -version-info $(OGMRIP_LT_VERSION)

libogmrip_la_LIBADD = \
  $(top_builddir)/libogmdvd/libogmdvd.la \
  $(top_builddir)/libogmjob/libogmjob.la

ogmrip-marshal.h: ogmrip-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=ogmrip_cclosure_marshal > $@

ogmrip-marshal.c: ogmrip-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"ogmrip-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=ogmrip_cclosure_marshal >> $@

#
# Common
#

libogmrip_mplayer_la_SOURCES = \
  ogmrip-mplayer.c
  
libogmrip_mplayer_la_LDFLAGS = \
  -version-info $(OGMRIP_LT_VERSION)

libogmrip_mplayer_la_LIBADD = \
  $(top_builddir)/libogmdvd/libogmdvd.la \
  $(top_builddir)/libogmjob/libogmjob.la \
  $(top_builddir)/libogmrip/libogmrip.la

libogmrip_mplayer_ladir = \
  $(includedir)/ogmrip

libogmrip_mplayer_la_HEADERS = \
  ogmrip-mplayer.h

libogmrip_lavc_la_SOURCES = \
  ogmrip-lavc.c
  
libogmrip_lavc_la_LDFLAGS = \
  -version-info $(OGMRIP_LT_VERSION)

libogmrip_lavc_la_LIBADD = \
  $(top_builddir)/libogmdvd/libogmdvd.la \
  $(top_builddir)/libogmjob/libogmjob.la \
  $(top_builddir)/libogmrip/libogmrip.la \
	$(top_builddir)/libogmrip/libogmrip-mplayer.la

libogmrip_lavc_ladir = \
  $(includedir)/ogmrip

libogmrip_lavc_la_HEADERS = \
  ogmrip-lavc.h

EXTRA_DIST = \
  ogmrip-marshal.list \
  ogmrip-version.h.in

INCLUDES = \
  -I$(top_srcdir)/libogmjob \
  -I$(top_srcdir)/libogmdvd \
  -DOGMRIP_LIB_DIR=\""$(libdir)"\" \
  -DOGMRIP_DATA_DIR=\""$(datadir)"\"

if MAINTAINER_MODE
INCLUDES += \
  -DG_ENABLE_DEBUG
endif

ogmripincdir = \
  $(includedir)/ogmrip

ogmripinc_DATA = \
  ogmrip-version.h

CLEANFILES = \
  $(BUILT_SOURCES)

DISTCLEANFILES = \
  ogmrip-version.h

