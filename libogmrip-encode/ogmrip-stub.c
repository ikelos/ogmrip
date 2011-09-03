/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-stub.h"
#include "ogmrip-plugin.h"
#include "ogmrip-video-codec.h"
#include "ogmrip-audio-codec.h"
#include "ogmrip-subp-codec.h"
#include "ogmrip-chapters.h"

#include <glib/gstdio.h>

struct _OGMRipStubPriv
{
  OGMRipCodec *codec;
};

enum
{
  PROP_0,
  PROP_CODEC
};

static gint64
ogmrip_codec_get_size (OGMRipCodec *codec)
{
  const gchar *output;
  gint64 size = 0;

  output = ogmrip_file_get_path (ogmrip_codec_get_output (codec));
  if (g_file_test (output, G_FILE_TEST_IS_REGULAR))
  {
    struct stat buf;

    if (g_stat (output, &buf) == 0)
      size = buf.st_size;
  }

  return size;
}

/*
 * Video stub
 */

static gint64
ogmrip_video_stub_get_media_size (OGMRipMedia *media)
{
  OGMRipVideoStub *stub = OGMRIP_VIDEO_STUB (media);

  return ogmrip_codec_get_size (stub->priv->codec);
}

static void
ogmrip_video_media_iface_init (OGMRipMediaInterface *iface)
{
  iface->get_size = ogmrip_video_stub_get_media_size;
}

static gdouble
ogmrip_video_stub_get_length (OGMRipTitle *title, OGMRipTime *time_)
{
  OGMRipVideoStub *stub = OGMRIP_VIDEO_STUB (title);

  return ogmrip_codec_get_length (stub->priv->codec, time_);
}

static gint64
ogmrip_video_stub_get_title_size (OGMRipTitle *title)
{
  OGMRipVideoStub *stub = OGMRIP_VIDEO_STUB (title);

  return ogmrip_codec_get_size (stub->priv->codec);
}

static void
ogmrip_video_title_iface_init (OGMRipTitleInterface *iface)
{
  iface->get_length = ogmrip_video_stub_get_length;
  iface->get_size = ogmrip_video_stub_get_title_size;
}

static gint
ogmrip_video_stub_get_format (OGMRipStream *stream)
{
  OGMRipVideoStub *stub = OGMRIP_VIDEO_STUB (stream);

  return ogmrip_plugin_get_video_codec_format (G_OBJECT_TYPE (stub->priv->codec));
}

static void
ogmrip_video_stream_iface_init (OGMRipStreamInterface *iface)
{
  iface->get_format = ogmrip_video_stub_get_format;
}

static void
ogmrip_video_stub_get_aspect_ratio (OGMRipVideoStream *video, guint *n, guint *d)
{
  OGMRipVideoStub *stub = OGMRIP_VIDEO_STUB (video);

  ogmrip_video_codec_get_aspect_ratio (OGMRIP_VIDEO_CODEC (stub->priv->codec), n, d);
}

static gint
ogmrip_video_stub_get_bitrate (OGMRipVideoStream *video)
{
  OGMRipVideoStub *stub = OGMRIP_VIDEO_STUB (video);

  return ogmrip_video_codec_get_bitrate (OGMRIP_VIDEO_CODEC (stub->priv->codec));
}

static void
ogmrip_video_stub_get_crop_size (OGMRipVideoStream *video, guint *x, guint *y, guint *w, guint *h)
{
  OGMRipVideoStub *stub = OGMRIP_VIDEO_STUB (video);

  ogmrip_video_codec_get_crop_size (OGMRIP_VIDEO_CODEC (stub->priv->codec), x, y, w, h);
}

static void
ogmrip_video_stub_get_framerate (OGMRipVideoStream *video, guint *n, guint *d)
{
  OGMRipVideoStub *stub = OGMRIP_VIDEO_STUB (video);

  ogmrip_video_codec_get_framerate (OGMRIP_VIDEO_CODEC (stub->priv->codec), n, d);
}

static void
ogmrip_video_stub_get_resolution (OGMRipVideoStream *video, guint *w, guint *h)
{
  OGMRipVideoStub *stub = OGMRIP_VIDEO_STUB (video);

  ogmrip_video_codec_get_scale_size (OGMRIP_VIDEO_CODEC (stub->priv->codec), w, h);
}

