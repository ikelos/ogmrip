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

/**
 * SECTION:ogmrip-video-codec
 * @title: OGMRipVideoCodec
 * @short_description: Base class for video codecs
 * @include: ogmrip-video-codec.h
 */

#include "ogmrip-video-codec.h"
#include "ogmrip-version.h"

#include <ogmjob-exec.h>
#include <ogmjob-queue.h>

#include <string.h>
#include <stdio.h>

#define OGMRIP_VIDEO_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_VIDEO_CODEC, OGMRipVideoCodecPriv))

#define ROUND(x) ((gint) ((x) + 0.5) != (gint) (x) ? ((gint) ((x) + 0.5)) : ((gint) (x)))
#define FLOOR(x) ((gint) (x))

#define CROP_FRAMES    100
#define ANALYZE_FRAMES 500

struct _OGMRipVideoCodecPriv
{
  gdouble bpp;
  gdouble quantizer;
  gint bitrate;
  guint angle;
  guint passes;
  guint threads;
  guint crop_x;
  guint crop_y;
  guint crop_width;
  guint crop_height;
  guint scale_width;
  guint scale_height;
  guint max_width;
  guint max_height;
  guint min_width;
  guint min_height;
  guint aspect_num;
  guint aspect_denom;
  guint max_b_frames;
  gboolean denoise;
  gboolean deblock;
  gboolean dering;
  gboolean turbo;
  gboolean expand;

  OGMDvdAudioStream *astream;
  OGMDvdSubpStream *sstream;
  OGMRipQualityType quality;
  OGMRipScalerType scaler;
  OGMRipDeintType deint;

  OGMJobSpawn *child;
  gboolean child_canceled;
  gboolean forced_subs;
  gint interlaced;
};

typedef struct
{
  gint val;
  gint ref;
} UInfo;

typedef struct
{
  guint nframes;
  guint frames;
  GSList *x;
  GSList *y;
  GSList *w;
  GSList *h;
} OGMRipCrop;

typedef struct
{
  gchar *cur_affinity;
  gchar* prev_affinity;
  guint naffinities;
  guint cur_duration;
  guint prev_duration;
  guint npatterns;
  guint cur_section;
  guint nsections;
  guint nframes;
  guint frames;
} OGMRipAnalyze;

enum
{
  SECTION_UNKNOWN,
  SECTION_24000_1001,
  SECTION_30000_1001
};

enum 
{
  PROP_0,
  PROP_ANGLE,
  PROP_BITRATE,
  PROP_QUANTIZER,
  PROP_BPP,
  PROP_PASSES,
  PROP_THREADS,
  PROP_TURBO,
  PROP_DENOISE,
  PROP_BFRAMES,
  PROP_DEBLOCK,
  PROP_DERING
};

