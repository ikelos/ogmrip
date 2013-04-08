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

/**
 * SECTION:ogmrip-audio-codec
 * @title: OGMRipAudioCodec
 * @short_description: Base class for audio codecs
 * @include: ogmrip-audio-codec.h
 */

#include "ogmrip-audio-codec.h"
#include "ogmrip-profile-keys.h"

#include <ogmrip-base.h>

#define DEFAULT_SPF 1024

#define OGMRIP_AUDIO_CODEC_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_AUDIO_CODEC, OGMRipAudioCodecPriv))

struct _OGMRipAudioCodecPriv
{
  guint srate;
  guint quality;

  gboolean fast;
  gboolean normalize;

  gchar *label;
  guint language;

  OGMRipChannels channels;
};

enum 
{
  PROP_0,
  PROP_QUALITY,
  PROP_NORMALIZE,
  PROP_CHANNELS,
  PROP_SRATE,
  PROP_FAST,
  PROP_LABEL,
  PROP_LANGUAGE
};

static void ogmrip_audio_codec_constructed  (GObject      *gobject);
static void ogmrip_audio_codec_finalize     (GObject      *gobject);
static void ogmrip_audio_codec_set_property (GObject      *gobject,
                                             guint        property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void ogmrip_audio_codec_get_property (GObject      *gobject,
                                             guint        property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);

G_DEFINE_ABSTRACT_TYPE (OGMRipAudioCodec, ogmrip_audio_codec, OGMRIP_TYPE_CODEC)

static void
ogmrip_audio_codec_class_init (OGMRipAudioCodecClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = ogmrip_audio_codec_constructed;
  gobject_class->finalize = ogmrip_audio_codec_finalize;
  gobject_class->set_property = ogmrip_audio_codec_set_property;
  gobject_class->get_property = ogmrip_audio_codec_get_property;

  g_object_class_install_property (gobject_class, PROP_QUALITY, 
        g_param_spec_uint ("quality", "Quality property", "Set quality", 
           0, 10, 3, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NORMALIZE, 
        g_param_spec_boolean ("normalize", "Normalize property", "Set normalize", 
           FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CHANNELS, 
        g_param_spec_int ("channels", "Channels property", "Set channels", 
           OGMRIP_CHANNELS_UNDEFINED, OGMRIP_CHANNELS_7_1, OGMRIP_CHANNELS_STEREO,
           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SRATE, 
        g_param_spec_uint ("sample-rate", "Sample rate property", "Set sample rate", 
           0, G_MAXUINT, 48000, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FAST, 
        g_param_spec_boolean ("fast", "Fast property", "Set fast", 
           FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LABEL, 
        g_param_spec_string ("label", "Label property", "Set label", 
           NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LANGUAGE, 
        g_param_spec_uint ("language", "Language property", "Set language", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipAudioCodecPriv));
}

static void
ogmrip_audio_codec_init (OGMRipAudioCodec *audio)
{
  audio->priv = OGMRIP_AUDIO_CODEC_GET_PRIVATE (audio);
  audio->priv->channels = OGMRIP_CHANNELS_STEREO;

  audio->priv->srate = 48000;
  audio->priv->quality = 3;
}

static void
ogmrip_audio_codec_constructed (GObject *gobject)
{
  OGMRipStream *stream;

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (gobject));
  if (!OGMRIP_IS_AUDIO_STREAM (stream))
    g_error ("No audio stream specified");

  G_OBJECT_CLASS (ogmrip_audio_codec_parent_class)->constructed (gobject);
}

static void
ogmrip_audio_codec_finalize (GObject *gobject)
{
  OGMRipAudioCodec *audio = OGMRIP_AUDIO_CODEC (gobject);

  if (audio->priv->label)
  {
    g_free (audio->priv->label);
    audio->priv->label = NULL;
  }

  G_OBJECT_CLASS (ogmrip_audio_codec_parent_class)->finalize (gobject);
}

static void
ogmrip_audio_codec_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipAudioCodec *audio = OGMRIP_AUDIO_CODEC (gobject);

  switch (property_id) 
  {
    case PROP_QUALITY: 
      ogmrip_audio_codec_set_quality (audio, g_value_get_uint (value));
      break;
    case PROP_NORMALIZE: 
      ogmrip_audio_codec_set_normalize (audio, g_value_get_boolean (value));
      break;
    case PROP_CHANNELS: 
      ogmrip_audio_codec_set_channels (audio, g_value_get_int (value));
      break;
    case PROP_SRATE:
      ogmrip_audio_codec_set_sample_rate (audio, g_value_get_uint (value));
      break;
    case PROP_FAST: 
      audio->priv->fast = g_value_get_boolean (value);
      break;
    case PROP_LABEL: 
      g_free (audio->priv->label);
      audio->priv->label = g_value_dup_string (value);
      break;
    case PROP_LANGUAGE: 
      audio->priv->language = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_audio_codec_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipAudioCodec *audio = OGMRIP_AUDIO_CODEC (gobject);

  switch (property_id) 
  {
    case PROP_QUALITY:
      g_value_set_uint (value, audio->priv->quality);
      break;
    case PROP_NORMALIZE:
      g_value_set_boolean (value, audio->priv->normalize);
      break;
    case PROP_CHANNELS:
      g_value_set_int (value, audio->priv->channels);
      break;
    case PROP_SRATE:
      g_value_set_uint (value, audio->priv->srate);
      break;
    case PROP_FAST:
      g_value_set_boolean (value, audio->priv->fast);
      break;
    case PROP_LABEL:
      g_value_set_string (value, audio->priv->label);
      break;
    case PROP_LANGUAGE:
      g_value_set_uint (value, audio->priv->language);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

OGMRipCodec *
ogmrip_audio_codec_new (GType type, OGMRipAudioStream *stream)
{
  g_return_val_if_fail (g_type_is_a (type, OGMRIP_TYPE_AUDIO_CODEC), NULL);
  g_return_val_if_fail (OGMRIP_IS_AUDIO_STREAM (stream), NULL);

  return g_object_new (type, "input", stream, NULL);
}

static const gint sample_rate[] =
{ 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000 };

OGMRipCodec *
ogmrip_audio_codec_new_from_profile (OGMRipAudioStream *stream, OGMRipProfile *profile)
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

  codec = ogmrip_audio_codec_new (type, stream);

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

/**
 * ogmrip_audio_codec_set_fast:
 * @audio: an #OGMRipAudioCodec
 * @fast: %TRUE to enable fast encoding
 *
 * Sets whether to encode faster than realtime.
 */
void
ogmrip_audio_codec_set_fast (OGMRipAudioCodec *audio, gboolean fast)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_CODEC (audio));

  audio->priv->fast = fast;

  g_object_notify (G_OBJECT (audio), "fast");
}

/**
 * ogmrip_audio_codec_get_fast:
 * @audio: an #OGMRipAudioCodec
 *
 * Returns whether to encode faster than realtime.
 *
 * Returns: %TRUE if fast encoding is enabled
 */
gint
ogmrip_audio_codec_get_fast (OGMRipAudioCodec *audio)
{
  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), FALSE);

  return audio->priv->fast;
}

/**
 * ogmrip_audio_codec_set_quality:
 * @audio: an #OGMRipAudioCodec
 * @quality: the quality of the encoding
 *
 * Sets the quality of the encoding, 0 for lowest, 10 for best.
 */
void
ogmrip_audio_codec_set_quality (OGMRipAudioCodec *audio, guint quality)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_CODEC (audio));

  audio->priv->quality = MIN (quality, 10);

  g_object_notify (G_OBJECT (audio), "quality");
}

/**
 * ogmrip_audio_codec_get_quality:
 * @audio: an #OGMRipAudioCodec
 *
 * Gets the quality of the encoding, 0 for lowest, 10 for best.
 *
 * Returns: the quality, or -1
 */
gint
ogmrip_audio_codec_get_quality (OGMRipAudioCodec *audio)
{
  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), -1);

  return audio->priv->quality;
}

