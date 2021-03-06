/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_MEDIA_ENUMS_H__
#define __OGMRIP_MEDIA_ENUMS_H__

#include <glib.h>

G_BEGIN_DECLS

/**
 * OGMRipStandard:
 * @OGMRIP_STANDARD_UNDEFINED: The video standard is undefined
 * @OGMRIP_STANDARD_NTSC: The title is NTSC
 * @OGMRIP_STANDARD_PAL: The title is PAL
 *
 * The standard of the title
 */
typedef enum
{
  OGMRIP_STANDARD_UNDEFINED = -1,
  OGMRIP_STANDARD_NTSC,
  OGMRIP_STANDARD_PAL
} OGMRipStandard;

/**
 * OGMRipChannels:
 * @OGMRIP_CHANNELS_UNDEFINED: Undefined number of channels
 * @OGMRIP_CHANNELS_MONO: The stream is mono
 * @OGMRIP_CHANNELS_STEREO: The stream is stereo
 * @OGMRIP_CHANNELS_SURROUND: The stream is surround
 * @OGMRIP_CHANNELS_5_1: The stream is 5.1
 * @OGMRIP_CHANNELS_6_1: The stream is 6.1
 * @OGMRIP_CHANNELS_7_1: The stream is 7.1
 *
 * The number of channels of the audio stream
 */
typedef enum
{
  OGMRIP_CHANNELS_UNDEFINED = -1,
  OGMRIP_CHANNELS_MONO,
  OGMRIP_CHANNELS_STEREO,
  OGMRIP_CHANNELS_SURROUND,
  OGMRIP_CHANNELS_5_1,
  OGMRIP_CHANNELS_6_1,
  OGMRIP_CHANNELS_7_1
} OGMRipChannels;

/**
 * OGMRipQuantization:
 * @OGMRIP_QUANTIZATION_16: The stream is quantized in 16 bits
 * @OGMRIP_QUANTIZATION_20: The stream is quantized in 20 bits
 * @OGMRIP_QUANTIZATION_24: The stream is quantized in 24 bits
 * @OGMRIP_QUANTIZATION_DRC: The stream is quantized in DRC
 *
 * The quantization of the audio stream
 */
typedef enum
{
  OGMRIP_QUANTIZATION_UNDEFINED = -1,
  OGMRIP_QUANTIZATION_16,
  OGMRIP_QUANTIZATION_20,
  OGMRIP_QUANTIZATION_24,
  OGMRIP_QUANTIZATION_DRC
} OGMRipQuantization;

/**
 * OGMRipAudioContent:
 * @OGMRIP_AUDIO_CONTENT_UNDEFINED: The stream has undefined content
 * @OGMRIP_AUDIO_CONTENT_NORMAL: The stream has normal content
 * @OGMRIP_AUDIO_CONTENT_IMPAIRED: The stream has content for impaired audience
 * @OGMRIP_AUDIO_CONTENT_COMMENTS1: The stream is a commentary
 * @OGMRIP_AUDIO_CONTENT_COMMENTS2: The stream is a commentary
 *
 * The content of the audio stream
 */
typedef enum
{
  OGMRIP_AUDIO_CONTENT_UNDEFINED = -1,
  OGMRIP_AUDIO_CONTENT_NORMAL,
  OGMRIP_AUDIO_CONTENT_IMPAIRED,
  OGMRIP_AUDIO_CONTENT_COMMENTS1,
  OGMRIP_AUDIO_CONTENT_COMMENTS2
} OGMRipAudioContent;

/**
 * OGMRipSubpContent:
 * @OGMRIP_SUBP_CONTENT_UNDEFINED: The stream has undefined content
 * @OGMRIP_SUBP_CONTENT_NORMAL: The stream has normal content
 * @OGMRIP_SUBP_CONTENT_LARGE: The stream has large content
 * @OGMRIP_SUBP_CONTENT_CHILDREN: The stream has content for children
 * @OGMRIP_SUBP_CONTENT_RESERVED1: Reserved
 * @OGMRIP_SUBP_CONTENT_NORMAL_CC: The stream has normal closed caption content
 * @OGMRIP_SUBP_CONTENT_LARGE_CC: The stream has large closed caption content
 * @OGMRIP_SUBP_CONTENT_CHILDREN_CC: The stream has closed caption content for children
 * @OGMRIP_SUBP_CONTENT_RESERVED2: Reserved
 * @OGMRIP_SUBP_CONTENT_FORCED: The stream has forced content
 * @OGMRIP_SUBP_CONTENT_RESERVED3: Reserved
 * @OGMRIP_SUBP_CONTENT_RESERVED4: Reserved
 * @OGMRIP_SUBP_CONTENT_RESERVED5: Reserved
 * @OGMRIP_SUBP_CONTENT_DIRECTOR: The stream has director commentary content
 * @OGMRIP_SUBP_CONTENT_LARGE_DIRECTOR: The stream has large director commentary content
 * @OGMRIP_SUBP_CONTENT_CHILDREN_DIRECTOR: The stream has director commentary content for children
 *
 * The content of the subtitle stream
 */
