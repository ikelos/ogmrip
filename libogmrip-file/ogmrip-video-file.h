/* OGMRipFile - A file library for OGMRip
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

#ifndef __OGMRIP_VIDEO_FILE_H__
#define __OGMRIP_VIDEO_FILE_H__

#include <ogmrip-file-object.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_VIDEO_FILE             (ogmrip_video_file_get_type ())
#define OGMRIP_VIDEO_FILE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_VIDEO_FILE, OGMRipVideoFile))
#define OGMRIP_IS_VIDEO_FILE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_VIDEO_FILE))
#define OGMRIP_VIDEO_FILE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_VIDEO_FILE, OGMRipVideoFileClass))
#define OGMRIP_IS_VIDEO_FILE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_VIDEO_FILE))
#define OGMRIP_VIDEO_FILE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_VIDEO_FILE, OGMRipVideoFileClass))

typedef struct _OGMRipVideoFile      OGMRipVideoFile;
typedef struct _OGMRipVideoFileClass OGMRipVideoFileClass;
typedef struct _OGMRipVideoFilePriv  OGMRipVideoFilePriv;

struct _OGMRipVideoFile
{
  OGMRipFile parent_instance;

  OGMRipVideoFilePriv *priv;
};

struct _OGMRipVideoFileClass
{
  OGMRipFileClass parent_class;
};

GType         ogmrip_video_file_get_type (void);
OGMRipMedia * ogmrip_video_file_new      (const gchar *uri);

G_END_DECLS

#endif /* __OGMRIP_VIDEO_FILE_H__ */

