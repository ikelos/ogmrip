/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * SECTION:ogmrip-encoding
 * @title: OGMRipEncoding
 * @short_description: An all-in-one component to encode DVD titles
 * @include: ogmrip-encoding.h
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-encoding.h"
#include "ogmrip-plugin.h"

#include <glib/gstdio.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <math.h>
#include <unistd.h>

#define OGMRIP_ENCODING_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_ENCODING, OGMRipEncodingPriv))

struct _OGMRipEncodingPriv
{
  OGMRipProfile *profile;

  OGMRipContainer  *container;
  OGMRipVideoCodec *video_codec;
  GSList *audio_codecs;
  GSList *subp_codecs;

  gboolean relative;
  gboolean autocrop;
  gboolean autoscale;
  gboolean test;
};

enum
{
  PROP_0,
  PROP_AUDIO_CODECS,
  PROP_AUTOCROP,
  PROP_AUTOSCALE,
  PROP_CONTAINER,
  PROP_PROFILE,
  PROP_RELATIVE,
  PROP_SUBP_CODECS,
  PROP_TEST,
  PROP_VIDEO_CODEC
};

static void   ogmrip_encoding_constructed   (GObject      *gobject);
static void   ogmrip_encoding_dispose       (GObject      *gobject);
static void   ogmrip_encoding_finalize      (GObject      *gobject);
static void   ogmrip_encoding_set_property  (GObject      *gobject,
                                             guint        property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void   ogmrip_encoding_get_property  (GObject      *gobject,
                                             guint        property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);

G_DEFINE_TYPE (OGMRipEncoding, ogmrip_encoding, G_TYPE_OBJECT)

static void
ogmrip_encoding_class_init (OGMRipEncodingClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_encoding_constructed;
  gobject_class->dispose = ogmrip_encoding_dispose;
  gobject_class->finalize = ogmrip_encoding_finalize;
  gobject_class->get_property = ogmrip_encoding_get_property;
  gobject_class->set_property = ogmrip_encoding_set_property;
/*
  g_object_class_install_property (gobject_class, PROP_AUDIO_CODECS, 
        g_param_spec_boxed ("audio-codecs", "Audio codecs property", "Set audio codecs", 
           G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
*/
  g_object_class_install_property (gobject_class, PROP_AUTOCROP, 
        g_param_spec_boolean ("autocrop", "Autocrop property", "Set autocrop", 
           TRUE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_AUTOSCALE, 
        g_param_spec_boolean ("autoscale", "Autoscale property", "Set autoscale", 
           TRUE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CONTAINER, 
        g_param_spec_object ("container", "Container property", "Set the container", 
           OGMRIP_TYPE_CONTAINER, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PROFILE, 
        g_param_spec_object ("profile", "Profile property", "Set the profile", 
           OGMRIP_TYPE_PROFILE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RELATIVE, 
        g_param_spec_boolean ("relative", "Relative property", "Set relative", 
           FALSE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
/*
  g_object_class_install_property (gobject_class, PROP_SUBP_CODECS, 
        g_param_spec_boxed ("subp-codecs", "Subp codecs property", "Set subp codecs", 
           G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
*/
  g_object_class_install_property (gobject_class, PROP_TEST, 
        g_param_spec_boolean ("test", "Test property", "Set test", 
           TRUE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VIDEO_CODEC, 
        g_param_spec_object ("video-codec", "Video codec property", "Set video codec", 
           OGMRIP_TYPE_VIDEO_CODEC, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipEncodingPriv));
}

static void
ogmrip_encoding_init (OGMRipEncoding *encoding)
{
  encoding->priv = OGMRIP_ENCODING_GET_PRIVATE (encoding);

  encoding->priv->autocrop = TRUE;
  encoding->priv->autoscale = TRUE;
  encoding->priv->test = TRUE;
}

static void
ogmrip_encoding_constructed (GObject *gobject)
{
  OGMRipEncoding *encoding = OGMRIP_ENCODING (gobject);

  if (!encoding->priv->container)
  {
    GType type;

    type = ogmrip_plugin_get_nth_container (0);
    encoding->priv->container = g_object_new (type, NULL);
  }

  (*G_OBJECT_CLASS (ogmrip_encoding_parent_class)->constructed) (gobject);
}

static void
ogmrip_encoding_dispose (GObject *gobject)
{
  OGMRipEncoding *encoding = OGMRIP_ENCODING (gobject);

  if (encoding->priv->container)
  {
    g_object_unref (encoding->priv->container);
    encoding->priv->container = NULL;
  }

  if (encoding->priv->profile)
  {
    g_object_unref (encoding->priv->profile);
    encoding->priv->profile = NULL;
  }

  if (encoding->priv->video_codec)
  {
    g_object_unref (encoding->priv->video_codec);
    encoding->priv->video_codec = NULL;
  }

  if (encoding->priv->audio_codecs)
  {
    g_slist_foreach (encoding->priv->audio_codecs, (GFunc) g_object_unref, NULL);
    g_slist_free (encoding->priv->audio_codecs);
    encoding->priv->audio_codecs = NULL;
  }

  if (encoding->priv->subp_codecs)
  {
    g_slist_foreach (encoding->priv->subp_codecs, (GFunc) g_object_unref, NULL);
    g_slist_free (encoding->priv->subp_codecs);
    encoding->priv->subp_codecs = NULL;
  }

  (*G_OBJECT_CLASS (ogmrip_encoding_parent_class)->dispose) (gobject);
}

static void
ogmrip_encoding_finalize (GObject *gobject)
{
  (*G_OBJECT_CLASS (ogmrip_encoding_parent_class)->finalize) (gobject);
}

static void
ogmrip_encoding_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipEncoding *encoding = OGMRIP_ENCODING (gobject);

  switch (property_id) 
  {
/*
    case PROP_AUDIO_CODECS:
      encoding->priv->audio_codecs = g_value_get_boxed (value);
      break;
*/
    case PROP_AUTOCROP:
      encoding->priv->autocrop = g_value_get_boolean (value);
      break;
    case PROP_AUTOSCALE:
      encoding->priv->autoscale = g_value_get_boolean (value);
      break;
    case PROP_CONTAINER:
      ogmrip_encoding_set_container (encoding, g_value_get_object (value));
      break;
    case PROP_PROFILE:
      ogmrip_encoding_set_profile (encoding, g_value_get_object (value));
      break;
    case PROP_RELATIVE:
      encoding->priv->relative = g_value_get_boolean (value);
      break;
/*
    case PROP_SUBP_CODECS:
      encoding->priv->subp_codecs = g_value_get_boxed (value);
      break;
*/
    case PROP_TEST:
      encoding->priv->test = g_value_get_boolean (value);
      break;
    case PROP_VIDEO_CODEC:
      ogmrip_encoding_set_video_codec (encoding, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_encoding_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipEncoding *encoding = OGMRIP_ENCODING (gobject);

  switch (property_id) 
  {
/*
    case PROP_AUDIO_CODECS:
      g_value_set_boxed (value, encoding->priv->audio_codecs);
      break;
*/
    case PROP_AUTOCROP:
      g_value_set_boolean (value, encoding->priv->autocrop);
      break;
    case PROP_AUTOSCALE:
      g_value_set_boolean (value, encoding->priv->autoscale);
      break;
    case PROP_CONTAINER:
      g_value_set_object (value, encoding->priv->container);
      break;
    case PROP_PROFILE:
      g_value_set_object (value, encoding->priv->profile);
      break;
    case PROP_RELATIVE:
      g_value_set_boolean (value, encoding->priv->relative);
      break;
/*
    case PROP_SUBP_CODECS:
      g_value_set_boxed (value, encoding->priv->subp_codecs);
      break;
*/
    case PROP_TEST:
      g_value_set_boolean (value, encoding->priv->test);
      break;
    case PROP_VIDEO_CODEC:
      g_value_set_object (value, encoding->priv->video_codec);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

/**
 * ogmrip_encoding_new:
 *
 * Creates a new #OGMRipEncoding.
 *
 * Returns: The newly created #OGMRipEncoding, or NULL
 */
OGMRipEncoding *
ogmrip_encoding_new (void)
{
  return g_object_new (OGMRIP_TYPE_ENCODING, NULL);
}

void
ogmrip_encoding_add_audio_codec (OGMRipEncoding *encoding, OGMRipAudioCodec *codec)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (OGMRIP_IS_AUDIO_CODEC (codec));

  encoding->priv->audio_codecs = g_slist_append (encoding->priv->audio_codecs, g_object_ref (codec));

  // g_object_notify (G_OBJECT (encoding), "audio-codecs");
}

void
ogmrip_encoding_add_subp_codec (OGMRipEncoding *encoding, OGMRipSubpCodec *codec)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (OGMRIP_IS_SUBP_CODEC (codec));

  encoding->priv->subp_codecs = g_slist_append (encoding->priv->subp_codecs, g_object_ref (codec));

  // g_object_notify (G_OBJECT (encoding), "subp-codecs");
}

gboolean
ogmrip_encoding_get_autocrop (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->autocrop;
}

void
ogmrip_encoding_set_autocrop (OGMRipEncoding *encoding, gboolean autocrop)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  encoding->priv->autocrop = autocrop;

  g_object_notify (G_OBJECT (encoding), "autocrop");
}

gboolean
ogmrip_encoding_get_autoscale (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->autoscale;
}

void
ogmrip_encoding_set_autoscale (OGMRipEncoding *encoding, gboolean autoscale)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  encoding->priv->autoscale = autoscale;

  g_object_notify (G_OBJECT (encoding), "autoscale");
}

OGMRipContainer *
ogmrip_encoding_get_container (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return encoding->priv->container;
}

void
ogmrip_encoding_set_container (OGMRipEncoding *encoding, OGMRipContainer *container)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));

  g_object_ref (container);
  if (encoding->priv->container)
    g_object_unref (encoding->priv->container);
  encoding->priv->container = container;

  g_object_notify (G_OBJECT (encoding), "container");
}

OGMRipProfile *
ogmrip_encoding_get_profile (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return encoding->priv->profile;
}

void
ogmrip_encoding_set_profile (OGMRipEncoding *encoding, OGMRipProfile *profile)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (OGMRIP_IS_PROFILE (profile));

  g_object_ref (profile);
  if (encoding->priv->profile)
    g_object_unref (encoding->priv->profile);
  encoding->priv->profile = profile;

  g_object_notify (G_OBJECT (encoding), "profile");
}

gboolean
ogmrip_encoding_get_test (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->test;
}

void
ogmrip_encoding_set_test (OGMRipEncoding *encoding, gboolean test)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  encoding->priv->test = test;

  g_object_notify (G_OBJECT (encoding), "test");
}

OGMRipVideoCodec *
ogmrip_encoding_get_video_codec (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return encoding->priv->video_codec;
}

void
ogmrip_encoding_set_video_codec (OGMRipEncoding *encoding, OGMRipVideoCodec *codec)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (codec == NULL || OGMRIP_IS_VIDEO_CODEC (codec));

  if (codec)
    g_object_ref (codec);
  if (encoding->priv->video_codec)
    g_object_unref (encoding->priv->video_codec);
  encoding->priv->video_codec = codec;

  g_object_notify (G_OBJECT (encoding), "video-codec");
}

gboolean
ogmrip_encoding_get_relative (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->relative;
}

void
ogmrip_encoding_set_relative (OGMRipEncoding *encoding, gboolean relative)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  encoding->priv->relative = relative;

  g_object_notify (G_OBJECT (encoding), "relative");
}

