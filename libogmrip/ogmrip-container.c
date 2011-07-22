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
 * SECTION:ogmrip-container
 * @title: OGMRipContainer
 * @short_description: Base class for containers
 * @include: ogmrip-container.h
 */

#include "ogmrip-container.h"
#include "ogmrip-plugin.h"
#include "ogmjob-exec.h"

#include <unistd.h>
#include <glib/gstdio.h>

#define DEFAULT_OVERHEAD 6

#define OGMRIP_CONTAINER_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_CONTAINER, OGMRipContainerPriv))

struct _OGMRipContainerPriv
{
  gchar *label;
  gchar *output;
  gchar *fourcc;

  guint tsize;
  guint tnumber;
  guint start_delay;

  GSList *subp;
  GSList *audio;
  GSList *chapters;
  GSList *files;

  OGMRipVideoCodec *video;
};

typedef struct
{
  OGMRipCodec *codec;
  gint language;
  guint demuxer;
} OGMRipContainerChild;

enum 
{
  PROP_0,
  PROP_OUTPUT,
  PROP_LABEL,
  PROP_FOURCC,
  PROP_TSIZE,
  PROP_TNUMBER,
  PROP_OVERHEAD,
  PROP_START_DELAY
};

static void ogmrip_container_dispose      (GObject      *gobject);
static void ogmrip_container_finalize     (GObject      *gobject);
static void ogmrip_container_set_property (GObject      *gobject,
                                           guint        property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec);
static void ogmrip_container_get_property (GObject      *gobject,
                                           guint        property_id,
                                           GValue       *value,
                                           GParamSpec   *pspec);

G_DEFINE_ABSTRACT_TYPE (OGMRipContainer, ogmrip_container, OGMJOB_TYPE_BIN)

