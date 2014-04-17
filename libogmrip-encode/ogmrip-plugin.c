/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-container.h"
#include "ogmrip-video-codec.h"
#include "ogmrip-audio-codec.h"
#include "ogmrip-subp-codec.h"

#include <ogmrip-base.h>
#include <ogmrip-media.h>

#define OGMRIP_TYPE_PLUGIN_INFO            (ogmrip_plugin_info_get_type ())
#define OGMRIP_PLUGIN_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_PLUGIN_INFO, OGMRipPluginInfo))
#define OGMRIP_PLUGIN_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_PLUGIN_INFO, OGMRipPluginInfoClass))
#define OGMRIP_IS_PLUGIN_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_PLUGIN_INFO))
#define OGMRIP_IS_PLUGIN_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_PLUGIN_INFO))

typedef struct _OGMRipPluginInfo      OGMRipPluginInfo;
typedef struct _OGMRipPluginInfoClass OGMRipPluginInfoClass;
typedef struct _OGMRipPluginInfoPriv  OGMRipPluginInfoPriv;

struct _OGMRipPluginInfo
{
  OGMRipTypeInfo parent_instance;

  GArray *formats;
  gchar *schema_id;
  gchar *schema_name;
};

struct _OGMRipPluginInfoClass
{
  OGMRipTypeInfoClass parent_class;
};

enum
{
  PROP_0,
  PROP_SCHEMA_ID,
  PROP_SCHEMA_NAME
};

G_DEFINE_TYPE (OGMRipPluginInfo, ogmrip_plugin_info, OGMRIP_TYPE_TYPE_INFO);

static void
ogmrip_plugin_info_finalize (GObject *gobject)
{
  OGMRipPluginInfo *info = OGMRIP_PLUGIN_INFO (gobject);

  g_array_free (info->formats, TRUE);

  g_free (info->schema_id);
  g_free (info->schema_name);

  G_OBJECT_CLASS (ogmrip_plugin_info_parent_class)->finalize (gobject);
}

