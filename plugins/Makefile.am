#
# Video Codecs
#

pluginsdir=$(libdir)/ogmrip/plugins

plugins_LTLIBRARIES =

plugin_includedir = \
  $(includedir)/ogmrip-encode

plugin_include_HEADERS =

resourcefiles =

ui_FILES =

if HAVE_LAVC_SUPPORT
plugins_LTLIBRARIES += \
  ogmrip-lavc-mpeg.la

ogmrip_lavc_mpeg_la_SOURCES = \
  ogmrip-lavc-mpeg.c
  
ogmrip_lavc_mpeg_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_lavc_mpeg_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_lavc_mpeg_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_lavc_mpeg_la_LIBADD = \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la
endif

if HAVE_XVID_SUPPORT
plugins_LTLIBRARIES += \
  ogmrip-xvid.la

ogmrip_xvid_la_SOURCES = \
  ogmrip-xvid.c

ogmrip_xvid_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_xvid_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_xvid_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_xvid_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la

plugin_include_HEADERS += \
  ogmrip-xvid.h
endif

if HAVE_THEORA_SUPPORT
plugins_LTLIBRARIES += \
  ogmrip-theora.la

ogmrip_theora_la_SOURCES = \
  ogmrip-theora.c

ogmrip_theora_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_theora_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_theora_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_theora_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la
endif

if HAVE_X264_SUPPORT
plugins_LTLIBRARIES += \
  ogmrip-x264.la

ogmrip_x264_la_SOURCES = \
  ogmrip-x264.c

ogmrip_x264_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_x264_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_x264_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_x264_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la \
  -lm

plugin_include_HEADERS += \
  ogmrip-x264.h
endif

if HAVE_VPX_SUPPORT
plugins_LTLIBRARIES += \
  ogmrip-vpx.la

ogmrip_vpx_la_SOURCES = \
  ogmrip-vpx.c

ogmrip_vpx_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_vpx_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_vpx_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_vpx_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la
endif

if HAVE_DIRAC_SUPPORT
plugins_LTLIBRARIES += \
  ogmrip-dirac.la

ogmrip_dirac_la_SOURCES = \
  ogmrip-dirac.c

ogmrip_dirac_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_dirac_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_dirac_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_dirac_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la
endif

#
# Audio Codecs
#

plugins_LTLIBRARIES += \
  ogmrip-copy.la  \
  ogmrip-wav.la

ogmrip_copy_la_SOURCES = \
  ogmrip-copy.c

ogmrip_copy_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_copy_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_copy_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_copy_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la

ogmrip_wav_la_SOURCES = \
  ogmrip-wav.c

ogmrip_wav_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_wav_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_wav_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_wav_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la

if HAVE_MP3_SUPPORT
plugins_LTLIBRARIES += \
  ogmrip-mp3.la

ogmrip_mp3_la_SOURCES = \
  ogmrip-mp3.c

ogmrip_mp3_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_mp3_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_mp3_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_mp3_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la
endif

if HAVE_VORBIS_SUPPORT
plugins_LTLIBRARIES += \
  ogmrip-vorbis.la

ogmrip_vorbis_la_SOURCES = \
  ogmrip-vorbis.c

ogmrip_vorbis_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_vorbis_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_vorbis_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_vorbis_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la
endif

if HAVE_AAC_SUPPORT
plugins_LTLIBRARIES += \
  ogmrip-aac.la

ogmrip_aac_la_SOURCES = \
  ogmrip-aac.c

ogmrip_aac_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_aac_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_aac_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_aac_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la
endif

if HAVE_FLAC_SUPPORT
plugins_LTLIBRARIES += \
  ogmrip-flac.la

ogmrip_flac_la_SOURCES = \
  ogmrip-flac.c

ogmrip_flac_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_flac_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_flac_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_flac_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la
endif

if HAVE_AC3_SUPPORT
plugins_LTLIBRARIES += \
  ogmrip-ac3.la

ogmrip_ac3_la_SOURCES = \
  ogmrip-ac3.c

ogmrip_ac3_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_ac3_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_ac3_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_ac3_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la
endif

#
# Subp Codecs
#

plugins_LTLIBRARIES += \
  ogmrip-vobsub.la

ogmrip_vobsub_la_SOURCES = \
  ogmrip-vobsub.c

ogmrip_vobsub_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_vobsub_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_vobsub_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_vobsub_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la

if HAVE_SRT_SUPPORT
plugins_LTLIBRARIES += \
  ogmrip-srt.la

ogmrip_srt_la_SOURCES = \
  ogmrip-srt.c

ogmrip_srt_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_srt_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_srt_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_srt_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la
endif

#
# Containers
#

plugins_LTLIBRARIES += \
  ogmrip-avi.la

ogmrip_avi_la_SOURCES = \
  ogmrip-avi.c

ogmrip_avi_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_avi_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_avi_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_avi_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la

if HAVE_OGM_SUPPORT
plugins_LTLIBRARIES += \
  ogmrip-ogg.la

ogmrip_ogg_la_SOURCES = \
  ogmrip-ogg.c

ogmrip_ogg_la_CFLAGS = \
  $(WARN_CLFAGS)

ogmrip_ogg_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_ogg_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_ogg_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la
endif

if HAVE_MKV_SUPPORT
plugins_LTLIBRARIES += \
  ogmrip-mkv.la

ogmrip_mkv_la_SOURCES = \
  ogmrip-mkv.c

ogmrip_mkv_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_mkv_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_mkv_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_mkv_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la
endif

if HAVE_MP4_SUPPORT
plugins_LTLIBRARIES += \
  ogmrip-mp4.la

