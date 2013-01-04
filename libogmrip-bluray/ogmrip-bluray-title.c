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

#include "ogmrip-bluray-title.h"
#include "ogmrip-bluray-disc.h"
#include "ogmrip-bluray-priv.h"

static void ogmbr_stream_iface_init       (OGMRipStreamInterface      *iface);
static void ogmbr_video_stream_iface_init (OGMRipVideoStreamInterface *iface);
static void ogmbr_title_iface_init        (OGMRipTitleInterface       *iface);

G_DEFINE_TYPE_WITH_CODE (OGMBrTitle, ogmbr_title, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_TITLE, ogmbr_title_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_STREAM, ogmbr_stream_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_VIDEO_STREAM, ogmbr_video_stream_iface_init));

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

  g_list_free (title->priv->streams);
  title->priv->streams = NULL;

  G_OBJECT_CLASS (ogmbr_title_parent_class)->dispose (gobject);
}

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

static gint
ogmbr_title_get_format (OGMRipStream *stream)
{
  return OGMBR_TITLE (stream)->priv->format;
}

static gint
ogmbr_title_get_id (OGMRipStream *stream)
{
  return OGMBR_TITLE (stream)->priv->id;
}

static OGMRipTitle *
ogmbr_title_get_title (OGMRipStream *stream)
{
  return OGMRIP_TITLE (stream);
}

static void
ogmbr_stream_iface_init (OGMRipStreamInterface *iface)
{
  iface->get_format = ogmbr_title_get_format;
  iface->get_id     = ogmbr_title_get_id;
  iface->get_title  = ogmbr_title_get_title;
}

static void
ogmbr_title_get_aspect_ratio (OGMRipVideoStream *video, guint *numerator, guint *denominator)
{
  OGMBrTitle *title = OGMBR_TITLE (video);

  if (numerator)
    *numerator = title->priv->aspect_num;

  if (denominator)
    *denominator = title->priv->aspect_denom;
}

static void
ogmbr_title_get_framerate (OGMRipVideoStream *video, guint *numerator, guint *denominator)
{
  OGMBrTitle *title = OGMBR_TITLE (video);

  if (numerator)
    *numerator = title->priv->rate_num;

  if (denominator)
    *denominator = title->priv->rate_denom;
}

static void
ogmbr_title_get_resolution (OGMRipVideoStream *video, guint *width, guint *height)
{
  OGMBrTitle *title = OGMBR_TITLE (video);

  if (width)
    *width = title->priv->raw_width;

  if (height)
    *height = title->priv->raw_height;
}

static void
ogmbr_title_get_crop_size (OGMRipVideoStream *video, guint *x, guint *y, guint *width, guint *height)
{
  OGMBrTitle *title = OGMBR_TITLE (video);

  if (x)
    *x = title->priv->crop_x;

  if (y)
    *y = title->priv->crop_y;

  if (width)
    *width = title->priv->crop_width;

  if (height)
    *height = title->priv->crop_height;
}

static gint
ogmbr_title_get_bitrate (OGMRipVideoStream *video)
{
  return OGMBR_TITLE (video)->priv->bitrate;
}

static void
ogmbr_video_stream_iface_init (OGMRipVideoStreamInterface *iface)
{
  iface->get_aspect_ratio = ogmbr_title_get_aspect_ratio;
  iface->get_framerate    = ogmbr_title_get_framerate;
  iface->get_resolution   = ogmbr_title_get_resolution;
  iface->get_crop_size    = ogmbr_title_get_crop_size;
  iface->get_aspect_ratio = ogmbr_title_get_aspect_ratio;
  iface->get_bitrate      = ogmbr_title_get_bitrate;
}

typedef struct
{
  OGMRipTitle *title;
  OGMRipTitleCallback callback;
  gpointer user_data;
} OpenData;

