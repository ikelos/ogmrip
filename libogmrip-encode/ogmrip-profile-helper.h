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

#ifndef __OGMRIP_PROFILE_HELPER_H__
#define __OGMRIP_PROFILE_HELPER_H__

#include <ogmrip-profile.h>
#include <ogmrip-encoding.h>
#include <ogmrip-container.h>
#include <ogmrip-codec.h>

G_BEGIN_DECLS

OGMRipContainer * ogmrip_create_container   (OGMRipProfile     *profile);
OGMRipCodec *     ogmrip_create_video_codec (OGMRipVideoStream *stream,
                                             OGMRipProfile     *profile);
OGMRipCodec *     ogmrip_create_audio_codec (OGMRipAudioStream *stream,
                                             OGMRipProfile     *profile);
OGMRipCodec *     ogmrip_create_subp_codec  (OGMRipSubpStream  *stream,
                                             OGMRipProfile     *profile);

G_END_DECLS

#endif /* __OGMRIP_PROFILE_HELPER_H__ */