static void
ogmrip_video_iface_init (OGMRipVideoStreamInterface *iface)
{
  iface->get_aspect_ratio = ogmrip_video_stub_get_aspect_ratio;
  iface->get_bitrate = ogmrip_video_stub_get_bitrate;
  iface->get_crop_size = ogmrip_video_stub_get_crop_size;
  iface->get_framerate = ogmrip_video_stub_get_framerate;
  iface->get_resolution = ogmrip_video_stub_get_resolution;
}

G_DEFINE_TYPE_WITH_CODE (OGMRipVideoStub, ogmrip_video_stub, OGMRIP_TYPE_VIDEO_FILE,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_MEDIA, ogmrip_video_media_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_TITLE, ogmrip_video_title_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_STREAM, ogmrip_video_stream_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_VIDEO_STREAM, ogmrip_video_iface_init));

static void
ogmrip_video_stub_init (OGMRipVideoStub *stub)
{
  stub->priv = G_TYPE_INSTANCE_GET_PRIVATE (stub, OGMRIP_TYPE_VIDEO_STUB, OGMRipStubPriv);
}

static void
ogmrip_video_stub_dispose (GObject *gobject)
{
  OGMRipVideoStub *stub = OGMRIP_VIDEO_STUB (gobject);

  if (stub->priv->codec)
  {
    g_object_unref (stub->priv->codec);
    stub->priv->codec = NULL;
  }

  G_OBJECT_CLASS (ogmrip_video_stub_parent_class)->dispose (gobject);
}

