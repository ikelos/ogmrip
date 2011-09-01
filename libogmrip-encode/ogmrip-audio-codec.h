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

#ifndef __OGMRIP_AUDIO_CODEC_H__
#define __OGMRIP_AUDIO_CODEC_H__

#include <ogmrip-codec.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_AUDIO_CODEC           (ogmrip_audio_codec_get_type ())
#define OGMRIP_AUDIO_CODEC(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_AUDIO_CODEC, OGMRipAudioCodec))
#define OGMRIP_AUDIO_CODEC_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_AUDIO_CODEC, OGMRipAudioCodecClass))
#define OGMRIP_IS_AUDIO_CODEC(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_AUDIO_CODEC))
#define OGMRIP_IS_AUDIO_CODEC_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_AUDIO_CODEC))
#define OGMRIP_AUDIO_CODEC_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_AUDIO_CODEC, OGMRipAudioCodecClass))

typedef struct _OGMRipAudioCodec      OGMRipAudioCodec;
typedef struct _OGMRipAudioCodecPriv  OGMRipAudioCodecPriv;
typedef struct _OGMRipAudioCodecClass OGMRipAudioCodecClass;

struct _OGMRipAudioCodec
{
  OGMRipCodec parent_instance;

  OGMRipAudioCodecPriv *priv;
};

struct _OGMRipAudioCodecClass
{
  OGMRipCodecClass parent_class;
};

GType         ogmrip_audio_codec_get_type              (void);
void          ogmrip_audio_codec_set_channels          (OGMRipAudioCodec    *audio,
                                                        OGMRipChannels      channels);
gint          ogmrip_audio_codec_get_channels          (OGMRipAudioCodec    *audio);
void          ogmrip_audio_codec_set_fast              (OGMRipAudioCodec    *audio,
                                                        gboolean            fast);
gboolean      ogmrip_audio_codec_get_fast              (OGMRipAudioCodec    *audio);
void          ogmrip_audio_codec_set_label             (OGMRipAudioCodec    *audio,
                                                        const gchar         *label);
const gchar * ogmrip_audio_codec_get_label             (OGMRipAudioCodec    *audio);
void          ogmrip_audio_codec_set_language          (OGMRipAudioCodec    *audio,
                                                        guint               language);
gint          ogmrip_audio_codec_get_language          (OGMRipAudioCodec    *audio);
void          ogmrip_audio_codec_set_normalize         (OGMRipAudioCodec    *audio,
                                                        gboolean            normalize);
gboolean      ogmrip_audio_codec_get_normalize         (OGMRipAudioCodec    *audio);
void          ogmrip_audio_codec_set_quality           (OGMRipAudioCodec    *audio,
                                                        guint               quality);
gint          ogmrip_audio_codec_get_quality           (OGMRipAudioCodec    *audio);
void          ogmrip_audio_codec_set_sample_rate       (OGMRipAudioCodec    *audio,
                                                        guint               srate);
gint          ogmrip_audio_codec_get_sample_rate       (OGMRipAudioCodec    *audio);
gint          ogmrip_audio_codec_get_samples_per_frame (OGMRipAudioCodec    *audio);

G_END_DECLS

#endif /* __OGMRIP_AUDIO_CODEC_H__ */