static void
ogmrip_plugin_info_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  OGMRipPluginInfo *info = OGMRIP_PLUGIN_INFO (gobject);

  switch (prop_id)
  {
    case PROP_SCHEMA_ID:
      g_value_set_string (value, info->schema_id);
      break;
    case PROP_SCHEMA_NAME:
      if (!info->schema_name && info->schema_id)
        g_object_get (info, "name", &info->schema_name, NULL);
      g_value_set_string (value, info->schema_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_plugin_info_set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipPluginInfo *info = OGMRIP_PLUGIN_INFO (gobject);

  switch (prop_id)
  {
    case PROP_SCHEMA_ID:
      info->schema_id = g_value_dup_string (value);
      break;
    case PROP_SCHEMA_NAME:
      info->schema_name = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_plugin_info_class_init (OGMRipPluginInfoClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = ogmrip_plugin_info_finalize;
  gobject_class->get_property = ogmrip_plugin_info_get_property;
  gobject_class->set_property = ogmrip_plugin_info_set_property;

  g_object_class_install_property (gobject_class, PROP_SCHEMA_ID,
      g_param_spec_string ("schema-id", "schema-id", "schema-id", NULL,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SCHEMA_NAME,
      g_param_spec_string ("schema-name", "schema-name", "schema-name", NULL,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_plugin_info_init (OGMRipPluginInfo *info)
{
  info->formats = g_array_new (FALSE, FALSE, sizeof (OGMRipFormat));
}

static OGMRipPluginInfo *
ogmrip_plugin_info_new (const gchar *name, const gchar *description, const gchar *property, va_list args)
{
  OGMRipPluginInfo *info;

  info = g_object_new (OGMRIP_TYPE_PLUGIN_INFO, "name", name, "description", description, NULL);

  while (property)
  {
    const gchar *str;

    if (g_str_equal (property, "schema-id"))
    {
      str = va_arg (args, gchar *);
      if (!str)
        break;
      info->schema_id = g_strdup (str);
    }
    else if (g_str_equal (property, "schema-name"))
    {
      str = va_arg (args, gchar *);
      if (!str)
        break;
      info->schema_name = g_strdup (str);
    }
    else
    {
      g_warning ("%s: object class `%s' has no property named `%s'",
          G_STRFUNC, g_type_name (OGMRIP_TYPE_PLUGIN_INFO), property);
      break;
    }

    property = va_arg (args, gchar *);
  }

  return info;
}

static gboolean
ogmrip_plugin_info_get_schema (OGMRipPluginInfo *info, const gchar **id, const gchar **name)
{
  g_return_val_if_fail (OGMRIP_IS_PLUGIN_INFO (info), FALSE);

  if (id)
    *id = info->schema_id;

  if (name)
  {
    if (!info->schema_name && info->schema_id)
      g_object_get (info, "name", &info->schema_name, NULL);

    *name = info->schema_name;
  }

  return info->schema_id != NULL && info->schema_name != NULL;
}

void
ogmrip_register_codec (GType gtype, const gchar *name, const gchar *description, OGMRipFormat format, const gchar *property, ...)
{
  OGMRipPluginInfo *info;
  va_list args;

  g_return_if_fail (g_type_is_a (gtype, OGMRIP_TYPE_CODEC));
  g_return_if_fail (name != NULL);

  va_start (args, property);
  info = ogmrip_plugin_info_new (name, description, property, args);
  va_end (args);

  g_array_append_val (info->formats, format);

  ogmrip_type_register (gtype, OGMRIP_TYPE_INFO (info));
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

  formats = (OGMRipFormat *) OGMRIP_PLUGIN_INFO (info)->formats->data;

  return formats[0];
}

gboolean
ogmrip_codec_get_schema (GType gtype, const gchar **id, const gchar **name)
{
  OGMRipPluginInfo *info;

  g_return_val_if_fail (g_type_is_a (gtype, OGMRIP_TYPE_CODEC), FALSE);

  info = (OGMRipPluginInfo *) ogmrip_type_info_lookup (gtype);

  return ogmrip_plugin_info_get_schema (info, id, name);
}

void
ogmrip_register_container (GType gtype, const gchar *name, const gchar *description, OGMRipFormat *formats, const gchar *property, ...)
{
  OGMRipPluginInfo *info;
  va_list args;
  guint len;

  g_return_if_fail (g_type_is_a (gtype, OGMRIP_TYPE_CONTAINER));
  g_return_if_fail (name != NULL);
  g_return_if_fail (formats != NULL);

  len = 0;
  while (formats[len] > OGMRIP_FORMAT_UNDEFINED)
    len ++;

  va_start (args, property);
  info = ogmrip_plugin_info_new (name, description, property, args);
  va_end (args);

  g_array_append_vals (info->formats, formats, len);

  ogmrip_type_register (gtype, OGMRIP_TYPE_INFO (info));
}

gboolean
ogmrip_container_contains (GType gtype, OGMRipFormat format)
{
  OGMRipTypeInfo *info;
  OGMRipFormat *formats;
  guint i;

  g_return_val_if_fail (g_type_is_a (gtype, OGMRIP_TYPE_CONTAINER), FALSE);

  info = ogmrip_type_info_lookup (gtype);
  if (!info)
    return FALSE;

  formats = (OGMRipFormat *) OGMRIP_PLUGIN_INFO (info)->formats->data;
  for (i = 0; i < OGMRIP_PLUGIN_INFO (info)->formats->len; i ++)
    if (formats[i] == format)
      return TRUE;

  return FALSE;
}

gboolean
ogmrip_container_get_schema (GType gtype, const gchar **id, const gchar **name)
{
  OGMRipPluginInfo *info;

  g_return_val_if_fail (g_type_is_a (gtype, OGMRIP_TYPE_CONTAINER), FALSE);

  info = (OGMRipPluginInfo *) ogmrip_type_info_lookup (gtype);

  return ogmrip_plugin_info_get_schema (info, id, name);
}

