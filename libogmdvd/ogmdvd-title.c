/* OGMDvd - A wrapper library around libdvdread
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
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
 * SECTION:ogmdvd-title
 * @title: OGMDvdTitle
 * @include: ogmdvd-title.h
 * @short_description: Structure describing a DVD title
 */

#include "ogmdvd-disc.h"
#include "ogmdvd-enums.h"
#include "ogmdvd-title.h"
#include "ogmdvd-stream.h"
#include "ogmdvd-video.h"
#include "ogmdvd-priv.h"

#include "ogmdvd-reader.h"
#include "ogmdvd-parser.h"

#include <ogmjob.h>

#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

typedef struct
{
  OGMDvdTitle *title;
  OGMDvdTitleCallback callback;
  gpointer user_data;
} OGMDvdProgress;

/**
 * ogmdvd_title_ref:
 * @title: An #OGMDvdTitle
 *
 * Increments the reference count of an #OGMDvdTitle.
 */
void
ogmdvd_title_ref (OGMDvdTitle *title)
{
  g_return_if_fail (title != NULL);

  ogmdvd_disc_ref (title->disc);
}

/**
 * ogmdvd_title_unref:
 * @title: An #OGMDvdTitle
 *
 * Decrements the reference count of an #OGMDvdTitle.
 */
void
ogmdvd_title_unref (OGMDvdTitle *title)
{
  g_return_if_fail (title != NULL);

  ogmdvd_disc_unref (title->disc);
}

/**
 * ogmdvd_title_open:
 * @title: An #OGMDvdTitle
 * @error: Location to store the error occuring, or NULL to ignore errors.
 *
 * Opens @title, opening the disc if needed.
 *
 * Returns: #TRUE on success
 *
 */
gboolean
ogmdvd_title_open (OGMDvdTitle *title, GError **error)
{
  g_return_val_if_fail (title != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  title->close_disc = !ogmdvd_disc_is_open (title->disc);

  if (!ogmdvd_disc_open (title->disc, error))
    return FALSE;

  title->vts_file = ifoOpen (title->disc->reader, title->title_set_nr);
  if (!title->vts_file)
  {
    ogmdvd_disc_close (title->disc);
    g_set_error (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_VTS, _("Cannot open video titleset"));
    return FALSE;
  }

  return TRUE;
}

/**
 * ogmdvd_title_close:
 * @title: A #OGMDvdTitle
 *
 * Closes @title.
 */
void
ogmdvd_title_close (OGMDvdTitle *title)
{
  g_return_if_fail (title != NULL);

  if (title->vts_file)
  {
    ifoClose (title->vts_file);
    title->vts_file = NULL;
  }

  if (title->close_disc)
  {
    ogmdvd_disc_close (title->disc);
    title->close_disc = FALSE;
  }
}

/**
 * ogmdvd_title_is_open:
 * @title: An #OGMDvdTitle
 *
 * Returns whether the title is open or not.
 *
 * Returns: TRUE if the title is open
 */
gboolean
ogmdvd_title_is_open (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, FALSE);

  return title->vts_file != NULL;
}

/**
 * ogmdvd_title_get_disc:
 * @title: An #OGMDvdTitle
 *
 * Returns the disc the #OGMDvdTitle was open from.
 *
 * Returns: The #OGMDvdDisc, or NULL
 */
OGMDvdDisc *
ogmdvd_title_get_disc (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, NULL);

  return title->disc;
}

/**
 * ogmdvd_title_get_nr:
 * @title: An #OGMDvdTitle
 *
 * Returns the title number.
 *
 * Returns: The title number, or -1
 */
gint
ogmdvd_title_get_nr (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, -1);

  return title->nr;
}

/**
 * ogmdvd_title_get_ts_nr:
 * @title: An #OGMDvdTitle
 *
 * Returns the titleset number.
 *
 * Returns: The titleset number, or -1
 */
gint
ogmdvd_title_get_ts_nr (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, -1);

  return title->title_set_nr;
}

gboolean
ogmdvd_title_get_progressive (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, FALSE);

  return title->progressive;
}