ogmrip_mp4_la_SOURCES = \
  ogmrip-mp4.c

ogmrip_mp4_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_mp4_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_mp4_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_mp4_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la

plugin_include_HEADERS += \
  ogmrip-mp4.h
endif

#
# Options
#

if HAVE_GTK_SUPPORT

extensionsdir=$(libdir)/ogmrip/extensions

extensions_LTLIBRARIES =

if HAVE_LAVC_SUPPORT
extensions_LTLIBRARIES += \
  ogmrip-lavc-options.la

ogmrip_lavc_options_la_SOURCES = \
  ogmrip-lavc-options.c \
  ogmrip-lavc-gresource.c

ogmrip_lavc_options_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_lavc_options_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_lavc_options_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_lavc_options_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la  \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la  \
  $(top_builddir)/libogmrip-module/libogmrip-module.la  \
  $(top_builddir)/libogmrip-mplayer/libogmrip-mplayer.la \
  $(top_builddir)/libogmrip-encode-gtk/libogmrip-encode-gtk.la

resourcefiles += \
  ogmrip-lavc.gresource.xml

ui_FILES += \
  ogmrip-lavc-options-dialog.ui

ogmrip-lavc-gresource.c: ogmrip-lavc.gresource.xml ogmrip-lavc-options-dialog.ui $(GLIB_COMPILE_RESOURCES)
	$(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/plugins --generate-source --target=$@ $<
endif

if HAVE_XVID_SUPPORT
extensions_LTLIBRARIES += \
  ogmrip-xvid-options.la

ogmrip_xvid_options_la_SOURCES = \
  ogmrip-xvid-options.c \
  ogmrip-xvid-gresource.c

ogmrip_xvid_options_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_xvid_options_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_xvid_options_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_xvid_options_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-module/libogmrip-module.la \
  $(top_builddir)/libogmrip-encode-gtk/libogmrip-encode-gtk.la

resourcefiles += \
  ogmrip-xvid.gresource.xml

ui_FILES += \
  ogmrip-xvid-options-dialog.ui

ogmrip-xvid-gresource.c: ogmrip-xvid.gresource.xml ogmrip-xvid-options-dialog.ui $(GLIB_COMPILE_RESOURCES)
	$(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/plugins --generate-source --target=$@ $<
endif

if HAVE_X264_SUPPORT
extensions_LTLIBRARIES += \
  ogmrip-x264-options.la

ogmrip_x264_options_la_SOURCES = \
  ogmrip-x264-options.c \
  ogmrip-x264-gresource.c

ogmrip_x264_options_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_x264_options_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_x264_options_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_x264_options_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-module/libogmrip-module.la \
  $(top_builddir)/libogmrip-encode-gtk/libogmrip-encode-gtk.la

resourcefiles += \
  ogmrip-x264.gresource.xml

ui_FILES += \
  ogmrip-x264-options-dialog.ui

ogmrip-x264-gresource.c: ogmrip-x264.gresource.xml ogmrip-x264-options-dialog.ui $(GLIB_COMPILE_RESOURCES)
	$(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/plugins --generate-source --target=$@ $<
endif

if HAVE_MP4_SUPPORT
extensions_LTLIBRARIES += \
  ogmrip-mp4-options.la

ogmrip_mp4_options_la_SOURCES = \
  ogmrip-mp4-options.c \
  ogmrip-mp4-gresource.c

ogmrip_mp4_options_la_CFLAGS = \
  $(WARN_CFLAGS)

ogmrip_mp4_options_la_LDFLAGS = \
  -export-dynamic -module -avoid-version \
  $(WARN_LDFLAGS)

ogmrip_mp4_options_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_mp4_options_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-module/libogmrip-module.la \
  $(top_builddir)/libogmrip-encode-gtk/libogmrip-encode-gtk.la

resourcefiles += \
  ogmrip-mp4.gresource.xml

ui_FILES += \
  ogmrip-mp4-options-dialog.ui

ogmrip-mp4-gresource.c: ogmrip-mp4.gresource.xml ogmrip-mp4-options-dialog.ui $(GLIB_COMPILE_RESOURCES)
	$(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/plugins --generate-source --target=$@ $<
endif
endif

RESOURCES = \
  $(resourcefiles:.gresource.xml=-gresource.c)

BUILT_SOURCES = \
  $(RESOURCES)

#
# Misc
#

AM_CPPFLAGS = \
  -I$(top_srcdir)/libogmrip-base      \
  -I$(top_srcdir)/libogmrip-job       \
  -I$(top_srcdir)/libogmrip-media     \
  -I$(top_srcdir)/libogmrip-media-gtk \
  -I$(top_srcdir)/libogmrip-file      \
  -I$(top_srcdir)/libogmrip-file      \
  -I$(top_srcdir)/libogmrip-file-gtk  \
  -I$(top_srcdir)/libogmrip-dvd       \
  -I$(top_srcdir)/libogmrip-encode    \
  -I$(top_srcdir)/libogmrip-mplayer   \
  -I$(top_srcdir)/libogmrip-module    \
  -I$(top_builddir)/libogmrip-encode  \
  -DOGMRIP_LIB_DIR=\""$(libdir)"\"    \
  -DOGMRIP_DATA_DIR=\""$(datadir)"\"

if HAVE_GTK_SUPPORT
AM_CPPFLAGS += \
  -I$(top_srcdir)/libogmrip-encode-gtk
endif

CLEANFILES = \
  $(RESOURCES)

MAINTAINERCLEANFILES = \
  Makefile.in

EXTRA_DIST = \
  $(resourcefiles) \
  $(ui_FILES)