static gint64
ogmrip_encoding_get_rip_size (OGMRipEncoding *encoding)
{
  guint number, size;
  gint bitrate;

  ogmrip_container_get_split (encoding->priv->container, &number, &size);

  if (size > 0)
  {
    gdouble factor = 1.0;

    if (encoding->priv->relative)
    {
      OGMDvdStream *stream;
      gdouble full_len;

      stream = ogmrip_codec_get_input (OGMRIP_CODEC (encoding->priv->video_codec));

      full_len = ogmdvd_title_get_length (ogmdvd_stream_get_title (stream), NULL);
      if (full_len < 0)
        return -1;

      factor = ogmrip_codec_get_length (OGMRIP_CODEC (encoding->priv->video_codec), NULL) / full_len;
    }

    return ceil (factor * size * number * 1024 * 1024);
  }

  bitrate = ogmrip_video_codec_get_bitrate (encoding->priv->video_codec);
  if (bitrate > 0)
    return ceil (bitrate * ogmrip_codec_get_length (OGMRIP_CODEC (encoding->priv->video_codec), NULL) / 8.0);

  return 0;
}

static gint64
ogmrip_encoding_get_file_size (OGMRipCodec *codec)
{
  struct stat buf;
  const gchar *filename;
  guint64 size = 0;

  filename = ogmrip_codec_get_output (codec);
  if (filename && g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    if (g_stat (filename, &buf) == 0)
      size = (guint64) buf.st_size;

  return size;
}

static gint64
ogmrip_encoding_get_nonvideo_size (OGMRipEncoding *encoding)
{
  GSList *link;
  gint64 nonvideo = 0;

  for (link = encoding->priv->audio_codecs; link; link = link->next)
    nonvideo += ogmrip_encoding_get_file_size (link->data);

  for (link = encoding->priv->subp_codecs; link; link = link->next)
    nonvideo += ogmrip_encoding_get_file_size (link->data);
/*
  for (link = encoding->priv->chapters; link; link = link->next)
    nonvideo += ogmrip_encoding_get_file_size (link->data);

  for (link = encoding->priv->files; link; link = link->next)
    nonvideo += ogmrip_file_get_size (link->data);
*/
  return nonvideo;
}

static gint64
ogmrip_encoding_get_video_overhead (OGMRipEncoding *encoding)
{
  OGMDvdStream *stream;
  gdouble framerate, length, frames;
  guint num, denom;
  gint overhead;

  if (!encoding->priv->video_codec)
    return 0;

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (encoding->priv->video_codec));
  ogmdvd_video_stream_get_framerate (OGMDVD_VIDEO_STREAM (stream), &num, &denom);
  framerate = num / (gdouble) denom;

  length = ogmrip_codec_get_length (OGMRIP_CODEC (encoding->priv->video_codec), NULL);
  frames = length * framerate;

  overhead = ogmrip_container_get_overhead (encoding->priv->container);

  return (gint64) (frames * overhead);
}

