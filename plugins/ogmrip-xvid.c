/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-fs.h"
#include "ogmrip-mplayer.h"
#include "ogmrip-plugin.h"
#include "ogmrip-version.h"
#include "ogmrip-xvid.h"

#include "ogmjob-exec.h"
#include "ogmjob-queue.h"

#include <stdio.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#define OGMRIP_XVID(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_XVID, OGMRipXvid))
#define OGMRIP_XVID_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_XVID, OGMRipXvidClass))
#define OGMRIP_IS_XVID(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_XVID))
#define OGMRIP_IS_XVID_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_XVID))

typedef struct _OGMRipXvid      OGMRipXvid;
typedef struct _OGMRipXvidClass OGMRipXvidClass;

struct _OGMRipXvid
{
  OGMRipVideoCodec parent_instance;
  
  gboolean cartoon;
  gboolean chroma_me;
  gboolean chroma_opt;
  gboolean closed_gop;
  gboolean gmc;
  gboolean grayscale;
  gboolean interlacing;
  gboolean packed;
  gboolean qpel;
  gboolean trellis;
  gint bquant_offset;
  gint bquant_ratio;
  gint bvhq;
  gint frame_drop_ratio;
  gint max_bquant;
  gint max_iquant;
  gint max_pquant;
  gint me_quality;
  gint min_bquant;
  gint min_iquant;
  gint min_pquant;
  gint max_keyint;
  gint par;
  gint par_height;
  gint par_width;
  gint profile;
  gint quant_type;
  gint vhq;
};

struct _OGMRipXvidClass
{
  OGMRipVideoCodecClass parent_class;
};

enum
{
  PROP_0,
  PROP_BQUANT_OFFSET,
  PROP_BQUANT_RATIO,
  PROP_BVHQ,
  PROP_CARTOON,
  PROP_CHROMA_ME,
  PROP_CHROMA_OPT,
  PROP_CLOSED_GOP,
  PROP_FRAME_DROP_RATIO,
  PROP_GMC,
  PROP_GRAYSCALE,
  PROP_INTERLACING,
  PROP_MAX_BQUANT,
  PROP_MAX_IQUANT,
  PROP_MAX_PQUANT,
  PROP_ME_QUALITY,
  PROP_MIN_BQUANT,
  PROP_MIN_IQUANT,
  PROP_MIN_PQUANT,
  PROP_MAX_KEYINT,
  PROP_PACKED,
  PROP_PAR,
  PROP_PAR_HEIGHT,
  PROP_PAR_WIDTH,
  PROP_PROFILE,
  PROP_QPEL,
  PROP_QUANT_TYPE,
  PROP_TRELLIS,
  PROP_VHQ
};

static gint ogmrip_xvid_run             (OGMJobSpawn       *spawn);
static void ogmrip_xvid_set_quality     (OGMRipVideoCodec  *video,
                                         OGMRipQualityType quality);
static void ogmrip_xvid_get_property    (GObject           *gobject,
                                         guint             property_id,
                                         GValue            *value,
                                         GParamSpec        *pspec);
static void ogmrip_xvid_set_property    (GObject           *gobject,
                                         guint             property_id,
                                         const GValue      *value,
                                         GParamSpec        *pspec);
static void ogmrip_xvid_set_profile     (OGMRipCodec       *codec,
                                         OGMRipProfile     *profile);

