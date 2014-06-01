/* OGMRip - A library for media ripping and encoding
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

/**
 * SECTION:ogmrip-codec
 * @title: OGMRipCodec
 * @short_description: Base class for codecs
 * @include: ogmrip-codec.h
 */

#include "ogmrip-codec.h"
#include "ogmrip-stub.h"
#include "ogmrip-fs.h"

#include <unistd.h>
#include <glib/gstdio.h>

struct _OGMRipCodecPriv
{
  OGMRipTitle *title;
  OGMRipStream *input;
  OGMRipFile *output;
  gboolean internal;

  guint start_chap;
  gint end_chap;

  gdouble start_position;
  gdouble play_length;
};

enum 
{
  PROP_0,
  PROP_INPUT,
  PROP_OUTPUT,
  PROP_START_CHAPTER,
  PROP_END_CHAPTER,
  PROP_PLAY_LENGTH,
  PROP_START_POSITION
};

GQuark
ogmrip_codec_error_quark (void)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_static_string ("ogmrip-codec-error-quark");

  return quark;
}

static void
ogmrip_codec_set_input (OGMRipCodec *codec, OGMRipStream *input)
{
  if (input != codec->priv->input)
  {
    if (codec->priv->input)
    {
      g_return_if_fail (ogmrip_stream_is_copy (codec->priv->input, input));

      g_object_unref (codec->priv->input);
      g_object_unref (codec->priv->title);
    }

    codec->priv->input = g_object_ref (input);
    codec->priv->title = g_object_ref (ogmrip_stream_get_title (input));
  }
}

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (OGMRipCodec, ogmrip_codec, OGMJOB_TYPE_BIN)

static void
ogmrip_codec_constructed (GObject *gobject)
{
  OGMRipCodec *codec = OGMRIP_CODEC (gobject);
  gchar *filename, *uri;

  if (!codec->priv->input)
    g_error ("No input stream specified");

  filename = ogmrip_fs_mktemp ("ogmrip.XXXXXX", NULL);
  uri = g_filename_to_uri (filename, NULL, NULL);
  g_free (filename);

  codec->priv->internal = TRUE;
  codec->priv->output = ogmrip_stub_new (codec, uri);
  g_free (uri);

  G_OBJECT_CLASS (ogmrip_codec_parent_class)->constructed (gobject);
}

static void
ogmrip_codec_dispose (GObject *gobject)
{
  OGMRipCodec *codec = OGMRIP_CODEC (gobject);

  if (codec->priv->input)
  {
    g_object_unref (codec->priv->input);
    codec->priv->input = NULL;
  }

  if (codec->priv->output && codec->priv->internal)
  {
    ogmrip_file_delete (codec->priv->output, NULL);
    g_object_unref (codec->priv->output);
    codec->priv->output = NULL;
  }

  if (codec->priv->title)
  {
    g_object_unref (codec->priv->title);
    codec->priv->title = NULL;
  }

  G_OBJECT_CLASS (ogmrip_codec_parent_class)->dispose (gobject);
}

#ifdef G_ENABLE_DEBUG
static void
ogmrip_codec_finalize (GObject *gobject)
{
  g_debug ("Finalizing %s", G_OBJECT_TYPE_NAME (gobject));

  G_OBJECT_CLASS (ogmrip_codec_parent_class)->finalize (gobject);
}
#endif

static void
ogmrip_codec_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipCodec *codec = OGMRIP_CODEC (gobject);

  switch (property_id) 
  {
    case PROP_INPUT:
      ogmrip_codec_set_input (codec, g_value_get_object (value));
      break;
    case PROP_OUTPUT:
      ogmrip_codec_set_output (codec, g_value_get_object (value));
      break;
    case PROP_START_CHAPTER: 
      ogmrip_codec_set_chapters (codec, g_value_get_int (value), codec->priv->end_chap);
      break;
    case PROP_END_CHAPTER: 
      ogmrip_codec_set_chapters (codec, codec->priv->start_chap, g_value_get_int (value));
      break;
    case PROP_PLAY_LENGTH:
      ogmrip_codec_set_play_length (codec, g_value_get_double (value));
      break;
    case PROP_START_POSITION:
      ogmrip_codec_set_start_position (codec, g_value_get_double (value));
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
      g_value_set_object (value, codec->priv->input);
      break;
    case PROP_OUTPUT:
      g_value_set_object (value, codec->priv->output);
      break;
    case PROP_START_CHAPTER: 
      g_value_set_int (value, codec->priv->start_chap);
      break;
    case PROP_END_CHAPTER: 
      g_value_set_int (value, codec->priv->end_chap);
      break;
    case PROP_PLAY_LENGTH:
      g_value_set_double (value, codec->priv->play_length);
      break;
    case PROP_START_POSITION:
      g_value_set_double (value, codec->priv->start_position);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static gboolean
ogmrip_codec_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMRipTitle *title = OGMRIP_CODEC (task)->priv->title;
  const gchar *filename;
  gboolean retval;
  gchar *path;

  g_return_val_if_fail (title != NULL, FALSE);
  g_return_val_if_fail (ogmrip_title_is_open (title), FALSE);

  filename = ogmrip_file_get_path (OGMRIP_CODEC (task)->priv->output);

  path = g_path_get_dirname (filename);
  retval = g_access (path, R_OK | W_OK);
  g_free (path);

  if (retval < 0)
  {
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
        "Output directory is not readable and/or writeable");
    return FALSE;
  }

  return OGMJOB_TASK_CLASS (ogmrip_codec_parent_class)->run (task, cancellable, error);
}

