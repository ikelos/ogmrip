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
 * SECTION:ogmrip-audio-codec
 * @title: OGMRipAudioCodec
 * @short_description: Base class for audio codecs
 * @include: ogmrip-audio-codec.h
 */

#include "ogmrip-audio-codec.h"

#define DEFAULT_SPF 1024

#define OGMRIP_AUDIO_CODEC_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_AUDIO_CODEC, OGMRipAudioCodecPriv))

struct _OGMRipAudioCodecPriv
{
  guint srate;
  guint quality;
  guint samples_per_frame;

  gboolean fast;
  gboolean normalize;

  gchar *label;

  OGMDvdAudioChannels channels;
  OGMDvdAudioStream *stream;
};

enum 
{
  PROP_0,
  PROP_STREAM,
  PROP_QUALITY,
  PROP_NORMALIZE,
  PROP_CHANNELS,
  PROP_SPF,
  PROP_FAST
};

static void ogmrip_audio_codec_dispose      (GObject      *gobject);
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

  gobject_class->dispose = ogmrip_audio_codec_dispose;
  gobject_class->finalize = ogmrip_audio_codec_finalize;
  gobject_class->set_property = ogmrip_audio_codec_set_property;
  gobject_class->get_property = ogmrip_audio_codec_get_property;

  g_object_class_install_property (gobject_class, PROP_STREAM, 
        g_param_spec_pointer ("stream", "Audio stream property", "Set audio stream", 
           G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_QUALITY, 
        g_param_spec_uint ("quality", "Quality property", "Set quality", 
           0, 10, 3, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_NORMALIZE, 
        g_param_spec_boolean ("normalize", "Normalize property", "Set normalize", 
           FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHANNELS, 
        g_param_spec_uint ("channels", "Channels property", "Set channels", 
           0, 10, OGMDVD_AUDIO_CHANNELS_STEREO, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SPF, 
        g_param_spec_uint ("samples-per-frame", "Samples per frame property", "Set samples per frame", 
           0, G_MAXUINT, 512, G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_FAST, 
        g_param_spec_boolean ("fast", "Fast property", "Set fast", 
           FALSE, G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (OGMRipAudioCodecPriv));
}

static void
ogmrip_audio_codec_init (OGMRipAudioCodec *audio)
{
  audio->priv = OGMRIP_AUDIO_CODEC_GET_PRIVATE (audio);
  audio->priv->channels = OGMDVD_AUDIO_CHANNELS_STEREO;

  audio->priv->srate = 48000;
  audio->priv->quality = 3;
}

static void
ogmrip_audio_codec_dispose (GObject *gobject)
{
  OGMRipAudioCodec *audio;

  audio = OGMRIP_AUDIO_CODEC (gobject);
  if (audio->priv->stream)
  {
    ogmdvd_stream_unref (OGMDVD_STREAM (audio->priv->stream));
    audio->priv->stream = NULL;
  }

  G_OBJECT_CLASS (ogmrip_audio_codec_parent_class)->dispose (gobject);
}

static void
ogmrip_audio_codec_finalize (GObject *gobject)
{
  OGMRipAudioCodec *audio;

  audio = OGMRIP_AUDIO_CODEC (gobject);
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
  OGMRipAudioCodec *audio;

  audio = OGMRIP_AUDIO_CODEC (gobject);

  switch (property_id) 
  {
    case PROP_STREAM:
      ogmrip_audio_codec_set_dvd_audio_stream (audio, g_value_get_pointer (value));
      break;
    case PROP_QUALITY: 
      ogmrip_audio_codec_set_quality (audio, g_value_get_uint (value));
      break;
    case PROP_NORMALIZE: 
      ogmrip_audio_codec_set_normalize (audio, g_value_get_boolean (value));
      break;
    case PROP_CHANNELS: 
      ogmrip_audio_codec_set_channels (audio, g_value_get_uint (value));
      break;
    case PROP_FAST: 
      audio->priv->fast = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_audio_codec_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipAudioCodec *audio;
  gint spf;

  audio = OGMRIP_AUDIO_CODEC (gobject);

  switch (property_id) 
  {
    case PROP_STREAM:
      g_value_set_pointer (value, audio->priv->stream);
      break;
    case PROP_QUALITY:
      g_value_set_uint (value, audio->priv->quality);
      break;
    case PROP_NORMALIZE:
      g_value_set_boolean (value, audio->priv->normalize);
      break;
    case PROP_CHANNELS:
      g_value_set_uint (value, audio->priv->channels);
      break;
    case PROP_SPF:
      spf = ogmrip_audio_codec_get_samples_per_frame (audio);
      g_value_set_uint (value, spf > 0 ? spf : DEFAULT_SPF);
      break;
    case PROP_FAST:
      g_value_set_boolean (value, audio->priv->fast);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
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
 * ogmrip_audio_codec_set_dvd_audio_stream:
 * @audio: an #OGMRipAudioCodec
 * @stream: an #OGMDvdAudioStream
 *
 * Sets the audio stream to encode.
 */
void
ogmrip_audio_codec_set_dvd_audio_stream (OGMRipAudioCodec *audio, OGMDvdAudioStream *stream)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_CODEC (audio));
  g_return_if_fail (stream != NULL);

  if (audio->priv->stream != stream)
  {
    ogmdvd_stream_ref (OGMDVD_STREAM (stream));

    if (audio->priv->stream)
      ogmdvd_stream_unref (OGMDVD_STREAM (audio->priv->stream));
    audio->priv->stream = stream;

    ogmrip_codec_set_input (OGMRIP_CODEC (audio), 
        ogmdvd_stream_get_title (OGMDVD_STREAM (stream)));

    ogmrip_audio_codec_set_channels (audio, audio->priv->channels);
  }
}

/**
 * ogmrip_audio_codec_get_dvd_audio_stream:
 * @audio: an #OGMRipAudioCodec
 *
 * Gets the audio stream to encode.
 *
 * Returns: an #OGMDvdAudioStream, or NULL
 */
OGMDvdAudioStream *
ogmrip_audio_codec_get_dvd_audio_stream (OGMRipAudioCodec *audio)
{
  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), NULL);

  return audio->priv->stream;
}

/**
 * ogmrip_audio_codec_set_channels:
 * @audio: an #OGMRipAudioCodec
 * @channels: an #OGMDvdAudioChannels
 *
 * Sets the number of channels of the output file.
 */
void
ogmrip_audio_codec_set_channels (OGMRipAudioCodec *audio, OGMDvdAudioChannels channels)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_CODEC (audio));
  
  audio->priv->channels = MIN (channels, 
      ogmdvd_audio_stream_get_channels (audio->priv->stream));
}

/**
 * ogmrip_audio_codec_get_channels:
 * @audio: an #OGMRipAudioCodec
 *
 * Gets the number of channels of the output file.
 *
 * Returns: an #OGMDvdAudioChannels, or -1
 */
gint
ogmrip_audio_codec_get_channels (OGMRipAudioCodec *audio)
{
  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), -1);

  return audio->priv->channels;
}