static const gchar * const properties[] =
{
  OGMRIP_XVID_PROP_BFRAMES,
  OGMRIP_XVID_PROP_BQUANT_OFFSET,
  OGMRIP_XVID_PROP_BQUANT_RATIO,
  OGMRIP_XVID_PROP_BVHQ,
  OGMRIP_XVID_PROP_CARTOON,
  OGMRIP_XVID_PROP_CHROMA_ME,
  OGMRIP_XVID_PROP_CHROMA_OPT,
  OGMRIP_XVID_PROP_CLOSED_GOP,
  OGMRIP_XVID_PROP_FRAME_DROP_RATIO,
  OGMRIP_XVID_PROP_GMC,
  OGMRIP_XVID_PROP_GRAYSCALE,
  OGMRIP_XVID_PROP_INTERLACING,
  OGMRIP_XVID_PROP_MAX_BQUANT,
  OGMRIP_XVID_PROP_MAX_IQUANT,
  OGMRIP_XVID_PROP_MAX_PQUANT,
  OGMRIP_XVID_PROP_ME_QUALITY,
  OGMRIP_XVID_PROP_MIN_BQUANT,
  OGMRIP_XVID_PROP_MIN_IQUANT,
  OGMRIP_XVID_PROP_MIN_PQUANT,
  OGMRIP_XVID_PROP_MAX_KEYINT,
  OGMRIP_XVID_PROP_PACKED,
  OGMRIP_XVID_PROP_PAR,
  OGMRIP_XVID_PROP_PAR_HEIGHT,
  OGMRIP_XVID_PROP_PAR_WIDTH,
  OGMRIP_XVID_PROP_PROFILE,
  OGMRIP_XVID_PROP_QUANT_TYPE,
  OGMRIP_XVID_PROP_QPEL,
  OGMRIP_XVID_PROP_TRELLIS,
  OGMRIP_XVID_PROP_VHQ,
  NULL
};

static gdouble
ogmrip_xvid_get_quantizer (OGMRipVideoCodec *video)
{
  gdouble quantizer;

  quantizer = ogmrip_video_codec_get_quantizer (video);

  return CLAMP (quantizer, 1, 31);
}

