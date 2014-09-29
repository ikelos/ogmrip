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

#include "ogmrip-bluray-title.h"
#include "ogmrip-bluray-disc.h"
#include "ogmrip-bluray-audio.h"
#include "ogmrip-bluray-subp.h"
#include "ogmrip-bluray-priv.h"

#include <glib/gi18n-lib.h>

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

  G_OBJECT_CLASS (ogmbr_title_parent_class)->dispose (gobject);
}

static void
ogmbr_title_finalize (GObject *gobject)
{
  OGMBrTitle *title = OGMBR_TITLE (gobject);

  g_free (title->priv->chapters);

  G_OBJECT_CLASS (ogmbr_title_parent_class)->finalize (gobject);
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
  gobject_class->finalize = ogmbr_title_finalize;

  g_type_class_add_private (klass, sizeof (OGMBrTitlePriv));
}

static gint
ogmbr_title_get_format (OGMRipStream *stream)
{
  OGMRipFormat format = OGMRIP_FORMAT_UNDEFINED;

  switch (OGMBR_TITLE (stream)->priv->type)
  {
    case BLURAY_STREAM_TYPE_VIDEO_MPEG1:
      format = OGMRIP_FORMAT_MPEG1;
      break;
    case BLURAY_STREAM_TYPE_VIDEO_MPEG2:
      format = OGMRIP_FORMAT_MPEG2;
      break;
    case BLURAY_STREAM_TYPE_VIDEO_VC1:
      format = OGMRIP_FORMAT_VC1;
      break;
    case BLURAY_STREAM_TYPE_VIDEO_H264:
      format = OGMRIP_FORMAT_H264;
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  return format;
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
  iface->get_title  = ogmbr_title_get_title;
}

static void
ogmbr_title_get_aspect_ratio (OGMRipVideoStream *video, guint *numerator, guint *denominator)
{
  guint num, denom;

  switch (OGMBR_TITLE (video)->priv->aspect)
  {
    case BLURAY_ASPECT_RATIO_4_3:
      num = 4;
      denom = 3;
      break;
    case BLURAY_ASPECT_RATIO_16_9:
      num = 16;
      denom = 9;
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  if (numerator)
    *numerator = num;

  if (denominator)
    *denominator = denom;
}

static void
ogmbr_title_get_framerate (OGMRipVideoStream *video, guint *numerator, guint *denominator)
{
  guint num, denom;

  switch (OGMBR_TITLE (video)->priv->rate)
  {
    case BLURAY_VIDEO_RATE_24000_1001:
      num = 24000;
      denom = 1001;
      break;
    case BLURAY_VIDEO_RATE_24:
      num = 24;
      denom = 1;
      break;
    case BLURAY_VIDEO_RATE_25:
      num = 25;
      denom = 1;
      break;
    case BLURAY_VIDEO_RATE_30000_1001:
      num = 30000;
      denom = 1001;
      break;
    case BLURAY_VIDEO_RATE_50:
      num = 50;
      denom = 1;
      break;
    case BLURAY_VIDEO_RATE_60000_1001:
      num = 60000;
      denom = 1001;
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  if (numerator)
    *numerator = num;

  if (denominator)
    *denominator = denom;
}

static void
ogmbr_title_get_resolution (OGMRipVideoStream *video, guint *width, guint *height)
{
  guint w, h;

  switch (OGMBR_TITLE (video)->priv->format)
  {
    case BLURAY_VIDEO_FORMAT_480I:
    case BLURAY_VIDEO_FORMAT_480P:
      w = 720;
      h = 480;
      break;
    case BLURAY_VIDEO_FORMAT_576I:
    case BLURAY_VIDEO_FORMAT_576P:
      w = 720;
      h = 576;
      break;
    case BLURAY_VIDEO_FORMAT_720P:
      w = 1280;
      h = 720;
      break;
    case BLURAY_VIDEO_FORMAT_1080I:
    case BLURAY_VIDEO_FORMAT_1080P:
      w = 1920;
      h = 1080;
      break;
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
  /* TODO get_bitrate */
  return 0;
}

static gboolean
ogmbr_title_get_progressive (OGMRipVideoStream *video)
{
  gboolean progressive;

  switch (OGMBR_TITLE (video)->priv->format)
  {
    case BLURAY_VIDEO_FORMAT_480P:
    case BLURAY_VIDEO_FORMAT_576P:
    case BLURAY_VIDEO_FORMAT_720P:
    case BLURAY_VIDEO_FORMAT_1080P:
      progressive = TRUE;
      break;
    default:
      progressive = FALSE;
      break;
  }

  return progressive;
}

static gboolean
ogmbr_title_get_telecine (OGMRipVideoStream *video)
{
  /* TODO get_telecine */
  return FALSE;
}

static gboolean
ogmbr_title_get_interlaced (OGMRipVideoStream *video)
{
  gboolean interlaced;

  switch (OGMBR_TITLE (video)->priv->format)
  {
    case BLURAY_VIDEO_FORMAT_480I:
    case BLURAY_VIDEO_FORMAT_576I:
    case BLURAY_VIDEO_FORMAT_1080I:
      interlaced = TRUE;
      break;
    default:
      interlaced = FALSE;
      break;
  }

  return interlaced;
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
  iface->get_progressive  = ogmbr_title_get_progressive;
  iface->get_telecine     = ogmbr_title_get_telecine;
  iface->get_interlaced   = ogmbr_title_get_interlaced;
}

static gboolean
ogmbr_title_is_open (OGMRipTitle *title)
{
  OGMRipMedia *media = OGMBR_TITLE (title)->priv->media;

  return ogmrip_media_is_open (media) &&
    OGMBR_DISC (media)->priv->selected == OGMBR_TITLE (title)->priv->id;
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
  OGMBrTitle *btitle = OGMBR_TITLE (title);
  OGMBrDisc *disc = OGMBR_DISC (btitle->priv->media);

  if (!btitle->priv->nopen)
  {
    OpenData data = { title, callback, user_data };
    guint ntitles;

    if (ogmrip_media_is_open (btitle->priv->media))
    {
      if (disc->priv->selected == btitle->priv->id)
      {
        btitle->priv->nopen ++;
        return TRUE;
      }

      if (disc->priv->selected >= 0)
      {
        g_set_error (error, OGMRIP_MEDIA_ERROR, OGMRIP_MEDIA_ERROR_OPEN,
            _("Title already openened"));

        return FALSE;
      }
    }

    if (!ogmrip_media_open (btitle->priv->media, cancellable,
          callback ? (OGMRipMediaCallback) ogmbr_media_open_cb : NULL, &data, error))
      return FALSE;

    ntitles = bd_get_titles (disc->priv->bd, TITLES_RELEVANT, 0);
    if (btitle->priv->id < ntitles && !bd_select_title (disc->priv->bd, btitle->priv->id))
      return FALSE;

    disc->priv->selected = btitle->priv->id;
  }

  btitle->priv->nopen ++;

  return TRUE;
}

static void
ogmbr_title_close (OGMRipTitle *title)
{
  OGMBrTitle *btitle = OGMBR_TITLE (title);

  if (btitle->priv->nopen)
  {
    btitle->priv->nopen --;

    if (!btitle->priv->nopen)
      ogmrip_media_close (OGMBR_TITLE (title)->priv->media);
  }
}

static gint 
ogmbr_title_get_id (OGMRipTitle *title)
{
  return OGMBR_TITLE (title)->priv->id;
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
ogmbr_title_get_n_angles (OGMRipTitle *title)
{
  return OGMBR_TITLE (title)->priv->nangles;
}

static gdouble
ogmbr_title_get_length (OGMRipTitle *title, OGMRipTime *length)
{
  guint64 duration = OGMBR_TITLE (title)->priv->duration / 90;

  if (length)
    ogmrip_msec_to_time (duration, length);

  return duration / 1000.;
}

static gdouble
ogmbr_title_get_chapters_length (OGMRipTitle *title, guint start, guint end, OGMRipTime *length)
{
  OGMBrTitle *btitle = OGMBR_TITLE (title);
  gulong total;

  for (total = 0; start <= end; start ++)
    total += btitle->priv->chapters[start] / 90;

  if (length)
    ogmrip_msec_to_time (total, length);

  return total / 1000.0;
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
  return g_list_length (OGMBR_TITLE (title)->priv->audio_streams);
}

static OGMRipAudioStream *
ogmbr_title_get_audio_stream (OGMRipTitle *title, guint id)
{
  GList *link;

  for (link = OGMBR_TITLE (title)->priv->audio_streams; link; link = link->next)
  {
    OGMBrAudioStream *stream = link->data;

    if (stream->priv->id == id)
      return link->data;
  }

  return NULL;
}

static GList *
ogmbr_title_get_audio_streams (OGMRipTitle *title)
{
  return g_list_copy (OGMBR_TITLE (title)->priv->audio_streams);
}

static gint
ogmbr_title_get_n_subp_streams (OGMRipTitle *title)
{
  return g_list_length (OGMBR_TITLE (title)->priv->subp_streams);
}

static OGMRipSubpStream *
ogmbr_title_get_subp_stream (OGMRipTitle *title, guint id)
{
  GList *link;

  for (link = OGMBR_TITLE (title)->priv->subp_streams; link; link = link->next)
  {
    OGMBrSubpStream *stream = link->data;

    if (stream->priv->id == id)
      return link->data;
  }

  return NULL;
}

static GList *
ogmbr_title_get_subp_streams (OGMRipTitle *title)
{
  return g_list_copy (OGMBR_TITLE (title)->priv->subp_streams);
}

typedef struct
{
  OGMRipTitleCallback callback;
  gpointer user_data;
} OGMBrAnalyze;
/*
static void
ogmbr_title_benchmark_cb (OGMRipTitle *title, gdouble percent, OGMBrAnalyze *analyze)
{
  analyze->callback (title, percent / 2.0, analyze->user_data);
}
*/
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
/*
  if (!ogmrip_title_benchmark (title, &dtitle->priv->progressive, &dtitle->priv->telecine,
        &dtitle->priv->interlaced, cancellable, (OGMRipTitleCallback) ogmbr_title_benchmark_cb, &analyze, error))
    return FALSE;
*/
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
  iface->open                = ogmbr_title_open;
  iface->close               = ogmbr_title_close;
  iface->is_open             = ogmbr_title_is_open;
  iface->get_id              = ogmbr_title_get_id;
  iface->get_media           = ogmbr_title_get_media;
  iface->get_size            = ogmbr_title_get_size;
  iface->get_length          = ogmbr_title_get_length;
  iface->get_n_angles        = ogmbr_title_get_n_angles;
  iface->get_chapters_length = ogmbr_title_get_chapters_length;
  iface->get_n_chapters      = ogmbr_title_get_n_chapters;
  iface->get_video_stream    = ogmbr_title_get_video_stream;
  iface->get_n_audio_streams = ogmbr_title_get_n_audio_streams;
  iface->get_audio_stream    = ogmbr_title_get_audio_stream;
  iface->get_audio_streams   = ogmbr_title_get_audio_streams;
  iface->get_n_subp_streams  = ogmbr_title_get_n_subp_streams;
  iface->get_subp_stream     = ogmbr_title_get_subp_stream;
  iface->get_subp_streams    = ogmbr_title_get_subp_streams;
  iface->analyze             = ogmbr_title_analyze;
}