typedef enum
{
  OGMRIP_SUBP_CONTENT_UNDEFINED = -1,
  OGMRIP_SUBP_CONTENT_NORMAL,
  OGMRIP_SUBP_CONTENT_LARGE,
  OGMRIP_SUBP_CONTENT_CHILDREN,
  OGMRIP_SUBP_CONTENT_RESERVED1,
  OGMRIP_SUBP_CONTENT_NORMAL_CC,
  OGMRIP_SUBP_CONTENT_LARGE_CC,
  OGMRIP_SUBP_CONTENT_CHILDREN_CC,
  OGMRIP_SUBP_CONTENT_RESERVED2,
  OGMRIP_SUBP_CONTENT_FORCED,
  OGMRIP_SUBP_CONTENT_RESERVED3,
  OGMRIP_SUBP_CONTENT_RESERVED4,
  OGMRIP_SUBP_CONTENT_RESERVED5,
  OGMRIP_SUBP_CONTENT_DIRECTOR,
  OGMRIP_SUBP_CONTENT_LARGE_DIRECTOR,
  OGMRIP_SUBP_CONTENT_CHILDREN_DIRECTOR
} OGMRipSubpContent;

/**
 * OGMRipCharset:
 * @OGMRIP_CHARSET_UTF8: UTF-8 charset
 * @OGMRIP_CHARSET_ISO8859_1: ISO8859-1 charset
 * @OGMRIP_CHARSET_ASCII: ASCII
 *
 * Available character sets.
 */
typedef enum
{
  OGMRIP_CHARSET_UNDEFINED = -1,
  OGMRIP_CHARSET_UTF8,
  OGMRIP_CHARSET_ISO8859_1,
  OGMRIP_CHARSET_ASCII
} OGMRipCharset;

/**
 * OGMRipNewline:
 * @OGMRIP_NEWLINE_LF: Line feed only
 * @OGMRIP_NEWLINE_CR_LF: Carriage return + line feed
 * @OGMRIP_NEWLINE_CR: Carriage return only
 *
 * Available end-of-line styles.
 */
typedef enum
{
  OGMRIP_NEWLINE_UNDEFINED = -1,
  OGMRIP_NEWLINE_LF,
  OGMRIP_NEWLINE_CR_LF,
  OGMRIP_NEWLINE_CR
} OGMRipNewline;

/**
 * OGMRipFormat:
 * @OGMRIP_FORMAT_UNDEFINED: The format is undefined
 * @OGMRIP_FORMAT_MPEG1: The Mpeg-1 video format
 * @OGMRIP_FORMAT_MPEG2: The Mpeg-2 video format
 * @OGMRIP_FORMAT_MPEG4: The Mpeg-4 video format
 * @OGMRIP_FORMAT_H264: The H264 video format
 * @OGMRIP_FORMAT_THEORA: The Ogg Theora video format
 * @OGMRIP_FORMAT_DIRAC: The Dirac video format
 * @OGMRIP_FORMAT_PCM: The PCM audio format
 * @OGMRIP_FORMAT_MP3: The MP3 audio format
 * @OGMRIP_FORMAT_AC3: The AC3 audio format
 * @OGMRIP_FORMAT_DTS: The DTS audio format
 * @OGMRIP_FORMAT_AAC: The AAC audio format
 * @OGMRIP_FORMAT_VORBIS: The Ogg Vorbis audio format
 * @OGMRIP_FORMAT_MICRODVD: The MicroDVD subtitle format
 * @OGMRIP_FORMAT_SUBRIP: The SubRip subtitle format
 * @OGMRIP_FORMAT_SRT: The SRT subtitle format
 * @OGMRIP_FORMAT_SAMI: The SAMI subtitle format
 * @OGMRIP_FORMAT_VPLAYER: The VPlayer subtitle format
 * @OGMRIP_FORMAT_RT: The RT subtitle format
 * @OGMRIP_FORMAT_SSA: The SSA subtitle format
 * @OGMRIP_FORMAT_PJS: The RJS subtitle format
 * @OGMRIP_FORMAT_MPSUB: The Mplayer subtitle format
 * @OGMRIP_FORMAT_AQT: The AQT subtitle format
 * @OGMRIP_FORMAT_SRT_2_0: The SRT version 2 subtitle format
 * @OGMRIP_FORMAT_SUBRIP_0_9: The SubRip version 0.9 subtitle format
 * @OGMRIP_FORMAT_JACOSUB: The JacoSub subtitle format
 * @OGMRIP_FORMAT_MPL_2: The MPlayer version subtitle format
 * @OGMRIP_FORMAT_VOBSUB: The VobSub subtitle format
 * @OGMRIP_FORMAT_COPY: A format for internal use only
 * @OGMRIP_FORMAT_MP2: The MP2 video format
 * @OGMRIP_FORMAT_MJPEG: The MJPEG video format
 * @OGMRIP_FORMAT_FLAC: The Flac audio format
 * @OGMRIP_FORMAT_VP8: The VP8 video format
 * @OGMRIP_FORMAT_VP9: The VP9 video format
 * @OGMRIP_FORMAT_CHAPTERS: The chapters format
 *
 * The formats supported by OGMRip.
 */
