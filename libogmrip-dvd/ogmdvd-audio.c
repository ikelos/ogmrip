/* OGMDvd - A wrapper library around libdvdread
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

/**
 * SECTION:ogmdvd-audio
 * @title: OGMDvdAudio
 * @include: ogmdvd-audio.h
 * @short_description: Structure describing an audio stream
 */

#include "ogmdvd-audio.h"
#include "ogmdvd-title.h"
#include "ogmdvd-priv.h"

enum
{
  OGMDVD_AUDIO_FORMAT_AC3      = 0,
  OGMDVD_AUDIO_FORMAT_MPEG1    = 2,
  OGMDVD_AUDIO_FORMAT_MPEG2EXT = 3,
  OGMDVD_AUDIO_FORMAT_LPCM     = 4,
  OGMDVD_AUDIO_FORMAT_SDDS     = 5,
  OGMDVD_AUDIO_FORMAT_DTS      = 6
};

static void ogmrip_stream_iface_init       (OGMRipStreamInterface      *iface);
static void ogmrip_audio_stream_iface_init (OGMRipAudioStreamInterface *iface);

G_DEFINE_TYPE_WITH_CODE (OGMDvdAudioStream, ogmdvd_audio_stream, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_STREAM, ogmrip_stream_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_AUDIO_STREAM, ogmrip_audio_stream_iface_init));

static void
ogmdvd_audio_stream_init (OGMDvdAudioStream *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream, OGMDVD_TYPE_AUDIO_STREAM, OGMDvdAudioStreamPriv);
}

static void
ogmdvd_audio_stream_class_init (OGMDvdAudioStreamClass *klass)
{
  g_type_class_add_private (klass, sizeof (OGMDvdAudioStreamPriv));
}

static gint
ogmdvd_audio_stream_get_format (OGMRipStream *stream)
{
  switch (OGMDVD_AUDIO_STREAM (stream)->priv->format)
  {
    case OGMDVD_AUDIO_FORMAT_AC3:
      return OGMRIP_FORMAT_AC3;
    case OGMDVD_AUDIO_FORMAT_LPCM:
      return OGMRIP_FORMAT_LPCM;
    case OGMDVD_AUDIO_FORMAT_DTS:
      return OGMRIP_FORMAT_DTS;
    default:
      return OGMRIP_FORMAT_UNDEFINED;
  }
}

static gint
ogmdvd_audio_stream_get_id (OGMRipStream *stream)
{
  return OGMDVD_AUDIO_STREAM (stream)->priv->id;
}

OGMRipTitle *
ogmdvd_audio_stream_get_title (OGMRipStream *stream)
{
  return OGMDVD_AUDIO_STREAM (stream)->priv->title;
}

static void
ogmrip_stream_iface_init (OGMRipStreamInterface *iface)
{
  iface->get_format = ogmdvd_audio_stream_get_format;
  iface->get_id     = ogmdvd_audio_stream_get_id;
  iface->get_title  = ogmdvd_audio_stream_get_title;
}

static gint
ogmdvd_audio_stream_get_bitrate (OGMRipAudioStream *audio)
{
  OGMDvdAudioStream *stream = OGMDVD_AUDIO_STREAM (audio);

  if (stream->priv->title)
  {
    OGMDvdTitle *title = OGMDVD_TITLE (stream->priv->title);

    if (title->priv->bitrates)
      return title->priv->bitrates[stream->priv->nr];
  }

  return 0;
}

static gint
ogmdvd_audio_stream_get_channels (OGMRipAudioStream *audio)
{
  switch (OGMDVD_AUDIO_STREAM (audio)->priv->channels)
  {
    case 0:
      return OGMRIP_AUDIO_CHANNELS_MONO;
    case 1:
      return OGMRIP_AUDIO_CHANNELS_STEREO;
    case 3:
      return OGMRIP_AUDIO_CHANNELS_SURROUND;
    case 5:
      return OGMRIP_AUDIO_CHANNELS_5_1;
    default:
      return OGMRIP_AUDIO_CHANNELS_UNDEFINED;
  }
}

static gint
ogmdvd_audio_stream_get_content (OGMRipAudioStream *audio)
{
  return OGMDVD_AUDIO_STREAM (audio)->priv->code_extension - 1;
}

static gint
ogmdvd_audio_stream_get_language (OGMRipAudioStream *audio)
{
  return OGMDVD_AUDIO_STREAM (audio)->priv->lang_code;
}

static gint
ogmdvd_audio_stream_get_nr (OGMRipAudioStream *stream)
{
  return OGMDVD_AUDIO_STREAM (stream)->priv->nr;
}

static gint
ogmdvd_audio_stream_get_quantization (OGMRipAudioStream *audio)
{
  return OGMDVD_AUDIO_STREAM (audio)->priv->quantization;
}

static gint
ogmdvd_audio_stream_get_sample_rate (OGMRipAudioStream *audio)
{
  return 48000;
}

static void
ogmrip_audio_stream_iface_init (OGMRipAudioStreamInterface *iface)
{
  iface->get_bitrate      = ogmdvd_audio_stream_get_bitrate;
  iface->get_channels     = ogmdvd_audio_stream_get_channels;
  iface->get_content      = ogmdvd_audio_stream_get_content;
  iface->get_language     = ogmdvd_audio_stream_get_language;
  iface->get_nr           = ogmdvd_audio_stream_get_nr;
  iface->get_quantization = ogmdvd_audio_stream_get_quantization;
  iface->get_sample_rate  = ogmdvd_audio_stream_get_sample_rate;
}

