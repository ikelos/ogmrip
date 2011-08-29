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

#ifndef __OGMRIP_MPLAYER_H__
#define __OGMRIP_MPLAYER_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ogmrip-job.h>
#include <ogmrip-container.h>
#include <ogmrip-video-codec.h>
#include <ogmrip-audio-codec.h>
#include <ogmrip-subp-codec.h>

G_BEGIN_DECLS

/*
 * Audio
 */

GPtrArray * ogmrip_mplayer_wav_command        (OGMRipAudioCodec *audio,
                                               gboolean         header,
                                               const gchar      *output);
gdouble  ogmrip_mplayer_wav_watch             (OGMJobExec       *exec,
                                               const gchar      *buffer,
                                               OGMRipAudioCodec *audio);

/*
 * Subtitles
 */

GPtrArray * ogmrip_mencoder_vobsub_command (OGMRipSubpCodec *subp,
                                            const gchar     *output);
gdouble     ogmrip_mencoder_vobsub_watch   (OGMJobExec      *exec, 
                                            const gchar     *buffer, 
                                            OGMRipSubpCodec *subp);

/*
 * Container
 */

GPtrArray * ogmrip_mencoder_container_command (OGMRipContainer *container);
gdouble     ogmrip_mencoder_container_watch   (OGMJobExec      *exec,
                                               const gchar     *buffer,
                                               OGMRipContainer *container);

/*
 * Misc
 */

GPtrArray * ogmrip_mencoder_video_command    (OGMRipVideoCodec  *video,
                                              const gchar       *output,
                                              guint             pass);
GPtrArray * ogmrip_mencoder_audio_command    (OGMRipAudioCodec  *audio,
                                              const gchar       *output);
gdouble     ogmrip_mencoder_codec_watch      (OGMJobExec        *exec,
                                              const gchar       *buffer,
                                              OGMRipCodec       *codec);

GPtrArray * ogmrip_mplayer_video_command     (OGMRipVideoCodec  *video,
                                              const gchar       *output);
gdouble     ogmrip_mplayer_video_watch       (OGMJobExec        *exec,
                                              const gchar       *buffer,
                                              OGMRipVideoCodec  *video);

G_END_DECLS

#endif /* __OGMRIP_MPLAYER_H__ */