/**
 * ogmrip_audio_codec_set_normalize:
 * @audio: an #OGMRipAudioCodec
 * @normalize: %TRUE to enable normalization
 *
 * Sets whether to normalize the volume of the audio stream.
 */
void
ogmrip_audio_codec_set_normalize (OGMRipAudioCodec *audio, gboolean normalize)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_CODEC (audio));

  audio->priv->normalize = normalize;

  g_object_notify (G_OBJECT (audio), "normalize");
}

/**
 * ogmrip_audio_codec_get_normalize:
 * @audio: an #OGMRipAudioCodec
 *
 * Returns whether the volume of the audio stream should be normalized.
 *
 * Returns: %TRUE if normalization is enabled
 */
gboolean
ogmrip_audio_codec_get_normalize (OGMRipAudioCodec *audio)
{
  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), FALSE);

  return audio->priv->normalize;
}

/**
 * ogmrip_audio_codec_set_channels:
 * @audio: an #OGMRipAudioCodec
 * @channels: an #OGMRipChannels
 *
 * Sets the number of channels of the output file.
 */
void
ogmrip_audio_codec_set_channels (OGMRipAudioCodec *audio, OGMRipChannels channels)
{
  OGMRipStream *stream;
  gint max_channels;

  g_return_if_fail (OGMRIP_IS_AUDIO_CODEC (audio));
  
  stream = ogmrip_codec_get_input (OGMRIP_CODEC (audio));
  max_channels = ogmrip_audio_stream_get_channels (OGMRIP_AUDIO_STREAM (stream));
  audio->priv->channels = MIN (channels, max_channels);

  g_object_notify (G_OBJECT (audio), "channels");
}