static void
ogmrip_container_class_init (OGMRipContainerClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  spawn_class = OGMJOB_SPAWN_CLASS (klass);

  gobject_class->dispose  = ogmrip_container_dispose;
  gobject_class->finalize = ogmrip_container_finalize;
  gobject_class->set_property = ogmrip_container_set_property;
  gobject_class->get_property = ogmrip_container_get_property;

  g_object_class_install_property (gobject_class, PROP_OUTPUT, 
        g_param_spec_string ("output", "Output property", "Set output file", 
           NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LABEL, 
        g_param_spec_string ("label", "Label property", "Set label", 
           NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FOURCC, 
        g_param_spec_string ("fourcc", "FourCC property", "Set fourcc", 
           NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TSIZE, 
        g_param_spec_uint ("target-size", "Target size property", "Set target size", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TNUMBER, 
        g_param_spec_uint ("target-number", "Target number property", "Set target number", 
           0, G_MAXUINT, 1, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_OVERHEAD, 
        g_param_spec_uint ("overhead", "Overhead property", "Get overhead", 
           0, G_MAXUINT, 6, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_START_DELAY, 
        g_param_spec_uint ("start-delay", "Start delay property", "Set start delay", 
           0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipContainerPriv));
}

static void
ogmrip_container_init (OGMRipContainer *container)
{
  container->priv = OGMRIP_CONTAINER_GET_PRIVATE (container);
  container->priv->tnumber = 1;
  container->priv->tsize = 0;
}

static void
ogmrip_stream_free (OGMRipContainerChild *child)
{
  if (child->codec)
    g_object_unref (child->codec);

  g_free (child);
}

static void
ogmrip_container_dispose (GObject *gobject)
{
  OGMRipContainer *container;

  container = OGMRIP_CONTAINER (gobject);

  if (container->priv->video)
    g_object_unref (container->priv->video);
  container->priv->video = NULL;

  g_slist_foreach (container->priv->audio, (GFunc) ogmrip_stream_free, NULL);
  g_slist_free (container->priv->audio);
  container->priv->audio = NULL;

  g_slist_foreach (container->priv->chapters, (GFunc) ogmrip_stream_free, NULL);
  g_slist_free (container->priv->chapters);
  container->priv->chapters = NULL;

  g_slist_foreach (container->priv->subp, (GFunc) ogmrip_stream_free, NULL);
  g_slist_free (container->priv->subp);
  container->priv->subp = NULL;

  g_slist_foreach (container->priv->files, (GFunc) ogmrip_file_unref, NULL);
  g_slist_free (container->priv->files);
  container->priv->files = NULL;
}

static void
ogmrip_container_finalize (GObject *gobject)
{
  OGMRipContainer *container;

  container = OGMRIP_CONTAINER (gobject);

  g_free (container->priv->label);
  container->priv->label = NULL;

  g_free (container->priv->output);
  container->priv->output = NULL;

  g_free (container->priv->fourcc);
  container->priv->fourcc = NULL;

  G_OBJECT_CLASS (ogmrip_container_parent_class)->finalize (gobject);
}

static void
ogmrip_container_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipContainer *container;

  container = OGMRIP_CONTAINER (gobject);

  switch (property_id) 
  {
    case PROP_OUTPUT:
      container->priv->output = g_value_dup_string (value);
      break;
    case PROP_LABEL: 
      ogmrip_container_set_label (container, g_value_get_string (value));
      break;
    case PROP_FOURCC: 
      ogmrip_container_set_fourcc (container, g_value_get_string (value));
      break;
    case PROP_TSIZE: 
      container->priv->tsize = g_value_get_uint (value);
      break;
    case PROP_TNUMBER: 
      container->priv->tnumber = g_value_get_uint (value);
      break;
    case PROP_START_DELAY: 
      container->priv->start_delay = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_container_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipContainer *container;
  gint overhead;

  container = OGMRIP_CONTAINER (gobject);

  switch (property_id) 
  {
    case PROP_OUTPUT:
      g_value_set_string (value, container->priv->output);
      break;
    case PROP_LABEL:
      g_value_set_string (value, container->priv->label);
      break;
    case PROP_FOURCC:
      g_value_set_string (value, container->priv->fourcc);
      break;
    case PROP_TSIZE:
      g_value_set_uint (value, container->priv->tsize);
      break;
    case PROP_TNUMBER:
      g_value_set_uint (value, container->priv->tnumber);
      break;
    case PROP_OVERHEAD:
      overhead = ogmrip_container_get_overhead (container);
      g_value_set_uint (value, overhead > 0 ? overhead : 6);
      break;
    case PROP_START_DELAY:
      g_value_set_uint (value, container->priv->start_delay);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static GSList *
ogmrip_container_copy_list (GSList *list)
{
  OGMRipContainerChild *child;
  GSList *new_list = NULL;

  while (list)
  {
    child = list->data;
    new_list = g_slist_append (new_list, child->codec);
    list = list->next;
  }

  return new_list;
}

static void
ogmrip_container_foreach_codec (OGMRipContainer *container, GSList *list, OGMRipContainerCodecFunc func, gpointer data)
{
  OGMRipContainerChild *child;
  GSList *next;

  while (list)
  {
    next = list->next;

    child = list->data;
    (* func) (container, child->codec, child->demuxer, child->language, data);
    list = next;
  }
}

/**
 * ogmrip_container_set_profile:
 * @container: An #OGMRipContainer
 * @profile: An #OGMRipProfile
 *
 * Sets container specific options from the specified profile.
 */
void
ogmrip_container_set_profile (OGMRipContainer *container, OGMRipProfile *profile)
{
  OGMRipContainerClass *klass;

  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (G_IS_SETTINGS (profile));

  klass = OGMRIP_CONTAINER_GET_CLASS (container);
  if (klass->set_profile)
    (* klass->set_profile) (container, profile);
}

/**
 * ogmrip_container_get_output:
 * @container: An #OGMRipContainer
 *
 * Gets the name of the output file.
 *
 * Returns: The filename, or NULL
 */
G_CONST_RETURN gchar *
ogmrip_container_get_output (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  return container->priv->output;
}

/**
 * ogmrip_container_get_label:
 * @container: An #OGMRipContainer
 *
 * Gets the label of the rip.
 *
 * Returns: The label, or NULL
 */
G_CONST_RETURN gchar *
ogmrip_container_get_label (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  return container->priv->label;
}

/**
 * ogmrip_container_set_label:
 * @container: An #OGMRipContainer
 * @label: the label
 *
 * Sets the label of the rip.
 */
void
ogmrip_container_set_label (OGMRipContainer *container, const gchar *label)
{
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));

  g_free (container->priv->label);
  container->priv->label = label ? g_strdup (label) : NULL;
}

/**
 * ogmrip_container_get_fourcc:
 * @container: An #OGMRipContainer
 *
 * Gets the FourCC of the rip.
 *
 * Returns: The FourCC, or NULL
 */
G_CONST_RETURN gchar *
ogmrip_container_get_fourcc (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  return container->priv->fourcc;
}

/**
 * ogmrip_container_set_fourcc:
 * @container: An #OGMRipContainer
 * @fourcc: the FourCC
 *
 * Sets the FourCC of the rip.
 */
void
ogmrip_container_set_fourcc (OGMRipContainer *container, const gchar *fourcc)
{
  gchar *str;

  g_return_if_fail (OGMRIP_IS_CONTAINER (container));

  if (container->priv->fourcc)
    g_free (container->priv->fourcc);
  container->priv->fourcc = NULL;

  if (fourcc)
  {
    str = g_utf8_strup (fourcc, -1);
    container->priv->fourcc = g_strndup (str, 4);
    g_free (str);
  }
}

/**
 * ogmrip_container_get_start_delay:
 * @container: An #OGMRipContainer
 *
 * Gets the start delay of the audio tracks.
 *
 * Returns: The start delay, or -1
 */
gint
ogmrip_container_get_start_delay (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), -1);

  return container->priv->start_delay;
}

/**
 * ogmrip_container_set_start_delay:
 * @container: An #OGMRipContainer
 * @start_delay: the start delay
 *
 * Sets the start delay of the audio tracks
 */
void
ogmrip_container_set_start_delay (OGMRipContainer *container, guint start_delay)
{
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));

  container->priv->start_delay = start_delay;
}

/**
 * ogmrip_container_get_overhead:
 * @container: An #OGMRipContainer
 *
 * Gets the overhead of the container.
 *
 * Returns: The overhead, or -1
 */
gint
ogmrip_container_get_overhead (OGMRipContainer *container)
{
  OGMRipContainerClass *klass;

  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), -1);

  klass = OGMRIP_CONTAINER_GET_CLASS (container);
  if (klass->get_overhead)
    return (* klass->get_overhead) (container);

  return DEFAULT_OVERHEAD;
}

