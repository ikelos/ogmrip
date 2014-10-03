/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-media-object.h"

#include <ogmrip-base.h>

#include <glib/gi18n.h>

static const gchar *untitled = N_("Untitled");

GQuark
ogmrip_media_error_quark (void)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_static_string ("ogmrip-media-error-quark");

  return quark;
}

G_DEFINE_INTERFACE (OGMRipMedia, ogmrip_media, G_TYPE_OBJECT)

static void
ogmrip_media_default_init (OGMRipMediaInterface *iface)
{
  g_object_interface_install_property (iface,
      g_param_spec_string ("uri", "URI", "The URI of the media", NULL,
        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

gboolean
ogmrip_media_open (OGMRipMedia *media, GCancellable *cancellable, OGMRipMediaCallback callback, gpointer user_data, GError **error)
{
  OGMRipMediaInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_MEDIA (media), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  
  iface = OGMRIP_MEDIA_GET_IFACE (media);

  if (!iface->open)
    return TRUE;

  return iface->open (media, cancellable, callback, user_data, error);
}

void
ogmrip_media_close (OGMRipMedia *media)
{
  OGMRipMediaInterface *iface;

  g_return_if_fail (OGMRIP_IS_MEDIA (media));

  iface = OGMRIP_MEDIA_GET_IFACE (media);

  if (iface->close)
    iface->close (media);
}

gboolean
ogmrip_media_is_open (OGMRipMedia *media)
{
  OGMRipMediaInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_MEDIA (media), FALSE);

  iface = OGMRIP_MEDIA_GET_IFACE (media);

  if (!iface)
    return TRUE;

  return iface->is_open (media);
}

const gchar *
ogmrip_media_get_uri (OGMRipMedia *media)
{
  OGMRipMediaInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_MEDIA (media), NULL);

  iface = OGMRIP_MEDIA_GET_IFACE (media);

  if (!iface->get_uri)
    return NULL;

  return iface->get_uri (media);
}

const gchar *
ogmrip_media_get_label (OGMRipMedia *media)
{
  OGMRipMediaInterface *iface;
  const gchar *label;

  g_return_val_if_fail (OGMRIP_IS_MEDIA (media), NULL);

  iface = OGMRIP_MEDIA_GET_IFACE (media);

  if (!iface->get_label)
    return untitled;

  label = iface->get_label (media);

  return label ? label : untitled;
}

const gchar *
ogmrip_media_get_id (OGMRipMedia *media)
{
  OGMRipMediaInterface *iface;
  const gchar *id = NULL;

  g_return_val_if_fail (OGMRIP_IS_MEDIA (media), NULL);

  iface = OGMRIP_MEDIA_GET_IFACE (media);

  if (iface->get_id)
    id = iface->get_id (media);

  g_assert (id != NULL);

  return id;
}

guint64
ogmrip_media_get_size (OGMRipMedia *media)
{
  OGMRipMediaInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_MEDIA (media), -1);

  iface = OGMRIP_MEDIA_GET_IFACE (media);

  if (!iface->get_size)
  {
/*
    OGMRipTitle *title;

    if (ogmrip_media_get_n_titles (media) != 1)
      return -1;

    title = ogmrip_media_get_nth_title (media, 0);
    if (!title)
      return -1;

    return ogmrip_title_get_size (title);
*/
  }

  return iface->get_size (media);
}

gint
ogmrip_media_get_n_titles (OGMRipMedia *media)
{
  OGMRipMediaInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_MEDIA (media), -1);

  iface = OGMRIP_MEDIA_GET_IFACE (media);

  if (!iface->get_n_titles)
    return 0;

  return iface->get_n_titles (media);
}

OGMRipTitle *
ogmrip_media_get_title (OGMRipMedia *media, guint id)
{
  OGMRipMediaInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_MEDIA (media), NULL);

  iface = OGMRIP_MEDIA_GET_IFACE (media);

  if (!iface->get_title)
    return NULL;

  return iface->get_title (media, id);
}

GList *
ogmrip_media_get_titles (OGMRipMedia *media)
{
  OGMRipMediaInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_MEDIA (media), NULL);

  iface = OGMRIP_MEDIA_GET_IFACE (media);

  if (!iface->get_titles)
    return NULL;

  return iface->get_titles (media);
}

gboolean
ogmrip_media_get_require_copy (OGMRipMedia *media)
{
  OGMRipMediaInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_MEDIA (media), FALSE);

  iface = OGMRIP_MEDIA_GET_IFACE (media);

  if (!iface->get_require_copy)
    return FALSE;

  return iface->get_require_copy (media);
}

OGMRipMedia *
ogmrip_media_copy (OGMRipMedia *media, const gchar *path, GCancellable *cancellable,
    OGMRipMediaCallback callback, gpointer user_data, GError **error)
{
  OGMRipMediaInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_MEDIA (media), NULL);
  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  iface = OGMRIP_MEDIA_GET_IFACE (media);

  if (!iface->copy)
    return NULL;

  return iface->copy (media, path, cancellable, callback, user_data, error);
}

gboolean
ogmrip_media_is_copy (OGMRipMedia *media, OGMRipMedia *copy)
{
  OGMRipMediaInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_MEDIA (media), FALSE);
  g_return_val_if_fail (OGMRIP_IS_MEDIA (copy), FALSE);

  iface = OGMRIP_MEDIA_GET_IFACE (media);

  if (!iface->is_copy)
    return FALSE;

  return iface->is_copy (media, copy);
}

OGMRipMedia *
ogmrip_media_new (const gchar *path, GError **error)
{
  OGMRipMedia *media = NULL;
  GType *types;
  guint i;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (!path)
    return NULL;

  types = ogmrip_type_children (OGMRIP_TYPE_MEDIA, NULL);
  if (!types)
    return NULL;

  for (i = 0; types[i] != G_TYPE_NONE; i ++)
  {
    if (g_type_is_a (types[i], G_TYPE_INITABLE))
      media = g_initable_new (types[i], NULL, error, "uri", path, NULL);
    else
      media = g_object_new (types[i], "uri", path, NULL);
    if (media)
      break;
  }

  g_free (types);

  return media;
}

