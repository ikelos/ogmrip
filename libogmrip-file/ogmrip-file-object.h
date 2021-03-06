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

#ifndef __OGMRIP_FILE_OBJECT_H__
#define __OGMRIP_FILE_OBJECT_H__

#include <ogmrip-media.h>

G_BEGIN_DECLS

typedef enum
{
  OGMRIP_FILE_ERROR_INFO
} OGMRipFileError;

#define OGMRIP_TYPE_FILE             (ogmrip_file_get_type ())
#define OGMRIP_FILE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_FILE, OGMRipFile))
#define OGMRIP_IS_FILE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_FILE))
#define OGMRIP_FILE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_FILE, OGMRipFileClass))
#define OGMRIP_IS_FILE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_FILE))
#define OGMRIP_FILE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_FILE, OGMRipFileClass))

typedef struct _OGMRipFile      OGMRipFile;
typedef struct _OGMRipFileClass OGMRipFileClass;
typedef struct _OGMRipFilePriv  OGMRipFilePrivate;

struct _OGMRipFile
{
  GObject parent_instance;

  OGMRipFilePrivate *priv;
};

struct _OGMRipFileClass
{
  GObjectClass parent_class;
};

GType         ogmrip_file_get_type (void);
const gchar * ogmrip_file_get_path (OGMRipFile *file);
gboolean      ogmrip_file_delete   (OGMRipFile *file,
                                    GError     **error);

G_END_DECLS

#endif /* __OGMRIP_FILE_OBJECT_H__ */