/**
 * ogmrip_container_get_video:
 * @container: An #OGMRipContainer
 *
 * Gets the video codec of the rip.
 *
 * Returns: An #OGMRipVideoCodec, or NULL
 */
OGMRipVideoCodec *
ogmrip_container_get_video (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  return container->priv->video;
}

/**
 * ogmrip_container_set_video:
 * @container: An #OGMRipContainer
 * @video: An #OGMRipVideoCodec
 *
 * Sets the video codec of the rip.
 */
void
ogmrip_container_set_video (OGMRipContainer *container, OGMRipVideoCodec *video)
{
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (video));

  g_object_ref (video);
  if (container->priv->video)
    g_object_unref (container->priv->video);
  container->priv->video = video;

  if (!ogmrip_plugin_get_container_bframes (G_TYPE_FROM_INSTANCE (container)))
    ogmrip_video_codec_set_max_b_frames (video, 0);
}

/**
 * ogmrip_container_add_audio:
 * @container: An #OGMRipContainer
 * @audio: An #OGMRipAudioCodec
 * @demuxer: The demuxer to be used
 * @language: The language of the stream
 *
 * Adds an audio codec to the rip.
 */
void
ogmrip_container_add_audio (OGMRipContainer *container, OGMRipAudioCodec *audio, OGMRipAudioDemuxer demuxer, gint language)
{
  OGMRipContainerChild *child;

  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (OGMRIP_IS_AUDIO_CODEC (audio));

  child = g_new0 (OGMRipContainerChild, 1);

  g_object_ref (audio);
  child->codec = OGMRIP_CODEC (audio);

  child->language = language;
  child->demuxer = demuxer;

  container->priv->audio = g_slist_append (container->priv->audio, child);
}

/**
 * ogmrip_container_remove_audio:
 * @container: An #OGMRipContainer
 * @audio: An #OGMRipAudioCodec
 *
 * Removes the audio codec from the rip.
 */
