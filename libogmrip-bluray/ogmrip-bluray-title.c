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

#include "ogmrip-bluray-audio.h"
#include "ogmrip-bluray-priv.h"
#include "ogmrip-bluray-subp.h"
#include "ogmrip-bluray-title.h"

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

static void ogmbr_stream_iface_init       (OGMRipStreamInterface      *iface);
static void ogmbr_video_stream_iface_init (OGMRipVideoStreamInterface *iface);
static void ogmbr_title_iface_init        (OGMRipTitleInterface       *iface);
static void ogmbr_title_dispose           (GObject                    *gobject);

G_DEFINE_TYPE_WITH_CODE (OGMBrTitle, ogmbr_title, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_TITLE, ogmbr_title_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_STREAM, ogmbr_stream_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_VIDEO_STREAM, ogmbr_video_stream_iface_init));

static void
ogmbr_title_init (OGMBrTitle *title)
{
  title->priv = G_TYPE_INSTANCE_GET_PRIVATE (title, OGMBR_TYPE_TITLE, OGMBrTitlePriv);
}

static void
ogmbr_title_class_init (OGMBrTitleClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmbr_title_dispose;

  g_type_class_add_private (klass, sizeof (OGMBrTitlePriv));
}

static void
ogmbr_title_dispose (GObject *gobject)
{
  OGMBrTitle *title = OGMBR_TITLE (gobject);

  if (title->priv->audio_streams)
  {
    g_list_foreach (title->priv->audio_streams, (GFunc) g_object_unref, NULL);
    g_list_free (title->priv->audio_streams);
    title->priv->audio_streams = NULL;
  }

  if (title->priv->subp_streams)
  {
    g_list_foreach (title->priv->subp_streams, (GFunc) g_object_unref, NULL);
    g_list_free (title->priv->subp_streams);
    title->priv->subp_streams = NULL;
  }

  G_OBJECT_CLASS (ogmbr_title_parent_class)->dispose (gobject);
}
/*
static gint
ogmbr_title_get_format (OGMRipStream *stream)
{
  return OGMRIP_FORMAT_MPEG2;
}
*/
static OGMRipTitle *
ogmbr_title_get_title  (OGMRipStream *stream)
{
  return OGMRIP_TITLE (stream);
}

static void
ogmbr_stream_iface_init (OGMRipStreamInterface *iface)
{
/*
  iface->get_format = ogmbr_title_get_format;
*/
  iface->get_title  = ogmbr_title_get_title;
}

static void
ogmbr_video_stream_get_framerate (OGMRipVideoStream *video, guint *numerator, guint *denominator)
{
  OGMBrTitle *title = OGMBR_TITLE (video);

  if (numerator)
    *numerator = title->priv->rate_num;

  if (denominator)
    *denominator = title->priv->rate_denom;
}

static void
ogmbr_video_stream_get_resolution (OGMRipVideoStream *video, guint *width, guint *height)
{
  OGMBrTitle *title = OGMBR_TITLE (video);

  if (width)
    *width = title->priv->raw_width;;

  if (height)
    *height = title->priv->raw_height;
}
/*
static void
ogmbr_video_stream_get_crop_size (OGMRipVideoStream *video, guint *x, guint *y, guint *width, guint *height)
{
  OGMBrTitle *title = OGMBR_TITLE (video);

  if (x)
    *x = title->priv->crop_x;

  if (y)
    *y = title->priv->crop_y;

  if (width)
    *width = title->priv->crop_w;

  if (height)
    *height = title->priv->crop_h;
}
*/
static void
ogmbr_video_stream_get_aspect_ratio  (OGMRipVideoStream *video, guint *numerator, guint *denominator)
{
  OGMBrTitle *title = OGMBR_TITLE (video);

  if (numerator)
    *numerator = title->priv->aspect_num;

  if (denominator)
    *denominator = title->priv->aspect_denom;
}
/*
static gint
ogmbr_video_stream_get_aspect (OGMRipVideoStream *video)
{
  switch (OGMBR_TITLE (video)->priv->display_aspect_ratio)
  {
    case 0:
      return OGMRIP_ASPECT_4_3;
    case 1:
    case 3:
      return OGMRIP_ASPECT_16_9;
    default:
      return OGMRIP_ASPECT_UNDEFINED;
  }
}

static gint
ogmbr_video_stream_get_standard (OGMRipVideoStream *video)
{
  return OGMBR_TITLE (video)->priv->video_format;
}
*/
static void
ogmbr_video_stream_iface_init (OGMRipVideoStreamInterface *iface)
{
  iface->get_framerate    = ogmbr_video_stream_get_framerate;
  iface->get_resolution   = ogmbr_video_stream_get_resolution;
/*
  iface->get_crop_size    = ogmbr_video_stream_get_crop_size;
*/
  iface->get_aspect_ratio = ogmbr_video_stream_get_aspect_ratio;
/*
  iface->get_aspect       = ogmbr_video_stream_get_aspect;
  iface->get_standard     = ogmbr_video_stream_get_standard;
*/
}
/*
typedef struct
{
  OGMRipTitle *title;
  OGMRipTitleCallback callback;
  gpointer user_data;
} OGMBrProgress;
*/
static OGMRipMedia *
ogmbr_title_get_media (OGMRipTitle *title)
{
  return OGMBR_TITLE (title)->priv->media;
}