gboolean
ogmdvd_title_get_telecine (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, FALSE);

  return title->telecine;
}

gboolean
ogmdvd_title_get_interlaced (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, FALSE);

  return title->interlaced;
}

/**
 * ogmdvd_title_get_vts_size:
 * @title: An #OGMDvdTitle
 *
 * Returns the size of the video title set in bytes.
 *
 * Returns: The size in bytes, or -1
 */
gint64
ogmdvd_title_get_vts_size (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, -1);

  return title->vts_size;
}

/**
 * ogmdvd_title_get_length:
 * @title: An #OGMDvdTitle
 * @length: A pointer to set the #OGMDvdTime, or NULL
 *
 * Returns the title length in seconds. If @length is not NULL, the data
 * structure will be filled with the length in hours, minutes seconds and
 * frames.
 *
 * Returns: The length in seconds, or -1.0
 */
gdouble
ogmdvd_title_get_length (OGMDvdTitle *title, OGMDvdTime  *length)
{
  dvd_time_t *dtime = &title->playback_time;

  g_return_val_if_fail (title != NULL, -1.0);

  if (length)
  {
    length->hour   = ((dtime->hour    & 0xf0) >> 4) * 10 + (dtime->hour    & 0x0f);
    length->min    = ((dtime->minute  & 0xf0) >> 4) * 10 + (dtime->minute  & 0x0f);
    length->sec    = ((dtime->second  & 0xf0) >> 4) * 10 + (dtime->second  & 0x0f);
    length->frames = ((dtime->frame_u & 0x30) >> 4) * 10 + (dtime->frame_u & 0x0f);
  }

  return ogmdvd_time_to_msec (dtime) / 1000.0;
}

/**
 * ogmdvd_title_get_chapters_length:
 * @title: An #OGMDvdTitle
 * @start: The start chapter
 * @end: The end chapter
 * @length: A pointer to set the #OGMDvdTime, or NULL
 *
 * Returns the length in seconds between start and end chapters. If @length is
 * not NULL, the data structure will be filled with the length in hours, minutes
 * seconds and frames.
 *
 * Returns: The length in seconds, or -1.0
 */
gdouble
ogmdvd_title_get_chapters_length (OGMDvdTitle *title, guint start, gint end, OGMDvdTime *length)
{
  gulong total;

  g_return_val_if_fail (title != NULL, -1.0);
  g_return_val_if_fail (start < title->nr_of_chapters, -1.0);
  g_return_val_if_fail (end < 0 || start <= end, -1.0);

  if (end < 0)
    end = title->nr_of_chapters - 1;

  if (start == 0 && end + 1 == title->nr_of_chapters)
    return ogmdvd_title_get_length (title, length);

  for (total = 0; start <= end; start ++)
    total += title->length_of_chapters[start];

  if (length)
    ogmdvd_msec_to_time (total, length);

  return total / 1000.0;
}

/**
 * ogmdvd_title_get_palette:
 * @title: An #OGMDvdTitle
 *
 * Returns the palette of the movie.
 *
 * Returns: a constant array of 16 integers, or NULL
 */
const guint *
ogmdvd_title_get_palette (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, NULL);

  return title->palette;
}

/**
 * ogmdvd_title_get_n_angles:
 * @title: An #OGMDvdTitle
 *
 * Returns the number of angles of the video title.
 *
 * Returns: The number of angles, or -1
 */
gint
ogmdvd_title_get_n_angles (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, -1);

  return title->nr_of_angles;
}

/**
 * ogmdvd_title_get_n_chapters:
 * @title: An #OGMDvdTitle
 *
 * Returns the number of chapters of the video title.
 *
 * Returns: The number of chapters, or -1
 */
gint
ogmdvd_title_get_n_chapters (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, -1);

  return title->nr_of_chapters;
}

/**
 * ogmdvd_title_get_video_stream:
 * @title: An #OGMDvdTitle
 *
 * Returns the video stream.
 *
 * Returns: The #OGMDvdVideoStream, or NULL
 */
