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

#ifndef __OGMRIP_VIDEO_CODEC_H__
#define __OGMRIP_VIDEO_CODEC_H__

#include <ogmrip-codec.h>
#include <ogmrip-enums.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_VIDEO_CODEC           (ogmrip_video_codec_get_type ())
#define OGMRIP_VIDEO_CODEC(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_VIDEO_CODEC, OGMRipVideoCodec))
#define OGMRIP_VIDEO_CODEC_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_VIDEO_CODEC, OGMRipVideoCodecClass))
#define OGMRIP_IS_VIDEO_CODEC(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_VIDEO_CODEC))
#define OGMRIP_IS_VIDEO_CODEC_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_VIDEO_CODEC))
#define OGMRIP_VIDEO_CODEC_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_VIDEO_CODEC, OGMRipVideoCodecClass))

typedef struct _OGMRipVideoCodec      OGMRipVideoCodec;
typedef struct _OGMRipVideoCodecPriv  OGMRipVideoCodecPriv;
typedef struct _OGMRipVideoCodecClass OGMRipVideoCodecClass;

struct _OGMRipVideoCodec
{
  OGMRipCodec parent_instance;

  OGMRipVideoCodecPriv *priv;
};

struct _OGMRipVideoCodecClass
{
  OGMRipCodecClass parent_class;

  /* signals */
  void (* pass) (OGMRipVideoCodec *video,
                 guint       pass);
};

GType               ogmrip_video_codec_get_type           (void);

gint                ogmrip_video_codec_get_angle          (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_set_angle          (OGMRipVideoCodec  *video,
                                                           guint             angle);
gint                ogmrip_video_codec_get_bitrate        (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_set_bitrate        (OGMRipVideoCodec  *video,
                                                           guint             bitrate);
gboolean            ogmrip_video_codec_get_can_crop       (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_set_can_crop       (OGMRipVideoCodec  *video,
                                                           gboolean          can_crop);
gdouble             ogmrip_video_codec_get_quantizer      (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_set_quantizer      (OGMRipVideoCodec  *video,
                                                           gdouble           quantizer);
gdouble             ogmrip_video_codec_get_bits_per_pixel (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_set_bits_per_pixel (OGMRipVideoCodec  *video,
                                                           gdouble           bpp);
gint                ogmrip_video_codec_get_passes         (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_set_passes         (OGMRipVideoCodec  *video,
                                                           guint             pass);
gint                ogmrip_video_codec_get_threads        (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_set_threads        (OGMRipVideoCodec  *video,
                                                           guint             threads);
gint                ogmrip_video_codec_get_scaler         (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_set_scaler         (OGMRipVideoCodec  *video,
                                                           OGMRipScalerType  scaler);
gint                ogmrip_video_codec_get_deinterlacer   (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_set_deinterlacer   (OGMRipVideoCodec  *video,
                                                           OGMRipDeintType   deint);
gboolean            ogmrip_video_codec_get_turbo          (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_set_turbo          (OGMRipVideoCodec  *video,
                                                           gboolean          turbo);
gboolean            ogmrip_video_codec_get_denoise        (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_set_denoise        (OGMRipVideoCodec  *video,
                                                           gboolean          denoise);
gint                ogmrip_video_codec_get_quality        (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_set_quality        (OGMRipVideoCodec  *video,
                                                           OGMRipQualityType quality);
gboolean            ogmrip_video_codec_get_deblock        (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_set_deblock        (OGMRipVideoCodec  *video,
                                                           gboolean          deblock);
gboolean            ogmrip_video_codec_get_dering         (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_set_dering         (OGMRipVideoCodec  *video,
                                                           gboolean          dering);
gint                ogmrip_video_codec_get_start_delay    (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_get_raw_size       (OGMRipVideoCodec  *video,
                                                           guint             *width,
                                                           guint             *height);
gboolean            ogmrip_video_codec_get_crop_size      (OGMRipVideoCodec  *video,
                                                           guint             *x,
                                                           guint             *y,
                                                           guint             *width,
                                                           guint             *height);
void                ogmrip_video_codec_set_crop_size      (OGMRipVideoCodec  *video,
                                                           guint             x,
                                                           guint             y,
                                                           guint             width,
                                                           guint             height);
gboolean            ogmrip_video_codec_get_scale_size     (OGMRipVideoCodec  *video,
                                                           guint             *width,
                                                           guint             *height);
void                ogmrip_video_codec_set_scale_size     (OGMRipVideoCodec  *video,
                                                           guint             width,
                                                           guint             height);
gboolean            ogmrip_video_codec_get_max_size       (OGMRipVideoCodec  *video,
                                                           guint             *width,
                                                           guint             *height,
                                                           gboolean          *expand);
void                ogmrip_video_codec_set_max_size       (OGMRipVideoCodec  *video,
                                                           guint             width,
                                                           guint             height,
                                                           gboolean          expand);
gboolean            ogmrip_video_codec_get_min_size       (OGMRipVideoCodec  *video,
                                                           guint             *width,
                                                           guint             *height);
void                ogmrip_video_codec_set_min_size       (OGMRipVideoCodec  *video,
                                                           guint             width,
                                                           guint             height);
void                ogmrip_video_codec_get_aspect_ratio   (OGMRipVideoCodec  *video,
                                                           guint             *num,
                                                           guint             *denom);
void                ogmrip_video_codec_set_aspect_ratio   (OGMRipVideoCodec  *video,
                                                           guint             num,
                                                           guint             denom);
void                ogmrip_video_codec_get_framerate      (OGMRipVideoCodec  *video,
                                                           guint             *num,
                                                           guint             *denom);
OGMRipAudioStream * ogmrip_video_codec_get_ensure_sync    (OGMRipVideoCodec  *video);
void                ogmrip_video_codec_set_ensure_sync    (OGMRipVideoCodec  *video,
                                                           OGMRipAudioStream *stream);

OGMRipSubpStream *  ogmrip_video_codec_get_hard_subp      (OGMRipVideoCodec  *video,
                                                           gboolean          *forced);
void                ogmrip_video_codec_set_hard_subp      (OGMRipVideoCodec  *video,
                                                           OGMRipSubpStream  *stream,
                                                           gboolean          forced);

G_END_DECLS

#endif /* __OGMRIP_VIDEO_CODEC_H__ */

