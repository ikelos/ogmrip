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
#include "ogmrip-video-codec.h"
#include "ogmrip-x264.h"

#include "ogmjob-exec.h"
#include "ogmjob-queue.h"

#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#define OGMRIP_TYPE_X264          (ogmrip_x264_get_type ())
#define OGMRIP_X264(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_X264, OGMRipX264))
#define OGMRIP_X264_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_X264, OGMRipX264Class))
#define OGMRIP_IS_X264(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_X264))
#define OGMRIP_IS_X264_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_X264))

typedef struct _OGMRipX264      OGMRipX264;
typedef struct _OGMRipX264Class OGMRipX264Class;

struct _OGMRipX264
{
  OGMRipVideoCodec parent_instance;

  guint b_pyramid;
  guint cqm;
  guint direct;
  guint frameref;
  guint keyint;
  guint level_idc;
  guint me;
  guint merange;
  guint rc_lookahead;
  guint subq;
  guint trellis;
  guint vbv_bufsize;
  guint vbv_maxrate;
  guint weight_p;
  gboolean aud;
  gboolean b_adapt;
  gboolean brdo;
  gboolean cabac;
  gboolean global_header;
  gboolean mixed_refs;
  gboolean weight_b;
  gboolean x88dct;
  gdouble psy_rd;
  gdouble psy_trellis;
  gchar *partitions;
};

struct _OGMRipX264Class
{
  OGMRipVideoCodecClass parent_class;
};

enum
{
  PROP_0,
  PROP_8X8DCT,
  PROP_AUD,
  PROP_B_ADAPT,
  PROP_B_PYRAMID,
  PROP_BRDO,
  PROP_CABAC,
  PROP_CQM,
  PROP_DIRECT,
  PROP_FRAMEREF,
  PROP_GLOBAL_HEADER,
  PROP_KEYINT,
  PROP_LEVEL_IDC,
  PROP_ME,
  PROP_MERANGE,
  PROP_MIXED_REFS,
  PROP_PARTITIONS,
  PROP_PSY_RD,
  PROP_PSY_TRELLIS,
  PROP_RC_LOOKAHEAD,
  PROP_SUBQ,
  PROP_TRELLIS,
  PROP_VBV_BUFSIZE,
  PROP_VBV_MAXRATE,
  PROP_WEIGHT_B,
  PROP_WEIGHT_P
};

enum
{
  ME_NONE,
  ME_DIA,
  ME_HEX,
  ME_UMH,
  ME_ESA,
  ME_TESA
};

enum
{
  DIRECT_NONE,
  DIRECT_SPATIAL,
  DIRECT_TEMPORAL,
  DIRECT_AUTO
};

enum
{
  B_PYRAMID_NONE,
  B_PYRAMID_STRICT,
  B_PYRAMID_NORMAL
};

static void ogmrip_x264_finalize        (GObject           *gobject);
static void ogmrip_x264_get_property    (GObject           *gobject,
                                         guint             property_id,
                                         GValue            *value,
                                         GParamSpec        *pspec);
static void ogmrip_x264_set_property    (GObject           *gobject,
                                         guint             property_id,
                                         const GValue      *value,
                                         GParamSpec        *pspec);
static gint ogmrip_x264_run             (OGMJobSpawn       *spawn);
static gint ogmrip_x264_get_start_delay (OGMRipVideoCodec  *video);
static void ogmrip_x264_set_quality     (OGMRipVideoCodec  *video,
                                         OGMRipQualityType quality);
static void ogmrip_x264_set_profile     (OGMRipCodec       *codec,
                                         OGMRipProfile     *profile);

static const gchar * const properties[] =
{
  OGMRIP_X264_PROP_8X8DCT,
  OGMRIP_X264_PROP_AUD,
  OGMRIP_X264_PROP_B_ADAPT,
  OGMRIP_X264_PROP_B_PYRAMID,
  OGMRIP_X264_PROP_BFRAMES,
  OGMRIP_X264_PROP_BRDO,
  OGMRIP_X264_PROP_CABAC,
  OGMRIP_X264_PROP_CQM,
  OGMRIP_X264_PROP_DIRECT,
  OGMRIP_X264_PROP_FRAMEREF,
  OGMRIP_X264_PROP_GLOBAL_HEADER,
  OGMRIP_X264_PROP_KEYINT,
  OGMRIP_X264_PROP_LEVEL_IDC,
  OGMRIP_X264_PROP_ME,
  OGMRIP_X264_PROP_MERANGE,
  OGMRIP_X264_PROP_MIXED_REFS,
  OGMRIP_X264_PROP_PARTITIONS,
  OGMRIP_X264_PROP_PSY_RD,
  OGMRIP_X264_PROP_PSY_TRELLIS,
  OGMRIP_X264_PROP_RC_LOOKAHEAD,
  OGMRIP_X264_PROP_SUBQ,
  OGMRIP_X264_PROP_TRELLIS,
  OGMRIP_X264_PROP_VBV_BUFSIZE,
  OGMRIP_X264_PROP_VBV_MAXRATE,
  OGMRIP_X264_PROP_WEIGHT_B,
  OGMRIP_X264_PROP_WEIGHT_P,
  NULL
};

