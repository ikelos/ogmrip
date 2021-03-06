AC_PREREQ(2.64)

m4_define([ogmrip_major_version], [1])
m4_define([ogmrip_minor_version], [9])
m4_define([ogmrip_micro_version], [0])
m4_define([ogmrip_version], [ogmrip_major_version.ogmrip_minor_version.ogmrip_micro_version])

AC_INIT([OGMRip], [ogmrip_version])
AX_IS_RELEASE([git-directory])

AC_CONFIG_SRCDIR(src/ogmrip-main.c)
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR(m4)

AM_INIT_AUTOMAKE([1.11.1])

dnl **************************************************************

OGMRIP_MAJOR_VERSION=ogmrip_major_version
OGMRIP_MINOR_VERSION=ogmrip_minor_version
OGMRIP_MICRO_VERSION=ogmrip_micro_version
OGMRIP_VERSION=ogmrip_version
AC_SUBST(OGMRIP_MAJOR_VERSION)
AC_SUBST(OGMRIP_MINOR_VERSION)
AC_SUBST(OGMRIP_MICRO_VERSION)
AC_SUBST(OGMRIP_VERSION)

dnl **************************************************************

AX_CHECK_ENABLE_DEBUG([yes],[GNOME_ENABLE_DEBUG])
AX_COMPILER_FLAGS([WARN_CFLAGS],[WARN_LDFLAGS])
# CFLAGS="$CFLAGS $WARN_CFLAGS"
# LDFLAGS="$LDFLAGS $WARN_LDFLAGS"

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LN_S

AC_HEADER_STDC
AC_C_BIGENDIAN
AC_SYS_LARGEFILE

PKG_PROG_PKG_CONFIG

dnl **************************************************************

AC_CHECK_HEADERS(sys/statvfs.h mntent.h sys/mnttab.h sys/vfstab.h sys/cdio.h)
AC_CHECK_FUNCS(statvfs)

dnl **************************************************************

AC_PATH_PROG(SED_PROG, gsed)
if test x"$SED_PROG" = x; then
  AC_PATH_PROG(SED_PROG, sed)
fi
if ! "$SED_PROG" --version 2> /dev/null | head -n 1 | grep -q "GNU sed"; then
  AC_MSG_ERROR(Unable to find GNU sed in the PATH. You need GNU sed to configure OGMRip. Find it on ftp://ftp.gnu.org/gnu/sed)
fi

dnl **************************************************************

GLIB_REQUIRED=2.40.0

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= $GLIB_REQUIRED gobject-2.0 >= $GLIB_REQUIRED gmodule-2.0 >= $GLIB_REQUIRED gio-2.0 >= $GLIB_REQUIRED])
CFLAGS="$CFLAGS $GLIB_CFLAGS"
LIBS="$LIBS $GLIB_LIBS"

GLIB_GSETTINGS

dnl **************************************************************

PKG_CHECK_MODULES(LIBMEDIAINFO, [libmediainfo])
CFLAGS="$CFLAGS $LIBMEDIAINFO_CFLAGS"
LIBS="$LIBS $LIBMEDIAINFO_LIBS"

PKG_CHECK_MODULES(LIBXML, [libxml-2.0])
CFLAGS="$CFLAGS $LIBXML_CFLAGS"
LIBS="$LIBS $LIBXML_LIBS"

PKG_CHECK_MODULES(LIBPNG, [libpng zlib], [have_png_support=yes], [have_png_support=no])
CFLAGS="$CFLAGS $LIBPNG_CFLAGS"
LIBS="$LIBS $LIBPNG_LIBS"

if test x"$have_png_support" = x"yes"; then
  AC_DEFINE(HAVE_PNG_SUPPORT, 1, [Define to 1 if png is supported.])
fi

AM_CONDITIONAL(HAVE_PNG_SUPPORT, [test x"$have_png_support" = x"yes"])

AC_CHECK_LIB(tiff, TIFFOpen, [LIBTIFF_LIBS="-ltiff" && have_tiff_support=yes], [have_tiff_support=no])
LIBS="$LIBS $LIBTIFF_LIBS"

if test x"$have_tiff_support" = x"yes"; then
  AC_DEFINE(HAVE_TIFF_SUPPORT, 1, [Define to 1 if tiff is supported.])
fi

AM_CONDITIONAL(HAVE_TIFF_SUPPORT, [test x"$have_tiff_support" = x"yes"])

dnl **************************************************************

GTK_REQUIRED=3.12

