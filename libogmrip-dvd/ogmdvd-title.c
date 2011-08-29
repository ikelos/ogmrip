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

#include "ogmdvd-audio.h"
#include "ogmdvd-disc.h"
#include "ogmdvd-priv.h"
#include "ogmdvd-subp.h"
#include "ogmdvd-title.h"

#include <ogmrip-job.h>

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

static void ogmdvd_stream_iface_init       (OGMRipStreamInterface      *iface);
static void ogmdvd_video_stream_iface_init (OGMRipVideoStreamInterface *iface);
static void ogmdvd_title_iface_init        (OGMRipTitleInterface       *iface);
static void ogmdvd_title_dispose           (GObject                    *gobject);
static void ogmdvd_title_finalize          (GObject                    *gobject);
static void ogmdvd_title_close             (OGMRipTitle                *title);

G_DEFINE_TYPE_WITH_CODE (OGMDvdTitle, ogmdvd_title, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_TITLE, ogmdvd_title_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_STREAM, ogmdvd_stream_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_VIDEO_STREAM, ogmdvd_video_stream_iface_init));

static void
ogmdvd_title_init (OGMDvdTitle *title)
{
  title->priv = G_TYPE_INSTANCE_GET_PRIVATE (title, OGMDVD_TYPE_TITLE, OGMDvdTitlePriv);
}

static void
ogmdvd_title_class_init (OGMDvdTitleClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmdvd_title_dispose;
  gobject_class->finalize = ogmdvd_title_finalize;

  g_type_class_add_private (klass, sizeof (OGMDvdTitlePriv));
}

static void
ogmdvd_title_dispose (GObject *gobject)
{
  OGMDvdTitle *title = OGMDVD_TITLE (gobject);

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

  G_OBJECT_CLASS (ogmdvd_title_parent_class)->dispose (gobject);
}

static void
ogmdvd_title_finalize (GObject *gobject)
{
  OGMDvdTitle *title = OGMDVD_TITLE (gobject);

  ogmdvd_title_close (OGMRIP_TITLE (title));

  if (title->priv->bitrates)
  {
    g_free (title->priv->bitrates);
    title->priv->bitrates = NULL;
  }

  if (title->priv->length_of_chapters)
  {
    g_free (title->priv->length_of_chapters);
    title->priv->length_of_chapters = NULL;
  }

  title->priv->disc = NULL;

  G_OBJECT_CLASS (ogmdvd_title_parent_class)->dispose (gobject);
}

static gint
ogmdvd_title_get_format (OGMRipStream *stream)
{
  return OGMRIP_FORMAT_MPEG2;
}

static OGMRipTitle *
ogmdvd_title_get_title  (OGMRipStream *stream)
{
  return OGMRIP_TITLE (stream);
}


static void
ogmdvd_stream_iface_init (OGMRipStreamInterface *iface)
{
  iface->get_format = ogmdvd_title_get_format;
  iface->get_title  = ogmdvd_title_get_title;
}