static void ogmrip_video_codec_dispose      (GObject      *gobject);
static void ogmrip_video_codec_set_property (GObject      *gobject,
                                             guint        property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void ogmrip_video_codec_get_property (GObject      *gobject,
                                             guint        property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);
static void ogmrip_video_codec_cancel       (OGMJobSpawn  *spawn);

static const gchar *deinterlacer[] = { "lb", "li", "ci", "md", "fd", "l5", "kerndeint", "yadif=0" };

static gint
g_ulist_compare (UInfo *info, gint val)
{
  return info->val - val;
}

static gint
g_ulist_min (UInfo *info1, UInfo *info2)
{
  return info2->val - info1->val;
}

static gint
g_ulist_max (UInfo *info1, UInfo *info2)
{
  return info1->val - info2->val;
}

static GSList *
g_ulist_add (GSList *ulist, GCompareFunc func, gint val)
{
  GSList *ulink;
  UInfo *info;

  ulink = g_slist_find_custom (ulist, GINT_TO_POINTER (val), (GCompareFunc) g_ulist_compare);
  if (ulink)
  {
    info = ulink->data;
    info->ref ++;
  }
  else
  {
    info = g_new0 (UInfo, 1);
    info->val = val;
    info->ref = 1;

    ulist = g_slist_insert_sorted (ulist, info, func);
  }

  return ulist;
}

static gint
g_ulist_get_most_frequent (GSList *ulist)
{
  GSList *ulink;
  UInfo *info, *umax;

  if (!ulist)
    return 0;

  umax = ulist->data;

  for (ulink = ulist; ulink; ulink = ulink->next)
  {
    info = ulink->data;

    if (info->ref > umax->ref)
      umax = info;
  }

  return umax->val;
}

static void
g_ulist_free (GSList *ulist)
{
  g_slist_foreach (ulist, (GFunc) g_free, NULL);
  g_slist_free (ulist);
}

static gdouble
ogmrip_video_codec_crop_watch (OGMJobExec *exec, const gchar *buffer, OGMRipCrop *info)
{
  gchar *str;

  static guint frame = 0;

  str = strstr (buffer, "-vf crop=");
  if (str)
  {
    gint x, y, w, h;

    if (sscanf (str, "-vf crop=%d:%d:%d:%d", &w, &h, &x, &y) == 4)
    {
      if (w > 0)
        info->w = g_ulist_add (info->w, (GCompareFunc) g_ulist_min, w);
      if (h > 0)
        info->h = g_ulist_add (info->h, (GCompareFunc) g_ulist_min, h);
      if (x > 0)
        info->x = g_ulist_add (info->x, (GCompareFunc) g_ulist_max, x);
      if (y > 0)
        info->y = g_ulist_add (info->y, (GCompareFunc) g_ulist_max, y);
    }

    frame ++;
    if (frame == info->nframes - 2)
    {
      frame = 0;
      return 1.0;
    }

    return frame / (gdouble) (info->nframes - 2);
  }
  else
  {
    gdouble d;

    if (sscanf (buffer, "V: %lf", &d))
    {
      info->frames ++;

      if (info->frames >= CROP_FRAMES)
        ogmjob_spawn_cancel (OGMJOB_SPAWN (exec));
    }
  }

  return -1.0;
}

static gdouble
ogmrip_video_codec_analyze_watch (OGMJobExec *exec, const gchar *buffer, OGMRipAnalyze *info)
{
  if (g_str_has_prefix (buffer, "V: "))
  {
    info->frames ++;

    if (info->frames == info->nframes)
      return 1.0;

    return info->frames / (gdouble) info->nframes;
  }
  else
  {
    if (g_str_has_prefix (buffer, "demux_mpg: 24000/1001"))
    {
      info->cur_section = SECTION_24000_1001;
      info->nsections ++;
    }
    else if (g_str_has_prefix (buffer, "demux_mpg: 30000/1001"))
    {
      info->cur_section = SECTION_30000_1001;
      info->nsections ++;
    }

    if (info->cur_section == SECTION_30000_1001)
    {
      if (g_str_has_prefix (buffer, "affinity: "))
      {
        g_free (info->prev_affinity);
        info->prev_affinity = g_strdup (info->cur_affinity);

        g_free (info->cur_affinity);
        info->cur_affinity = g_strdup (buffer + 10);
      }
      else if (g_str_has_prefix (buffer, "duration: "))
      {
        info->prev_duration = info->cur_duration;
        sscanf (buffer, "duration: %u", &info->cur_duration);

        if (info->prev_duration == 3 && info->cur_duration == 2)
        {
          info->npatterns ++;

          if (strncmp (info->prev_affinity, ".0+.1.+2", 8) == 0 && strncmp (info->cur_affinity, ".0++1", 5) == 0)
            info->naffinities ++;
        }
      }
    }
  }

  return -1.0;
}

static gchar **
ogmrip_video_codec_crop_command (OGMRipVideoCodec *video, gdouble start, gulong nframes)
{
  OGMDvdTitle *title;
  OGMRipDeintType deint;
  GPtrArray *argv;

  GString *filter;
  const gchar *device;
  gint vid;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), NULL);

  title = ogmrip_codec_get_input (OGMRIP_CODEC (video));
  g_return_val_if_fail (title != NULL, NULL);

  argv = g_ptr_array_new ();

  if (MPLAYER_CHECK_VERSION (1,0,0,8))
  {
    g_ptr_array_add (argv, g_strdup ("mplayer"));
    g_ptr_array_add (argv, g_strdup ("-nolirc"));
    g_ptr_array_add (argv, g_strdup ("-vo"));
    g_ptr_array_add (argv, g_strdup ("null"));
  }
  else
  {
    g_ptr_array_add (argv, g_strdup ("mencoder"));
    g_ptr_array_add (argv, g_strdup ("-ovc"));
    g_ptr_array_add (argv, g_strdup ("lavc"));
    g_ptr_array_add (argv, g_strdup ("-o"));
    g_ptr_array_add (argv, g_strdup ("/dev/null"));
  }

  g_ptr_array_add (argv, g_strdup ("-nosound"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));

  if (ogmrip_check_mplayer_nosub ())
    g_ptr_array_add (argv, g_strdup ("-nosub"));

  if (MPLAYER_CHECK_VERSION (1,0,3,0))
  {
    g_ptr_array_add (argv, g_strdup ("-noconfig"));
    g_ptr_array_add (argv, g_strdup ("all"));
  }

  g_ptr_array_add (argv, g_strdup ("-speed"));
  g_ptr_array_add (argv, g_strdup ("100"));

  g_ptr_array_add (argv, g_strdup ("-dvdangle"));
  g_ptr_array_add (argv, g_strdup_printf ("%d",
        ogmrip_video_codec_get_angle (video)));

  filter = g_string_new (NULL);

  deint = ogmrip_video_codec_get_deinterlacer (video);
  if (deint != OGMRIP_DEINT_NONE)
  {
    if (deint == OGMRIP_DEINT_KERNEL || OGMRIP_DEINT_YADIF)
      g_string_append (filter, deinterlacer[deint - 1]);
    else
      g_string_append_printf (filter, "pp=%s", deinterlacer[deint - 1]);
  }

  if (filter->len > 0)
    g_string_append_c (filter, ',');
  g_string_append (filter, "cropdetect");

  g_ptr_array_add (argv, g_strdup ("-vf"));
  g_ptr_array_add (argv, g_string_free (filter, FALSE));

  g_ptr_array_add (argv, g_strdup ("-ss"));
  g_ptr_array_add (argv, g_strdup_printf ("%.0lf", start));

  g_ptr_array_add (argv, g_strdup ("-frames"));
  g_ptr_array_add (argv, g_strdup_printf ("%lu", nframes));

  device = ogmdvd_disc_get_device (ogmdvd_title_get_disc (title));
  g_ptr_array_add (argv, g_strdup ("-dvd-device"));
  g_ptr_array_add (argv, g_strdup (device));

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

static gchar **
ogmrip_video_codec_analyze_command (OGMRipVideoCodec *video, gulong nframes)
{
  OGMDvdTitle *title;
  GPtrArray *argv;

  const gchar *device;
  gint vid;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), NULL);

  title = ogmrip_codec_get_input (OGMRIP_CODEC (video));
  g_return_val_if_fail (title != NULL, NULL);

  argv = g_ptr_array_new ();

  g_ptr_array_add (argv, g_strdup ("mplayer"));
  g_ptr_array_add (argv, g_strdup ("-nolirc"));
  g_ptr_array_add (argv, g_strdup ("-nosound"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));

  if (ogmrip_check_mplayer_nosub ())
    g_ptr_array_add (argv, g_strdup ("-nosub"));

  if (MPLAYER_CHECK_VERSION (1,0,3,0))
  {
    g_ptr_array_add (argv, g_strdup ("-noconfig"));
    g_ptr_array_add (argv, g_strdup ("all"));
  }

  g_ptr_array_add (argv, g_strdup ("-v"));
  g_ptr_array_add (argv, g_strdup ("-benchmark"));

  g_ptr_array_add (argv, g_strdup ("-vo"));
  g_ptr_array_add (argv, g_strdup ("null"));

  g_ptr_array_add (argv, g_strdup ("-vf"));
  g_ptr_array_add (argv, g_strdup ("pullup"));

  g_ptr_array_add (argv, g_strdup ("-dvdangle"));
  g_ptr_array_add (argv, g_strdup_printf ("%d",
        ogmrip_video_codec_get_angle (video)));

  g_ptr_array_add (argv, g_strdup ("-frames"));
  g_ptr_array_add (argv, g_strdup_printf ("%lu", nframes));

  device = ogmdvd_disc_get_device (ogmdvd_title_get_disc (title));
  g_ptr_array_add (argv, g_strdup ("-dvd-device"));
  g_ptr_array_add (argv, g_strdup (device));

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

