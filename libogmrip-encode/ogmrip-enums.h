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

#ifndef __OGMRIP_ENUMS_H__
#define __OGMRIP_ENUMS_H__

#include <glib.h>

G_BEGIN_DECLS

/**
 * OGMRipFormatType:
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
 * @OGMRIP_FORMAT_LPCM: The LPCM audio format
 * @OGMRIP_FORMAT_BPCM: The BPCM audio format
 * @OGMRIP_FORMAT_MP12: The MP12 video format
 * @OGMRIP_FORMAT_MJPEG: The MJPEG video format
 * @OGMRIP_FORMAT_FLAC: The Flac audio format
 * @OGMRIP_FORMAT_VP8: The VP8 video format
 * @OGMRIP_FORMAT_CHAPTERS: The chapters format
 *
 * The formats supported by OGMRip.
 */
typedef enum
{
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
  OGMRIP_FORMAT_MICRODVD,
  OGMRIP_FORMAT_SUBRIP,
  OGMRIP_FORMAT_SRT,
  OGMRIP_FORMAT_SAMI,
  OGMRIP_FORMAT_VPLAYER,
  OGMRIP_FORMAT_RT,
  OGMRIP_FORMAT_SSA,
  OGMRIP_FORMAT_PJS,
  OGMRIP_FORMAT_MPSUB,
  OGMRIP_FORMAT_AQT,
  OGMRIP_FORMAT_SRT_2_0,
  OGMRIP_FORMAT_SUBRIP_0_9,
  OGMRIP_FORMAT_JACOSUB,
  OGMRIP_FORMAT_MPL_2,
  OGMRIP_FORMAT_VOBSUB,
  OGMRIP_FORMAT_COPY,
  OGMRIP_FORMAT_LPCM,
  OGMRIP_FORMAT_BPCM,
  OGMRIP_FORMAT_MP12,
  OGMRIP_FORMAT_MJPEG,
  OGMRIP_FORMAT_FLAC,
  OGMRIP_FORMAT_VP8,
  OGMRIP_FORMAT_CHAPTERS,
} OGMRipFormatType;

#define OGMRIP_IS_VIDEO_FORMAT(format)    ((format) == OGMRIP_FORMAT_MPEG1  || \
                                           (format) == OGMRIP_FORMAT_MPEG2  || \
                                           (format) == OGMRIP_FORMAT_MPEG4  || \
                                           (format) == OGMRIP_FORMAT_H264   || \
                                           (format) == OGMRIP_FORMAT_THEORA || \
                                           (format) == OGMRIP_FORMAT_DIRAC  || \
                                           (format) == OGMRIP_FORMAT_MJPEG  || \
                                           (format) == OGMRIP_FORMAT_MP12   || \
                                           (format) == OGMRIP_FORMAT_VP8)

#define OGMRIP_IS_AUDIO_FORMAT(format)    ((format) == OGMRIP_FORMAT_PCM    || \
                                           (format) == OGMRIP_FORMAT_MP3    || \
                                           (format) == OGMRIP_FORMAT_AC3    || \
                                           (format) == OGMRIP_FORMAT_DTS    || \
                                           (format) == OGMRIP_FORMAT_AAC    || \
                                           (format) == OGMRIP_FORMAT_VORBIS || \
                                           (format) == OGMRIP_FORMAT_LPCM   || \
                                           (format) == OGMRIP_FORMAT_BPCM   || \
                                           (format) == OGMRIP_FORMAT_FLAC)

#define OGMRIP_IS_SUBP_FORMAT(format)     ((format) == OGMRIP_FORMAT_MICRODVD   || \
                                           (format) == OGMRIP_FORMAT_SUBRIP     || \
                                           (format) == OGMRIP_FORMAT_SRT        || \
                                           (format) == OGMRIP_FORMAT_SAMI       || \
                                           (format) == OGMRIP_FORMAT_VPLAYER    || \
                                           (format) == OGMRIP_FORMAT_RT         || \
                                           (format) == OGMRIP_FORMAT_SSA        || \
                                           (format) == OGMRIP_FORMAT_PJS        || \
                                           (format) == OGMRIP_FORMAT_MPSUB      || \
                                           (format) == OGMRIP_FORMAT_AQT        || \
                                           (format) == OGMRIP_FORMAT_SRT_2_0    || \
                                           (format) == OGMRIP_FORMAT_SUBRIP_0_9 || \
                                           (format) == OGMRIP_FORMAT_JACOSUB    || \
                                           (format) == OGMRIP_FORMAT_MPL_2      || \
                                           (format) == OGMRIP_FORMAT_VOBSUB)

#define OGMRIP_IS_CHAPTERS_FORMAT(format) ((format) == OGMRIP_FORMAT_CHAPTERS)
/**
 * OGMRipScalerType:
 * @OGMRIP_SCALER_NONE: No scaling
 * @OGMRIP_SCALER_FAST_BILINEAR: Fast bilinear
 * @OGMRIP_SCALER_BILINEAR: Bilinear
 * @OGMRIP_SCALER_BICUBIC: Bicubic (good quality)
 * @OGMRIP_SCALER_EXPERIMENTAL: Experimental
 * @OGMRIP_SCALER_NEAREST_NEIGHBOUR: Nearest neighbour (bad quality)
 * @OGMRIP_SCALER_AREA: Area
 * @OGMRIP_SCALER_LUMA_BICUBIC_CHROMA_BILINEAR: Luma bicubic / Chroma bilinear
 * @OGMRIP_SCALER_GAUSS: Gauss (best for downscaling)
 * @OGMRIP_SCALER_SINCR: SincR
 * @OGMRIP_SCALER_LANCZOS: Lanczos
 * @OGMRIP_SCALER_BICUBIC_SPLINE: Natural bicubic spline
 *
 * Available software scalers.
 */