static gint64
ogmrip_encoding_get_audio_overhead (OGMRipEncoding *encoding, OGMRipAudioCodec *codec)
{
  gdouble length, audio_frames;
  gint samples_per_frame, sample_rate, channels, overhead;

  length = ogmrip_codec_get_length (OGMRIP_CODEC (codec), NULL);
  samples_per_frame = ogmrip_audio_codec_get_samples_per_frame (codec);

  sample_rate = 48000;
  channels = 1;

  if (ogmrip_plugin_get_audio_codec_format (G_OBJECT_TYPE (codec)) == OGMRIP_FORMAT_COPY)
  {
    OGMDvdStream *stream;

    stream = ogmrip_codec_get_input (OGMRIP_CODEC (codec));
    sample_rate = ogmdvd_audio_stream_get_sample_rate (OGMDVD_AUDIO_STREAM (stream));
    channels = ogmdvd_audio_stream_get_channels (OGMDVD_AUDIO_STREAM (stream));
  }
  else
  {
    sample_rate = ogmrip_audio_codec_get_sample_rate (codec);
    channels = ogmrip_audio_codec_get_channels (codec);
  }

  audio_frames = length * sample_rate * (channels + 1) / samples_per_frame;

  overhead = ogmrip_container_get_overhead (encoding->priv->container);

  return (gint64) (audio_frames * overhead);
}