static void
ogmrip_video_stub_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipVideoStub *stub = OGMRIP_VIDEO_STUB (gobject);

  switch (property_id)
  {
    case PROP_CODEC:
      g_value_set_object (value, stub->priv->codec);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_video_stub_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipVideoStub *stub = OGMRIP_VIDEO_STUB (gobject);

  switch (property_id)
  {
    case PROP_CODEC:
      stub->priv->codec = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_video_stub_class_init (OGMRipVideoStubClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_video_stub_dispose;
  gobject_class->get_property = ogmrip_video_stub_get_property;
  gobject_class->set_property = ogmrip_video_stub_set_property;

  g_type_class_add_private (klass, sizeof (OGMRipStubPriv));
}

/*
 * Audio stub
 */

static gint64
ogmrip_audio_stub_get_media_size (OGMRipMedia *media)
{
  OGMRipAudioStub *stub = OGMRIP_AUDIO_STUB (media);

  return ogmrip_codec_get_size (stub->priv->codec);
}

static void
ogmrip_audio_media_iface_init (OGMRipMediaInterface *iface)
{
  iface->get_size = ogmrip_audio_stub_get_media_size;
}

static gdouble
ogmrip_audio_stub_get_length (OGMRipTitle *title, OGMRipTime *time_)
{
  OGMRipAudioStub *stub = OGMRIP_AUDIO_STUB (title);

  return ogmrip_codec_get_length (stub->priv->codec, time_);
}

static gint64
ogmrip_audio_stub_get_title_size (OGMRipTitle *title)
{
  OGMRipAudioStub *stub = OGMRIP_AUDIO_STUB (title);

  return ogmrip_codec_get_size (stub->priv->codec);
}

static void
ogmrip_audio_title_iface_init (OGMRipTitleInterface *iface)
{
  iface->get_length = ogmrip_audio_stub_get_length;
  iface->get_size = ogmrip_audio_stub_get_title_size;
}

static gint
ogmrip_audio_stub_get_format (OGMRipStream *stream)
{
  OGMRipAudioStub *stub = OGMRIP_AUDIO_STUB (stream);

  return ogmrip_plugin_get_audio_codec_format (G_OBJECT_TYPE (stub->priv->codec));
}

static void
ogmrip_audio_stream_iface_init (OGMRipStreamInterface *iface)
{
  iface->get_format = ogmrip_audio_stub_get_format;
}

static gint
ogmrip_audio_stub_get_channels (OGMRipAudioStream *audio)
{
  OGMRipAudioStub *stub = OGMRIP_AUDIO_STUB (audio);

  return ogmrip_audio_codec_get_channels (OGMRIP_AUDIO_CODEC (stub->priv->codec));
}

static const gchar *
ogmrip_audio_stub_get_label (OGMRipAudioStream *audio)
{
  OGMRipAudioStub *stub = OGMRIP_AUDIO_STUB (audio);

  return ogmrip_audio_codec_get_label (OGMRIP_AUDIO_CODEC (stub->priv->codec));
}

static gint
ogmrip_audio_stub_get_language (OGMRipAudioStream *audio)
{
  OGMRipAudioStub *stub = OGMRIP_AUDIO_STUB (audio);

  return ogmrip_audio_codec_get_language (OGMRIP_AUDIO_CODEC (stub->priv->codec));
}

static gint
ogmrip_audio_stub_get_sample_rate (OGMRipAudioStream *audio)
{
  OGMRipAudioStub *stub = OGMRIP_AUDIO_STUB (audio);

  return ogmrip_audio_codec_get_sample_rate (OGMRIP_AUDIO_CODEC (stub->priv->codec));
}

static void
ogmrip_audio_iface_init (OGMRipAudioStreamInterface *iface)
{
  iface->get_channels = ogmrip_audio_stub_get_channels;
  iface->get_label = ogmrip_audio_stub_get_label;
  iface->get_language = ogmrip_audio_stub_get_language;
  iface->get_sample_rate = ogmrip_audio_stub_get_sample_rate;
}

G_DEFINE_TYPE_WITH_CODE (OGMRipAudioStub, ogmrip_audio_stub, OGMRIP_TYPE_AUDIO_FILE,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_MEDIA, ogmrip_audio_media_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_TITLE, ogmrip_audio_title_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_STREAM, ogmrip_audio_stream_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_AUDIO_STREAM, ogmrip_audio_iface_init));

static void
ogmrip_audio_stub_init (OGMRipAudioStub *stub)
{
  stub->priv = G_TYPE_INSTANCE_GET_PRIVATE (stub, OGMRIP_TYPE_AUDIO_STUB, OGMRipStubPriv);
}

static void
ogmrip_audio_stub_dispose (GObject *gobject)
{
  OGMRipAudioStub *stub = OGMRIP_AUDIO_STUB (gobject);

  if (stub->priv->codec)
  {
    g_object_unref (stub->priv->codec);
    stub->priv->codec = NULL;
  }

  G_OBJECT_CLASS (ogmrip_audio_stub_parent_class)->dispose (gobject);
}

static void
ogmrip_audio_stub_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipAudioStub *stub = OGMRIP_AUDIO_STUB (gobject);

  switch (property_id)
  {
    case PROP_CODEC:
      g_value_set_object (value, stub->priv->codec);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_audio_stub_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipAudioStub *stub = OGMRIP_AUDIO_STUB (gobject);

  switch (property_id)
  {
    case PROP_CODEC:
      stub->priv->codec = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_audio_stub_class_init (OGMRipAudioStubClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_audio_stub_dispose;
  gobject_class->get_property = ogmrip_audio_stub_get_property;
  gobject_class->set_property = ogmrip_audio_stub_set_property;

  g_type_class_add_private (klass, sizeof (OGMRipStubPriv));
}

/*
 * Subp stub
 */

static gint64
ogmrip_subp_stub_get_media_size (OGMRipMedia *media)
{
  OGMRipSubpStub *stub = OGMRIP_SUBP_STUB (media);

  return ogmrip_codec_get_size (stub->priv->codec);
}

static void
ogmrip_subp_media_iface_init (OGMRipMediaInterface *iface)
{
  iface->get_size = ogmrip_subp_stub_get_media_size;
}

static gdouble
ogmrip_subp_stub_get_length (OGMRipTitle *title, OGMRipTime *time_)
{
  OGMRipSubpStub *stub = OGMRIP_SUBP_STUB (title);

  return ogmrip_codec_get_length (stub->priv->codec, time_);
}

static gint64
ogmrip_subp_stub_get_title_size (OGMRipTitle *title)
{
  OGMRipSubpStub *stub = OGMRIP_SUBP_STUB (title);

  return ogmrip_codec_get_size (stub->priv->codec);
}

static void
ogmrip_subp_title_iface_init (OGMRipTitleInterface *iface)
{
  iface->get_length = ogmrip_subp_stub_get_length;
  iface->get_size = ogmrip_subp_stub_get_title_size;
}

static gint
ogmrip_subp_stub_get_format (OGMRipStream *stream)
{
  OGMRipSubpStub *stub = OGMRIP_SUBP_STUB (stream);

  return ogmrip_plugin_get_subp_codec_format (G_OBJECT_TYPE (stub->priv->codec));
}

static void
ogmrip_subp_stream_iface_init (OGMRipStreamInterface *iface)
{
  iface->get_format = ogmrip_subp_stub_get_format;
}

static gint
ogmrip_subp_stub_get_charset (OGMRipSubpStream *subp)
{
  OGMRipSubpStub *stub = OGMRIP_SUBP_STUB (subp);

  return ogmrip_subp_codec_get_charset (OGMRIP_SUBP_CODEC (stub->priv->codec));
}

static const gchar *
ogmrip_subp_stub_get_label (OGMRipSubpStream *subp)
{
  OGMRipSubpStub *stub = OGMRIP_SUBP_STUB (subp);

  return ogmrip_subp_codec_get_label (OGMRIP_SUBP_CODEC (stub->priv->codec));
}

static gint
ogmrip_subp_stub_get_language (OGMRipSubpStream *subp)
{
  OGMRipSubpStub *stub = OGMRIP_SUBP_STUB (subp);

  return ogmrip_subp_codec_get_language (OGMRIP_SUBP_CODEC (stub->priv->codec));
}

static gint
ogmrip_subp_stub_get_newline (OGMRipSubpStream *subp)
{
  OGMRipSubpStub *stub = OGMRIP_SUBP_STUB (subp);

  return ogmrip_subp_codec_get_newline (OGMRIP_SUBP_CODEC (stub->priv->codec));
}

static void
ogmrip_subp_iface_init (OGMRipSubpStreamInterface *iface)
{
  iface->get_charset = ogmrip_subp_stub_get_charset;
  iface->get_label = ogmrip_subp_stub_get_label;
  iface->get_language = ogmrip_subp_stub_get_language;
  iface->get_newline = ogmrip_subp_stub_get_newline;
}

G_DEFINE_TYPE_WITH_CODE (OGMRipSubpStub, ogmrip_subp_stub, OGMRIP_TYPE_SUBP_FILE,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_MEDIA, ogmrip_subp_media_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_TITLE, ogmrip_subp_title_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_STREAM, ogmrip_subp_stream_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_SUBP_STREAM, ogmrip_subp_iface_init));

static void
ogmrip_subp_stub_init (OGMRipSubpStub *stub)
{
  stub->priv = G_TYPE_INSTANCE_GET_PRIVATE (stub, OGMRIP_TYPE_SUBP_STUB, OGMRipStubPriv);
}

static void
ogmrip_subp_stub_dispose (GObject *gobject)
{
  OGMRipSubpStub *stub = OGMRIP_SUBP_STUB (gobject);

  if (stub->priv->codec)
  {
    g_object_unref (stub->priv->codec);
    stub->priv->codec = NULL;
  }

  G_OBJECT_CLASS (ogmrip_subp_stub_parent_class)->dispose (gobject);
}

static void
ogmrip_subp_stub_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipSubpStub *stub = OGMRIP_SUBP_STUB (gobject);

  switch (property_id)
  {
    case PROP_CODEC:
      g_value_set_object (value, stub->priv->codec);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_subp_stub_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipSubpStub *stub = OGMRIP_SUBP_STUB (gobject);

  switch (property_id)
  {
    case PROP_CODEC:
      stub->priv->codec = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_subp_stub_class_init (OGMRipSubpStubClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_subp_stub_dispose;
  gobject_class->get_property = ogmrip_subp_stub_get_property;
  gobject_class->set_property = ogmrip_subp_stub_set_property;

  g_type_class_add_private (klass, sizeof (OGMRipStubPriv));
}

/*
 * Common
 */

OGMRipFile *
ogmrip_stub_new (OGMRipCodec *codec, const gchar *uri)
{
  g_return_val_if_fail (OGMRIP_IS_CODEC (codec), NULL);
  g_return_val_if_fail (uri != NULL, NULL);

  if (OGMRIP_IS_VIDEO_CODEC (codec))
    return g_object_new (OGMRIP_TYPE_VIDEO_STUB, "codec", codec, "uri", uri, NULL);

  if (OGMRIP_IS_AUDIO_CODEC (codec))
    return g_object_new (OGMRIP_TYPE_AUDIO_STUB, "codec", codec, "uri", uri, NULL);

  if (OGMRIP_IS_SUBP_CODEC (codec))
    return g_object_new (OGMRIP_TYPE_SUBP_STUB, "codec", codec, "uri", uri, NULL);

  g_assert_not_reached ();

  return NULL;
}

