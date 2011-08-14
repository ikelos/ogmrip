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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/**
 * SECTION:ogmdvd-chapter-list
 * @title: OGMDvdChapterList
 * @include: ogmdvd-chapter-list.h
 * @short_description: A widget that lists the chapters of a DVD title
 */

#include "ogmdvd-chapter-list.h"

#include <glib/gi18n-lib.h>

enum
{
  PROP_0,
  PROP_TITLE
};

enum
{
  COL_CHAPTER,
  COL_LABEL,
  COL_LENGTH,
  COL_SECONDS,
  COL_LAST
};

#define OGMDVD_CHAPTER_LIST_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMDVD_TYPE_CHAPTER_LIST, OGMDvdChapterListPriv))

struct _OGMDvdChapterListPriv
{
  OGMDvdTitle *title;
};

static void ogmdvd_chapter_list_dispose      (GObject      *object);
static void ogmdvd_chapter_list_get_property (GObject      *gobject,
                                              guint        property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);
static void ogmdvd_chapter_list_set_property (GObject      *gobject,
                                              guint        property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);

G_DEFINE_TYPE (OGMDvdChapterList, ogmdvd_chapter_list, GTK_TYPE_TREE_VIEW)

static void
ogmdvd_chapter_list_class_init (OGMDvdChapterListClass *klass)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *) klass;

  object_class->dispose = ogmdvd_chapter_list_dispose;
  object_class->get_property = ogmdvd_chapter_list_get_property;
  object_class->set_property = ogmdvd_chapter_list_set_property;

  g_object_class_install_property (object_class, PROP_TITLE, 
        g_param_spec_pointer ("title", "Title property", "The DVD title",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMDvdChapterListPriv));
}

static void
ogmdvd_chapter_list_label_edited (OGMDvdChapterList *list, gchar *path, gchar *text, GtkCellRenderer *renderer)
{
  GtkTreeIter iter;
  GtkTreeModel *model;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));

  gtk_tree_model_get_iter_from_string (model, &iter, path);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_LABEL, text, -1);
}

static void
ogmdvd_chapter_list_init (OGMDvdChapterList *list)
{
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  list->priv = OGMDVD_CHAPTER_LIST_GET_PRIVATE (list);

  store = gtk_list_store_new (COL_LAST, 
      G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_DOUBLE);
  gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (store));
  g_object_unref (store);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Chapter"), renderer, "text", COL_CHAPTER, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Duration"), renderer, "text", COL_LENGTH, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Label"), renderer, "text", COL_LABEL, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
  gtk_tree_view_column_set_resizable (column, TRUE);
  g_object_set (renderer, "editable", TRUE, NULL);

  g_signal_connect_swapped (renderer, "edited", G_CALLBACK (ogmdvd_chapter_list_label_edited), list);
}

static void
ogmdvd_chapter_list_dispose (GObject *object)
{
  OGMDvdChapterList *list = OGMDVD_CHAPTER_LIST (object);

  if (list->priv->title)
  {
    ogmdvd_title_unref (list->priv->title);
    list->priv->title = NULL;
  }

  (*G_OBJECT_CLASS (ogmdvd_chapter_list_parent_class)->dispose) (object);
}

static void
ogmdvd_chapter_list_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id) 
  {
    case PROP_TITLE:
      g_value_set_pointer (value, OGMDVD_CHAPTER_LIST (gobject)->priv->title);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmdvd_chapter_list_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id) 
  {
    case PROP_TITLE:
      ogmdvd_chapter_list_set_title (OGMDVD_CHAPTER_LIST (gobject), g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

/**
 * ogmdvd_chapter_list_new:
 *
 * Creates a new empty #OGMDvdChapterList.
 *
 * Returns: The new #OGMDvdChapterList
 */
GtkWidget *
ogmdvd_chapter_list_new (void)
{
  return g_object_new (OGMDVD_TYPE_CHAPTER_LIST, NULL);
}

/**
 * ogmdvd_chapter_list_get_title:
 * @list: An #OGMDvdChapterList
 *
 * Returns the #OGMDvdTitle which was passed to ogmdvd_chapter_list_set_title().
 *
 * Returns: An #OGMDvdTitle
 */
OGMDvdTitle *
ogmdvd_chapter_list_get_title (OGMDvdChapterList *list)
{
  g_return_val_if_fail (OGMDVD_IS_CHAPTER_LIST (list), NULL);

  return list->priv->title;
}

/**
 * ogmdvd_chapter_list_set_title:
 * @list: An #OGMDvdChapterList
 * @title: An #OGMDvdTitle
 *
 * Adds to the list the chapters of the given #OGMDvdTitle.
 */
void
ogmdvd_chapter_list_set_title (OGMDvdChapterList *list, OGMDvdTitle *title)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  OGMDvdTime time_;

  gint chap, nchap;
  gdouble seconds;
  gchar *str;

  g_return_if_fail (OGMDVD_IS_CHAPTER_LIST (list));

  if (title)
    ogmdvd_title_ref (title);
  if (list->priv->title)
    ogmdvd_title_unref (list->priv->title);
  list->priv->title = title;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));
  gtk_list_store_clear (GTK_LIST_STORE (model));

  if (title)
  {
    nchap = ogmdvd_title_get_n_chapters (title);
    for (chap = 0; chap < nchap; chap++)
    {
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);

      str = g_strdup_printf ("%s %02d", _("Chapter"), chap + 1);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_CHAPTER, chap + 1, COL_LABEL, str, -1);
      g_free (str);

      if ((seconds = ogmdvd_title_get_chapters_length (title, chap, chap, &time_)) > 0)
      {
        str = g_strdup_printf ("%02d:%02d:%02d", time_.hour, time_.min, time_.sec);
        gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_LENGTH, str, /*COL_SECONDS, seconds,*/ -1);
        g_free (str);
      }
    }
  }

  g_object_notify (G_OBJECT (list), "title");
}

/**
 * ogmdvd_chapter_list_get_label:
 * @list: An #OGMDvdChapterList
 * @chapter: A chapter number
 *
 * Returns the label of the given chapter.
 *
 * Returns: The chapter's label
 */
gchar *
ogmdvd_chapter_list_get_label (OGMDvdChapterList *list, guint chapter)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *label;

  g_return_val_if_fail (OGMDVD_IS_CHAPTER_LIST (list), NULL);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));

  if (!gtk_tree_model_iter_nth_child (model, &iter, NULL, chapter))
    return NULL;

  gtk_tree_model_get (model, &iter, COL_LABEL, &label, -1);

  return label;
}

/**
 * ogmdvd_chapter_list_set_label:
 * @list: An #OGMDvdChapterList
 * @chapter: A chapter number
 * @label: The label of the chapter
 *
 * Sets the label of the given chapter.
 */
void
ogmdvd_chapter_list_set_label (OGMDvdChapterList *list, guint chapter, const gchar *label)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  g_return_if_fail (OGMDVD_IS_CHAPTER_LIST (list));

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));

  if (gtk_tree_model_iter_nth_child (model, &iter, NULL, chapter))
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_LABEL, label, -1);
}

