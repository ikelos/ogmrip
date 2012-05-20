/* OGMRip - A library for DVD ripping and encoding
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

#include "ogmrip-type-store.h"

struct _OGMRipTypeStorePriv
{
  GType parent;
};

enum
{
  PROP_0,
  PROP_PARENT
};

static GType column_types[] =
{
  G_TYPE_STRING,  /* description */
  G_TYPE_NONE     /* gtype */
};

G_STATIC_ASSERT (G_N_ELEMENTS (column_types) == OGMRIP_TYPE_STORE_N_COLUMNS);

static void ogmrip_type_store_constructed  (GObject      *gobject);
static void ogmrip_type_store_get_property (GObject      *gobject,
                                            guint        prop_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);
static void ogmrip_type_store_set_property (GObject      *gobject,
                                            guint        prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);

static gint
ogmrip_type_store_description_sort_func (OGMRipTypeStore *store, GtkTreeIter *iter1, GtkTreeIter *iter2, gpointer user_data)
{
  gchar *name1, *name2;
  gint retval;

  gtk_tree_model_get (GTK_TREE_MODEL (store), iter1, OGMRIP_TYPE_STORE_DESCRIPTION_COLUMN, &name1, -1);
  gtk_tree_model_get (GTK_TREE_MODEL (store), iter2, OGMRIP_TYPE_STORE_DESCRIPTION_COLUMN, &name2, -1);

  retval = g_utf8_collate (name1, name2);

  g_free (name1);
  g_free (name2);

  return retval;
}

G_DEFINE_TYPE (OGMRipTypeStore, ogmrip_type_store, GTK_TYPE_LIST_STORE);

static void
ogmrip_type_store_class_init (OGMRipTypeStoreClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_type_store_constructed;
  gobject_class->get_property = ogmrip_type_store_get_property;
  gobject_class->set_property = ogmrip_type_store_set_property;

  g_object_class_install_property (gobject_class, PROP_PARENT,
      g_param_spec_gtype ("parent", "parent", "parent", G_TYPE_OBJECT,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipTypeStorePriv));
}

static void
ogmrip_type_store_init (OGMRipTypeStore *store)
{
  store->priv = G_TYPE_INSTANCE_GET_PRIVATE (store, OGMRIP_TYPE_TYPE_STORE, OGMRipTypeStorePriv);

  column_types[1] = G_TYPE_GTYPE;

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
      OGMRIP_TYPE_STORE_N_COLUMNS, (GType *) column_types);

  gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (store),
      (GtkTreeIterCompareFunc) ogmrip_type_store_description_sort_func, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
      OGMRIP_TYPE_STORE_DESCRIPTION_COLUMN, GTK_SORT_ASCENDING);
}

static void
ogmrip_type_store_constructed (GObject *gobject)
{
  ogmrip_type_store_reload (OGMRIP_TYPE_STORE (gobject));

  G_OBJECT_CLASS (ogmrip_type_store_parent_class)->constructed (gobject);
}

static void
ogmrip_type_store_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  OGMRipTypeStore *store = OGMRIP_TYPE_STORE (gobject);

  switch (prop_id)
  {
    case PROP_PARENT:
      g_value_set_gtype (value, store->priv->parent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_type_store_set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipTypeStore *store = OGMRIP_TYPE_STORE (gobject);

  switch (prop_id)
  {
    case PROP_PARENT:
      store->priv->parent = g_value_get_gtype (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

OGMRipTypeStore *
ogmrip_type_store_new (GType parent)
{
  return g_object_new (OGMRIP_TYPE_TYPE_STORE, "parent", parent, NULL);
}

void
ogmrip_type_store_reload (OGMRipTypeStore *store)
{
  GType *types;

  g_return_if_fail (OGMRIP_IS_TYPE_STORE (store));

  gtk_list_store_clear (GTK_LIST_STORE (store));

  types = ogmrip_type_children (store->priv->parent, NULL);
  if (types)
  {
    GtkTreeIter iter;
    const gchar *desc;
    guint i;

    for (i = 0; types[i] != G_TYPE_NONE; i ++)
    {
      desc = ogmrip_type_description (types[i]);

      gtk_list_store_append (GTK_LIST_STORE (store), &iter);
      gtk_list_store_set (GTK_LIST_STORE (store), &iter,
          OGMRIP_TYPE_STORE_DESCRIPTION_COLUMN, desc,
          OGMRIP_TYPE_STORE_TYPE_COLUMN, types[i],
          -1);
    }
    g_free (types);
  }
}

GType
ogmrip_type_store_get_gtype (OGMRipTypeStore *store, GtkTreeIter *iter)
{
  GValue value = { 0 };
  GType gtype;

  g_return_val_if_fail (OGMRIP_IS_TYPE_STORE (store), G_TYPE_NONE);
  g_return_val_if_fail (iter != NULL, G_TYPE_NONE);

  gtk_tree_model_get_value (GTK_TREE_MODEL (store), iter,
      OGMRIP_TYPE_STORE_TYPE_COLUMN, &value);

  g_return_val_if_fail (G_VALUE_HOLDS_GTYPE (&value), G_TYPE_NONE);

  gtype = g_value_get_gtype (&value);

  g_value_unset (&value);

  return gtype;
}

gboolean
ogmrip_type_store_get_iter (OGMRipTypeStore *store, GtkTreeIter *iter, GType gtype)
{
  g_return_val_if_fail (OGMRIP_IS_TYPE_STORE (store), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), iter))
  {
    GType current_gtype;

    do
    {
      current_gtype = ogmrip_type_store_get_gtype (store, iter);
      if (current_gtype == gtype)
        return TRUE;
    }
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), iter));
  }

  return FALSE;
}