static gint
ogmbr_title_get_nr (OGMRipTitle *title)
{
  OGMRipMedia *media = OGMBR_TITLE (title)->priv->media;

  return g_list_index (OGMBR_DISC (media)->priv->titles, title);
}
/*
static gboolean
ogmbr_title_get_progressive (OGMRipTitle *title)
{
  return OGMBR_TITLE (title)->priv->progressive;
}

static gboolean
ogmbr_title_get_telecine (OGMRipTitle *title)
{
  return OGMBR_TITLE (title)->priv->telecine;
}

static gboolean
ogmbr_title_get_interlaced (OGMRipTitle *title)
{
  return OGMBR_TITLE (title)->priv->interlaced;
}
*/
static gint64
ogmbr_title_get_size (OGMRipTitle *title)
{
  return OGMBR_TITLE (title)->priv->size;
}

static gdouble
ogmbr_title_get_length (OGMRipTitle *title, OGMRipTime  *length)
{
  /*
   * TODO OGMRipTitme
   */
  return OGMBR_TITLE (title)->priv->length;
}
/*
static gdouble
ogmbr_title_get_chapters_length (OGMRipTitle *title, guint start, gint end, OGMRipTime *length)
{
  OGMBrTitle *dtitle = OGMBR_TITLE (title);
  gulong total;

  if (end < 0)
    end = dtitle->priv->nr_of_chapters - 1;

  if (start == 0 && end + 1 == dtitle->priv->nr_of_chapters)
    return ogmbr_title_get_length (title, length);

  for (total = 0; start <= end; start ++)
    total += dtitle->priv->length_of_chapters[start];

  if (length)
    ogmrip_msec_to_time (total, length);

  return total / 1000.0;
}

static const guint *
ogmbr_title_get_palette (OGMRipTitle *title)
{
  return OGMBR_TITLE (title)->priv->palette;
}

static gint
ogmbr_title_get_n_angles (OGMRipTitle *title)
{
  return OGMBR_TITLE (title)->priv->nr_of_angles;
}
*/
static gint
ogmbr_title_get_n_chapters (OGMRipTitle *title)
{
  return OGMBR_TITLE (title)->priv->nchapters;
}

static OGMRipVideoStream *
ogmbr_title_get_video_stream (OGMRipTitle *title)
{
  return OGMRIP_VIDEO_STREAM (title);
}

static gint
ogmbr_title_get_n_audio_streams (OGMRipTitle *title)
{
  return OGMBR_TITLE (title)->priv->naudio_streams;
}

static OGMRipAudioStream *
ogmbr_title_get_nth_audio_stream (OGMRipTitle *title, guint nr)
{
  return g_list_nth_data (OGMBR_TITLE (title)->priv->audio_streams, nr);
}

static gint
ogmbr_title_get_n_subp_streams (OGMRipTitle *title)
{
  return OGMBR_TITLE (title)->priv->nsubp_streams;
}

