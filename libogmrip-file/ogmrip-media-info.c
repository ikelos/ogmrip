/* OGMRipFile - A file library for OGMRip
 * Copyright (C) 2010-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-media-info.h"

#include <MediaInfoDLL/MediaInfoDLL.h>

struct _OGMRipMediaInfoPriv
{
  const gchar *filename;
  gboolean is_open;
  gpointer handle;
};

static OGMRipMediaInfo *default_media_info = NULL;

static void ogmrip_media_info_constructed  (GObject *gobject);
static void ogmrip_media_info_finalize     (GObject *gobject);

G_DEFINE_TYPE (OGMRipMediaInfo, ogmrip_media_info, G_TYPE_OBJECT)

static void
ogmrip_media_info_class_init (OGMRipMediaInfoClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_media_info_constructed;
  gobject_class->finalize = ogmrip_media_info_finalize;

  g_type_class_add_private (klass, sizeof (OGMRipMediaInfoPriv));
}

static void
ogmrip_media_info_init (OGMRipMediaInfo *info)
{
  info->priv = G_TYPE_INSTANCE_GET_PRIVATE (info, OGMRIP_TYPE_MEDIA_INFO, OGMRipMediaInfoPriv);
}

static void
ogmrip_media_info_constructed (GObject *gobject)
{
  OGMRipMediaInfo *info = OGMRIP_MEDIA_INFO (gobject);

  MediaInfoDLL_Load ();

  info->priv->handle = MediaInfo_New ();
  if (!info->priv->handle)
  {
  }

  G_OBJECT_CLASS (ogmrip_media_info_parent_class)->constructed (gobject);
}

static void
ogmrip_media_info_finalize (GObject *gobject)
{
  OGMRipMediaInfo *info = OGMRIP_MEDIA_INFO (gobject);

  if (info->priv->handle)
  {
    if (info->priv->is_open)
      MediaInfo_Close (info->priv->handle);
    info->priv->handle = NULL;
  }

  MediaInfoDLL_UnLoad ();

  G_OBJECT_CLASS (ogmrip_media_info_parent_class)->finalize (gobject);
}

OGMRipMediaInfo *
ogmrip_media_info_get_default (void)
{
  if (!default_media_info)
    default_media_info = g_object_new (OGMRIP_TYPE_MEDIA_INFO, NULL);

  return default_media_info;
}

gboolean
ogmrip_media_info_open (OGMRipMediaInfo *info, const gchar *filename)
{
  g_return_val_if_fail (OGMRIP_IS_MEDIA_INFO (info), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

  if (info->priv->is_open)
    MediaInfo_Close (info->priv->handle);

  info->priv->is_open = MediaInfo_Open (info->priv->handle, filename);

  return info->priv->is_open;
}

void
ogmrip_media_info_close (OGMRipMediaInfo *info)
{
  g_return_if_fail (OGMRIP_IS_MEDIA_INFO (info));

  if (info->priv->is_open)
    MediaInfo_Close (info->priv->handle);
}

const gchar *
ogmrip_media_info_get (OGMRipMediaInfo *info, OGMRipCategoryType category, guint stream, const gchar *name)
{
  const gchar *value;

  g_return_val_if_fail (OGMRIP_IS_MEDIA_INFO (info), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  if (!info->priv->handle)
    return NULL;

  value = MediaInfo_Get (info->priv->handle, category, stream, name, MediaInfo_Info_Text, MediaInfo_Info_Name);

  return *value ? value : NULL;
}

const gchar *
ogmrip_media_info_options (OGMRipMediaInfo *info, const gchar *name, const gchar *value)
{
  g_return_val_if_fail (OGMRIP_IS_MEDIA_INFO (info), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  if (!info->priv->handle)
    return NULL;

  return MediaInfo_Option (info->priv->handle, name, value);
}

