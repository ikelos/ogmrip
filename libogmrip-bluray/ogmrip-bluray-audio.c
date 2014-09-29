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
  OGMRipFormat format = OGMRIP_FORMAT_UNDEFINED;

  switch (OGMBR_AUDIO_STREAM (stream)->priv->type)
  {
    case BLURAY_STREAM_TYPE_AUDIO_MPEG1:
      format = OGMRIP_FORMAT_MPEG1;
      break;
    case BLURAY_STREAM_TYPE_AUDIO_MPEG2:
      format = OGMRIP_FORMAT_MPEG2;
      break;
    case BLURAY_STREAM_TYPE_AUDIO_LPCM:
      format = OGMRIP_FORMAT_PCM;
      break;
    case BLURAY_STREAM_TYPE_AUDIO_AC3:
      format = OGMRIP_FORMAT_AC3;
      break;
    case BLURAY_STREAM_TYPE_AUDIO_DTS:
      format = OGMRIP_FORMAT_DTS;
      break;
    case BLURAY_STREAM_TYPE_AUDIO_TRUHD:
      format = OGMRIP_FORMAT_TRUEHD;
      break;
    case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS:
      format = OGMRIP_FORMAT_EAC3;
      break;
    case BLURAY_STREAM_TYPE_AUDIO_DTSHD:
      format = OGMRIP_FORMAT_DTS_HD;
      break;
    case BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER:
      format = OGMRIP_FORMAT_DTS_HD_MA;
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  return format;
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
  /* TODO bitrate */
  return 0;
}

static gint
ogmbr_audio_stream_get_channels (OGMRipAudioStream *audio)
{
  OGMRipChannels channels;

  switch (OGMBR_AUDIO_STREAM (audio)->priv->format)
  {
    case BLURAY_AUDIO_FORMAT_MONO:
      channels = OGMRIP_CHANNELS_MONO;
      break;
    case BLURAY_AUDIO_FORMAT_STEREO:
      channels = OGMRIP_CHANNELS_STEREO;
      break;
    case BLURAY_AUDIO_FORMAT_MULTI_CHAN:
      channels = OGMRIP_CHANNELS_5_1;
      break;
    case BLURAY_AUDIO_FORMAT_COMBO:
      channels = OGMRIP_CHANNELS_UNDEFINED;
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  return channels;
}

static gint
ogmbr_audio_stream_get_content (OGMRipAudioStream *audio)
{
  /* TODO content */
  return 0;
}

static gint
ogmbr_audio_stream_get_language (OGMRipAudioStream *audio)
{
  return OGMBR_AUDIO_STREAM (audio)->priv->lang;
}

static gint
ogmbr_audio_stream_get_sample_rate (OGMRipAudioStream *audio)
{
  gint rate;

  switch (OGMBR_AUDIO_STREAM (audio)->priv->rate)
  {
    case BLURAY_AUDIO_RATE_48:
      rate = 48000;
      break;
    case BLURAY_AUDIO_RATE_96:
      rate = 96000;
      break;
    case BLURAY_AUDIO_RATE_192:
      rate = 192000;
      break;
    case BLURAY_AUDIO_RATE_192_COMBO:
      rate = 192000;
      break;
    case BLURAY_AUDIO_RATE_96_COMBO:
      rate = 96000;
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  return rate;
}

static void
ogmbr_audio_stream_iface_init (OGMRipAudioStreamInterface *iface)
{
  iface->get_bitrate     = ogmbr_audio_stream_get_bitrate;
  iface->get_channels    = ogmbr_audio_stream_get_channels;
  iface->get_content     = ogmbr_audio_stream_get_content;
  iface->get_language    = ogmbr_audio_stream_get_language;
  iface->get_sample_rate = ogmbr_audio_stream_get_sample_rate;
}

