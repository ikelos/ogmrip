/* OGMRipMEdia - A media library for OGMRip
 * Copyright (C) 2010 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-media-audio.h"
#include "ogmrip-media-stream.h"
#include "ogmrip-media-enums.h"

G_DEFINE_INTERFACE_WITH_CODE (OGMRipAudioStream, ogmrip_audio_stream, G_TYPE_OBJECT,
    g_type_interface_add_prerequisite (g_define_type_id, OGMRIP_TYPE_STREAM);)

static void
ogmrip_audio_stream_default_init (OGMRipAudioStreamInterface *iface)
{
}

gint 
ogmrip_audio_stream_get_bitrate (OGMRipAudioStream *audio)
{
  OGMRipAudioStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_STREAM (audio), -1);

  iface = OGMRIP_AUDIO_STREAM_GET_IFACE (audio);

  if (!iface->get_bitrate)
    return -1;

  return iface->get_bitrate (audio);
}

gint 
ogmrip_audio_stream_get_channels (OGMRipAudioStream *audio)
{
  OGMRipAudioStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_STREAM (audio), -1);

  iface = OGMRIP_AUDIO_STREAM_GET_IFACE (audio);

  if (!iface->get_channels)
    return OGMRIP_CHANNELS_UNDEFINED;

  return iface->get_channels (audio);
}

gint 
ogmrip_audio_stream_get_content (OGMRipAudioStream *audio)
{
  OGMRipAudioStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_STREAM (audio), -1);

  iface = OGMRIP_AUDIO_STREAM_GET_IFACE (audio);

  if (!iface->get_content)
    return OGMRIP_AUDIO_CONTENT_UNDEFINED;

  return iface->get_content (audio);
}

const gchar * 
ogmrip_audio_stream_get_label (OGMRipAudioStream *audio)
{
  OGMRipAudioStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_STREAM (audio), NULL);

  iface = OGMRIP_AUDIO_STREAM_GET_IFACE (audio);

  if (!iface->get_label)
    return NULL;

  return iface->get_label (audio);
}

gint 
ogmrip_audio_stream_get_language (OGMRipAudioStream *audio)
{
  OGMRipAudioStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_STREAM (audio), -1);

  iface = OGMRIP_AUDIO_STREAM_GET_IFACE (audio);

  if (!iface->get_language)
    return 0;

  return iface->get_language (audio);
}

gint
ogmrip_audio_stream_get_nr (OGMRipAudioStream *audio)
{
  OGMRipAudioStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_STREAM (audio), -1);

  iface = OGMRIP_AUDIO_STREAM_GET_IFACE (audio);

  if (!iface->get_nr)
    return 0;

  return iface->get_nr (audio);
}

gint 
ogmrip_audio_stream_get_quantization (OGMRipAudioStream *audio)
{
  OGMRipAudioStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_STREAM (audio), -1);

  iface = OGMRIP_AUDIO_STREAM_GET_IFACE (audio);

  if (!iface->get_quantization)
    return OGMRIP_QUANTIZATION_UNDEFINED;

  return iface->get_quantization (audio);
}

gint 
ogmrip_audio_stream_get_sample_rate (OGMRipAudioStream *audio)
{
  OGMRipAudioStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_STREAM (audio), -1);

  iface = OGMRIP_AUDIO_STREAM_GET_IFACE (audio);

  if (!iface->get_sample_rate)
    return -1;

  return iface->get_sample_rate (audio);
}

gint
ogmrip_audio_stream_get_samples_per_frame (OGMRipAudioStream *audio)
{
  gint format;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_STREAM (audio), -1);

  format = ogmrip_stream_get_format (OGMRIP_STREAM (audio));

  if (format == OGMRIP_FORMAT_MP3)
    return 1152;

  if (format == OGMRIP_FORMAT_AC3 || format == OGMRIP_FORMAT_DTS)
    return 1536;

  return 1024;
}

