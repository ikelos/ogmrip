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

#ifndef __OGMRIP_MEDIA_AUDIO_H__
#define __OGMRIP_MEDIA_AUDIO_H__

#include <ogmrip-media-types.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_AUDIO_STREAM            (ogmrip_audio_stream_get_type ())
#define OGMRIP_AUDIO_STREAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_AUDIO_STREAM, OGMRipAudioStream))
#define OGMRIP_IS_AUDIO_STREAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_AUDIO_STREAM))
#define OGMRIP_AUDIO_STREAM_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMRIP_TYPE_AUDIO_STREAM, OGMRipAudioStreamInterface))

typedef struct _OGMRipAudioStreamInterface OGMRipAudioStreamInterface;

struct _OGMRipAudioStreamInterface
{
  GTypeInterface base_iface;

  gint          (* get_bitrate)      (OGMRipAudioStream *audio);
  gint          (* get_channels)     (OGMRipAudioStream *audio);
  gint          (* get_content)      (OGMRipAudioStream *audio);
  const gchar * (* get_label)        (OGMRipAudioStream *audio);
  gint          (* get_language)     (OGMRipAudioStream *audio);
  gint          (* get_nr)           (OGMRipAudioStream *audio);
  gint          (* get_quantization) (OGMRipAudioStream *audio);
  gint          (* get_sample_rate)  (OGMRipAudioStream *audio);
};

GType         ogmrip_audio_stream_get_type              (void) G_GNUC_CONST;
gint          ogmrip_audio_stream_get_bitrate           (OGMRipAudioStream *audio);
gint          ogmrip_audio_stream_get_channels          (OGMRipAudioStream *audio);
gint          ogmrip_audio_stream_get_content           (OGMRipAudioStream *audio);
const gchar * ogmrip_audio_stream_get_label             (OGMRipAudioStream *audio);
gint          ogmrip_audio_stream_get_language          (OGMRipAudioStream *audio);
gint          ogmrip_audio_stream_get_nr                (OGMRipAudioStream *audio);
gint          ogmrip_audio_stream_get_quantization      (OGMRipAudioStream *audio);
gint          ogmrip_audio_stream_get_sample_rate       (OGMRipAudioStream *audio);
gint          ogmrip_audio_stream_get_samples_per_frame (OGMRipAudioStream *audio);

G_END_DECLS

#endif /* __OGMRIP_MEDIA_AUDIO_H__ */

