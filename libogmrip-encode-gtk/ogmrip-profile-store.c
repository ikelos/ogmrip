/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-profile-store.h"
#include "ogmrip-profile-keys.h"

#define OGMRIP_PROFILE_STORE_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_PROFILE_STORE, OGMRipProfileStorePriv))

struct _OGMRipProfileStorePriv
{
  OGMRipProfileEngine *engine;
  gboolean valid_only;
};

enum
{
  PROP_0,
  PROP_ENGINE,
  PROP_VALID_ONLY
};

static const GType column_types[] =
{
  G_TYPE_STRING,  /* name */
  G_TYPE_POINTER  /* profile */
};

G_STATIC_ASSERT (G_N_ELEMENTS (column_types) == OGMRIP_PROFILE_STORE_N_COLUMNS);

static void ogmrip_profile_store_constructed  (GObject      *gobject);
static void ogmrip_profile_store_dispose      (GObject      *gobject);
static void ogmrip_profile_store_get_property (GObject      *gobject,
                                               guint        property_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);
static void ogmrip_profile_store_set_property (GObject      *gobject,
                                               guint        property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);

static void
ogmrip_profile_store_name_changed (OGMRipProfileStore *store, gchar *key, OGMRipProfile *profile)
{
  GtkTreeIter iter;

  if (ogmrip_profile_store_get_iter (store, &iter, profile))
  {
    gchar *name;

    name = g_settings_get_string (G_SETTINGS (profile), key);
    gtk_list_store_set (GTK_LIST_STORE (store), &iter, OGMRIP_PROFILE_STORE_NAME_COLUMN, name, -1);
    g_free (name);
  }
}

static void
ogmrip_profile_store_add (OGMRipProfileStore *store, OGMRipProfile *profile)
{
  GtkTreeIter iter;
  gchar *name;

  name = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME);

  gtk_list_store_append (GTK_LIST_STORE (store), &iter);
  gtk_list_store_set (GTK_LIST_STORE (store), &iter,
      OGMRIP_PROFILE_STORE_NAME_COLUMN,    name,
      OGMRIP_PROFILE_STORE_PROFILE_COLUMN, profile,
      -1);

  g_signal_connect_swapped (profile, "changed::" OGMRIP_PROFILE_NAME,
      G_CALLBACK (ogmrip_profile_store_name_changed), store);
}

static void
ogmrip_profile_store_remove (OGMRipProfileStore *store, OGMRipProfile *profile)
{
  GtkTreeIter iter;

  if (ogmrip_profile_store_get_iter (store, &iter, profile))
  {
    gtk_list_store_remove (GTK_LIST_STORE (store), &iter);
    g_signal_handlers_disconnect_by_func (profile,
        ogmrip_profile_store_name_changed, store);
  }
}

static void
ogmrip_profile_store_clear (OGMRipProfileStore *store)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
  {
    OGMRipProfile *profile;

    do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
          OGMRIP_PROFILE_STORE_PROFILE_COLUMN, &profile, -1);
      if (profile)
        g_signal_handlers_disconnect_by_func (profile,
            ogmrip_profile_store_name_changed, store);
    }
    while (gtk_list_store_remove (GTK_LIST_STORE (store), &iter));
  }
}

static gint
ogmrip_profile_store_name_sort_func (OGMRipProfileStore *store, GtkTreeIter *iter1, GtkTreeIter *iter2, gpointer user_data)
{
  gchar *name1, *name2;
  gint retval;

  gtk_tree_model_get (GTK_TREE_MODEL (store), iter1, OGMRIP_PROFILE_STORE_NAME_COLUMN, &name1, -1);
  gtk_tree_model_get (GTK_TREE_MODEL (store), iter2, OGMRIP_PROFILE_STORE_NAME_COLUMN, &name2, -1);

  retval = g_utf8_collate (name1, name2);

  g_free (name1);
  g_free (name2);

  return retval;
}

G_DEFINE_TYPE (OGMRipProfileStore, ogmrip_profile_store, GTK_TYPE_LIST_STORE);

static void
ogmrip_profile_store_class_init (OGMRipProfileStoreClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_profile_store_constructed;
  gobject_class->dispose = ogmrip_profile_store_dispose;
  gobject_class->get_property = ogmrip_profile_store_get_property;
  gobject_class->set_property = ogmrip_profile_store_set_property;

  g_object_class_install_property (gobject_class, PROP_ENGINE,
      g_param_spec_object ("engine", "engine", "engine", OGMRIP_TYPE_PROFILE_ENGINE,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VALID_ONLY,
      g_param_spec_boolean ("valid-only", "valid-only", "valid-only", FALSE,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipProfileStorePriv));
}

static void
ogmrip_profile_store_init (OGMRipProfileStore *store)
{
  store->priv = OGMRIP_PROFILE_STORE_GET_PRIVATE (store);

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
      OGMRIP_PROFILE_STORE_N_COLUMNS, (GType *) column_types);

  gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (store),
      (GtkTreeIterCompareFunc) ogmrip_profile_store_name_sort_func, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
      OGMRIP_PROFILE_STORE_NAME_COLUMN, GTK_SORT_ASCENDING);
}