static void
ogmbr_media_open_cb (OGMRipMedia *media, gdouble percent, OpenData *data)
{
  data->callback (data->title, percent, data->user_data);
}

static gboolean
ogmbr_title_open (OGMRipTitle *title, GCancellable *cancellable, OGMRipTitleCallback callback, gpointer user_data, GError **error)
{
  OGMRipMedia *media = OGMBR_TITLE (title)->priv->media;
  OpenData data;

  data.title = title;
  data.callback = callback;
  data.user_data = user_data;

  return ogmrip_media_open (media, cancellable, callback ? (OGMRipMediaCallback) ogmbr_media_open_cb : NULL, &data, error);
}

static gboolean
ogmbr_title_is_open (OGMRipTitle *title)
{
  OGMRipMedia *media = OGMBR_TITLE (title)->priv->media;

  return OGMBR_DISC (media)->priv->is_open;
}

static OGMRipMedia *
ogmbr_title_get_media (OGMRipTitle *title)
{
  return OGMBR_TITLE (title)->priv->media;
}

static gint64
ogmbr_title_get_size (OGMRipTitle *title)
{
  return OGMBR_TITLE (title)->priv->size;
}

static gint
ogmbr_title_get_nr (OGMRipTitle *title)
{
  OGMRipMedia *media = OGMBR_TITLE (title)->priv->media;

  return g_list_index (OGMBR_DISC (media)->priv->titles, title);
}

static gdouble
ogmbr_title_get_length (OGMRipTitle *title, OGMRipTime *length)
{
  gdouble val = OGMBR_TITLE (title)->priv->length;

  if (length)
    ogmrip_msec_to_time (val * 1000, length);

  return val;
}

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

typedef struct
{
  OGMRipTitleCallback callback;
  gpointer user_data;
} OGMBrAnalyze;

static void
ogmbr_title_benchmark_cb (OGMRipTitle *title, gdouble percent, OGMBrAnalyze *analyze)
{
  analyze->callback (title, percent / 2.0, analyze->user_data);
}

static void
ogmbr_title_crop_detect_cb (OGMRipTitle *title, gdouble percent, OGMBrAnalyze *analyze)
{
  analyze->callback (title, 0.5 + percent / 2.0, analyze->user_data);
}

static gboolean
ogmbr_title_analyze (OGMRipTitle *title, GCancellable *cancellable, OGMRipTitleCallback callback, gpointer user_data, GError **error)
{
  OGMBrTitle *dtitle = OGMBR_TITLE (title);
  OGMBrAnalyze analyze = { callback, user_data };

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  if (dtitle->priv->analyzed)
    return TRUE;

  if (!ogmrip_title_benchmark (title, &dtitle->priv->progressive, &dtitle->priv->telecine,
        &dtitle->priv->interlaced, cancellable, (OGMRipTitleCallback) ogmbr_title_benchmark_cb, &analyze, error))
    return FALSE;

  if (!ogmrip_title_crop_detect (title, &dtitle->priv->crop_x, &dtitle->priv->crop_y,
        &dtitle->priv->crop_width, &dtitle->priv->crop_height, cancellable,
        (OGMRipTitleCallback) ogmbr_title_crop_detect_cb, &analyze, error))
    return FALSE;

  dtitle->priv->analyzed = TRUE;

  return TRUE;
}

static void
ogmbr_title_iface_init (OGMRipTitleInterface *iface)
{
  iface->open                 = ogmbr_title_open;
  iface->is_open              = ogmbr_title_is_open;
  iface->get_media            = ogmbr_title_get_media;
  iface->get_size             = ogmbr_title_get_size;
  iface->get_nr               = ogmbr_title_get_nr;
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
  iface->get_progressive      = ogmbr_title_get_progressive;
  iface->get_telecine         = ogmbr_title_get_telecine;
  iface->get_interlaced       = ogmbr_title_get_interlaced;
  iface->analyze              = ogmbr_title_analyze;
}

