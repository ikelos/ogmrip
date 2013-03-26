/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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
 * SECTION:ogmrip-chapters
 * @title: OGMRipChapters
 * @short_description: A codec to extract chapters information
 * @include: ogmrip-chapters.h
 */

#include "ogmrip-chapters.h"

#define OGMRIP_CHAPTERS_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_CHAPTERS, OGMRipChaptersPriv))

static void     ogmrip_chapters_constructed  (GObject      *gobject);
static void     ogmrip_chapters_finalize     (GObject      *gobject);
static void     ogmrip_chapters_set_property (GObject      *gobject,
                                              guint        property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void     ogmrip_chapters_get_property (GObject      *gobject,
                                              guint        property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);
static gboolean ogmrip_chapters_run          (OGMJobTask   *task,
                                              GCancellable *cancellable,
                                              GError       **error);

struct _OGMRipChaptersPriv
{
  gint nchapters;
  gchar **labels;
  guint language;
};

enum 
{
  PROP_0,
  PROP_LANGUAGE
};

G_DEFINE_TYPE (OGMRipChapters, ogmrip_chapters, OGMRIP_TYPE_CODEC)

static void
ogmrip_chapters_class_init (OGMRipChaptersClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobTaskClass *task_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_chapters_constructed;
  gobject_class->finalize = ogmrip_chapters_finalize;
  gobject_class->get_property = ogmrip_chapters_get_property;
  gobject_class->set_property = ogmrip_chapters_set_property;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_chapters_run;

  g_object_class_install_property (gobject_class, PROP_LANGUAGE, 
        g_param_spec_uint ("language", "Language property", "Set language", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipChaptersPriv));
}

static void
ogmrip_chapters_init (OGMRipChapters *chapters)
{
  chapters->priv = OGMRIP_CHAPTERS_GET_PRIVATE (chapters);
}

static void
ogmrip_chapters_constructed (GObject *gobject)
{
  OGMRipChapters *chapters = OGMRIP_CHAPTERS (gobject);
  OGMRipStream *stream;

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (chapters));

  if (!OGMRIP_IS_VIDEO_STREAM (stream))
    g_error ("No video stream specified"); 

  chapters->priv->nchapters = ogmrip_title_get_n_chapters (ogmrip_stream_get_title (stream));
  if (chapters->priv->nchapters > 0)
    chapters->priv->labels = g_new0 (gchar *, chapters->priv->nchapters + 1);

  G_OBJECT_CLASS (ogmrip_chapters_parent_class)->constructed (gobject);
}

static void
ogmrip_chapters_finalize (GObject *gobject)
{
  OGMRipChapters *chapters = OGMRIP_CHAPTERS (gobject);

  if (chapters->priv->labels)
  {
    g_strfreev (chapters->priv->labels);
    chapters->priv->labels = NULL;
  }

  G_OBJECT_CLASS (ogmrip_chapters_parent_class)->finalize (gobject);
}

static void
ogmrip_chapters_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipChapters *chapters = OGMRIP_CHAPTERS (gobject);

  switch (property_id) 
  {
    case PROP_LANGUAGE: 
      chapters->priv->language = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_chapters_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipChapters *chapters = OGMRIP_CHAPTERS (gobject);

  switch (property_id) 
  {
    case PROP_LANGUAGE:
      g_value_set_uint (value, chapters->priv->language);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_chapters_save (OGMRipChapters *chapters, GIOChannel *channel, guint n, const gchar *label, gulong length)
{
  gchar *str;

  str = g_strdup_printf ("CHAPTER%02d=%02lu:%02lu:%02lu.%03lu\n", n, 
      length / (60 * 60 * 1000), length / (60 * 1000) % 60, length / 1000 % 60, length % 1000);
  g_io_channel_write_chars (channel, str, -1, NULL, NULL);
  g_free (str);

  if (label)
    str = g_strdup_printf ("CHAPTER%02dNAME=%s\n", n, label);
  else
    str = g_strdup_printf ("CHAPTER%02dNAME=Chapter %02d\n", n, n);

  g_io_channel_write_chars (channel, str, -1, NULL, NULL);

  g_free (str);
}

static gboolean
ogmrip_chapters_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  GIOChannel *channel;
  OGMRipStream *stream;

  const gchar *output;
  guint start_chapter, end_chapter;
  gdouble seconds, length;
  gint i;

  output = ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (task)));
  channel = g_io_channel_new_file (output, "w", error);
  if (!channel)
    return FALSE;

  ogmrip_codec_get_chapters (OGMRIP_CODEC (task), &start_chapter, &end_chapter);

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (task));

  for (i = start_chapter, seconds = length = 0.0; i <= end_chapter; i++)
  {
    length += seconds;

    if (i < end_chapter)
      seconds = ogmrip_title_get_chapters_length (ogmrip_stream_get_title (stream), i, i, NULL);

    ogmrip_chapters_save (OGMRIP_CHAPTERS (task), 
        channel, i - start_chapter + 1, OGMRIP_CHAPTERS (task)->priv->labels[i], length * 1000);
  }

  g_io_channel_shutdown (channel, TRUE, NULL);
  g_io_channel_unref (channel);

  return TRUE;
}

/**
 * ogmrip_chapters_new:
 * @stream: An #OGMRipVideoStream
 *
 * Creates a new #OGMRipChapters.
 *
 * Returns: The new #OGMRipChapters
 */
OGMRipCodec *
ogmrip_chapters_new (OGMRipVideoStream *stream)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_STREAM (stream), NULL);

  return g_object_new (OGMRIP_TYPE_CHAPTERS, "input", stream, NULL);
}

/**
 * ogmrip_chapters_set_label:
 * @chapters: An #OGMRipChapters
 * @n: A chapter number
 * @label: A label
 *
 * Sets the label this chapter.
 */
void
ogmrip_chapters_set_label (OGMRipChapters *chapters, guint n, const gchar *label)
{
  g_return_if_fail (OGMRIP_IS_CHAPTERS (chapters));
  g_return_if_fail (n < chapters->priv->nchapters);

  if (chapters->priv->labels[n])
    g_free (chapters->priv->labels[n]);
  chapters->priv->labels[n] = NULL;

  if (label)
    chapters->priv->labels[n] = g_strdup (label);
}

/**
 * ogmrip_chapters_get_label:
 * @chapters: An #OGMRipChapters
 * @n: A chapter number
 *
 * Returns the label of this chapter.
 *
 * Returns: The label
 */
const gchar *
ogmrip_chapters_get_label (OGMRipChapters *chapters, guint n)
{
  g_return_val_if_fail (OGMRIP_IS_CHAPTERS (chapters), NULL);
  g_return_val_if_fail (n < chapters->priv->nchapters, NULL);

  return chapters->priv->labels[n];
}

/**
 * ogmrip_chapters_set_language:
 * @chapters: an #OGMRipChapters
 * @language: the language
 *
 * Sets the language of the chapters.
 */
void
ogmrip_chapters_set_language (OGMRipChapters *chapters, guint language)
{
  g_return_if_fail (OGMRIP_IS_CHAPTERS (chapters));

  chapters->priv->language = language;

  g_object_notify (G_OBJECT (chapters), "language");
}

/**
 * ogmrip_chapters_get_language:
 * @chapters: an #OGMRipChapters
 *
 * Gets the language of the chapters.
 *
 * Returns: the language
 */
gint
ogmrip_chapters_get_language (OGMRipChapters *chapters)
{
  g_return_val_if_fail (OGMRIP_IS_CHAPTERS (chapters), -1);

  return chapters->priv->language;
}

