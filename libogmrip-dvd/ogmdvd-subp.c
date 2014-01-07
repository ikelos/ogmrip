/* OGMRipDvd - A DVD library for OGMRip
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/**
 * SECTION:ogmdvd-subp
 * @title: OGMDvdSubp
 * @include: ogmdvd-subp.h
 * @short_description: Structure describing a subtitles stream
 */

#include "ogmdvd-subp.h"
#include "ogmdvd-priv.h"

static void ogmrip_stream_iface_init      (OGMRipStreamInterface     *iface);
static void ogmrip_subp_stream_iface_init (OGMRipSubpStreamInterface *iface);

G_DEFINE_TYPE_WITH_CODE (OGMDvdSubpStream, ogmdvd_subp_stream, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_STREAM, ogmrip_stream_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_SUBP_STREAM, ogmrip_subp_stream_iface_init));

static void
ogmdvd_subp_stream_finalize (GObject *gobject)
{
#ifdef G_ENABLE_DEBUG
  // g_message ("Finalizing %s (%d)", G_OBJECT_TYPE_NAME (gobject), OGMDVD_SUBP_STREAM (gobject)->priv->id);
#endif

  G_OBJECT_CLASS (ogmdvd_subp_stream_parent_class)->finalize (gobject);
}  

static void
ogmdvd_subp_stream_init (OGMDvdSubpStream *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream, OGMDVD_TYPE_SUBP_STREAM, OGMDvdSubpStreamPriv);
}

static void
ogmdvd_subp_stream_class_init (OGMDvdSubpStreamClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = ogmdvd_subp_stream_finalize;

  g_type_class_add_private (klass, sizeof (OGMDvdSubpStreamPriv));
}

static gint
ogmdvd_subp_stream_get_format (OGMRipStream *stream)
{
  return OGMRIP_FORMAT_VOBSUB;
}

static gint
ogmdvd_subp_stream_get_id (OGMRipStream *stream)
{
  return OGMDVD_SUBP_STREAM (stream)->priv->id;
}

OGMRipTitle *
ogmdvd_subp_stream_get_title (OGMRipStream *stream)
{
  return OGMDVD_SUBP_STREAM (stream)->priv->title;
}

static void
ogmrip_stream_iface_init (OGMRipStreamInterface *iface)
{
  iface->get_format = ogmdvd_subp_stream_get_format;
  iface->get_id     = ogmdvd_subp_stream_get_id;
  iface->get_title  = ogmdvd_subp_stream_get_title;
}

static gint
ogmdvd_subp_stream_get_content (OGMRipSubpStream *subp)
{
  return OGMDVD_SUBP_STREAM (subp)->priv->lang_extension - 1;
}

static gint
ogmdvd_subp_stream_get_language (OGMRipSubpStream *subp)
{
  return OGMDVD_SUBP_STREAM (subp)->priv->lang_code;
}

static void
ogmrip_subp_stream_iface_init (OGMRipSubpStreamInterface *iface)
{
  iface->get_content  = ogmdvd_subp_stream_get_content;
  iface->get_language = ogmdvd_subp_stream_get_language;
}

