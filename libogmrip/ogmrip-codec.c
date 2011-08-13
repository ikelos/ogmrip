/* OGMRip - A library for DVD ripping and encoding
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

/**
 * SECTION:ogmrip-codec
 * @title: OGMRipCodec
 * @short_description: Base class for codecs
 * @include: ogmrip-codec.h
 */

#include "ogmrip-codec.h"

#include <unistd.h>
#include <glib/gstdio.h>

#define OGMRIP_CODEC_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_CODEC, OGMRipCodecPriv))

struct _OGMRipCodecPriv
{
  OGMDvdTitle *title;
  OGMDvdStream *input;
  OGMDvdTime time_;
  gchar *output;

  gboolean do_unlink;
  gboolean dirty;

  gdouble length;

  guint start_chap;
  gint end_chap;

  gdouble start_second;
  gdouble play_length;
};

enum 
{
  PROP_0,
  PROP_INPUT,
  PROP_OUTPUT,
  PROP_LENGTH,
  PROP_START_CHAPTER,
  PROP_END_CHAPTER
};

static void ogmrip_codec_constructed  (GObject      *gobject);
static void ogmrip_codec_dispose      (GObject      *gobject);
static void ogmrip_codec_finalize     (GObject      *gobject);
static void ogmrip_codec_set_property (GObject      *gobject,
                                       guint        property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);
static void ogmrip_codec_get_property (GObject      *gobject,
                                       guint        property_id,
                                       GValue       *value,
                                       GParamSpec   *pspec);
static gint ogmrip_codec_run          (OGMJobSpawn  *spawn);

static void
ogmrip_codec_set_input (OGMRipCodec *codec, OGMDvdStream *input)
{
  g_return_if_fail (OGMRIP_IS_CODEC (codec));
  g_return_if_fail (input != NULL);

  ogmdvd_stream_ref (input);

  if (codec->priv->input)
    ogmdvd_stream_unref (codec->priv->input);

  codec->priv->title = ogmdvd_stream_get_title (input);

  codec->priv->dirty = TRUE;
  codec->priv->start_chap = 0;
  codec->priv->end_chap = -1;
}

G_DEFINE_ABSTRACT_TYPE (OGMRipCodec, ogmrip_codec, OGMJOB_TYPE_BIN)

