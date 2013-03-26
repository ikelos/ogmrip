/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2013 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-media-stream.h"
#include "ogmrip-media-title.h"
#include "ogmrip-media-object.h"

G_DEFINE_INTERFACE (OGMRipStream, ogmrip_stream, G_TYPE_OBJECT)

static void
ogmrip_stream_default_init (OGMRipStreamInterface *iface)
{
}

const gchar *
ogmrip_stream_get_uri (OGMRipStream *stream)
{
  g_return_val_if_fail (OGMRIP_IS_STREAM (stream), NULL);

  return ogmrip_media_get_uri (ogmrip_title_get_media (ogmrip_stream_get_title (stream)));
}

OGMRipTitle * 
ogmrip_stream_get_title (OGMRipStream *stream)
{
  OGMRipStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_STREAM (stream), NULL);

  iface = OGMRIP_STREAM_GET_IFACE (stream);

  if (!iface->get_title)
    return NULL;

  return iface->get_title (stream);
}

gint 
ogmrip_stream_get_id (OGMRipStream *stream)
{
  OGMRipStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_STREAM (stream), -1);

  iface = OGMRIP_STREAM_GET_IFACE (stream);

  if (!iface->get_id)
    return 0;

  return iface->get_id (stream);
}

gint
ogmrip_stream_get_format (OGMRipStream *stream)
{
  OGMRipStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_STREAM (stream), -1);

  iface = OGMRIP_STREAM_GET_IFACE (stream);

  if (!iface->get_format)
    return -1;

  return iface->get_format (stream);
}

gboolean
ogmrip_stream_is_copy (OGMRipStream *stream, OGMRipStream *copy)
{
  OGMRipStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_STREAM (stream), -1);
  g_return_val_if_fail (OGMRIP_IS_STREAM (copy), -1);

  iface = OGMRIP_STREAM_GET_IFACE (stream);

  if (iface->is_copy)
    return iface->is_copy (stream, copy);

  if (G_OBJECT_TYPE (stream) != G_OBJECT_TYPE (copy))
    return FALSE;

  if (ogmrip_stream_get_id (stream) != ogmrip_stream_get_id (copy))
    return FALSE;

  return ogmrip_title_is_copy (ogmrip_stream_get_title (stream), ogmrip_stream_get_title (copy));
}