static const gchar *me_name[] =
{
  NULL, "dia", "hex", "umh", "esa", "tesa"
};

static const gchar *direct_name[] =
{
  "none", "spatial", "temporal", "auto"
};

static const gchar *b_pyramid_name[] =
{
  "none", "strict", "normal"
};

static const gchar *cqm_name[] =
{
  "flat", "jvm"
};

gboolean x264_have_8x8dct         = FALSE;
gboolean x264_have_aud            = FALSE;
gboolean x264_have_bime           = FALSE;
gboolean x264_have_b_pyramid      = FALSE;
gboolean x264_have_brdo           = FALSE;
gboolean x264_have_lookahead      = FALSE;
gboolean x264_have_me             = FALSE;
gboolean x264_have_me_tesa        = FALSE;
gboolean x264_have_mixed_refs     = FALSE;
gboolean x264_have_partitions     = FALSE;
gboolean x264_have_psy            = FALSE;
gboolean x264_have_turbo          = FALSE;
gboolean x264_have_weight_p       = FALSE;
gboolean x264_have_slow_firstpass = FALSE;
gboolean x264_have_nombtree       = FALSE;

static gint
ogmrip_x264_get_crf (OGMRipVideoCodec *video, gdouble quantizer)
{
  gint crf;

  if (quantizer < 0.0)
    quantizer = 2.3;

  crf = 12 + (unsigned int) (6.0 * log (quantizer) / log (2.0));

  return CLAMP (crf, 0, 50);
}

