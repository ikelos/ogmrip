uidir = \
  $(datadir)/ogmrip/ui

ui_DATA = \
  ogmrip-ui.xml               \
  ogmrip-crop.glade           \
  ogmrip-main.glade           \
  ogmrip-options.glade        \
  ogmrip-pref.glade           \
  ogmrip-profile-editor.glade \
  ogmrip-profiles.glade       \
  ogmrip-progress.glade       \
  ogmrip-queue.glade          \
  ogmrip-spell.glade          \
  ogmrip-lavc.glade           \
  ogmrip-x264.glade           \
  ogmrip-xvid.glade

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
  $(ui_DATA)     \
  $(pixmap_DATA) \
  $(desktopfile)

DISTCLEANFILES = \
  $(desktop_DATA)