static void
ogmrip_profile_store_constructed (GObject *gobject)
{
  OGMRipProfileStore *store = OGMRIP_PROFILE_STORE (gobject);

  if (!store->priv->engine)
  {
    store->priv->engine = ogmrip_profile_engine_get_default ();
    g_object_ref (store->priv->engine);
  }

  g_signal_connect_swapped (store->priv->engine, "add",
      G_CALLBACK (ogmrip_profile_store_add), store);
  g_signal_connect_swapped (store->priv->engine, "remove",
      G_CALLBACK (ogmrip_profile_store_remove), store);

  ogmrip_profile_store_reload (store);

  G_OBJECT_CLASS (ogmrip_profile_store_parent_class)->constructed (gobject);
}

static void
ogmrip_profile_store_dispose (GObject *gobject)
{
  OGMRipProfileStore *store = OGMRIP_PROFILE_STORE (gobject);

  ogmrip_profile_store_clear (store);

  if (store->priv->engine)
  {
    g_signal_handlers_disconnect_by_func (store->priv->engine,
        ogmrip_profile_store_add, store);
    g_signal_handlers_disconnect_by_func (store->priv->engine,
        ogmrip_profile_store_remove, store);

    g_object_unref (store->priv->engine);
    store->priv->engine = NULL;
  }

  G_OBJECT_CLASS (ogmrip_profile_store_parent_class)->dispose (gobject);
}

static void
ogmrip_profile_store_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipProfileStore *store = OGMRIP_PROFILE_STORE (gobject);

  switch (property_id)
  {
    case PROP_ENGINE:
      g_value_set_object (value, store->priv->engine);
      break;
    case PROP_VALID_ONLY:
      g_value_set_boolean (value, store->priv->valid_only);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_profile_store_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipProfileStore *store = OGMRIP_PROFILE_STORE (gobject);

  switch (property_id)
  {
    case PROP_ENGINE:
      store->priv->engine = g_value_dup_object (value);
      break;
    case PROP_VALID_ONLY:
      store->priv->valid_only = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

OGMRipProfileStore *
ogmrip_profile_store_new (OGMRipProfileEngine *engine, gboolean valid_only)
{
  g_return_val_if_fail (engine == NULL || OGMRIP_IS_PROFILE_ENGINE (engine), NULL);

  return g_object_new (OGMRIP_TYPE_PROFILE_STORE, "engine", engine, "valid-only", valid_only, NULL);
}

void
ogmrip_profile_store_reload (OGMRipProfileStore *store)
{
  GSList *list, *link;

  g_return_if_fail (OGMRIP_IS_PROFILE_STORE (store));

  ogmrip_profile_store_clear (store);

  list = ogmrip_profile_engine_get_list (store->priv->engine);
  for (link = list; link; link = link->next)
    if (!store->priv->valid_only || ogmrip_profile_is_valid (link->data))
      ogmrip_profile_store_add (store, link->data);
  g_slist_free (list);
}

OGMRipProfile *
ogmrip_profile_store_get_profile (OGMRipProfileStore *store, GtkTreeIter *iter)
{
  GValue value = { 0 };
  OGMRipProfile *profile;

  g_return_val_if_fail (OGMRIP_IS_PROFILE_STORE (store), NULL);
  g_return_val_if_fail (iter != NULL, NULL);

  gtk_tree_model_get_value (GTK_TREE_MODEL (store), iter,
      OGMRIP_PROFILE_STORE_PROFILE_COLUMN, &value);

  g_return_val_if_fail (G_VALUE_HOLDS_POINTER (&value), NULL);

  profile = g_value_get_pointer (&value);

  g_value_unset (&value);

  return profile;
}

gboolean
ogmrip_profile_store_get_iter (OGMRipProfileStore *store, GtkTreeIter *iter, OGMRipProfile *profile)
{
  g_return_val_if_fail (OGMRIP_IS_PROFILE_STORE (store), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (OGMRIP_IS_PROFILE (profile), FALSE);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), iter))
  {
    OGMRipProfile *current_profile;

    do
    {
      current_profile = ogmrip_profile_store_get_profile (store, iter);
      if (current_profile == profile)
        return TRUE;
    }
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), iter));
  }

  return FALSE;
}

