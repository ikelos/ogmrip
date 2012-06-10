/* OGMRipBluray - A bluray library for OGMRip
 * Copyright (C) 2012 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMBR_AUDIO_H__
#define __OGMBR_AUDIO_H__

#include <ogmrip-media.h>

G_BEGIN_DECLS

#define OGMBR_TYPE_AUDIO_STREAM             (ogmbr_audio_stream_get_type ())
#define OGMBR_AUDIO_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMBR_TYPE_AUDIO_STREAM, OGMBrAudioStream))
#define OGMBR_IS_AUDIO_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMBR_TYPE_AUDIO_STREAM))
#define OGMBR_AUDIO_STREAM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMBR_TYPE_AUDIO_STREAM, OGMBrAudioStreamClass))
#define OGMBR_IS_AUDIO_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMBR_TYPE_AUDIO_STREAM))
#define OGMBR_AUDIO_STREAM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMBR_TYPE_AUDIO_STREAM, OGMBrAudioStreamClass))

typedef struct _OGMBrAudioStream      OGMBrAudioStream;
typedef struct _OGMBrAudioStreamClass OGMBrAudioStreamClass;
typedef struct _OGMBrAudioStreamPriv  OGMBrAudioStreamPriv;

struct _OGMBrAudioStream
{
  GObject parent_instance;

  OGMBrAudioStreamPriv *priv;
};

struct _OGMBrAudioStreamClass
{
  GObjectClass parent_class;
};

GType ogmbr_audio_stream_get_type (void);

G_END_DECLS

#endif /* __OGMBR_AUDIO_H__ */