static void
ogmrip_codec_class_init (OGMRipCodecClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_codec_constructed;
  gobject_class->dispose = ogmrip_codec_dispose;
  gobject_class->finalize = ogmrip_codec_finalize;
  gobject_class->set_property = ogmrip_codec_set_property;
  gobject_class->get_property = ogmrip_codec_get_property;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmrip_codec_run;

  g_object_class_install_property (gobject_class, PROP_INPUT, 
        g_param_spec_pointer ("input", "Input property", "Set input title", 
           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_OUTPUT, 
        g_param_spec_string ("output", "Output property", "Set output file", 
           NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LENGTH, 
        g_param_spec_double ("length", "Length property", "Get length", 
           0.0, G_MAXDOUBLE, 0.0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_START_CHAPTER, 
        g_param_spec_int ("start-chapter", "Start chapter property", "Set start chapter", 
           0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_END_CHAPTER, 
        g_param_spec_int ("end-chapter", "End chapter property", "Set end chapter", 
           -1, G_MAXINT, -1, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipCodecPriv));
}

static void
ogmrip_codec_init (OGMRipCodec *codec)
{
  codec->priv = OGMRIP_CODEC_GET_PRIVATE (codec);

  codec->priv->end_chap = -1;
  codec->priv->play_length = -1.0;
  codec->priv->start_second = -1.0;
}

static void
ogmrip_codec_constructed (GObject *gobject)
{
  OGMRipCodec *codec = OGMRIP_CODEC (gobject);

  if (!codec->priv->output)
    codec->priv->output = g_strdup ("/dev/null");

  G_OBJECT_CLASS (ogmrip_codec_parent_class)->constructed (gobject);
}

static void
ogmrip_codec_dispose (GObject *gobject)
{
  OGMRipCodec *codec = OGMRIP_CODEC (gobject);

  if (codec->priv->input)
  {
    ogmdvd_stream_unref (codec->priv->input);
    codec->priv->input = NULL;
  }

  G_OBJECT_CLASS (ogmrip_codec_parent_class)->dispose (gobject);
}

static void
ogmrip_codec_finalize (GObject *gobject)
{
  OGMRipCodec *codec = OGMRIP_CODEC (gobject);

  if (codec->priv->output)
  {
    if (codec->priv->do_unlink && g_file_test (codec->priv->output, G_FILE_TEST_IS_REGULAR))
      g_unlink (codec->priv->output);

    g_free (codec->priv->output);
    codec->priv->output = NULL;
  }

  G_OBJECT_CLASS (ogmrip_codec_parent_class)->finalize (gobject);
}

static void
ogmrip_codec_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipCodec *codec = OGMRIP_CODEC (gobject);

  switch (property_id) 
  {
    case PROP_INPUT:
      ogmrip_codec_set_input (codec, g_value_get_pointer (value));
      break;
    case PROP_OUTPUT:
      ogmrip_codec_set_output (codec, g_value_get_string (value));
      break;
    case PROP_START_CHAPTER: 
      ogmrip_codec_set_chapters (codec, g_value_get_int (value), codec->priv->end_chap);
      break;
    case PROP_END_CHAPTER: 
      ogmrip_codec_set_chapters (codec, codec->priv->start_chap, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_codec_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipCodec *codec = OGMRIP_CODEC (gobject);

  switch (property_id) 
  {
    case PROP_INPUT:
      g_value_set_pointer (value, codec->priv->input);
      break;
    case PROP_OUTPUT:
      g_value_set_string (value, codec->priv->output);
      break;
    case PROP_LENGTH: 
      g_value_set_double (value, ogmrip_codec_get_length (codec, NULL));
      break;
    case PROP_START_CHAPTER: 
      g_value_set_int (value, codec->priv->start_chap);
      break;
    case PROP_END_CHAPTER: 
      g_value_set_int (value, codec->priv->end_chap);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static gint
ogmrip_codec_run (OGMJobSpawn *spawn)
{
  OGMDvdTitle *title = OGMRIP_CODEC (spawn)->priv->title;

  g_return_val_if_fail (title != NULL, OGMJOB_RESULT_ERROR);
  g_return_val_if_fail (ogmdvd_title_is_open (title), OGMJOB_RESULT_ERROR);

  return OGMJOB_SPAWN_CLASS (ogmrip_codec_parent_class)->run (spawn);
}

/**
 * ogmrip_codec_get_output:
 * @codec: an #OGMRipCodec
 *
 * Gets the name of the output file.
 *
 * Returns: the filename, or NULL
 */
const gchar *
ogmrip_codec_get_output (OGMRipCodec *codec)
{
  g_return_val_if_fail (OGMRIP_IS_CODEC (codec), NULL);

  return codec->priv->output;
}

/**
 * ogmrip_codec_set_output:
 * @codec: an #OGMRipCodec
 * @output: the name of the output file
 *
 * Sets the name of the output file.
 */
void
ogmrip_codec_set_output (OGMRipCodec *codec, const gchar *output)
{
  g_return_if_fail (OGMRIP_IS_CODEC (codec));

  g_free (codec->priv->output);
  codec->priv->output = g_strdup (output);

  g_object_notify (G_OBJECT (codec), "output");
}

/**
 * ogmrip_codec_get_input:
 * @codec: an #OGMRipCodec
 *
 * Gets the input DVD title.
 *
 * Returns: an #OGMDvdTitle, or NULL
 */
OGMDvdStream *
ogmrip_codec_get_input (OGMRipCodec *codec)
{
  g_return_val_if_fail (OGMRIP_IS_CODEC (codec), NULL);

  return codec->priv->input;
}

/**
 * ogmrip_codec_set_chapters:
 * @codec: an #OGMRipCodec
 * @start: the start chapter
 * @end: the end chapter
 *
 * Sets which chapters to start and end at.
 */
void
ogmrip_codec_set_chapters (OGMRipCodec *codec, guint start, gint end)
{
  g_return_if_fail (OGMRIP_IS_CODEC (codec));

  if (codec->priv->start_chap != start || codec->priv->end_chap != end)
  {
    gint nchap;

    nchap = ogmdvd_title_get_n_chapters (codec->priv->title);

    if (end < 0)
      end = nchap - 1;

    codec->priv->start_chap = MIN ((gint) start, nchap - 1);
    codec->priv->end_chap = CLAMP (end, (gint) start, nchap - 1);

    codec->priv->start_second = -1.0;
    codec->priv->play_length = -1.0;

    codec->priv->dirty = TRUE;
  }
}

/**
 * ogmrip_codec_get_chapters:
 * @codec: an #OGMRipCodec
 * @start: a pointer to set the start chapter
 * @end: a pointer to set the end chapter
 *
 * Gets which chapters to start and end at.
 */
void
ogmrip_codec_get_chapters (OGMRipCodec *codec, guint *start, guint *end)
{
  g_return_if_fail (OGMRIP_IS_CODEC (codec));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);

  *start = codec->priv->start_chap;

  if (codec->priv->end_chap < 0)
    *end = ogmdvd_title_get_n_chapters (codec->priv->title) - 1;
  else
    *end = codec->priv->end_chap;
}

static void
ogmrip_codec_sec_to_time (gdouble length, gdouble fps, OGMDvdTime *dtime)
{
  glong sec = (glong) length;

  dtime->hour = sec / (60 * 60);
  dtime->min = sec / 60 % 60;
  dtime->sec = sec % 60;

  dtime->frames = (length - sec) * fps;
}

/**
 * ogmrip_codec_get_length:
 * @codec: an #OGMRipCodec
 * @length: a pointer to store an #OGMDvdTime, or NULL
 *
 * Returns the length of the encoding in seconds. If @length is not NULL, the
 * data structure will be filled with the length in hours, minutes seconds and
 * frames.
 *
 * Returns: the length in seconds, or -1.0
 */
gdouble
ogmrip_codec_get_length (OGMRipCodec *codec, OGMDvdTime *length)
{
  g_return_val_if_fail (OGMRIP_IS_CODEC (codec), -1.0);

  if (!codec->priv->title)
    return -1.0;

  if (codec->priv->dirty)
  {
    guint num, denom;

    ogmdvd_video_stream_get_framerate (ogmdvd_title_get_video_stream (codec->priv->title), &num, &denom);

    if (codec->priv->play_length > 0.0)
    {
      codec->priv->length = codec->priv->play_length;
      ogmrip_codec_sec_to_time (codec->priv->play_length, num / (gdouble) denom, &codec->priv->time_);
    }
    else if (codec->priv->start_chap == 0 && codec->priv->end_chap == -1)
      codec->priv->length = ogmdvd_title_get_length (codec->priv->title, &codec->priv->time_);
    else
      codec->priv->length = ogmdvd_title_get_chapters_length (codec->priv->title, 
            codec->priv->start_chap, codec->priv->end_chap, &codec->priv->time_);

    codec->priv->dirty = FALSE;
  }

  if (length)
    *length = codec->priv->time_;

  return codec->priv->length;
}

/**
 * ogmrip_codec_set_play_length:
 * @codec: an #OGMRipCodec
 * @length: the length to encode in seconds
 *
 * Encodes only @length seconds.
 */
void
ogmrip_codec_set_play_length (OGMRipCodec *codec, gdouble length)
{
  g_return_if_fail (OGMRIP_IS_CODEC (codec));
  g_return_if_fail (length > 0.0);

  codec->priv->start_chap = 0;
  codec->priv->end_chap = -1;

  codec->priv->play_length = length;

  codec->priv->dirty = TRUE;
}

/**
 * ogmrip_codec_get_play_length:
 * @codec: an #OGMRipCodec
 *
 * Gets the length to encode in seconds.
 *
 * Returns: the length, or -1.0
 */
gdouble
ogmrip_codec_get_play_length (OGMRipCodec *codec)
{
  g_return_val_if_fail (OGMRIP_IS_CODEC (codec), -1.0);

  return codec->priv->play_length;
}

/**
 * ogmrip_codec_set_start_position:
 * @codec: an #OGMRipCodec
 * @start: the position to seek in seconds
 *
 * Seeks to the given time position.
 */
void
ogmrip_codec_set_start_position (OGMRipCodec *codec, gdouble start)
{
  g_return_if_fail (OGMRIP_IS_CODEC (codec));
  g_return_if_fail (start >= 0.0);

  codec->priv->start_chap = 0;
  codec->priv->end_chap = -1;

  codec->priv->start_second = start;
}

/**
 * ogmrip_codec_get_start_position:
 * @codec: an #OGMRipCodec
 *
 * Gets the position to seek in seconds.
 *
 * Returns: the position, or -1.0
 */
gdouble
ogmrip_codec_get_start_position (OGMRipCodec *codec)
{
  g_return_val_if_fail (OGMRIP_IS_CODEC (codec), -1);

  return codec->priv->start_second;
}

