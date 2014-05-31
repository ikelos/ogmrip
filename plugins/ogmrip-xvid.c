/* OGMRipXvid - An XviD plugin for OGMRip
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-xvid.h"

#include <ogmrip-base.h>
#include <ogmrip-job.h>
#include <ogmrip-mplayer.h>
#include <ogmrip-module.h>

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
  gboolean lumi_mask;
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
  gint max_bframes;
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
  PROP_LUMI_MASK,
  PROP_MAX_BFRAMES,
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
  PROP_PASSES,
  PROP_PROFILE,
  PROP_QPEL,
  PROP_QUANT_TYPE,
  PROP_TRELLIS,
  PROP_VHQ
};

static void     ogmrip_xvid_notify       (GObject      *gobject,
                                          GParamSpec   *pspec);
static void     ogmrip_xvid_get_property (GObject      *gobject,
                                          guint        property_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);
static void     ogmrip_xvid_set_property (GObject      *gobject,
                                          guint        property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static gboolean ogmrip_xvid_run          (OGMJobTask   *task,
                                          GCancellable *cancellable,
                                          GError       **error);

static gdouble
ogmrip_xvid_get_quantizer (OGMRipVideoCodec *video)
{
  gdouble quantizer;

  quantizer = ogmrip_video_codec_get_quantizer (video);

  return CLAMP (quantizer, 1, 31);
}

static OGMJobTask *
ogmrip_xvid_command (OGMRipVideoCodec *video, guint pass, guint passes, const gchar *log_file)
{
  OGMRipXvid *xvid = OGMRIP_XVID (video);
  gchar *opts[] = { "-ovc", "xvid", "-xvidencopts", NULL, NULL, NULL, NULL };

  OGMJobTask *task;
  OGMRipTitle *title;
  GString *options;

  const char *output;
  gint bitrate, threads, interlaced;

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

  output = ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (video)));
  title = ogmrip_stream_get_title (ogmrip_codec_get_input (OGMRIP_CODEC (video)));

  options = g_string_new (NULL);

  if (xvid->quant_type)
    g_string_append (options, "quant_type=mpeg");
  else
    g_string_append (options, "quant_type=h263");

  if (xvid->chroma_opt)
    g_string_append (options, ":chroma_opt");
  else
    g_string_append (options, ":nochroma_opt");

  if (xvid->lumi_mask)
    g_string_append (options, ":lumi_mask");
  else
    g_string_append (options, ":nolumi_mask");

  g_string_append_printf (options, ":vhq=%u:bvhq=%u", xvid->vhq, xvid->bvhq);

  if (ogmrip_video_codec_get_quality (video) == OGMRIP_QUALITY_USER)
  {
    g_string_append_printf (options, ":profile=%s", profiles[CLAMP (xvid->profile, 0, 16)]);

    if (xvid->gmc)
      g_string_append (options, ":gmc");
    else
      g_string_append (options, ":nogmc");

    interlaced = ogmrip_title_get_interlaced (title);
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

    if (xvid->cartoon)
      g_string_append (options, ":cartoon");
    else
      g_string_append (options, ":nocartoon");

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

    g_string_append_printf (options, ":max_bframes=%d", xvid->max_bframes);

    if (xvid->max_bframes == 0)
      g_string_append_printf (options, ":frame_drop_ratio=%u", xvid->frame_drop_ratio);
  }
  else
  {
    g_string_append (options, ":autoaspect");
    g_string_append_printf (options, ":max_bframes=%d", xvid->max_bframes);
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
    if (bitrate > 16000)
      g_string_append_printf (options, ":bitrate=%u", bitrate);
    else
      g_string_append_printf (options, ":bitrate=%u", bitrate / 1000);
  }
  else
    g_string_append_printf (options, ":fixed_quant=%.0lf", ogmrip_xvid_get_quantizer (video));

  if (passes > 1 && log_file)
  {
    g_string_append_printf (options, ":pass=%u", pass);

    opts[4] = "-passlogfile";
    opts[5] = (gchar *) log_file;
  }

  threads = ogmrip_video_codec_get_threads (video);
  if (!threads)
    threads = ogmrip_get_nprocessors ();
  if (threads > 0)
  {
    guint height;

    ogmrip_video_codec_get_scale_size (video, NULL, &height);
    threads = CLAMP (threads, 1, height / 16);
  }
  g_string_append_printf (options, ":threads=%u", threads);

  opts[3] = options->str;

  task = ogmrip_mencoder_video_command (video, (const gchar * const *) opts, pass == passes ? output : "/dev/null");

  g_string_free (options, TRUE);

  return task;
}

static void
ogmrip_xvid_configure (OGMRipConfigurable *configurable, OGMRipProfile *profile)
{
  GSettings *settings;

  settings = ogmrip_profile_get_child (profile, "xvid");
  if (settings)
  {
    g_settings_bind (settings, "bquant-offset", configurable, OGMRIP_XVID_PROP_BQUANT_OFFSET,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "bquant-ratio", configurable, OGMRIP_XVID_PROP_BQUANT_RATIO,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "bvhq", configurable, OGMRIP_XVID_PROP_BVHQ,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "cartoon", configurable, OGMRIP_XVID_PROP_CARTOON,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "chroma-me", configurable, OGMRIP_XVID_PROP_CHROMA_ME,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "chroma-opt", configurable, OGMRIP_XVID_PROP_CHROMA_OPT,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "closed-gop", configurable, OGMRIP_XVID_PROP_CLOSED_GOP,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "frame-drop-ratio", configurable, OGMRIP_XVID_PROP_FRAME_DROP_RATIO,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "gmc", configurable, OGMRIP_XVID_PROP_GMC,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "grayscale", configurable, OGMRIP_XVID_PROP_GRAYSCALE,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "interlacing", configurable, OGMRIP_XVID_PROP_INTERLACING,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "lumi-mask", configurable, OGMRIP_XVID_PROP_LUMI_MASK,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "max-bframes", configurable, OGMRIP_XVID_PROP_MAX_BFRAMES,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "max-bquant", configurable, OGMRIP_XVID_PROP_MAX_BQUANT,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "max-iquant", configurable, OGMRIP_XVID_PROP_MAX_IQUANT,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "max-key-interval", configurable, OGMRIP_XVID_PROP_MAX_KEYINT,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "max-pquant", configurable, OGMRIP_XVID_PROP_MAX_PQUANT,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "me-quality", configurable, OGMRIP_XVID_PROP_ME_QUALITY,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "min-bquant", configurable, OGMRIP_XVID_PROP_MIN_BQUANT,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "min-iquant", configurable, OGMRIP_XVID_PROP_MIN_IQUANT,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "min-pquant", configurable, OGMRIP_XVID_PROP_MIN_PQUANT,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "packed", configurable, OGMRIP_XVID_PROP_PACKED,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "par", configurable, OGMRIP_XVID_PROP_PAR,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "par-height", configurable, OGMRIP_XVID_PROP_PAR_HEIGHT,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "par-width", configurable, OGMRIP_XVID_PROP_PAR_WIDTH,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "profile", configurable, OGMRIP_XVID_PROP_PROFILE,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "qpel", configurable, OGMRIP_XVID_PROP_QPEL,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "quant-type", configurable, OGMRIP_XVID_PROP_QUANT_TYPE,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "trellis", configurable, OGMRIP_XVID_PROP_TRELLIS,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "vhq", configurable, OGMRIP_XVID_PROP_VHQ,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);

    g_object_unref (settings);
  }
}

static void
ogmrip_configurable_iface_init (OGMRipConfigurableInterface *iface)
{
  iface->configure = ogmrip_xvid_configure;
}

G_DEFINE_TYPE_EXTENDED (OGMRipXvid, ogmrip_xvid, OGMRIP_TYPE_VIDEO_CODEC, 0,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_CONFIGURABLE, ogmrip_configurable_iface_init));

static void
ogmrip_xvid_class_init (OGMRipXvidClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobTaskClass *task_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->notify = ogmrip_xvid_notify;
  gobject_class->get_property = ogmrip_xvid_get_property;
  gobject_class->set_property = ogmrip_xvid_set_property;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_xvid_run;

  g_object_class_install_property (gobject_class, PROP_PROFILE,
      g_param_spec_uint (OGMRIP_XVID_PROP_PROFILE, "Profile property", "Set profile",
        0, 16, OGMRIP_XVID_DEFAULT_PROFILE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QUANT_TYPE,
      g_param_spec_uint (OGMRIP_XVID_PROP_QUANT_TYPE, NULL, NULL,
        0, 1, OGMRIP_XVID_DEFAULT_QUANT_TYPE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_ME_QUALITY,
      g_param_spec_uint (OGMRIP_XVID_PROP_ME_QUALITY, NULL, NULL,
        0, 6, OGMRIP_XVID_DEFAULT_ME_QUALITY, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VHQ,
      g_param_spec_uint (OGMRIP_XVID_PROP_VHQ, NULL, NULL,
        0, 4, OGMRIP_XVID_DEFAULT_VHQ, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BVHQ,
      g_param_spec_uint (OGMRIP_XVID_PROP_BVHQ, NULL, NULL,
        0, 1, OGMRIP_XVID_DEFAULT_BVHQ, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PAR,
      g_param_spec_uint (OGMRIP_XVID_PROP_PAR, NULL, NULL,
        0, 6, OGMRIP_XVID_DEFAULT_PAR, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MIN_IQUANT,
      g_param_spec_uint (OGMRIP_XVID_PROP_MIN_IQUANT, NULL, NULL,
        1, 31, OGMRIP_XVID_DEFAULT_MIN_IQUANT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_IQUANT,
      g_param_spec_uint (OGMRIP_XVID_PROP_MAX_IQUANT, NULL, NULL,
        1, 31, OGMRIP_XVID_DEFAULT_MAX_IQUANT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MIN_PQUANT,
      g_param_spec_uint (OGMRIP_XVID_PROP_MIN_PQUANT, NULL, NULL,
        1, 31, OGMRIP_XVID_DEFAULT_MIN_PQUANT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_PQUANT,
      g_param_spec_uint (OGMRIP_XVID_PROP_MAX_PQUANT, NULL, NULL,
        1, 31, OGMRIP_XVID_DEFAULT_MAX_PQUANT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MIN_BQUANT,
      g_param_spec_uint (OGMRIP_XVID_PROP_MIN_BQUANT, NULL, NULL,
        1, 31, OGMRIP_XVID_DEFAULT_MIN_BQUANT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_BQUANT,
      g_param_spec_uint (OGMRIP_XVID_PROP_MAX_BQUANT, NULL, NULL,
        1, 31, OGMRIP_XVID_DEFAULT_MAX_BQUANT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_BFRAMES,
      g_param_spec_uint (OGMRIP_XVID_PROP_MAX_BFRAMES, NULL, NULL,
        0, 4, OGMRIP_XVID_DEFAULT_MAX_BFRAMES, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_MAX_KEYINT,
        g_param_spec_uint (OGMRIP_XVID_PROP_MAX_KEYINT, NULL, NULL,
          0, G_MAXUINT, OGMRIP_XVID_DEFAULT_MAX_KEYINT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FRAME_DROP_RATIO,
      g_param_spec_uint (OGMRIP_XVID_PROP_FRAME_DROP_RATIO, NULL, NULL,
        0, 100, OGMRIP_XVID_DEFAULT_FRAME_DROP_RATIO, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BQUANT_RATIO,
      g_param_spec_uint (OGMRIP_XVID_PROP_BQUANT_RATIO, NULL, NULL,
        0, 1000, OGMRIP_XVID_DEFAULT_BQUANT_RATIO, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BQUANT_OFFSET,
      g_param_spec_int (OGMRIP_XVID_PROP_BQUANT_OFFSET, NULL, NULL,
        -1000, 1000, OGMRIP_XVID_DEFAULT_BQUANT_OFFSET, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PAR_WIDTH,
      g_param_spec_uint (OGMRIP_XVID_PROP_PAR_WIDTH, NULL, NULL,
        1, 255, OGMRIP_XVID_DEFAULT_PAR_WIDTH, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PAR_HEIGHT,
      g_param_spec_uint (OGMRIP_XVID_PROP_PAR_HEIGHT, NULL, NULL,
        1, 255, OGMRIP_XVID_DEFAULT_PAR_HEIGHT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_GMC,
      g_param_spec_boolean (OGMRIP_XVID_PROP_GMC, NULL, NULL,
        OGMRIP_XVID_DEFAULT_GMC, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_INTERLACING,
      g_param_spec_boolean (OGMRIP_XVID_PROP_INTERLACING, NULL, NULL,
        OGMRIP_XVID_DEFAULT_INTERLACING, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CHROMA_ME,
      g_param_spec_boolean (OGMRIP_XVID_PROP_CHROMA_ME, NULL, NULL,
        OGMRIP_XVID_DEFAULT_CHROMA_ME, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CHROMA_OPT,
      g_param_spec_boolean (OGMRIP_XVID_PROP_CHROMA_OPT, NULL, NULL,
        OGMRIP_XVID_DEFAULT_CHROMA_OPT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PACKED,
      g_param_spec_boolean (OGMRIP_XVID_PROP_PACKED, NULL, NULL,
        OGMRIP_XVID_DEFAULT_PACKED, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CLOSED_GOP,
      g_param_spec_boolean (OGMRIP_XVID_PROP_CLOSED_GOP, NULL, NULL,
        OGMRIP_XVID_DEFAULT_CLOSED_GOP, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CARTOON,
      g_param_spec_boolean (OGMRIP_XVID_PROP_CARTOON, NULL, NULL,
        OGMRIP_XVID_DEFAULT_CARTOON, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_GRAYSCALE,
      g_param_spec_boolean (OGMRIP_XVID_PROP_GRAYSCALE, NULL, NULL,
        OGMRIP_XVID_DEFAULT_GRAYSCALE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QPEL,
      g_param_spec_boolean (OGMRIP_XVID_PROP_QPEL, NULL, NULL,
        OGMRIP_XVID_DEFAULT_QPEL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TRELLIS,
      g_param_spec_boolean (OGMRIP_XVID_PROP_TRELLIS, NULL, NULL,
        OGMRIP_XVID_DEFAULT_TRELLIS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PASSES, 
        g_param_spec_uint ("passes", "Passes property", "Set the number of passes", 
           1, 2, 1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LUMI_MASK,
      g_param_spec_boolean (OGMRIP_XVID_PROP_LUMI_MASK, NULL, NULL,
        OGMRIP_XVID_DEFAULT_LUMI_MASK, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_xvid_init (OGMRipXvid *xvid)
{
  xvid->bquant_offset = OGMRIP_XVID_DEFAULT_BQUANT_OFFSET;
  xvid->bquant_ratio = OGMRIP_XVID_DEFAULT_BQUANT_RATIO;
  xvid->bvhq = OGMRIP_XVID_DEFAULT_BVHQ;
  xvid->cartoon = OGMRIP_XVID_DEFAULT_CARTOON;
  xvid->chroma_me = OGMRIP_XVID_DEFAULT_CHROMA_ME;
  xvid->chroma_opt = OGMRIP_XVID_DEFAULT_CHROMA_OPT;
  xvid->closed_gop = OGMRIP_XVID_DEFAULT_CLOSED_GOP;
  xvid->frame_drop_ratio = OGMRIP_XVID_DEFAULT_FRAME_DROP_RATIO;
  xvid->gmc = OGMRIP_XVID_DEFAULT_GMC;
  xvid->grayscale = OGMRIP_XVID_DEFAULT_GRAYSCALE;
  xvid->interlacing = OGMRIP_XVID_DEFAULT_INTERLACING;
  xvid->lumi_mask = OGMRIP_XVID_DEFAULT_LUMI_MASK;
  xvid->max_bframes = OGMRIP_XVID_DEFAULT_MAX_BFRAMES;
  xvid->max_bquant = OGMRIP_XVID_DEFAULT_MAX_BQUANT;
  xvid->max_iquant = OGMRIP_XVID_DEFAULT_MAX_IQUANT;
  xvid->max_keyint = OGMRIP_XVID_DEFAULT_MAX_KEYINT;
  xvid->max_pquant = OGMRIP_XVID_DEFAULT_MAX_PQUANT;
  xvid->me_quality = OGMRIP_XVID_DEFAULT_ME_QUALITY;
  xvid->min_bquant = OGMRIP_XVID_DEFAULT_MIN_BQUANT;
  xvid->min_iquant = OGMRIP_XVID_DEFAULT_MIN_IQUANT;
  xvid->min_pquant = OGMRIP_XVID_DEFAULT_MIN_PQUANT;
  xvid->packed = OGMRIP_XVID_DEFAULT_PACKED;
  xvid->par_height = OGMRIP_XVID_DEFAULT_PAR_HEIGHT;
  xvid->par = OGMRIP_XVID_DEFAULT_PAR;
  xvid->par_width = OGMRIP_XVID_DEFAULT_PAR_WIDTH;
  xvid->profile = OGMRIP_XVID_DEFAULT_PROFILE;
  xvid->qpel = OGMRIP_XVID_DEFAULT_QPEL;
  xvid->quant_type = OGMRIP_XVID_DEFAULT_QUANT_TYPE;
  xvid->trellis = OGMRIP_XVID_DEFAULT_TRELLIS;
  xvid->vhq = OGMRIP_XVID_DEFAULT_VHQ;
}

static void
ogmrip_xvid_notify (GObject *gobject, GParamSpec *pspec)
{
  OGMRipXvid *xvid = OGMRIP_XVID (gobject);

  if (g_str_equal (pspec->name, "quality"))
  {
    ogmrip_xvid_init (xvid);

    switch (ogmrip_video_codec_get_quality (OGMRIP_VIDEO_CODEC (xvid)))
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
}

static void
ogmrip_xvid_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipXvid *xvid;

  xvid = OGMRIP_XVID (gobject);

  switch (property_id) 
  {
    case PROP_PROFILE:
      g_value_set_uint (value, xvid->profile);
      break;
    case PROP_QUANT_TYPE:
      g_value_set_uint (value, xvid->quant_type);
      break;
    case PROP_GMC:
      g_value_set_boolean (value, xvid->gmc);
      break;
    case PROP_INTERLACING:
      g_value_set_boolean (value, xvid->interlacing);
      break;
    case PROP_MIN_IQUANT:
      g_value_set_uint (value, xvid->min_iquant);
      break;
    case PROP_MAX_IQUANT:
      g_value_set_uint (value, xvid->max_iquant);
      break;
    case PROP_MIN_PQUANT:
      g_value_set_uint (value, xvid->min_pquant);
      break;
    case PROP_MAX_PQUANT:
      g_value_set_uint (value, xvid->min_pquant);
      break;
    case PROP_MIN_BQUANT:
      g_value_set_uint (value, xvid->min_bquant);
      break;
    case PROP_MAX_BQUANT:
      g_value_set_uint (value, xvid->min_bquant);
      break;
    case PROP_MAX_BFRAMES:
      g_value_set_uint (value, xvid->max_bframes);
      break;
    case PROP_MAX_KEYINT:
      g_value_set_uint (value, xvid->max_keyint);
      break;
    case PROP_CHROMA_ME:
      g_value_set_boolean (value, xvid->chroma_me);
      break;
    case PROP_CHROMA_OPT:
      g_value_set_boolean (value, xvid->chroma_opt);
      break;
    case PROP_ME_QUALITY:
      g_value_set_uint (value, xvid->me_quality);
      break;
    case PROP_VHQ:
      g_value_set_uint (value, xvid->vhq);
      break;
    case PROP_BVHQ:
      g_value_set_uint (value, xvid->bvhq);
      break;
    case PROP_FRAME_DROP_RATIO:
      g_value_set_uint (value, xvid->frame_drop_ratio);
      break;
    case PROP_PACKED:
      g_value_set_boolean (value, xvid->packed);
      break;
    case PROP_CLOSED_GOP:
      g_value_set_boolean (value, xvid->closed_gop);
      break;
    case PROP_BQUANT_RATIO:
      g_value_set_uint (value, xvid->bquant_ratio);
      break;
    case PROP_BQUANT_OFFSET:
      g_value_set_int (value, xvid->bquant_offset);
      break;
    case PROP_PAR:
      g_value_set_uint (value, xvid->par);
      break;
    case PROP_PAR_WIDTH:
      g_value_set_uint (value, xvid->par_width);
      break;
    case PROP_PAR_HEIGHT:
      g_value_set_uint (value, xvid->par_height);
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
    case PROP_PASSES:
      g_value_set_uint (value, ogmrip_video_codec_get_passes (OGMRIP_VIDEO_CODEC (xvid)));
      break;
    case PROP_LUMI_MASK:
      g_value_set_boolean (value, xvid->lumi_mask);
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
      xvid->profile = g_value_get_uint (value);
      break;
    case PROP_QUANT_TYPE:
      xvid->quant_type = g_value_get_uint (value);
      break;
    case PROP_GMC:
      xvid->gmc = g_value_get_boolean (value);
      break;
    case PROP_INTERLACING:
      xvid->interlacing = g_value_get_boolean (value);
      break;
    case PROP_MIN_IQUANT:
      xvid->min_iquant = g_value_get_uint (value);
      break;
    case PROP_MAX_IQUANT:
      xvid->max_iquant = g_value_get_uint (value);
      break;
    case PROP_MIN_PQUANT:
      xvid->min_pquant = g_value_get_uint (value);
      break;
    case PROP_MAX_PQUANT:
      xvid->max_pquant = g_value_get_uint (value);
      break;
    case PROP_MIN_BQUANT:
      xvid->min_bquant = g_value_get_uint (value);
      break;
    case PROP_MAX_BQUANT:
      xvid->max_bquant = g_value_get_uint (value);
      break;
    case PROP_MAX_BFRAMES:
      xvid->max_bframes = g_value_get_uint (value);
      break;
    case PROP_MAX_KEYINT:
      xvid->max_keyint = g_value_get_uint (value);
      break;
    case PROP_CHROMA_ME:
      xvid->chroma_me = g_value_get_boolean (value);
      break;
    case PROP_CHROMA_OPT:
      xvid->chroma_opt = g_value_get_boolean (value);
      break;
    case PROP_ME_QUALITY:
      xvid->me_quality = g_value_get_uint (value);
      break;
    case PROP_VHQ:
      xvid->vhq = g_value_get_uint (value);
      break;
    case PROP_BVHQ:
      xvid->bvhq = g_value_get_uint (value);
      break;
    case PROP_FRAME_DROP_RATIO:
      xvid->frame_drop_ratio = g_value_get_uint (value);
      break;
    case PROP_PACKED:
      xvid->packed = g_value_get_boolean (value);
      break;
    case PROP_CLOSED_GOP:
      xvid->closed_gop = g_value_get_boolean (value);
      break;
    case PROP_BQUANT_RATIO:
      xvid->bquant_ratio = g_value_get_uint (value);
      break;
    case PROP_BQUANT_OFFSET:
      xvid->bquant_offset = g_value_get_int (value);
      break;
    case PROP_PAR:
      xvid->par = g_value_get_uint (value);
      break;
    case PROP_PAR_WIDTH:
      xvid->par_width = g_value_get_uint (value);
      break;
    case PROP_PAR_HEIGHT:
      xvid->par_height = g_value_get_uint (value);
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
    case PROP_PASSES:
      ogmrip_video_codec_set_passes (OGMRIP_VIDEO_CODEC (xvid), g_value_get_uint (value));
      break;
    case PROP_LUMI_MASK:
      xvid->lumi_mask = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static gboolean
ogmrip_xvid_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobTask *queue, *child;
  gchar *log_file, *cwd = NULL;
  gint pass, passes;
  gboolean result;

  queue = ogmjob_queue_new ();
  ogmjob_container_add (OGMJOB_CONTAINER (task), queue);
  g_object_unref (queue);

  passes = ogmrip_video_codec_get_passes (OGMRIP_VIDEO_CODEC (task));

  log_file = NULL;
  if (passes > 1)
  {
    log_file = ogmrip_fs_mktemp ("log.XXXXXX", error);
    if (!log_file)
      return FALSE;
  }

  for (pass = 0; pass < MIN (passes, 2); pass ++)
  {
    child = ogmrip_xvid_command (OGMRIP_VIDEO_CODEC (task), pass + 1, passes, log_file);
    ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
    g_object_unref (child);
  }

  result = OGMJOB_TASK_CLASS (ogmrip_xvid_parent_class)->run (task, cancellable, error);

  if (cwd)
  {
    /*
     * Return in cwd
     */
    g_chdir (cwd);
    g_free (cwd);
  }

  ogmjob_container_remove (OGMJOB_CONTAINER (task), queue);

  g_unlink (log_file);
  g_free (log_file);

  return result;
}

void
ogmrip_module_load (OGMRipModule *module)
{
  gboolean match;
  gchar *output;

  if (!ogmrip_check_mencoder ())
  {
    g_warning (_("MEncoder is missing"));
    return;
  }

  if (!g_spawn_command_line_sync ("mencoder -ovc help", &output, NULL, NULL, NULL))
    return;

  match = g_regex_match_simple ("^ *xvid *- .*$", output, G_REGEX_MULTILINE, 0);
  g_free (output);

  if (!match)
  {
    g_warning (_("MEncoder is built without XviD support"));
    return;
  }

  ogmrip_register_codec (OGMRIP_TYPE_XVID, "xvid", _("XviD"), OGMRIP_FORMAT_MPEG4,
      "schema-id", "org.ogmrip.xvid", NULL);
}