G_DEFINE_ABSTRACT_TYPE (OGMRipVideoCodec, ogmrip_video_codec, OGMRIP_TYPE_CODEC)

static void
ogmrip_video_codec_class_init (OGMRipVideoCodecClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_video_codec_dispose;
  gobject_class->set_property = ogmrip_video_codec_set_property;
  gobject_class->get_property = ogmrip_video_codec_get_property;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->cancel = ogmrip_video_codec_cancel;

  g_object_class_install_property (gobject_class, PROP_ANGLE, 
        g_param_spec_uint ("angle", "Angle property", "Set angle", 
           1, G_MAXUINT, 1, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BITRATE, 
        g_param_spec_uint ("bitrate", "Bitrate property", "Set bitrate", 
           4000, 24000000, 800000, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_QUANTIZER, 
        g_param_spec_double ("quantizer", "Quantizer property", "Set quantizer", 
           -1.0, 31.0, -1.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BPP, 
        g_param_spec_double ("bpp", "Bits per pixel property", "Set bits per pixel", 
           0.0, 1.0, 0.25, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PASSES, 
        g_param_spec_uint ("passes", "Passes property", "Set the number of passes", 
           1, G_MAXUINT, 1, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_THREADS, 
        g_param_spec_uint ("threads", "Threads property", "Set the number of threads", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TURBO, 
        g_param_spec_boolean ("turbo", "Turbo property", "Set turbo", 
           FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DENOISE, 
        g_param_spec_boolean ("denoise", "Denoise property", "Set denoise", 
           FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BFRAMES, 
        g_param_spec_uint ("bframes", "B frames property", "Set b-frames", 
           0, 4, 2, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DEBLOCK, 
        g_param_spec_boolean ("deblock", "Deblock property", "Set deblock", 
           TRUE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DERING, 
        g_param_spec_boolean ("dering", "Dering property", "Set dering", 
           TRUE, G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (OGMRipVideoCodecPriv));
}

static void
ogmrip_video_codec_init (OGMRipVideoCodec *video)
{
  video->priv = OGMRIP_VIDEO_GET_PRIVATE (video);
  video->priv->scaler = OGMRIP_SCALER_GAUSS;
  video->priv->astream = NULL;
  video->priv->bitrate = 800000;
  video->priv->quantizer = -1.0;
  video->priv->turbo = FALSE;
  video->priv->max_b_frames = 2;
  video->priv->angle = 1;
  video->priv->bpp = 0.25;
  video->priv->passes = 1;
  video->priv->interlaced = -1;
}

static void
ogmrip_video_codec_dispose (GObject *gobject)
{
  OGMRipVideoCodec *video;

  video = OGMRIP_VIDEO_CODEC (gobject);

  if (video->priv->astream)
    ogmdvd_stream_unref (OGMDVD_STREAM (video->priv->astream));
  video->priv->astream = NULL;

  if (video->priv->sstream)
    ogmdvd_stream_unref (OGMDVD_STREAM (video->priv->sstream));
  video->priv->sstream = NULL;

  G_OBJECT_CLASS (ogmrip_video_codec_parent_class)->dispose (gobject);
}

static void
ogmrip_video_codec_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipVideoCodec *video;

  video = OGMRIP_VIDEO_CODEC (gobject);

  switch (property_id) 
  {
    case PROP_ANGLE:
      ogmrip_video_codec_set_angle (video, g_value_get_uint (value));
      break;
    case PROP_BITRATE:
      ogmrip_video_codec_set_bitrate (video, g_value_get_uint (value));
      break;
    case PROP_QUANTIZER:
      ogmrip_video_codec_set_quantizer (video, g_value_get_double (value));
      break;
    case PROP_BPP:
      ogmrip_video_codec_set_bits_per_pixel (video, g_value_get_double (value));
      break;
    case PROP_PASSES:
      ogmrip_video_codec_set_passes (video, g_value_get_uint (value));
      break;
    case PROP_THREADS:
      ogmrip_video_codec_set_threads (video, g_value_get_uint (value));
      break;
    case PROP_TURBO:
      ogmrip_video_codec_set_turbo (video, g_value_get_boolean (value));
      break;
    case PROP_DENOISE:
      ogmrip_video_codec_set_denoise (video, g_value_get_boolean (value));
      break;
    case PROP_BFRAMES:
      ogmrip_video_codec_set_max_b_frames (video, g_value_get_uint (value));
      break;
    case PROP_DEBLOCK:
      ogmrip_video_codec_set_deblock (video, g_value_get_boolean (value));
      break;
    case PROP_DERING:
      ogmrip_video_codec_set_dering (video, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_video_codec_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipVideoCodec *video;

  video = OGMRIP_VIDEO_CODEC (gobject);

  switch (property_id) 
  {
    case PROP_ANGLE:
      g_value_set_uint (value, video->priv->angle);
      break;
    case PROP_BITRATE:
      g_value_set_uint (value, video->priv->bitrate);
      break;
    case PROP_QUANTIZER:
      g_value_set_double (value, video->priv->quantizer);
      break;
    case PROP_BPP:
      g_value_set_double (value, video->priv->bpp);
      break;
    case PROP_PASSES:
      g_value_set_uint (value, video->priv->passes);
      break;
    case PROP_THREADS:
      g_value_set_uint (value, video->priv->threads);
      break;
    case PROP_TURBO:
      g_value_set_boolean (value, video->priv->turbo);
      break;
    case PROP_DENOISE:
      g_value_set_boolean (value, video->priv->denoise);
      break;
    case PROP_BFRAMES:
      g_value_set_uint (value, video->priv->max_b_frames);
      break;
    case PROP_DEBLOCK:
      g_value_set_boolean (value, video->priv->deblock);
      break;
    case PROP_DERING:
      g_value_set_boolean (value, video->priv->dering);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_video_codec_autosize (OGMRipVideoCodec *video)
{
  guint max_width, max_height;
  guint min_width, min_height;
  gboolean expand;

  ogmrip_video_codec_get_max_size (video, &max_width, &max_height, &expand);
  ogmrip_video_codec_get_min_size (video, &min_width, &min_height);

  if ((max_width && max_height) || (min_width || min_height))
  {
    guint scale_width, scale_height;

    if (ogmrip_video_codec_get_scale_size (video, &scale_width, &scale_height) &&
        (scale_width > max_width || scale_height > max_height || scale_width < min_width || scale_height < min_height))
    {
      gdouble ratio = scale_width / (gdouble) scale_height;

      if (scale_width > max_width)
      {
        scale_width = max_width;
        scale_height = FLOOR (scale_width / ratio);
      }

      if (scale_height > max_height)
      {
        scale_height = max_height;
        scale_width = FLOOR (scale_height * ratio);
      }

      if (scale_width < min_width)
      {
        scale_width = min_width;
        scale_height = ROUND (scale_width / ratio);
      }

      if (scale_height < min_height)
      {
        scale_height = min_height;
        scale_width = ROUND (scale_height * ratio);
      }

      video->priv->scale_width = scale_width;
      video->priv->scale_height = scale_height;
    }
  }
}

static void
ogmrip_video_codec_cancel (OGMJobSpawn *spawn)
{
  OGMRipVideoCodec *video;

  video = OGMRIP_VIDEO_CODEC (spawn);

  if (video->priv->child)
  {
    ogmjob_spawn_cancel (video->priv->child);
    video->priv->child_canceled = TRUE;
  }

  OGMJOB_SPAWN_CLASS (ogmrip_video_codec_parent_class)->cancel (spawn);
}

/**
 * ogmrip_video_codec_get_ensure_sync:
 * @video: an #OGMRipVideoCodec
 *
 * Gets the audio stream that will be encoded along with the video to ensure
 * the A/V synchronization.
 *
 * Returns: the #OGMDvdAudioStream, or NULL
 */
OGMDvdAudioStream * 
ogmrip_video_codec_get_ensure_sync (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), NULL);

  return video->priv->astream;
}

/**
 * ogmrip_video_codec_set_ensure_sync:
 * @video: an #OGMRipVideoCodec
 * @stream: an #OGMDvdAudioStream
 *
 * Sets the audio stream that will be encoded along with the video to ensure
 * the A/V synchronization.
 */
void
ogmrip_video_codec_set_ensure_sync (OGMRipVideoCodec *video, OGMDvdAudioStream *stream)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  if (video->priv->astream != stream)
  {
    if (stream)
      ogmdvd_stream_ref (OGMDVD_STREAM (stream));

    if (video->priv->astream)
      ogmdvd_stream_unref (OGMDVD_STREAM (video->priv->astream));

    video->priv->astream = stream;
  }
}

/**
 * ogmrip_video_codec_set_angle:
 * @video: an #OGMRipVideoCodec
 * @angle: the angle
 *
 * Sets the angle to encode.
 */
void
ogmrip_video_codec_set_angle (OGMRipVideoCodec *video, guint angle)
{
  OGMDvdTitle *title;

  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  title = ogmrip_codec_get_input (OGMRIP_CODEC (video));

  g_return_if_fail (angle > 0 && angle <= ogmdvd_title_get_n_angles (title));

  video->priv->angle = angle;
}

/**
 * ogmrip_video_codec_get_angle:
 * @video: an #OGMRipVideoCodec
 *
 * Gets the current angle.
 *
 * Returns: the angle, or -1
 */
gint
ogmrip_video_codec_get_angle (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), -1);

  return video->priv->angle;
}

/**
 * ogmrip_video_codec_set_bitrate:
 * @video: an #OGMRipVideoCodec
 * @bitrate: the video bitrate
 *
 * Sets the video bitrate to be used in bits/second, 4000 being the lowest and
 * 24000000 the highest available bitrates.
 */
void
ogmrip_video_codec_set_bitrate (OGMRipVideoCodec *video, guint bitrate)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  video->priv->bitrate = CLAMP (bitrate, 4000, 24000000);
  video->priv->quantizer = -1.0;
}

/**
 * ogmrip_video_codec_get_bitrate:
 * @video: an #OGMRipVideoCodec
 *
 * Gets the video bitrate in bits/second.
 *
 * Returns: the video bitrate, or -1
 */
gint
ogmrip_video_codec_get_bitrate (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), -1);

  return video->priv->bitrate;
}

/**
 * ogmrip_video_codec_set_quantizer:
 * @video: an #OGMRipVideoCodec
 * @quantizer: the video quantizer
 *
 * Sets the video quantizer to be used, 1 being the lowest and 31 the highest
 * available quantizers.
 */
void
ogmrip_video_codec_set_quantizer (OGMRipVideoCodec *video, gdouble quantizer)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  video->priv->quantizer = CLAMP (quantizer, 0, 31);
  video->priv->bitrate = -1;
}

/**
 * ogmrip_video_codec_get_quantizer:
 * @video: an #OGMRipVideoCodec
 *
 * Gets the video quantizer.
 *
 * Returns: the video quantizer, or -1
 */
gdouble
ogmrip_video_codec_get_quantizer (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), -1.0);

  return video->priv->quantizer;
}

/**
 * ogmrip_video_codec_set_bits_per_pixel:
 * @video: an #OGMRipVideoCodec
 * @bpp: the number of bits per pixel
 *
 * Sets the number of bits per pixel to be used.
 */
void
ogmrip_video_codec_set_bits_per_pixel (OGMRipVideoCodec *video, gdouble bpp)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));
  g_return_if_fail (bpp > 0.0 && bpp <= 1.0);

  video->priv->bpp = bpp;
}

