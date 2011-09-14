lib_LTLIBRARIES = \
  libogmrip-dvd-gtk.la

BUILT_SOURCES = \
  ogmdvd-marshal.c \
  ogmdvd-marshal.h

libogmrip_dvd_gtk_la_SOURCES =  \
  ogmdvd-drive-chooser.c        \
  ogmdvd-drive-chooser-widget.c \
  ogmdvd-drive-chooser-dialog.c \
  ogmdvd-marshal.c

libogmrip_dvd_gtk_ladir = \
  $(includedir)/ogmrip-dvd

libogmrip_dvd_gtk_la_HEADERS =  \
  ogmdvd-drive-chooser.h        \
  ogmdvd-drive-chooser-widget.h \
  ogmdvd-drive-chooser-dialog.h \
  ogmdvd-marshal.h              \
  ogmrip-dvd-gtk.h

libogmrip_dvd_gtk_la_LDFLAGS = \
  -version-info $(OGMRIP_MEDIA_LT_VERSION)

libogmrip_dvd_gtk_la_LIBADD = \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la

ogmdvd-marshal.h: ogmdvd-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=ogmdvd_cclosure_marshal > $@

ogmdvd-marshal.c: ogmdvd-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"ogmdvd-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=ogmdvd_cclosure_marshal >> $@

EXTRA_DIST = \
  ogmdvd-marshal.list

INCLUDES = \
  -I$(top_srcdir)/libogmrip-media \
  -I$(top_srcdir)/libogmrip-dvd

CLEANFILES = \
  $(BUILT_SOURCES)

