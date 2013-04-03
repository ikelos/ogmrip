/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-profile-helper.h"
#include "ogmrip-profile-keys.h"
#include "ogmrip-video-codec.h"
#include "ogmrip-audio-codec.h"
#include "ogmrip-subp-codec.h"

static const gchar *fourcc[] =
{
  NULL,
  "XVID",
  "DIVX",
  "DX50",
  "FMP4"
};

OGMRipContainer *
ogmrip_create_container (OGMRipProfile *profile)
{
  OGMRipContainer *container;
  GSettings *settings;
  GType type;
  gchar *name;

  g_return_val_if_fail (OGMRIP_IS_PROFILE (profile), NULL);

  ogmrip_profile_get (profile, OGMRIP_PROFILE_GENERAL, OGMRIP_PROFILE_CONTAINER, "s", &name);
  type = ogmrip_type_from_name (name);
  g_free (name);

  g_assert (type != G_TYPE_NONE);

  container = g_object_new (type, NULL);

  settings = ogmrip_profile_get_child (profile, OGMRIP_PROFILE_GENERAL);
  ogmrip_container_set_fourcc (container,
      fourcc[g_settings_get_uint (settings, OGMRIP_PROFILE_FOURCC)]);
  ogmrip_container_set_split (container,
      g_settings_get_uint (settings, OGMRIP_PROFILE_TARGET_NUMBER),
      g_settings_get_uint (settings, OGMRIP_PROFILE_TARGET_SIZE));

  g_object_unref (settings);

  return container;
}

OGMRipCodec *
ogmrip_create_video_codec (OGMRipVideoStream *stream, OGMRipProfile *profile)
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

  codec = g_object_new (type, "input", stream, NULL);

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

static const gint sample_rate[] =
{ 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000 };

OGMRipCodec *
ogmrip_create_audio_codec (OGMRipAudioStream *stream, OGMRipProfile *profile)
{
  OGMRipCodec *codec;
  GSettings *settings;
  GType type;
  gchar *name;

  g_return_val_if_fail (OGMRIP_IS_PROFILE (profile), NULL);
  g_return_val_if_fail (OGMRIP_IS_AUDIO_STREAM (stream), NULL);

  ogmrip_profile_get (profile, OGMRIP_PROFILE_AUDIO, OGMRIP_PROFILE_CODEC, "s", &name);
  type = ogmrip_type_from_name (name);
  g_free (name);
  
  if (type == G_TYPE_NONE)
    return NULL;

  codec = g_object_new (type, "input", stream, NULL);

  settings = ogmrip_profile_get_child (profile, OGMRIP_PROFILE_AUDIO);

  ogmrip_audio_codec_set_channels (OGMRIP_AUDIO_CODEC (codec),
      g_settings_get_uint (settings, OGMRIP_PROFILE_CHANNELS));
  ogmrip_audio_codec_set_normalize (OGMRIP_AUDIO_CODEC (codec),
      g_settings_get_boolean (settings, OGMRIP_PROFILE_NORMALIZE));
  ogmrip_audio_codec_set_quality (OGMRIP_AUDIO_CODEC (codec),
      g_settings_get_uint (settings, OGMRIP_PROFILE_QUALITY));
  ogmrip_audio_codec_set_sample_rate (OGMRIP_AUDIO_CODEC (codec),
      sample_rate[g_settings_get_uint (settings, OGMRIP_PROFILE_SAMPLERATE)]);

  g_object_unref (settings);

  return codec;
}

OGMRipCodec *
ogmrip_create_subp_codec (OGMRipSubpStream *stream, OGMRipProfile *profile)
{
  OGMRipCodec *codec;
  GSettings *settings;
  GType type;
  gchar *name;

  g_return_val_if_fail (OGMRIP_IS_PROFILE (profile), NULL);
  g_return_val_if_fail (OGMRIP_IS_SUBP_STREAM (stream), NULL);

  ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_CODEC, "s", &name);
  type = ogmrip_type_from_name (name);
  g_free (name);
  
  if (type == G_TYPE_NONE)
    return NULL;

  codec = g_object_new (type, "input", stream, NULL);

  settings = ogmrip_profile_get_child (profile, OGMRIP_PROFILE_SUBP);

    ogmrip_subp_codec_set_charset (OGMRIP_SUBP_CODEC (codec),
        g_settings_get_uint (settings, OGMRIP_PROFILE_CHARACTER_SET));
    ogmrip_subp_codec_set_newline (OGMRIP_SUBP_CODEC (codec),
        g_settings_get_uint (settings, OGMRIP_PROFILE_NEWLINE_STYLE));
    ogmrip_subp_codec_set_forced_only (OGMRIP_SUBP_CODEC (codec),
        g_settings_get_boolean (settings, OGMRIP_PROFILE_FORCED_ONLY));

  g_object_unref (settings);

  return codec;
}

