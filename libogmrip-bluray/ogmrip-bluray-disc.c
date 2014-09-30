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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-bluray-disc.h"
#include "ogmrip-bluray-title.h"
#include "ogmrip-bluray-audio.h"
#include "ogmrip-bluray-subp.h"
#include "ogmrip-bluray-priv.h"
#include "ogmrip-bluray-makemkv.h"

#include <ogmrip-base.h>
#include <ogmrip-job.h>

#include <stdio.h>
#include <stdlib.h>

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

const guint MIN_DURATION = 120 * 90 * 1000;

enum
{
  PROP_0,
  PROP_URI
};

static void ogmbr_initable_iface_init (GInitableIface       *iface);
static void ogmbr_media_iface_init    (OGMRipMediaInterface *iface);

static OGMBrAudioStream *
ogmbr_audio_stream_new (OGMBrTitle *title, BLURAY *bd, BLURAY_TITLE_INFO *info, guint nr)
{
  OGMBrAudioStream *stream;

  stream = g_object_new (OGMBR_TYPE_AUDIO_STREAM, NULL);

  stream->priv->id     = nr;
  stream->priv->title  = OGMRIP_TITLE (title);
  stream->priv->type   = info->clips[0].audio_streams[nr].coding_type;
  stream->priv->format = info->clips[0].audio_streams[nr].format;
  stream->priv->rate   = info->clips[0].audio_streams[nr].rate;
  stream->priv->lang   = ogmrip_language_from_iso639_2 ((gchar *) info->clips[0].audio_streams[nr].lang);

  // g_object_add_weak_pointer (G_OBJECT (title), (gpointer *) &stream->priv->title);

  return stream;
}

static OGMBrSubpStream *
ogmbr_subp_stream_new (OGMBrTitle *title, BLURAY *bd, BLURAY_TITLE_INFO *info, guint nr)
{
  OGMBrSubpStream *stream;

  stream = g_object_new (OGMBR_TYPE_SUBP_STREAM, NULL);

  stream->priv->id    = nr;
  stream->priv->title = OGMRIP_TITLE (title);
  stream->priv->lang  = ogmrip_language_from_iso639_2 ((gchar *) info->clips[0].pg_streams[nr].lang);

  // g_object_add_weak_pointer (G_OBJECT (title), (gpointer *) &stream->priv->title);

  return stream;
}

static OGMBrTitle *
ogmbr_title_new (OGMBrDisc *disc, BLURAY *bd, BLURAY_TITLE_INFO *info, guint nr)
{
  OGMBrTitle *title;
  guint i;

  if (!info->clip_count || !info->clips[0].video_stream_count || info->duration < MIN_DURATION)
    return NULL;

  title = g_object_new (OGMBR_TYPE_TITLE, NULL);

  title->priv->id        = nr;
  title->priv->media     = OGMRIP_MEDIA (disc);
  title->priv->duration  = info->duration;
  title->priv->nangles   = info->angle_count;
  title->priv->size      = bd_get_title_size (bd);
  title->priv->type      = info->clips[0].video_streams[0].coding_type;
  title->priv->aspect    = info->clips[0].video_streams[0].aspect;
  title->priv->format    = info->clips[0].video_streams[0].format;
  title->priv->rate      = info->clips[0].video_streams[0].rate;

  // g_object_add_weak_pointer (G_OBJECT (disc), (gpointer *) &title->priv->media);

  for (i = 0; i < info->clips[0].audio_stream_count; i ++)
  {
    OGMBrAudioStream *stream;

    stream = ogmbr_audio_stream_new (title, bd, info, i);
    if (stream)
      title->priv->audio_streams = g_list_prepend (title->priv->audio_streams, stream);
  }
  title->priv->audio_streams = g_list_reverse (title->priv->audio_streams);

  for (i = 0; i < info->clips[0].pg_stream_count; i ++)
  {
    OGMBrSubpStream *stream;

    stream = ogmbr_subp_stream_new (title, bd, info, i);
    if (stream)
      title->priv->subp_streams = g_list_prepend (title->priv->subp_streams, stream);
  }
  title->priv->subp_streams = g_list_reverse (title->priv->subp_streams);

  title->priv->nchapters = info->chapter_count;
  title->priv->chapters = g_new0 (guint64, info->chapter_count);

  for (i = 0; i < info->chapter_count; i ++)
    title->priv->chapters[i] = info->chapters[i].duration;

  ogmrip_video_stream_get_resolution (OGMRIP_VIDEO_STREAM (title),
      &title->priv->crop_width, &title->priv->crop_height);

  return title;
}

