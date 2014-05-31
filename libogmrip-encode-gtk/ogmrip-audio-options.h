/* OGMRip - A library for media ripping and encoding
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

#ifndef __OGMRIP_AUDIO_OPTIONS_H__
#define __OGMRIP_AUDIO_OPTIONS_H__

#include <ogmrip-media.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_AUDIO_OPTIONS            (ogmrip_audio_options_get_type ())
#define OGMRIP_AUDIO_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_AUDIO_OPTIONS, OGMRipAudioOptions))
#define OGMRIP_IS_AUDIO_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_AUDIO_OPTIONS))
#define OGMRIP_AUDIO_OPTIONS_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMRIP_TYPE_AUDIO_OPTIONS, OGMRipAudioOptionsInterface))

typedef struct _OGMRipAudioOptions          OGMRipAudioOptions;
typedef struct _OGMRipAudioOptionsInterface OGMRipAudioOptionsInterface;

struct _OGMRipAudioOptionsInterface
{
  GTypeInterface base_iface;
};

typedef enum
{
  OGMRIP_SAMPLE_RATE_UNDEFINED = -1,
  OGMRIP_SAMPLE_RATE_48000,
  OGMRIP_SAMPLE_RATE_44100,
  OGMRIP_SAMPLE_RATE_32000,
  OGMRIP_SAMPLE_RATE_24000,
  OGMRIP_SAMPLE_RATE_22050,
  OGMRIP_SAMPLE_RATE_16000,
  OGMRIP_SAMPLE_RATE_12000,
  OGMRIP_SAMPLE_RATE_11025,
  OGMRIP_SAMPLE_RATE_8000
} OGMRipSampleRate;

GType    ogmrip_audio_options_get_type        (void);
guint    ogmrip_audio_options_get_channels    (OGMRipAudioOptions  *options);
void     ogmrip_audio_options_set_channels    (OGMRipAudioOptions  *options,
                                               OGMRipChannels      channels);
GType    ogmrip_audio_options_get_codec       (OGMRipAudioOptions  *options);
void     ogmrip_audio_options_set_codec       (OGMRipAudioOptions  *options,
                                               GType               codec);
gboolean ogmrip_audio_options_get_normalize   (OGMRipAudioOptions  *options);
void     ogmrip_audio_options_set_normalize   (OGMRipAudioOptions  *options,
                                               gboolean            normalize);
guint    ogmrip_audio_options_get_quality     (OGMRipAudioOptions  *options);
void     ogmrip_audio_options_set_quality     (OGMRipAudioOptions  *options,
                                               guint               quality);
guint    ogmrip_audio_options_get_sample_rate (OGMRipAudioOptions  *options);
void     ogmrip_audio_options_set_sample_rate (OGMRipAudioOptions  *options,
                                               OGMRipSampleRate    srate);

G_END_DECLS

#endif /* __OGMRIP_AUDIO_OPTIONS_H__ */