void
ogmrip_container_remove_audio (OGMRipContainer *container, OGMRipAudioCodec *audio)
{
  OGMRipContainerChild *child;
  GSList *link;

  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (OGMRIP_IS_AUDIO_CODEC (audio));

  for (link = container->priv->audio; link; link = link->next)
  {
    child = link->data;

    if (child->codec == OGMRIP_CODEC (audio))
      break;
  }

  if (link)
  {
    container->priv->audio = g_slist_remove_link (container->priv->audio, link);
    ogmrip_stream_free (link->data);
    g_slist_free (link);
  }
}

/**
 * ogmrip_container_get_audio:
 * @container: An #OGMRipContainer
 *
 * Gets a list of the audio codecs of the rip.
 *
 * Returns: A #GSList, or NULL
 */
GSList *
ogmrip_container_get_audio (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  return ogmrip_container_copy_list (container->priv->audio);
}

/**
 * ogmrip_container_get_nth_audio:
 * @container: an #OGMRipContainer
 * @n: The index of the audio codec
 *
 * Gets the audio codec at the given position.
 *
 * Returns: An #OGMRipAudioCodec, or NULL
 */
OGMRipAudioCodec *
ogmrip_container_get_nth_audio (OGMRipContainer *container, gint n)
{
  OGMRipContainerChild *child;
  GSList *link;

  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  if (n < 0)
    link = g_slist_last (container->priv->audio);
  else
    link = g_slist_nth (container->priv->audio, n);

  if (!link)
    return NULL;

  child = link->data;

  return OGMRIP_AUDIO_CODEC (child->codec);
}

/**
 * ogmrip_container_get_n_audio:
 * @container: an #OGMRipContainer
 *
 * Gets the number of audio codecs.
 *
 * Returns: the number of audio codecs
 */
gint
ogmrip_container_get_n_audio (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), -1);

  return g_slist_length (container->priv->audio);
}

/**
 * ogmrip_container_foreach_audio:
 * @container: an #OGMRipContainer
 * @func: The function to call with each audio codec
 * @data: User data to pass to the function
 *
 * Calls a function for each audio codec
 */
void
ogmrip_container_foreach_audio (OGMRipContainer *container, OGMRipContainerCodecFunc func, gpointer data)
{
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (func != NULL);

  ogmrip_container_foreach_codec (container, container->priv->audio, func, data);
}

/**
 * ogmrip_container_add_subp:
 * @container: An #OGMRipContainer
 * @subp: An #OGMRipSubpCodec
 * @demuxer: The demuxer to be used
 * @language: The language of the stream
 *
 * Adds a subtitle codec to the rip.
 */
void
ogmrip_container_add_subp (OGMRipContainer *container, OGMRipSubpCodec *subp, OGMRipSubpDemuxer demuxer, gint language)
{
  OGMRipContainerChild *child;

  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (OGMRIP_IS_SUBP_CODEC (subp));

  child = g_new0 (OGMRipContainerChild, 1);

  g_object_ref (subp);
  child->codec = OGMRIP_CODEC (subp);

  child->language = language;
  child->demuxer = demuxer;

  container->priv->subp = g_slist_append (container->priv->subp, child);
}

/**
 * ogmrip_container_remove_subp:
 * @container: An #OGMRipContainer
 * @subp: An #OGMRipSubpCodec
 *
 * Removes the subp codec from the rip.
 */
void
ogmrip_container_remove_subp (OGMRipContainer *container, OGMRipSubpCodec *subp)
{
  OGMRipContainerChild *child;
  GSList *link;

  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (OGMRIP_IS_SUBP_CODEC (subp));

  for (link = container->priv->subp; link; link = link->next)
  {
    child = link->data;

    if (child->codec == OGMRIP_CODEC (subp))
      break;
  }

  if (link)
  {
    container->priv->subp = g_slist_remove_link (container->priv->subp, link);
    ogmrip_stream_free (link->data);
    g_slist_free (link);
  }
}

/**
 * ogmrip_container_get_subp:
 * @container: An #OGMRipContainer
 *
 * Gets a list of the subtitle codecs of the rip.
 *
 * Returns: A #GSList, or NULL
 */
GSList *
ogmrip_container_get_subp (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  return ogmrip_container_copy_list (container->priv->subp);
}