AC_ARG_ENABLE(gtk-support,
  AC_HELP_STRING([--disable-gtk-support], [disable GTK+ support]),
  [have_gtk_support="$enableval"], [have_gtk_support="yes"])

if test x"$have_gtk_support" != x"no"; then
  PKG_CHECK_MODULES(GTK, [gtk+-3.0 >= $GTK_REQUIRED], [have_gtk_support=yes], [have_gtk_support=no])
fi

if test x"$have_gtk_support" = x"yes"; then
  AC_DEFINE(HAVE_GTK_SUPPORT, 1, [Define to 1 if gtk+ is supported.])
fi

AM_CONDITIONAL(HAVE_GTK_SUPPORT, [test x"$have_gtk_support" = x"yes"])

CFLAGS="$CFLAGS $GTK_CFLAGS "
LIBS="$LIBS $GTK_LIBS"

dnl **************************************************************

AC_ARG_ENABLE(enchant-support,
  AC_HELP_STRING([--disable-enchant-support], [disable Enchant support]),
  [have_enchant_support="$enableval"], [have_enchant_support="yes"])

if test x"$have_enchant_support" != x"no"; then
  PKG_CHECK_MODULES(ENCHANT, [enchant >= 1.1.0], [have_enchant_support=yes], [have_enchant_support=no])
fi

if test x"$have_enchant_support" = x"yes"; then
  AC_DEFINE(HAVE_ENCHANT_SUPPORT, 1, [Define to 1 if enchant is supported.])
fi

AM_CONDITIONAL(HAVE_ENCHANT_SUPPORT, [test x"$have_enchant_support" != x"no"])

CFLAGS="$CFLAGS $ENCHANT_CFLAGS "
LIBS="$LIBS $ENCHANT_LIBS"

dnl **************************************************************

PKG_CHECK_MODULES(ENCA, [enca])
CFLAGS="$CFLAGS $ENCA_CFLAGS"
LIBS="$LIBS $ENCA_LIBS"

dnl **************************************************************

DVDREAD_REQUIRED=4.2.0

PKG_CHECK_MODULES(DVDREAD, [dvdread >= $DVDREAD_REQUIRED])
CFLAGS="$CFLAGS $DVDREAD_CFLAGS"
LIBS="$LIBS $DVDREAD_LIBS"

dnl **************************************************************

PKG_CHECK_MODULES(LIBBLURAY, [libbluray], [have_bluray_support=yes], [have_bluray_support=no])
CFLAGS="$CFLAGS $LIBBLURAY_CFLAGS"
LIBS="$LIBS $LIBBLURAY_LIBS"

if test x"$have_bluray_support" = x"yes"; then
  AC_DEFINE(HAVE_BLURAY_SUPPORT, 1, [Define to 1 if bluray is supported.])
fi

AM_CONDITIONAL(HAVE_BLURAY_SUPPORT, [test x"$have_bluray_support" = x"yes"])

dnl **************************************************************