/**
 * ogmrip_video_codec_get_bits_per_pixel:
 * @video: an #OGMRipVideoCodec
 *
 * Gets the number of bits per pixel.
 *
 * Returns: the number of bits per pixel, or -1
 */
gdouble
ogmrip_video_codec_get_bits_per_pixel (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), -1.0);

  return video->priv->bpp;
}

/**
 * ogmrip_video_codec_set_passes:
 * @video: an #OGMRipVideoCodec
 * @pass: the pass number
 *
 * Sets the number of passes.
 */
void
ogmrip_video_codec_set_passes (OGMRipVideoCodec *video, guint pass)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  video->priv->passes = MAX (pass, 1);
}

/**
 * ogmrip_video_codec_get_passes:
 * @video: an #OGMRipVideoCodec
 *
 * Gets the number of passes.
 *
 * Returns: the pass number, or -1
 */
gint
ogmrip_video_codec_get_passes (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), -1);

  return video->priv->passes;
}

/**
 * ogmrip_video_codec_set_threads:
 * @video: an #OGMRipVideoCodec
 * @threads: the number of threads
 *
 * Sets the number of threads to be used.
 */
void
ogmrip_video_codec_set_threads (OGMRipVideoCodec *video, guint threads)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  video->priv->threads = MAX (threads, 0);
}

