pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ogmrip-encode.pc \
		 ogmrip-module.pc \
                 ogmrip-media.pc  \
                 ogmrip-file.pc   \
                 ogmrip-dvd.pc    \
                 ogmrip-job.pc

profilesdir = $(datadir)/ogmrip/profiles
profilesfile = profile-avi.xml.in \
               profile-mkv.xml.in \
               profile-ogm.xml.in
profiles_DATA = $(profilesfile:.xml.in=.xml)
@INTLTOOL_XML_RULE@

gsettings_SCHEMAS = org.ogmrip.preferences.gschema.xml \
                    org.ogmrip.profile.gschema.xml \
                    org.ogmrip.lavc.gschema.xml \
                    org.ogmrip.x264.gschema.xml \
                    org.ogmrip.xvid.gschema.xml
@GSETTINGS_RULES@

if HAVE_GTK_SUPPORT

pkgconfig_DATA += ogmrip-encode-gtk.pc \
                  ogmrip-media-gtk.pc  \
                  ogmrip-file-gtk.pc   \
                  ogmrip-dvd-gtk.pc

uidir = $(datadir)/ogmrip/ui
ui_DATA = ogmrip-ui.xml               \
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

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = ogmrip.png

desktopdir  = $(datadir)/applications
desktopfile = ogmrip.desktop.in
desktop_DATA = $(desktopfile:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

endif # HAVE_GTK_SUPPORT

EXTRA_DIST =      \
  $(ui_DATA)      \
  $(pixmap_DATA)  \
  $(profilesfile) \
  $(desktopfile)  \
  $(gsettings_SCHEMAS)

CLEANFILES = \
  *.gladep   \
  *.bak

DISTCLEANFILES = \
  ogmrip.desktop \
  profile-avi.xml \
  profile-mkv.xml \
  profile-ogm.xml

