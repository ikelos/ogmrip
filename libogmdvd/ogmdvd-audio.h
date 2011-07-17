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

#ifndef __OGMDVD_AUDIO_H__
#define __OGMDVD_AUDIO_H__

#include <ogmdvd-types.h>

G_BEGIN_DECLS

gint ogmdvd_audio_stream_get_format       (OGMDvdAudioStream *audio);
gint ogmdvd_audio_stream_get_channels     (OGMDvdAudioStream *audio);
gint ogmdvd_audio_stream_get_language     (OGMDvdAudioStream *audio);
gint ogmdvd_audio_stream_get_quantization (OGMDvdAudioStream *audio);
gint ogmdvd_audio_stream_get_content      (OGMDvdAudioStream *audio);
gint ogmdvd_audio_stream_get_bitrate      (OGMDvdAudioStream *audio);

G_END_DECLS

#endif /* __OGMDVD_H__ */

