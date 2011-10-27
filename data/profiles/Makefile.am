profilesdir = \
  $(datadir)/ogmrip/profiles

profilesfile = \
  profile-avi.xml.in  \
  profile-mkv.xml.in  \
  profile-ogm.xml.in  \
  profile-webm.xml.in \
  profile-copy.xml.in

profiles_DATA = \
  $(profilesfile:.xml.in=.xml)

@INTLTOOL_XML_RULE@

EXTRA_DIST = \
  $(profilesfile)

DISTCLEANFILES = \
  $(profiles_DATA)

