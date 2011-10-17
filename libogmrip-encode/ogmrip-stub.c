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
#include "ogmrip-video-codec.h"
#include "ogmrip-audio-codec.h"
#include "ogmrip-subp-codec.h"
#include "ogmrip-chapters.h"
#include "ogmrip-type.h"

#include <glib/gstdio.h>

struct _OGMRipStubPriv
{
  OGMRipCodec *codec;
  gint format;
};

enum
{
  PROP_0,
  PROP_CODEC
};

static void ogmrip_stub_dispose      (GObject      *gobject);
static void ogmrip_stub_get_property (GObject      *gobject,
                                      guint        property_id,
                                      GValue       *value,
                                      GParamSpec   *pspec);
static void ogmrip_stub_set_property (GObject      *gobject,
                                      guint        property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec);

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

static gint64
ogmrip_stub_get_media_size (OGMRipMedia *media)
{
  OGMRipStub *stub = OGMRIP_STUB (media);

  return ogmrip_codec_get_size (stub->priv->codec);
}

static void
ogmrip_media_iface_init (OGMRipMediaInterface *iface)
{
  iface->get_size = ogmrip_stub_get_media_size;
}

static gdouble
ogmrip_stub_get_chapters_length (OGMRipTitle *title, guint start, gint end, OGMRipTime *time_)
{
  OGMRipStub *stub = OGMRIP_STUB (title);
  OGMRipStream *input;
  guint real_start, real_end;

  input = ogmrip_codec_get_input (stub->priv->codec);
  ogmrip_codec_get_chapters (stub->priv->codec, &real_start, &real_end);

  return ogmrip_title_get_chapters_length (ogmrip_stream_get_title (input),
      start + real_start, end + real_start, time_);
}

static gdouble
ogmrip_stub_get_length (OGMRipTitle *title, OGMRipTime *time_)
{
  OGMRipStub *stub = OGMRIP_STUB (title);

  return ogmrip_codec_get_length (stub->priv->codec, time_);
}

static gint
ogmrip_stub_get_n_chapters (OGMRipTitle *title)
{
  OGMRipStub *stub = OGMRIP_STUB (title);
  guint start, end;

  ogmrip_codec_get_chapters (stub->priv->codec, &start, &end);

  return end - start + 1;
}

static gint64
ogmrip_stub_get_title_size (OGMRipTitle *title)
{
  OGMRipStub *stub = OGMRIP_STUB (title);

  return ogmrip_codec_get_size (stub->priv->codec);
}

static void
ogmrip_title_iface_init (OGMRipTitleInterface *iface)
{
  iface->get_chapters_length = ogmrip_stub_get_chapters_length;
  iface->get_length = ogmrip_stub_get_length;
  iface->get_n_chapters = ogmrip_stub_get_n_chapters;
  iface->get_size = ogmrip_stub_get_title_size;
}

static gint
ogmrip_stub_get_format (OGMRipStream *stream)
{
  return OGMRIP_STUB (stream)->priv->format;
}

static void
ogmrip_stream_iface_init (OGMRipStreamInterface *iface)
{
  iface->get_format = ogmrip_stub_get_format;
}

static void
ogmrip_stub_init (OGMRipStub *stub)
{
  stub->priv = G_TYPE_INSTANCE_GET_PRIVATE (stub, OGMRIP_TYPE_STUB, OGMRipStubPriv);
}

static void
ogmrip_stub_class_init (OGMRipStubClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_stub_dispose;
  gobject_class->get_property = ogmrip_stub_get_property;
  gobject_class->set_property = ogmrip_stub_set_property;

  g_object_class_install_property (gobject_class, PROP_CODEC,
      g_param_spec_object ("codec", "Codec property", "Set codec",
        OGMRIP_TYPE_CODEC, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipStubPriv));
}

G_DEFINE_TYPE_WITH_CODE (OGMRipStub, ogmrip_stub, OGMRIP_TYPE_FILE,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_MEDIA, ogmrip_media_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_TITLE, ogmrip_title_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_STREAM, ogmrip_stream_iface_init));

static void
ogmrip_stub_dispose (GObject *gobject)
{
  OGMRipStub *stub = OGMRIP_STUB (gobject);

  if (stub->priv->codec)
  {
    g_object_unref (stub->priv->codec);
    stub->priv->codec = NULL;
  }

  G_OBJECT_CLASS (ogmrip_stub_parent_class)->dispose (gobject);
}

static void
ogmrip_stub_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipStub *stub = OGMRIP_STUB (gobject);

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
ogmrip_stub_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipStub *stub = OGMRIP_STUB (gobject);

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

