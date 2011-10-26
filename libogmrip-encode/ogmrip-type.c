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
};

struct _OGMRipPluginInfoClass
{
  OGMRipTypeInfoClass parent_class;
};

G_DEFINE_TYPE (OGMRipPluginInfo, ogmrip_plugin_info, OGMRIP_TYPE_TYPE_INFO);

static void
ogmrip_plugin_info_finalize (GObject *gobject)
{
  OGMRipPluginInfo *info = OGMRIP_PLUGIN_INFO (gobject);

  g_array_free (info->formats, TRUE);

  G_OBJECT_CLASS (ogmrip_plugin_info_parent_class)->finalize (gobject);
}

static void
ogmrip_plugin_info_class_init (OGMRipPluginInfoClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = ogmrip_plugin_info_finalize;
}

static void
ogmrip_plugin_info_init (OGMRipPluginInfo *info)
{
  info->formats = g_array_new (FALSE, FALSE, sizeof (OGMRipFormat));
}

void
ogmrip_register_codec (GType gtype, const gchar *name, const gchar *description, OGMRipFormat format)
{
  OGMRipPluginInfo *info;

  g_return_if_fail (gtype == G_TYPE_NONE || g_type_is_a (gtype, OGMRIP_TYPE_CODEC));
  g_return_if_fail (name != NULL);

  info = g_object_new (OGMRIP_TYPE_PLUGIN_INFO, "name", name, "description", description, NULL);

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

void
ogmrip_register_container (GType gtype, const gchar *name, const gchar *description, OGMRipFormat *formats)
{
  OGMRipPluginInfo *info;
  guint len;

  g_return_if_fail (g_type_is_a (gtype, OGMRIP_TYPE_CONTAINER));
  g_return_if_fail (name != NULL);
  g_return_if_fail (formats != NULL);

  len = 0;
  while (formats[len] > OGMRIP_FORMAT_UNDEFINED)
    len ++;

  info = g_object_new (OGMRIP_TYPE_PLUGIN_INFO, "name", name, "description", description, NULL);

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

  if (format == OGMRIP_FORMAT_UNDEFINED)
    return TRUE;

  formats = (OGMRipFormat *) OGMRIP_PLUGIN_INFO (info)->formats->data;
  for (i = 0; i < OGMRIP_PLUGIN_INFO (info)->formats->len; i ++)
    if (formats[i] == format)
      return TRUE;

  return FALSE;
}