OGMDvdVideoStream *
ogmdvd_title_get_video_stream (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, NULL);

  return title->video_stream;
}

/**
 * ogmdvd_title_get_n_audio_streams:
 * @title: An #OGMDvdTitle
 *
 * Returns the number of audio streams of the video title.
 *
 * Returns: The number of audio streams, or -1
 */
gint
ogmdvd_title_get_n_audio_streams (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, -1);

  return title->nr_of_audio_streams;
}

static gint
ogmdvd_stream_find_by_nr (OGMDvdStream *stream, guint nr)
{
  return stream->nr - nr;
}

/**
 * ogmdvd_title_get_nth_audio_stream:
 * @title: An #OGMDvdTitle
 * @nr: The audio stream number
 *
 * Returns the audio stream at position nr. The first nr is 0.
 *
 * Returns: The #OGMDvdAudioStream, or NULL
 */
OGMDvdAudioStream *
ogmdvd_title_get_nth_audio_stream (OGMDvdTitle *title, guint nr)
{
  GList *link;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (nr < title->nr_of_audio_streams, NULL);

  link = g_list_find_custom (title->audio_streams, GUINT_TO_POINTER (nr), (GCompareFunc) ogmdvd_stream_find_by_nr);
  if (!link)
    return NULL;

  return link->data;
}

/**
 * ogmdvd_title_get_audio_streams:
 * @title: An #OGMDvdTitle
 *
 * Returns a list of audio stream.
 *
 * Returns: The #GList, or NULL
 */
GList *
ogmdvd_title_get_audio_streams (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, NULL);

  return g_list_copy (title->audio_streams);;
}

/**
 * ogmdvd_title_get_n_subp_streams:
 * @title: An #OGMDvdTitle
 *
 * Returns the number of subtitles streams of the video title.
 *
 * Returns: The number of subtitles streams, or -1
 */
gint
ogmdvd_title_get_n_subp_streams (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, -1);

  return title->nr_of_subp_streams;
}

/**
 * ogmdvd_title_get_nth_subp_stream:
 * @title: An #OGMDvdTitle
 * @nr: The subtitles stream number
 *
 * Returns the subtitles stream at position nr. The first nr is 0.
 *
 * Returns: The #OGMDvdSubpStream, or NULL
 */
OGMDvdSubpStream *
ogmdvd_title_get_nth_subp_stream (OGMDvdTitle *title, guint nr)
{
  GList *link;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (nr < title->nr_of_subp_streams, NULL);

  link = g_list_find_custom (title->subp_streams, GUINT_TO_POINTER (nr), (GCompareFunc) ogmdvd_stream_find_by_nr);
  if (!link)
    return NULL;

  return link->data;
}

/**
 * ogmdvd_title_get_subp_streams:
 * @title: An #OGMDvdTitle
 *
 * Returns a list of subp stream.
 *
 * Returns: The #GList, or NULL
 */
GList *
ogmdvd_title_get_subp_streams (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, NULL);

  return g_list_copy (title->subp_streams);
}

/**
 * ogmdvd_title_analyze:
 * @title: An #OGMDvdTitle
 *
 * Performs a depper analysis of the title to get more information about it and
 * its audio and subtitle streams. This function should be called multiple times
 * until the analysis is complete.
 *
 * Returns: FALSE if the analysis is complete, TRUE otherwise
 */