/*
 * Video stub
 */

static void
ogmrip_video_stub_get_aspect_ratio (OGMRipVideoStream *video, guint *n, guint *d)
{
  OGMRipStub *stub = OGMRIP_STUB (video);

  ogmrip_video_codec_get_aspect_ratio (OGMRIP_VIDEO_CODEC (stub->priv->codec), n, d);
}

static gint
ogmrip_video_stub_get_bitrate (OGMRipVideoStream *video)
{
  OGMRipStub *stub = OGMRIP_STUB (video);

  return ogmrip_video_codec_get_bitrate (OGMRIP_VIDEO_CODEC (stub->priv->codec));
}

static void
ogmrip_video_stub_get_crop_size (OGMRipVideoStream *video, guint *x, guint *y, guint *w, guint *h)
{
  OGMRipStub *stub = OGMRIP_STUB (video);

  ogmrip_video_codec_get_crop_size (OGMRIP_VIDEO_CODEC (stub->priv->codec), x, y, w, h);
}

static void
ogmrip_video_stub_get_framerate (OGMRipVideoStream *video, guint *n, guint *d)
{
  OGMRipStub *stub = OGMRIP_STUB (video);

  ogmrip_video_codec_get_framerate (OGMRIP_VIDEO_CODEC (stub->priv->codec), n, d);
}

static void
ogmrip_video_stub_get_resolution (OGMRipVideoStream *video, guint *w, guint *h)
{
  OGMRipStub *stub = OGMRIP_STUB (video);

  ogmrip_video_codec_get_scale_size (OGMRIP_VIDEO_CODEC (stub->priv->codec), w, h);
}

static gint
ogmrip_video_stub_get_start_delay (OGMRipVideoStream *video)
{
  OGMRipStub *stub = OGMRIP_STUB (video);

  return ogmrip_video_codec_get_start_delay (OGMRIP_VIDEO_CODEC (stub->priv->codec));
}

static void
ogmrip_video_iface_init (OGMRipVideoStreamInterface *iface)
{
  iface->get_aspect_ratio = ogmrip_video_stub_get_aspect_ratio;
  iface->get_bitrate = ogmrip_video_stub_get_bitrate;
  iface->get_crop_size = ogmrip_video_stub_get_crop_size;
  iface->get_framerate = ogmrip_video_stub_get_framerate;
  iface->get_resolution = ogmrip_video_stub_get_resolution;
  iface->get_start_delay = ogmrip_video_stub_get_start_delay;
}

G_DEFINE_TYPE_WITH_CODE (OGMRipVideoStub, ogmrip_video_stub, OGMRIP_TYPE_STUB,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_VIDEO_STREAM, ogmrip_video_iface_init));

static void
ogmrip_video_stub_init (OGMRipVideoStub *stub)
{
}

static void
ogmrip_video_stub_constructed (GObject *gobject)
{
  OGMRipStub *stub = OGMRIP_STUB (gobject);

  if (!stub->priv->codec)
    g_error ("No video codec specified");

  stub->priv->format = ogmrip_codec_format (G_OBJECT_TYPE (stub->priv->codec));

  G_OBJECT_CLASS (ogmrip_video_stub_parent_class)->constructed (gobject);
}

static void
ogmrip_video_stub_class_init (OGMRipVideoStubClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_video_stub_constructed;
}

/*
 * Audio stub
 */

static gint
ogmrip_audio_stub_get_channels (OGMRipAudioStream *audio)
{
  OGMRipStub *stub = OGMRIP_STUB (audio);

  return ogmrip_audio_codec_get_channels (OGMRIP_AUDIO_CODEC (stub->priv->codec));
}

static const gchar *
ogmrip_audio_stub_get_label (OGMRipAudioStream *audio)
{
  OGMRipStub *stub = OGMRIP_STUB (audio);

  return ogmrip_audio_codec_get_label (OGMRIP_AUDIO_CODEC (stub->priv->codec));
}

static gint
ogmrip_audio_stub_get_language (OGMRipAudioStream *audio)
{
  OGMRipStub *stub = OGMRIP_STUB (audio);

  return ogmrip_audio_codec_get_language (OGMRIP_AUDIO_CODEC (stub->priv->codec));
}

static gint
ogmrip_audio_stub_get_sample_rate (OGMRipAudioStream *audio)
{
  OGMRipStub *stub = OGMRIP_STUB (audio);

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

G_DEFINE_TYPE_WITH_CODE (OGMRipAudioStub, ogmrip_audio_stub, OGMRIP_TYPE_STUB,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_AUDIO_STREAM, ogmrip_audio_iface_init));

