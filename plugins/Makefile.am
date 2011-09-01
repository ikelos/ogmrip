#
# Video Codecs
#

video_codecsdir=$(libdir)/ogmrip/video-plugins

video_codecs_LTLIBRARIES =

video_codecs_HEADERS =

if HAVE_LAVC_SUPPORT
video_codecs_LTLIBRARIES += \
  ogmrip-lavc-mpeg4.la

ogmrip_lavc_mpeg4_la_SOURCES = \
  ogmrip-lavc-mpeg4.c
  
ogmrip_lavc_mpeg4_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_lavc_mpeg4_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_lavc_mpeg4_la_LIBADD = \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode/libogmrip-lavc.la \
  $(top_builddir)/libogmrip-encode/libogmrip-mplayer.la

video_codecs_HEADERS += \
  ogmrip-lavc-mpeg4.h
endif

if HAVE_XVID_SUPPORT
video_codecs_LTLIBRARIES += \
  ogmrip-xvid.la

ogmrip_xvid_la_SOURCES = \
  ogmrip-xvid.c

ogmrip_xvid_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_xvid_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_xvid_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode/libogmrip-mplayer.la

video_codecs_HEADERS += \
  ogmrip-xvid.h
endif

if HAVE_THEORA_SUPPORT
video_codecs_LTLIBRARIES += \
  ogmrip-theora.la

ogmrip_theora_la_SOURCES = \
  ogmrip-theora.c

ogmrip_theora_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_theora_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_theora_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode/libogmrip-mplayer.la
endif

if HAVE_X264_SUPPORT
video_codecs_LTLIBRARIES += \
  ogmrip-x264.la

ogmrip_x264_la_SOURCES = \
  ogmrip-x264.c

ogmrip_x264_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_x264_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_x264_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode/libogmrip-mplayer.la \
  -lm

video_codecs_HEADERS += \
  ogmrip-x264.h
endif

#
# Audio Codecs
#

audio_codecsdir=$(libdir)/ogmrip/audio-plugins

audio_codecs_LTLIBRARIES = \
  ogmrip-acopy.la  \
  ogmrip-wav.la

ogmrip_acopy_la_SOURCES = \
  ogmrip-acopy.c

ogmrip_acopy_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_acopy_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_acopy_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode/libogmrip-mplayer.la

ogmrip_wav_la_SOURCES = \
  ogmrip-wav.c

ogmrip_wav_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_wav_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_wav_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode/libogmrip-mplayer.la

if HAVE_MP3_SUPPORT
audio_codecs_LTLIBRARIES += \
  ogmrip-mp3.la

ogmrip_mp3_la_SOURCES = \
  ogmrip-mp3.c

ogmrip_mp3_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_mp3_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_mp3_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode/libogmrip-mplayer.la
endif

if HAVE_VORBIS_SUPPORT
audio_codecs_LTLIBRARIES += \
  ogmrip-vorbis.la

ogmrip_vorbis_la_SOURCES = \
  ogmrip-vorbis.c

ogmrip_vorbis_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_vorbis_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_vorbis_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode/libogmrip-mplayer.la
endif

if HAVE_AAC_SUPPORT
audio_codecs_LTLIBRARIES += \
  ogmrip-aac.la

ogmrip_aac_la_SOURCES = \
  ogmrip-aac.c

ogmrip_aac_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_aac_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_aac_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode/libogmrip-mplayer.la
endif

#
# Subp Codecs
#

subp_codecsdir=$(libdir)/ogmrip/subp-plugins

subp_codecs_LTLIBRARIES = \
  ogmrip-vobsub.la

ogmrip_vobsub_la_SOURCES = \
  ogmrip-vobsub.c

ogmrip_vobsub_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_vobsub_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_vobsub_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode/libogmrip-mplayer.la

if HAVE_SRT_SUPPORT
subp_codecs_LTLIBRARIES += \
  ogmrip-srt.la

