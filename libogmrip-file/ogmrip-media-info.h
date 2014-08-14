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

#ifndef __OGMRIP_MEDIA_INFO_H__
#define __OGMRIP_MEDIA_INFO_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_MEDIA_INFO          (ogmrip_media_info_get_type ())
#define OGMRIP_MEDIA_INFO(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MEDIA_INFO, OGMRipMediaInfo))
#define OGMRIP_MEDIA_INFO_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_MEDIA_INFO, OGMRipMediaInfoClass))
#define OGMRIP_IS_MEDIA_INFO(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_MEDIA_INFO))
#define OGMRIP_IS_MEDIA_INFO_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_MEDIA_INFO))

typedef struct _OGMRipMediaInfo      OGMRipMediaInfo;
typedef struct _OGMRipMediaInfoClass OGMRipMediaInfoClass;
typedef struct _OGMRipMediaInfoPriv  OGMRipMediaInfoPrivate;

struct _OGMRipMediaInfo
{
  GObject parent_instance;

  OGMRipMediaInfoPrivate *priv;
};

struct _OGMRipMediaInfoClass
{
  GObjectClass parent_class;
};

typedef enum
{
  OGMRIP_CATEGORY_GENERAL,
  OGMRIP_CATEGORY_VIDEO,
  OGMRIP_CATEGORY_AUDIO,
  OGMRIP_CATEGORY_TEXT,
  OGMRIP_CATEGORY_CHAPTERS,
  OGMRIP_CATEGORY_IMAGE,
  OGMRIP_CATEGORY_MENU
} OGMRipCategoryType;

GType             ogmrip_media_info_get_type    (void);
OGMRipMediaInfo * ogmrip_media_info_get_default (void);
gboolean          ogmrip_media_info_open        (OGMRipMediaInfo    *info,
                                                 const gchar        *filename);
void              ogmrip_media_info_close       (OGMRipMediaInfo    *info);
const gchar *     ogmrip_media_info_get         (OGMRipMediaInfo    *info,
                                                 OGMRipCategoryType category,
                                                 guint              stream,
                                                 const gchar        *name);

G_END_DECLS

#endif /* __OGMRIP_MEDIA_INFO_H__ */