/**
 * ogmrip_video_codec_get_threads:
 * @video: an #OGMRipVideoCodec
 *
 * Gets the number of threads.
 *
 * Returns: the number of threads, or -1
 */
gint
ogmrip_video_codec_get_threads (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), -1);

  return video->priv->threads;
}

/**
 * ogmrip_video_codec_set_scaler:
 * @video: an #OGMRipVideoCodec
 * @scaler: an #OGMRipScalerType
 *
 * Sets the software scaler to be used.
 */
void
ogmrip_video_codec_set_scaler (OGMRipVideoCodec *video, OGMRipScalerType scaler)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  video->priv->scaler = scaler;
}

/**
 * ogmrip_video_codec_get_scaler:
 * @video: an #OGMRipVideoCodec
 *
 * Gets the current software scaler.
 *
 * Returns: the software scaler, or -1
 */
gint
ogmrip_video_codec_get_scaler (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), -1);

  return video->priv->scaler;
}

/**
 * ogmrip_video_codec_set_deinterlacer:
 * @video: an #OGMRipVideoCodec
 * @deint: an #OGMRipDeintType
 *
 * Sets the deinterlacer to be used.
 */
void
ogmrip_video_codec_set_deinterlacer (OGMRipVideoCodec *video, OGMRipDeintType deint)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  if (video->priv->interlaced != 0)
    video->priv->deint = deint;
}

/**
 * ogmrip_video_codec_get_deinterlacer:
 * @video: an #OGMRipVideoCodec
 *
 * Gets the currnet deinterlacer.
 *
 * Returns: the deinterlacer, or -1
 */
gint
ogmrip_video_codec_get_deinterlacer (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), -1);

  return video->priv->deint;
}

/**
 * ogmrip_video_codec_set_turbo:
 * @video: an #OGMRipVideoCodec
 * @turbo: %TRUE to enable turbo
 *
 * Sets whether to enable turbo.
 */
void
ogmrip_video_codec_set_turbo (OGMRipVideoCodec *video, gboolean turbo)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  video->priv->turbo = turbo;
}

/**
 * ogmrip_video_codec_get_turbo:
 * @video: an #OGMRipVideoCodec
 *
 * Gets whether turbo is enabled.
 *
 * Returns: %TRUE if turbo is enabled
 */
gboolean
ogmrip_video_codec_get_turbo (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), FALSE);

  return video->priv->turbo;
}

/**
 * ogmrip_video_codec_set_denoise:
 * @video: an #OGMRipVideoCodec
 * @denoise: %TRUE to reduce image noise
 *
 * Sets whether to reduce image noise.
 */
void
ogmrip_video_codec_set_denoise (OGMRipVideoCodec *video, gboolean denoise)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  video->priv->denoise = denoise;
}

/**
 * ogmrip_video_codec_get_denoise:
 * @video: an #OGMRipVideoCodec
 *
 * Gets whether to reduce image noise.
 *
 * Returns: %TRUE to reduce image noise
 */
gboolean
ogmrip_video_codec_get_denoise (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), FALSE);

  return video->priv->denoise;
}

/**
 * ogmrip_video_codec_set_max_b_frames:
 * @video: an #OGMRipVideoCodec
 * @max_b_frames: the maximum number of B-frames
 *
 * Sets the maximum number of B-frames to put between I/P-frames.
 */
void
ogmrip_video_codec_set_max_b_frames (OGMRipVideoCodec *video, guint max_b_frames)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  video->priv->max_b_frames = MIN (max_b_frames, 4);
}

/**
 * ogmrip_video_codec_get_max_b_frames:
 * @video: an #OGMRipVideoCodec
 *
 * Gets the maximum number of B-frames to put between I/P-frames.
 *
 * Returns: the maximum number of B-frames, or -1
 */
gint
ogmrip_video_codec_get_max_b_frames (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), -1);

  return video->priv->max_b_frames;
}

/**
 * ogmrip_video_codec_set_quality:
 * @video: an #OGMRipVideoCodec
 * @quality: the #OGMRipQualityType
 *
 * Sets the quality of the encoding.
 */
void
ogmrip_video_codec_set_quality (OGMRipVideoCodec *video, OGMRipQualityType quality)
{
  OGMRipVideoCodecClass *klass;

  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  video->priv->quality = CLAMP (quality, OGMRIP_QUALITY_EXTREME, OGMRIP_QUALITY_USER);

  klass = OGMRIP_VIDEO_CODEC_GET_CLASS (video);

  if (klass->set_quality)
    (* klass->set_quality) (video, video->priv->quality);
}

/**
 * ogmrip_video_codec_get_quality:
 * @video: an #OGMRipVideoCodec
 *
 * Gets the quality of the encoding.
 *
 * Returns: the #OGMRipQualityType, or -1
 */
gint
ogmrip_video_codec_get_quality (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), -1);

  return video->priv->quality;
}

/**
 * ogmrip_video_codec_set_deblock:
 * @video: an #OGMRipVideoCodec
 * @deblock: %TRUE to apply a deblocking filter
 *
 * Sets whether to apply a deblocking filter.
 */
void
ogmrip_video_codec_set_deblock (OGMRipVideoCodec *video, gboolean deblock)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  video->priv->deblock = deblock;
}

/**
 * ogmrip_video_codec_get_deblock:
 * @video: an #OGMRipVideoCodec
 *
 * Gets whether a deblocking filter will be applied.
 *
 * Returns: %TRUE if a deblocking filter will be applied
 */
gboolean
ogmrip_video_codec_get_deblock (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), FALSE);

  return video->priv->deblock;
}

/**
 * ogmrip_video_codec_set_dering:
 * @video: an #OGMRipVideoCodec
 * @dering: %TRUE to apply a deringing filter
 *
 * Sets whether to apply a deringing filter.
 */
void
ogmrip_video_codec_set_dering (OGMRipVideoCodec *video, gboolean dering)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  video->priv->dering = dering;
}

/**
 * ogmrip_video_codec_get_dering:
 * @video: an #OGMRipVideoCodec
 *
 * Gets whether a deringing filter will be applied.
 *
 * Returns: %TRUE if a deringing filter will be applied
 */
