/* OGMRipMedia - A media library for OGMRip
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

#ifndef __OGMRIP_CHAPTER_STORE_H__
#define __OGMRIP_CHAPTER_STORE_H__

#include <gtk/gtk.h>

#include <ogmrip-media.h>

G_BEGIN_DECLS

typedef enum
{
  OGMRIP_CHAPTER_STORE_CHAPTER_COLUMN,
  OGMRIP_CHAPTER_STORE_LABEL_COLUMN,
  OGMRIP_CHAPTER_STORE_DURATION_COLUMN,
  OGMRIP_CHAPTER_STORE_SELECTED_COLUMN,
  OGMRIP_CHAPTER_STORE_N_COLUMNS
} OGMRipChapterStoreColumns;

#define OGMRIP_TYPE_CHAPTER_STORE            (ogmrip_chapter_store_get_type ())
#define OGMRIP_CHAPTER_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_CHAPTER_STORE, OGMRipChapterStore))
#define OGMRIP_CHAPTER_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_CHAPTER_STORE, OGMRipChapterStoreClass))
#define OGMRIP_IS_CHAPTER_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_CHAPTER_STORE))
#define OGMRIP_IS_CHAPTER_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_CHAPTER_STORE))

typedef struct _OGMRipChapterStore      OGMRipChapterStore;
typedef struct _OGMRipChapterStoreClass OGMRipChapterStoreClass;
typedef struct _OGMRipChapterStorePriv  OGMRipChapterStorePriv;

struct _OGMRipChapterStore
{
  GtkListStore parent_instance;

  OGMRipChapterStorePriv *priv;
};

struct _OGMRipChapterStoreClass
{
  GtkListStoreClass parent_class;

  void (* selection_changed) (OGMRipChapterStore *store);
};

GType                ogmrip_chapter_store_get_type      (void);
OGMRipChapterStore * ogmrip_chapter_store_new           (void);
void                 ogmrip_chapter_store_set_title     (OGMRipChapterStore *store,
                                                         OGMRipTitle        *title);
OGMRipTitle *        ogmrip_chapter_store_get_title     (OGMRipChapterStore *store);
gchar *              ogmrip_chapter_store_get_label     (OGMRipChapterStore *store,
                                                         guint              chapter);
void                 ogmrip_chapter_store_set_label     (OGMRipChapterStore *store,
                                                         guint              chapter,
                                                         const gchar        *label);
gboolean             ogmrip_chapter_store_get_selected  (OGMRipChapterStore *store,
                                                         GtkTreeIter        *iter);
void                 ogmrip_chapter_store_set_selected  (OGMRipChapterStore *store,
                                                         GtkTreeIter        *iter,
                                                         gboolean           selected);
void                 ogmrip_chapter_store_select_all    (OGMRipChapterStore *store);
void                 ogmrip_chapter_store_deselect_all  (OGMRipChapterStore *store);
gboolean             ogmrip_chapter_store_get_selection (OGMRipChapterStore *store,
                                                         guint              *start_chapter,
                                                         gint               *end_chapter);
gboolean             ogmrip_chapter_store_get_editable  (OGMRipChapterStore *store);

G_END_DECLS

#endif /* __OGMRIP_CHAPTER_STORE_H__ */

