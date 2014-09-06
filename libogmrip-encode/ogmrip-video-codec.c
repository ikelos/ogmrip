/* OGMRip - A library for media ripping and encoding
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

/**
 * SECTION:ogmrip-video-codec
 * @title: OGMRipVideoCodec
 * @short_description: Base class for video codecs
 * @include: ogmrip-video-codec.h
 */

#include "ogmrip-video-codec.h"
#include "ogmrip-profile-keys.h"
#include "ogmrip-encoding.h"
#include "ogmrip-version.h"

#include <ogmrip-base.h>

#include <string.h>
#include <stdio.h>

#define DEFAULT_BPP 0.15

#define ROUND(x) ((gint) ((x) + 0.5) != (gint) (x) ? ((gint) ((x) + 0.5)) : ((gint) (x)))
#define FLOOR(x) ((gint) (x))

struct _OGMRipVideoCodecPriv
{
  gdouble bpp;
  gdouble quantizer;
  guint bitrate;
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
  gboolean can_crop;
  gboolean denoise;
  gboolean deblock;
  gboolean dering;
  gboolean turbo;
  gboolean expand;

  OGMRipAudioStream *astream;
  OGMRipSubpStream *sstream;
  OGMRipQualityType quality;
  OGMRipScalerType scaler;
  OGMRipDeintType deint;

  gboolean forced_subs;
};

enum 
{
  PROP_0,
  PROP_ANGLE,
  PROP_BITRATE,
  PROP_CAN_CROP,
  PROP_QUANTIZER,
  PROP_BPP,
  PROP_PASSES,
  PROP_THREADS,
  PROP_TURBO,
  PROP_DENOISE,
  PROP_DEBLOCK,
  PROP_DERING,
  PROP_SCALER,
  PROP_DEINT,
  PROP_QUALITY,
  PROP_DELAY,
  PROP_MIN_WIDTH,
  PROP_MIN_HEIGHT,
  PROP_MAX_WIDTH,
  PROP_MAX_HEIGHT,
  PROP_EXPAND,
  PROP_CROP_X,
  PROP_CROP_Y,
  PROP_CROP_WIDTH,
  PROP_CROP_HEIGHT,
  PROP_SCALE_WIDTH,
  PROP_SCALE_HEIGHT
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (OGMRipVideoCodec, ogmrip_video_codec, OGMRIP_TYPE_CODEC)

static void
ogmrip_video_codec_constructed (GObject *gobject)
{
  OGMRipStream *stream;

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (gobject));
  if (!OGMRIP_IS_VIDEO_STREAM (stream))
    g_error ("No video stream specified");

  G_OBJECT_CLASS (ogmrip_video_codec_parent_class)->constructed (gobject);
}

static void
ogmrip_video_codec_dispose (GObject *gobject)
{
  OGMRipVideoCodec *video = OGMRIP_VIDEO_CODEC (gobject);

  if (video->priv->astream)
  {
    g_object_unref (video->priv->astream);
    video->priv->astream = NULL;
  }

  if (video->priv->sstream)
  {
    g_object_unref (video->priv->sstream);
    video->priv->sstream = NULL;
  }

  G_OBJECT_CLASS (ogmrip_video_codec_parent_class)->dispose (gobject);
}