gboolean
ogmrip_video_codec_get_dering (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), FALSE);

  return video->priv->dering;
}

/**
 * ogmrip_video_codec_get_start_delay:
 * @video: an #OGMRipVideoCodec
 *
 * Gets the start delay that must be applied to audio streams when merging.
 *
 * Returns: the start delay, or -1
 */
gint
ogmrip_video_codec_get_start_delay (OGMRipVideoCodec *video)
{
  OGMRipVideoCodecClass *klass;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), -1);

  klass = OGMRIP_VIDEO_CODEC_GET_CLASS (video);

  if (klass->get_start_delay)
    return (* klass->get_start_delay) (video);

  return 0;
}

/**
 * ogmrip_video_codec_get_raw_size:
 * @video: an #OGMRipVideoCodec
 * @width: a pointer to store the width
 * @height: a pointer to store the height
 *
 * Gets the raw size of the video.
 */
void
ogmrip_video_codec_get_raw_size (OGMRipVideoCodec *video, guint *width, guint *height)
{
  OGMDvdTitle *title;

  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));
  g_return_if_fail (width != NULL);
  g_return_if_fail (height != NULL);

  title = ogmrip_codec_get_input (OGMRIP_CODEC (video));

  g_return_if_fail (title != NULL);

  ogmdvd_title_get_size (title, width, height);
}

/**
 * ogmrip_video_codec_get_crop_size:
 * @video: an #OGMRipVideoCodec
 * @x: a pointer to store the cropped x position
 * @y: a pointer to store the cropped y position
 * @width: a pointer to store the cropped width
 * @height: a pointer to store the cropped height
 *
 * Gets whether the video will be cropped and the crop size.
 *
 * Returns: %TRUE if the video will be cropped
 */
gboolean
ogmrip_video_codec_get_crop_size (OGMRipVideoCodec *video, guint *x, guint *y, guint *width, guint *height)
{
  guint raw_width, raw_height;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), FALSE);
  g_return_val_if_fail (x != NULL, FALSE);
  g_return_val_if_fail (y != NULL, FALSE);
  g_return_val_if_fail (width != NULL, FALSE);
  g_return_val_if_fail (height != NULL, FALSE);

  ogmrip_video_codec_get_raw_size (video, &raw_width, &raw_height);

  *x = video->priv->crop_x;
  *y = video->priv->crop_y;
  *width = video->priv->crop_width;
  *height = video->priv->crop_height;

  if (*x == 0 && *y == 0 && *width == 0 && *height == 0)
  {
    *width = raw_width;
    *height = raw_height;
  }

  if (*x == 0 && *y == 0 && *width == raw_width && *height == raw_height)
    return FALSE;

  return TRUE;
}

/**
 * ogmrip_video_codec_set_crop_size:
 * @video: an #OGMRipVideoCodec
 * @x: the cropped x position
 * @y: the cropped y position
 * @width: the cropped width
 * @height: the cropped height
 *
 * Sets the crop size of the movie.
 */
void
ogmrip_video_codec_set_crop_size (OGMRipVideoCodec *video, guint x, guint y, guint width, guint height)
{
  guint raw_width, raw_height;

  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  ogmrip_video_codec_get_raw_size (video, &raw_width, &raw_height);

  if (width > 0 && height > 0)
  {
    if (x + width > raw_width)
      x = 0;

    if (y + height > raw_height)
      y = 0;

    if (x + width <= raw_width)
    {
      video->priv->crop_x = x;
      video->priv->crop_width = (width / 16) * 16;
    }

    if (y + height <= raw_height)
    {
      video->priv->crop_y = y;
      video->priv->crop_height = (height / 16) * 16;
    }
  }
}

/**
 * ogmrip_video_codec_get_scale_size:
 * @video: an #OGMRipVideoCodec
 * @width: a pointer to store the scaled width
 * @height: a pointer to store the scaled height
 *
 * Gets whether the video will be scaled and the scale size.
 *
 * Returns: %TRUE if the video will be scaled
 */
gboolean
ogmrip_video_codec_get_scale_size (OGMRipVideoCodec *video, guint *width, guint *height)
{
  guint raw_width, raw_height;
  guint scale_width, scale_height;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), FALSE);

  ogmrip_video_codec_get_raw_size (video, &raw_width, &raw_height);

  scale_width = video->priv->scale_width;
  scale_height = video->priv->scale_height;

  if (!scale_width)
    scale_width = raw_width;

  if (!scale_height)
    scale_height = raw_height;

  if (width)
    *width = 16 * ROUND (scale_width / 16.0);

  if (height)
    *height = 16 * ROUND (scale_height / 16.0);

  return scale_width != raw_width || scale_height != raw_height;
}

/**
 * ogmrip_video_codec_set_scale_size:
 * @video: an #OGMRipVideoCodec
 * @width: the scaled width
 * @height: the scaled height
 *
 * Sets the scaled size of the movie.
 */
void
ogmrip_video_codec_set_scale_size (OGMRipVideoCodec *video, guint width, guint height)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));
  g_return_if_fail (width > 0 && height > 0);

  video->priv->scale_width = width;
  video->priv->scale_height = height;

  ogmrip_video_codec_autosize (video);
}

/**
 * ogmrip_video_codec_get_max_size:
 * @video: an #OGMRipVideoCodec
 * @width: a pointer to store the maximum width
 * @height: a pointer to store the maximum height
 * @expand: whether the video must be expanded
 *
 * Gets wether the video has a maximum size and the maximum size.
 *
 * Returns: %TRUE if the video has a maximum size
 */
gboolean
ogmrip_video_codec_get_max_size (OGMRipVideoCodec *video, guint *width, guint *height, gboolean *expand)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), FALSE);

  if (width)
    *width = video->priv->max_width;

  if (height)
    *height = video->priv->max_height;

  if (expand)
    *expand = video->priv->expand;

  return video->priv->max_width && video->priv->max_height;
}

/**
 * ogmrip_video_codec_set_max_size:
 * @video: an #OGMRipVideoCodec
 * @width: the maximum width
 * @height: the maximum height
 * @expand: wheter to expand the video
 *
 * Sets the maximum size of the movie.
 */
