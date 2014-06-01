/* OGMRipBluray - A bluray library for OGMRip
 * Copyright (C) 2012 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-bluray-audio.h"
#include "ogmrip-bluray-title.h"
#include "ogmrip-bluray-priv.h"

static void ogmbr_stream_iface_init       (OGMRipStreamInterface      *iface);
static void ogmbr_audio_stream_iface_init (OGMRipAudioStreamInterface *iface);

G_DEFINE_TYPE_WITH_CODE (OGMBrAudioStream, ogmbr_audio_stream, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_STREAM, ogmbr_stream_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_AUDIO_STREAM, ogmbr_audio_stream_iface_init));

static void
ogmbr_audio_stream_init (OGMBrAudioStream *audio)
{
  audio->priv = G_TYPE_INSTANCE_GET_PRIVATE (audio, OGMBR_TYPE_AUDIO_STREAM, OGMBrAudioStreamPriv);
}

static void
ogmbr_audio_stream_class_init (OGMBrAudioStreamClass *klass)
{
  g_type_class_add_private (klass, sizeof (OGMBrAudioStreamPriv));
}

static gint
ogmbr_audio_stream_get_format (OGMRipStream *stream)
{
  return OGMBR_AUDIO_STREAM (stream)->priv->format;
}

static gint
ogmbr_audio_stream_get_id (OGMRipStream *stream)
{
  return OGMBR_AUDIO_STREAM (stream)->priv->id;
}

static OGMRipTitle *
ogmbr_audio_stream_get_title (OGMRipStream *stream)
{
  return OGMBR_AUDIO_STREAM (stream)->priv->title;
}

static void
ogmbr_stream_iface_init (OGMRipStreamInterface *iface)
{
  iface->get_format = ogmbr_audio_stream_get_format;
  iface->get_id     = ogmbr_audio_stream_get_id;
  iface->get_title  = ogmbr_audio_stream_get_title;
}

static gint
ogmbr_audio_stream_get_bitrate (OGMRipAudioStream *audio)
{
  return OGMBR_AUDIO_STREAM (audio)->priv->bitrate;
}

static gint
ogmbr_audio_stream_get_channels (OGMRipAudioStream *audio)
{
  switch (OGMBR_AUDIO_STREAM (audio)->priv->channels)
  {
    case 1:
      return OGMRIP_CHANNELS_MONO;
    case 2:
      return OGMRIP_CHANNELS_STEREO;
    case 4:
      return OGMRIP_CHANNELS_SURROUND;
    case 6:
      return OGMRIP_CHANNELS_5_1;
    case 7:
      return OGMRIP_CHANNELS_6_1;
    case 8:
      return OGMRIP_CHANNELS_7_1;
    default:
      return OGMRIP_CHANNELS_UNDEFINED;
  }
}

static gint
ogmbr_audio_stream_get_content (OGMRipAudioStream *audio)
{
  return OGMBR_AUDIO_STREAM (audio)->priv->content;
}

static gint
ogmbr_audio_stream_get_language (OGMRipAudioStream *audio)
{
  return OGMBR_AUDIO_STREAM (audio)->priv->language;
}

static gint
ogmbr_audio_sterma_get_sample_rate (OGMRipAudioStream *audio)
{
  return OGMBR_AUDIO_STREAM (audio)->priv->samplerate;
}

static void
ogmbr_audio_stream_iface_init (OGMRipAudioStreamInterface *iface)
{
  iface->get_bitrate     = ogmbr_audio_stream_get_bitrate;
  iface->get_channels    = ogmbr_audio_stream_get_channels;
  iface->get_content     = ogmbr_audio_stream_get_content;
  iface->get_language    = ogmbr_audio_stream_get_language;
  iface->get_sample_rate = ogmbr_audio_sterma_get_sample_rate;
}