AC_PATH_PROG(MPLAYER_PROG, mplayer)
if test x"$MPLAYER_PROG" = x; then
  AC_MSG_ERROR(Unable to find mplayer in the PATH. You need mplayer to use OGMRip. Find it on http://www.mplayerhq.hu)
fi

dnl **************************************************************

AC_PATH_PROG(MENCODER_PROG, mencoder)
if test x"$MENCODER_PROG" = x; then
  AC_MSG_ERROR(Unable to find mencoder in the PATH. You need mencoder to use OGMRip. Find it on http://www.mplayerhq.hu)
fi

dnl **************************************************************

AC_ARG_ENABLE(ogm-support,
  AC_HELP_STRING([--disable-ogm-support], [disable OGM support]),
  [have_ogm_support="$enableval"], [have_ogm_support="yes"])

AM_CONDITIONAL(HAVE_OGM_SUPPORT, [test x"$have_ogm_support" = x"yes"])

dnl **************************************************************

AC_ARG_ENABLE(mkv-support,
  AC_HELP_STRING([--disable-mkv-support], [disable Matroska support]),
  [have_mkv_support="$enableval"], [have_mkv_support="yes"])

AM_CONDITIONAL(HAVE_MKV_SUPPORT, [test x"$have_mkv_support" = x"yes"])

dnl **************************************************************

AC_ARG_ENABLE(mp4-support,
  AC_HELP_STRING([--disable-mp4-support], [disable Mpeg-4 support]),
  [have_mp4_support="$enableval"], [have_mp4_support="yes"])

AM_CONDITIONAL(HAVE_MP4_SUPPORT, [test x"$have_mp4_support" = x"yes"])

dnl **************************************************************

AC_ARG_ENABLE(lavf-support,
  AC_HELP_STRING([--disable-lavf-support], [disable Lavf support]),
  [have_lavf_support="$enableval"], [have_lavf_support="yes"])

AM_CONDITIONAL(HAVE_LAVF_SUPPORT, [test x"$have_lavf_support" = x"yes"])

dnl **************************************************************

AC_ARG_ENABLE(xvid-support,
  AC_HELP_STRING([--disable-xvid-support], [disable XviD support]),
  [have_xvid_support="$enableval"], [have_xvid_support="yes"])

AM_CONDITIONAL(HAVE_XVID_SUPPORT, [test x"$have_xvid_support" = x"yes"])

dnl **************************************************************

AC_ARG_ENABLE(lavc-support,
  AC_HELP_STRING([--disable-lavc-support], [disable Lavc support]),
  [have_lavc_support="$enableval"], [have_lavc_support="yes"])

AM_CONDITIONAL(HAVE_LAVC_SUPPORT, [test x"$have_lavc_support" = x"yes"])

dnl **************************************************************

AC_ARG_ENABLE(x264-support,
  AC_HELP_STRING([--disable-x264-support], [disable X264 support]),
  [have_x264_support="$enableval"], [have_x264_support="yes"])

AM_CONDITIONAL(HAVE_X264_SUPPORT, [test x"$have_x264_support" = x"yes"])

dnl **************************************************************

AC_ARG_ENABLE(theora-support,
  AC_HELP_STRING([--disable-theora-support], [disable Theora support]),
  [have_theora_support="$enableval"], [have_theora_support="yes"])

if test x"$have_theora_support" != x"no"; then
  PKG_CHECK_MODULES(THEORA, [theora >= 1.0alpha5], [have_theora_support=yes], [have_theora_support=no])
fi

AM_CONDITIONAL(HAVE_THEORA_SUPPORT, [test x"$have_theora_support" = x"yes"])

CFLAGS="$CFLAGS $THEORA_CFLAGS"
LIBS="$LIBS $THEORA_LIBS"

dnl **************************************************************

AC_ARG_ENABLE(dirac-support,
  AC_HELP_STRING([--disable-dirac-support], [disable Dirac support]),
  [have_dirac_support="$enableval"], [have_dirac_support="yes"])

AM_CONDITIONAL(HAVE_DIRAC_SUPPORT, [test x"$have_dirac_support" = x"yes"])

dnl **************************************************************

AC_ARG_ENABLE(vpx-support,
  AC_HELP_STRING([--disable-vpx-support], [disable VPX support]),
  [have_vpx_support="$enableval"], [have_vpx_support="yes"])

AM_CONDITIONAL(HAVE_VPX_SUPPORT, [test x"$have_vpx_support" = x"yes"])

dnl **************************************************************

AC_ARG_ENABLE(vorbis-support,
  AC_HELP_STRING([--disable-vorbis-support], [disable Ogg Vorbis support]),
  [have_vorbis_support="$enableval"], [have_vorbis_support="yes"])

AM_CONDITIONAL(HAVE_VORBIS_SUPPORT, [test x"$have_vorbis_support" = x"yes"])

dnl **************************************************************

AC_ARG_ENABLE(mp3-support,
  AC_HELP_STRING([--disable-mp3-support], [disable Mp3 support]),
  [have_mp3_support="$enableval"], [have_mp3_support="yes"])

AM_CONDITIONAL(HAVE_MP3_SUPPORT, [test x"$have_mp3_support" = x"yes"])

dnl **************************************************************

AC_ARG_ENABLE(aac-support,
  AC_HELP_STRING([--disable-aac-support], [disable AAC support]),
  [have_aac_support="$enableval"], [have_aac_support="yes"])

AM_CONDITIONAL(HAVE_AAC_SUPPORT, [test x"$have_aac_support" = x"yes"])

dnl **************************************************************

AC_ARG_ENABLE(flac-support,
  AC_HELP_STRING([--disable-flac-support], [disable FLAC support]),
  [have_flac_support="$enableval"], [have_flac_support="yes"])

AM_CONDITIONAL(HAVE_FLAC_SUPPORT, [test x"$have_flac_support" = x"yes"])

dnl **************************************************************

AC_ARG_ENABLE(ac3-support,
  AC_HELP_STRING([--disable-ac3-support], [disable AC3 support]),
  [have_ac3_support="$enableval"], [have_ac3_support="yes"])

AM_CONDITIONAL(HAVE_AC3_SUPPORT, [test x"$have_ac3_support" = x"yes"])

dnl **************************************************************

AC_ARG_ENABLE(srt-support,
  AC_HELP_STRING([--disable-srt-support], [disable SRT support]),
  [have_srt_support="$enableval"], [have_srt_support="yes"])

AM_CONDITIONAL(HAVE_SRT_SUPPORT, [test x"$have_srt_support" = x"yes"])

AC_ARG_WITH(ocr,
  AC_HELP_STRING([--with-ocr=auto|gocr|ocrad|tesseract],
  [Select the OCR program to use (default gocr)]),,
  with_ocr=auto)

if test x"$have_srt_support" = x"yes"; then
  if test x"$with_ocr" = x"auto"; then
    AC_DEFINE(HAVE_GOCR_SUPPORT, 1, [Define to 1 if gocr should be supported.])
    AC_DEFINE(HAVE_OCRAD_SUPPORT, 1, [Define to 1 if ocrad should be supported.])
    AC_DEFINE(HAVE_TESSERACT_SUPPORT, 1, [Define to 1 if ocrad should be supported.])
  elif test x"$with_ocr" = x"gocr"; then
    AC_DEFINE(HAVE_GOCR_SUPPORT, 1, [Define to 1 if gocr is should be supported.])
  elif test x"$with_ocr" = x"ocrad"; then
    AC_DEFINE(HAVE_OCRAD_SUPPORT, 1, [Define to 1 if ocrad is should be supported.])
  elif test x"$with_ocr" = x"tesseract"; then
    AC_DEFINE(HAVE_TESSERACT_SUPPORT, 1, [Define to 1 if tesseract is should be supported.])
  fi
fi

dnl **************************************************************

AC_LANG(C)

AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <unistd.h>
int main(void) { return sysconf(_SC_NPROCESSORS_ONLN) == -1 ? 1 : 0; }
]])],[have_sysconf_nproc=yes],[have_sysconf_nproc=no],[have_sysconf_nproc=no])

