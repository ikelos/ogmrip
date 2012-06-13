/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2012 Olivier Rolland <billl@users.sourceforge.net>
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
 * SECTION:ogmrip-media-labels
 * @title: Conversion functions
 * @include: ogmrip-media.h
 * @short_description: Converts enumerated types into human readable strings
 */

#include "ogmrip-media-labels.h"

#include <string.h>

const gchar *
ogmrip_format_get_label (OGMRipFormat format)
{
  static const gchar *format_type[] = 
  {
    "Undefined",
    "MPEG1",
    "MPEG2",
    "MPEG4",
    "h264",
    "Theora",
    "Dirac",
    "PCM",
    "MP3",
    "AC3",
    "DTS",
    "AAC",
    "Vorbis",
    "MicroDVD",
    "SubRip",
    "SRT",
    "SAMI",
    "VPLAYER",
    "RT",
    "SSA",
    "PJS",
    "MPSub",
    "AQT",
    "SRT_2_0",
    "SubRip_0_9",
    "JacoSub",
    "MPL_2",
    "VOBSUB",
    "COPY",
    "LPCM",
    "BPCM",
    "MP12",
    "MJPEG",
    "Flac",
    "VP8",
    "Chapters"
  };

  return format_type[format + 1];
}

/**
 * ogmrip_standard_get_label:
 * @format: The standard
 *
 * Returns a human readable standard.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_standard_get_label (OGMRipStandard standard)
{
  static const gchar *video_format[] = 
  {
    "Undefined",
    "NTSC",
    "PAL"
  };

  return video_format[standard + 1];
}

/**
 * ogmrip_aspect_get_label:
 * @aspect: The aspect
 *
 * Returns a human readable aspect.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_aspect_get_label (OGMRipAspect aspect)
{
  static const gchar *display_aspect[] =
  {
    "Undefined",
    "4/3", 
    "16/9",
  };

  return display_aspect[aspect + 1];
}

/**
 * ogmrip_channels_get_label:
 * @channels: The number of channels
 *
 * Returns a human readable number of channels.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_channels_get_label (OGMRipChannels channels)
{
  static const gchar *audio_channels[] =
  {
    "Undefined",
    "Mono",
    "Stereo",
    "Surround",
    "5.1",
    "6.1",
    "7.1"
  };

  return audio_channels[channels + 1];
}

/**
 * ogmrip_quantization_get_label:
 * @quantization: The quantization
 *
 * Returns a human readable quantization.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_quantization_get_label (OGMRipQuantization quantization)
{
  static const gchar *audio_quantization[] = 
  {
    "Undefined",
    "16 bits", 
    "20 bits", 
    "24 bits", 
    "DRC"
  };

  return audio_quantization[quantization + 1];
}

/**
 * ogmrip_audio_content_get_label:
 * @content: The audio content
 *
 * Returns a human readable audio content.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_audio_content_get_label (OGMRipAudioContent content)
{
  static const gchar *audio_content[] = 
  {
    "Undefined", 
    "Normal", 
    "Impaired", 
    "Comments 1", 
    "Comments 2"
  };

  return audio_content[content + 1];
}

/**
 * ogmrip_subp_content_get_label:
 * @content: The subtitles content
 *
 * Returns a human readable subtitles content.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_subp_content_get_label (OGMRipSubpContent content)
{
  static const gchar *subp_content[] = 
  {
    "Undefined", 
    "Normal", 
    "Large", 
    "Children", 
    "Reserved", 
    "Normal CC", 
    "Large CC", 
    "Children CC",
    "Reserved", 
    "Forced", 
    "Reserved", 
    "Reserved", 
    "Reserved", 
    "Director", 
    "Large Director", 
    "Children Director"
  };

  return subp_content[content + 1];
}

