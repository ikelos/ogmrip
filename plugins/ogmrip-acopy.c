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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ogmjob.h>
#include <ogmrip.h>
#include <ogmrip-mplayer.h>

#include <stdio.h>
#include <glib/gi18n-lib.h>

#define ACOPY_SPF 1536

#define OGMRIP_TYPE_AUDIO_COPY          (ogmrip_audio_copy_get_type ())
#define OGMRIP_AUDIO_COPY(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_AUDIO_COPY, OGMRipAudioCopy))
#define OGMRIP_AUDIO_COPY_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_AUDIO_COPY, OGMRipAudioCopyClass))
#define OGMRIP_IS_AUDIO_COPY(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_AUDIO_COPY))
#define OGMRIP_IS_AUDIO_COPY_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_AUDIO_COPY))

typedef struct _OGMRipAudioCopy      OGMRipAudioCopy;
typedef struct _OGMRipAudioCopyClass OGMRipAudioCopyClass;

struct _OGMRipAudioCopy
{
  OGMRipAudioCodec parent_instance;
};

struct _OGMRipAudioCopyClass
{
  OGMRipAudioCodecClass parent_class;
};

enum
{
  PROP_0,
  PROP_SPF
};

static void ogmrip_audio_copy_get_property (GObject     *gobject,
                                            guint       property_id,
                                            GValue      *value,
                                            GParamSpec  *pspec);
static gint ogmrip_audio_copy_run          (OGMJobSpawn *spawn);

static gchar **
ogmrip_audio_copy_command (OGMRipAudioCodec *audio, const gchar *input, const gchar *output)
{
  OGMDvdTitle *title;
  GPtrArray *argv;
  gint vid;

  if (!output)
    output = ogmrip_codec_get_output (OGMRIP_CODEC (audio));

  title = ogmdvd_stream_get_title (ogmrip_codec_get_input (OGMRIP_CODEC (audio)));

  argv = ogmrip_mencoder_audio_command (audio, output);

  g_ptr_array_add (argv, g_strdup ("-ovc"));
  if (MPLAYER_CHECK_VERSION (1,0,0,8))
  {
    g_ptr_array_add (argv, g_strdup ("copy"));
    g_ptr_array_add (argv, g_strdup ("-of"));
    g_ptr_array_add (argv, g_strdup ("rawaudio"));
  }
  else
    g_ptr_array_add (argv, g_strdup ("frameno"));

  g_ptr_array_add (argv, g_strdup ("-oac"));
  g_ptr_array_add (argv, g_strdup ("copy"));

  vid = ogmdvd_title_get_nr (title);

  if (MPLAYER_CHECK_VERSION (1,0,0,1))
    g_ptr_array_add (argv, g_strdup_printf ("dvd://%d", vid + 1));
  else
  {
    g_ptr_array_add (argv, g_strdup ("-dvd"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", vid + 1));
  }

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

G_DEFINE_TYPE (OGMRipAudioCopy, ogmrip_audio_copy, OGMRIP_TYPE_AUDIO_CODEC)

static void
ogmrip_audio_copy_class_init (OGMRipAudioCopyClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_audio_copy_get_property;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmrip_audio_copy_run;

  g_object_class_install_property (gobject_class, PROP_SPF,
      g_param_spec_uint ("samples-per-frame", "Samples per frame property", "Set samples per frame",
        0, G_MAXUINT, ACOPY_SPF, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_audio_copy_init (OGMRipAudioCopy *audio_copy)
{
}

static void
ogmrip_audio_copy_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_SPF:
      g_value_set_uint (value, ACOPY_SPF);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static gint
ogmrip_audio_copy_run (OGMJobSpawn *spawn)
{
  OGMJobSpawn *child;
  gchar **argv;
  gint result;

  argv = ogmrip_audio_copy_command (OGMRIP_AUDIO_CODEC (spawn), NULL, NULL);
  if (!argv)
    return OGMJOB_RESULT_ERROR;

  child = ogmjob_exec_newv (argv);
  ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_mencoder_codec_watch, spawn, TRUE, FALSE, FALSE);
  ogmjob_container_add (OGMJOB_CONTAINER (spawn), child);
  g_object_unref (child);

  result = OGMJOB_SPAWN_CLASS (ogmrip_audio_copy_parent_class)->run (spawn);

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), child);

  return result;
}

static OGMRipAudioPlugin acopy_plugin =
{
  NULL,
  G_TYPE_NONE,
  "copy",
  N_("Copy (for AC3 or DTS)"),
  OGMRIP_FORMAT_COPY
};

OGMRipAudioPlugin *
ogmrip_init_plugin (void)
{
  if (!ogmrip_check_mencoder ())
    return NULL;

  acopy_plugin.type = OGMRIP_TYPE_AUDIO_COPY;

  return &acopy_plugin;
}

