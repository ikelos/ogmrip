/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-media-chapters.h"
#include "ogmrip-media-stream.h"
#include "ogmrip-media-enums.h"

G_DEFINE_INTERFACE_WITH_CODE (OGMRipChaptersStream, ogmrip_chapters_stream, G_TYPE_OBJECT,
    g_type_interface_add_prerequisite (g_define_type_id, OGMRIP_TYPE_STREAM);)

static void
ogmrip_chapters_stream_default_init (OGMRipChaptersStreamInterface *iface)
{
}

const gchar * 
ogmrip_chapters_stream_get_label (OGMRipChaptersStream *chapters, guint nr)
{
  OGMRipChaptersStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_CHAPTERS_STREAM (chapters), NULL);

  iface = OGMRIP_CHAPTERS_STREAM_GET_IFACE (chapters);

  if (!iface->get_label)
    return NULL;

  return iface->get_label (chapters, nr);
}

gint 
ogmrip_chapters_stream_get_language (OGMRipChaptersStream *chapters)
{
  OGMRipChaptersStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_CHAPTERS_STREAM (chapters), -1);

  iface = OGMRIP_CHAPTERS_STREAM_GET_IFACE (chapters);

  if (!iface->get_language)
    return 0;

  return iface->get_language (chapters);
}