static gint64
ogmrip_encoding_get_subp_overhead (OGMRipEncoding *encoding, OGMRipSubpCodec *codec)
{
  return 0;
}
/*
static gint64
ogmrip_encoding_get_file_overhead (OGMRipEncoding *encoding, OGMRipFile *file)
{
  glong length, audio_frames;
  gint samples_per_frame, sample_rate, channels, overhead;

  if (ogmrip_file_get_type (file) == OGMRIP_FILE_TYPE_SUBP)
    return 0;

  length = ogmrip_audio_file_get_length (OGMRIP_AUDIO_FILE (file));
  sample_rate = ogmrip_audio_file_get_sample_rate (OGMRIP_AUDIO_FILE (file));
  samples_per_frame = ogmrip_audio_file_get_samples_per_frame (OGMRIP_AUDIO_FILE (file));

  channels = 1;
  if (ogmrip_file_get_format (file) != OGMRIP_FORMAT_COPY)
    channels = ogmrip_audio_file_get_channels (OGMRIP_AUDIO_FILE (file));

  audio_frames = length * sample_rate * (channels + 1) / samples_per_frame;

  overhead = ogmrip_container_get_overhead (encoding->priv->container);

  return (gint64) (audio_frames * overhead);
}
*/
static gint64
ogmrip_encoding_get_overhead_size (OGMRipEncoding *encoding)
{
  GSList *link;
  gint64 overhead = 0;

  overhead = ogmrip_encoding_get_video_overhead (encoding);

  for (link = encoding->priv->audio_codecs; link; link = link->next)
    overhead += ogmrip_encoding_get_audio_overhead (encoding, link->data);

  for (link = encoding->priv->subp_codecs; link; link = link->next)
    overhead += ogmrip_encoding_get_subp_overhead (encoding, link->data);
/*
  for (link = encoding->priv->files; link; link = link->next)
    overhead += ogmrip_encoding_get_file_overhead (encoding, link->data);
*/
  return overhead;
}

