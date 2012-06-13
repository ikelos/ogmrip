/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-audio-options.h"
#include "ogmrip-audio-codec.h"

G_DEFINE_INTERFACE (OGMRipAudioOptions, ogmrip_audio_options, G_TYPE_OBJECT);

static void
ogmrip_audio_options_default_init (OGMRipAudioOptionsInterface *iface)
{
  g_object_interface_install_property (iface,
      g_param_spec_uint ("channels", "Channels property", "Set channels",
        OGMRIP_CHANNELS_MONO, OGMRIP_CHANNELS_7_1, OGMRIP_CHANNELS_STEREO,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
      g_param_spec_gtype ("codec", "Codec property", "Set codec",
        OGMRIP_TYPE_AUDIO_CODEC, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
      g_param_spec_boolean ("normalize", "Normalize property", "Set normalize",
        TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
      g_param_spec_uint ("quality", "Quality property", "Set quality",
        0, 10, 3, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
      g_param_spec_uint ("sample-rate", "Sample rate property", "Set sample rate",
        OGMRIP_SAMPLE_RATE_48000, OGMRIP_SAMPLE_RATE_8000, OGMRIP_SAMPLE_RATE_48000,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

guint
ogmrip_audio_options_get_channels (OGMRipAudioOptions *options)
{
  guint channels;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_OPTIONS (options), -1);

  g_object_get (options, "channels", &channels, NULL);

  return channels;
}

void
ogmrip_audio_options_set_channels (OGMRipAudioOptions *options, OGMRipChannels channels)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_OPTIONS (options));
  g_return_if_fail (channels > OGMRIP_CHANNELS_UNDEFINED);

  g_object_set (options, "channels", channels, NULL);
}

GType
ogmrip_audio_options_get_codec (OGMRipAudioOptions *options)
{
  GType codec;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_OPTIONS (options), G_TYPE_NONE);

  g_object_get (options, "codec", &codec, NULL);

  return codec;
}

void
ogmrip_audio_options_set_codec (OGMRipAudioOptions *options, GType codec)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_OPTIONS (options));
  g_return_if_fail (g_type_is_a (codec, OGMRIP_TYPE_AUDIO_CODEC));

  g_object_set (options, "codec", codec, NULL);
}

gboolean
ogmrip_audio_options_get_normalize (OGMRipAudioOptions *options)
{
  gboolean normalize;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_OPTIONS (options), FALSE);

  g_object_get (options, "normalize", &normalize, NULL);

  return normalize;
}

void
ogmrip_audio_options_set_normalize (OGMRipAudioOptions *options, gboolean normalize)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_OPTIONS (options));

  g_object_set (options, "normalize", normalize, NULL);
}

guint
ogmrip_audio_options_get_quality (OGMRipAudioOptions *options)
{
  guint quality;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_OPTIONS (options), -1);

  g_object_get (options, "quality", &quality, NULL);

  return quality;
}

void
ogmrip_audio_options_set_quality (OGMRipAudioOptions *options, guint quality)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_OPTIONS (options));

  g_object_set (options, "quality", quality, NULL);
}

guint
ogmrip_audio_options_get_sample_rate (OGMRipAudioOptions *options)
{
  guint srate;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_OPTIONS (options), -1);

  g_object_get (options, "sample-rate", &srate, NULL);

  return srate;
}

void
ogmrip_audio_options_set_sample_rate (OGMRipAudioOptions *options, OGMRipSampleRate srate)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_OPTIONS (options));
  g_return_if_fail (srate > OGMRIP_SAMPLE_RATE_UNDEFINED);

  g_object_set (options, "sample-rate", srate, NULL);
}

