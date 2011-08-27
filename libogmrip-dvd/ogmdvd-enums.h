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

#ifndef __OGMDVD_ENUMS_H__
#define __OGMDVD_ENUMS_H__

#include <glib.h>

G_BEGIN_DECLS

/**
 * OGMDvdVideoFormat:
 * @OGMDVD_VIDEO_FORMAT_NTSC: The title is NTSC
 * @OGMDVD_VIDEO_FORMAT_PAL: The title is PAL
 *
 * The video format of the DVD title
 */
typedef enum
{
  OGMDVD_VIDEO_FORMAT_NTSC = 0,
  OGMDVD_VIDEO_FORMAT_PAL  = 1
} OGMDvdVideoFormat;

/**
 * OGMDvdDisplayAspect:
 * @OGMDVD_DISPLAY_ASPECT_4_3: The title is 4/3
 * @OGMDVD_DISPLAY_ASPECT_16_9: The title is 16/9
 *
 * The display aspect of the DVD title
 */
typedef enum
{
  OGMDVD_DISPLAY_ASPECT_4_3  = 0,
  OGMDVD_DISPLAY_ASPECT_16_9 = 1
} OGMDvdDisplayAspect;

/**
 * OGMDvdDisplayFormat:
 * @OGMDVD_DISPLAY_FORMAT_PS_LETTER: The title is pan & scan letter
 * @OGMDVD_DISPLAY_FORMAT_PAN_SCAN: The title is is pan & scan
 * @OGMDVD_DISPLAY_FORMAT_LETTERBOX: The title is letterbox
 *
 * The display format of the DVD title
 */
typedef enum
{
  OGMDVD_DISPLAY_FORMAT_PS_LETTER = 0,
  OGMDVD_DISPLAY_FORMAT_PAN_SCAN  = 1,
  OGMDVD_DISPLAY_FORMAT_LETTERBOX = 2
} OGMDvdDisplayFormat;

/**
 * OGMDvdAudioFormat:
 * @OGMDVD_AUDIO_FORMAT_AC3: The stream is in AC3
 * @OGMDVD_AUDIO_FORMAT_MPEG1: The stream is in Mpeg-1
 * @OGMDVD_AUDIO_FORMAT_MPEG2EXT: The stream is in Mpeg-2 extended
 * @OGMDVD_AUDIO_FORMAT_LPCM: The stream is in LPCM
 * @OGMDVD_AUDIO_FORMAT_SDDS: The stream is in SDDS
 * @OGMDVD_AUDIO_FORMAT_DTS: The stream is in DTS
 *
 * The format of the audio stream
 */
typedef enum
{
  OGMDVD_AUDIO_FORMAT_AC3      = 0,
  OGMDVD_AUDIO_FORMAT_MPEG1    = 2,
  OGMDVD_AUDIO_FORMAT_MPEG2EXT = 3,
  OGMDVD_AUDIO_FORMAT_LPCM     = 4,
  OGMDVD_AUDIO_FORMAT_SDDS     = 5,
  OGMDVD_AUDIO_FORMAT_DTS      = 6
} OGMDvdAudioFormat;

/**
 * OGMDvdAudioChannels:
 * @OGMDVD_AUDIO_CHANNELS_MONO: The stream is mono
 * @OGMDVD_AUDIO_CHANNELS_STEREO: The stream is stereo
 * @OGMDVD_AUDIO_CHANNELS_SURROUND: The stream is surround
 * @OGMDVD_AUDIO_CHANNELS_5_1: The stream is 5.1
 *
 * The number of channels of the audio stream
 */
typedef enum
{
  OGMDVD_AUDIO_CHANNELS_MONO      = 0,
  OGMDVD_AUDIO_CHANNELS_STEREO    = 1,
  OGMDVD_AUDIO_CHANNELS_SURROUND  = 3,
  OGMDVD_AUDIO_CHANNELS_5_1       = 5
} OGMDvdAudioChannels;

/**
 * OGMDvdAudioQuantization:
 * @OGMDVD_AUDIO_QUANTIZATION_16: The stream is quantized in 16 bits
 * @OGMDVD_AUDIO_QUANTIZATION_20: The stream is quantized in 20 bits
 * @OGMDVD_AUDIO_QUANTIZATION_24: The stream is quantized in 24 bits
 * @OGMDVD_AUDIO_QUANTIZATION_DRC: The stream is quantized in DRC
 *
 * The quantization of the audio stream
 */