ogmrip_srt_la_SOURCES = \
  ogmrip-srt.c

ogmrip_srt_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_srt_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_srt_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode/libogmrip-mplayer.la
endif

#
# Containers
#

containersdir=$(libdir)/ogmrip/container-plugins

containers_LTLIBRARIES = \
  ogmrip-avi.la

ogmrip_avi_la_SOURCES = \
  ogmrip-avi.c

ogmrip_avi_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_avi_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_avi_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode/libogmrip-mplayer.la

if HAVE_OGM_SUPPORT
containers_LTLIBRARIES += \
  ogmrip-ogg.la

ogmrip_ogg_la_SOURCES = \
  ogmrip-ogg.c

ogmrip_ogg_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_ogg_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_ogg_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la
endif

if HAVE_MKV_SUPPORT
containers_LTLIBRARIES += \
  ogmrip-mkv.la

ogmrip_mkv_la_SOURCES = \
  ogmrip-mkv.c

ogmrip_mkv_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_mkv_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_mkv_la_LIBADD = \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la
endif

if HAVE_MP4_SUPPORT
containers_LTLIBRARIES += \
  ogmrip-mp4.la

ogmrip_mp4_la_SOURCES = \
  ogmrip-mp4.c

ogmrip_mp4_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_mp4_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_mp4_la_LIBADD = \
  $(top_builddir)/libogmrip-job/libogmrip-job.la \
  $(top_builddir)/libogmrip-media/libogmrip-media.la \
  $(top_builddir)/libogmrip-dvd/libogmrip-dvd.la \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode/libogmrip-mplayer.la
endif

#
# Options
#

if HAVE_GTK_SUPPORT
options_plugindir=$(libdir)/ogmrip/options-plugins

options_plugin_LTLIBRARIES =

if HAVE_LAVC_SUPPORT
options_plugin_LTLIBRARIES += \
  ogmrip-lavc-options.la

ogmrip_lavc_options_la_SOURCES = \
  ogmrip-lavc-options.c

ogmrip_lavc_options_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_lavc_options_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_lavc_options_la_LIBADD = \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode/libogmrip-lavc.la \
  $(top_builddir)/libogmrip-encode-gtk/libogmrip-encode-gtk.la
endif

if HAVE_XVID_SUPPORT
options_plugin_LTLIBRARIES += \
  ogmrip-xvid-options.la

ogmrip_xvid_options_la_SOURCES = \
  ogmrip-xvid-options.c

ogmrip_xvid_options_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_xvid_options_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_xvid_options_la_LIBADD = \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode-gtk/libogmrip-encode-gtk.la
endif

if HAVE_X264_SUPPORT
options_plugin_LTLIBRARIES += \
  ogmrip-x264-options.la

ogmrip_x264_options_la_SOURCES = \
  ogmrip-x264-options.c

ogmrip_x264_options_la_LDFLAGS = \
  -export-dynamic -module -avoid-version

ogmrip_x264_options_la_LIBTOOLFLAGS = \
  --tag=disable-static

ogmrip_x264_options_la_LIBADD = \
  $(top_builddir)/libogmrip-encode/libogmrip-encode.la \
  $(top_builddir)/libogmrip-encode-gtk/libogmrip-encode-gtk.la
endif
endif

#
# Misc
#

INCLUDES = \
  -I$(top_srcdir)/libogmrip-job \
  -I$(top_srcdir)/libogmrip-media \
  -I$(top_srcdir)/libogmrip-file \
  -I$(top_srcdir)/libogmrip-dvd \
  -I$(top_srcdir)/libogmrip-encode \
  -DOGMRIP_LIB_DIR=\""$(libdir)"\" \
  -DOGMRIP_DATA_DIR=\""$(datadir)"\"

if HAVE_GTK_SUPPORT
INCLUDES += \
  -I$(top_srcdir)/libogmrip-encode-gtk
endif

if MAINTAINER_MODE
INCLUDES += \
  -DG_ENABLE_DEBUG
endif

