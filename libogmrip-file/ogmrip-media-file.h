/* OGMRipFile - A file library for OGMRip
 * Copyright (C) 2010-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_MEDIA_FILE_H__
#define __OGMRIP_MEDIA_FILE_H__

#include <ogmrip-video-file.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_MEDIA_FILE             (ogmrip_media_file_get_type ())
#define OGMRIP_MEDIA_FILE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MEDIA_FILE, OGMRipMediaFile))
#define OGMRIP_IS_MEDIA_FILE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_MEDIA_FILE))
#define OGMRIP_MEDIA_FILE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_MEDIA_FILE, OGMRipMediaFileClass))
#define OGMRIP_IS_MEDIA_FILE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_MEDIA_FILE))
#define OGMRIP_MEDIA_FILE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_MEDIA_FILE, OGMRipMediaFileClass))

typedef struct _OGMRipMediaFile      OGMRipMediaFile;
typedef struct _OGMRipMediaFileClass OGMRipMediaFileClass;
typedef struct _OGMRipMediaFilePriv  OGMRipMediaFilePriv;

struct _OGMRipMediaFile
{
  OGMRipVideoFile parent_instance;

  OGMRipMediaFilePriv *priv;
};

struct _OGMRipMediaFileClass
{
  OGMRipVideoFileClass parent_class;
};

void          ogmrip_file_register_media (void);

GType         ogmrip_media_file_get_type (void);
OGMRipMedia * ogmrip_media_file_new      (const gchar *uri);

G_END_DECLS

#endif /* __OGMRIP_MEDIA_FILE_H__ */