/*
gboolean
ogmdvd_title_analyze (OGMDvdTitle *title)
{
  gint status;

  g_return_val_if_fail (title != NULL, FALSE);

  if (!title->reader)
    title->reader = ogmdvd_reader_new (title, 0, -1, 0);

  if (!title->reader)
    return FALSE;

  if (!title->parser)
  {
    title->parser = ogmdvd_parser_new (title);
    title->buffer = g_new0 (guchar, 1024 * DVD_VIDEO_LB_LEN);
    title->block_len = 0;
  }

  if (!title->parser)
    return FALSE;

  if (title->block_len > 0)
  {
    title->ptr += DVD_VIDEO_LB_LEN;
    title->block_len --;
  }

  if (!title->block_len)
  {
    title->block_len = ogmdvd_reader_get_block (title->reader, 1024, title->buffer);
    if (title->block_len <= 0)
    {
      /?* ERROR *?/
    }
    title->ptr = title->buffer;
  }

  status = ogmdvd_parser_analyze (title->parser, title->ptr);

  if (status)
  {
    gint i, n;

    n = ogmdvd_title_get_n_audio_streams (title);
    title->bitrates = g_new0 (gint, n);
    for (i = 0; i < n; i ++)
      title->bitrates[i] = ogmdvd_parser_get_audio_bitrate (title->parser, i);

    ogmdvd_parser_unref (title->parser);
    title->parser = NULL;

    ogmdvd_reader_unref (title->reader);
    title->reader = NULL;

    g_free (title->buffer);
    title->buffer = NULL;
    title->ptr = NULL;

    return FALSE;
  }

  return TRUE;
}
*/
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
} OGMDvdAnalyze;

static gchar **
ogmdvd_title_analyze_command (OGMDvdTitle *title, gulong nframes)
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

  device = ogmdvd_disc_get_device (ogmdvd_title_get_disc (title));
  g_ptr_array_add (argv, g_strdup ("-dvd-device"));
  g_ptr_array_add (argv, g_strdup (device));

  vid = ogmdvd_title_get_nr (title);
  g_ptr_array_add (argv, g_strdup_printf ("dvd://%d", vid + 1));

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gdouble
ogmdvd_title_analyze_watch (OGMJobExec *exec, const gchar *buffer, OGMDvdAnalyze *info)
{
  if (g_str_has_prefix (buffer, "V: "))
  {
    info->frames ++;

    if (info->frames == info->nframes)
      return 1.0;

    return info->frames / (gdouble) info->nframes;
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

  return -1.0;
}

typedef struct
{
  GSList *x;
  GSList *y;
  GSList *w;
  GSList *h;
  guint nframes;
  guint frames;
} OGMDvdCrop;

static gchar **
ogmdvd_title_crop_command (OGMDvdTitle *title, gdouble start, gulong nframes)
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

  if (ogmdvd_title_get_interlaced (title))
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

  device = ogmdvd_disc_get_device (ogmdvd_title_get_disc (title));
  g_ptr_array_add (argv, g_strdup ("-dvd-device"));
  g_ptr_array_add (argv, g_strdup (device));

  vid = ogmdvd_title_get_nr (title);
  g_ptr_array_add (argv, g_strdup_printf ("dvd://%d", vid + 1));

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gdouble
ogmdvd_title_crop_watch (OGMJobExec *exec, const gchar *buffer, OGMDvdCrop *info)
{
  gchar *str;

  static guint frame = 0;

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

    frame ++;
    if (frame == info->nframes - 2)
    {
      frame = 0;
      return 1.0;
    }

    return frame / (gdouble) (info->nframes - 2);
  }
  else
  {
    gdouble d;

    if (sscanf (buffer, "V: %lf", &d))
    {
      info->frames ++;

      if (info->frames >= 100)
        ogmjob_spawn_cancel (OGMJOB_SPAWN (exec));
    }
  }

  return -1.0;
}

static void
ogmdvd_title_analyze_progress_cb (OGMJobSpawn *spawn, gdouble fraction, OGMDvdProgress *progress)
{
  progress->callback (progress->title, fraction / 2, progress->user_data);
}

static void
ogmdvd_title_crop_progress_cb (OGMJobSpawn *spawn, gdouble fraction, OGMDvdProgress *progress)
{
  progress->callback (progress->title, 0.5 + fraction / 2, progress->user_data);
}