static void
ogmrip_codec_class_init (OGMRipCodecClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobTaskClass *task_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_codec_constructed;
  gobject_class->dispose = ogmrip_codec_dispose;
  gobject_class->set_property = ogmrip_codec_set_property;
  gobject_class->get_property = ogmrip_codec_get_property;

#ifdef G_ENABLE_DEBUG
  gobject_class->finalize = ogmrip_codec_finalize;
#endif

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_codec_run;

  g_object_class_install_property (gobject_class, PROP_INPUT, 
        g_param_spec_object ("input", "Input property", "Set input stream", 
           OGMRIP_TYPE_STREAM, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_OUTPUT, 
        g_param_spec_object ("output", "Output property", "Set output file", 
           OGMRIP_TYPE_FILE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_START_CHAPTER, 
        g_param_spec_int ("start-chapter", "Start chapter property", "Set start chapter", 
           0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_END_CHAPTER, 
        g_param_spec_int ("end-chapter", "End chapter property", "Set end chapter", 
           -1, G_MAXINT, -1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PLAY_LENGTH, 
        g_param_spec_double ("play-length", "Play length property", "Get play length", 
           -1.0, G_MAXDOUBLE, -1.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_START_POSITION, 
        g_param_spec_double ("start-position", "Start position property", "Get start position", 
           0.0, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_codec_init (OGMRipCodec *codec)
{
  codec->priv = ogmrip_codec_get_instance_private (codec);

  codec->priv->end_chap = -1;
  codec->priv->play_length = -1.0;
}

/**
 * ogmrip_codec_get_output:
 * @codec: an #OGMRipCodec
 *
 * Gets the output file.
 *
 * Returns: an #OGMRipFile, or NULL
 */
OGMRipFile *
ogmrip_codec_get_output (OGMRipCodec *codec)
{
  g_return_val_if_fail (OGMRIP_IS_CODEC (codec), NULL);

  return codec->priv->output;
}

void
ogmrip_codec_set_output (OGMRipCodec *codec, OGMRipFile *file)
{
  g_return_if_fail (OGMRIP_IS_CODEC (codec));
  g_return_if_fail (OGMRIP_IS_FILE (file));

  if (codec->priv->output != file)
  {
    if (codec->priv->output)
      g_object_unref (codec->priv->output);
    codec->priv->output = g_object_ref (file);
    codec->priv->internal = FALSE;

    g_object_notify (G_OBJECT (codec), "output");
  }
}

/**
 * ogmrip_codec_get_input:
 * @codec: an #OGMRipCodec
 *
 * Gets the input stream.
 *
 * Returns: an #OGMRipStream, or NULL
 */
OGMRipStream *
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
  gint nchap;

  g_return_if_fail (OGMRIP_IS_CODEC (codec));

  nchap = ogmrip_title_get_n_chapters (codec->priv->title);

  if (end < 0)
    end = nchap - 1;

  codec->priv->start_chap = MIN ((gint) start, nchap - 1);
  codec->priv->end_chap = CLAMP (end, (gint) start, nchap - 1);
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
    *end = ogmrip_title_get_n_chapters (codec->priv->title) - 1;
  else
    *end = codec->priv->end_chap;
}

/**
 * ogmrip_codec_get_length:
 * @codec: an #OGMRipCodec
 * @length: a pointer to store an #OGMRipTime, or NULL
 *
 * Returns the length of the encoding in seconds. If @length is not NULL, the
 * data structure will be filled with the length in hours, minutes seconds and
 * frames.
 *
 * Returns: the length in seconds, or -1.0
 */
gdouble
ogmrip_codec_get_length (OGMRipCodec *codec, OGMRipTime *time_)
{
  gint nchap;

  g_return_val_if_fail (OGMRIP_IS_CODEC (codec), -1.0);

  if (!codec->priv->title)
    return -1.0;

  if (codec->priv->play_length > 0.0)
  {
    if (time_)
      ogmrip_msec_to_time (codec->priv->play_length * 1000, time_);

    return codec->priv->play_length;
  }

  nchap = ogmrip_title_get_n_chapters (codec->priv->title);

  if (codec->priv->start_chap == 0 &&
      (codec->priv->end_chap == -1 ||
       codec->priv->end_chap == nchap - 1))
    return ogmrip_title_get_length (codec->priv->title, time_);

  return ogmrip_title_get_chapters_length (codec->priv->title,
      codec->priv->start_chap, codec->priv->end_chap, time_);
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

  codec->priv->play_length = length;
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

  codec->priv->start_position = start;
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

  return codec->priv->start_position;
}

