/* OGMRip - A wrapper library around libdvdread
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

#include "ogmrip-chapter-store.h"

#include <glib/gi18n-lib.h>

struct _OGMRipChapterStorePriv
{
  OGMRipTitle *title;
};

enum
{
  PROP_0,
  PROP_TITLE
};

static const GType column_types[] =
{
  G_TYPE_UINT,   /* chapter */
  G_TYPE_STRING, /* label */
  G_TYPE_STRING, /* length */
  G_TYPE_BOOLEAN /* selected */
};

G_STATIC_ASSERT (G_N_ELEMENTS (column_types) == OGMRIP_CHAPTER_STORE_N_COLUMNS);

static void ogmrip_chapter_store_dispose      (GObject      *gobject);
static void ogmrip_chapter_store_get_property (GObject      *gobject,
                                               guint        property_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);
static void ogmrip_chapter_store_set_property (GObject      *gobject,
                                               guint        property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);

G_DEFINE_TYPE (OGMRipChapterStore, ogmrip_chapter_store, GTK_TYPE_LIST_STORE);

static void
ogmrip_chapter_store_class_init (OGMRipChapterStoreClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_chapter_store_dispose;
  gobject_class->get_property = ogmrip_chapter_store_get_property;
  gobject_class->set_property = ogmrip_chapter_store_set_property;

  g_object_class_install_property (gobject_class, PROP_TITLE,
      g_param_spec_object ("title", "Title property", "The DVD title",
        OGMRIP_TYPE_TITLE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipChapterStorePriv));
}

static void
ogmrip_chapter_store_init (OGMRipChapterStore *store)
{
  store->priv = G_TYPE_INSTANCE_GET_PRIVATE (store, OGMRIP_TYPE_CHAPTER_STORE, OGMRipChapterStorePriv);

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
      OGMRIP_CHAPTER_STORE_N_COLUMNS, (GType *) column_types);
}

static void
ogmrip_chapter_store_dispose (GObject *gobject)
{
  OGMRipChapterStore *store = OGMRIP_CHAPTER_STORE (gobject);

  if (store->priv->title)
  {
    g_object_unref (store->priv->title);
    store->priv->title = NULL;
  }

  G_OBJECT_CLASS (ogmrip_chapter_store_parent_class)->dispose (gobject);
}

static void
ogmrip_chapter_store_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_TITLE:
      g_value_set_object (value, OGMRIP_CHAPTER_STORE (gobject)->priv->title);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_chapter_store_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_TITLE:
      ogmrip_chapter_store_set_title (OGMRIP_CHAPTER_STORE (gobject), g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

OGMRipChapterStore *
ogmrip_chapter_store_new (OGMRipTitle *title)
{
  return g_object_new (OGMRIP_TYPE_CHAPTER_STORE, "title", title, NULL);
}

/**
 * ogmdvd_chapter_store_get_title:
 * @store: An #OGMRipChapterStore
 *
 * Returns the #OGMRipTitle which was passed to ogmrip_chapter_store_set_title().
 *
 * Returns: An #OGMRipTitle
 */
OGMRipTitle *
ogmrip_chapter_list_get_title (OGMRipChapterStore *store)
{
  g_return_val_if_fail (OGMRIP_IS_CHAPTER_STORE (store), NULL);

  return store->priv->title;
}

/**
 * ogmrip_chapter_store_set_title:
 * @store: An #OGMRipChapterStore
 * @title: An #OGMRipTitle
 *
 * Adds to the list the chapters of the given #OGMRipTitle.
 */
void
ogmrip_chapter_list_set_title (OGMRipChapterStore *store, OGMRipTitle *title)
{
  GtkTreeIter iter;
  OGMRipTime time_;

  gint chap, nchap;
  gdouble seconds;
  gchar *str;

  g_return_if_fail (OGMRIP_IS_CHAPTER_STORE (store));
  g_return_if_fail (title == NULL || OGMRIP_IS_TITLE (title));

  if (title)
    g_object_ref (title);
  if (store->priv->title)
    g_object_unref (store->priv->title);
  store->priv->title = title;

  gtk_list_store_clear (GTK_LIST_STORE (store));

  if (title)
  {
    nchap = ogmrip_title_get_n_chapters (title);
    for (chap = 0; chap < nchap; chap++)
    {
      gtk_list_store_append (GTK_LIST_STORE (store), &iter);

      str = g_strdup_printf ("%s %02d", _("Chapter"), chap + 1);
      gtk_list_store_set (GTK_LIST_STORE (store), &iter,
          OGMRIP_CHAPTER_STORE_CHAPTER_COLUMN, chap + 1,
          OGMRIP_CHAPTER_STORE_LABEL_COLUMN, str,
          -1);
      g_free (str);

      if ((seconds = ogmrip_title_get_chapters_length (title, chap, chap, &time_)) > 0)
      {
        str = g_strdup_printf ("%02lu:%02lu:%02lu", time_.hour, time_.min, time_.sec);
        gtk_list_store_set (GTK_LIST_STORE (store), &iter,
            OGMRIP_CHAPTER_STORE_LENGTH_COLUMN, str,
            -1);
        g_free (str);
      }
    }
  }

  g_object_notify (G_OBJECT (store), "title");
}

/**
 * ogmrip_chapter_store_get_label:
 * @store: An #OGMRipChapterStore
 * @chapter: A chapter number
 *
 * Returns the label of the given chapter.
 *
 * Returns: The chapter's label
 */
gchar *
ogmrip_chapter_list_get_label (OGMRipChapterStore *store, guint chapter)
{
  GtkTreeIter iter;
  gchar *label;

  g_return_val_if_fail (OGMRIP_IS_CHAPTER_STORE (store), NULL);

  if (!gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store), &iter, NULL, chapter))
    return NULL;

  gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
      OGMRIP_CHAPTER_STORE_LABEL_COLUMN, &label,
      -1);

  return label;
}

/**
 * ogmrip_chapter_store_set_label:
 * @store: An #OGMDvdChapterStore
 * @chapter: A chapter number
 * @label: The label of the chapter
 *
 * Sets the label of the given chapter.
 */
void
ogmrip_chapter_list_set_label (OGMRipChapterStore *store, guint chapter, const gchar *label)
{
  GtkTreeIter iter;

  g_return_if_fail (OGMRIP_IS_CHAPTER_STORE (store));

  if (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store), &iter, NULL, chapter))
    gtk_list_store_set (GTK_LIST_STORE (store), &iter,
        OGMRIP_CHAPTER_STORE_LABEL_COLUMN, label,
        -1);
}

