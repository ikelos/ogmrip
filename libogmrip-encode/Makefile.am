lib_LTLIBRARIES =   \
  libogmrip-encode.la

BUILT_SOURCES = \
  ogmrip-marshal.c \
  ogmrip-marshal.h

libogmrip_encode_la_SOURCES = \
  ogmrip-analyze.c          \
  ogmrip-audio-codec.c      \
  ogmrip-chapters.c         \
  ogmrip-codec.c            \
  ogmrip-configurable.c     \
  ogmrip-container.c        \
  ogmrip-copy.c             \
  ogmrip-encoding.c         \
  ogmrip-encoding-manager.c \
  ogmrip-encoding-parser.c  \
  ogmrip-hardsub.c          \
  ogmrip-marshal.c          \
  ogmrip-novideo.c          \
  ogmrip-plugin.c           \
  ogmrip-profile.c          \
  ogmrip-profile-engine.c   \
  ogmrip-profile-parser.c   \
  ogmrip-stub.c             \
  ogmrip-subp-codec.c       \
  ogmrip-test.c             \
  ogmrip-video-codec.c

libogmrip_encode_ladir = \
  $(includedir)/ogmrip-encode

libogmrip_encode_la_HEADERS = \
  ogmrip-analyze.h          \
  ogmrip-audio-codec.h      \
  ogmrip-chapters.h         \
  ogmrip-codec.h            \
  ogmrip-configurable.h     \
  ogmrip-container.h        \
  ogmrip-copy.h             \
  ogmrip-encoding.h         \
  ogmrip-encoding-manager.h \
  ogmrip-encoding-parser.h  \
  ogmrip-enums.h            \
  ogmrip-hardsub.h          \
  ogmrip-marshal.h          \
  ogmrip-novideo.h          \
  ogmrip-profile.h          \
  ogmrip-profile-engine.h   \
  ogmrip-profile-keys.h     \
  ogmrip-profile-parser.h   \
  ogmrip-stub.h             \
  ogmrip-subp-codec.h       \
  ogmrip-test.h             \
  ogmrip-version.h          \
  ogmrip-video-codec.h      \
  ogmrip-encode.h

libogmrip_encode_la_CPPFLAGS = \
  -I$(top_srcdir)/libogmrip-base     \
  -I$(top_srcdir)/libogmrip-job      \
  -I$(top_srcdir)/libogmrip-media    \
  -I$(top_srcdir)/libogmrip-file     \
  -I$(top_srcdir)/libogmrip-dvd      \
  -I$(top_builddir)/libogmrip-encode \
  -DOGMRIP_LIB_DIR=\""$(libdir)"\" \
  -DOGMRIP_DATA_DIR=\""$(datadir)"\"

libogmrip_encode_la_CFLAGS = \
  $(WARN_CFLAGS)

libogmrip_encode_la_LDFLAGS = \
  -version-info $(OGMRIP_ENCODE_LT_VERSION) \
  $(WARN_LDFLAGS)

libogmrip_encode_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la   \
  $(top_builddir)/libogmrip-job/libogmrip-job.la     \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-file/libogmrip-file.la   \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la

ogmrip-marshal.h: ogmrip-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=ogmrip_cclosure_marshal > $@

ogmrip-marshal.c: ogmrip-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"ogmrip-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=ogmrip_cclosure_marshal >> $@

EXTRA_DIST = \
  ogmrip-marshal.list \
  ogmrip-version.h.in

ogmripincdir = \
  $(includedir)/ogmrip-encode

ogmripinc_DATA = \
  ogmrip-version.h

CLEANFILES = \
  $(BUILT_SOURCES)

DISTCLEANFILES = \
  ogmrip-version.h

MAINTAINERCLEANFILES = \
  Makefile.in
