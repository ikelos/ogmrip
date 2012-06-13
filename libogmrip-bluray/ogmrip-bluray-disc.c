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

#include "ogmrip-bluray-disc.h"
#include "ogmrip-bluray-title.h"
#include "ogmrip-bluray-makemkv.h"
#include "ogmrip-bluray-priv.h"

#include <ogmrip-base.h>

enum
{
  PROP_0,
  PROP_URI
};

static void ogmbr_media_iface_init (OGMRipMediaInterface *iface);

G_DEFINE_TYPE_WITH_CODE (OGMBrDisc, ogmbr_disc, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_MEDIA, ogmbr_media_iface_init));

static void
ogmbr_disc_constructed (GObject *gobject)
{
  OGMBrDisc *disc = OGMBR_DISC (gobject);

  if (!disc->priv->uri)
    disc->priv->uri = g_strdup ("br:///dev/dvd");

  disc->priv->device = g_strdup (disc->priv->uri + 5);

  G_OBJECT_CLASS (ogmbr_disc_parent_class)->constructed (gobject);
}

static void
ogmbr_disc_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  OGMBrDisc *disc = OGMBR_DISC (gobject);

  switch (prop_id)
  {
    case PROP_URI:
      g_value_set_string (value, disc->priv->uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmbr_disc_set_uri (OGMBrDisc *disc, const gchar *uri)
{
  if (g_str_has_prefix (uri, "br://"))
    disc->priv->uri = g_strdup (uri);
  else
    disc->priv->uri = g_strdup_printf ("br://%s", uri);
}

static void
ogmbr_disc_set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  OGMBrDisc *disc = OGMBR_DISC (gobject);

  switch (prop_id)
  {
    case PROP_URI:
      ogmbr_disc_set_uri (disc, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmbr_disc_dispose (GObject *gobject)
{
  ogmrip_media_close (OGMRIP_MEDIA (gobject));

  G_OBJECT_CLASS (ogmbr_disc_parent_class)->dispose (gobject);
}

static void
ogmbr_disc_finalize (GObject *gobject)
{
  OGMBrDisc *disc = OGMBR_DISC (gobject);

  g_free (disc->priv->device);
  g_free (disc->priv->uri);

  G_OBJECT_CLASS (ogmbr_disc_parent_class)->finalize (gobject);
}

static void
ogmbr_disc_init (OGMBrDisc *disc)
{
  disc->priv = G_TYPE_INSTANCE_GET_PRIVATE (disc, OGMBR_TYPE_DISC, OGMBrDiscPriv);
}

static void
ogmbr_disc_class_init (OGMBrDiscClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmbr_disc_constructed;
  gobject_class->get_property = ogmbr_disc_get_property;
  gobject_class->set_property = ogmbr_disc_set_property;
  gobject_class->dispose = ogmbr_disc_dispose;
  gobject_class->finalize = ogmbr_disc_finalize;

  g_object_class_override_property (gobject_class, PROP_URI, "uri");

  g_type_class_add_private (klass, sizeof (OGMBrDiscPriv));
}

static gboolean
ogmbr_disc_open (OGMRipMedia *media, GCancellable *cancellable, OGMRipMediaCallback callback, gpointer user_data, GError **error)
{
  OGMBrDisc *disc = OGMBR_DISC (media);

  if (disc->priv->handle)
    return TRUE;

  return ogmbr_makemkv_open_disc (ogmbr_makemkv_get_default (), disc, NULL, NULL, NULL, error);
}

static gboolean
ogmbr_disc_is_open (OGMRipMedia *media)
{
  return OGMBR_DISC (media)->priv->handle != 0;
}

static void
ogmbr_disc_close (OGMRipMedia *media)
{
  OGMBrDisc *disc = OGMBR_DISC (media);

  g_list_foreach (disc->priv->titles, (GFunc) g_object_unref, NULL);
  g_list_free (disc->priv->titles);
  disc->priv->titles = NULL;

  g_free (disc->priv->label);
  disc->priv->label = NULL;

  disc->priv->ntitles = 0;
  disc->priv->handle = 0;

  ogmbr_makemkv_close_disc (ogmbr_makemkv_get_default (), NULL, NULL);
}

static const gchar *
ogmbr_disc_get_label (OGMRipMedia *media)
{
  return OGMBR_DISC (media)->priv->label;
}

static const gchar *
ogmbr_disc_get_uri (OGMRipMedia *media)
{
  return OGMBR_DISC (media)->priv->uri;
}

static gint64
ogmbr_disc_get_size (OGMRipMedia *media)
{
  GList *link;
  guint64 size = 0;

  for (link = OGMBR_DISC (media)->priv->titles; link; link = link->next)
  {
    OGMBrTitle *title = link->data;

    size += title->priv->size;
  }

  return size;
}

static gint
ogmbr_disc_get_n_titles (OGMRipMedia *media)
{
  return OGMBR_DISC (media)->priv->ntitles;
}

static OGMRipTitle *
ogmbr_disc_get_nth_title (OGMRipMedia *media, guint nr)
{
  return g_list_nth_data (OGMBR_DISC (media)->priv->titles, nr);
}

static void
ogmbr_media_iface_init (OGMRipMediaInterface *iface)
{
  iface->open          = ogmbr_disc_open;
  iface->close         = ogmbr_disc_close;
  iface->is_open       = ogmbr_disc_is_open;
  iface->get_label     = ogmbr_disc_get_label;
/*
  iface->get_id        = ogmbr_disc_get_id;
*/
  iface->get_uri       = ogmbr_disc_get_uri;
  iface->get_size      = ogmbr_disc_get_size;
  iface->get_n_titles  = ogmbr_disc_get_n_titles;
  iface->get_nth_title = ogmbr_disc_get_nth_title;
/*
  iface->copy          = ogmbr_disc_copy;
*/
}

void
ogmrip_dvd_register_media (void)
{
  OGMRipTypeInfo *info;
  OGMBrMakeMKV *mmkv;

  info = g_object_new (OGMRIP_TYPE_TYPE_INFO, "name", "Bluray", "description", "Bluray", NULL);

  mmkv = ogmbr_makemkv_get_default ();
  g_object_set_data_full (G_OBJECT (info), "makemkv", mmkv, g_object_unref);

  ogmrip_type_register (OGMBR_TYPE_DISC, info);
}

