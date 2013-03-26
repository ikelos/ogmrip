/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2013 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_CHAPTER_VIEW_H__
#define __OGMRIP_CHAPTER_VIEW_H__

#include <ogmrip-chapter-store.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_CHAPTER_VIEW            (ogmrip_chapter_view_get_type ())
#define OGMRIP_CHAPTER_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_CHAPTER_VIEW, OGMRipChapterView))
#define OGMRIP_CHAPTER_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_CHAPTER_VIEW, OGMRipChapterViewClass))
#define OGMRIP_IS_CHAPTER_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_CHAPTER_VIEW))
#define OGMRIP_IS_CHAPTER_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_CHAPTER_VIEW))

typedef struct _OGMRipChapterView      OGMRipChapterView;
typedef struct _OGMRipChapterViewClass OGMRipChapterViewClass;
typedef struct _OGMRipChapterViewPriv  OGMRipChapterViewPriv;

struct _OGMRipChapterView
{
  GtkTreeView parent_instance;

  OGMRipChapterViewPriv *priv;
};

struct _OGMRipChapterViewClass
{
  GtkTreeViewClass parent_class;
};

GType       ogmrip_chapter_view_get_type (void);
GtkWidget * ogmrip_chapter_view_new      (void);

G_END_DECLS

#endif /* __OGMRIP_CHAPTER_VIEW_H__ */

