/* OGMDvd - A wrapper library around libdvdread
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

#ifndef __OGMDVD_CHAPTER_LIST_H__
#define __OGMDVD_CHAPTER_LIST_H__

#include <gtk/gtk.h>

#include <ogmdvd.h>

G_BEGIN_DECLS

#define OGMDVD_TYPE_CHAPTER_LIST            (ogmdvd_chapter_list_get_type ())
#define OGMDVD_CHAPTER_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMDVD_TYPE_CHAPTER_LIST, OGMDvdChapterList))
#define OGMDVD_CHAPTER_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMDVD_TYPE_CHAPTER_LIST, OGMDvdChapterListClass))
#define OGMDVD_IS_CHAPTER_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMDVD_TYPE_CHAPTER_LIST))
#define OGMDVD_IS_CHAPTER_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMDVD_TYPE_CHAPTER_LIST))

typedef struct _OGMDvdChapterList      OGMDvdChapterList;
typedef struct _OGMDvdChapterListClass OGMDvdChapterListClass;
typedef struct _OGMDvdChapterListPriv  OGMDvdChapterListPriv;

struct _OGMDvdChapterList
{
  GtkTreeView widget;
  OGMDvdChapterListPriv *priv;
};

struct _OGMDvdChapterListClass
{
  GtkTreeViewClass parent_class;
};

GType         ogmdvd_chapter_list_get_type  (void);
GtkWidget *   ogmdvd_chapter_list_new       (void);
void          ogmdvd_chapter_list_set_title (OGMDvdChapterList *list,
                                             OGMDvdTitle       *title);
OGMDvdTitle * ogmdvd_chapter_list_get_title (OGMDvdChapterList *list);
gchar *       ogmdvd_chapter_list_get_label (OGMDvdChapterList *list,
                                             guint             chapter);
void          ogmdvd_chapter_list_set_label (OGMDvdChapterList *list,
                                             guint             chapter,
                                             const gchar       *label);

G_END_DECLS

#endif /* __OGMDVD_CHAPTER_LIST_H__ */

