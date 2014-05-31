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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-chapter-view.h"

#include <glib/gi18n-lib.h>

#define OGMRIP_CHAPTER_VIEW_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_CHAPTER_VIEW, OGMRipChapterViewPriv))

struct _OGMRipChapterViewPriv
{
  OGMRipChapterStore *store;
};

static void ogmrip_chapter_view_dispose (GObject *gobject);

G_DEFINE_TYPE (OGMRipChapterView, ogmrip_chapter_view, GTK_TYPE_TREE_VIEW)

static void
ogmrip_chapter_view_class_init (OGMRipChapterViewClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) klass;
  gobject_class->dispose = ogmrip_chapter_view_dispose;

  g_type_class_add_private (klass, sizeof (OGMRipChapterViewPriv));
}

static void
ogmrip_chapter_view_selection_toggled (OGMRipChapterView *view, const gchar *path)
{
  if (ogmrip_chapter_store_get_editable (view->priv->store))
  {
    GtkTreeModel *model = GTK_TREE_MODEL (view->priv->store);
    GtkTreeIter iter, next_iter, prev_iter;
    gboolean extract, next_extract, prev_extract;

    next_extract = prev_extract = FALSE;

    gtk_tree_model_get_iter_from_string (model, &iter, path);

    next_iter = iter;
    if (gtk_tree_model_iter_next (model, &next_iter))
      next_extract = ogmrip_chapter_store_get_selected (view->priv->store, &next_iter);

    prev_iter = iter;
    if (gtk_tree_model_iter_previous (model, &prev_iter))
      prev_extract = ogmrip_chapter_store_get_selected (view->priv->store, &prev_iter);

    extract = ogmrip_chapter_store_get_selected (view->priv->store, &iter);

    if ((prev_extract && next_extract) ||
        (!prev_extract && !next_extract && !extract))
      ogmrip_chapter_store_deselect_all (view->priv->store);

    extract = ogmrip_chapter_store_get_selected (view->priv->store, &iter);
    ogmrip_chapter_store_set_selected (view->priv->store, &iter, !extract);
  }
}

static void
ogmrip_chapter_view_label_edited (OGMRipChapterView *view, const gchar *path, const gchar *text)
{
  if (ogmrip_chapter_store_get_editable (view->priv->store))
  {
    GtkTreeIter iter;

    gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (view->priv->store), &iter, path);
    gtk_list_store_set (GTK_LIST_STORE (view->priv->store), &iter,
        OGMRIP_CHAPTER_STORE_LABEL_COLUMN, text,
        -1);
  }
}

static void
ogmrip_chapter_view_init (OGMRipChapterView *view)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  view->priv = OGMRIP_CHAPTER_VIEW_GET_PRIVATE (view);

  view->priv->store = ogmrip_chapter_store_new ();
  gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (view->priv->store));

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Extract?"),
      renderer, "active", OGMRIP_CHAPTER_STORE_SELECTED_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
  g_object_set (renderer, "activatable", TRUE, NULL);

  g_signal_connect_swapped (renderer, "toggled",
      G_CALLBACK (ogmrip_chapter_view_selection_toggled), view);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Chapter"),
      renderer, "text", OGMRIP_CHAPTER_STORE_CHAPTER_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Duration"),
      renderer, "text", OGMRIP_CHAPTER_STORE_DURATION_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Label"),
      renderer, "text", OGMRIP_CHAPTER_STORE_LABEL_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
  gtk_tree_view_column_set_resizable (column, TRUE);
  g_object_set (renderer, "editable", TRUE, NULL);

  g_signal_connect_swapped (renderer, "edited",
      G_CALLBACK (ogmrip_chapter_view_label_edited), view);
}

static void
ogmrip_chapter_view_dispose (GObject *gobject)
{
  OGMRipChapterView *view = OGMRIP_CHAPTER_VIEW (gobject);

  if (view->priv->store)
  {
    g_object_unref (view->priv->store);
    view->priv->store = NULL;
  }

  (*G_OBJECT_CLASS (ogmrip_chapter_view_parent_class)->dispose) (gobject);
}

/**
 * ogmrip_chapter_view_new:
 *
 * Creates a new empty #OGMRipChapterView.
 *
 * Returns: The new #OGMRipChapterView
 */
GtkWidget *
ogmrip_chapter_view_new (void)
{
  return g_object_new (OGMRIP_TYPE_CHAPTER_VIEW, NULL);
}

