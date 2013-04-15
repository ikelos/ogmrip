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

#include "ogmrip-media-title.h"
#include "ogmrip-media-object.h"
#include "ogmrip-media-video.h"

#include <ogmrip-base.h>
#include <ogmrip-job.h>

#include <stdio.h>
#include <string.h>

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

static GSList *
g_ulist_add_min (GSList *ulist, gint val)
{
  return g_ulist_add (ulist, (GCompareFunc) g_ulist_min, val);
}

static GSList *
g_ulist_add_max (GSList *ulist, gint val)
{
  return g_ulist_add (ulist, (GCompareFunc) g_ulist_max, val);
}

static gint
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

static void
g_ulist_free (GSList *ulist)
{
  g_slist_foreach (ulist, (GFunc) g_free, NULL);
  g_slist_free (ulist);
}

static void
ogmrip_title_set_input (OGMRipTitle *title, GPtrArray *argv)
{
  const gchar *uri;

  uri = ogmrip_media_get_uri (ogmrip_title_get_media (title));
  if (g_str_has_prefix (uri, "file://"))
    g_ptr_array_add (argv, g_strdup (uri + 7));
  else if (g_str_has_prefix (uri, "dvd://"))
  {
    g_ptr_array_add (argv, g_strdup ("-dvd-device"));
    g_ptr_array_add (argv, g_strdup (uri + 6));
    g_ptr_array_add (argv, g_strdup_printf ("dvd://%d", ogmrip_title_get_id (title) + 1));
  }
  else if (g_str_has_prefix (uri, "br://"))
  {
    g_ptr_array_add (argv, g_strdup ("-bluray-device"));
    g_ptr_array_add (argv, g_strdup (uri + 5));
    g_ptr_array_add (argv, g_strdup_printf ("br://%d", ogmrip_title_get_id (title) + 1));
  }
  else
    g_warning ("Unknown scheme for '%s'", uri);
}

typedef struct
{
  OGMRipTitle *title;
  OGMRipTitleCallback callback;
  gpointer user_data;
} OGMRipProgress;

enum
{
  SECTION_UNKNOWN,
  SECTION_24000_1001,
  SECTION_30000_1001
};

typedef struct
{
  gchar *cur_affinity;
  gchar* prev_affinity;
  guint naffinities;
  guint cur_duration;
  guint prev_duration;
  guint npatterns;
  guint cur_section;
  guint nsections;
  guint nframes;
  guint frames;
} OGMRipBenchmark;

