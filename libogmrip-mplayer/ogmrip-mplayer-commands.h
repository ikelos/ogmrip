/* OGMRipMplayer - A library around mplayer/mencoder for OGMRip
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_MPLAYER_COMMANDS_H__
#define __OGMRIP_MPLAYER_COMMANDS_H__

#include <ogmrip-encode.h>

G_BEGIN_DECLS

typedef enum
{
  OGMRIP_ENCODER_XVID,
  OGMRIP_ENCODER_LAVC,
  OGMRIP_ENCODER_X264,
  OGMRIP_ENCODER_YUV,
  OGMRIP_ENCODER_PCM,
  OGMRIP_ENCODER_WAV,
  OGMRIP_ENCODER_VOBSUB,
  OGMRIP_ENCODER_COPY
} OGMRipEncoder;

void         ogmrip_mplayer_set_input   (GPtrArray        *argc,
                                         OGMRipTitle      *title,
                                         gint             angle);
OGMJobTask * ogmrip_video_encoder_new   (OGMRipVideoCodec *codec,
                                         OGMRipEncoder    encoder,
                                         const gchar      *options,
                                         const gchar      *passlog,
                                         const gchar      *output);
OGMJobTask * ogmrip_audio_encoder_new   (OGMRipAudioCodec *codec,
                                         OGMRipEncoder    encoder,
                                         const gchar      *options,
                                         const gchar      *output);
OGMJobTask * ogmrip_subp_encoder_new    (OGMRipSubpCodec  *codec,
                                         OGMRipEncoder    encoder,
                                         const gchar      *options,
                                         const gchar      *output);
OGMJobTask * ogmrip_video_extractor_new (OGMRipContainer  *container,
                                         OGMRipFile       *file,
                                         const gchar      *output);


G_END_DECLS

#endif /* __OGMRIP_MPLAYER_COMMANDS_H__ */