typedef enum
{
  OGMRIP_FORMAT_UNDEFINED = -1,
  OGMRIP_FORMAT_MPEG1,
  OGMRIP_FORMAT_MPEG2,
  OGMRIP_FORMAT_MPEG4,
  OGMRIP_FORMAT_H264,
  OGMRIP_FORMAT_THEORA,
  OGMRIP_FORMAT_DIRAC,
  OGMRIP_FORMAT_PCM,
  OGMRIP_FORMAT_MP3,
  OGMRIP_FORMAT_AC3,
  OGMRIP_FORMAT_DTS,
  OGMRIP_FORMAT_AAC,
  OGMRIP_FORMAT_VORBIS,
  OGMRIP_FORMAT_SRT,
  OGMRIP_FORMAT_SSA,
  OGMRIP_FORMAT_VOBSUB,
  OGMRIP_FORMAT_COPY,
  OGMRIP_FORMAT_MP2,
  OGMRIP_FORMAT_MJPEG,
  OGMRIP_FORMAT_FLAC,
  OGMRIP_FORMAT_VP8,
  OGMRIP_FORMAT_VP9,
  OGMRIP_FORMAT_CHAPTERS,
  OGMRIP_FORMAT_PGS,
  OGMRIP_FORMAT_VC1,
  OGMRIP_FORMAT_TRUEHD,
  OGMRIP_FORMAT_EAC3,
  OGMRIP_FORMAT_DTS_HD,
  OGMRIP_FORMAT_DTS_HD_MA,
  OGMRIP_FORMAT_LAST
} OGMRipFormat;

#define OGMRIP_IS_VIDEO_FORMAT(format)    ((format) == OGMRIP_FORMAT_MPEG1  || \
                                           (format) == OGMRIP_FORMAT_MPEG2  || \
                                           (format) == OGMRIP_FORMAT_MPEG4  || \
                                           (format) == OGMRIP_FORMAT_H264   || \
                                           (format) == OGMRIP_FORMAT_THEORA || \
                                           (format) == OGMRIP_FORMAT_DIRAC  || \
                                           (format) == OGMRIP_FORMAT_MJPEG  || \
                                           (format) == OGMRIP_FORMAT_VP8    || \
                                           (format) == OGMRIP_FORMAT_VP9    || \
                                           (format) == OGMRIP_FORMAT_VC1)

#define OGMRIP_IS_AUDIO_FORMAT(format)    ((format) == OGMRIP_FORMAT_PCM    || \
                                           (format) == OGMRIP_FORMAT_MP2    || \
                                           (format) == OGMRIP_FORMAT_MP3    || \
                                           (format) == OGMRIP_FORMAT_AC3    || \
                                           (format) == OGMRIP_FORMAT_DTS    || \
                                           (format) == OGMRIP_FORMAT_AAC    || \
                                           (format) == OGMRIP_FORMAT_VORBIS || \
                                           (format) == OGMRIP_FORMAT_FLAC   || \
                                           (format) == OGMRIP_FORMAT_TRUEHD || \
                                           (format) == OGMRIP_FORMAT_EAC3   || \
                                           (format) == OGMRIP_FORMAT_DTS_HD || \
                                           (format) == OGMRIP_FORMAT_DTS_HD_MA)

#define OGMRIP_IS_SUBP_FORMAT(format)     ((format) == OGMRIP_FORMAT_SRT || \
                                           (format) == OGMRIP_FORMAT_SSA || \
                                           (format) == OGMRIP_FORMAT_PGS || \
                                           (format) == OGMRIP_FORMAT_VOBSUB)

#define OGMRIP_IS_CHAPTERS_FORMAT(format) ((format) == OGMRIP_FORMAT_CHAPTERS)

G_END_DECLS

#endif /* __OGMRIP_MEDIA_ENUMS_H__ */