G_DEFINE_TYPE_WITH_CODE (OGMBrDisc, ogmbr_disc, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, ogmbr_initable_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_MEDIA, ogmbr_media_iface_init)
    G_ADD_PRIVATE (OGMBrDisc));

static gboolean
ogmbr_disc_initable_init (GInitable *initable, GCancellable *cancellable, GError **error)
{
  OGMBrDisc *disc = OGMBR_DISC (initable);

  BLURAY *bd;
  const BLURAY_DISC_INFO *dinfo;
  const META_DL *meta;
  guint ntitles, nr;

  if (!disc->priv->uri)
    disc->priv->uri = g_strdup ("br:///dev/dvd");

  if (!g_str_has_prefix (disc->priv->uri, "br://"))
  {
    g_set_error (error, OGMRIP_MEDIA_ERROR, OGMRIP_MEDIA_ERROR_SCHEME,
        _("Unknown scheme for '%s'"), disc->priv->uri);
    return FALSE;
  }

  disc->priv->device = g_strdup (disc->priv->uri + 5);

  bd = bd_open (disc->priv->device, NULL);
  if (!bd)
  {
    g_set_error (error, OGMRIP_MEDIA_ERROR, OGMRIP_MEDIA_ERROR_OPEN,
        _("Cannot open '%s'"), disc->priv->uri);
    return FALSE;
  }

  dinfo = bd_get_disc_info (bd);
  if (!dinfo)
  {
    bd_close (bd);
    return FALSE;
  }

  if (!dinfo->bluray_detected)
  {
    bd_close (bd);
    return FALSE;
  }

  meta = bd_get_meta (bd);

  disc->priv->label = meta->di_name ? g_strdup (meta->di_name) : NULL;

  ntitles = bd_get_titles (bd, TITLES_RELEVANT, 0);
  for (nr = 0; nr < ntitles; nr ++)
  {
    if (bd_select_title (bd, nr))
    {
      BLURAY_TITLE_INFO *tinfo;
      OGMBrTitle *title;

      tinfo = bd_get_title_info (bd, nr, 0);

      title = ogmbr_title_new (disc, bd, tinfo, nr);
      if (title)
        disc->priv->titles = g_list_prepend (disc->priv->titles, title);
      bd_free_title_info (tinfo);
    }
  }
  disc->priv->titles = g_list_reverse (disc->priv->titles);

  bd_close (bd);

  return TRUE;
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
  OGMBrDisc *disc = OGMBR_DISC (gobject);

  g_clear_object (&disc->priv->copy);

  if (disc->priv->titles)
  {
    g_list_foreach (disc->priv->titles, (GFunc) g_object_unref, NULL);
    g_list_free (disc->priv->titles);
    disc->priv->titles = NULL;
  }

  G_OBJECT_CLASS (ogmbr_disc_parent_class)->dispose (gobject);
}

static void
ogmbr_disc_finalize (GObject *gobject)
{
  OGMBrDisc *disc = OGMBR_DISC (gobject);

  ogmrip_media_close (OGMRIP_MEDIA (disc));

  g_free (disc->priv->label);
  g_free (disc->priv->device);
  g_free (disc->priv->uri);

  G_OBJECT_CLASS (ogmbr_disc_parent_class)->finalize (gobject);
}

static void
ogmbr_disc_init (OGMBrDisc *disc)
{
  disc->priv = ogmbr_disc_get_instance_private (disc);
  disc->priv->selected = -1;
}

static void
ogmbr_disc_class_init (OGMBrDiscClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmbr_disc_get_property;
  gobject_class->set_property = ogmbr_disc_set_property;
  gobject_class->dispose = ogmbr_disc_dispose;
  gobject_class->finalize = ogmbr_disc_finalize;

  g_object_class_override_property (gobject_class, PROP_URI, "uri");
}

static gboolean
ogmbr_disc_is_open (OGMRipMedia *media)
{
  return OGMBR_DISC (media)->priv->bd != NULL;;
}