static gchar **
ogmrip_title_benchmark_command (OGMRipTitle *title, gulong nframes)
{
  GPtrArray *argv;

  argv = g_ptr_array_new ();

  g_ptr_array_add (argv, g_strdup ("mplayer"));
  g_ptr_array_add (argv, g_strdup ("-nolirc"));
  g_ptr_array_add (argv, g_strdup ("-nosound"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-nosub"));

  g_ptr_array_add (argv, g_strdup ("-noconfig"));
  g_ptr_array_add (argv, g_strdup ("all"));

  g_ptr_array_add (argv, g_strdup ("-v"));
  g_ptr_array_add (argv, g_strdup ("-benchmark"));

  g_ptr_array_add (argv, g_strdup ("-vo"));
  g_ptr_array_add (argv, g_strdup ("null"));

  g_ptr_array_add (argv, g_strdup ("-vf"));
  g_ptr_array_add (argv, g_strdup ("pullup"));

  g_ptr_array_add (argv, g_strdup ("-frames"));
  g_ptr_array_add (argv, g_strdup_printf ("%lu", nframes));

  ogmrip_title_set_input (title, argv);

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gboolean
ogmrip_title_benchmark_watch (OGMJobTask *spawn, const gchar *buffer, OGMRipBenchmark *info, GError **error)
{
  if (g_str_has_prefix (buffer, "V: "))
  {
    info->frames ++;

    if (info->frames < info->nframes)
      ogmjob_task_set_progress (spawn, info->frames / (gdouble) info->nframes);
    else
      ogmjob_task_set_progress (spawn, 1.0);
  }
  else
  {
    if (g_str_has_prefix (buffer, "demux_mpg: 24000/1001"))
    {
      info->cur_section = SECTION_24000_1001;
      info->nsections ++;
    }
    else if (g_str_has_prefix (buffer, "demux_mpg: 30000/1001"))
    {
      info->cur_section = SECTION_30000_1001;
      info->nsections ++;
    }

    if (info->cur_section == SECTION_30000_1001)
    {
      if (g_str_has_prefix (buffer, "affinity: "))
      {
        g_free (info->prev_affinity);
        info->prev_affinity = g_strdup (info->cur_affinity);

        g_free (info->cur_affinity);
        info->cur_affinity = g_strdup (buffer + 10);
      }
      else if (g_str_has_prefix (buffer, "duration: "))
      {
        info->prev_duration = info->cur_duration;
        sscanf (buffer, "duration: %u", &info->cur_duration);

        if (info->prev_duration == 3 && info->cur_duration == 2)
        {
          info->npatterns ++;

          if (strncmp (info->prev_affinity, ".0+.1.+2", 8) == 0 && strncmp (info->cur_affinity, ".0++1", 5) == 0)
            info->naffinities ++;
        }
      }
    }
  }

  return TRUE;
}

static void
ogmrip_title_benchmark_progress_cb (OGMJobTask *spawn, GParamSpec *pspec, OGMRipProgress *progress)
{
  progress->callback (progress->title, ogmjob_task_get_progress (spawn), progress->user_data);
}

gboolean
ogmrip_title_benchmark (OGMRipTitle *title, gboolean *progressive, gboolean *telecine, gboolean *interlaced,
    GCancellable *cancellable, OGMRipTitleCallback callback, gpointer user_data, GError **error)
{
  OGMJobTask *spawn/*, *queue*/;
  OGMRipProgress progress;
  OGMRipBenchmark benchmark;

  gboolean is_open;
  gchar **argv;
  gint result;

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  is_open = ogmrip_title_is_open (title);
  if (!is_open && !ogmrip_title_open (title, cancellable, callback, user_data, error))
    return FALSE;

  progress.title = title;
  progress.callback = callback;
  progress.user_data = user_data;

  memset (&benchmark, 0, sizeof (OGMRipBenchmark));
  benchmark.nframes = 500;

  argv = ogmrip_title_benchmark_command (title, benchmark.nframes);

  spawn = ogmjob_spawn_newv (argv);
  ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (spawn),
      (OGMJobWatch) ogmrip_title_benchmark_watch, &benchmark);

  if (callback)
    g_signal_connect (spawn, "notify::progress",
        G_CALLBACK (ogmrip_title_benchmark_progress_cb), &progress);

  result = ogmjob_task_run (spawn, cancellable, error);
  g_object_unref (spawn);

  if (!result)
  {
    if (!is_open)
      ogmrip_title_close (title);

    return FALSE;
  }

  if (benchmark.nsections > 0)
  {
    /*
     * Progressive
     */
    if (benchmark.cur_section == SECTION_24000_1001 && benchmark.nsections == 1)
    {
      *progressive = TRUE;
      *telecine = FALSE;
      *interlaced = FALSE;
    }
    else if (benchmark.nsections > 1)
    {
      *progressive = TRUE;

      if (benchmark.npatterns > 0 && benchmark.naffinities > 0)
      {
        /*
         * Mixed progressive and telecine
         */
        *telecine = TRUE;
        *interlaced = FALSE;
      }
      else
      {
        /*
         * Mixed progressive and interlaced
         */
        *telecine = FALSE;
        *interlaced = TRUE;
      }
    }
  }
  else
  {
    *telecine = FALSE;
    *progressive = FALSE;
    *interlaced = FALSE;
  }

  g_free (benchmark.prev_affinity);
  g_free (benchmark.cur_affinity);

  return TRUE;
}

typedef struct
{
  GSList *x;
  GSList *y;
  GSList *w;
  GSList *h;
  guint nframes;
  guint frames;
  guint total;
} OGMRipCrop;

static gchar **
ogmrip_title_crop_command (OGMRipTitle *title, gdouble start, gulong nframes)
{
  GPtrArray *argv;
  GString *filter;

  argv = g_ptr_array_new ();

  g_ptr_array_add (argv, g_strdup ("mplayer"));
  g_ptr_array_add (argv, g_strdup ("-nolirc"));
  g_ptr_array_add (argv, g_strdup ("-nosound"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-nosub"));

  g_ptr_array_add (argv, g_strdup ("-noconfig"));
  g_ptr_array_add (argv, g_strdup ("all"));

  g_ptr_array_add (argv, g_strdup ("-vo"));
  g_ptr_array_add (argv, g_strdup ("null"));

  g_ptr_array_add (argv, g_strdup ("-speed"));
  g_ptr_array_add (argv, g_strdup ("100"));

  filter = g_string_new (NULL);

  if (ogmrip_title_get_interlaced (title))
    g_string_append (filter, "yadif=0");

  if (filter->len > 0)
    g_string_append_c (filter, ',');
  g_string_append (filter, "cropdetect");

  g_ptr_array_add (argv, g_strdup ("-vf"));
  g_ptr_array_add (argv, g_string_free (filter, FALSE));

  g_ptr_array_add (argv, g_strdup ("-ss"));
  g_ptr_array_add (argv, g_strdup_printf ("%.0lf", start));

  g_ptr_array_add (argv, g_strdup ("-frames"));
  g_ptr_array_add (argv, g_strdup_printf ("%lu", nframes));

  ogmrip_title_set_input (title, argv);

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gboolean
ogmrip_title_crop_watch (OGMJobTask *spawn, const gchar *buffer, OGMRipCrop *info, GError **error)
{
  gchar *str;

  str = strstr (buffer, "-vf crop=");
  if (str)
  {
    gint x, y, w, h;

    if (sscanf (str, "-vf crop=%d:%d:%d:%d", &w, &h, &x, &y) == 4)
    {
      if (w > 0)
        info->w = g_ulist_add_min (info->w, w);
      if (h > 0)
        info->h = g_ulist_add_min (info->h, h);
      if (x > 0)
        info->x = g_ulist_add_max (info->x, x);
      if (y > 0)
        info->y = g_ulist_add_max (info->y, y);
    }

    info->frames ++;
    if (info->frames < info->nframes - 2)
      ogmjob_task_set_progress (spawn, info->frames / (gdouble) (info->nframes - 2));
    else
      ogmjob_task_set_progress (spawn, 1.0);
  }
  else
  {
    gdouble d;

    if (sscanf (buffer, "V: %lf", &d))
    {
      info->total ++;
/*
      if (info->total >= 100)
        ogmjob_task_cancel (spawn);
*/
    }
  }

  return TRUE;
}

static void
ogmrip_title_crop_progress_cb (OGMJobTask *spawn, GParamSpec *pspec, OGMRipProgress *progress)
{
  progress->callback (progress->title, ogmjob_task_get_progress (spawn), progress->user_data);
}

gboolean
ogmrip_title_crop_detect (OGMRipTitle *title, guint *crop_x, guint *crop_y, guint *crop_w, guint *crop_h,
    GCancellable *cancellable, OGMRipTitleCallback callback, gpointer user_data, GError **error)
{
  OGMRipVideoStream *stream;
  OGMJobTask *spawn, *queue;
  OGMRipProgress progress;
  OGMRipCrop crop;

  guint raw_w, raw_h;
  gdouble length, start, step;
  gboolean is_open;
  gchar **argv;
  gint result;

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  is_open = ogmrip_title_is_open (title);
  if (!is_open && !ogmrip_title_open (title, cancellable, callback, user_data, error))
    return FALSE;

  progress.title = title;
  progress.callback = callback;
  progress.user_data = user_data;

  memset (&crop, 0, sizeof (OGMRipCrop));
  crop.nframes = 12;

  length = ogmrip_title_get_length (title, NULL);
  step = length / 5.;

  queue = ogmjob_queue_new ();

  if (callback)
    g_signal_connect (queue, "notify::progress",
        G_CALLBACK (ogmrip_title_crop_progress_cb), &progress);

  for (start = step; start < length; start += step)
  {
    argv = ogmrip_title_crop_command (title, start, crop.nframes);

    spawn = ogmjob_spawn_newv (argv);
    ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (spawn),
        (OGMJobWatch) ogmrip_title_crop_watch, &crop);

    ogmjob_container_add (OGMJOB_CONTAINER (queue), spawn);
    g_object_unref (spawn);
  }

  result = ogmjob_task_run (queue, cancellable, error);
  g_object_unref (queue);

  if (!result)
  {
    if (!is_open)
      ogmrip_title_close (title);

    return FALSE;
  }

  *crop_x = g_ulist_get_most_frequent (crop.x);
  g_ulist_free (crop.x);

  *crop_y = g_ulist_get_most_frequent (crop.y);
  g_ulist_free (crop.y);

  *crop_w = g_ulist_get_most_frequent (crop.w);
  g_ulist_free (crop.w);

  *crop_h = g_ulist_get_most_frequent (crop.h);
  g_ulist_free (crop.h);

  stream = ogmrip_title_get_video_stream (title);
  ogmrip_video_stream_get_resolution (stream, &raw_w, &raw_h);

  if (*crop_w == 0)
    *crop_w = raw_w;

  if (*crop_h == 0)
    *crop_h = raw_h;

  if (*crop_x + *crop_w > raw_w)
    *crop_x = 0;

  if (*crop_y + *crop_h > raw_h)
    *crop_y = 0;

  if (!is_open)
    ogmrip_title_close (title);

  return TRUE;
}

static GPtrArray *
ogmrip_title_grab_frame_command (OGMRipTitle *title, guint pos, gboolean deint)
{
  GPtrArray *argv;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("mplayer"));
  g_ptr_array_add (argv, g_strdup ("-nolirc"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-nosound"));
  g_ptr_array_add (argv, g_strdup ("-nozoom"));
  g_ptr_array_add (argv, g_strdup ("-nosub"));

  g_ptr_array_add (argv, g_strdup ("-vo"));
  g_ptr_array_add (argv, g_strdup_printf ("jpeg:outdir=%s", ogmrip_fs_get_tmp_dir ()));

  g_ptr_array_add (argv, g_strdup ("-frames"));
  g_ptr_array_add (argv, g_strdup ("1"));

  g_ptr_array_add (argv, g_strdup ("-speed"));
  g_ptr_array_add (argv, g_strdup ("100"));

  if (deint)
  {
    g_ptr_array_add (argv, g_strdup ("-vf"));
    g_ptr_array_add (argv, g_strdup ("pp=lb"));
  }

  g_ptr_array_add (argv, g_strdup ("-ss"));
  g_ptr_array_add (argv, g_strdup_printf ("%u", pos));

  ogmrip_title_set_input (title, argv);

  g_ptr_array_add (argv, NULL);

  return argv;
}

GFile *
ogmrip_title_grab_frame (OGMRipTitle *title, gulong frame, GCancellable *cancellable, GError **error)
{
  GFile *file = NULL;
  OGMJobTask *spawn;
  GPtrArray *argv;

  guint num, denom;

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  ogmrip_video_stream_get_framerate (ogmrip_title_get_video_stream (title), &num, &denom);
  argv = ogmrip_title_grab_frame_command (title, (guint) (frame / (gdouble) num * denom), FALSE);

  spawn = ogmjob_spawn_newv ((gchar **) g_ptr_array_free (argv, FALSE));
  if (ogmjob_task_run (spawn, cancellable, error))
  {
    gchar *filename;

    filename = g_build_filename (ogmrip_fs_get_tmp_dir (), "00000001.jpg", NULL);
    file = g_file_new_for_path (filename);
    g_free (filename);
  }
  g_object_unref (spawn);

  return file;
}

