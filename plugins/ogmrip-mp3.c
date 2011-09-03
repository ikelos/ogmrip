/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#include <ogmrip-job.h>
#include <ogmrip-encode.h>
#include <ogmrip-mplayer.h>

#include <unistd.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#define PROGRAM "lame"
#define MP3_SPF 1152

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

enum
{
  PROP_0,
  PROP_SPF
};

GType ogmrip_mp3_get_type (void);
static void ogmrip_mp3_get_property (GObject     *gobject,
                                     guint       property_id,
                                     GValue      *value,
                                     GParamSpec  *pspec);
static gint ogmrip_mp3_run          (OGMJobSpawn *spawn);

static gchar **
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

  GPtrArray *argv;
  const gchar *output;
  gint quality;

  quality = ogmrip_audio_codec_get_quality (audio);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup (PROGRAM));
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

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gchar **
ogmrip_wav_command (OGMRipAudioCodec *audio, gboolean header, const gchar *output)
{
  GPtrArray *argv;

  argv = ogmrip_mplayer_wav_command (audio, header, output);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

G_DEFINE_TYPE (OGMRipMp3, ogmrip_mp3, OGMRIP_TYPE_AUDIO_CODEC)

static void
ogmrip_mp3_class_init (OGMRipMp3Class *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_mp3_get_property;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmrip_mp3_run;

  g_object_class_install_property (gobject_class, PROP_SPF,
      g_param_spec_uint ("samples-per-frame", "Samples per frame property", "Set samples per frame",
        0, G_MAXUINT, MP3_SPF, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_mp3_init (OGMRipMp3 *mp3)
{
}

static void
ogmrip_mp3_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_SPF:
      g_value_set_uint (value, MP3_SPF);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static gint
ogmrip_mp3_run (OGMJobSpawn *spawn)
{
  GError *error = NULL;
  OGMJobSpawn *pipeline;
  OGMJobSpawn *child;
  gchar **argv, *fifo;
  gint result;

  result = OGMJOB_RESULT_ERROR;

  fifo = ogmrip_fs_mkftemp ("fifo.XXXXXX", &error);
  if (!fifo)
  {
    ogmjob_spawn_propagate_error (spawn, error);
    return OGMJOB_RESULT_ERROR;
  }

  pipeline = ogmjob_pipeline_new ();
  ogmjob_container_add (OGMJOB_CONTAINER (spawn), pipeline);
  g_object_unref (pipeline);

  argv = ogmrip_wav_command (OGMRIP_AUDIO_CODEC (spawn), FALSE, fifo);
  if (argv)
  {
    child = ogmjob_exec_newv (argv);
    ogmjob_exec_add_watch_full (OGMJOB_EXEC (child),
        (OGMJobWatch) ogmrip_mplayer_wav_watch, spawn, TRUE, FALSE, FALSE);
    ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
    g_object_unref (child);

    argv = ogmrip_mp3_command (OGMRIP_AUDIO_CODEC (spawn), FALSE, fifo);
    if (argv)
    {
      child = ogmjob_exec_newv (argv);
      ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
      g_object_unref (child);

      result = OGMJOB_SPAWN_CLASS (ogmrip_mp3_parent_class)->run (spawn);
    }
  }

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), pipeline);

  if (g_file_test (fifo, G_FILE_TEST_EXISTS))
    g_unlink (fifo);
  g_free (fifo);

  return result;
}

static OGMRipAudioPlugin mp3_plugin =
{
  NULL,
  G_TYPE_NONE,
  "mp3",
  N_("MPEG-1 layer III (MP3)"),
  OGMRIP_FORMAT_MP3
};

OGMRipAudioPlugin *
ogmrip_init_plugin (GError **error)
{
  gboolean have_mplayer, have_lame;
  gchar *fullname;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  have_mplayer = ogmrip_check_mplayer ();

  fullname = g_find_program_in_path (PROGRAM);
  have_lame = fullname != NULL;
  g_free (fullname);

  mp3_plugin.type = OGMRIP_TYPE_MP3;

  if (have_mplayer && have_lame)
    return &mp3_plugin;

  if (!have_mplayer && !have_lame)
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("MPlayer and LAME are missing"));
  else if (!have_mplayer)
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("MPlayer is missing"));
  else if (!have_lame)
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("LAME is missing"));

  return NULL;
}

