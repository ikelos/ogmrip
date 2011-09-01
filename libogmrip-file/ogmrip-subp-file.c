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

#include "ogmrip-subp-file.h"
#include "ogmrip-media-info.h"
#include "ogmrip-file-priv.h"

#include <stdlib.h>

static void ogmrip_subp_iface_init (OGMRipSubpStreamInterface *iface);
static GObject * ogmrip_subp_file_constructor  (GType type,
                                                guint n_properties,
                                                GObjectConstructParam *properties);

G_DEFINE_TYPE_WITH_CODE (OGMRipSubpFile, ogmrip_subp_file, OGMRIP_TYPE_FILE,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_SUBP_STREAM, ogmrip_subp_iface_init));

static void
ogmrip_subp_file_init (OGMRipSubpFile *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream, OGMRIP_TYPE_SUBP_FILE, OGMRipSubpFilePriv);
}

static void
ogmrip_subp_file_class_init (OGMRipSubpFileClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructor = ogmrip_subp_file_constructor;

  g_type_class_add_private (klass, sizeof (OGMRipSubpFilePriv));
}

static GObject *
ogmrip_subp_file_constructor (GType type, guint n_properties, GObjectConstructParam *properties)
{
  GObject *gobject;
  OGMRipMediaInfo *info;
  const gchar *str;

  gobject = G_OBJECT_CLASS (ogmrip_subp_file_parent_class)->constructor (type, n_properties, properties);

  info = ogmrip_media_info_get_default ();

  if (!info || !OGMRIP_FILE (gobject)->priv->path ||
      !ogmrip_media_info_open (info, OGMRIP_FILE (gobject)->priv->path))
  {
    g_object_unref (gobject);
    return NULL;
  }

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_GENERAL, 0, "TextCount");
  if (!str || !g_str_equal (str, "1"))
  {
    g_object_unref (info);
    g_object_unref (gobject);
    return NULL;
  }

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_TEXT, 0, "Duration");
  OGMRIP_FILE (gobject)->priv->length = str ? atoi (str) / 1000. : -1.0;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_TEXT, 0, "StreamSize");
  OGMRIP_FILE (gobject)->priv->size = str ? atoll (str) : -1;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_GENERAL, 0, "Title");
  OGMRIP_FILE (gobject)->priv->label = str ? g_strdup (str) : NULL;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_TEXT, 0, "Language/String2");
  OGMRIP_SUBP_FILE (gobject)->priv->language = str ? (str[0] << 8) | str[1] : 0;

  ogmrip_media_info_close (info);

  return gobject;
}

static const gchar *
ogmrip_subp_file_get_label (OGMRipSubpStream *subp)
{
  return OGMRIP_FILE (subp)->priv->label;;
}

static gint
ogmrip_subp_file_get_language (OGMRipSubpStream *subp)
{
  return OGMRIP_SUBP_FILE (subp)->priv->language;
}

static void
ogmrip_subp_iface_init (OGMRipSubpStreamInterface *iface)
{
  iface->get_label    = ogmrip_subp_file_get_label;
  iface->get_language = ogmrip_subp_file_get_language;
}

OGMRipMedia *
ogmrip_subp_file_new (const gchar *uri)
{
  g_return_val_if_fail (uri != NULL, NULL);

  return g_object_new (OGMRIP_TYPE_SUBP_FILE, "uri", uri, NULL);
}

void
ogmrip_subp_file_set_language (OGMRipSubpFile *file, guint language)
{
  g_return_if_fail (OGMRIP_IS_SUBP_FILE (file));

  file->priv->language = language;
}