gboolean
ogmdvd_title_analyze (OGMDvdTitle *title, OGMDvdTitleCallback callback, gpointer user_data, GError **error)
{
  OGMJobSpawn *spawn, *queue;
  OGMDvdProgress progress;
  OGMDvdAnalyze analyze;
  OGMDvdCrop crop;

  gdouble length, start, step;
  gboolean is_open;
  gchar **argv;
  gint result;

  g_return_val_if_fail (title != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (title->analyzed)
    return TRUE;

  is_open = ogmdvd_title_is_open (title);
  if (!is_open && !ogmdvd_title_open (title, error))
    return FALSE;

  progress.title = title;
  progress.callback = callback;
  progress.user_data = user_data;

  memset (&analyze, 0, sizeof (OGMDvdAnalyze));
  analyze.nframes = 500;

  argv = ogmdvd_title_analyze_command (title, analyze.nframes);

  spawn = ogmjob_exec_newv (argv);
  ogmjob_exec_add_watch_full (OGMJOB_EXEC (spawn),
      (OGMJobWatch) ogmdvd_title_analyze_watch, &analyze, TRUE, FALSE, FALSE);

  if (callback)
    g_signal_connect (spawn, "progress",
        G_CALLBACK (ogmdvd_title_analyze_progress_cb), &progress);

  result = ogmjob_spawn_run (spawn, error);
  g_object_unref (spawn);

  if (result != OGMJOB_RESULT_SUCCESS)
  {
    if (!is_open)
      ogmdvd_title_close (title);
    return FALSE;
  }

  if (analyze.nsections > 0)
  {
    /*
     * Progressive
     */
    if (analyze.cur_section == SECTION_24000_1001 && analyze.nsections == 1)
    {
      title->progressive = TRUE;
      title->telecine = FALSE;
      title->interlaced = FALSE;
    }
    else if (analyze.nsections > 1)
    {
      title->progressive = TRUE;
      if (analyze.npatterns > 0 && analyze.naffinities > 0)
      {
        /*
         * Mixed progressive and telecine
         */
        title->telecine = TRUE;
        title->interlaced = FALSE;
      }
      else
      {
        /*
         * Mixed progressive and interlaced
         */
        title->telecine = FALSE;
        title->interlaced = TRUE;
      }
    }
  }
  else
  {
    title->telecine = FALSE;
    title->progressive = FALSE;
    title->interlaced = FALSE;
  }

  g_free (analyze.prev_affinity);
  g_free (analyze.cur_affinity);

  memset (&crop, 0, sizeof (OGMDvdCrop));
  crop.nframes = 12;

  length = ogmdvd_title_get_length (title, NULL);
  step = length / 5.;

  queue = ogmjob_queue_new ();

  if (callback)
    g_signal_connect (queue, "progress",
        G_CALLBACK (ogmdvd_title_crop_progress_cb), &progress);

  for (start = step; start < length; start += step)
  {
    argv = ogmdvd_title_crop_command (title, start, crop.nframes);

    spawn = ogmjob_exec_newv (argv);
    ogmjob_exec_add_watch_full (OGMJOB_EXEC (spawn),
        (OGMJobWatch) ogmdvd_title_crop_watch, &crop, TRUE, FALSE, FALSE);

    ogmjob_container_add (OGMJOB_CONTAINER (queue), spawn);
    g_object_unref (spawn);
  }

  result = ogmjob_spawn_run (queue, error);
  g_object_unref (queue);

  if (result != OGMJOB_RESULT_SUCCESS)
  {
    if (!is_open)
      ogmdvd_title_close (title);
    return FALSE;
  }

  title->video_stream->crop_x = g_ulist_get_most_frequent (crop.x);
  g_ulist_free (crop.x);

  title->video_stream->crop_y = g_ulist_get_most_frequent (crop.y);
  g_ulist_free (crop.y);

  title->video_stream->crop_w = g_ulist_get_most_frequent (crop.w);
  g_ulist_free (crop.w);

  title->video_stream->crop_h = g_ulist_get_most_frequent (crop.h);
  g_ulist_free (crop.h);

  if (!title->video_stream->crop_w)
    ogmdvd_video_stream_get_resolution (title->video_stream, &title->video_stream->crop_w, NULL);

  if (!title->video_stream->crop_h)
    ogmdvd_video_stream_get_resolution (title->video_stream, NULL, &title->video_stream->crop_h);

  ogmdvd_title_close (title);

  title->analyzed = TRUE;

  return TRUE;
}