typedef enum
{
  OGMRIP_SCALER_NONE,
  OGMRIP_SCALER_FAST_BILINEAR,
  OGMRIP_SCALER_BILINEAR,
  OGMRIP_SCALER_BICUBIC,
  OGMRIP_SCALER_EXPERIMENTAL,
  OGMRIP_SCALER_NEAREST_NEIGHBOUR,
  OGMRIP_SCALER_AREA,
  OGMRIP_SCALER_LUMA_BICUBIC_CHROMA_BILINEAR,
  OGMRIP_SCALER_GAUSS,
  OGMRIP_SCALER_SINCR,
  OGMRIP_SCALER_LANCZOS,
  OGMRIP_SCALER_BICUBIC_SPLINE
} OGMRipScalerType;

/**
 * OGMRipDeintType:
 * @OGMRIP_DEINT_NONE: No deinterlacing
 * @OGMRIP_DEINT_LINEAR_BLEND: Linear blend
 * @OGMRIP_DEINT_LINEAR_INTERPOLATING: Linear interpolating
 * @OGMRIP_DEINT_CUBIC_INTERPOLATING: Cubic interpolating
 * @OGMRIP_DEINT_MEDIAN: Median
 * @OGMRIP_DEINT_FFMPEG: FFMpeg
 * @OGMRIP_DEINT_LOWPASS: Lowpass
 * @OGMRIP_DEINT_KERNEL: Kernel
 * @OGMRIP_DEINT_YADIF: Yadif (best)
 *
 * Available deinterlacer filters.
 */
typedef enum
{
  OGMRIP_DEINT_NONE,
  OGMRIP_DEINT_LINEAR_BLEND,
  OGMRIP_DEINT_LINEAR_INTERPOLATING,
  OGMRIP_DEINT_CUBIC_INTERPOLATING,
  OGMRIP_DEINT_MEDIAN,
  OGMRIP_DEINT_FFMPEG,
  OGMRIP_DEINT_LOWPASS,
  OGMRIP_DEINT_KERNEL,
  OGMRIP_DEINT_YADIF
} OGMRipDeintType;

/**
 * OGMRipQualityType:
 * @OGMRIP_QUALITY_EXTREME: Extreme quality
 * @OGMRIP_QUALITY_HIGH: High quality
 * @OGMRIP_QUALITY_NORMAL: Normal quality
 * @OGMRIP_QUALITY_USER: User quality
 *
 * Available quality presets.
 */
typedef enum
{
  OGMRIP_QUALITY_EXTREME,
  OGMRIP_QUALITY_HIGH,
  OGMRIP_QUALITY_NORMAL,
  OGMRIP_QUALITY_USER
} OGMRipQualityType;

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
  OGMRIP_NEWLINE_LF,
  OGMRIP_NEWLINE_CR_LF,
  OGMRIP_NEWLINE_CR
} OGMRipNewline;

/**
 * OGMRipAudioDemuxer:
 * @OGMRIP_AUDIO_DEMUXER_AUTO: The demuxer is autodetected
 * @OGMRIP_AUDIO_DEMUXER_AC3: The AC3 demuxer must be used
 * @OGMRIP_AUDIO_DEMUXER_DTS: The DTS demuxer must be used
 *
 * The audio demuxer to be used when embedding the stream.
 */
typedef enum
{
  OGMRIP_AUDIO_DEMUXER_AUTO = 0,
  OGMRIP_AUDIO_DEMUXER_AC3  = 0x2000,
  OGMRIP_AUDIO_DEMUXER_DTS  = 0x2001
} OGMRipAudioDemuxer;

/**
 * OGMRipSubpDemuxer:
 * @OGMRIP_SUBP_DEMUXER_AUTO: The demuxer is autodetected
 * @OGMRIP_SUBP_DEMUXER_VOBSUB: The VobSub demuxer must be used
 *
 * The subtitle demuxer to be used when embedding the stream.
 */
typedef enum
{
  OGMRIP_SUBP_DEMUXER_AUTO,
  OGMRIP_SUBP_DEMUXER_VOBSUB
} OGMRipSubpDemuxer;

/**
 * OGMRipVideoPreset:
 * @OGMRIP_VIDEO_QUALITY_EXTREME: Extreme preset
 * @OGMRIP_VIDEO_QUALITY_HIGH: High preset
 * @OGMRIP_VIDEO_QUALITY_NORMAL: Normal preset
 * @OGMRIP_VIDEO_QUALITY_USER: User defined preset
 *
 * Available video qualities.
 */
typedef enum
{
  OGMRIP_VIDEO_QUALITY_EXTREME,
  OGMRIP_VIDEO_QUALITY_HIGH,
  OGMRIP_VIDEO_QUALITY_NORMAL,
  OGMRIP_VIDEO_QUALITY_USER
} OGMRipVideoQuality;

G_END_DECLS

#endif /* __OGMRIP_ENUMS_H__ */

