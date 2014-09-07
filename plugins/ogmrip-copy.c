/* OGMRipCopy - Audio and video copy plugins for OGMRip
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

static OGMJobTask *
ogmrip_audio_copy_command (OGMRipAudioCodec *audio)
{
  OGMJobTask *task;
  OGMRipStream *input;
  OGMRipFile *output;

  input  = ogmrip_codec_get_input (OGMRIP_CODEC (audio));
  output = ogmrip_codec_get_output (OGMRIP_CODEC (audio));

  if (ogmrip_stream_get_format (input) != OGMRIP_FORMAT_PCM)
    task = ogmrip_audio_encoder_new (audio, OGMRIP_ENCODER_COPY, NULL, ogmrip_file_get_path (output));
  else
  {
    ogmrip_audio_codec_set_fast (audio, FALSE);
    ogmrip_audio_codec_set_normalize (audio, FALSE);
    ogmrip_audio_codec_set_sample_rate (audio,
        ogmrip_audio_stream_get_sample_rate (OGMRIP_AUDIO_STREAM (input)));
    ogmrip_audio_codec_set_channels (audio,
        ogmrip_audio_stream_get_channels (OGMRIP_AUDIO_STREAM (input)));

    task = ogmrip_audio_encoder_new (audio, OGMRIP_ENCODER_PCM, NULL, ogmrip_file_get_path (output));
  }

  return task;
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
  gboolean result;

  child = ogmrip_audio_copy_command (OGMRIP_AUDIO_CODEC (task));
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

static OGMJobTask *
ogmrip_video_copy_command (OGMRipVideoCodec *video)
{
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), NULL);

  ogmrip_video_codec_set_ensure_sync (video, NULL);

  return ogmrip_video_encoder_new (video, OGMRIP_ENCODER_COPY, NULL, NULL,
      ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (video))));
}

static gboolean
ogmrip_video_copy_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobTask *child;
  gboolean result;

  child = ogmrip_video_copy_command (OGMRIP_VIDEO_CODEC (task));
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

