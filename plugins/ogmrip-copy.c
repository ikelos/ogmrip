/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#include <ogmrip-encode.h>
#include <ogmrip-mplayer.h>
#include <ogmrip-module.h>

#include <stdio.h>
#include <glib/gi18n-lib.h>

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

static gboolean ogmrip_audio_copy_run (OGMJobTask   *task,
                                       GCancellable *cancellable,
                                       GError       **error);

static gchar **
ogmrip_audio_copy_command (OGMRipAudioCodec *audio)
{
  OGMRipTitle *title;
  OGMRipFile *output;
  GPtrArray *argv;
  gint vid;

  output = ogmrip_codec_get_output (OGMRIP_CODEC (audio));
  argv = ogmrip_mencoder_audio_command (audio, ogmrip_file_get_path (output));

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

  title = ogmrip_stream_get_title (ogmrip_codec_get_input (OGMRIP_CODEC (audio)));
  vid = ogmrip_title_get_nr (title);

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
  OGMJobTaskClass *task_class;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_audio_copy_run;
}

static void
ogmrip_audio_copy_init (OGMRipAudioCopy *audio_copy)
{
}

static gboolean
ogmrip_audio_copy_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobTask *child;
  gchar **argv;
  gboolean result;

  argv = ogmrip_audio_copy_command (OGMRIP_AUDIO_CODEC (task));
  if (!argv)
    return FALSE;

  child = ogmjob_spawn_newv (argv);
  ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (child), (OGMJobWatch) ogmrip_mencoder_codec_watch, task);
  ogmjob_container_add (OGMJOB_CONTAINER (task), child);
  g_object_unref (child);

  result = OGMJOB_TASK_CLASS (ogmrip_audio_copy_parent_class)->run (task, cancellable, error);

  ogmjob_container_remove (OGMJOB_CONTAINER (task), child);

  return result;
}

typedef struct _OGMRipVideoCopy OGMRipVideoCopy;
typedef struct _OGMRipVideoCopyClass OGMRipVideoCopyClass;

struct _OGMRipVideoCopy
{
  OGMRipVideoCodec parent_instance;
};

struct _OGMRipVideoCopyClass
{
  OGMRipVideoCodecClass parent_class;
};

enum
{
  PROP_0,
  PROP_PASSES
};

static void     ogmrip_copy_get_property (GObject      *gobject,
                                          guint        property_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);
static void     ogmrip_copy_set_property (GObject      *gobject,
                                          guint        property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static gboolean ogmrip_video_copy_run    (OGMJobTask   *task,
                                          GCancellable *cancellable,
                                          GError       **error);

G_DEFINE_TYPE (OGMRipVideoCopy, ogmrip_video_copy, OGMRIP_TYPE_VIDEO_CODEC)

static void
ogmrip_video_copy_class_init (OGMRipVideoCopyClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobTaskClass *task_class;
  
  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_copy_get_property;
  gobject_class->set_property = ogmrip_copy_set_property;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_video_copy_run;

  g_object_class_install_property (gobject_class, PROP_PASSES,
        g_param_spec_uint ("passes", "Passes property", "Set the number of passes",
           1, 1, 1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_video_copy_init (OGMRipVideoCopy *nouveau)
{
}

static void
ogmrip_copy_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_PASSES:
      g_value_set_uint (value, ogmrip_video_codec_get_passes (OGMRIP_VIDEO_CODEC (gobject)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_copy_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_PASSES:
      ogmrip_video_codec_set_passes (OGMRIP_VIDEO_CODEC (gobject), g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static gchar **
ogmrip_video_copy_command (OGMRipVideoCodec *video)
{
  OGMRipTitle *title;
  GPtrArray *argv;
  gint vid;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), NULL);

  ogmrip_video_codec_set_ensure_sync (video, NULL);

  argv = ogmrip_mencoder_video_command (video,
      ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (video))), 0);

  g_ptr_array_add (argv, g_strdup ("-ovc"));
  g_ptr_array_add (argv, g_strdup ("copy"));

  g_ptr_array_add (argv, g_strdup ("-of"));
  g_ptr_array_add (argv, g_strdup ("mpeg"));

  g_ptr_array_add (argv, g_strdup ("-mpegopts"));
  g_ptr_array_add (argv, g_strdup ("format=dvd:tsaf"));

  title = ogmrip_stream_get_title (ogmrip_codec_get_input (OGMRIP_CODEC (video)));
  vid = ogmrip_title_get_nr (title);

  if (MPLAYER_CHECK_VERSION(1,0,0,1))
    g_ptr_array_add (argv, g_strdup_printf ("dvd://%d", vid + 1));
  else
  {
    g_ptr_array_add (argv, g_strdup ("-dvd"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", vid + 1));
  }

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gboolean
ogmrip_video_copy_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobTask *child;
  gchar **argv;
  gboolean result;

  argv = ogmrip_video_copy_command (OGMRIP_VIDEO_CODEC (task));
  if (!argv)
    return FALSE;

  child = ogmjob_spawn_newv (argv);
  ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (child), (OGMJobWatch) ogmrip_mencoder_codec_watch, task);
  ogmjob_container_add (OGMJOB_CONTAINER (task), child);
  g_object_unref (child);

  result = OGMJOB_TASK_CLASS (ogmrip_video_copy_parent_class)->run (task, cancellable, error);

  ogmjob_container_remove (OGMJOB_CONTAINER (task), child);

  return result;
}

void
ogmrip_module_load (OGMRipModule *module)
{
  if (!ogmrip_check_mencoder ())
    g_warning (_("MEncoder is missing"));
  else
  {
    ogmrip_register_codec (ogmrip_audio_copy_get_type (),
        "audio-copy", _("Copy (for AC3 or DTS)"), OGMRIP_FORMAT_COPY, NULL);
    ogmrip_register_codec (ogmrip_video_copy_get_type (),
        "video-copy", _("Video Copy"), OGMRIP_FORMAT_COPY, NULL);
  }
}