void
ogmrip_video_codec_set_max_size (OGMRipVideoCodec *video, guint width, guint height, gboolean expand)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));
  g_return_if_fail (width > 0 && height > 0);

  video->priv->max_width = width;
  video->priv->max_height = height;
  video->priv->expand = expand;

  ogmrip_video_codec_autosize (video);
}

/**
 * ogmrip_video_codec_get_min_size:
 * @video: an #OGMRipVideoCodec
 * @width: a pointer to store the minimum width
 * @height: a pointer to store the minimum height
 *
 * Gets wether the video has a minimum size and the minimum size.
 *
 * Returns: %TRUE if the video has a minimum size
 */
gboolean
ogmrip_video_codec_get_min_size (OGMRipVideoCodec *video, guint *width, guint *height)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), FALSE);

  if (width)
    *width = video->priv->min_width;

  if (height)
    *height = video->priv->min_height;

  return video->priv->min_width && video->priv->min_height;
}

/**
 * ogmrip_video_codec_set_min_size:
 * @video: an #OGMRipVideoCodec
 * @width: the minimum width
 * @height: the minimum height
 *
 * Sets the minimum size of the movie.
 */
void
ogmrip_video_codec_set_min_size (OGMRipVideoCodec *video, guint width, guint height)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));
  g_return_if_fail (width >= 0 && height >= 0);

  video->priv->min_width = width;
  video->priv->min_height = height;

  ogmrip_video_codec_autosize (video);
}

/**
 * ogmrip_video_codec_get_aspect_ratio:
 * @video: an #OGMRipVideoCodec
 * @num: a pointer to store the numerator of the aspect ratio
 * @denom: a pointer to store the denominator of the aspect ratio
 *
 * Gets the aspect ratio of the movie.
 */
void
ogmrip_video_codec_get_aspect_ratio (OGMRipVideoCodec *video, guint *num, guint *denom)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  if (!video->priv->aspect_num || !video->priv->aspect_denom)
  {
    OGMDvdTitle *title;

    title = ogmrip_codec_get_input (OGMRIP_CODEC (video));
    ogmdvd_title_get_aspect_ratio (title, &video->priv->aspect_num, &video->priv->aspect_denom);
  }

  if (num)
    *num = video->priv->aspect_num;

  if (denom)
    *denom = video->priv->aspect_denom;
}

/**
 * ogmrip_video_codec_set_aspect_ratio:
 * @video: an #OGMRipVideoCodec
 * @num: the numerator of the aspect ratio
 * @denom: the denominator of the aspect ratio
 *
 * Sets the aspect ratio of the movie.
 */
void
ogmrip_video_codec_set_aspect_ratio (OGMRipVideoCodec *video, guint num, guint denom)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  video->priv->aspect_num = num;
  video->priv->aspect_denom = denom;
}

/**
 * ogmrip_video_codec_autoscale:
 * @video: an #OGMRipVideoCodec
 *
 * Autodetects the scaling parameters.
 */
void
ogmrip_video_codec_autoscale (OGMRipVideoCodec *video)
{
  OGMDvdTitle *title;
  guint anumerator, adenominator;
  guint rnumerator, rdenominator;
  guint scale_width, scale_height;
  guint crop_width, crop_height;
  guint raw_width, raw_height;
  gfloat ratio, bpp;

  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  title = ogmrip_codec_get_input (OGMRIP_CODEC (video));
  g_return_if_fail (title != NULL);

  ogmrip_video_codec_get_raw_size (video, &raw_width, &raw_height);
  ogmrip_video_codec_get_aspect_ratio (video, &anumerator, &adenominator);

  crop_width = video->priv->crop_width > 0 ? video->priv->crop_width : raw_width;
  crop_height = video->priv->crop_height > 0 ? video->priv->crop_height : raw_height;

  ogmdvd_title_get_framerate (title, &rnumerator, &rdenominator);

  ratio = crop_width / (gdouble) crop_height * raw_height / raw_width * anumerator / adenominator;

  if (video->priv->bitrate > 0)
  {
    scale_height = raw_height;
    for (scale_width = raw_width - 25 * 16; scale_width <= raw_width; scale_width += 16)
    {
      scale_height = ROUND (scale_width / ratio);

      bpp = (video->priv->bitrate * rdenominator) / 
        (gdouble) (scale_width * scale_height * rnumerator);

      if (bpp < video->priv->bpp)
        break;
    }
  }
  else
  {
    scale_width = crop_width;
    scale_height = ROUND (scale_width / ratio);
  }

  scale_width = MIN (scale_width, raw_width);

  ogmrip_video_codec_set_scale_size (video, scale_width, scale_height);
}

/**
 * ogmrip_video_codec_autobitrate:
 * @video: an #OGMRipVideoCodec
 * @nonvideo_size: the size of the non video streams
 * @overhead_size: the size of the overhead
 * @total_size: the total targetted size
 *
 * Autodetects the video bitrate.
 */
void
ogmrip_video_codec_autobitrate (OGMRipVideoCodec *video, guint64 nonvideo_size, guint64 overhead_size, guint64 total_size)
{
  OGMDvdTitle *title;
  gdouble video_size, length;

  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  title = ogmrip_codec_get_input (OGMRIP_CODEC (video));
  g_return_if_fail (title != NULL);

  video_size = total_size - nonvideo_size - overhead_size;
  length = ogmrip_codec_get_length (OGMRIP_CODEC (video), NULL);

  ogmrip_video_codec_set_bitrate (video, (video_size * 8.) / length);
}

static void
ogmrip_video_codec_child_progress (OGMJobSpawn *spawn, gdouble fraction, OGMRipVideoCodec *video)
{
  g_signal_emit_by_name (video, "progress", fraction);
}

/**
 * ogmrip_video_codec_autocrop:
 * @video: an #OGMRipVideoCodec
 * @nframes: the number of frames
 *
 * Autodetects the cropping parameters.
 *
 * Returns: %FALSE, on error or cancel
 */