static void
ogmrip_audio_stub_init (OGMRipAudioStub *stub)
{
}

static void
ogmrip_audio_stub_constructed (GObject *gobject)
{
  OGMRipStub *stub = OGMRIP_STUB (gobject);

  if (!stub->priv->codec)
    g_error ("No audio codec specified");

  stub->priv->format = ogmrip_codec_format (G_OBJECT_TYPE (stub->priv->codec));

  G_OBJECT_CLASS (ogmrip_audio_stub_parent_class)->constructed (gobject);
}

static void
ogmrip_audio_stub_class_init (OGMRipAudioStubClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_audio_stub_constructed;
}

/*
 * Subp stub
 */

static gint
ogmrip_subp_stub_get_charset (OGMRipSubpStream *subp)
{
  OGMRipStub *stub = OGMRIP_STUB (subp);

  return ogmrip_subp_codec_get_charset (OGMRIP_SUBP_CODEC (stub->priv->codec));
}

static const gchar *
ogmrip_subp_stub_get_label (OGMRipSubpStream *subp)
{
  OGMRipStub *stub = OGMRIP_STUB (subp);

  return ogmrip_subp_codec_get_label (OGMRIP_SUBP_CODEC (stub->priv->codec));
}

static gint
ogmrip_subp_stub_get_language (OGMRipSubpStream *subp)
{
  OGMRipStub *stub = OGMRIP_STUB (subp);

  return ogmrip_subp_codec_get_language (OGMRIP_SUBP_CODEC (stub->priv->codec));
}

static gint
ogmrip_subp_stub_get_newline (OGMRipSubpStream *subp)
{
  OGMRipStub *stub = OGMRIP_STUB (subp);

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

G_DEFINE_TYPE_WITH_CODE (OGMRipSubpStub, ogmrip_subp_stub, OGMRIP_TYPE_STUB,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_SUBP_STREAM, ogmrip_subp_iface_init));

static void
ogmrip_subp_stub_init (OGMRipSubpStub *stub)
{
}

static void
ogmrip_subp_stub_constructed (GObject *gobject)
{
  OGMRipStub *stub = OGMRIP_STUB (gobject);

  if (!stub->priv->codec)
    g_error ("No subp codec specified");

  stub->priv->format = ogmrip_codec_format(G_OBJECT_TYPE (stub->priv->codec));

  G_OBJECT_CLASS (ogmrip_subp_stub_parent_class)->constructed (gobject);
}

static void
ogmrip_subp_stub_class_init (OGMRipSubpStubClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_subp_stub_constructed;
}

/*
 * Chapters
 */

static const gchar *
ogmrip_chapters_stub_get_label (OGMRipChaptersStream *chapters, guint nr)
{
  OGMRipStub *stub = OGMRIP_STUB (chapters);

  return ogmrip_chapters_get_label (OGMRIP_CHAPTERS (stub->priv->codec), nr);
}

static gint
ogmrip_chapters_stub_get_language (OGMRipChaptersStream *chapters)
{
  OGMRipStub *stub = OGMRIP_STUB (chapters);

  return ogmrip_chapters_get_language (OGMRIP_CHAPTERS (stub->priv->codec));
}

static void
ogmrip_chapters_iface_init (OGMRipChaptersStreamInterface *iface)
{
  iface->get_label = ogmrip_chapters_stub_get_label;
  iface->get_language = ogmrip_chapters_stub_get_language;
}

G_DEFINE_TYPE_WITH_CODE (OGMRipChaptersStub, ogmrip_chapters_stub, OGMRIP_TYPE_STUB,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_CHAPTERS_STREAM, ogmrip_chapters_iface_init));

static void
ogmrip_chapters_stub_init (OGMRipChaptersStub *stub)
{
}

static void
ogmrip_chapters_stub_constructed (GObject *gobject)
{
  OGMRipStub *stub = OGMRIP_STUB (gobject);

  stub->priv->format = OGMRIP_FORMAT_CHAPTERS;

  G_OBJECT_CLASS (ogmrip_chapters_stub_parent_class)->constructed (gobject);
}

static void
ogmrip_chapters_stub_class_init (OGMRipChaptersStubClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_chapters_stub_constructed;
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

  if (OGMRIP_IS_CHAPTERS (codec))
    return g_object_new (OGMRIP_TYPE_CHAPTERS_STUB, "codec", codec, "uri", uri, NULL);

  g_assert_not_reached ();

  return NULL;
}