static void
ogmdvd_video_stream_get_framerate (OGMRipVideoStream *video, guint *numerator, guint *denominator)
{
  switch ((OGMDVD_TITLE (video)->priv->playback_time.frame_u & 0xc0) >> 6)
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

static void
ogmdvd_video_stream_get_resolution (OGMRipVideoStream *video, guint *width, guint *height)
{
  OGMDvdTitle *title = OGMDVD_TITLE (video);
  guint w, h;

  w = 0;
  h = 480;
  if (title->priv->video_format != 0)
    h = 576;

  switch (title->priv->picture_size)
  {
    case 0:
      w = 720;
      break;
    case 1:
      w = 704;
      break;
    case 2:
      w = 352;
      break;
    case 3:
      w = 352;
      w /= 2;
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  if (width)
    *width = w;

  if (height)
    *height = h;
}

static void
ogmdvd_video_stream_get_crop_size (OGMRipVideoStream *video, guint *x, guint *y, guint *width, guint *height)
{
  OGMDvdTitle *title = OGMDVD_TITLE (video);

  if (x)
    *x = title->priv->crop_x;

  if (y)
    *y = title->priv->crop_y;

  if (width)
    *width = title->priv->crop_w;

  if (height)
    *height = title->priv->crop_h;
}

static void
ogmdvd_video_stream_get_aspect_ratio  (OGMRipVideoStream *video, guint *numerator, guint *denominator)
{
  OGMDvdTitle *title = OGMDVD_TITLE (video);

  switch (title->priv->display_aspect_ratio)
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

static gint
ogmdvd_video_stream_get_display_aspect (OGMRipVideoStream *video)
{
  switch (OGMDVD_TITLE (video)->priv->display_aspect_ratio)
  {
    case 0:
      return OGMRIP_DISPLAY_ASPECT_4_3;
    case 1:
    case 3:
      return OGMRIP_DISPLAY_ASPECT_16_9;
    default:
      return OGMRIP_DISPLAY_ASPECT_UNDEFINED;
  }
}

static gint
ogmdvd_video_stream_get_display_format (OGMRipVideoStream *video)
{
  return OGMDVD_TITLE (video)->priv->video_format;
}

static void
ogmdvd_video_stream_iface_init (OGMRipVideoStreamInterface *iface)
{
  iface->get_framerate      = ogmdvd_video_stream_get_framerate;
  iface->get_resolution     = ogmdvd_video_stream_get_resolution;
  iface->get_crop_size      = ogmdvd_video_stream_get_crop_size;
  iface->get_aspect_ratio   = ogmdvd_video_stream_get_aspect_ratio;
  iface->get_display_aspect = ogmdvd_video_stream_get_display_aspect;
  iface->get_display_format = ogmdvd_video_stream_get_display_format;
}

typedef struct
{
  OGMRipTitle *title;
  OGMRipTitleCallback callback;
  gpointer user_data;
} OGMDvdProgress;

static gboolean
ogmdvd_title_open (OGMRipTitle *title, GError **error)
{
  OGMDvdTitle *dtitle = OGMDVD_TITLE (title);

  dtitle->priv->close_disc = !ogmrip_media_is_open (OGMRIP_MEDIA (dtitle->priv->disc));

  if (!ogmrip_media_open (OGMRIP_MEDIA (dtitle->priv->disc), error))
    return FALSE;

  dtitle->priv->vts_file = ifoOpen (OGMDVD_DISC (dtitle->priv->disc)->priv->reader, dtitle->priv->title_set_nr);
  if (!dtitle->priv->vts_file)
  {
    ogmrip_media_close (OGMRIP_MEDIA (dtitle->priv->disc));
    g_set_error (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_VTS, _("Cannot open video titleset"));
    return FALSE;
  }

  return TRUE;
}

static void
ogmdvd_title_close (OGMRipTitle *title)
{
  OGMDvdTitle *dtitle = OGMDVD_TITLE (title);

  if (dtitle->priv->vts_file)
  {
    ifoClose (dtitle->priv->vts_file);
    dtitle->priv->vts_file = NULL;
  }

  if (dtitle->priv->close_disc)
  {
    ogmrip_media_close (OGMRIP_MEDIA (dtitle->priv->disc));
    dtitle->priv->close_disc = FALSE;
  }
}

static gboolean
ogmdvd_title_is_open (OGMRipTitle *title)
{
  return OGMDVD_TITLE (title)->priv->vts_file != NULL;
}

static OGMRipMedia *
ogmdvd_title_get_media (OGMRipTitle *title)
{
  return OGMRIP_MEDIA (OGMDVD_TITLE (title)->priv->disc);
}

static gint
ogmdvd_title_get_nr (OGMRipTitle *title)
{
  return OGMDVD_TITLE (title)->priv->nr;
}

static gint
ogmdvd_title_get_ts_nr (OGMRipTitle *title)
{
  return OGMDVD_TITLE (title)->priv->title_set_nr;
}

static gboolean
ogmdvd_title_get_progressive (OGMRipTitle *title)
{
  return OGMDVD_TITLE (title)->priv->progressive;
}

static gboolean
ogmdvd_title_get_telecine (OGMRipTitle *title)
{
  return OGMDVD_TITLE (title)->priv->telecine;
}

static gboolean
ogmdvd_title_get_interlaced (OGMRipTitle *title)
{
  return OGMDVD_TITLE (title)->priv->interlaced;
}

static gint64
ogmdvd_title_get_vts_size (OGMRipTitle *title)
{
  return OGMDVD_TITLE (title)->priv->vts_size;
}

static gdouble
ogmdvd_title_get_length (OGMRipTitle *title, OGMRipTime  *length)
{
  dvd_time_t *dtime = &OGMDVD_TITLE (title)->priv->playback_time;

  if (length)
  {
    gulong frames;

    length->hour   = ((dtime->hour    & 0xf0) >> 4) * 10 + (dtime->hour    & 0x0f);
    length->min    = ((dtime->minute  & 0xf0) >> 4) * 10 + (dtime->minute  & 0x0f);
    length->sec    = ((dtime->second  & 0xf0) >> 4) * 10 + (dtime->second  & 0x0f);

    frames = ((dtime->frame_u & 0x30) >> 4) * 10 + (dtime->frame_u & 0x0f);

    switch ((dtime->frame_u & 0xc0) >> 6)
    {
      case 1:
        length->msec = frames * 40;
        break;
      case 3:
        length->msec = frames * 1001 / 30;
        break;
      default:
        g_assert_not_reached ();
        break;
    }
  }

  return ogmdvd_time_to_msec (dtime) / 1000.0;
}

static gdouble
ogmdvd_title_get_chapters_length (OGMRipTitle *title, guint start, gint end, OGMRipTime *length)
{
  OGMDvdTitle *dtitle = OGMDVD_TITLE (title);
  gulong total;

  if (end < 0)
    end = dtitle->priv->nr_of_chapters - 1;

  if (start == 0 && end + 1 == dtitle->priv->nr_of_chapters)
    return ogmdvd_title_get_length (title, length);

  for (total = 0; start <= end; start ++)
    total += dtitle->priv->length_of_chapters[start];

  if (length)
    ogmrip_msec_to_time (total, length);

  return total / 1000.0;
}

static const guint *
ogmdvd_title_get_palette (OGMRipTitle *title)
{
  return OGMDVD_TITLE (title)->priv->palette;
}

static gint
ogmdvd_title_get_n_angles (OGMRipTitle *title)
{
  return OGMDVD_TITLE (title)->priv->nr_of_angles;
}

static gint
ogmdvd_title_get_n_chapters (OGMRipTitle *title)
{
  return OGMDVD_TITLE (title)->priv->nr_of_chapters;
}

static OGMRipVideoStream *
ogmdvd_title_get_video_stream (OGMRipTitle *title)
{
  return OGMRIP_VIDEO_STREAM (title);
}

static gint
ogmdvd_title_get_n_audio_streams (OGMRipTitle *title)
{
  return OGMDVD_TITLE (title)->priv->nr_of_audio_streams;
}

static gint
ogmdvd_audio_stream_find_by_nr (OGMDvdAudioStream *audio, guint nr)
{
  return audio->priv->nr - nr;
}

static OGMRipAudioStream *
ogmdvd_title_get_nth_audio_stream (OGMRipTitle *title, guint nr)
{
  GList *link;

  link = g_list_find_custom (OGMDVD_TITLE (title)->priv->audio_streams,
      GUINT_TO_POINTER (nr), (GCompareFunc) ogmdvd_audio_stream_find_by_nr);
  if (!link)
    return NULL;

  return link->data;
}

static gint
ogmdvd_title_get_n_subp_streams (OGMRipTitle *title)
{
  return OGMDVD_TITLE (title)->priv->nr_of_subp_streams;
}

static gint
ogmdvd_subp_stream_find_by_nr (OGMDvdSubpStream *audio, guint nr)
{
  return audio->priv->nr - nr;
}

static OGMRipSubpStream *
ogmdvd_title_get_nth_subp_stream (OGMRipTitle *title, guint nr)
{
  GList *link;

  link = g_list_find_custom (OGMDVD_TITLE (title)->priv->subp_streams,
      GUINT_TO_POINTER (nr), (GCompareFunc) ogmdvd_subp_stream_find_by_nr);
  if (!link)
    return NULL;

  return link->data;
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

  device = OGMDVD_DISC (OGMDVD_TITLE (title)->priv->disc)->priv->device;
  g_ptr_array_add (argv, g_strdup ("-dvd-device"));
  g_ptr_array_add (argv, g_strdup (device));

  vid = ogmdvd_title_get_nr (OGMRIP_TITLE (title));
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

  if (OGMDVD_TITLE (title)->priv->interlaced)
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

  device = OGMDVD_DISC (OGMDVD_TITLE (title)->priv->disc)->priv->device;
  g_ptr_array_add (argv, g_strdup ("-dvd-device"));
  g_ptr_array_add (argv, g_strdup (device));

  vid = OGMDVD_TITLE (title)->priv->nr;
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

static void
ogmdvd_title_analyze_cancel_cb (GCancellable *cancellable, OGMJobSpawn *spawn)
{
  ogmjob_spawn_cancel (spawn);
}

static gboolean
ogmdvd_title_analyze (OGMRipTitle *title, GCancellable *cancellable, OGMRipTitleCallback callback, gpointer user_data, GError **error)
{
  OGMDvdTitle *dtitle = OGMDVD_TITLE (title);
  OGMJobSpawn *spawn, *queue;
  OGMDvdProgress progress;
  OGMDvdAnalyze analyze;
  OGMDvdCrop crop;

  gulong handler_id;
  gdouble length, start, step;
  gboolean is_open;
  gchar **argv;
  gint result;

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  if (dtitle->priv->analyzed)
    return TRUE;

  is_open = ogmdvd_title_is_open (title);
  if (!is_open && !ogmdvd_title_open (title, error))
    return FALSE;

  progress.title = title;
  progress.callback = callback;
  progress.user_data = user_data;

  memset (&analyze, 0, sizeof (OGMDvdAnalyze));
  analyze.nframes = 500;

  argv = ogmdvd_title_analyze_command (dtitle, analyze.nframes);

  spawn = ogmjob_exec_newv (argv);
  ogmjob_exec_add_watch_full (OGMJOB_EXEC (spawn),
      (OGMJobWatch) ogmdvd_title_analyze_watch, &analyze, TRUE, FALSE, FALSE);

  handler_id = cancellable ? g_cancellable_connect (cancellable,
      G_CALLBACK (ogmdvd_title_analyze_cancel_cb), spawn, NULL) : 0;

  if (callback)
    g_signal_connect (spawn, "progress",
        G_CALLBACK (ogmdvd_title_analyze_progress_cb), &progress);

  result = ogmjob_spawn_run (spawn, error);
  g_object_unref (spawn);

  if (handler_id)
    g_cancellable_disconnect (cancellable, handler_id);

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
      dtitle->priv->progressive = TRUE;
      dtitle->priv->telecine = FALSE;
      dtitle->priv->interlaced = FALSE;
    }
    else if (analyze.nsections > 1)
    {
      dtitle->priv->progressive = TRUE;
      if (analyze.npatterns > 0 && analyze.naffinities > 0)
      {
        /*
         * Mixed progressive and telecine
         */
        dtitle->priv->telecine = TRUE;
        dtitle->priv->interlaced = FALSE;
      }
      else
      {
        /*
         * Mixed progressive and interlaced
         */
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

  memset (&crop, 0, sizeof (OGMDvdCrop));
  crop.nframes = 12;

  length = ogmdvd_title_get_length (title, NULL);
  step = length / 5.;

  queue = ogmjob_queue_new ();

  handler_id = cancellable ? g_cancellable_connect (cancellable,
      G_CALLBACK (ogmdvd_title_analyze_cancel_cb), spawn, NULL) : 0;

  if (callback)
    g_signal_connect (queue, "progress",
        G_CALLBACK (ogmdvd_title_crop_progress_cb), &progress);

  for (start = step; start < length; start += step)
  {
    argv = ogmdvd_title_crop_command (dtitle, start, crop.nframes);

    spawn = ogmjob_exec_newv (argv);
    ogmjob_exec_add_watch_full (OGMJOB_EXEC (spawn),
        (OGMJobWatch) ogmdvd_title_crop_watch, &crop, TRUE, FALSE, FALSE);

    ogmjob_container_add (OGMJOB_CONTAINER (queue), spawn);
    g_object_unref (spawn);
  }

  result = ogmjob_spawn_run (queue, error);
  g_object_unref (queue);

  if (handler_id)
    g_cancellable_disconnect (cancellable, handler_id);

  if (result != OGMJOB_RESULT_SUCCESS)
  {
    if (!is_open)
      ogmdvd_title_close (title);
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
    ogmdvd_video_stream_get_resolution (OGMRIP_VIDEO_STREAM (title), &dtitle->priv->crop_w, NULL);

  if (!dtitle->priv->crop_h)
    ogmdvd_video_stream_get_resolution (OGMRIP_VIDEO_STREAM (title), NULL, &dtitle->priv->crop_h);

  if (!is_open)
    ogmdvd_title_close (title);

  dtitle->priv->analyzed = TRUE;

  return TRUE;
}

static void
ogmdvd_title_iface_init (OGMRipTitleInterface *iface)
{
  iface->open                 = ogmdvd_title_open;
  iface->close                = ogmdvd_title_close;
  iface->is_open              = ogmdvd_title_is_open;
  iface->get_media            = ogmdvd_title_get_media;
  iface->get_nr               = ogmdvd_title_get_nr;
  iface->get_ts_nr            = ogmdvd_title_get_ts_nr;
  iface->get_size             = ogmdvd_title_get_vts_size;
  iface->get_length           = ogmdvd_title_get_length;
  iface->get_chapters_length  = ogmdvd_title_get_chapters_length;
  iface->get_palette          = ogmdvd_title_get_palette;
  iface->get_n_angles         = ogmdvd_title_get_n_angles;
  iface->get_n_chapters       = ogmdvd_title_get_n_chapters;
  iface->get_video_stream     = ogmdvd_title_get_video_stream;
  iface->get_n_audio_streams  = ogmdvd_title_get_n_audio_streams;
  iface->get_nth_audio_stream = ogmdvd_title_get_nth_audio_stream;
  iface->get_n_subp_streams   = ogmdvd_title_get_n_subp_streams;
  iface->get_nth_subp_stream  = ogmdvd_title_get_nth_subp_stream;
  iface->get_progressive      = ogmdvd_title_get_progressive;
  iface->get_telecine         = ogmdvd_title_get_telecine;
  iface->get_interlaced       = ogmdvd_title_get_interlaced;
  iface->analyze              = ogmdvd_title_analyze;
}

