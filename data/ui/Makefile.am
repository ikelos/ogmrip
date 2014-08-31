desktopdir  = \
  $(datadir)/applications

desktopfile = \
  ogmrip.desktop.in

desktop_DATA = \
  $(desktopfile:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =     \
  $(desktopfile)

DISTCLEANFILES = \
  $(desktop_DATA)

