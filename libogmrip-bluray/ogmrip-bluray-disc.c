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
#include <ogmrip-job.h>

#include <stdio.h>
#include <stdlib.h>

#include <libbluray/bluray.h>

#include <glib/gstdio.h>

enum
{
  PROP_0,
  PROP_URI
};

enum
{
  AP_AVStreamFlag_DirectorsComments          = 1,
  AP_AVStreamFlag_AlternateDirectorsComments = 2,
  AP_AVStreamFlag_ForVisuallyImpaired        = 4,
  AP_AVStreamFlag_CoreAudio                  = 256,
  AP_AVStreamFlag_SecondaryAudio             = 512,
  AP_AVStreamFlag_HasCoreAudio               = 1024,
  AP_AVStreamFlag_DerivedStream              = 2048,
  AP_AVStreamFlag_ForcedSubtitles            = 4096,
  AP_AVStreamFlag_ProfileSecondaryStream     = 16384
};

static void ogmbr_media_iface_init (OGMRipMediaInterface *iface);

static GVolume *
g_volume_get_from_device (const gchar *device)
{
  GVolume *volume = NULL;
  GVolumeMonitor *monitor;
  GList *list, *link;
  gchar *str;

  monitor = g_volume_monitor_get ();
  list = g_volume_monitor_get_volumes (monitor);
  g_object_unref (monitor);

  for (link = list; link; link = link->next)
  {
    if (!volume)
    {
      str = g_volume_get_identifier (link->data, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
      if (g_str_equal (str, device))
        volume = g_object_ref (link->data);
      g_free (str);
    }

    g_object_unref (link->data);
  }

  g_list_free (list);

  return volume;
}

G_DEFINE_TYPE_WITH_CODE (OGMBrDisc, ogmbr_disc, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_MEDIA, ogmbr_media_iface_init));