/**
 * ogmrip_container_get_nth_subp:
 * @container: an #OGMRipContainer
 * @n: The index of the subtitle codec
 *
 * Gets the subtitle codec at the given position.
 *
 * Returns: An #OGMRipSubpCodec, or NULL
 */
OGMRipSubpCodec *
ogmrip_container_get_nth_subp (OGMRipContainer *container, gint n)
{
  OGMRipContainerChild *child;
  GSList *link;

  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  if (n < 0)
    link = g_slist_last (container->priv->subp);
  else
    link = g_slist_nth (container->priv->subp, n);

  if (!link)
    return NULL;

  child = link->data;

  return OGMRIP_SUBP_CODEC (child->codec);
}

/**
 * ogmrip_container_get_n_subp:
 * @container: an #OGMRipContainer
 *
 * Gets the number of subtitle codecs.
 *
 * Returns: the number of subtitle codecs
 */
gint
ogmrip_container_get_n_subp (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), -1);

  return g_slist_length (container->priv->subp);
}

/**
 * ogmrip_container_foreach_subp:
 * @container: An #OGMRipContainer
 * @func: The function to call with each subtitle codec
 * @data: User data to pass to the function
 *
 * Calls a function for each subtitle codec
 */
void
ogmrip_container_foreach_subp (OGMRipContainer *container, OGMRipContainerCodecFunc func, gpointer data)
{
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (func != NULL);

  ogmrip_container_foreach_codec (container, container->priv->subp, func, data);
}

/**
 * ogmrip_container_add_chapters:
 * @container: An #OGMRipContainer
 * @chapters: An #OGMRipChapters
 * @language: The language of the chapters
 *
 * Adds a chapters codec to the rip.
 */
void
ogmrip_container_add_chapters (OGMRipContainer *container, OGMRipChapters *chapters, gint language)
{
  OGMRipContainerChild *child;

  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (OGMRIP_IS_CHAPTERS (chapters));

  child = g_new0 (OGMRipContainerChild, 1);

  g_object_ref (chapters);
  child->codec = OGMRIP_CODEC (chapters);
  child->language = language;

  container->priv->chapters = g_slist_append (container->priv->chapters, child);
}

/**
 * ogmrip_container_remove_chapters:
 * @container: An #OGMRipContainer
 * @chapters: An #OGMRipChaptersCodec
 *
 * Removes the chapters from the rip.
 */
void
ogmrip_container_remove_chapters (OGMRipContainer *container, OGMRipChapters *chapters)
{
  OGMRipContainerChild *child;
  GSList *link;

  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (OGMRIP_IS_CHAPTERS (chapters));

  for (link = container->priv->chapters; link; link = link->next)
  {
    child = link->data;

    if (child->codec == OGMRIP_CODEC (chapters))
      break;
  }

  if (link)
  {
    container->priv->chapters = g_slist_remove_link (container->priv->chapters, link);
    ogmrip_stream_free (link->data);
    g_slist_free (link);
  }
}

/**
 * ogmrip_container_get_chapters:
 * @container: An #OGMRipContainer
 *
 * Gets a list of the chapters codecs of the rip.
 *
 * Returns: A #GSList, or NULL
 */
GSList *
ogmrip_container_get_chapters (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  return ogmrip_container_copy_list (container->priv->chapters);
}

/**
 * ogmrip_container_get_nth_chapters:
 * @container: an #OGMRipContainer
 * @n: The index of the chapters codec
 *
 * Gets the chapters codec at the given position.
 *
 * Returns: An #OGMRipChapters, or NULL
 */
OGMRipChapters *
ogmrip_container_get_nth_chapters (OGMRipContainer *container, gint n)
{
  OGMRipContainerChild *child;
  GSList *link;

  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  if (n < 0)
    link = g_slist_last (container->priv->chapters);
  else
    link = g_slist_nth (container->priv->chapters, n);

  if (!link)
    return NULL;

  child = link->data;

  return OGMRIP_CHAPTERS (child->codec);
}

/**
 * ogmrip_container_get_n_chapters:
 * @container: an #OGMRipContainer
 *
 * Gets the number of chapters codecs.
 *
 * Returns: the number of chapters codecs
 */
