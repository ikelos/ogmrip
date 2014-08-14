/* OGMRipFile - A file library for OGMRip
 * Copyright (C) 2010-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_AUDIO_FILE_H__
#define __OGMRIP_AUDIO_FILE_H__

#include <ogmrip-file-object.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_AUDIO_FILE             (ogmrip_audio_file_get_type ())
#define OGMRIP_AUDIO_FILE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_AUDIO_FILE, OGMRipAudioFile))
#define OGMRIP_IS_AUDIO_FILE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_AUDIO_FILE))
#define OGMRIP_AUDIO_FILE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_AUDIO_FILE, OGMRipAudioFileClass))
#define OGMRIP_IS_AUDIO_FILE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_AUDIO_FILE))
#define OGMRIP_AUDIO_FILE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_AUDIO_FILE, OGMRipAudioFileClass))

typedef struct _OGMRipAudioFile      OGMRipAudioFile;
typedef struct _OGMRipAudioFileClass OGMRipAudioFileClass;
typedef struct _OGMRipAudioFilePriv  OGMRipAudioFilePrivate;

struct _OGMRipAudioFile
{
  OGMRipFile parent_instance;

  OGMRipAudioFilePrivate *priv;
};

struct _OGMRipAudioFileClass
{
  OGMRipFileClass parent_class;
};

GType         ogmrip_audio_file_get_type     (void);
OGMRipMedia * ogmrip_audio_file_new          (const gchar     *uri);
void          ogmrip_audio_file_set_language (OGMRipAudioFile *file,
                                              guint           language);

G_END_DECLS

#endif /* __OGMRIP_AUDIO_FILE_H__ */