static GObject *
ogmbr_disc_constructor (GType gtype, guint n_properties, GObjectConstructParam *properties)
{
  GObject *gobject;
  OGMBrDisc *disc;
  GVolume *volume;

  BLURAY *bd;
  const BLURAY_DISC_INFO *info;

  gobject = G_OBJECT_CLASS (ogmbr_disc_parent_class)->constructor (gtype, n_properties, properties);

  disc = OGMBR_DISC (gobject);

  if (!disc->priv->uri)
    disc->priv->uri = g_strdup ("br:///dev/dvd");

  if (!g_str_has_prefix (disc->priv->uri, "br://"))
  {
    g_object_unref (disc);
    return NULL;
  }

  disc->priv->device = g_strdup (disc->priv->uri + 5);

  volume = g_volume_get_from_device (disc->priv->device);
  if (!volume)
  {
    g_object_unref (disc);
    return NULL;
  }

  disc->priv->id = g_volume_get_name (volume);
  g_object_unref (volume);

  bd = bd_open (disc->priv->device, NULL);
  if (!bd)
  {
    g_warning ("Cannot open bluray disc");
    g_object_unref (disc);
    return NULL;
  }

  info = bd_get_disc_info (bd);
  if (!info)
  {
    g_warning ("Cannot get bluray disc info");
    g_object_unref (disc);
    bd_close (bd);
    return NULL;
  }

  if (!info->bluray_detected)
  {
    g_warning ("No bluray disc detected");
    g_object_unref (disc);
    bd_close (bd);
    return NULL;
  }

  bd_close (bd);

  return gobject;
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
ogmbr_disc_finalize (GObject *gobject)
{
  OGMBrDisc *disc = OGMBR_DISC (gobject);

  ogmrip_media_close (OGMRIP_MEDIA (gobject));

  g_free (disc->priv->label);
  g_free (disc->priv->device);
  g_free (disc->priv->uri);
  g_free (disc->priv->id);

  G_OBJECT_CLASS (ogmbr_disc_parent_class)->finalize (gobject);
}

static void
ogmbr_disc_init (OGMBrDisc *disc)
{
  disc->priv = G_TYPE_INSTANCE_GET_PRIVATE (disc, OGMBR_TYPE_DISC, OGMBrDiscPriv);

  disc->priv->nr = -1;
}

static void
ogmbr_disc_class_init (OGMBrDiscClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructor = ogmbr_disc_constructor;
  gobject_class->get_property = ogmbr_disc_get_property;
  gobject_class->set_property = ogmbr_disc_set_property;
  gobject_class->finalize = ogmbr_disc_finalize;

  g_object_class_override_property (gobject_class, PROP_URI, "uri");

  g_type_class_add_private (klass, sizeof (OGMBrDiscPriv));
}

typedef struct
{
  OGMRipMedia *media;
  OGMRipMediaCallback callback;
  gpointer user_data;
} ProgressData;

static void
ogmbr_disc_action_progress_cb (OGMJobTask *task, GParamSpec *pspec, ProgressData *data)
{
  data->callback (data->media, ogmjob_task_get_progress (task), data->user_data);
}

static gboolean
ogmbr_disc_open (OGMRipMedia *media, GCancellable *cancellable, OGMRipMediaCallback callback, gpointer user_data, GError **error)
{
  OGMBrDisc *disc = OGMBR_DISC (media);
  OGMBrMakeMKV *mmkv;
  ProgressData data;

  if (disc->priv->is_open)
    return TRUE;

  mmkv = ogmbr_makemkv_get_default ();

  if (callback)
  {
    data.media = g_object_ref (media);
    data.callback = callback;
    data.user_data = user_data;

    g_signal_connect (mmkv, "notify::progress",
        G_CALLBACK (ogmbr_disc_action_progress_cb), &data);
  }

  disc->priv->is_open = ogmbr_makemkv_open_disc (mmkv, disc, cancellable, error);
  g_object_unref (mmkv);

  return disc->priv->is_open;
}

static gboolean
ogmbr_disc_is_open (OGMRipMedia *media)
{
  return OGMBR_DISC (media)->priv->is_open;
}

static const gchar *
ogmbr_disc_get_id (OGMRipMedia *media)
{
  return OGMBR_DISC (media)->priv->id;
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

static gboolean
ogmbr_disc_copy_exists (OGMBrDisc *disc, const gchar *path)
{
  OGMRipMedia *media;
  const gchar *id;
  gboolean retval;

  if (!g_file_test (path, G_FILE_TEST_IS_DIR))
    return FALSE;

  media = g_object_new (OGMBR_TYPE_DISC, "uri", path, NULL);
  if (!media)
    return FALSE;

  id = ogmrip_media_get_id (media);

  retval = id && disc->priv->id && g_str_equal (id, disc->priv->id);

  g_object_unref (media);

  return retval;
}

static OGMRipMedia *
ogmbr_disc_copy (OGMRipMedia *media, const gchar *path, GCancellable *cancellable,
    OGMRipMediaCallback callback, gpointer user_data, GError **error)
{
  OGMBrDisc *disc = OGMBR_DISC (media), *copy;
  struct stat buf;

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  if (g_stat (disc->priv->device, &buf) != 0)
    return NULL;

  if (!S_ISBLK (buf.st_mode))
    return NULL;

  if (!ogmbr_disc_copy_exists (disc, path))
  {
    OGMBrMakeMKV *mmkv;
    ProgressData data;
    gboolean retval;

    mmkv = ogmbr_makemkv_get_default ();

    if (callback)
    {
      data.media = g_object_ref (media);
      data.callback = callback;
      data.user_data = user_data;

      g_signal_connect (mmkv, "notify::progress",
          G_CALLBACK (ogmbr_disc_action_progress_cb), &data);
    }

    retval = ogmbr_makemkv_backup_disc (mmkv, disc, path, cancellable, error);
    g_object_unref (mmkv);

    if (!retval)
      return NULL;
  }

  copy = g_object_new (OGMBR_TYPE_DISC, "uri", path, NULL);

  if (!copy->priv->id || !g_str_equal (copy->priv->id, disc->priv->id))
  {
    copy->priv->real_id = copy->priv->id;
    copy->priv->id = g_strdup (disc->priv->id);
  }

  return OGMRIP_MEDIA (copy);
}

static void
ogmbr_media_iface_init (OGMRipMediaInterface *iface)
{
  iface->open          = ogmbr_disc_open;
  iface->is_open       = ogmbr_disc_is_open;
  iface->get_label     = ogmbr_disc_get_label;
  iface->get_id        = ogmbr_disc_get_id;
  iface->get_uri       = ogmbr_disc_get_uri;
  iface->get_size      = ogmbr_disc_get_size;
  iface->get_n_titles  = ogmbr_disc_get_n_titles;
  iface->get_nth_title = ogmbr_disc_get_nth_title;
  iface->copy          = ogmbr_disc_copy;
}

void
ogmrip_bluray_register_media (void)
{
  OGMRipTypeInfo *info;

  info = g_object_new (OGMRIP_TYPE_TYPE_INFO, "name", "Bluray", "description", "Bluray", NULL);
  ogmrip_type_register (OGMBR_TYPE_DISC, info);
}

