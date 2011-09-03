/* OGMDvd - A wrapper library around libdvdread
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#include <ogmrip-media.h>

G_BEGIN_DECLS

#define OGMDVD_TYPE_AUDIO_STREAM             (ogmdvd_audio_stream_get_type ())
#define OGMDVD_AUDIO_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMDVD_TYPE_AUDIO_STREAM, OGMDvdAudioStream))
#define OGMDVD_IS_AUDIO_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMDVD_TYPE_AUDIO_STREAM))
#define OGMDVD_AUDIO_STREAM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMDVD_TYPE_AUDIO_STREAM, OGMDvdAudioStreamClass))
#define OGMDVD_IS_AUDIO_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMDVD_TYPE_AUDIO_STREAM))
#define OGMDVD_AUDIO_STREAM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMDVD_TYPE_AUDIO_STREAM, OGMDvdAudioStreamClass))

typedef struct _OGMDvdAudioStream      OGMDvdAudioStream;
typedef struct _OGMDvdAudioStreamClass OGMDvdAudioStreamClass;
typedef struct _OGMDvdAudioStreamPriv  OGMDvdAudioStreamPriv;

struct _OGMDvdAudioStream
{
  GObject parent_instance;

  OGMDvdAudioStreamPriv *priv;
};

struct _OGMDvdAudioStreamClass
{
  GObjectClass parent_class;
};

GType ogmdvd_audio_stream_get_type (void);

G_END_DECLS

#endif /* __OGMDVD_AUDIO_H__ */