static gchar **
ogmrip_xvid_command (OGMRipVideoCodec *video, guint pass, guint passes, const gchar *log_file)
{
  OGMRipXvid *xvid;
  OGMDvdTitle *title;
  GPtrArray *argv;
  GString *options;

  const char *output;
  gint quality, bitrate, vid, threads, bframes, interlaced;

  static const gchar *profiles[] =
  {
    "unrestricted",
    "sp0",
    "sp1",
    "sp2",
    "sp3",
    "asp0",
    "asp1",
    "asp2",
    "asp3",
    "asp4",
    "asp5",
    "dxnhandheld",
    "dxnportntsc",
    "dxnportpal",
    "dxnhtntsc",
    "dxnhtpal",
    "dxnhdtv"
  };

  static const gchar *par[] =
  {
    NULL,
    "vga11",
    "pal43",
    "pal169",
    "ntsc43",
    "ntsc169",
    "ext"
  };

  output = ogmrip_codec_get_output (OGMRIP_CODEC (video));

  xvid = OGMRIP_XVID (video);

  argv = ogmrip_mencoder_video_command (video, pass == passes ? output : "/dev/null", pass);

  g_ptr_array_add (argv, g_strdup ("-ovc"));
  g_ptr_array_add (argv, g_strdup ("xvid"));

  quality = ogmrip_video_codec_get_quality (video);
  bframes = ogmrip_video_codec_get_max_b_frames (video);

  options = g_string_new (NULL);

  if (xvid->quant_type)
    g_string_append (options, "quant_type=mpeg");
  else
    g_string_append (options, "quant_type=h263");

  if (xvid->chroma_opt)
    g_string_append (options, ":chroma_opt");
  else
    g_string_append (options, ":nochroma_opt");

  g_string_append_printf (options, ":vhq=%u:bvhq=%u", xvid->vhq, xvid->bvhq);

  if (quality == OGMRIP_QUALITY_USER)
  {
    g_string_append_printf (options, ":profile=%s", profiles[CLAMP (xvid->profile, 0, 16)]);

    if (xvid->gmc)
      g_string_append (options, ":gmc");
    else
      g_string_append (options, ":nogmc");

    interlaced = ogmrip_video_codec_is_interlaced (video);
    if (interlaced > 0 && ogmrip_video_codec_get_deinterlacer (video) != OGMRIP_DEINT_NONE)
      interlaced = 0;

    if (interlaced > 0 || xvid->interlacing)
      g_string_append (options, ":interlacing");
    else
      g_string_append (options, ":nointerlacing");

    g_string_append_printf (options, ":min_iquant=%u:max_iquant=%u", xvid->min_iquant, xvid->max_iquant);
    g_string_append_printf (options, ":min_pquant=%u:max_pquant=%u", xvid->min_pquant, xvid->max_pquant);
    g_string_append_printf (options, ":min_bquant=%u:max_bquant=%u", xvid->min_bquant, xvid->max_bquant);
    g_string_append_printf (options, ":max_key_interval=%u", xvid->max_keyint);

    if (xvid->chroma_me)
      g_string_append (options, ":chroma_me");
    else
      g_string_append (options, ":nochroma_me");

    g_string_append_printf (options, ":me_quality=%u", xvid->me_quality);

    if (MPLAYER_CHECK_VERSION (1,0,0,6))
    {
      if (xvid->cartoon)
        g_string_append (options, ":cartoon");
      else
        g_string_append (options, ":nocartoon");
    }

    if (xvid->packed)
      g_string_append (options, ":packed");
    else
      g_string_append (options, ":nopacked");

    if (xvid->closed_gop)
      g_string_append (options, ":closed_gop");
    else
      g_string_append (options, ":noclosed_gop");

    g_string_append_printf (options, ":bquant_ratio=%u:bquant_offset=%d", xvid->bquant_ratio, xvid->bquant_offset);

    if (!xvid->par)
      g_string_append (options, ":autoaspect");
    else
    {
      g_string_append_printf (options, ":par=%s", par[CLAMP (xvid->par, 1, 6)]);

      if (xvid->par == 6)
        g_string_append_printf (options, ":par_width=%d:par_height=%d", xvid->par_width, xvid->par_height);
    }

    g_string_append_printf (options, ":max_bframes=%d", bframes);

    if (bframes == 0)
      g_string_append_printf (options, ":frame_drop_ratio=%u", xvid->frame_drop_ratio);
  }
  else
  {
    g_string_append (options, ":autoaspect");
    g_string_append_printf (options, ":max_bframes=%d", bframes);
  }

  if (xvid->qpel)
    g_string_append (options, ":qpel");
  else
    g_string_append (options, ":noqpel");

  if (pass != passes && ogmrip_video_codec_get_turbo (video))
    g_string_append (options, ":turbo");

  if (xvid->trellis)
    g_string_append (options, ":trellis");
  else
    g_string_append (options, ":notrellis");

  if (xvid->grayscale)
    g_string_append (options, ":greyscale");
  else
    g_string_append (options, ":nogreyscale");

  bitrate = ogmrip_video_codec_get_bitrate (video);
  if (bitrate > 0)
  {
    if (bitrate < 16001)
      g_string_append_printf (options, ":bitrate=%u", bitrate / 1000);
    else
      g_string_append_printf (options, ":bitrate=%u", bitrate);
  }
  else
    g_string_append_printf (options, ":fixed_quant=%.0lf", ogmrip_xvid_get_quantizer (video));

  if (passes > 1 && log_file)
  {
    g_string_append_printf (options, ":pass=%u", pass);
    g_ptr_array_add (argv, g_strdup ("-passlogfile"));
    g_ptr_array_add (argv, g_strdup (log_file));
  }

  threads = ogmrip_video_codec_get_threads (video);
  if (threads > 0)
  {
    guint height;

    ogmrip_video_codec_get_scale_size (video, NULL, &height);
    g_string_append_printf (options, ":threads=%u", CLAMP (threads, 1, height / 16));
  }

  g_ptr_array_add (argv, g_strdup ("-xvidencopts"));
  g_ptr_array_add (argv, g_string_free (options, FALSE));

  title = ogmdvd_stream_get_title (ogmrip_codec_get_input (OGMRIP_CODEC (video)));
  vid = ogmdvd_title_get_nr (title);

  if (MPLAYER_CHECK_VERSION (1,0,0,1))
    g_ptr_array_add (argv, g_strdup_printf ("dvd://%d", vid + 1));
  else
  {
    g_ptr_array_add (argv, g_strdup ("-dvd"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", vid + 1));
  }

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

G_DEFINE_TYPE (OGMRipXvid, ogmrip_xvid, OGMRIP_TYPE_VIDEO_CODEC)

static void
ogmrip_xvid_class_init (OGMRipXvidClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;
  OGMRipVideoCodecClass *video_class;
  OGMRipCodecClass *codec_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_xvid_get_property;
  gobject_class->set_property = ogmrip_xvid_set_property;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmrip_xvid_run;

  video_class = OGMRIP_VIDEO_CODEC_CLASS (klass);
  video_class->set_quality = ogmrip_xvid_set_quality;

  codec_class = OGMRIP_CODEC_CLASS (klass);
  codec_class->set_profile = ogmrip_xvid_set_profile;

  g_object_class_install_property (gobject_class, PROP_PROFILE,
      g_param_spec_uint (OGMRIP_XVID_PROP_PROFILE,
        "Profile property", "Set profile", 0, 16, OGMRIP_XVID_DEFAULT_PROFILE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_QUANT_TYPE,
      g_param_spec_uint (OGMRIP_XVID_PROP_QUANT_TYPE,
        NULL, NULL, 0, 1, OGMRIP_XVID_DEFAULT_QUANT_TYPE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ME_QUALITY,
      g_param_spec_uint (OGMRIP_XVID_PROP_ME_QUALITY,
        NULL, NULL, 0, 6, OGMRIP_XVID_DEFAULT_ME_QUALITY, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_VHQ,
      g_param_spec_uint (OGMRIP_XVID_PROP_VHQ,
        NULL, NULL, 0, 4, OGMRIP_XVID_DEFAULT_VHQ, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BVHQ,
      g_param_spec_uint (OGMRIP_XVID_PROP_BVHQ,
        NULL, NULL, 0, 1, OGMRIP_XVID_DEFAULT_BVHQ, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PAR,
      g_param_spec_uint (OGMRIP_XVID_PROP_PAR,
        NULL, NULL, 0, 6, OGMRIP_XVID_DEFAULT_PAR, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MIN_IQUANT,
      g_param_spec_uint (OGMRIP_XVID_PROP_MIN_IQUANT,
        NULL, NULL, 0, 31, OGMRIP_XVID_DEFAULT_MIN_IQUANT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MAX_IQUANT,
      g_param_spec_uint (OGMRIP_XVID_PROP_MAX_IQUANT,
        NULL, NULL, 0, 31, OGMRIP_XVID_DEFAULT_MAX_IQUANT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MIN_PQUANT,
      g_param_spec_uint (OGMRIP_XVID_PROP_MIN_PQUANT,
        NULL, NULL, 0, 31, OGMRIP_XVID_DEFAULT_MIN_PQUANT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MAX_PQUANT,
      g_param_spec_uint (OGMRIP_XVID_PROP_MAX_PQUANT,
        NULL, NULL, 0, 31, OGMRIP_XVID_DEFAULT_MAX_PQUANT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MIN_BQUANT,
      g_param_spec_uint (OGMRIP_XVID_PROP_MIN_BQUANT,
        NULL, NULL, 0, 31, OGMRIP_XVID_DEFAULT_MIN_BQUANT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MAX_BQUANT,
      g_param_spec_uint (OGMRIP_XVID_PROP_MAX_BQUANT,
        NULL, NULL, 0, 31, OGMRIP_XVID_DEFAULT_MAX_BQUANT, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_MAX_KEYINT,
        g_param_spec_uint (OGMRIP_XVID_PROP_MAX_KEYINT,
          NULL, NULL, 0, G_MAXUINT, OGMRIP_XVID_DEFAULT_MAX_KEYINT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FRAME_DROP_RATIO,
      g_param_spec_uint (OGMRIP_XVID_PROP_FRAME_DROP_RATIO,
        NULL, NULL, 0, 100, OGMRIP_XVID_DEFAULT_FRAME_DROP_RATIO, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BQUANT_RATIO,
      g_param_spec_uint (OGMRIP_XVID_PROP_BQUANT_RATIO,
        NULL, NULL, 0, 1000, OGMRIP_XVID_DEFAULT_BQUANT_RATIO, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BQUANT_OFFSET,
      g_param_spec_int (OGMRIP_XVID_PROP_BQUANT_OFFSET,
        NULL, NULL, -1000, 1000, OGMRIP_XVID_DEFAULT_BQUANT_OFFSET, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PAR_WIDTH,
      g_param_spec_uint (OGMRIP_XVID_PROP_PAR_WIDTH,
        NULL, NULL, 1, 255, OGMRIP_XVID_DEFAULT_PAR_WIDTH, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PAR_HEIGHT,
      g_param_spec_uint (OGMRIP_XVID_PROP_PAR_HEIGHT,
        NULL, NULL, 1, 255, OGMRIP_XVID_DEFAULT_PAR_HEIGHT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_GMC,
      g_param_spec_boolean (OGMRIP_XVID_PROP_GMC,
        NULL, NULL, OGMRIP_XVID_DEFAULT_GMC, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_INTERLACING,
      g_param_spec_boolean (OGMRIP_XVID_PROP_INTERLACING,
        NULL, NULL, OGMRIP_XVID_DEFAULT_INTERLACING, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHROMA_ME,
      g_param_spec_boolean (OGMRIP_XVID_PROP_CHROMA_ME,
        NULL, NULL, OGMRIP_XVID_DEFAULT_CHROMA_ME, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHROMA_OPT,
      g_param_spec_boolean (OGMRIP_XVID_PROP_CHROMA_OPT,
        NULL, NULL, OGMRIP_XVID_DEFAULT_CHROMA_OPT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PACKED,
      g_param_spec_boolean (OGMRIP_XVID_PROP_PACKED,
        NULL, NULL, OGMRIP_XVID_DEFAULT_PACKED, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CLOSED_GOP,
      g_param_spec_boolean (OGMRIP_XVID_PROP_CLOSED_GOP,
        NULL, NULL, OGMRIP_XVID_DEFAULT_CLOSED_GOP, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CARTOON,
      g_param_spec_boolean (OGMRIP_XVID_PROP_CARTOON,
        NULL, NULL, OGMRIP_XVID_DEFAULT_CARTOON, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_GRAYSCALE,
      g_param_spec_boolean (OGMRIP_XVID_PROP_GRAYSCALE,
        NULL, NULL, OGMRIP_XVID_DEFAULT_GRAYSCALE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_QPEL,
      g_param_spec_boolean (OGMRIP_XVID_PROP_QPEL,
        NULL, NULL, OGMRIP_XVID_DEFAULT_QPEL, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TRELLIS,
      g_param_spec_boolean (OGMRIP_XVID_PROP_TRELLIS,
        NULL, NULL, OGMRIP_XVID_DEFAULT_TRELLIS, G_PARAM_READWRITE));
}

static void
ogmrip_xvid_init (OGMRipXvid *xvid)
{
  xvid->chroma_me = OGMRIP_XVID_DEFAULT_CHROMA_ME;
  xvid->chroma_opt = OGMRIP_XVID_DEFAULT_CHROMA_OPT;
  xvid->closed_gop = OGMRIP_XVID_DEFAULT_CLOSED_GOP;
  xvid->gmc = OGMRIP_XVID_DEFAULT_GMC;
  xvid->interlacing = OGMRIP_XVID_DEFAULT_INTERLACING;
  xvid->packed = OGMRIP_XVID_DEFAULT_PACKED;
  xvid->bquant_offset = OGMRIP_XVID_DEFAULT_BQUANT_OFFSET;
  xvid->bquant_ratio = OGMRIP_XVID_DEFAULT_BQUANT_RATIO;
  xvid->bvhq = OGMRIP_XVID_DEFAULT_BVHQ;
  xvid->frame_drop_ratio = OGMRIP_XVID_DEFAULT_FRAME_DROP_RATIO;
  xvid->max_bquant = OGMRIP_XVID_DEFAULT_MAX_BQUANT;
  xvid->max_iquant = OGMRIP_XVID_DEFAULT_MAX_IQUANT;
  xvid->max_pquant = OGMRIP_XVID_DEFAULT_MAX_PQUANT;
  xvid->me_quality = OGMRIP_XVID_DEFAULT_ME_QUALITY;
  xvid->min_bquant = OGMRIP_XVID_DEFAULT_MIN_BQUANT;
  xvid->min_iquant = OGMRIP_XVID_DEFAULT_MIN_IQUANT;
  xvid->min_pquant = OGMRIP_XVID_DEFAULT_MIN_PQUANT;
  xvid->max_keyint = OGMRIP_XVID_DEFAULT_MAX_KEYINT;
  xvid->par = OGMRIP_XVID_DEFAULT_PAR;
  xvid->par_height = OGMRIP_XVID_DEFAULT_PAR_HEIGHT;
  xvid->par_width = OGMRIP_XVID_DEFAULT_PAR_WIDTH;
  xvid->profile = OGMRIP_XVID_DEFAULT_PROFILE;
  xvid->quant_type = OGMRIP_XVID_DEFAULT_QUANT_TYPE;
  xvid->vhq = OGMRIP_XVID_DEFAULT_VHQ;
  xvid->cartoon = OGMRIP_XVID_DEFAULT_CARTOON;
  xvid->grayscale = OGMRIP_XVID_DEFAULT_GRAYSCALE;
  xvid->qpel = OGMRIP_XVID_DEFAULT_QPEL;
  xvid->trellis = OGMRIP_XVID_DEFAULT_TRELLIS;
}

static void
ogmrip_xvid_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipXvid *xvid;

  xvid = OGMRIP_XVID (gobject);

  switch (property_id) 
  {
    case PROP_PROFILE:
      g_value_set_int (value, xvid->profile);
      break;
    case PROP_QUANT_TYPE:
      g_value_set_int (value, xvid->quant_type);
      break;
    case PROP_GMC:
      g_value_set_boolean (value, xvid->gmc);
      break;
    case PROP_INTERLACING:
      g_value_set_boolean (value, xvid->interlacing);
      break;
    case PROP_MIN_IQUANT:
      g_value_set_int (value, xvid->min_iquant);
      break;
    case PROP_MAX_IQUANT:
      g_value_set_int (value, xvid->max_iquant);
      break;
    case PROP_MIN_PQUANT:
      g_value_set_int (value, xvid->min_pquant);
      break;
    case PROP_MAX_PQUANT:
      g_value_set_int (value, xvid->min_pquant);
      break;
    case PROP_MIN_BQUANT:
      g_value_set_int (value, xvid->min_bquant);
      break;
    case PROP_MAX_BQUANT:
      g_value_set_int (value, xvid->min_bquant);
      break;
    case PROP_MAX_KEYINT:
      g_value_set_int (value, xvid->max_keyint);
      break;
    case PROP_CHROMA_ME:
      g_value_set_boolean (value, xvid->chroma_me);
      break;
    case PROP_CHROMA_OPT:
      g_value_set_boolean (value, xvid->chroma_opt);
      break;
    case PROP_ME_QUALITY:
      g_value_set_int (value, xvid->me_quality);
      break;
    case PROP_VHQ:
      g_value_set_int (value, xvid->vhq);
      break;
    case PROP_BVHQ:
      g_value_set_int (value, xvid->bvhq);
      break;
    case PROP_FRAME_DROP_RATIO:
      g_value_set_int (value, xvid->frame_drop_ratio);
      break;
    case PROP_PACKED:
      g_value_set_boolean (value, xvid->packed);
      break;
    case PROP_CLOSED_GOP:
      g_value_set_boolean (value, xvid->closed_gop);
      break;
    case PROP_BQUANT_RATIO:
      g_value_set_int (value, xvid->bquant_ratio);
      break;
    case PROP_BQUANT_OFFSET:
      g_value_set_int (value, xvid->bquant_offset);
      break;
    case PROP_PAR:
      g_value_set_int (value, xvid->par);
      break;
    case PROP_PAR_WIDTH:
      g_value_set_int (value, xvid->par_width);
      break;
    case PROP_PAR_HEIGHT:
      g_value_set_int (value, xvid->par_height);
      break;
    case PROP_CARTOON:
      g_value_set_boolean (value, xvid->cartoon);
      break;
    case PROP_GRAYSCALE:
      g_value_set_boolean (value, xvid->grayscale);
      break;
    case PROP_QPEL:
      g_value_set_boolean (value, xvid->qpel);
      break;
    case PROP_TRELLIS:
      g_value_set_boolean (value, xvid->trellis);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_xvid_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipXvid *xvid;

  xvid = OGMRIP_XVID (gobject);

  switch (property_id) 
  {
    case PROP_PROFILE:
      xvid->profile = g_value_get_int (value);
      break;
    case PROP_QUANT_TYPE:
      xvid->quant_type = g_value_get_int (value);
      break;
    case PROP_GMC:
      xvid->gmc = g_value_get_boolean (value);
      break;
    case PROP_INTERLACING:
      xvid->interlacing = g_value_get_boolean (value);
      break;
    case PROP_MIN_IQUANT:
      xvid->min_iquant = g_value_get_int (value);
      break;
    case PROP_MAX_IQUANT:
      xvid->max_iquant = g_value_get_int (value);
      break;
    case PROP_MIN_PQUANT:
      xvid->min_pquant = g_value_get_int (value);
      break;
    case PROP_MAX_PQUANT:
      xvid->max_pquant = g_value_get_int (value);
      break;
    case PROP_MIN_BQUANT:
      xvid->min_bquant = g_value_get_int (value);
      break;
    case PROP_MAX_BQUANT:
      xvid->max_bquant = g_value_get_int (value);
      break;
    case PROP_MAX_KEYINT:
      xvid->max_keyint = g_value_get_int (value);
      break;
    case PROP_CHROMA_ME:
      xvid->chroma_me = g_value_get_boolean (value);
      break;
    case PROP_CHROMA_OPT:
      xvid->chroma_opt = g_value_get_boolean (value);
      break;
    case PROP_ME_QUALITY:
      xvid->me_quality = g_value_get_int (value);
      break;
    case PROP_VHQ:
      xvid->vhq = g_value_get_int (value);
      break;
    case PROP_BVHQ:
      xvid->bvhq = g_value_get_int (value);
      break;
    case PROP_FRAME_DROP_RATIO:
      xvid->frame_drop_ratio = g_value_get_int (value);
      break;
    case PROP_PACKED:
      xvid->packed = g_value_get_boolean (value);
      break;
    case PROP_CLOSED_GOP:
      xvid->closed_gop = g_value_get_boolean (value);
      break;
    case PROP_BQUANT_RATIO:
      xvid->bquant_ratio = g_value_get_int (value);
      break;
    case PROP_BQUANT_OFFSET:
      xvid->bquant_offset = g_value_get_int (value);
      break;
    case PROP_PAR:
      xvid->par = g_value_get_int (value);
      break;
    case PROP_PAR_WIDTH:
      xvid->par_width = g_value_get_int (value);
      break;
    case PROP_PAR_HEIGHT:
      xvid->par_height = g_value_get_int (value);
      break;
    case PROP_CARTOON:
      xvid->cartoon = g_value_get_boolean (value);
      break;
    case PROP_GRAYSCALE:
      xvid->grayscale = g_value_get_boolean (value);
      break;
    case PROP_QPEL:
      xvid->qpel = g_value_get_boolean (value);
      break;
    case PROP_TRELLIS:
      xvid->trellis = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static gint
ogmrip_xvid_run (OGMJobSpawn *spawn)
{
  OGMJobSpawn *queue, *child;
  gchar **argv, *log_file, *cwd = NULL;
  gint pass, passes, result;

  queue = ogmjob_queue_new ();
  ogmjob_container_add (OGMJOB_CONTAINER (spawn), queue);
  g_object_unref (queue);

  passes = ogmrip_video_codec_get_passes (OGMRIP_VIDEO_CODEC (spawn));

  log_file = NULL;
  if (passes > 1)
  {
    if (MPLAYER_CHECK_VERSION (1,0,0,8))
      log_file = ogmrip_fs_mktemp ("log.XXXXXX", NULL);
    else
    {
      /*
       * Workaround against xvid pass log file
       * Should disappear someday
       */
        log_file = g_build_filename (g_get_tmp_dir (), "xvid-twopass.stats", NULL);
    }
  }

  for (pass = 0; pass < passes; pass ++)
  {
    argv = ogmrip_xvid_command (OGMRIP_VIDEO_CODEC (spawn), pass + 1, passes, log_file);
    if (!argv)
      return OGMJOB_RESULT_ERROR;

    child = ogmjob_exec_newv (argv);
    ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_mencoder_codec_watch, spawn, TRUE, FALSE, FALSE);
    ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
    g_object_unref (child);
  }

  if (!MPLAYER_CHECK_VERSION (1,0,0,8))
  {
    /*
     * Workaround against xvid pass log file
     */
    cwd = g_get_current_dir ();
    g_chdir (g_get_tmp_dir ());
  }

  result = OGMJOB_SPAWN_CLASS (ogmrip_xvid_parent_class)->run (spawn);

  if (cwd)
  {
    /*
     * Return in cwd
     */
    g_chdir (cwd);
    g_free (cwd);
  }

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), queue);

  g_unlink (log_file);
  g_free (log_file);

  return result;
}

static void
ogmrip_xvid_set_default_values (OGMRipXvid *xvid)
{
  ogmrip_xvid_init (xvid);

  ogmrip_video_codec_set_max_b_frames (OGMRIP_VIDEO_CODEC (xvid), OGMRIP_XVID_DEFAULT_BFRAMES);
}

static void
ogmrip_xvid_set_quality (OGMRipVideoCodec *video, OGMRipQualityType quality)
{
  OGMRipXvid *xvid;

  xvid = OGMRIP_XVID (video);
  ogmrip_xvid_set_default_values (xvid);

  switch (quality)
  {
    case OGMRIP_QUALITY_EXTREME:
      xvid->chroma_opt = TRUE;
      xvid->quant_type = 1;
      xvid->bvhq = 1;
      xvid->vhq = 4;
      break;
    case OGMRIP_QUALITY_HIGH:
      xvid->chroma_opt = TRUE;
      xvid->quant_type = 0;
      xvid->bvhq = 1;
      xvid->vhq = 2;
      break;
    case OGMRIP_QUALITY_NORMAL:
      xvid->chroma_opt = FALSE;
      xvid->quant_type = 0;
      xvid->bvhq = 0;
      xvid->vhq = 0;
      break;
    default:
      break;
  }
}

static void
ogmrip_xvid_set_profile (OGMRipCodec *codec, OGMRipProfile *profile)
{
/*
  OGMRipSettings *settings;

  settings = ogmrip_settings_get_default ();
  if (settings)
  {
    gchar *key;
    guint i;

    for (i = 0; properties[i]; i++)
    {
      key = ogmrip_settings_build_section (settings, OGMRIP_XVID_SECTION, properties[i], NULL);
      ogmrip_settings_set_property_from_key (settings, G_OBJECT (codec), properties[i], section, key);
      g_free (key);
    }
  }
*/
}

static OGMRipVideoPlugin xvid_plugin =
{
  NULL,
  G_TYPE_NONE,
  "xvid",
  N_("XviD"),
  OGMRIP_FORMAT_MPEG4,
  2,
  G_MAXINT
};

OGMRipVideoPlugin *
ogmrip_init_plugin (GError **error)
{
  gboolean match;
  gchar *output;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (!ogmrip_check_mencoder ())
  {
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("MEncoder is missing"));
    return NULL;
  }

  if (!g_spawn_command_line_sync ("mencoder -ovc help", &output, NULL, NULL, NULL))
    return NULL;

  match = g_regex_match_simple ("^ *xvid *- .*$", output, G_REGEX_MULTILINE, 0);
  g_free (output);

  if (!match)
  {
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("MEncoder is built without XviD support"));
    return NULL;
  }
/*
  settings = ogmrip_settings_get_default ();
  if (settings)
  {
    GObjectClass *klass;
    guint i;

    klass = g_type_class_ref (OGMRIP_TYPE_XVID);

    for (i = 0; properties[i]; i++)
      ogmrip_settings_install_key_from_property (settings, klass,
          OGMRIP_XVID_SECTION, properties[i], properties[i]);

    g_type_class_unref (klass);
  }
*/
  xvid_plugin.type = OGMRIP_TYPE_XVID;

  return &xvid_plugin;
}

