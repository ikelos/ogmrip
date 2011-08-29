/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * SECTION:ogmrip-chapter-list
 * @title: OGMRipChapterList
 * @include: ogmrip-chapter-list.h
 * @short_description: A widget that lists the chapters of a DVD title
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-chapter-list.h"
#include "ogmrip-helper.h"

#include <glib/gi18n-lib.h>

enum
{
  SELECTION_CHANGED,
  LAST_SIGNAL
};

enum
{
  COL_CHAPTER,
  COL_LABEL,
  COL_LENGTH,
  COL_FRAMES,
  COL_EXTRACT,
  COL_LAST
};

static int signals[LAST_SIGNAL] = { 0 };

#if !GTK_CHECK_VERSION(3, 0, 0)
static gboolean
gtk_tree_model_iter_previous (GtkTreeModel *model, GtkTreeIter  *iter)
{
  GtkTreePath *path;
  gboolean retval;

  path = gtk_tree_model_get_path (model, iter);
  if (!path)
    return FALSE;

  retval = gtk_tree_path_prev (path) &&
    gtk_tree_model_get_iter (model, iter, path);
  if (!retval)
    iter->stamp = 0;

  gtk_tree_path_free (path);

  return retval;
}
#endif

G_DEFINE_TYPE (OGMRipChapterList, ogmrip_chapter_list, OGMDVD_TYPE_CHAPTER_LIST)

static void
ogmrip_chapter_list_class_init (OGMRipChapterListClass *klass)
{
  /**
   * OGMRipChapterList::selection-changed
   * @list: the widget that received the signal
   *
   * Emitted each time the selection of chapters changes
   */
  signals[SELECTION_CHANGED] = g_signal_new ("selection-changed", G_TYPE_FROM_CLASS (klass), 
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipChapterListClass, selection_changed), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
ogmrip_chapter_extract_toggled (OGMRipChapterList *list, gchar *path)
{
  GtkTreeModel *model;
  GtkTreeIter iter, next_iter, prev_iter;
  gboolean extract, next_extract, prev_extract;

  next_extract = prev_extract = FALSE;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));

  gtk_tree_model_get_iter_from_string (model, &iter, path);

  next_iter = iter;
  if (gtk_tree_model_iter_next (model, &next_iter))
    gtk_tree_model_get (model, &next_iter, COL_EXTRACT, &next_extract, -1);

  prev_iter = iter;
  if (gtk_tree_model_iter_previous (model, &prev_iter))
    gtk_tree_model_get (model, &prev_iter, COL_EXTRACT, &prev_extract, -1);

  gtk_tree_model_get (model, &iter, COL_EXTRACT, &extract, -1);

  if ((prev_extract && next_extract) || (!prev_extract && !next_extract && !extract))
    ogmrip_chapter_list_deselect_all (list);

  gtk_tree_model_get (model, &iter, COL_EXTRACT, &extract, -1);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_EXTRACT, extract ? FALSE : TRUE, -1);

  g_signal_emit (G_OBJECT (list), signals[SELECTION_CHANGED], 0);
}

static void
ogmrip_chapter_list_init (OGMRipChapterList *list)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Extract?"), renderer, "active", COL_EXTRACT, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (list), column, 0);
  g_object_set (renderer, "activatable", TRUE, NULL);

  g_signal_connect_swapped (renderer, "toggled", G_CALLBACK (ogmrip_chapter_extract_toggled), list);
}

/**
 * ogmrip_chapter_list_new:
 *
 * Creates a new #OGMRipChapterList.
 *
 * Returns: The new #OGMRipChapterList
 */
GtkWidget *
ogmrip_chapter_list_new (void)
{
  OGMRipChapterList *list;
  GtkListStore *store;

  list = g_object_new (OGMRIP_TYPE_CHAPTER_LIST, NULL);

  store = gtk_list_store_new (COL_LAST, 
      G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_ULONG, G_TYPE_BOOLEAN);
  gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (store));
  g_object_unref (store);

  return GTK_WIDGET (list);
}

/**
 * ogmrip_chapter_list_select_all:
 * @list: An #OGMRipChapterList
 *
 * Select all the chapters of the list.
 */
void
ogmrip_chapter_list_select_all (OGMRipChapterList *list)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  g_return_if_fail (OGMRIP_IS_CHAPTER_LIST (list));

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));
  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    do
    {
      gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_EXTRACT, TRUE, -1);
    }
    while (gtk_tree_model_iter_next (model, &iter));
  }

  g_signal_emit (G_OBJECT (list), signals[SELECTION_CHANGED], 0);
}

/**
 * ogmrip_chapter_list_deselect_all:
 * @list: An #OGMRipChapterList
 *
 * Deselects all the chapters of the list.
 */
void
ogmrip_chapter_list_deselect_all (OGMRipChapterList *list)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  g_return_if_fail (OGMRIP_IS_CHAPTER_LIST (list));

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));
  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    do
    {
      gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_EXTRACT, FALSE, -1);
    }
    while (gtk_tree_model_iter_next (model, &iter));
  }

  g_signal_emit (G_OBJECT (list), signals[SELECTION_CHANGED], 0);
}

/**
 * ogmrip_chapter_list_get_selected:
 * @list: An #OGMRipChapterList
 * @start_chapter: The first selected chapter
 * @end_chapter: The last selected chapter
 *
 * Gets the range of the selected chapters.
 *
 * Returns: %TRUE, if @start_chapter and @end_chapter were set
 */
gboolean
ogmrip_chapter_list_get_selected (OGMRipChapterList *list, guint *start_chapter, gint *end_chapter)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gboolean extract, valid;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));

  valid = gtk_tree_model_get_iter_first (model, &iter);

  extract = *start_chapter = 0;
  while (valid && !extract)
  {
    gtk_tree_model_get (model, &iter, COL_EXTRACT, &extract, -1);
    valid = gtk_tree_model_iter_next (model, &iter);
    if (valid && !extract)
      (*start_chapter) ++;
  }

  *end_chapter = *start_chapter;
  if (!valid && !extract)
    return FALSE;

  while (valid && extract)
  {
    gtk_tree_model_get (model, &iter, COL_EXTRACT, &extract, -1);
    valid = gtk_tree_model_iter_next (model, &iter);
    if (extract)
      (*end_chapter) ++;
  }

  if (extract && !valid)
    *end_chapter = -1;

  return TRUE;
}

