/* OGMRipModule - A module library for OGMRip
 * Copyright (C) 2011 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-type.h"

struct _OGMRipTypeInfoPriv
{
  GType gtype;
  gpointer klass;

  gchar *name;
  gchar *description;

  GArray *extensions;
};

enum
{
  PROP_0,
  PROP_NAME,
  PROP_DESCRIPTION
};

static GHashTable *table;
G_LOCK_DEFINE_STATIC (table);

G_DEFINE_TYPE (OGMRipTypeInfo, ogmrip_type_info, G_TYPE_INITIALLY_UNOWNED);

static void
ogmrip_type_info_dispose (GObject *gobject)
{
  OGMRipTypeInfo *info = OGMRIP_TYPE_INFO (gobject);

  if (info->priv->klass)
  {
    g_type_class_unref (info->priv->klass);
    info->priv->klass = NULL;
  }

  G_OBJECT_CLASS (ogmrip_type_info_parent_class)->dispose (gobject);
}

static void
ogmrip_type_info_finalize (GObject *gobject)
{
  OGMRipTypeInfo *info = OGMRIP_TYPE_INFO (gobject);

  g_free (info->priv->name);
  g_free (info->priv->description);

  g_array_free (info->priv->extensions, TRUE);

  G_OBJECT_CLASS (ogmrip_type_info_parent_class)->finalize (gobject);
}

static void
ogmrip_type_info_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  OGMRipTypeInfo *info = OGMRIP_TYPE_INFO (gobject);

  switch (prop_id)
  {
    case PROP_NAME:
      g_value_set_string (value, info->priv->name);
      break;
    case PROP_DESCRIPTION:
      g_value_set_string (value, info->priv->description);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_type_info_set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipTypeInfo *info = OGMRIP_TYPE_INFO (gobject);

  switch (prop_id)
  {
    case PROP_NAME:
      info->priv->name = g_value_dup_string (value);
      break;
    case PROP_DESCRIPTION:
      info->priv->description = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_type_info_class_init (OGMRipTypeInfoClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_type_info_dispose;
  gobject_class->finalize = ogmrip_type_info_finalize;
  gobject_class->get_property = ogmrip_type_info_get_property;
  gobject_class->set_property = ogmrip_type_info_set_property;

  g_object_class_install_property (gobject_class, PROP_NAME,
      g_param_spec_string ("name", "name", "name", NULL,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DESCRIPTION,
      g_param_spec_string ("description", "description", "description", NULL,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipTypeInfoPriv));
}

static void
ogmrip_type_info_init (OGMRipTypeInfo *info)
{
  info->priv = G_TYPE_INSTANCE_GET_PRIVATE (info, OGMRIP_TYPE_TYPE_INFO, OGMRipTypeInfoPriv);

  info->priv->extensions = g_array_new (FALSE, FALSE, sizeof (GType));
}

void
ogmrip_type_register (GType gtype, OGMRipTypeInfo *info)
{
  g_return_if_fail (gtype == G_TYPE_NONE || g_type_is_a (gtype, G_TYPE_OBJECT));
  g_return_if_fail (OGMRIP_IS_TYPE_INFO (info));

  G_LOCK (table);

  if (!table)
    table = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) g_object_unref);

  info->priv->gtype = gtype;

  g_hash_table_insert (table, GUINT_TO_POINTER (gtype), g_object_ref_sink (info));

  G_UNLOCK (table);
}

OGMRipTypeInfo *
ogmrip_type_info_lookup (GType gtype)
{
  OGMRipTypeInfo *info;

  if (!table)
    return NULL;

  G_LOCK (table);
  info = g_hash_table_lookup (table, GUINT_TO_POINTER (gtype));
  G_UNLOCK (table);

  return info;
}

void
ogmrip_type_add_extension (GType gtype, GType extension)
{
  OGMRipTypeInfo *info;

  info = ogmrip_type_info_lookup (gtype);
  if (info)
    g_array_append_val (info->priv->extensions, extension);
}

GType
ogmrip_type_get_extension (GType gtype, GType iface)
{
  OGMRipTypeInfo *info;
  GType *extensions;
  guint i;

  info = ogmrip_type_info_lookup (gtype);
  if (!info)
    return G_TYPE_NONE;

  extensions = (GType *) info->priv->extensions->data;
  for (i = 0; i < info->priv->extensions->len; i ++)
    if (g_type_is_a (extensions[i], iface))
      return extensions[i];

  return G_TYPE_NONE;
}

const gchar *
ogmrip_type_name (GType gtype)
{
  OGMRipTypeInfo *info;

  if (gtype == G_TYPE_NONE)
    return "none";

  info = ogmrip_type_info_lookup (gtype);
  if (!info)
    return NULL;

  return info->priv->name;
}

const gchar *
ogmrip_type_description (GType gtype)
{
  OGMRipTypeInfo *info;

  info = ogmrip_type_info_lookup (gtype);
  if (!info)
    return NULL;

  return info->priv->description;
}

static gboolean
compare_with_name (gpointer key, OGMRipTypeInfo *info, const gchar *name)
{
  return g_str_equal (info->priv->name, name);
}

GType
ogmrip_type_from_name (const gchar *name)
{
  OGMRipTypeInfo *info;

  g_return_val_if_fail (name != NULL, G_TYPE_NONE);

  if (!table)
    return G_TYPE_NONE;

  G_LOCK (table);
  info = g_hash_table_find (table, (GHRFunc) compare_with_name, (gpointer) name);
  G_UNLOCK (table);

  if (!info)
    return G_TYPE_NONE;

  return info->priv->gtype;
}

GType *
ogmrip_type_children (GType gtype, guint *n)
{
  GArray *types;
  GHashTableIter iter;
  OGMRipTypeInfo *info;

  if (!table)
    return NULL;

  types = g_array_new (FALSE, FALSE, sizeof (GType));

  G_LOCK (table);

  g_hash_table_iter_init (&iter, table);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer*) &info))
    if (g_type_is_a (info->priv->gtype, gtype))
      g_array_append_val (types, info->priv->gtype);

  gtype = G_TYPE_NONE;
  g_array_append_val (types, gtype);

  G_UNLOCK (table);

  if (n)
    *n = types->len;

  return (GType *) g_array_free (types, FALSE);
}

GParamSpec *
ogmrip_type_property (GType gtype, const gchar *property)
{
  OGMRipTypeInfo *info;

  info = ogmrip_type_info_lookup (gtype);
  if (!info)
    return NULL;

  if (!info->priv->klass)
    info->priv->klass = g_type_class_ref (gtype);

  return g_object_class_find_property (info->priv->klass, property);
}