typedef enum
{
  OGMDVD_AUDIO_QUANTIZATION_16  = 0,
  OGMDVD_AUDIO_QUANTIZATION_20  = 1,
  OGMDVD_AUDIO_QUANTIZATION_24  = 2,
  OGMDVD_AUDIO_QUANTIZATION_DRC = 3
} OGMDvdAudioQuantization;

/**
 * OGMDvdAudioContent:
 * @OGMDVD_AUDIO_CONTENT_UNDEFINED: The stream has undefined content
 * @OGMDVD_AUDIO_CONTENT_NORMAL: The stream has normal content
 * @OGMDVD_AUDIO_CONTENT_IMPAIRED: The stream has content for impaired audience
 * @OGMDVD_AUDIO_CONTENT_COMMENTS1: The stream is a commentary
 * @OGMDVD_AUDIO_CONTENT_COMMENTS2: The stream is a commentary
 *
 * The content of the audio stream
 */
typedef enum
{
  OGMDVD_AUDIO_CONTENT_UNDEFINED = 0,
  OGMDVD_AUDIO_CONTENT_NORMAL    = 1,
  OGMDVD_AUDIO_CONTENT_IMPAIRED  = 2,
  OGMDVD_AUDIO_CONTENT_COMMENTS1 = 3,
  OGMDVD_AUDIO_CONTENT_COMMENTS2 = 4
} OGMDvdAudioContent;

/**
 * OGMDvdSubpContent:
 * @OGMDVD_SUBP_CONTENT_UNDEFINED: The stream has undefined content
 * @OGMDVD_SUBP_CONTENT_NORMAL: The stream has normal content
 * @OGMDVD_SUBP_CONTENT_LARGE: The stream has large content
 * @OGMDVD_SUBP_CONTENT_CHILDREN: The stream has content for children
 * @OGMDVD_SUBP_CONTENT_RESERVED1: Reserved
 * @OGMDVD_SUBP_CONTENT_NORMAL_CC: The stream has normal closed caption content
 * @OGMDVD_SUBP_CONTENT_LARGE_CC: The stream has large closed caption content
 * @OGMDVD_SUBP_CONTENT_CHILDREN_CC: The stream has closed caption content for children
 * @OGMDVD_SUBP_CONTENT_RESERVED2: Reserved
 * @OGMDVD_SUBP_CONTENT_FORCED: The stream has forced content
 * @OGMDVD_SUBP_CONTENT_RESERVED3: Reserved
 * @OGMDVD_SUBP_CONTENT_RESERVED4: Reserved
 * @OGMDVD_SUBP_CONTENT_RESERVED5: Reserved
 * @OGMDVD_SUBP_CONTENT_DIRECTOR: The stream has director commentary content
 * @OGMDVD_SUBP_CONTENT_LARGE_DIRECTOR: The stream has large director commentary content
 * @OGMDVD_SUBP_CONTENT_CHILDREN_DIRECTOR: The stream has director commentary content for children
 *
 * The content of the subtitle stream
 */
typedef enum
{
  OGMDVD_SUBP_CONTENT_UNDEFINED,
  OGMDVD_SUBP_CONTENT_NORMAL,
  OGMDVD_SUBP_CONTENT_LARGE,
  OGMDVD_SUBP_CONTENT_CHILDREN,
  OGMDVD_SUBP_CONTENT_RESERVED1,
  OGMDVD_SUBP_CONTENT_NORMAL_CC,
  OGMDVD_SUBP_CONTENT_LARGE_CC,
  OGMDVD_SUBP_CONTENT_CHILDREN_CC,
  OGMDVD_SUBP_CONTENT_RESERVED2,
  OGMDVD_SUBP_CONTENT_FORCED,
  OGMDVD_SUBP_CONTENT_RESERVED3,
  OGMDVD_SUBP_CONTENT_RESERVED4,
  OGMDVD_SUBP_CONTENT_RESERVED5,
  OGMDVD_SUBP_CONTENT_DIRECTOR,
  OGMDVD_SUBP_CONTENT_LARGE_DIRECTOR,
  OGMDVD_SUBP_CONTENT_CHILDREN_DIRECTOR
} OGMDvdSubpContent;

enum
{
  OGMDVD_LANGUAGE_ISO639_1,
  OGMDVD_LANGUAGE_ISO639_2,
  OGMDVD_LANGUAGE_NAME
};

G_END_DECLS

#endif /* __OGMDVD_ENUMS_H__ */

