uidir = \
  $(datadir)/ogmrip/ui

ui_DATA = \
  ogmrip-menu.ui                    \
  ogmrip-crop-dialog.ui             \
  ogmrip-main-window.ui             \
  ogmrip-options-dialog.ui          \
  ogmrip-pref-dialog.ui             \
  ogmrip-profile-editor.ui          \
  ogmrip-profile-manager-dialog.ui  \
  ogmrip-progress-dialog.ui         \
  ogmrip-encoding-manager-dialog.ui \
  ogmrip-spell-dialog.ui            \
  ogmrip-lavc-options-dialog.ui     \
  ogmrip-x264-options-dialog.ui     \
  ogmrip-xvid-options-dialog.ui     \
	ogmrip-mp4-options-dialog.ui

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

