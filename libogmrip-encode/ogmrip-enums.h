/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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

