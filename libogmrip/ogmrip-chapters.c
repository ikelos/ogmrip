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
 * SECTION:ogmrip-chapters
 * @title: OGMRipChapters
 * @short_description: A codec to extract chapters information
 * @include: ogmrip-chapters.h
 */

#include "ogmrip-chapters.h"

#define OGMRIP_CHAPTERS_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_CHAPTERS, OGMRipChaptersPriv))

static void ogmrip_chapters_finalize (GObject     *gobject);
static gint ogmrip_chapters_run      (OGMJobSpawn *spawn);

struct _OGMRipChaptersPriv
{
  gint nchapters;
  gchar **labels;
};

G_DEFINE_TYPE (OGMRipChapters, ogmrip_chapters, OGMRIP_TYPE_CODEC)

static void
ogmrip_chapters_class_init (OGMRipChaptersClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  spawn_class = OGMJOB_SPAWN_CLASS (klass);

  gobject_class->finalize = ogmrip_chapters_finalize;
  spawn_class->run = ogmrip_chapters_run;

  g_type_class_add_private (klass, sizeof (OGMRipChaptersPriv));
}

static void
ogmrip_chapters_init (OGMRipChapters *chapters)
{
  chapters->priv = OGMRIP_CHAPTERS_GET_PRIVATE (chapters);
}

static void
ogmrip_chapters_finalize (GObject *gobject)
{
  OGMRipChapters *chapters;

  chapters = OGMRIP_CHAPTERS (gobject);
  if (chapters->priv->labels)
  {
    gint i;

    for (i = 0; i < chapters->priv->nchapters; i++)
      g_free (chapters->priv->labels[i]);

    g_free (chapters->priv->labels);
    chapters->priv->labels = NULL;
  }

  G_OBJECT_CLASS (ogmrip_chapters_parent_class)->finalize (gobject);
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

static gint
ogmrip_chapters_run (OGMJobSpawn *spawn)
{
  GIOChannel *channel;
  OGMDvdTitle *title;

  const gchar *output;
  guint numerator, denominator;
  guint start_chapter, end_chapter;
  gdouble seconds, length;
  gint i;

  output = ogmrip_codec_get_output (OGMRIP_CODEC (spawn));
  channel = g_io_channel_new_file (output, "w", NULL);
  if (!channel)
    return OGMJOB_RESULT_ERROR;

  ogmrip_codec_get_chapters (OGMRIP_CODEC (spawn), &start_chapter, &end_chapter);

  title = ogmrip_codec_get_input (OGMRIP_CODEC (spawn));
  ogmdvd_title_get_framerate (title, &numerator, &denominator);

  for (i = start_chapter, seconds = length = 0.0; i <= end_chapter; i++)
  {
    length += seconds;

    if (i < end_chapter)
      seconds = ogmdvd_title_get_chapters_length (title, i, i, NULL);

    ogmrip_chapters_save (OGMRIP_CHAPTERS (spawn), 
        channel, i - start_chapter + 1, OGMRIP_CHAPTERS (spawn)->priv->labels[i], length * 1000);
  }

  g_io_channel_shutdown (channel, TRUE, NULL);
  g_io_channel_unref (channel);

  return OGMJOB_RESULT_SUCCESS;
}

/**
 * ogmrip_chapters_new:
 * @title: An #OGMDvdTitle
 * @output: The output file
 *
 * Creates a new #OGMRipChapters.
 *
 * Returns: The new #OGMRipChapters
 */
OGMJobSpawn *
ogmrip_chapters_new (OGMDvdTitle *title, const gchar *output)
{
  OGMRipChapters *chapters;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (output && *output, NULL);

  chapters = g_object_new (OGMRIP_TYPE_CHAPTERS, "input", title, "output", output, NULL);

  if (chapters->priv->labels)
  {
    gint i;

    for (i = 0; i < chapters->priv->nchapters; i++)
      g_free (chapters->priv->labels[i]);
    g_free (chapters->priv->labels);
  }

  chapters->priv->nchapters = ogmdvd_title_get_n_chapters (title);
  if (chapters->priv->nchapters > 0)
    chapters->priv->labels = g_new0 (gchar *, chapters->priv->nchapters);

  return OGMJOB_SPAWN (chapters);
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
G_CONST_RETURN gchar *
ogmrip_chapters_get_label (OGMRipChapters *chapters, guint n)
{
  g_return_val_if_fail (OGMRIP_IS_CHAPTERS (chapters), NULL);
  g_return_val_if_fail (n < chapters->priv->nchapters, NULL);

  return chapters->priv->labels[n];
}

