/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-media-subp.h"
#include "ogmrip-media-stream.h"
#include "ogmrip-media-enums.h"

G_DEFINE_INTERFACE_WITH_CODE (OGMRipSubpStream, ogmrip_subp_stream, G_TYPE_OBJECT,
    g_type_interface_add_prerequisite (g_define_type_id, OGMRIP_TYPE_STREAM);)

static void
ogmrip_subp_stream_default_init (OGMRipSubpStreamInterface *iface)
{
}

gint 
ogmrip_subp_stream_get_charset (OGMRipSubpStream *subp)
{
  OGMRipSubpStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_SUBP_STREAM (subp), -1);

  iface = OGMRIP_SUBP_STREAM_GET_IFACE (subp);

  if (!iface->get_charset)
    return OGMRIP_CHARSET_UNDEFINED;

  return iface->get_charset (subp);
}

gint 
ogmrip_subp_stream_get_content (OGMRipSubpStream *subp)
{
  OGMRipSubpStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_SUBP_STREAM (subp), -1);

  iface = OGMRIP_SUBP_STREAM_GET_IFACE (subp);

  if (!iface->get_content)
    return OGMRIP_SUBP_CONTENT_UNDEFINED;

  return iface->get_content (subp);
}

const gchar * 
ogmrip_subp_stream_get_label (OGMRipSubpStream *subp)
{
  OGMRipSubpStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_SUBP_STREAM (subp), NULL);

  iface = OGMRIP_SUBP_STREAM_GET_IFACE (subp);

  if (!iface->get_label)
    return NULL;

  return iface->get_label (subp);
}

gint 
ogmrip_subp_stream_get_language (OGMRipSubpStream *subp)
{
  OGMRipSubpStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_SUBP_STREAM (subp), -1);

  iface = OGMRIP_SUBP_STREAM_GET_IFACE (subp);

  if (!iface->get_language)
    return 0;

  return iface->get_language (subp);
}

gint 
ogmrip_subp_stream_get_newline (OGMRipSubpStream *subp)
{
  OGMRipSubpStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_SUBP_STREAM (subp), -1);

  iface = OGMRIP_SUBP_STREAM_GET_IFACE (subp);

  if (!iface->get_newline)
    return OGMRIP_NEWLINE_UNDEFINED;

  return iface->get_newline (subp);
}

gint
ogmrip_subp_stream_get_nr (OGMRipSubpStream *subp)
{
  OGMRipSubpStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_SUBP_STREAM (subp), -1);

  iface = OGMRIP_SUBP_STREAM_GET_IFACE (subp);

  if (!iface->get_newline)
    return 0;

  return iface->get_nr (subp);
}