static gchar **
ogmrip_x264_command (OGMRipVideoCodec *video, guint pass, guint passes, const gchar *log_file)
{
  OGMRipX264 *x264;
  OGMDvdTitle *title;
  GPtrArray *argv;
  GString *options;

  const gchar *output;
  gint quality, bitrate, vid, threads, bframes;
  gboolean cartoon;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), NULL);

  output = ogmrip_codec_get_output (OGMRIP_CODEC (video));
  g_return_val_if_fail (output != NULL, NULL);

  title = ogmrip_codec_get_input (OGMRIP_CODEC (video));
  g_return_val_if_fail (title != NULL, NULL);

  g_return_val_if_fail (pass == 1 || log_file != NULL, NULL);

  cartoon = FALSE;

  x264 = OGMRIP_X264 (video);
  quality = ogmrip_video_codec_get_quality (video);

  argv = ogmrip_mencoder_video_command (video, pass == passes ? output : "/dev/null", pass);

  g_ptr_array_add (argv, g_strdup ("-ovc"));
  g_ptr_array_add (argv, g_strdup ("x264"));

  options = g_string_new (cartoon ? "deblock=1,1:aq_strength=0.6" : "deblock=-1,-1");
  g_string_append_printf (options, ":subq=%u:direct_pred=%s",
      x264_have_brdo ? CLAMP (x264->subq, 1, 6) : x264->subq,
      direct_name[CLAMP (x264->direct, DIRECT_NONE, DIRECT_AUTO)]);
  g_string_append_printf (options, ":frameref=%u", cartoon ? x264->frameref * 2 : x264->frameref);
  g_string_append_printf (options, ":b_adapt=%u", x264->b_adapt);

  if (passes > 1 && x264_have_nombtree)
    g_string_append (options, ":nombtree");

  if (x264_have_me)
  {
    g_string_append_printf (options, ":me=%s", me_name[CLAMP (x264->me, ME_DIA, ME_TESA)]);

    if (x264->me <= ME_HEX)
      g_string_append_printf (options, ":merange=%u", CLAMP (x264->merange, 4, 16));
    else
      g_string_append_printf (options, ":merange=%u", CLAMP (x264->merange, 4, G_MAXINT));
  }
  else
    g_string_append_printf (options, ":me=%u", x264->me);

  if (x264_have_brdo)
    g_string_append (options, x264->brdo ? ":brdo" : ":nobrdo");

  if (x264_have_lookahead)
    g_string_append_printf (options, ":rc_lookahead=%u", x264->rc_lookahead);

  bframes = ogmrip_video_codec_get_max_b_frames (video);
  g_string_append_printf (options, ":bframes=%d", cartoon ? bframes + 2 : bframes);

  if (pass != passes)
  {
    gboolean turbo;

    turbo = ogmrip_video_codec_get_turbo (video);
    if (x264_have_slow_firstpass && !turbo)
      g_string_append (options, ":slow_firstpass");
    else if (x264_have_turbo)
      g_string_append (options, turbo ? ":turbo=2" : ":turbo=1");
  }

  if (x264->trellis)
    g_string_append (options, quality == OGMRIP_QUALITY_EXTREME ? ":trellis=2" : ":trellis=1");
  else
    g_string_append (options, ":trellis=0");

  quality = ogmrip_video_codec_get_quality (video);
  if (quality == OGMRIP_QUALITY_USER)
  {
    g_string_append_printf (options, ":keyint=%u", x264->keyint);
    g_string_append_printf (options, ":cqm=%s", cqm_name[CLAMP (x264->cqm, 0, 1)]);

    g_string_append (options, x264->weight_b ? ":weight_b" : ":noweight_b");
    g_string_append (options, x264->global_header ? ":global_header" : ":noglobal_header");
    g_string_append (options, x264->cabac ? ":cabac" : ":nocabac");

    if (x264_have_weight_p)
      g_string_append_printf (options, ":weightp=%d", CLAMP (x264->weight_p, 0, 2));

    if (x264_have_8x8dct)
      g_string_append (options, x264->x88dct ? ":8x8dct" : ":no8x8dct");

    if (x264_have_mixed_refs)
      g_string_append (options, x264->mixed_refs ? ":mixed_refs" : ":nomixed_refs");

    if (x264->level_idc > 0)
      g_string_append_printf (options, ":level_idc=%d", CLAMP (x264->level_idc, 10, 51));

    if (x264_have_b_pyramid)
      g_string_append_printf (options, ":b_pyramid=%s", b_pyramid_name[CLAMP (x264->b_pyramid, B_PYRAMID_NONE, B_PYRAMID_NORMAL)]);
    else
      g_string_append (options, x264->b_pyramid ? ":b_pyramid" : ":nob_pyramid");

    if (x264->vbv_maxrate > 0 && x264->vbv_bufsize > 0)
      g_string_append_printf (options, ":vbv_maxrate=%d:vbv_bufsize=%d",
          x264->vbv_maxrate, x264->vbv_bufsize);

    if (x264_have_partitions && x264->partitions)
      g_string_append_printf (options, ":partitions=%s", x264->partitions);

    if (x264_have_bime && bframes > 0)
      g_string_append (options, ":bime");

    if (x264_have_psy && x264->subq >= 6)
    {
      gchar psy_rd[G_ASCII_DTOSTR_BUF_SIZE], psy_trellis[G_ASCII_DTOSTR_BUF_SIZE];

      g_ascii_formatd (psy_rd, G_ASCII_DTOSTR_BUF_SIZE, "%.2f", cartoon ? 0.4 : x264->psy_rd);
      g_ascii_formatd (psy_trellis, G_ASCII_DTOSTR_BUF_SIZE, "%.2f", cartoon ? 0 : x264->psy_trellis);

      if (x264->trellis)
        g_string_append_printf (options, ":psy-rd=%s,%s", psy_rd, psy_trellis);
      else
        g_string_append_printf (options, ":psy-rd=%s", psy_rd);
    }

    if (x264_have_aud && x264->aud)
      g_string_append (options, ":aud");
  }

  bitrate = ogmrip_video_codec_get_bitrate (video);
  if (bitrate > 0)
    g_string_append_printf (options, ":bitrate=%u", bitrate / 1000);
  else
  {
    gdouble quantizer;

    quantizer = ogmrip_video_codec_get_quantizer (video);
    if (quantizer == 0.0)
      g_string_append (options, ":qp=0");
    else
      g_string_append_printf (options, ":crf=%u", ogmrip_x264_get_crf (video, quantizer));
  }

  if (passes > 1 && log_file)
  {
    g_string_append_printf (options, ":pass=%u", pass == 1 ? 1 : pass == passes ? 2 : 3);

    g_ptr_array_add (argv, g_strdup ("-passlogfile"));
    g_ptr_array_add (argv, g_strdup (log_file));
  }
  
  threads = ogmrip_video_codec_get_threads (video);
  if (threads > 0)
    g_string_append_printf (options, ":threads=%u", CLAMP (threads, 1, 16));
  else
    g_string_append (options, ":threads=auto");

  g_ptr_array_add (argv, g_strdup ("-x264encopts"));
  g_ptr_array_add (argv, g_string_free (options, FALSE));

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