/**
 * ogmrip_audio_codec_get_channels:
 * @audio: an #OGMRipAudioCodec
 *
 * Gets the number of channels of the output file.
 *
 * Returns: an #OGMRipAudioChannels, or -1
 */
gint
ogmrip_audio_codec_get_channels (OGMRipAudioCodec *audio)
{
  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), -1);

  return audio->priv->channels;
}

/**
 * ogmrip_audio_codec_set_sample_rate:
 * @audio: an #OGMRipAudioCodec
 * @srate: the sample rate
 *
 * Sets the output sample rate in Hz.
 */
void
ogmrip_audio_codec_set_sample_rate (OGMRipAudioCodec *audio, guint srate)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_CODEC (audio));

  audio->priv->srate = srate;

  g_object_notify (G_OBJECT (audio), "sample-rate");
}

/**
 * ogmrip_audio_codec_get_sample_rate:
 * @audio: an #OGMRipAudioCodec
 *
 * Gets the output sample rate in Hz.
 *
 * Returns: the sample rate
 */
gint
ogmrip_audio_codec_get_sample_rate (OGMRipAudioCodec *audio)
{
  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), -1);

  return audio->priv->srate;
}

/**
 * ogmrip_audio_codec_set_label:
 * @audio: an #OGMRipAudioCodec
 * @label: the track name
 *
 * Sets the name of the track.
 */
void
ogmrip_audio_codec_set_label (OGMRipAudioCodec *audio, const gchar *label)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_CODEC (audio));

  if (audio->priv->label)
  {
    g_free (audio->priv->label);
    audio->priv->label = NULL;
  }

  if (label)
    audio->priv->label = g_strdup (label);

  g_object_notify (G_OBJECT (audio), "label");
}

/**
 * ogmrip_audio_codec_get_label:
 * @audio: an #OGMRipAudioCodec
 *
 * Gets the name of the track.
 *
 * Returns: the track name
 */
const gchar *
ogmrip_audio_codec_get_label (OGMRipAudioCodec *audio)
{
  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), NULL);

  return audio->priv->label;
}

/**
 * ogmrip_audio_codec_set_language:
 * @audio: an #OGMRipAudioCodec
 * @language: the language
 *
 * Sets the language of the track.
 */
void
ogmrip_audio_codec_set_language (OGMRipAudioCodec *audio, guint language)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_CODEC (audio));

  audio->priv->language = language;

  g_object_notify (G_OBJECT (audio), "language");
}

/**
 * ogmrip_audio_codec_get_language:
 * @audio: an #OGMRipAudioCodec
 *
 * Gets the language of the track.
 *
 * Returns: the language
 */
gint
ogmrip_audio_codec_get_language (OGMRipAudioCodec *audio)
{
  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), -1);

  return audio->priv->language;
}