if test x"$have_sysconf_nproc" = x"yes" ; then
  AC_DEFINE(HAVE_SYSCONF_NPROC, 1, [Whether sysconf(_SC_NPROCESSORS_ONLN) is available])
fi

dnl **************************************************************

LT_PREREQ([2.2.6])
LT_INIT

IT_PROG_INTLTOOL([0.40.6])
AM_GLIB_GNU_GETTEXT

GETTEXT_PACKAGE=ogmrip
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [GetText Package])

if test "x${prefix}" = "xNONE"; then 
  AC_DEFINE_UNQUOTED(LOCALEDIR, "${ac_default_prefix}/${DATADIRNAME}/locale", [Locale directory])
else 
  AC_DEFINE_UNQUOTED(LOCALEDIR, "${prefix}/${DATADIRNAME}/locale", [Locale directory])
fi

dnl **************************************************************

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources)

dnl **************************************************************

# Before making a release, the OGMRIP_LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A

OGMRIP_BASE_LT_VERSION=1:0:0
AC_SUBST(OGMRIP_BASE_LT_VERSION)

OGMRIP_JOB_LT_VERSION=1:0:0
AC_SUBST(OGMRIP_JOB_LT_VERSION)

OGMRIP_MEDIA_LT_VERSION=1:0:0
AC_SUBST(OGMRIP_MEDIA_LT_VERSION)

OGMRIP_MODULE_LT_VERSION=1:0:0
AC_SUBST(OGMRIP_MODULE_LT_VERSION)

OGMRIP_ENCODE_LT_VERSION=1:0:0
AC_SUBST(OGMRIP_ENCODE_LT_VERSION)

OGMRIP_MPLAYER_LT_VERSION=1:0:0
AC_SUBST(OGMRIP_MPLAYER_LT_VERSION)

dnl **************************************************************

