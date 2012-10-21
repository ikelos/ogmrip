/* OGMRipDvd - A DVD library for OGMRip
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmdvd-priv.h"

#include <ogmrip-job.h>

#include <stdio.h>
#include <string.h>

#include <sys/stat.h>

#include <glib/gstdio.h>

gulong
ogmdvd_time_to_msec (dvd_time_t *dtime)
{
  guint hour, min, sec, frames;
  gfloat fps;

  hour   = ((dtime->hour    & 0xf0) >> 4) * 10 + (dtime->hour    & 0x0f);
  min    = ((dtime->minute  & 0xf0) >> 4) * 10 + (dtime->minute  & 0x0f);
  sec    = ((dtime->second  & 0xf0) >> 4) * 10 + (dtime->second  & 0x0f);
  frames = ((dtime->frame_u & 0x30) >> 4) * 10 + (dtime->frame_u & 0x0f);

  if (((dtime->frame_u & 0xc0) >> 6) == 1)
    fps = 25.0;
  else
    fps = 30000 / 1001.0;

  return hour * 60 * 60 * 1000 + min * 60 * 1000 + sec * 1000 + (gfloat) frames * 1000.0 / fps;
}

typedef struct
{
  gint val;
  gint ref;
} UInfo;

static gint
g_ulist_min (UInfo *info1, UInfo *info2)
{
  return info2->val - info1->val;
}

static gint
g_ulist_max (UInfo *info1, UInfo *info2)
{
  return info1->val - info2->val;
}

static GSList *
g_ulist_add (GSList *ulist, GCompareFunc func, gint val)
{
  GSList *ulink;
  UInfo *info;

  for (ulink = ulist; ulink; ulink = ulink->next)
  {
    info = ulink->data;

    if (info->val == val)
      break;
  }

  if (ulink)
  {
    info = ulink->data;
    info->ref ++;
  }
  else
  {
    info = g_new0 (UInfo, 1);
    info->val = val;
    info->ref = 1;

    ulist = g_slist_insert_sorted (ulist, info, func);
  }

  return ulist;
}

GSList *
g_ulist_add_min (GSList *ulist, gint val)
{
  return g_ulist_add (ulist, (GCompareFunc) g_ulist_min, val);
}

GSList *
g_ulist_add_max (GSList *ulist, gint val)
{
  return g_ulist_add (ulist, (GCompareFunc) g_ulist_max, val);
}

gint
g_ulist_get_most_frequent (GSList *ulist)
{
  GSList *ulink;
  UInfo *info, *umax;

  if (!ulist)
    return 0;

  umax = ulist->data;

  for (ulink = ulist; ulink; ulink = ulink->next)
  {
    info = ulink->data;

    if (info->ref > umax->ref)
      umax = info;
  }

  return umax->val;
}

void
g_ulist_free (GSList *ulist)
{
  g_slist_foreach (ulist, (GFunc) g_free, NULL);
  g_slist_free (ulist);
}

typedef struct
{
  OGMRipMedia *disc;
  OGMRipMediaCallback callback;
  gpointer user_data;
} OGMDvdProgress;

static gboolean
ogmdvd_disc_copy_watch (OGMJobTask *spawn, const gchar *buffer, gpointer data, GError **error)
{
  guint bytes, total, percent;

  if (sscanf (buffer, "%u/%u blocks written (%u%%)", &bytes, &total, &percent) == 3)
    ogmjob_task_set_progress (spawn, percent / 100.);

  return TRUE;
}

static void
ogmdvd_disc_copy_progress_cb (OGMJobTask *spawn, GParamSpec *pspec, OGMDvdProgress *progress)
{
  progress->callback (progress->disc, ogmjob_task_get_progress (spawn), progress->user_data);
}

static gboolean
ogmdvd_disc_copy_exists (OGMDvdDisc *disc, const gchar *path)
{
  OGMRipMedia *media;
  gboolean retval;

  if (!g_file_test (path, G_FILE_TEST_IS_DIR))
    return FALSE;

  media = ogmdvd_disc_new (path, NULL);
  if (!media)
    return FALSE;

  retval = g_str_equal (ogmrip_media_get_id (OGMRIP_MEDIA (disc)), ogmrip_media_get_id (media));

  g_object_unref (media);

  return retval;
}

static gchar **
ogmdvd_disc_copy_command (OGMDvdDisc *disc, OGMDvdTitle *title, const gchar *path)
{
  GPtrArray *argv;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("dvdcpy"));
  g_ptr_array_add (argv, g_strdup ("-s"));
  g_ptr_array_add (argv, g_strdup ("skip"));
  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (path));
  g_ptr_array_add (argv, g_strdup ("-m"));

  if (title)
  {
    g_ptr_array_add (argv, g_strdup ("-t"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", title->priv->nr + 1));
  }

  g_ptr_array_add (argv, g_strdup (disc->priv->device));

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

OGMDvdDisc *
ogmdvd_disc_copy (OGMDvdDisc *disc, OGMDvdTitle *title, const gchar *path,
    GCancellable *cancellable, OGMRipMediaCallback callback, gpointer user_data, GError **error)
{
  OGMRipMedia *media = OGMRIP_MEDIA (disc);
  OGMJobTask *spawn;
  OGMDvdDisc *copy;

  struct stat buf;
  gboolean is_open;
  gchar **argv;
  gint result;

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  if (g_stat (disc->priv->device, &buf) != 0)
    return NULL;

  if (!S_ISBLK (buf.st_mode))
    return NULL;

  if (!ogmdvd_disc_copy_exists (disc, path))
  {
    is_open = ogmrip_media_is_open (media);
    if (!is_open && !ogmrip_media_open (media, cancellable, callback, user_data, error))
      return NULL;

    argv = ogmdvd_disc_copy_command (disc, title, path);

    spawn = ogmjob_spawn_newv (argv);
    ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (spawn),
        (OGMJobWatch) ogmdvd_disc_copy_watch, NULL);

    if (callback)
    {
      OGMDvdProgress progress;

      progress.disc = media;
      progress.callback = callback;
      progress.user_data = user_data;

      g_signal_connect (spawn, "notify::progress",
          G_CALLBACK (ogmdvd_disc_copy_progress_cb), &progress);
    }

    result = ogmjob_task_run (spawn, cancellable, error);
    g_object_unref (spawn);

    if (!is_open)
      ogmrip_media_close (media);

    if (!result)
      return NULL;
  }

  copy = g_object_new (OGMDVD_TYPE_DISC, "uri", path, NULL);

  if (!copy->priv->id || !g_str_equal (copy->priv->id, disc->priv->id))
  {
    copy->priv->real_id = copy->priv->id;
    copy->priv->id = g_strdup (disc->priv->id);
  }

  return copy;
}

