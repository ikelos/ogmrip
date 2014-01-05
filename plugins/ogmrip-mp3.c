/* OGMRipMp3 - An MP3 plugin for OGMRip
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ogmrip-base.h>
#include <ogmrip-encode.h>
#include <ogmrip-mplayer.h>
#include <ogmrip-module.h>

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#define LAME "lame"

#define OGMRIP_TYPE_MP3          (ogmrip_mp3_get_type ())
#define OGMRIP_MP3(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MP3, OGMRipMp3))
#define OGMRIP_MP3_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_MP3, OGMRipMp3Class))
#define OGMRIP_IS_MP3(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_MP3))
#define OGMRIP_IS_MP3_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_MP3))

typedef struct _OGMRipMp3      OGMRipMp3;
typedef struct _OGMRipMp3Class OGMRipMp3Class;

struct _OGMRipMp3
{
  OGMRipAudioCodec parent_instance;
};

struct _OGMRipMp3Class
{
  OGMRipAudioCodecClass parent_class;
};

static gboolean ogmrip_mp3_run (OGMJobTask   *task,
                                GCancellable *cancellable,
                                GError       **error);

static OGMJobTask *
ogmrip_mp3_command (OGMRipAudioCodec *audio, gboolean header, const gchar *input)
{
  static gchar *presets[][2] =
  {
    { "96",       NULL      },
    { "112",      NULL      },
    { "128",      NULL      },
    { "fast",    "medium"   },
    { "medium",   NULL      },
    { "fast",    "standard" },
    { "standard", NULL      },
    { "fast",    "extreme"  },
    { "extreme",  NULL      },
    { "320",      NULL      },
    { "insane",   NULL      }
  };

  OGMJobTask *task;

  GPtrArray *argv;
  const gchar *output;
  gint quality;

  quality = ogmrip_audio_codec_get_quality (audio);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup (LAME));
  g_ptr_array_add (argv, g_strdup ("--nohist"));
  g_ptr_array_add (argv, g_strdup ("-h"));

  if (!header)
  {
    g_ptr_array_add (argv, g_strdup ("-r"));
    g_ptr_array_add (argv, g_strdup ("-s"));
    g_ptr_array_add (argv, g_strdup_printf ("%.1f",
          ogmrip_audio_codec_get_sample_rate (audio) / 1000.0));
  }

  g_ptr_array_add (argv, g_strdup ("--preset"));
  g_ptr_array_add (argv, g_strdup (presets[quality][0]));
  if (presets[quality][1])
    g_ptr_array_add (argv, g_strdup (presets[quality][1]));
  g_ptr_array_add (argv, g_strdup (input));

  output = ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (audio)));
  g_ptr_array_add (argv, g_strdup (output));

  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  return task;
}

G_DEFINE_TYPE (OGMRipMp3, ogmrip_mp3, OGMRIP_TYPE_AUDIO_CODEC)

static void
ogmrip_mp3_class_init (OGMRipMp3Class *klass)
{
  OGMJobTaskClass *task_class;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_mp3_run;
}

static void
ogmrip_mp3_init (OGMRipMp3 *mp3)
{
}

static gboolean
ogmrip_mp3_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobTask *pipeline;
  OGMJobTask *child;

  gchar *fifo;
  gboolean result;

  result = FALSE;

  fifo = ogmrip_fs_mkftemp ("fifo.XXXXXX", error);
  if (!fifo)
    return FALSE;

  pipeline = ogmjob_pipeline_new ();
  ogmjob_container_add (OGMJOB_CONTAINER (task), pipeline);
  g_object_unref (pipeline);

  child = ogmrip_mplayer_wav_command (OGMRIP_AUDIO_CODEC (task), FALSE, fifo);
  ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
  g_object_unref (child);

  child = ogmrip_mp3_command (OGMRIP_AUDIO_CODEC (task), FALSE, fifo);
  ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
  g_object_unref (child);

  result = OGMJOB_TASK_CLASS (ogmrip_mp3_parent_class)->run (task, cancellable, error);

  ogmjob_container_remove (OGMJOB_CONTAINER (task), pipeline);

  g_unlink (fifo);
  g_free (fifo);

  return result;
}

void
ogmrip_module_load (OGMRipModule *module)
{
  gboolean have_mplayer, have_lame;
  gchar *fullname;

  have_mplayer = ogmrip_check_mplayer ();

  fullname = g_find_program_in_path (LAME);
  have_lame = fullname != NULL;
  g_free (fullname);

  if (!have_mplayer && !have_lame)
  {
    g_warning (_("MPlayer and LAME are missing"));
    return;
  }

  if (!have_mplayer)
  {
    g_warning (_("MPlayer is missing"));
    return;
  }

  if (!have_lame)
  {
    g_warning (_("LAME is missing"));
    return;
  }

  ogmrip_register_codec (OGMRIP_TYPE_MP3,
      "mp3", _("MPEG-1 layer III (MP3)"), OGMRIP_FORMAT_MP3, NULL);
}