gint
ogmrip_container_get_n_chapters (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), -1);

  return g_slist_length (container->priv->chapters);
}

/**
 * ogmrip_container_foreach_chapters:
 * @container: An #OGMRipContainer
 * @func: The function to call with each chapters codec
 * @data: User data to pass to the function
 *
 * Calls a function for each chapters codec
 */
void
ogmrip_container_foreach_chapters (OGMRipContainer *container, OGMRipContainerCodecFunc func, gpointer data)
{
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (func != NULL);

  ogmrip_container_foreach_codec (container, container->priv->chapters, func, data);
}

/**
 * ogmrip_container_add_file:
 * @container: An #OGMRipContainer
 * @file: An #OOGMRipFile
 *
 * Adds a file to the rip.
 */
void
ogmrip_container_add_file (OGMRipContainer *container, OGMRipFile *file)
{
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (file != NULL);

  ogmrip_file_ref (file);

  container->priv->files = g_slist_append (container->priv->files, file);
}

/**
 * ogmrip_container_remove_file:
 * @container: An #OGMRipContainer
 * @file: An #OGMRipFile
 *
 * Removes the file from the rip.
 */
void
ogmrip_container_remove_file (OGMRipContainer *container, OGMRipFile *file)
{
  GSList *link;

  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (file != NULL);

  for (link = container->priv->files; link; link = link->next)
    if (OGMRIP_FILE (link->data) == file)
      break;

  if (link)
  {
    container->priv->files = g_slist_remove_link (container->priv->files, link);
    ogmrip_file_unref (file);
    g_slist_free (link);
  }
}

/**
 * ogmrip_container_get_files:
 * @container: An #OGMRipContainer
 *
 * Gets a list of the files of the rip.
 *
 * Returns: A #GSList, or NULL
 */
GSList *
ogmrip_container_get_files (OGMRipContainer *container)
{
  return g_slist_copy (container->priv->files);
}

/**
 * ogmrip_container_get_nth_file:
 * @container: an #OGMRipContainer
 * @n: The index of the file
 *
 * Gets the file at the given position.
 *
 * Returns: An #OGMRipFile, or NULL
 */
OGMRipFile *
ogmrip_container_get_nth_file (OGMRipContainer *container, gint n)
{
  void *file;

  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  if (n < 0)
    file = g_slist_last (container->priv->chapters);
  else
    file = g_slist_nth (container->priv->chapters, n);

  return file;
}

/**
 * ogmrip_container_get_n_files:
 * @container: an #OGMRipContainer
 *
 * Gets the number of files.
 *
 * Returns: the number of files
 */
gint
ogmrip_container_get_n_files (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), -1);

  return g_slist_length (container->priv->files);
}

/**
 * ogmrip_container_foreach_file:
 * @container: An #OGMRipContainer
 * @func: The function to call with each file
 * @data: User data to pass to the function
 *
 * Calls a function for each file
 */
void
ogmrip_container_foreach_file (OGMRipContainer *container, OGMRipContainerFileFunc func, gpointer data)
{
  GSList *link, *next;

  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (func != NULL);
  
  link = container->priv->files;
  while (link)
  {
    next = link->next;
    (* func) (container, OGMRIP_FILE (link->data), data);
    link = next;
  }
}

/**
 * ogmrip_container_set_split:
 * @container: An #OGMRipContainer
 * @number:  The number of file
 * @size: The size of each file
 *
 * Sets the number of output files and the maximum size of each one.
 */
void
ogmrip_container_set_split (OGMRipContainer *container, guint number, guint size)
{
   g_return_if_fail (OGMRIP_IS_CONTAINER (container));

   if (number > 0)
     container->priv->tnumber = number;

   if (size > 0)
     container->priv->tsize = size;
}

/**
 * ogmrip_container_get_split:
 * @container: An #OGMRipContainer
 * @number:  A pointer to store the number of file
 * @size: A pointer to store the size of each file
 *
 * Gets the number of output files and the maximum size of each one.
 */
void
ogmrip_container_get_split (OGMRipContainer *container, guint *number, guint *size)
{
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));

  if (number)
    *number = container->priv->tnumber;

  if (size)
    *size = container->priv->tsize;
}