/**
 * ogmrip_audio_codec_get_samples_per_frame:
 * @audio: an #OGMRipAudioCodec
 *
 * Gets the number of samples per frame.
 *
 * Returns: the number of samples per frame, or -1
 */
gint
ogmrip_audio_codec_get_samples_per_frame (OGMRipAudioCodec *audio)
{
  OGMRipAudioCodecClass *klass;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), -1);

  klass = OGMRIP_AUDIO_CODEC_GET_CLASS (audio);
  if (klass->get_samples_per_frame)
    return (* klass->get_samples_per_frame) (audio);

  return DEFAULT_SPF;
}

/**
 * ogmrip_audio_codec_set_sample_rate:
 * @audio: an #OGMRipAudioCodec
 * @srate: the sample rate
 *
 * Sets the output sample rate to be used.
 */
void
ogmrip_audio_codec_set_sample_rate (OGMRipAudioCodec *audio, guint srate)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_CODEC (audio));

  audio->priv->srate = srate;
}

/**
 * ogmrip_audio_codec_get_sample_rate:
 * @audio: an #OGMRipAudioCodec
 *
 * Gets the output sample rate.
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
}

/**
 * ogmrip_audio_codec_get_label:
 * @audio: an #OGMRipAudioCodec
 *
 * Gets the name of the track.
 *
 * Returns: the track name
 */
G_CONST_RETURN gchar *
ogmrip_audio_codec_get_label (OGMRipAudioCodec *audio)
{
  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), NULL);

  return audio->priv->label;
}