G_DEFINE_TYPE (OGMRipX264, ogmrip_x264, OGMRIP_TYPE_VIDEO_CODEC)

static void
ogmrip_x264_class_init (OGMRipX264Class *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;
  OGMRipVideoCodecClass *video_class;
  OGMRipCodecClass *codec_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = ogmrip_x264_finalize;
  gobject_class->get_property = ogmrip_x264_get_property;
  gobject_class->set_property = ogmrip_x264_set_property;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmrip_x264_run;

  video_class = OGMRIP_VIDEO_CODEC_CLASS (klass);
  video_class->get_start_delay = ogmrip_x264_get_start_delay;
  video_class->set_quality = ogmrip_x264_set_quality;

  codec_class = OGMRIP_CODEC_CLASS (klass);
  codec_class->set_profile = ogmrip_x264_set_profile;

  g_object_class_install_property (gobject_class, PROP_8X8DCT,
      g_param_spec_boolean (OGMRIP_X264_PROP_8X8DCT,
        "8x8 dct property", "Set 8x8 dct", OGMRIP_X264_DEFAULT_8X8DCT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_AUD,
      g_param_spec_boolean (OGMRIP_X264_PROP_AUD,
        "Aud property", "Set aud", OGMRIP_X264_DEFAULT_AUD, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_B_ADAPT,
      g_param_spec_uint (OGMRIP_X264_PROP_B_ADAPT,
        "B adapt", "Set b adapt", 0, 2, OGMRIP_X264_DEFAULT_B_ADAPT, G_PARAM_READWRITE));

  if (x264_have_b_pyramid)
    g_object_class_install_property (gobject_class, PROP_B_PYRAMID,
        g_param_spec_uint (OGMRIP_X264_PROP_B_PYRAMID,
          "B pyramid property", "Set b pyramid", B_PYRAMID_NONE, B_PYRAMID_NORMAL, OGMRIP_X264_DEFAULT_B_PYRAMID, G_PARAM_READWRITE));
  else
    g_object_class_install_property (gobject_class, PROP_B_PYRAMID,
        g_param_spec_uint (OGMRIP_X264_PROP_B_PYRAMID,
          "B pyramid property", "Set b pyramid", B_PYRAMID_NONE, B_PYRAMID_STRICT, x264_have_b_pyramid ? OGMRIP_X264_DEFAULT_B_PYRAMID : TRUE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BRDO,
      g_param_spec_boolean (OGMRIP_X264_PROP_BRDO,
        "Brdo property", "Set brdo", OGMRIP_X264_DEFAULT_BRDO, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CABAC,
      g_param_spec_boolean (OGMRIP_X264_PROP_CABAC,
        "Cabac property", "Set cabac", OGMRIP_X264_DEFAULT_CABAC, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CQM,
      g_param_spec_uint (OGMRIP_X264_PROP_CQM,
        "Cqm property", "Set cqm", 0, 1, OGMRIP_X264_DEFAULT_CQM, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DIRECT,
      g_param_spec_uint (OGMRIP_X264_PROP_DIRECT,
        "Direct property", "Set direct", DIRECT_NONE, DIRECT_AUTO, OGMRIP_X264_DEFAULT_DIRECT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FRAMEREF,
      g_param_spec_uint (OGMRIP_X264_PROP_FRAMEREF,
        "Frameref property", "Set frameref", 1, 16, OGMRIP_X264_DEFAULT_FRAMEREF, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_GLOBAL_HEADER,
      g_param_spec_boolean (OGMRIP_X264_PROP_GLOBAL_HEADER,
        "global header property", "Set global header", OGMRIP_X264_DEFAULT_GLOBAL_HEADER, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_KEYINT,
      g_param_spec_uint (OGMRIP_X264_PROP_KEYINT,
        "Keyint property", "Set keyint", 0, G_MAXUINT, OGMRIP_X264_DEFAULT_KEYINT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LEVEL_IDC,
      g_param_spec_uint (OGMRIP_X264_PROP_LEVEL_IDC,
        "Level IDC property", "Set level IDC", 0, 51, OGMRIP_X264_DEFAULT_LEVEL_IDC, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ME,
      g_param_spec_uint (OGMRIP_X264_PROP_ME,
        "Motion estimation property", "Set motion estimation", ME_DIA,
        x264_have_me_tesa ? ME_TESA : ME_ESA, OGMRIP_X264_DEFAULT_ME, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MERANGE,
      g_param_spec_uint (OGMRIP_X264_PROP_MERANGE,
        "Motion estimation range property", "Set motion estimation range", 4, G_MAXUINT, OGMRIP_X264_DEFAULT_MERANGE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MIXED_REFS,
      g_param_spec_boolean (OGMRIP_X264_PROP_MIXED_REFS,
        "Mixed refs property", "Set mixed refs", OGMRIP_X264_DEFAULT_MIXED_REFS, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PARTITIONS,
      g_param_spec_string (OGMRIP_X264_PROP_PARTITIONS,
        "Partitions property", "Set partitions", OGMRIP_X264_DEFAULT_PARTITIONS, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PSY_RD,
      g_param_spec_double (OGMRIP_X264_PROP_PSY_RD,
        "Psy RD property", "Set psy-rd", 0.0, G_MAXDOUBLE, OGMRIP_X264_DEFAULT_PSY_RD, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PSY_TRELLIS,
      g_param_spec_double (OGMRIP_X264_PROP_PSY_TRELLIS,
        "Psy trellis property", "Set psy-trellis", 0.0, G_MAXDOUBLE, OGMRIP_X264_DEFAULT_PSY_TRELLIS, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RC_LOOKAHEAD,
      g_param_spec_uint (OGMRIP_X264_PROP_RC_LOOKAHEAD,
        "RC look ahead property", "Set rc lookahead", 0, 250, OGMRIP_X264_DEFAULT_RC_LOOKAHEAD, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SUBQ,
      g_param_spec_uint (OGMRIP_X264_PROP_SUBQ,
        "Subpel quality property", "Set subpel quality", 0, 10, OGMRIP_X264_DEFAULT_SUBQ, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TRELLIS,
      g_param_spec_uint (OGMRIP_X264_PROP_TRELLIS,
        "Trellis property", "Set trellis", 0, 2, OGMRIP_X264_DEFAULT_TRELLIS, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_VBV_BUFSIZE,
      g_param_spec_uint (OGMRIP_X264_PROP_VBV_BUFSIZE,
        "Buffer size property", "Set buffer size", 0, G_MAXUINT, OGMRIP_X264_DEFAULT_VBV_BUFSIZE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_VBV_MAXRATE,
      g_param_spec_uint (OGMRIP_X264_PROP_VBV_MAXRATE,
        "Max rate property", "Set max rate", 0, G_MAXUINT, OGMRIP_X264_DEFAULT_VBV_MAXRATE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_WEIGHT_B,
      g_param_spec_boolean (OGMRIP_X264_PROP_WEIGHT_B,
        "Weight B property", "Set weight B", OGMRIP_X264_DEFAULT_WEIGHT_B, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_WEIGHT_P,
      g_param_spec_uint (OGMRIP_X264_PROP_WEIGHT_P,
        "Weight P property", "Set weight P", 0, 2, OGMRIP_X264_DEFAULT_WEIGHT_P, G_PARAM_READWRITE));
}

static void
ogmrip_x264_init (OGMRipX264 *x264)
{
  x264->aud = OGMRIP_X264_DEFAULT_AUD;
  x264->b_adapt = OGMRIP_X264_DEFAULT_B_ADAPT;
  x264->b_pyramid = OGMRIP_X264_DEFAULT_B_PYRAMID;
  x264->brdo = OGMRIP_X264_DEFAULT_BRDO;
  x264->cabac = OGMRIP_X264_DEFAULT_CABAC;
  x264->cqm = OGMRIP_X264_DEFAULT_CQM;
  x264->direct = OGMRIP_X264_DEFAULT_DIRECT;
  x264->frameref = OGMRIP_X264_DEFAULT_FRAMEREF;
  x264->global_header = OGMRIP_X264_DEFAULT_GLOBAL_HEADER;
  x264->keyint = OGMRIP_X264_DEFAULT_KEYINT;
  x264->level_idc = OGMRIP_X264_DEFAULT_LEVEL_IDC;
  x264->me = OGMRIP_X264_DEFAULT_ME;
  x264->merange = OGMRIP_X264_DEFAULT_MERANGE;
  x264->mixed_refs = OGMRIP_X264_DEFAULT_MIXED_REFS;
  x264->psy_rd = OGMRIP_X264_DEFAULT_PSY_RD;
  x264->psy_trellis = OGMRIP_X264_DEFAULT_PSY_TRELLIS;
  x264->rc_lookahead = OGMRIP_X264_DEFAULT_RC_LOOKAHEAD;
  x264->subq = OGMRIP_X264_DEFAULT_SUBQ;
  x264->trellis = OGMRIP_X264_DEFAULT_TRELLIS;
  x264->vbv_bufsize = OGMRIP_X264_DEFAULT_VBV_BUFSIZE;
  x264->vbv_maxrate = OGMRIP_X264_DEFAULT_VBV_MAXRATE;
  x264->weight_b = OGMRIP_X264_DEFAULT_WEIGHT_B;
  x264->weight_p = OGMRIP_X264_DEFAULT_WEIGHT_P;
  x264->x88dct = OGMRIP_X264_DEFAULT_8X8DCT;
}

static void
ogmrip_x264_finalize (GObject *gobject)
{
  OGMRipX264 *x264 = OGMRIP_X264 (gobject);

  g_free (x264->partitions);

  G_OBJECT_CLASS (ogmrip_x264_parent_class)->finalize (gobject);
}

static void
ogmrip_x264_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipX264 *x264 = OGMRIP_X264 (gobject);

  switch (property_id) 
  {
    case PROP_8X8DCT:
      g_value_set_boolean (value, x264->x88dct);
      break;
    case PROP_AUD:
      g_value_set_boolean (value, x264->aud);
      break;
    case PROP_B_ADAPT:
      g_value_set_uint (value, x264->b_adapt);
      break;
    case PROP_B_PYRAMID:
      g_value_set_uint (value, x264->b_pyramid);
      break;
    case PROP_BRDO:
      g_value_set_boolean (value, x264->brdo);
      break;
    case PROP_CABAC:
      g_value_set_boolean (value, x264->cabac);
      break;
    case PROP_CQM:
      g_value_set_uint (value, x264->cqm);
      break;
    case PROP_DIRECT:
      g_value_set_uint (value, x264->direct);
      break;
    case PROP_FRAMEREF:
      g_value_set_uint (value, x264->frameref);
      break;
    case PROP_GLOBAL_HEADER:
      g_value_set_boolean (value, x264->global_header);
      break;
    case PROP_KEYINT:
      g_value_set_uint (value, x264->keyint);
      break;
    case PROP_LEVEL_IDC:
      g_value_set_uint (value, x264->level_idc);
      break;
    case PROP_ME:
      g_value_set_uint (value, x264->me);
      break;
    case PROP_MERANGE:
      g_value_set_uint (value, x264->merange);
      break;
    case PROP_MIXED_REFS:
      g_value_set_boolean (value, x264->mixed_refs);
      break;
    case PROP_PARTITIONS:
      g_value_set_string (value, x264->partitions);
      break;
    case PROP_PSY_RD:
      g_value_set_double (value, x264->psy_rd);
      break;
    case PROP_PSY_TRELLIS:
      g_value_set_double (value, x264->psy_trellis);
      break;
    case PROP_RC_LOOKAHEAD:
      g_value_set_uint (value, x264->rc_lookahead);
      break;
    case PROP_SUBQ:
      g_value_set_uint (value, x264->subq);
      break;
    case PROP_TRELLIS:
      g_value_set_uint (value, x264->trellis);
      break;
    case PROP_VBV_BUFSIZE:
      g_value_set_uint (value, x264->vbv_bufsize);
      break;
    case PROP_VBV_MAXRATE:
      g_value_set_uint (value, x264->vbv_maxrate);
      break;
    case PROP_WEIGHT_B:
      g_value_set_boolean (value, x264->weight_b);
      break;
    case PROP_WEIGHT_P:
      g_value_set_uint (value, x264->weight_p);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_x264_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipX264 *x264 = OGMRIP_X264 (gobject);

  switch (property_id) 
  {
    case PROP_8X8DCT:
      x264->x88dct = g_value_get_boolean (value);
      break;
    case PROP_AUD:
      x264->aud = g_value_get_boolean (value);
      break;
    case PROP_B_ADAPT:
      x264->b_adapt = g_value_get_uint (value);
      break;
    case PROP_B_PYRAMID:
      x264->b_pyramid = g_value_get_uint (value);
      break;
    case PROP_BRDO:
      x264->brdo = g_value_get_boolean (value);
      break;
    case PROP_CABAC:
      x264->cabac = g_value_get_boolean (value);
      break;
    case PROP_CQM:
      x264->cqm = g_value_get_uint (value);
      break;
    case PROP_DIRECT:
      x264->direct = g_value_get_uint (value);
      break;
    case PROP_FRAMEREF:
      x264->frameref = g_value_get_uint (value);
      break;
    case PROP_GLOBAL_HEADER:
      x264->global_header = g_value_get_boolean (value);
      break;
    case PROP_KEYINT:
      x264->keyint = g_value_get_uint (value);
      break;
    case PROP_LEVEL_IDC:
      x264->level_idc = g_value_get_uint (value);
      break;
    case PROP_ME:
      x264->me = g_value_get_uint (value);
      break;
    case PROP_MERANGE:
      x264->merange = g_value_get_uint (value);
      break;
    case PROP_MIXED_REFS:
      x264->mixed_refs = g_value_get_boolean (value);
      break;
    case PROP_PARTITIONS:
      g_free (x264->partitions);
      x264->partitions = g_value_dup_string (value);
      break;
    case PROP_PSY_RD:
      x264->psy_rd = g_value_get_double (value);
      break;
    case PROP_PSY_TRELLIS:
      x264->psy_trellis = g_value_get_double (value);
      break;
    case PROP_RC_LOOKAHEAD:
      x264->rc_lookahead = g_value_get_uint (value);
      break;
    case PROP_SUBQ:
      x264->subq = g_value_get_uint (value);
      break;
    case PROP_TRELLIS:
      x264->trellis = g_value_get_uint (value);
      break;
    case PROP_VBV_BUFSIZE:
      x264->vbv_bufsize = g_value_get_uint (value);
      break;
    case PROP_VBV_MAXRATE:
      x264->vbv_maxrate = g_value_get_uint (value);
      break;
    case PROP_WEIGHT_B:
      x264->weight_b = g_value_get_boolean (value);
      break;
    case PROP_WEIGHT_P:
      x264->weight_p = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static gint
ogmrip_x264_run (OGMJobSpawn *spawn)
{
  OGMJobSpawn *queue, *child;
  gchar **argv, *log_file, *mbtree_file;
  gint pass, passes, result;

  queue = ogmjob_queue_new ();
  ogmjob_container_add (OGMJOB_CONTAINER (spawn), queue);
  g_object_unref (queue);

  passes = ogmrip_video_codec_get_passes (OGMRIP_VIDEO_CODEC (spawn));

  log_file = NULL;
  if (passes > 1)
    log_file = ogmrip_fs_mktemp ("log.XXXXXX", NULL);

  for (pass = 0; pass < passes; pass ++)
  {
    argv = ogmrip_x264_command (OGMRIP_VIDEO_CODEC (spawn), pass + 1, passes, log_file);
    if (!argv)
      return OGMJOB_RESULT_ERROR;

    child = ogmjob_exec_newv (argv);
    ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_mencoder_codec_watch, spawn, TRUE, FALSE, FALSE);
    ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
    g_object_unref (child);
  }

  result = OGMJOB_SPAWN_CLASS (ogmrip_x264_parent_class)->run (spawn);

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), queue);

  mbtree_file = g_strconcat (log_file, ".mbtree", NULL);
  if (g_file_test (mbtree_file, G_FILE_TEST_EXISTS))
    g_unlink (mbtree_file);
  g_free (mbtree_file);

  g_unlink (log_file);
  g_free (log_file);

  return result;
}

static gint
ogmrip_x264_get_start_delay (OGMRipVideoCodec *video)
{
  if (ogmrip_video_codec_get_max_b_frames (video) > 0)
    return 2;
  
  return 1;
}

static void
ogmrip_x264_set_default_values (OGMRipX264 *x264)
{
  ogmrip_x264_init (x264);

  ogmrip_video_codec_set_max_b_frames (OGMRIP_VIDEO_CODEC (x264), OGMRIP_X264_DEFAULT_B_FRAMES);
}

static void
ogmrip_x264_set_quality (OGMRipVideoCodec *video, OGMRipQualityType quality)
{
  OGMRipX264 *x264;

  x264 = OGMRIP_X264 (video);
  ogmrip_x264_set_default_values (x264);

  switch (quality)
  {
    case OGMRIP_QUALITY_EXTREME:
      x264->b_adapt = 2;
      x264->brdo = TRUE;
      x264->direct = DIRECT_AUTO;
      x264->frameref = 16;
      x264->me = ME_UMH;
      x264->merange = 24;
      x264->rc_lookahead = 60;
      x264->subq = 10;
      ogmrip_video_codec_set_max_b_frames (OGMRIP_VIDEO_CODEC (x264), 8);
      break;
    case OGMRIP_QUALITY_HIGH:
      x264->b_adapt = 2;
      x264->direct = DIRECT_AUTO;
      x264->frameref = 5;
      x264->me = ME_UMH;
      x264->rc_lookahead = 50;
      x264->subq = 8;
      break;
    default:
      break;
  }
}

static OGMRipVideoPlugin x264_plugin =
{
  NULL,
  G_TYPE_NONE,
  "x264",
  N_("X264"),
  OGMRIP_FORMAT_H264,
  G_MAXINT,
  16
};

static gboolean
ogmrip_x264_check_option (const gchar *option)
{
  GPtrArray *argv;
  gchar *options, *output = NULL;
  gint status;

  argv = g_ptr_array_new ();

  g_ptr_array_add (argv, "mencoder");
  g_ptr_array_add (argv, "-nocache");
  g_ptr_array_add (argv, "-nosound");
  g_ptr_array_add (argv, "-quiet");
  g_ptr_array_add (argv, "-frames");
  g_ptr_array_add (argv, "1");
  g_ptr_array_add (argv, "-rawvideo");
  g_ptr_array_add (argv, "pal:fps=25");
  g_ptr_array_add (argv, "-demuxer");
  g_ptr_array_add (argv, "rawvideo");
  g_ptr_array_add (argv, "-o");
  g_ptr_array_add (argv, "/dev/null");
  g_ptr_array_add (argv, "-ovc");
  g_ptr_array_add (argv, "x264");
  g_ptr_array_add (argv, "-x264encopts");

  options = g_strdup_printf ("%s:bitrate=800:threads=1", option);
  g_ptr_array_add (argv, options);

  g_ptr_array_add (argv, "/dev/zero");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (gchar **) argv->pdata, NULL,
      G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL,
      NULL, NULL, NULL, &output, &status, NULL);
  
  g_ptr_array_free (argv, TRUE);

  g_free (options);

  if (status == 0 && output != NULL)
  {
    gchar *substr;

    substr = g_strdup_printf ("Option x264encopts: Unknown suboption %s", option);
    if (strstr (output, substr))
      status = 1;
    g_free (substr);
  }

  if (output)
    g_free (output);

  return status == 0;
}

static void
ogmrip_x264_set_profile (OGMRipCodec *codec, OGMRipProfile *profile)
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
      key = ogmrip_settings_build_section (settings, OGMRIP_X264_SECTION, properties[i], NULL);
      ogmrip_settings_set_property_from_key (settings, G_OBJECT (codec), properties[i], section, key);
      g_free (key);
    }
  }
*/
}

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

  match = g_regex_match_simple ("^ *x264 *- .*$", output, G_REGEX_MULTILINE, 0);
  g_free (output);

  if (!match)
  {
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("MEncoder is build without X264 support"));
    return NULL;
  }

  x264_have_8x8dct         = ogmrip_x264_check_option ("8x8dct");
  x264_have_aud            = ogmrip_x264_check_option ("aud");
  x264_have_bime           = ogmrip_x264_check_option ("bime");
  x264_have_b_pyramid      = ogmrip_x264_check_option ("b_pyramid=none");
  x264_have_brdo           = ogmrip_x264_check_option ("brdo");
  x264_have_lookahead      = ogmrip_x264_check_option ("rc_lookahead=40");
  x264_have_me             = ogmrip_x264_check_option ("me=hex");
  x264_have_me_tesa        = ogmrip_x264_check_option ("me=tesa");
  x264_have_mixed_refs     = ogmrip_x264_check_option ("mixed_refs");
  x264_have_partitions     = ogmrip_x264_check_option ("partitions=all");
  x264_have_psy            = ogmrip_x264_check_option ("psy-rd=1,1");
  x264_have_turbo          = ogmrip_x264_check_option ("turbo=2");
  x264_have_weight_p       = ogmrip_x264_check_option ("weightp=2");
  x264_have_slow_firstpass = ogmrip_x264_check_option ("slow_firstpass");
  x264_have_nombtree       = ogmrip_x264_check_option ("nombtree");
/*
  settings = ogmrip_settings_get_default ();
  if (settings)
  {
    GObjectClass *klass;
    guint i;

    klass = g_type_class_ref (OGMRIP_TYPE_X264);

    for (i = 0; properties[i]; i++)
      ogmrip_settings_install_key_from_property (settings, klass,
          OGMRIP_X264_SECTION, properties[i], properties[i]);

    g_type_class_unref (klass);
  }
*/
  x264_plugin.type = OGMRIP_TYPE_X264;

  return &x264_plugin;
}

