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
#include "ogmdvd-priv.h"

#include "ogmdvd-reader.h"
#include "ogmdvd-parser.h"

#include <glib/gi18n-lib.h>

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
 * ogmdvd_title_get_framerate:
 * @title: An #OGMDvdTitle
 * @numerator: A pointer to set the framerate numerator, or NULL
 * @denominator: A pointer to set the framerate denominator, or NULL
 
 * Gets the framerate of the DVD title in the form of a fraction.
 */
void
ogmdvd_title_get_framerate (OGMDvdTitle *title, guint *numerator, guint *denominator)
{
  g_return_if_fail (title != NULL);
  g_return_if_fail (numerator != NULL);
  g_return_if_fail (denominator != NULL);

  switch ((title->playback_time.frame_u & 0xc0) >> 6)
  {
    case 1:
      *numerator = 25;
      *denominator = 1;
      break;
    case 3:
      *numerator = 30000;
      *denominator = 1001;
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

/**
 * ogmdvd_title_get_size:
 * @title: An #OGMDvdTitle
 * @width: A pointer to set the width of the picture, or NULL
 * @height: A pointer to set the height of the picture, or NULL
 
 * Gets the size of the picture.
 */
void
ogmdvd_title_get_size (OGMDvdTitle *title, guint *width, guint *height)
{
  g_return_if_fail (title != NULL);
  g_return_if_fail (width != NULL);
  g_return_if_fail (height != NULL);

  *width = 0;
  *height = 480;
  if (title->video_format != 0)
    *height = 576;

  switch (title->picture_size)
  {
    case 0:
      *width = 720;
      break;
    case 1:
      *width = 704;
      break;
    case 2:
      *width = 352;
      break;
    case 3:
      *width = 352;
      *width /= 2;
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

/**
 * ogmdvd_title_get_aspect_ratio:
 * @title: An #OGMDvdTitle
 * @numerator: A pointer to set the aspect ratio numerator, or NULL
 * @denominator: A pointer to set the aspect ratio denominator, or NULL
 
 * Gets the aspect ratio of the DVD title in the form of a fraction.
 */
void
ogmdvd_title_get_aspect_ratio  (OGMDvdTitle *title, guint *numerator, guint *denominator)
{
  g_return_if_fail (title != NULL);
  g_return_if_fail (numerator != NULL);
  g_return_if_fail (denominator != NULL);

  switch (title->display_aspect_ratio)
  {
    case 0:
      *numerator = 4;
      *denominator = 3;
      break;
    case 1:
    case 3:
      *numerator = 16;
      *denominator = 9;
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

/**
 * ogmdvd_title_get_video_format:
 * @title: An #OGMDvdTitle
 *
 * Returns the video format of the movie.
 *
 * Returns: #OGMDvdVideoFormat, or -1
 */
gint
ogmdvd_title_get_video_format (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, -1);

  return title->video_format;
}

/**
 * ogmdvd_title_get_display_aspect:
 * @title: An #OGMDvdTitle
 *
 * Returns the display aspect of the movie.
 *
 * Returns: #OGMDvdDisplayAspect, or -1
 */
gint
ogmdvd_title_get_display_aspect (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, -1);

  switch (title->display_aspect_ratio)
  {
    case 0:
      return OGMDVD_DISPLAY_ASPECT_4_3;
    case 1:
    case 3:
      return OGMDVD_DISPLAY_ASPECT_16_9;
    default:
      return -1;
  }
}

/**
 * ogmdvd_title_get_display_format:
 * @title: An #OGMDvdTitle
 *
 * Returns the display format of the movie.
 *
 * Returns: #OGMDvdDisplayFormat, or -1
 */
gint
ogmdvd_title_get_display_format (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, -1);

  return title->permitted_df;
}

/**
 * ogmdvd_title_get_palette:
 * @title: An #OGMDvdTitle
 *
 * Returns the palette of the movie.
 *
 * Returns: a constant array of 16 integers, or NULL
 */
G_CONST_RETURN guint *
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
  OGMDvdAudioStream *audio = NULL;
  GSList *link;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (nr < title->nr_of_audio_streams, NULL);

  link = g_slist_find_custom (title->audio_streams, GUINT_TO_POINTER (nr), (GCompareFunc) ogmdvd_stream_find_by_nr);
  if (link)
  {
    audio = link->data;
    ogmdvd_stream_ref (&audio->stream);
  }

  return audio;
}

/**
 * ogmdvd_title_get_audio_streams:
 * @title: An #OGMDvdTitle
 *
 * Returns a list of audio stream.
 *
 * Returns: The #GSList, or NULL
 */
GSList *
ogmdvd_title_get_audio_streams (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, NULL);

  g_slist_foreach (title->audio_streams, (GFunc) ogmdvd_stream_ref, NULL);

  return g_slist_copy (title->audio_streams);;
}

/**
 * ogmdvd_title_find_audio_stream:
 * @title: An #OGMDvdTitle
 * @func: A #GCompareFunc
 * @data: The data to pass to @func
 *
 * Searches for an audio stream with custom criteria.
 *
 * Returns: An #OGMDvdAudioStream, or NULL
 */
OGMDvdAudioStream *
ogmdvd_title_find_audio_stream (OGMDvdTitle *title, GCompareFunc func, gpointer data)
{
  OGMDvdAudioStream *audio = NULL;
  GSList *link;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);

  link = g_slist_find_custom (title->audio_streams, data, func);
  if (link)
  {
    audio = link->data;
    ogmdvd_stream_ref (&audio->stream);
  };

  return audio;
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
  OGMDvdSubpStream *subp = NULL;
  GSList *link;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (nr < title->nr_of_subp_streams, NULL);

  link = g_slist_find_custom (title->subp_streams, GUINT_TO_POINTER (nr), (GCompareFunc) ogmdvd_stream_find_by_nr);
  if (link)
  {
    subp = link->data;
    ogmdvd_stream_ref (&subp->stream);
  }

  return subp;
}

/**
 * ogmdvd_title_get_subp_streams:
 * @title: An #OGMDvdTitle
 *
 * Returns a list of subp stream.
 *
 * Returns: The #GSList, or NULL
 */
GSList *
ogmdvd_title_get_subp_streams (OGMDvdTitle *title)
{
  g_return_val_if_fail (title != NULL, NULL);

  g_slist_foreach (title->subp_streams, (GFunc) ogmdvd_stream_ref, NULL);

  return g_slist_copy (title->subp_streams);
}

/**
 * ogmdvd_title_find_subp_stream:
 * @title: An #OGMDvdTitle
 * @func: A #GCompareFunc
 * @data: The data to pass to @func
 *
 * Searches for a subp stream with custom criteria.
 *
 * Returns: An #OGMDvdSubpStream, or NULL
 */
OGMDvdSubpStream *
ogmdvd_title_find_subp_stream (OGMDvdTitle *title, GCompareFunc func, gpointer data)
{
  OGMDvdSubpStream *subp = NULL;
  GSList *link;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);

  link = g_slist_find_custom (title->subp_streams, data, func);
  if (link)
  {
    subp = link->data;
    ogmdvd_stream_ref (&subp->stream);
  };

  return subp;
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
      /* ERROR */
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