gboolean
ogmrip_video_codec_autocrop (OGMRipVideoCodec *video, guint nframes)
{
  OGMJobSpawn *child;
  OGMRipCrop crop;

  gdouble length, start, step;
  gint x, y, w, h;
  gchar **argv;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), FALSE);

  memset (&crop, 0, sizeof (OGMRipCrop));

  if (!nframes)
  {
    if (MPLAYER_CHECK_VERSION (1,0,0,8))
      crop.nframes = 12;
    else
      crop.nframes = 30;
  }
  else
    crop.nframes = nframes + 5;

  video->priv->child = ogmjob_queue_new ();

  g_signal_connect (video->priv->child, "progress",
      G_CALLBACK (ogmrip_video_codec_child_progress), video);

  length = ogmrip_codec_get_length (OGMRIP_CODEC (video), NULL);
  step = length / 5.;

  for (start = step; start < length; start += step)
  {
    argv = ogmrip_video_codec_crop_command (video, start, crop.nframes);

    child = ogmjob_exec_newv (argv);
    ogmjob_container_add (OGMJOB_CONTAINER (video->priv->child), child);
    g_object_unref (child);

    ogmjob_exec_add_watch_full (OGMJOB_EXEC (child),
        (OGMJobWatch) ogmrip_video_codec_crop_watch, &crop, TRUE, FALSE, FALSE);
  }

  video->priv->child_canceled = FALSE;

  ogmjob_spawn_run (video->priv->child, NULL);
  g_object_unref (video->priv->child);
  video->priv->child = NULL;

  if (video->priv->child_canceled)
    return FALSE;

  w = g_ulist_get_most_frequent (crop.w);
  g_ulist_free (crop.w);

  h = g_ulist_get_most_frequent (crop.h);
  g_ulist_free (crop.h);

  x = g_ulist_get_most_frequent (crop.x);
  g_ulist_free (crop.x);

  y = g_ulist_get_most_frequent (crop.y);
  g_ulist_free (crop.y);

  ogmrip_video_codec_set_crop_size (video, x, y, w, h);

  return TRUE;
}

/**
 * ogmrip_video_codec_analyze:
 * @video: an #OGMRipVideoCodec
 * @nframes: the number of frames
 *
 * Analyze the video stream to detect if the video is progressive,
 * interlaced and/or telecine.
 *
 * Returns: %FALSE, on error or cancel
 */
gboolean
ogmrip_video_codec_analyze (OGMRipVideoCodec *video, guint nframes)
{
  OGMRipAnalyze analyze;
  gchar **argv;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), FALSE);

  if (!nframes)
    nframes = ANALYZE_FRAMES;

  memset (&analyze, 0, sizeof (OGMRipAnalyze));
  analyze.nframes = nframes;

  argv = ogmrip_video_codec_analyze_command (video, analyze.nframes);

  video->priv->child = ogmjob_exec_newv (argv);

  g_signal_connect (video->priv->child, "progress",
      G_CALLBACK (ogmrip_video_codec_child_progress), video);

  ogmjob_exec_add_watch_full (OGMJOB_EXEC (video->priv->child),
      (OGMJobWatch) ogmrip_video_codec_analyze_watch, &analyze, TRUE, FALSE, FALSE);

  video->priv->child_canceled = FALSE;

  ogmjob_spawn_run (video->priv->child, NULL);
  g_object_unref (video->priv->child);
  video->priv->child = NULL;

  if (video->priv->child_canceled)
    return FALSE;

  if (analyze.nsections > 0)
  {
    /*
     * Progressive
     */
    if (analyze.cur_section == SECTION_24000_1001 && analyze.nsections == 1)
    {
      ogmrip_codec_set_progressive (OGMRIP_CODEC (video), TRUE);
      ogmrip_codec_set_telecine (OGMRIP_CODEC (video), FALSE);
      ogmrip_video_codec_set_deinterlacer (video, OGMRIP_DEINT_NONE);
    }
    else if (analyze.nsections > 1)
    {
      ogmrip_codec_set_progressive (OGMRIP_CODEC (video), TRUE);
      if (analyze.npatterns > 0 && analyze.naffinities > 0)
      {
        /*
         * Mixed progressive and telecine
         */
        ogmrip_codec_set_telecine (OGMRIP_CODEC (video), TRUE);
        ogmrip_video_codec_set_deinterlacer (video, OGMRIP_DEINT_NONE);
      }
      else
      {
        /*
         * Mixed progressive and interlaced
         */
        ogmrip_codec_set_telecine (OGMRIP_CODEC (video), FALSE);
        ogmrip_video_codec_set_deinterlacer (video, OGMRIP_DEINT_YADIF);
      }
    }
  }
  else
  {
    ogmrip_codec_set_telecine (OGMRIP_CODEC (video), FALSE);
    ogmrip_codec_set_progressive (OGMRIP_CODEC (video), FALSE);
    ogmrip_video_codec_set_deinterlacer (video, OGMRIP_DEINT_NONE);
  }

  g_free (analyze.prev_affinity);
  g_free (analyze.cur_affinity);

  return TRUE;
}

/**
 * ogmrip_video_codec_get_hard_subp:
 * @video: an #OGMRipVideoCodec
 * @forced: location to store whether to hardcode forced subs only
 *
 * Gets the subp stream that will be hardcoded in the video.
 *
 * Returns: the #OGMDvdSubpStream, or NULL
 */
OGMDvdSubpStream *
ogmrip_video_codec_get_hard_subp (OGMRipVideoCodec *video, gboolean *forced)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), NULL);

  if (forced)
    *forced = video->priv->forced_subs;

  return video->priv->sstream;
}

/**
 * ogmrip_video_codec_set_hard_subp:
 * @video: an #OGMRipVideoCodec
 * @stream: an #OGMDvdSubpStream
 * @forced: whether to hardcode forced subs only
 *
 * Sets the subp stream that will be hardcoded in the video.
 */
void
ogmrip_video_codec_set_hard_subp (OGMRipVideoCodec *video, OGMDvdSubpStream *stream, gboolean forced)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  if (video->priv->sstream != stream)
  {
    if (stream)
      ogmdvd_stream_ref (OGMDVD_STREAM (stream));

    if (video->priv->sstream)
      ogmdvd_stream_unref (OGMDVD_STREAM (video->priv->sstream));

    video->priv->sstream = stream;
    video->priv->forced_subs = forced;
  }
}

/**
 * ogmrip_video_codec_is_interlaced:
 * @video: an #OGMRipVideoCodec
 *
 * Gets whether the video stream is interlaced.
 *
 * Returns: 1 if the video interlaced, 0 if it isn't, -1 otherwise.
 */
gint
ogmrip_video_codec_is_interlaced (OGMRipVideoCodec  *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), -1);

  return video->priv->interlaced;
}