static OGMRipSubpStream *
ogmbr_title_get_nth_subp_stream (OGMRipTitle *title, guint nr)
{
  return g_list_nth_data (OGMBR_TITLE (title)->priv->subp_streams, nr);
}
/*
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
} OGMBrAnalyze;

static gchar **
ogmbr_title_analyze_command (OGMBrTitle *title, gulong nframes)
{
  GPtrArray *argv;

  const gchar *device;
  gint vid;

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

  device = OGMBR_DISC (OGMBR_TITLE (title)->priv->disc)->priv->device;
  g_ptr_array_add (argv, g_strdup ("-dvd-device"));
  g_ptr_array_add (argv, g_strdup (device));

  vid = ogmbr_title_get_nr (OGMRIP_TITLE (title));
  g_ptr_array_add (argv, g_strdup_printf ("dvd://%d", vid + 1));

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gboolean
ogmbr_title_analyze_watch (OGMJobTask *spawn, const gchar *buffer, OGMBrAnalyze *info, GError **error)
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

typedef struct
{
  GSList *x;
  GSList *y;
  GSList *w;
  GSList *h;
  guint nframes;
  guint frames;
  guint total;
} OGMBrCrop;

static gchar **
ogmbr_title_crop_command (OGMBrTitle *title, gdouble start, gulong nframes)
{
  GPtrArray *argv;

  GString *filter;
  const gchar *device;
  gint vid;

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

  if (OGMBR_TITLE (title)->priv->interlaced)
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

  device = OGMBR_DISC (OGMBR_TITLE (title)->priv->disc)->priv->device;
  g_ptr_array_add (argv, g_strdup ("-dvd-device"));
  g_ptr_array_add (argv, g_strdup (device));

  vid = OGMBR_TITLE (title)->priv->nr;
  g_ptr_array_add (argv, g_strdup_printf ("dvd://%d", vid + 1));

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gboolean
ogmbr_title_crop_watch (OGMJobTask *spawn, const gchar *buffer, OGMBrCrop *info, GError **error)
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
    }
  }

  return TRUE;
}

static void
ogmbr_title_analyze_progress_cb (OGMJobTask *spawn, GParamSpec *pspec, OGMBrProgress *progress)
{
  progress->callback (progress->title, ogmjob_task_get_progress (spawn) / 2, progress->user_data);
}

static void
ogmbr_title_crop_progress_cb (OGMJobTask *spawn, GParamSpec *pspec, OGMBrProgress *progress)
{
  progress->callback (progress->title, 0.5 + ogmjob_task_get_progress (spawn) / 2, progress->user_data);
}

static gboolean
ogmbr_title_analyze (OGMRipTitle *title, GCancellable *cancellable, OGMRipTitleCallback callback, gpointer user_data, GError **error)
{
  OGMBrTitle *dtitle = OGMBR_TITLE (title);
  OGMJobTask *spawn, *queue;
  OGMBrProgress progress;
  OGMBrAnalyze analyze;
  OGMBrCrop crop;

  gdouble length, start, step;
  gboolean is_open;
  gchar **argv;
  gint result;

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  if (dtitle->priv->analyzed)
    return TRUE;

  is_open = ogmbr_title_is_open (title);
  if (!is_open && !ogmbr_title_open (title, error))
    return FALSE;

  progress.title = title;
  progress.callback = callback;
  progress.user_data = user_data;

  memset (&analyze, 0, sizeof (OGMBrAnalyze));
  analyze.nframes = 500;

  argv = ogmbr_title_analyze_command (dtitle, analyze.nframes);

  spawn = ogmjob_spawn_newv (argv);
  ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (spawn),
      (OGMJobWatch) ogmbr_title_analyze_watch, &analyze);

  if (callback)
    g_signal_connect (spawn, "notify::progress",
        G_CALLBACK (ogmbr_title_analyze_progress_cb), &progress);

  result = ogmjob_task_run (spawn, cancellable, error);
  g_object_unref (spawn);

  if (!result)
  {
    if (!is_open)
      ogmbr_title_close (title);

    return FALSE;
  }

  if (analyze.nsections > 0)
  {
    /?*
     * Progressive
     *?/
    if (analyze.cur_section == SECTION_24000_1001 && analyze.nsections == 1)
    {
      dtitle->priv->progressive = TRUE;
      dtitle->priv->telecine = FALSE;
      dtitle->priv->interlaced = FALSE;
    }
    else if (analyze.nsections > 1)
    {
      dtitle->priv->progressive = TRUE;
      if (analyze.npatterns > 0 && analyze.naffinities > 0)
      {
        /?*
         * Mixed progressive and telecine
         *?/
        dtitle->priv->telecine = TRUE;
        dtitle->priv->interlaced = FALSE;
      }
      else
      {
        /?*
         * Mixed progressive and interlaced
         *?/
        dtitle->priv->telecine = FALSE;
        dtitle->priv->interlaced = TRUE;
      }
    }
  }
  else
  {
    dtitle->priv->telecine = FALSE;
    dtitle->priv->progressive = FALSE;
    dtitle->priv->interlaced = FALSE;
  }

  g_free (analyze.prev_affinity);
  g_free (analyze.cur_affinity);

  memset (&crop, 0, sizeof (OGMBrCrop));
  crop.nframes = 12;

  length = ogmbr_title_get_length (title, NULL);
  step = length / 5.;

  queue = ogmjob_queue_new ();

  if (callback)
    g_signal_connect (queue, "notify::progress",
        G_CALLBACK (ogmbr_title_crop_progress_cb), &progress);

  for (start = step; start < length; start += step)
  {
    argv = ogmbr_title_crop_command (dtitle, start, crop.nframes);

    spawn = ogmjob_spawn_newv (argv);
    ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (spawn),
        (OGMJobWatch) ogmbr_title_crop_watch, &crop);

    ogmjob_container_add (OGMJOB_CONTAINER (queue), spawn);
    g_object_unref (spawn);
  }

  result = ogmjob_task_run (queue, cancellable, error);
  g_object_unref (queue);

  if (!result)
  {
    if (!is_open)
      ogmbr_title_close (title);

    return FALSE;
  }

  dtitle->priv->crop_x = g_ulist_get_most_frequent (crop.x);
  g_ulist_free (crop.x);

  dtitle->priv->crop_y = g_ulist_get_most_frequent (crop.y);
  g_ulist_free (crop.y);

  dtitle->priv->crop_w = g_ulist_get_most_frequent (crop.w);
  g_ulist_free (crop.w);

  dtitle->priv->crop_h = g_ulist_get_most_frequent (crop.h);
  g_ulist_free (crop.h);

  if (!dtitle->priv->crop_w)
    ogmbr_video_stream_get_resolution (OGMRIP_VIDEO_STREAM (title), &dtitle->priv->crop_w, NULL);

  if (!dtitle->priv->crop_h)
    ogmbr_video_stream_get_resolution (OGMRIP_VIDEO_STREAM (title), NULL, &dtitle->priv->crop_h);

  if (!is_open)
    ogmbr_title_close (title);

  dtitle->priv->analyzed = TRUE;

  return TRUE;
}
*/
static void
ogmbr_title_iface_init (OGMRipTitleInterface *iface)
{
  iface->get_media            = ogmbr_title_get_media;
  iface->get_nr               = ogmbr_title_get_nr;
  iface->get_size             = ogmbr_title_get_size;
  iface->get_length           = ogmbr_title_get_length;
/*
  iface->get_chapters_length  = ogmbr_title_get_chapters_length;
  iface->get_palette          = ogmbr_title_get_palette;
  iface->get_n_angles         = ogmbr_title_get_n_angles;
*/
  iface->get_n_chapters       = ogmbr_title_get_n_chapters;
  iface->get_video_stream     = ogmbr_title_get_video_stream;
  iface->get_n_audio_streams  = ogmbr_title_get_n_audio_streams;
  iface->get_nth_audio_stream = ogmbr_title_get_nth_audio_stream;
  iface->get_n_subp_streams   = ogmbr_title_get_n_subp_streams;
  iface->get_nth_subp_stream  = ogmbr_title_get_nth_subp_stream;
/*
  iface->get_progressive      = ogmbr_title_get_progressive;
  iface->get_telecine         = ogmbr_title_get_telecine;
  iface->get_interlaced       = ogmbr_title_get_interlaced;
  iface->analyze              = ogmbr_title_analyze;
*/
}

