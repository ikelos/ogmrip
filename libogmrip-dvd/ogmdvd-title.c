/* OGMRipDvd - A DVD library for OGMRip
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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
static void ogmdvd_title_close             (OGMRipTitle                *title);

G_DEFINE_TYPE_WITH_CODE (OGMDvdTitle, ogmdvd_title, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_TITLE, ogmdvd_title_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_STREAM, ogmdvd_stream_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_VIDEO_STREAM, ogmdvd_video_stream_iface_init)
    G_ADD_PRIVATE (OGMDvdTitle));

static void
ogmdvd_title_dispose (GObject *gobject)
{
  OGMDvdTitle *title = OGMDVD_TITLE (gobject);

  if (title->priv->disc)
  {
    g_object_remove_weak_pointer (G_OBJECT (title->priv->disc), (gpointer *) &title->priv->disc);
    title->priv->disc = NULL;
  }

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

#ifdef G_ENABLE_DEBUG
  g_debug ("Finalizing %s (%d)", G_OBJECT_TYPE_NAME (gobject), OGMDVD_TITLE (gobject)->priv->nr);
#endif

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

  G_OBJECT_CLASS (ogmdvd_title_parent_class)->finalize (gobject);
}

static void
ogmdvd_title_init (OGMDvdTitle *title)
{
  title->priv = ogmdvd_title_get_instance_private (title);
}

static void
ogmdvd_title_class_init (OGMDvdTitleClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmdvd_title_dispose;
  gobject_class->finalize = ogmdvd_title_finalize;
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
ogmdvd_video_stream_get_standard (OGMRipVideoStream *video)
{
  return OGMDVD_TITLE (video)->priv->video_format;
}

static gboolean
ogmdvd_video_stream_get_progressive (OGMRipVideoStream *video)
{
  return OGMDVD_TITLE (video)->priv->progressive;
}

static gboolean
ogmdvd_video_stream_get_telecine (OGMRipVideoStream *video)
{
  return OGMDVD_TITLE (video)->priv->telecine;
}

static gboolean
ogmdvd_video_stream_get_interlaced (OGMRipVideoStream *video)
{
  return OGMDVD_TITLE (video)->priv->interlaced;
}

static void
ogmdvd_video_stream_iface_init (OGMRipVideoStreamInterface *iface)
{
  iface->get_framerate    = ogmdvd_video_stream_get_framerate;
  iface->get_resolution   = ogmdvd_video_stream_get_resolution;
  iface->get_crop_size    = ogmdvd_video_stream_get_crop_size;
  iface->get_aspect_ratio = ogmdvd_video_stream_get_aspect_ratio;
  iface->get_standard     = ogmdvd_video_stream_get_standard;
  iface->get_progressive  = ogmdvd_video_stream_get_progressive;
  iface->get_telecine     = ogmdvd_video_stream_get_telecine;
  iface->get_interlaced   = ogmdvd_video_stream_get_interlaced;
}

typedef struct
{
  OGMRipTitle *title;
  OGMRipTitleCallback callback;
  gpointer user_data;
} OGMDvdProgress;

static void
ogmdvd_title_open_cb (OGMRipMedia *media, gdouble percent, gpointer user_data)
{
  OGMDvdProgress *progress = user_data;

  progress->callback (progress->title, percent, progress->user_data);
}

static gboolean
ogmdvd_title_open (OGMRipTitle *title, GCancellable *cancellable, OGMRipTitleCallback callback, gpointer user_data, GError **error)
{
  OGMDvdTitle *dtitle = OGMDVD_TITLE (title);

  if (!dtitle->priv->nopen)
  {
    OGMDvdProgress progress = { title, callback, user_data };

    if (!ogmrip_media_open (dtitle->priv->disc, cancellable, callback ? ogmdvd_title_open_cb : NULL, &progress, error))
      return FALSE;

    dtitle->priv->vts_file = ifoOpen (OGMDVD_DISC (dtitle->priv->disc)->priv->reader, dtitle->priv->title_set_nr);
    if (!dtitle->priv->vts_file)
    {
      ogmrip_media_close (dtitle->priv->disc);
      g_set_error (error, OGMRIP_MEDIA_ERROR, OGMRIP_MEDIA_ERROR_TITLE,
          _("Cannot open title %d of '%s'"), dtitle->priv->nr, OGMDVD_DISC (dtitle->priv->disc)->priv->uri);
      return FALSE;
    }
  }

  dtitle->priv->nopen ++;

  return TRUE;
}

static void
ogmdvd_title_close (OGMRipTitle *title)
{
  OGMDvdTitle *dtitle = OGMDVD_TITLE (title);

  if (dtitle->priv->nopen)
  {
    dtitle->priv->nopen --;

    if (!dtitle->priv->nopen && dtitle->priv->vts_file)
    {
      ifoClose (dtitle->priv->vts_file);
      dtitle->priv->vts_file = NULL;

      ogmrip_media_close (dtitle->priv->disc);
    }
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
ogmdvd_title_get_id (OGMRipTitle *title)
{
  return OGMDVD_TITLE (title)->priv->nr;
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
ogmdvd_title_get_chapters_length (OGMRipTitle *title, guint start, guint end, OGMRipTime *length)
{
  OGMDvdTitle *dtitle = OGMDVD_TITLE (title);
  gulong total;

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

static OGMRipAudioStream *
ogmdvd_title_get_audio_stream (OGMRipTitle *title, guint id)
{
  GList *link;

  for (link = OGMDVD_TITLE (title)->priv->audio_streams; link; link = link->next)
  {
    OGMDvdAudioStream *stream = link->data;

    if (stream->priv->id == id)
      return link->data;
  }

  return NULL;
}

static GList *
ogmdvd_title_get_audio_streams (OGMRipTitle *title)
{
  return g_list_copy (OGMDVD_TITLE (title)->priv->audio_streams);
}

static gint
ogmdvd_title_get_n_subp_streams (OGMRipTitle *title)
{
  return OGMDVD_TITLE (title)->priv->nr_of_subp_streams;
}

static OGMRipSubpStream *
ogmdvd_title_get_subp_stream (OGMRipTitle *title, guint id)
{
  GList *link;

  for (link = OGMDVD_TITLE (title)->priv->subp_streams; link; link = link->next)
  {
    OGMDvdSubpStream *stream = link->data;

    if (stream->priv->id == id)
      return link->data;
  }

  return NULL;
}

static GList *
ogmdvd_title_get_subp_streams (OGMRipTitle *title)
{
  return g_list_copy (OGMDVD_TITLE (title)->priv->subp_streams);
}

typedef struct
{
  OGMRipTitleCallback callback;
  gpointer user_data;
} OGMDvdAnalyze;

static void
ogmdvd_title_benchmark_cb (OGMRipTitle *title, gdouble percent, OGMDvdAnalyze *analyze)
{
  analyze->callback (title, percent / 2.0, analyze->user_data);
}

static void
ogmdvd_title_crop_detect_cb (OGMRipTitle *title, gdouble percent, OGMDvdAnalyze *analyze)
{
  analyze->callback (title, 0.5 + percent / 2.0, analyze->user_data);
}

static gboolean
ogmdvd_title_analyze (OGMRipTitle *title, GCancellable *cancellable, OGMRipTitleCallback callback, gpointer user_data, GError **error)
{
  OGMDvdTitle *dtitle = OGMDVD_TITLE (title);
  OGMDvdAnalyze analyze = { callback, user_data };

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  if (dtitle->priv->analyzed)
    return TRUE;

  if (!ogmrip_title_benchmark (title, &dtitle->priv->progressive, &dtitle->priv->telecine,
        &dtitle->priv->interlaced, cancellable, (OGMRipTitleCallback) ogmdvd_title_benchmark_cb, &analyze, error))
    return FALSE;

  if (!ogmrip_title_crop_detect (title, &dtitle->priv->crop_x, &dtitle->priv->crop_y,
        &dtitle->priv->crop_w, &dtitle->priv->crop_h, cancellable, (OGMRipTitleCallback) ogmdvd_title_crop_detect_cb, &analyze, error))
    return FALSE;

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
  iface->get_id               = ogmdvd_title_get_id;
  iface->get_size             = ogmdvd_title_get_vts_size;
  iface->get_length           = ogmdvd_title_get_length;
  iface->get_chapters_length  = ogmdvd_title_get_chapters_length;
  iface->get_palette          = ogmdvd_title_get_palette;
  iface->get_n_angles         = ogmdvd_title_get_n_angles;
  iface->get_n_chapters       = ogmdvd_title_get_n_chapters;
  iface->get_video_stream     = ogmdvd_title_get_video_stream;
  iface->get_n_audio_streams  = ogmdvd_title_get_n_audio_streams;
  iface->get_audio_stream     = ogmdvd_title_get_audio_stream;
  iface->get_audio_streams    = ogmdvd_title_get_audio_streams;
  iface->get_n_subp_streams   = ogmdvd_title_get_n_subp_streams;
  iface->get_subp_stream      = ogmdvd_title_get_subp_stream;
  iface->get_subp_streams     = ogmdvd_title_get_subp_streams;
  iface->analyze              = ogmdvd_title_analyze;
}