gint
ogmrip_encoding_autobitrate (OGMRipEncoding *encoding)
{
  gdouble video_size, length;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (encoding->priv->video_codec), -1);

  video_size = ogmrip_encoding_get_rip_size (encoding) -
    ogmrip_encoding_get_nonvideo_size (encoding) -
    ogmrip_encoding_get_overhead_size (encoding);

  length = ogmrip_codec_get_length (OGMRIP_CODEC (encoding->priv->video_codec), NULL);

  return (video_size * 8.) / length;
}

#define ROUND(x) ((gint) ((x) + 0.5) != (gint) (x) ? ((gint) ((x) + 0.5)) : ((gint) (x)))

void
ogmrip_encoding_autoscale (OGMRipEncoding *encoding, gdouble bpp, guint *width, guint *height)
{
  OGMDvdStream *stream;
  guint an, ad, rn, rd, cw, ch, rw, rh;
  gdouble ratio, rbpp;
  gint br;

  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (encoding->priv->video_codec));
  g_return_if_fail (width != NULL && height != NULL);

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (encoding->priv->video_codec));

  ogmdvd_video_stream_get_resolution (OGMDVD_VIDEO_STREAM (stream), &rw, &rh);
  ogmdvd_video_stream_get_framerate (OGMDVD_VIDEO_STREAM (stream), &rn, &rd);

  ogmrip_video_codec_get_crop_size (encoding->priv->video_codec, NULL, NULL, &cw, &ch);
  ogmrip_video_codec_get_aspect_ratio (encoding->priv->video_codec, &an, &ad);

  ratio = cw / (gdouble) ch * rh / rw * an / ad;

  br = ogmrip_video_codec_get_bitrate (encoding->priv->video_codec);
  if (br > 0)
  {
    *height = rh;
    for (*width = rw - 25 * 16; *width <= rw; *width += 16)
    {
      *height = ROUND (*width / ratio);

      rbpp = (br * rd) / (gdouble) (*width * *height * rn);

      if (rbpp < bpp)
        break;
    }
  }
  else
  {
    *width = cw;
    *height = ROUND (*width / ratio);
  }

  *width = MIN (*width, rw);
}