static gint64
ogmrip_container_get_video_overhead (OGMRipContainer *container)
{
  OGMDvdStream *stream;
  gdouble framerate, length, video_frames;
  guint num, denom;
  gint overhead;

  if (!container->priv->video)
    return 0;

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (container->priv->video));
  ogmdvd_video_stream_get_framerate (OGMDVD_VIDEO_STREAM (stream), &num, &denom);
  framerate = num / (gdouble) denom;

  length = ogmrip_codec_get_length (OGMRIP_CODEC (container->priv->video), NULL);
  video_frames = length * framerate;

  overhead = ogmrip_container_get_overhead (container);

  return (gint64) (video_frames * overhead);
}

static gint64
ogmrip_container_get_audio_overhead (OGMRipContainer *container, OGMRipContainerChild *child)
{
  gdouble length, audio_frames;
  gint samples_per_frame, sample_rate, channels, overhead;

  length = ogmrip_codec_get_length (child->codec, NULL);
  samples_per_frame = ogmrip_audio_codec_get_samples_per_frame (OGMRIP_AUDIO_CODEC (child->codec));

  sample_rate = 48000;
  channels = 1;

  if (ogmrip_plugin_get_audio_codec_format (G_OBJECT_TYPE (child->codec)) != OGMRIP_FORMAT_COPY)
  {
    sample_rate = ogmrip_audio_codec_get_sample_rate (OGMRIP_AUDIO_CODEC (child->codec));
    channels = ogmrip_audio_codec_get_channels (OGMRIP_AUDIO_CODEC (child->codec));
  }

  audio_frames = length * sample_rate * (channels + 1) / samples_per_frame;

  overhead = ogmrip_container_get_overhead (container);

  return (gint64) (audio_frames * overhead);
}

static gint64
ogmrip_container_get_subp_overhead (OGMRipContainer *container, OGMRipContainerChild *child)
{
  return 0;
}

static gint64
ogmrip_container_get_file_overhead (OGMRipContainer *container, OGMRipFile *file)
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

  overhead = ogmrip_container_get_overhead (container);

  return (gint64) (audio_frames * overhead);
}

/**
 * ogmrip_container_get_overhead_size:
 * @container: An #OGMRipContainer
 *
 * Returns the size of the overhead generated by the video, audio and subtitle
 * stream, the chapters information and the files in bytes.
 * 
 * Returns: The overhead size
 */
gint64
ogmrip_container_get_overhead_size (OGMRipContainer *container)
{
  GSList *link;
  gint64 overhead = 0;

  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), -1);

  overhead = ogmrip_container_get_video_overhead (container);

  for (link = container->priv->audio; link; link = link->next)
    overhead += ogmrip_container_get_audio_overhead (container, link->data);

  for (link = container->priv->subp; link; link = link->next)
    overhead += ogmrip_container_get_subp_overhead (container, link->data);

  for (link = container->priv->files; link; link = link->next)
    overhead += ogmrip_container_get_file_overhead (container, link->data);

  return overhead;
}

static gint64
ogmrip_container_child_get_size (OGMRipContainerChild *child)
{
  struct stat buf;
  const gchar *filename;
  guint64 size = 0;

  filename = ogmrip_codec_get_output (child->codec);
  if (filename && g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    if (g_stat (filename, &buf) == 0)
      size = (guint64) buf.st_size;

  return size;
}

/**
 * ogmrip_container_get_nonvideo_size:
 * @container: An #OGMRipContainer
 *
 * Returns the size of the audio and subtitle streams, the chapters information
 * and the files in bytes.
 * 
 * Returns: The nonvideo size
 */
gint64
ogmrip_container_get_nonvideo_size (OGMRipContainer *container)
{
  GSList *link;
  gint64 nonvideo = 0;

  for (link = container->priv->audio; link; link = link->next)
    nonvideo += ogmrip_container_child_get_size (link->data);

  for (link = container->priv->subp; link; link = link->next)
    nonvideo += ogmrip_container_child_get_size (link->data);

  for (link = container->priv->chapters; link; link = link->next)
    nonvideo += ogmrip_container_child_get_size (link->data);

  for (link = container->priv->files; link; link = link->next)
    nonvideo += ogmrip_file_get_size (link->data);

  return nonvideo;
}

