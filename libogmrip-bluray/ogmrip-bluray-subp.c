/* OGMRipBluray - A bluray library for OGMRip
 * Copyright (C) 2012 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-bluray-subp.h"
#include "ogmrip-bluray-title.h"
#include "ogmrip-bluray-priv.h"

static void ogmbr_stream_iface_init      (OGMRipStreamInterface     *iface);
static void ogmbr_subp_stream_iface_init (OGMRipSubpStreamInterface *iface);

G_DEFINE_TYPE_WITH_CODE (OGMBrSubpStream, ogmbr_subp_stream, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_STREAM, ogmbr_stream_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_SUBP_STREAM, ogmbr_subp_stream_iface_init));

static void
ogmbr_subp_stream_init (OGMBrSubpStream *subp)
{
  subp->priv = G_TYPE_INSTANCE_GET_PRIVATE (subp, OGMBR_TYPE_SUBP_STREAM, OGMBrSubpStreamPriv);
}

static void
ogmbr_subp_stream_class_init (OGMBrSubpStreamClass *klass)
{
  g_type_class_add_private (klass, sizeof (OGMBrSubpStreamPriv));
}

static gint
ogmbr_subp_stream_get_format (OGMRipStream *stream)
{
  return OGMRIP_FORMAT_PGS;
}

static gint
ogmbr_subp_stream_get_id (OGMRipStream *stream)
{
  return OGMBR_SUBP_STREAM (stream)->priv->id;
}

static OGMRipTitle *
ogmbr_subp_stream_get_title (OGMRipStream *stream)
{
  g_message ("get title");

  return OGMBR_SUBP_STREAM (stream)->priv->title;
}

static void
ogmbr_stream_iface_init (OGMRipStreamInterface *iface)
{
  iface->get_format = ogmbr_subp_stream_get_format;
  iface->get_id     = ogmbr_subp_stream_get_id;
  iface->get_title  = ogmbr_subp_stream_get_title;
}

static gint
ogmbr_subp_stream_get_content (OGMRipSubpStream *subp)
{
  /* TODO content */
  return 0;
}

static gint
ogmbr_subp_stream_get_language (OGMRipSubpStream *subp)
{
  return OGMBR_SUBP_STREAM (subp)->priv->lang;
}

static void
ogmbr_subp_stream_iface_init (OGMRipSubpStreamInterface *iface)
{
  iface->get_content  = ogmbr_subp_stream_get_content;
  iface->get_language = ogmbr_subp_stream_get_language;
}