static gboolean
ogmbr_disc_open (OGMRipMedia *media, GCancellable *cancellable, OGMRipMediaCallback callback, gpointer user_data, GError **error)
{
  OGMBrDisc *disc = OGMBR_DISC (media);

  if (!disc->priv->bd)
  {
    BLURAY *bd;
    const BLURAY_DISC_INFO *dinfo;
    const META_DL *meta;

    bd = bd_open (disc->priv->device, NULL);
    if (!bd)
    {
      g_set_error (error, OGMRIP_MEDIA_ERROR, OGMRIP_MEDIA_ERROR_OPEN,
          _("Cannot open '%s'"), disc->priv->uri);
      return FALSE;
    }

    dinfo = bd_get_disc_info (bd);
    if (!dinfo)
    {
      bd_close (bd);
      return FALSE;
    }

    if (!dinfo->bluray_detected)
    {
      bd_close (bd);
      return FALSE;
    }

    meta = bd_get_meta (bd);

    if (!meta->di_name || !g_str_equal (meta->di_name, disc->priv->label))
    {
      g_set_error (error, OGMRIP_MEDIA_ERROR, OGMRIP_MEDIA_ERROR_ID,
          _("Device does not contain the expected Blu-ray"));
      bd_close (bd);
      return FALSE;
    }

    disc->priv->bd = bd;
  }

  return TRUE;
}

static void
ogmbr_disc_close (OGMRipMedia *media)
{
  OGMBrDisc *disc = OGMBR_DISC (media);

  if (disc->priv->bd)
  {
    bd_close (disc->priv->bd);
    disc->priv->selected = -1;
    disc->priv->bd = NULL;
  }
}

static const gchar *
ogmbr_disc_get_id (OGMRipMedia *media)
{
  return OGMBR_DISC (media)->priv->label;
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
  return g_list_length (OGMBR_DISC (media)->priv->titles);
}

static OGMRipTitle *
ogmbr_disc_get_title (OGMRipMedia *media, guint id)
{
  GList *link;

  for (link = OGMBR_DISC (media)->priv->titles; link; link = link->next)
  {
    OGMBrTitle *title = link->data;

    if (title->priv->id == id)
      return link->data;
  }

  return NULL;
}

static GList *
ogmbr_disc_get_titles (OGMRipMedia *media)
{
  return g_list_copy (OGMBR_DISC (media)->priv->titles);
}

static gboolean
ogmbr_disc_copy_exists (OGMBrDisc *disc, const gchar *path)
{
  OGMRipMedia *media;
  const gchar *id, *copy_id;
  gboolean retval;

  if (!g_file_test (path, G_FILE_TEST_IS_DIR))
    return FALSE;

  media = g_object_new (OGMBR_TYPE_DISC, "uri", path, NULL);
  if (!media)
    return FALSE;

  copy_id = ogmrip_media_get_id (media);
  id = ogmrip_media_get_id (OGMRIP_MEDIA (disc));

  retval = id && copy_id && g_str_equal (id, copy_id);

  g_object_unref (media);

  return retval;
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

static OGMRipMedia *
ogmbr_disc_copy (OGMRipMedia *media, const gchar *path, GCancellable *cancellable,
    OGMRipMediaCallback callback, gpointer user_data, GError **error)
{
  OGMBrDisc *disc = OGMBR_DISC (media);
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

  disc->priv->copy = g_object_new (OGMBR_TYPE_DISC, "uri", path, NULL);

  return g_object_ref (disc->priv->copy);
}

static void
ogmbr_initable_iface_init (GInitableIface *iface)
{
  iface->init = ogmbr_disc_initable_init;
}

static void
ogmbr_media_iface_init (OGMRipMediaInterface *iface)
{
  iface->open         = ogmbr_disc_open;
  iface->close        = ogmbr_disc_close;
  iface->is_open      = ogmbr_disc_is_open;
  iface->get_label    = ogmbr_disc_get_label;
  iface->get_id       = ogmbr_disc_get_id;
  iface->get_uri      = ogmbr_disc_get_uri;
  iface->get_size     = ogmbr_disc_get_size;
  iface->get_n_titles = ogmbr_disc_get_n_titles;
  iface->get_title    = ogmbr_disc_get_title;
  iface->get_titles   = ogmbr_disc_get_titles;
  iface->copy         = ogmbr_disc_copy;
}

void
ogmrip_bluray_register_media (void)
{
  OGMRipTypeInfo *info;

  info = g_object_new (OGMRIP_TYPE_TYPE_INFO, "name", "Bluray", "description", "Bluray", NULL);
  ogmrip_type_register (OGMBR_TYPE_DISC, info);
}

