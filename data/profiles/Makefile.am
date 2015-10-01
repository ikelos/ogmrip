profilesdir = \
  $(datadir)/ogmrip/profiles

profilesfile = \
  profile-avi.xml.in  \
  profile-mkv.xml.in  \
  profile-ogm.xml.in  \
  profile-webm.xml.in \
  profile-copy.xml.in \
	profile-ios.xml.in  \
	profile-ps3.xml.in  \
	profile-psp.xml.in  \
	profile-xbox.xml.in

profiles_DATA = \
  $(profilesfile:.xml.in=.xml)

@INTLTOOL_XML_RULE@

EXTRA_DIST = \
  $(profilesfile)

DISTCLEANFILES = \
  $(profiles_DATA)

MAINTAINERCLEANFILES = \
  Makefile.in
