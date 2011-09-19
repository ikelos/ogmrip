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

#include <ogmrip-encode.h>
#include <ogmrip-mplayer.h>
#include <ogmrip-module.h>

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#define THEORAENC "theoraenc"

#define OGMRIP_TYPE_THEORA          (ogmrip_theora_get_type ())
#define OGMRIP_THEORA(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_THEORA, OGMRipTheora))
#define OGMRIP_THEORA_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_THEORA, OGMRipTheoraClass))
#define OGMRIP_IS_THEORA(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_THEORA))
#define OGMRIP_IS_THEORA_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_THEORA))

typedef struct _OGMRipTheora      OGMRipTheora;
typedef struct _OGMRipTheoraClass OGMRipTheoraClass;

struct _OGMRipTheora
{
  OGMRipVideoCodec parent_instance;
};

struct _OGMRipTheoraClass
{
  OGMRipVideoCodecClass parent_class;
};

enum
{
  PROP_0,
  PROP_PASSES
};

static gint ogmrip_theora_run          (OGMJobSpawn  *spawn);
static void ogmrip_theora_get_property (GObject      *gobject,
                                        guint        property_id,
                                        GValue       *value,
                                        GParamSpec   *pspec);
static void ogmrip_theora_set_property (GObject      *gobject,
                                        guint        property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec);

static gchar **
ogmrip_yuv4mpeg_command (OGMRipVideoCodec *video, const gchar *output, const gchar *logf)
{
  OGMRipTitle *title;
  GPtrArray *argv;
  gint vid;

  argv = ogmrip_mplayer_video_command (video, output);

  g_ptr_array_add (argv, g_strdup ("-vo"));
  if (MPLAYER_CHECK_VERSION (1,0,0,6))
    g_ptr_array_add (argv, g_strdup_printf ("yuv4mpeg:file=%s", output));
  else
    g_ptr_array_add (argv, g_strdup ("yuv4mpeg"));

  title = ogmrip_stream_get_title (ogmrip_codec_get_input (OGMRIP_CODEC (video)));
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

static gchar **
ogmrip_theora_command (OGMRipVideoCodec *video, const gchar *input)
{
  GPtrArray *argv;
  const gchar *output;
  gint bitrate;

  output = ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (video)));

  argv = g_ptr_array_new ();

  g_ptr_array_add (argv, g_strdup (THEORAENC));

  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (output));

  bitrate = ogmrip_video_codec_get_bitrate (video);
  if (bitrate > 0)
  {
    bitrate /= 1000;
    g_ptr_array_add (argv, g_strdup ("-b"));
    g_ptr_array_add (argv, g_strdup_printf ("%u", CLAMP (bitrate, 45, 2000)));
  }
  else
  {
    gint quantizer;

    quantizer = ogmrip_video_codec_get_quantizer (video);
    g_ptr_array_add (argv, g_strdup ("-q"));
    g_ptr_array_add (argv, g_strdup_printf ("%u", (31 - quantizer) / 3));
  }

  g_ptr_array_add (argv, g_strdup (input));

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

G_DEFINE_DYNAMIC_TYPE (OGMRipTheora, ogmrip_theora, OGMRIP_TYPE_VIDEO_CODEC)

static void
ogmrip_theora_class_init (OGMRipTheoraClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_theora_get_property;
  gobject_class->set_property = ogmrip_theora_set_property;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmrip_theora_run;

  g_object_class_install_property (gobject_class, PROP_PASSES, 
        g_param_spec_uint ("passes", "Passes property", "Set the number of passes", 
           1, 1, 1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_theora_class_finalize (OGMRipTheoraClass *klass)
{
}

static void
ogmrip_theora_init (OGMRipTheora *theora)
{
}

static void
ogmrip_theora_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
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
ogmrip_theora_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
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

static gint
ogmrip_theora_run (OGMJobSpawn *spawn)
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

  argv = ogmrip_yuv4mpeg_command (OGMRIP_VIDEO_CODEC (spawn), fifo, NULL);
  if (argv)
  {
    child = ogmjob_exec_newv (argv);
    ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_mplayer_video_watch, spawn, TRUE, FALSE, FALSE);
    ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
    g_object_unref (child);

    argv = ogmrip_theora_command (OGMRIP_VIDEO_CODEC (spawn), fifo);
    if (argv)
    {
      child = ogmjob_exec_newv (argv);
      ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
      g_object_unref (child);

      result = OGMJOB_SPAWN_CLASS (ogmrip_theora_parent_class)->run (spawn);
    }
  }

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), pipeline);

  if (g_file_test (fifo, G_FILE_TEST_EXISTS))
    g_unlink (fifo);
  g_free (fifo);

  return result;
}

void
ogmrip_module_load (OGMRipModule *module)
{
  gboolean have_mplayer, have_theoraenc;
  gchar *fullname;

  have_mplayer = ogmrip_check_mplayer ();

  fullname = g_find_program_in_path (THEORAENC);
  have_theoraenc = fullname != NULL;
  g_free (fullname);

  if (!have_mplayer && !have_theoraenc)
  {
    g_warning (_("MPlayer and theoraenc are missing"));
    return;
  }

  if (!have_mplayer)
  {
    g_warning (_("MPlayer is missing"));
    return;
  }

  if (!have_theoraenc)
  {
    g_warning (_("theoraenc is missing"));
    return;
  }

  ogmrip_theora_register_type (G_TYPE_MODULE (module));
  ogmrip_type_register_codec (module,
      OGMRIP_TYPE_THEORA, "theora", N_("Ogg Theora"), OGMRIP_FORMAT_THEORA);
}