static void
ogmrip_video_codec_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipVideoCodec *video = OGMRIP_VIDEO_CODEC (gobject);

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
    case PROP_DEBLOCK:
      ogmrip_video_codec_set_deblock (video, g_value_get_boolean (value));
      break;
    case PROP_DERING:
      ogmrip_video_codec_set_dering (video, g_value_get_boolean (value));
      break;
    case PROP_SCALER:
      ogmrip_video_codec_set_scaler (video, g_value_get_uint (value));
      break;
    case PROP_DEINT:
      ogmrip_video_codec_set_deinterlacer (video, g_value_get_uint (value));
      break;
    case PROP_QUALITY:
      ogmrip_video_codec_set_quality (video, g_value_get_uint (value));
      break;
    case PROP_MIN_WIDTH:
      ogmrip_video_codec_set_min_size (video, g_value_get_uint (value), video->priv->min_height);
      break;
    case PROP_MIN_HEIGHT:
      ogmrip_video_codec_set_min_size (video, video->priv->min_width, g_value_get_uint (value));
      break;
    case PROP_MAX_WIDTH:
      ogmrip_video_codec_set_max_size (video, g_value_get_uint (value), video->priv->max_height, video->priv->expand);
      break;
    case PROP_MAX_HEIGHT:
      ogmrip_video_codec_set_max_size (video, video->priv->max_width, g_value_get_uint (value), video->priv->expand);
      break;
    case PROP_EXPAND:
      ogmrip_video_codec_set_max_size (video, video->priv->max_width, video->priv->max_height, g_value_get_boolean (value));
      break;
    case PROP_CAN_CROP:
      ogmrip_video_codec_set_can_crop (video, g_value_get_boolean (value));
      break;
    case PROP_CROP_X:
      ogmrip_video_codec_set_crop_size (video, g_value_get_uint (value), video->priv->crop_y, video->priv->crop_width, video->priv->crop_height);
      break;
    case PROP_CROP_Y:
      ogmrip_video_codec_set_crop_size (video, video->priv->crop_x, g_value_get_uint (value), video->priv->crop_width, video->priv->crop_height);
      break;
    case PROP_CROP_WIDTH:
      ogmrip_video_codec_set_crop_size (video, video->priv->crop_x, video->priv->crop_y, g_value_get_uint (value), video->priv->crop_height);
      break;
    case PROP_CROP_HEIGHT:
      ogmrip_video_codec_set_crop_size (video, video->priv->crop_x, video->priv->crop_y, video->priv->crop_width, g_value_get_uint (value));
      break;
    case PROP_SCALE_WIDTH:
      ogmrip_video_codec_set_scale_size (video, g_value_get_uint (value), video->priv->scale_height);
      break;
    case PROP_SCALE_HEIGHT:
      ogmrip_video_codec_set_scale_size (video, video->priv->scale_width, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_video_codec_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipVideoCodec *video = OGMRIP_VIDEO_CODEC (gobject);

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
    case PROP_DEBLOCK:
      g_value_set_boolean (value, video->priv->deblock);
      break;
    case PROP_DERING:
      g_value_set_boolean (value, video->priv->dering);
      break;
    case PROP_SCALER:
      g_value_set_uint (value, video->priv->scaler);
      break;
    case PROP_DEINT:
      g_value_set_uint (value, video->priv->deint);
      break;
    case PROP_QUALITY:
      g_value_set_uint (value, video->priv->quality);
      break;
    case PROP_DELAY:
      g_value_set_uint (value, 0);
      break;
    case PROP_MIN_WIDTH:
      g_value_set_uint (value, video->priv->min_width);
      break;
    case PROP_MIN_HEIGHT:
      g_value_set_uint (value, video->priv->min_height);
      break;
    case PROP_MAX_WIDTH:
      g_value_set_uint (value, video->priv->max_width);
      break;
    case PROP_MAX_HEIGHT:
      g_value_set_uint (value, video->priv->max_height);
      break;
    case PROP_EXPAND:
      g_value_set_boolean (value, video->priv->expand);
      break;
    case PROP_CAN_CROP:
      g_value_set_boolean (value, video->priv->can_crop);
      break;
    case PROP_CROP_X:
      g_value_set_uint (value, video->priv->crop_x);
      break;
    case PROP_CROP_Y:
      g_value_set_uint (value, video->priv->crop_y);
      break;
    case PROP_CROP_WIDTH:
      g_value_set_uint (value, video->priv->crop_width);
      break;
    case PROP_CROP_HEIGHT:
      g_value_set_uint (value, video->priv->crop_height);
      break;
    case PROP_SCALE_WIDTH:
      g_value_set_uint (value, video->priv->scale_width);
      break;
    case PROP_SCALE_HEIGHT:
      g_value_set_uint (value, video->priv->scale_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_video_codec_class_init (OGMRipVideoCodecClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_video_codec_constructed;
  gobject_class->dispose = ogmrip_video_codec_dispose;
  gobject_class->set_property = ogmrip_video_codec_set_property;
  gobject_class->get_property = ogmrip_video_codec_get_property;

  g_object_class_install_property (gobject_class, PROP_ANGLE, 
        g_param_spec_uint ("angle", "Angle property", "Set angle", 
           1, G_MAXUINT, 1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BITRATE, 
        g_param_spec_uint ("bitrate", "Bitrate property", "Set bitrate", 
           0, G_MAXUINT, 800000, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CAN_CROP, 
        g_param_spec_boolean ("can-crop", "Can crop property", "Set can crop", 
           TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QUANTIZER, 
        g_param_spec_double ("quantizer", "Quantizer property", "Set quantizer", 
           0.0, 31.0, 0.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BPP, 
        g_param_spec_double ("bpp", "Bits per pixel property", "Set bits per pixel", 
           0.0, 1.0, DEFAULT_BPP, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PASSES, 
        g_param_spec_uint ("passes", "Passes property", "Set the number of passes", 
           1, G_MAXUINT, 1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_THREADS, 
        g_param_spec_uint ("threads", "Threads property", "Set the number of threads", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SCALER, 
        g_param_spec_uint ("scaler", "Scaler property", "Set the scaler", 
           OGMRIP_SCALER_NONE, OGMRIP_SCALER_BICUBIC_SPLINE, OGMRIP_SCALER_GAUSS,
           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEINT, 
        g_param_spec_uint ("deinterlacer", "Deinterlacer property", "Set the deinterlacer", 
           OGMRIP_DEINT_NONE, OGMRIP_DEINT_YADIF, OGMRIP_DEINT_NONE,
           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QUALITY, 
        g_param_spec_uint ("quality", "Quality property", "Set the quality", 
           OGMRIP_QUALITY_EXTREME, OGMRIP_QUALITY_USER, OGMRIP_QUALITY_NORMAL,
           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TURBO, 
        g_param_spec_boolean ("turbo", "Turbo property", "Set turbo", 
           FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DENOISE, 
        g_param_spec_boolean ("denoise", "Denoise property", "Set denoise", 
           FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEBLOCK, 
        g_param_spec_boolean ("deblock", "Deblock property", "Set deblock", 
           TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DERING, 
        g_param_spec_boolean ("dering", "Dering property", "Set dering", 
           TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DELAY, 
        g_param_spec_uint ("start-delay", "Start delay property", "Set start delay", 
           0, G_MAXUINT, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MIN_WIDTH, 
        g_param_spec_uint ("min-width", "Min width property", "Set min width", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MIN_HEIGHT, 
        g_param_spec_uint ("min-height", "Min height property", "Set min height", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_WIDTH, 
        g_param_spec_uint ("max-width", "Max width property", "Set max width", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_HEIGHT, 
        g_param_spec_uint ("max-height", "Max height property", "Set max height", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_EXPAND, 
        g_param_spec_boolean ("expand", "Expand property", "Set expand", 
           FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CROP_X, 
        g_param_spec_uint ("crop-x", "Crop x property", "Set crop x", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CROP_Y, 
        g_param_spec_uint ("crop-y", "Crop y property", "Set crop y", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CROP_WIDTH, 
        g_param_spec_uint ("crop-width", "Crop width property", "Set crop width", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CROP_HEIGHT, 
        g_param_spec_uint ("crop-height", "Crop height property", "Set crop height", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SCALE_WIDTH, 
        g_param_spec_uint ("scale-width", "Scale width property", "Set scale width", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SCALE_HEIGHT, 
        g_param_spec_uint ("scale-height", "Scale height property", "Set scale height", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_video_codec_init (OGMRipVideoCodec *video)
{
  video->priv = ogmrip_video_codec_get_instance_private (video);

  video->priv->scaler = OGMRIP_SCALER_GAUSS;
  video->priv->quality = OGMRIP_QUALITY_NORMAL;
  video->priv->astream = NULL;
  video->priv->bitrate = 800000;
  video->priv->turbo = TRUE;
  video->priv->can_crop = TRUE;
  video->priv->angle = 1;
  video->priv->bpp = DEFAULT_BPP;
  video->priv->passes = 1;
  video->priv->can_crop = TRUE;
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

GType
ogmrip_video_codec_get_default (GType container)
{
  GType *types;
  guint i;

  g_return_val_if_fail (g_type_is_a (container, OGMRIP_TYPE_CONTAINER), G_TYPE_NONE);

  types = ogmrip_type_children (OGMRIP_TYPE_VIDEO_CODEC, NULL);
  for (i = 0; types[i] != G_TYPE_NONE; i ++)
    if (ogmrip_container_contains (container, types[i]))
      return types[i];

  return G_TYPE_NONE;
}

OGMRipCodec *
ogmrip_video_codec_new (GType type, OGMRipVideoStream *stream)
{
  g_return_val_if_fail (g_type_is_a (type, OGMRIP_TYPE_VIDEO_CODEC), NULL);
  g_return_val_if_fail (OGMRIP_IS_VIDEO_STREAM (stream), NULL);

  return g_object_new (type, "input", stream, NULL);
}

OGMRipCodec *
ogmrip_video_codec_new_from_profile (OGMRipVideoStream *stream, OGMRipProfile *profile)
{
  OGMRipCodec *codec;
  GSettings *settings;
  GType type;
  guint w, h, m;
  gchar *name;

  g_return_val_if_fail (OGMRIP_IS_PROFILE (profile), NULL);
  g_return_val_if_fail (OGMRIP_IS_VIDEO_STREAM (stream), NULL);

  ogmrip_profile_get (profile, OGMRIP_PROFILE_VIDEO, OGMRIP_PROFILE_CODEC, "s", &name);
  type = ogmrip_type_from_name (name);
  g_free (name);
  
  if (type == G_TYPE_NONE)
    return NULL;

  codec = ogmrip_video_codec_new (type, stream);

  settings = ogmrip_profile_get_child (profile, OGMRIP_PROFILE_VIDEO);

  ogmrip_video_codec_set_passes (OGMRIP_VIDEO_CODEC (codec),
      g_settings_get_uint (settings, OGMRIP_PROFILE_PASSES));
  ogmrip_video_codec_set_scaler (OGMRIP_VIDEO_CODEC (codec),
      g_settings_get_uint (settings, OGMRIP_PROFILE_SCALER));
  ogmrip_video_codec_set_turbo (OGMRIP_VIDEO_CODEC (codec),
      g_settings_get_boolean (settings, OGMRIP_PROFILE_TURBO));
  ogmrip_video_codec_set_denoise (OGMRIP_VIDEO_CODEC (codec),
      g_settings_get_boolean (settings, OGMRIP_PROFILE_DENOISE));
  ogmrip_video_codec_set_deblock (OGMRIP_VIDEO_CODEC (codec),
      g_settings_get_boolean (settings, OGMRIP_PROFILE_DEBLOCK));
  ogmrip_video_codec_set_dering (OGMRIP_VIDEO_CODEC (codec),
      g_settings_get_boolean (settings, OGMRIP_PROFILE_DERING));
  ogmrip_video_codec_set_quality (OGMRIP_VIDEO_CODEC (codec),
      g_settings_get_uint (settings, OGMRIP_PROFILE_QUALITY));

  w = g_settings_get_uint (settings, OGMRIP_PROFILE_MIN_WIDTH);
  h = g_settings_get_uint (settings, OGMRIP_PROFILE_MIN_HEIGHT);

  if (w > 0 && h > 0)
    ogmrip_video_codec_set_min_size (OGMRIP_VIDEO_CODEC (codec), w, h);

  w = g_settings_get_uint (settings, OGMRIP_PROFILE_MAX_WIDTH);
  h = g_settings_get_uint (settings, OGMRIP_PROFILE_MAX_HEIGHT);

  if (w > 0 && h > 0)
    ogmrip_video_codec_set_max_size (OGMRIP_VIDEO_CODEC (codec), w, h,
        g_settings_get_boolean (settings, OGMRIP_PROFILE_EXPAND));

  ogmrip_profile_get (profile, OGMRIP_PROFILE_GENERAL, OGMRIP_PROFILE_ENCODING_METHOD, "u", &m);
  if (m == OGMRIP_ENCODING_BITRATE)
    ogmrip_video_codec_set_bitrate (OGMRIP_VIDEO_CODEC (codec),
        g_settings_get_uint (settings, OGMRIP_PROFILE_BITRATE));
  else if (m == OGMRIP_ENCODING_QUANTIZER)
    ogmrip_video_codec_set_quantizer (OGMRIP_VIDEO_CODEC (codec),
        g_settings_get_double (settings, OGMRIP_PROFILE_QUANTIZER));

  g_object_unref (settings);

  return codec;
}

/**
 * ogmrip_video_codec_get_ensure_sync:
 * @video: an #OGMRipVideoCodec
 *
 * Gets the audio stream that will be encoded along with the video to ensure
 * the A/V synchronization.
 *
 * Returns: the #OGMRipAudioStream, or NULL
 */
OGMRipAudioStream * 
ogmrip_video_codec_get_ensure_sync (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), NULL);

  return video->priv->astream;
}

/**
 * ogmrip_video_codec_set_ensure_sync:
 * @video: an #OGMRipVideoCodec
 * @stream: an #OGMRipAudioStream
 *
 * Sets the audio stream that will be encoded along with the video to ensure
 * the A/V synchronization.
 */
void
ogmrip_video_codec_set_ensure_sync (OGMRipVideoCodec *video, OGMRipAudioStream *stream)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  if (video->priv->astream != stream)
  {
    if (stream)
      g_object_ref (stream);

    if (video->priv->astream)
      g_object_unref (video->priv->astream);

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
  OGMRipTitle *title;

  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  title = ogmrip_stream_get_title (ogmrip_codec_get_input (OGMRIP_CODEC (video)));

  g_return_if_fail (angle > 0 && angle <= ogmrip_title_get_n_angles (title));

  video->priv->angle = angle;

  g_object_notify (G_OBJECT (video), "angle");
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
 * Sets the video bitrate to be used in bits/second.
 */
void
ogmrip_video_codec_set_bitrate (OGMRipVideoCodec *video, guint bitrate)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  if (bitrate > 0)
  {
    video->priv->bitrate = bitrate;
    video->priv->quantizer = 0.0;

    g_object_notify (G_OBJECT (video), "bitrate");
    g_object_notify (G_OBJECT (video), "quantizer");
  }
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

gboolean
ogmrip_video_codec_get_can_crop (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), FALSE);

  return video->priv->can_crop;
}

void
ogmrip_video_codec_set_can_crop (OGMRipVideoCodec *video, gboolean can_crop)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  video->priv->can_crop = can_crop;

  g_object_notify (G_OBJECT (video), "can-crop");
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

  if (quantizer > 0.0)
  {
    video->priv->quantizer = CLAMP (quantizer, 1, 31);
    video->priv->bitrate = 0;

    g_object_notify (G_OBJECT (video), "quantizer");
    g_object_notify (G_OBJECT (video), "bitrate");
  }
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

  g_object_notify (G_OBJECT (video), "bpp");
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

  g_object_notify (G_OBJECT (video), "passes");
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

  if (!video->priv->bitrate)
    return 1;

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

  g_object_notify (G_OBJECT (video), "threads");
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

  g_object_notify (G_OBJECT (video), "scaler");
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

  video->priv->deint = deint;

  g_object_notify (G_OBJECT (video), "deinterlacer");
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

  g_object_notify (G_OBJECT (video), "turbo");
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

  g_object_notify (G_OBJECT (video), "denoise");
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
 * ogmrip_video_codec_set_quality:
 * @video: an #OGMRipVideoCodec
 * @quality: the #OGMRipQualityType
 *
 * Sets the quality of the encoding.
 */
void
ogmrip_video_codec_set_quality (OGMRipVideoCodec *video, OGMRipQualityType quality)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  video->priv->quality = CLAMP (quality, OGMRIP_QUALITY_EXTREME, OGMRIP_QUALITY_USER);

  g_object_notify (G_OBJECT (video), "quality");
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

  g_object_notify (G_OBJECT (video), "deblock");
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

  g_object_notify (G_OBJECT (video), "dering");
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
  gint delay;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), -1);

  g_object_get (video, "start-delay", &delay, NULL);

  return delay;
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
  OGMRipStream *stream;
  guint w, h;

  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (video));
  ogmrip_video_stream_get_resolution (OGMRIP_VIDEO_STREAM (stream), &w, &h);

  if (width)
    *width = w;

  if (height)
    *height = h;
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

  ogmrip_video_codec_get_raw_size (video, &raw_width, &raw_height);

  if (!video->priv->crop_width)
    video->priv->crop_width = raw_width;

  if (!video->priv->crop_height)
    video->priv->crop_height = raw_height;

  if (x)
    *x = video->priv->crop_x;

  if (y)
    *y = video->priv->crop_y;

  if (width)
    *width = video->priv->crop_width;

  if (height)
    *height = video->priv->crop_height;

  if (video->priv->crop_x == 0 && video->priv->crop_width == raw_width &&
      video->priv->crop_y == 0 && video->priv->crop_height == raw_height)
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

  if (!width)
    width = raw_width;

  if (!height)
    height = raw_height;

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

  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), FALSE);

  ogmrip_video_codec_get_raw_size (video, &raw_width, &raw_height);

  if (!video->priv->scale_width)
    video->priv->scale_width = raw_width;

  if (!video->priv->scale_height)
    video->priv->scale_height = raw_height;

  if (width)
    *width = 16 * ROUND (video->priv->scale_width / 16.0);

  if (height)
    *height = 16 * ROUND (video->priv->scale_height / 16.0);

  return video->priv->scale_width != raw_width || video->priv->scale_height != raw_height;
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
  guint raw_width, raw_height;

  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  ogmrip_video_codec_get_raw_size (video, &raw_width, &raw_height);

  video->priv->scale_width = width > 0 ? width : raw_width;
  video->priv->scale_height = height > 0 ? height : raw_height;

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
    OGMRipStream *stream;

    stream = ogmrip_codec_get_input (OGMRIP_CODEC (video));
    ogmrip_video_stream_get_aspect_ratio (OGMRIP_VIDEO_STREAM (stream),
        &video->priv->aspect_num, &video->priv->aspect_denom);
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

void
ogmrip_video_codec_get_framerate (OGMRipVideoCodec *video, guint *num, guint *denom)
{
  OGMRipStream *stream;

  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));
  g_return_if_fail (num != NULL);
  g_return_if_fail (denom != NULL);

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (video));
  ogmrip_video_stream_get_framerate (OGMRIP_VIDEO_STREAM (stream), num, denom);

  if (ogmrip_video_stream_get_telecine (OGMRIP_VIDEO_STREAM (stream)) ||
      ogmrip_video_stream_get_progressive (OGMRIP_VIDEO_STREAM (stream)))
  {
    *num = 24000;
    *denom = 1001;
  }
}

/**
 * ogmrip_video_codec_get_hard_subp:
 * @video: an #OGMRipVideoCodec
 * @forced: location to store whether to hardcode forced subs only
 *
 * Gets the subp stream that will be hardcoded in the video.
 *
 * Returns: the #OGMRipSubpStream, or NULL
 */
OGMRipSubpStream *
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
 * @stream: an #OGMRipSubpStream
 * @forced: whether to hardcode forced subs only
 *
 * Sets the subp stream that will be hardcoded in the video.
 */
void
ogmrip_video_codec_set_hard_subp (OGMRipVideoCodec *video, OGMRipSubpStream *stream, gboolean forced)
{
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  if (video->priv->sstream != stream)
  {
    if (stream)
      g_object_ref (stream);

    if (video->priv->sstream)
      g_object_unref (video->priv->sstream);

    video->priv->sstream = stream;
    video->priv->forced_subs = forced;
  }
}
