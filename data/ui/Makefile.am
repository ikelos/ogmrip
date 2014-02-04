pixmapdir = \
  $(datadir)/pixmaps

pixmap_DATA = \
  ogmrip.png

desktopdir  = \
  $(datadir)/applications

desktopfile = \
  ogmrip.desktop.in

desktop_DATA = \
  $(desktopfile:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =     \
  $(pixmap_DATA) \
  $(desktopfile)

DISTCLEANFILES = \
  $(desktop_DATA)

