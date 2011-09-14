/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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
#include "ogmrip-container.h"
#include "ogmrip-video-codec.h"
#include "ogmrip-audio-codec.h"
#include "ogmrip-subp-codec.h"

struct _OGMRipTypeInfoPriv
{
  GType gtype;
  gpointer klass;

  gchar *name;
  gchar *description;

  GArray *extensions;

  GArray *formats;
};

enum
{
  PROP_0,
  PROP_NAME,
  PROP_DESCRIPTION
};

static GHashTable *table;

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

  g_array_free (info->priv->formats, TRUE);
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

  info->priv->formats = g_array_new (FALSE, FALSE, sizeof (OGMRipFormat));
  info->priv->extensions = g_array_new (FALSE, FALSE, sizeof (GType));
}

void
ogmrip_type_register_static (GType gtype, OGMRipTypeInfo *info)
{
  ogmrip_type_register_dynamic (NULL, gtype, info);
}

static void
ogmrip_type_module_unloaded (GTypeModule *module, gpointer gtype)
{
  g_hash_table_remove (table, gtype);

  g_signal_handlers_disconnect_by_func (module, ogmrip_type_module_unloaded, gtype);
}

void
ogmrip_type_register_dynamic (GTypeModule *module, GType gtype, OGMRipTypeInfo *info)
{
  g_return_if_fail (module == NULL || G_IS_TYPE_MODULE (module));
  g_return_if_fail (OGMRIP_IS_TYPE_INFO (info));

  if (!table)
    table = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) g_object_unref);

  info->priv->gtype = gtype;

  g_hash_table_insert (table, GUINT_TO_POINTER (gtype), g_object_ref_sink (info));

  if (module)
    g_signal_connect (module, "unload",
        G_CALLBACK (ogmrip_type_module_unloaded), GUINT_TO_POINTER (gtype));
}

static OGMRipTypeInfo *
ogmrip_type_info_lookup (GType gtype)
{
  if (!table)
    return NULL;

  return g_hash_table_lookup (table, GUINT_TO_POINTER (gtype));
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

  info = g_hash_table_find (table, (GHRFunc) compare_with_name, (gpointer) name);
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

  types = g_array_new (TRUE, FALSE, sizeof (GType));

  g_hash_table_iter_init (&iter, table);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer*) &info))
    if (g_type_is_a (info->priv->gtype, gtype))
      g_array_append_vals (types, &info->priv->gtype, 1);

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

void
ogmrip_type_register_codec (GTypeModule *module, GType gtype, const gchar *name, const gchar *description, OGMRipFormat format)
{
  OGMRipTypeInfo *info;

  g_return_if_fail (gtype == G_TYPE_NONE || g_type_is_a (gtype, OGMRIP_TYPE_CODEC));
  g_return_if_fail (name != NULL);

  info = g_object_new (OGMRIP_TYPE_TYPE_INFO, NULL);
  info->priv->name = g_strdup (name);

  if (description)
    info->priv->description = g_strdup (description);

  g_array_append_val (info->priv->formats, format);

  ogmrip_type_register_static (gtype, info);
}

OGMRipFormat
ogmrip_codec_format (GType gtype)
{
  OGMRipTypeInfo *info;
  OGMRipFormat *formats;

  g_return_val_if_fail (g_type_is_a (gtype, OGMRIP_TYPE_CODEC), OGMRIP_FORMAT_UNDEFINED);

  info = ogmrip_type_info_lookup (gtype);
  if (!info)
    return OGMRIP_FORMAT_UNDEFINED;

  formats = (OGMRipFormat *) info->priv->formats->data;

  return formats[0];
}

void
ogmrip_type_register_container (GTypeModule *module, GType gtype, const gchar *name, const gchar *description, OGMRipFormat *formats)
{
  OGMRipTypeInfo *info;
  guint len;

  g_return_if_fail (g_type_is_a (gtype, OGMRIP_TYPE_CONTAINER));
  g_return_if_fail (name != NULL);
  g_return_if_fail (formats != NULL);

  len = 0;
  while (formats[len] > OGMRIP_FORMAT_UNDEFINED)
    len ++;

  info = g_object_new (OGMRIP_TYPE_TYPE_INFO, NULL);
  info->priv->name = g_strdup (name);

  if (description)
    info->priv->description = g_strdup (description);

  g_array_append_vals (info->priv->formats, formats, len);

  ogmrip_type_register_static (gtype, info);
}

gboolean
ogmrip_container_contains (GType gtype, OGMRipFormat format)
{
  OGMRipTypeInfo *info;
  OGMRipFormat *formats;
  guint i;

  g_return_val_if_fail (g_type_is_a (gtype, OGMRIP_TYPE_CONTAINER), FALSE);
  g_return_val_if_fail (format != OGMRIP_FORMAT_UNDEFINED, FALSE);

  info = ogmrip_type_info_lookup (gtype);
  if (!info)
    return OGMRIP_FORMAT_UNDEFINED;

  formats = (OGMRipFormat *) info->priv->formats->data;
  for (i = 0; i < info->priv->formats->len; i ++)
    if (formats[i] == format)
      return TRUE;

  return FALSE;
}