AC_CONFIG_FILES([
src/Makefile
avibox/Makefile
subrip/Makefile
dvdcpy/Makefile
theoraenc/Makefile
libogmrip-base/Makefile
libogmrip-job/Makefile
libogmrip-media/Makefile
libogmrip-media-gtk/Makefile
libogmrip-file/Makefile
libogmrip-file-gtk/Makefile
libogmrip-dvd/Makefile
libogmrip-bluray/Makefile
libogmrip-module/Makefile
libogmrip-encode/Makefile
libogmrip-encode/ogmrip-version.h
libogmrip-encode-gtk/Makefile
libogmrip-mplayer/Makefile
plugins/Makefile
data/Makefile
data/pkgconfig/Makefile
data/pkgconfig/ogmrip-base.pc
data/pkgconfig/ogmrip-dvd.pc
data/pkgconfig/ogmrip-bluray.pc
data/pkgconfig/ogmrip-encode-gtk.pc
data/pkgconfig/ogmrip-encode.pc
data/pkgconfig/ogmrip-file-gtk.pc
data/pkgconfig/ogmrip-file.pc
data/pkgconfig/ogmrip-job.pc
data/pkgconfig/ogmrip-media-gtk.pc
data/pkgconfig/ogmrip-media.pc
data/pkgconfig/ogmrip-module.pc
data/profiles/Makefile
data/schemas/Makefile
data/ui/Makefile
po/Makefile.in
Makefile
])
AC_OUTPUT

AC_MSG_NOTICE([OGMRip was configured with the following options:])

# if test x"$have_mplayer_dev" = xyes; then
#   AC_MSG_NOTICE([** MPlayer SVN/CVS detected - USE AT YOUR OWN RISK])
# fi

if test x"$have_gtk_support" = xyes; then
  AC_MSG_NOTICE([** The GUI will be build])
else
  AC_MSG_NOTICE([   The GUI will not be build])
fi

if test x"$have_enchant_support" = xyes; then
  AC_MSG_NOTICE([** Enchant support enabled])
else
  AC_MSG_NOTICE([   Enchant support disabled])
fi

if test x"$have_ogm_support" = xyes; then
  AC_MSG_NOTICE([** Ogg Media support enabled])
else
  AC_MSG_NOTICE([   Ogg Media support disabled])
fi

if test x"$have_mkv_support" = xyes; then
  AC_MSG_NOTICE([** Matroska support enabled])
else
  AC_MSG_NOTICE([   Matroska support disabled])
fi

if test x"$have_mp4_support" = xyes; then
  AC_MSG_NOTICE([** Mpeg-4 support enabled])
else
  AC_MSG_NOTICE([   Mpeg-4 support disabled])
fi

if test x"$have_lavf_support" = xyes; then
  AC_MSG_NOTICE([** Lavf support enabled])
else
  AC_MSG_NOTICE([   Lavf support disabled])
fi

if test x"$have_lavc_support" = xyes; then
  AC_MSG_NOTICE([** Lavc support enabled])
else
  AC_MSG_NOTICE([   Lavc support disabled])
fi

if test x"$have_xvid_support" = xyes; then
  AC_MSG_NOTICE([** XviD support enabled])
else
  AC_MSG_NOTICE([   XviD support disabled])
fi

if test x"$have_x264_support" = xyes; then
  AC_MSG_NOTICE([** X264 support enabled])
else
  AC_MSG_NOTICE([   X264 support disabled])
fi

if test x"$have_theora_support" = xyes; then
  AC_MSG_NOTICE([** Ogg Theora support enabled])
else
  AC_MSG_NOTICE([   Ogg Theora support disabled])
fi

if test x"$have_dirac_support" = xyes; then
  AC_MSG_NOTICE([** Dirac support enabled])
else
  AC_MSG_NOTICE([   Dirac support disabled])
fi

if test x"$have_vpx_support" = xyes; then
  AC_MSG_NOTICE([** VPX support enabled])
else
  AC_MSG_NOTICE([   VPX support disabled])
fi

if test x"$have_mp3_support" = xyes; then
  AC_MSG_NOTICE([** MP3 support enabled])
else
  AC_MSG_NOTICE([   MP3 support disabled])
fi

if test x"$have_vorbis_support" = xyes; then
  AC_MSG_NOTICE([** Ogg Vorbis support enabled])
else
  AC_MSG_NOTICE([   Ogg Vorbis support disabled])
fi

if test x"$have_aac_support" = xyes; then
  AC_MSG_NOTICE([** AAC support enabled])
else
  AC_MSG_NOTICE([   AAC support disabled])
fi

if test x"$have_flac_support" = xyes; then
  AC_MSG_NOTICE([** FLAC support enabled])
else
  AC_MSG_NOTICE([   FLAC support disabled])
fi

if test x"$have_srt_support" = xyes; then
  AC_MSG_NOTICE([** SRT support enabled])
else
  AC_MSG_NOTICE([   SRT support disabled])
fi

