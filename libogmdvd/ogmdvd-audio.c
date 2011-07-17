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
#include "ogmdvd-priv.h"

/**
 * ogmdvd_audio_stream_get_format:
 * @audio: An #OGMDvdAudioStream
 *
 * Returns the format of the audio stream.
 *
 * Returns: #OGMDvdAudioFormat, or -1
 */
gint
ogmdvd_audio_stream_get_format (OGMDvdAudioStream *audio)
{
  g_return_val_if_fail (audio != NULL, -1);

  return audio->format;
}

/**
 * ogmdvd_audio_stream_get_channels:
 * @audio: An #OGMDvdAudioStream
 *
 * Returns the number of channels of the audio stream.
 *
 * Returns: #OGMDvdAudioChannels, or -1
 */
gint
ogmdvd_audio_stream_get_channels (OGMDvdAudioStream *audio)
{
  g_return_val_if_fail (audio != NULL, -1);

  return audio->channels;
}

/**
 * ogmdvd_audio_stream_get_quantization:
 * @audio: An #OGMDvdAudioStream
 *
 * Returns the quantization of the audio stream.
 *
 * Returns: #OGMDvdAudioQuantization, or -1
 */
gint
ogmdvd_audio_stream_get_quantization (OGMDvdAudioStream *audio)
{
  g_return_val_if_fail (audio != NULL, -1);

  return audio->quantization;
}

/**
 * ogmdvd_audio_stream_get_content:
 * @audio: An #OGMDvdAudioStream
 *
 * Returns the content of the audio stream.
 *
 * Returns: #OGMDvdAudioContent, or -1
 */
gint
ogmdvd_audio_stream_get_content (OGMDvdAudioStream *audio)
{
  g_return_val_if_fail (audio != NULL, -1);

  return audio->code_extension;
}

/**
 * ogmdvd_audio_stream_get_language:
 * @audio: An #OGMDvdAudioStream
 *
 * Returns the language of the audio stream.
 *
 * Returns: the language code, or -1
 */
gint
ogmdvd_audio_stream_get_language (OGMDvdAudioStream *audio)
{
  g_return_val_if_fail (audio != NULL, -1);

  return audio->lang_code;
}

/**
 * ogmdvd_audio_stream_get_bitrate:
 * @audio: An #OGMDvdAudioStream
 *
 * Returns the bitrate of the audio stream in bps.
 *
 * Returns: the bitrate, or -1
 */
gint
ogmdvd_audio_stream_get_bitrate (OGMDvdAudioStream *audio)
{
  OGMDvdStream *stream;

  g_return_val_if_fail (audio != NULL, -1);

  stream = OGMDVD_STREAM (audio);

  if (stream->title && stream->title->bitrates)
    return stream->title->bitrates[stream->nr];

  return 0;
}

