/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-language-store.h"

#include <ogmrip-media.h>

extern const gchar *ogmdvd_languages[][3];
extern const guint  ogmdvd_nlanguages;

static const GType column_types[] =
{
  G_TYPE_STRING, /* name */
  G_TYPE_UINT    /* code */
};

G_STATIC_ASSERT (G_N_ELEMENTS (column_types) == OGMRIP_LANGUAGE_STORE_N_COLUMNS);

static gint
ogmrip_language_store_name_sort_func (OGMRipLanguageStore *store, GtkTreeIter *iter1, GtkTreeIter *iter2, gpointer user_data)
{
  gchar *name1, *name2;
  gint retval;

  gtk_tree_model_get (GTK_TREE_MODEL (store), iter1, OGMRIP_LANGUAGE_STORE_NAME_COLUMN, &name1, -1);
  gtk_tree_model_get (GTK_TREE_MODEL (store), iter2, OGMRIP_LANGUAGE_STORE_NAME_COLUMN, &name2, -1);

  retval = g_utf8_collate (name1, name2);

  g_free (name1);
  g_free (name2);

  return retval;
}

G_DEFINE_TYPE (OGMRipLanguageStore, ogmrip_language_store, GTK_TYPE_LIST_STORE);

static void
ogmrip_language_store_class_init (OGMRipLanguageStoreClass *klass)
{
}

static void
ogmrip_language_store_init (OGMRipLanguageStore *store)
{
  GtkTreeIter iter;
  const gchar *lang;
  guint i;

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
      OGMRIP_LANGUAGE_STORE_N_COLUMNS, (GType *) column_types);

  gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (store),
      (GtkTreeIterCompareFunc) ogmrip_language_store_name_sort_func, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
      OGMRIP_LANGUAGE_STORE_NAME_COLUMN, GTK_SORT_ASCENDING);

  for (i = 2; i < ogmdvd_nlanguages; i ++)
  {
    lang = ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_1];

    gtk_list_store_append (GTK_LIST_STORE (store), &iter);
    gtk_list_store_set (GTK_LIST_STORE (store), &iter,
        OGMRIP_LANGUAGE_STORE_NAME_COLUMN, ogmdvd_languages[i][OGMRIP_LANGUAGE_NAME],
        OGMRIP_LANGUAGE_STORE_CODE_COLUMN, (lang[0] << 8) | lang[1], -1);
  }
}

OGMRipLanguageStore *
ogmrip_language_store_new (void)
{
  return g_object_new (OGMRIP_TYPE_LANGUAGE_STORE, NULL);
}

