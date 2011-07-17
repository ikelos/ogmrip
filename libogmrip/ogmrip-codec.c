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
  OGMDvdTime time_;
  gchar *output;

  guint framerate_numerator;
  guint framerate_denominator;
  guint framestep;

  gboolean telecine;
  gboolean progressive;
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
  PROP_END_CHAPTER,
  PROP_FRAMESTEP,
  PROP_PROGRESSIVE,
  PROP_TELECINE
};

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

G_DEFINE_ABSTRACT_TYPE (OGMRipCodec, ogmrip_codec, OGMJOB_TYPE_BIN)

static void
ogmrip_codec_class_init (OGMRipCodecClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_codec_dispose;
  gobject_class->finalize = ogmrip_codec_finalize;
  gobject_class->set_property = ogmrip_codec_set_property;
  gobject_class->get_property = ogmrip_codec_get_property;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmrip_codec_run;

  g_object_class_install_property (gobject_class, PROP_INPUT, 
        g_param_spec_pointer ("input", "Input property", "Set input title", 
           G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_OUTPUT, 
        g_param_spec_string ("output", "Output property", "Set output file", 
           NULL, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LENGTH, 
        g_param_spec_double ("length", "Length property", "Get length", 
           0.0, G_MAXDOUBLE, 0.0, G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_START_CHAPTER, 
        g_param_spec_int ("start-chapter", "Start chapter property", "Set start chapter", 
           0, G_MAXINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_END_CHAPTER, 
        g_param_spec_int ("end-chapter", "End chapter property", "Set end chapter", 
           -1, G_MAXINT, -1, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FRAMESTEP, 
        g_param_spec_uint ("framestep", "Framestep property", "Set framestep", 
           0, G_MAXUINT, 1, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PROGRESSIVE, 
        g_param_spec_boolean ("progressive", "Progressive property", "Set progressive", 
           FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TELECINE, 
        g_param_spec_boolean ("telecine", "Telecine property", "Set telecine", 
           FALSE, G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (OGMRipCodecPriv));
}

static void
ogmrip_codec_init (OGMRipCodec *codec)
{
  codec->priv = OGMRIP_CODEC_GET_PRIVATE (codec);
  codec->priv->framestep = 1;
  codec->priv->end_chap = -1;

  codec->priv->play_length = -1.0;
  codec->priv->start_second = -1.0;
}

static void
ogmrip_codec_dispose (GObject *gobject)
{
  OGMRipCodec *codec;

  codec = OGMRIP_CODEC (gobject);

  if (codec->priv->title)
    ogmdvd_title_unref (codec->priv->title);
  codec->priv->title = NULL;

  G_OBJECT_CLASS (ogmrip_codec_parent_class)->dispose (gobject);
}

static void
ogmrip_codec_finalize (GObject *gobject)
{
  OGMRipCodec *codec;

  codec = OGMRIP_CODEC (gobject);

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
  OGMRipCodec *codec;

  codec = OGMRIP_CODEC (gobject);

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
    case PROP_FRAMESTEP: 
      ogmrip_codec_set_framestep (codec, g_value_get_uint (value));
      break;
    case PROP_PROGRESSIVE: 
      ogmrip_codec_set_progressive (codec, g_value_get_boolean (value));
      break;
    case PROP_TELECINE: 
      ogmrip_codec_set_telecine (codec, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_codec_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipCodec *codec;

  codec = OGMRIP_CODEC (gobject);

  switch (property_id) 
  {
    case PROP_INPUT:
      g_value_set_pointer (value, codec->priv->title);
      break;
    case PROP_OUTPUT:
      g_value_set_static_string (value, codec->priv->output);
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
    case PROP_FRAMESTEP: 
      g_value_set_uint (value, codec->priv->framestep);
      break;
    case PROP_PROGRESSIVE: 
      g_value_set_boolean (value, codec->priv->progressive);
      break;
    case PROP_TELECINE: 
      g_value_set_boolean (value, codec->priv->telecine);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static gint
ogmrip_codec_run (OGMJobSpawn *spawn)
{
  OGMDvdTitle *title;

  title = OGMRIP_CODEC (spawn)->priv->title;

  g_return_val_if_fail (title != NULL, OGMJOB_RESULT_ERROR);
  g_return_val_if_fail (ogmdvd_title_is_open (title), OGMJOB_RESULT_ERROR);

  return OGMJOB_SPAWN_CLASS (ogmrip_codec_parent_class)->run (spawn);
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
}

/**
 * ogmrip_codec_get_output:
 * @codec: an #OGMRipCodec
 *
 * Gets the name of the output file.
 *
 * Returns: the filename, or NULL
 */
G_CONST_RETURN gchar *
ogmrip_codec_get_output (OGMRipCodec *codec)
{
  g_return_val_if_fail (OGMRIP_IS_CODEC (codec), NULL);

  return codec->priv->output;
}

/**
 * ogmrip_codec_set_input:
 * @codec: an #OGMRipCodec
 * @title: an #OGMDvdTitle
 *
 * Sets the input DVD title.
 */
void
ogmrip_codec_set_input (OGMRipCodec *codec, OGMDvdTitle *title)
{
  g_return_if_fail (OGMRIP_IS_CODEC (codec));
  g_return_if_fail (title != NULL);

  ogmdvd_title_ref (title);

  if (codec->priv->title)
    ogmdvd_title_unref (codec->priv->title);

  ogmdvd_title_get_framerate (title, 
      &codec->priv->framerate_numerator, 
      &codec->priv->framerate_denominator);

  codec->priv->title = title;
  codec->priv->dirty = TRUE;

  codec->priv->start_chap = 0;
  codec->priv->end_chap = -1;
}

/**
 * ogmrip_codec_get_input:
 * @codec: an #OGMRipCodec
 *
 * Gets the input DVD title.
 *
 * Returns: an #OGMDvdTitle, or NULL
 */
OGMDvdTitle *
ogmrip_codec_get_input (OGMRipCodec *codec)
{
  g_return_val_if_fail (OGMRIP_IS_CODEC (codec), NULL);

  return codec->priv->title;
}

/**
 * ogmrip_codec_set_profile:
 * @codec: An #OGMRipCodec
 * @profile: An #OGMRipProfile
 *
 * Sets codec specific options from the specified profile.
 */
void
ogmrip_codec_set_profile (OGMRipCodec *codec, OGMRipProfile *profile)
{
  OGMRipCodecClass *klass;

  g_return_if_fail (OGMRIP_IS_CODEC (codec));
  g_return_if_fail (G_IS_SETTINGS (profile));

  klass = OGMRIP_CODEC_GET_CLASS (codec);
  if (klass->set_profile)
    (* klass->set_profile) (codec, profile);
}

/**
 * ogmrip_codec_set_progressive:
 * @codec: an #OGMRipCodec
 * @progressive: %TRUE to inverse progressive
 *
 * Sets whether an inverse progressive filter will be applied
 */
void
ogmrip_codec_set_progressive (OGMRipCodec *codec, gboolean progressive)
{
  g_return_if_fail (OGMRIP_IS_CODEC (codec));

  codec->priv->progressive = progressive;
}

/**
 * ogmrip_codec_get_progressive:
 * @codec: an #OGMRipCodec
 *
 * Gets whether an inverse progressive filter will be applied
 *
 * Returns: %TRUE if inverse progressive
 */
gboolean
ogmrip_codec_get_progressive (OGMRipCodec *codec)
{
  g_return_val_if_fail (OGMRIP_IS_CODEC (codec), FALSE);

  return codec->priv->progressive;
}

/**
 * ogmrip_codec_set_telecine:
 * @codec: an #OGMRipCodec
 * @telecine: %TRUE to inverse telecine
 *
 * Sets whether an inverse telecine filter will be applied
 */
void
ogmrip_codec_set_telecine (OGMRipCodec *codec, gboolean telecine)
{
  g_return_if_fail (OGMRIP_IS_CODEC (codec));

  codec->priv->telecine = telecine;
}

/**
 * ogmrip_codec_get_telecine:
 * @codec: an #OGMRipCodec
 *
 * Gets whether an inverse telecine filter will be applied
 *
 * Returns: %TRUE if inverse telecine
 */
gboolean
ogmrip_codec_get_telecine (OGMRipCodec *codec)
{
  g_return_val_if_fail (OGMRIP_IS_CODEC (codec), FALSE);

  return codec->priv->telecine;
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
    if (codec->priv->play_length > 0.0)
    {
      codec->priv->length = codec->priv->play_length;
      ogmrip_codec_sec_to_time (codec->priv->play_length,
          codec->priv->framerate_numerator / (gdouble) codec->priv->framerate_denominator, &codec->priv->time_);
    }
    else if (codec->priv->start_chap == 0 && codec->priv->end_chap == -1)
      codec->priv->length = ogmdvd_title_get_length (codec->priv->title, &codec->priv->time_);
    else
      codec->priv->length = ogmdvd_title_get_chapters_length (codec->priv->title, 
            codec->priv->start_chap, codec->priv->end_chap, &codec->priv->time_);
/*
    ogmdvd_title_get_framerate (codec->priv->title, &numerator, &denominator);
    if (numerator != codec->priv->framerate_numerator || denominator != codec->priv->framerate_denominator)
      codec->priv->length *= ((denominator * codec->priv->framerate_numerator) / 
          (gdouble) (numerator * codec->priv->framerate_denominator));
*/
    codec->priv->dirty = FALSE;
  }

  if (length)
    *length = codec->priv->time_;

  return codec->priv->length;
}

/**
 * ogmrip_codec_set_framerate:
 * @codec: an #OGMRipCodec
 * @numerator: the framerate numerator
 * @denominator: the framerate denominator
 *
 * Sets a frames per second (fps) value for the output file, which can be
 * different from that of the source material.
 */
void
ogmrip_codec_set_framerate (OGMRipCodec *codec, guint numerator, guint denominator)
{
  g_return_if_fail (OGMRIP_IS_CODEC (codec));
  g_return_if_fail (numerator > 0 && denominator > 0);

  codec->priv->framerate_numerator = numerator;
  codec->priv->framerate_denominator = denominator;
}

/**
 * ogmrip_codec_get_framerate:
 * @codec: an #OGMRipCodec
 * @numerator: a pointer to store the framerate numerator
 * @denominator: a pointer to store the framerate denominator
 *
 * Gets the framerate of the output file in the form of a fraction.
 */
void
ogmrip_codec_get_framerate (OGMRipCodec *codec, guint *numerator, guint *denominator)
{
  g_return_if_fail (OGMRIP_IS_CODEC (codec));
  g_return_if_fail (denominator != NULL);
  g_return_if_fail (numerator != NULL);

  *numerator = codec->priv->framerate_numerator;
  *denominator = codec->priv->framerate_denominator;
}

/**
 * ogmrip_codec_set_framestep:
 * @codec: an #OGMRipCodec
 * @framestep: the framestep
 *
 * Skips @framestep frames after every frame.
 */
void
ogmrip_codec_set_framestep (OGMRipCodec *codec, guint framestep)
{
  g_return_if_fail (OGMRIP_IS_CODEC (codec));

  codec->priv->framestep = MAX (framestep, 1);
}

/**
 * ogmrip_codec_get_framestep:
 * @codec: an #OGMRipCodec
 *
 * Gets the number of frames to skip after every frame.
 *
 * Returns: the framestep, or -1
 */
gint
ogmrip_codec_get_framestep (OGMRipCodec *codec)
{
  g_return_val_if_fail (OGMRIP_IS_CODEC (codec), -1);

  return codec->priv->framestep;
}

/**
 * ogmrip_codec_set_unlink_on_unref:
 * @codec: an #OGMRipCodec
 * @do_unlink: %TRUE to unlink the output file
 *
 * Whether to unlink the output file on the final unref of the #OGMRipCodec
 * data structure.
 */
void
ogmrip_codec_set_unlink_on_unref (OGMRipCodec *codec, gboolean do_unlink)
{
  g_return_if_fail (OGMRIP_IS_CODEC (codec));

  codec->priv->do_unlink = do_unlink;
}

/**
 * ogmrip_codec_get_unlink_on_unref:
 * @codec: an #OGMRipCodec
 *
 * Returns whether the output file will be unlinked on the final unref of
 * @codec.
 *
 * Returns: a boolean
 */
gboolean
ogmrip_codec_get_unlink_on_unref (OGMRipCodec *codec)
{
  g_return_val_if_fail (OGMRIP_IS_CODEC (codec), FALSE);

  return codec->priv->do_unlink;
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
 * ogmrip_codec_set_start:
 * @codec: an #OGMRipCodec
 * @start: the position to seek in seconds
 *
 * Seeks to the given time position.
 */
void
ogmrip_codec_set_start (OGMRipCodec *codec, gdouble start)
{
  g_return_if_fail (OGMRIP_IS_CODEC (codec));
  g_return_if_fail (start >= 0.0);

  codec->priv->start_chap = 0;
  codec->priv->end_chap = -1;

  codec->priv->start_second = start;
}

/**
 * ogmrip_codec_get_start:
 * @codec: an #OGMRipCodec
 *
 * Gets the position to seek in seconds.
 *
 * Returns: the position, or -1.0
 */
gdouble
ogmrip_codec_get_start (OGMRipCodec *codec)
{
  g_return_val_if_fail (OGMRIP_IS_CODEC (codec), -1);

  return codec->priv->start_second;
}

