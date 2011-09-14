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

#define PROGRAM "oggenc"

#define OGMRIP_TYPE_VORBIS          (ogmrip_vorbis_get_type ())
#define OGMRIP_VORBIS(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_VORBIS, OGMRipVorbis))
#define OGMRIP_VORBIS_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_VORBIS, OGMRipVorbisClass))
#define OGMRIP_IS_VORBIS(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_VORBIS))
#define OGMRIP_IS_VORBIS_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_VORBIS))

typedef struct _OGMRipVorbis      OGMRipVorbis;
typedef struct _OGMRipVorbisClass OGMRipVorbisClass;

struct _OGMRipVorbis
{
  OGMRipAudioCodec parent_instance;
};

struct _OGMRipVorbisClass
{
  OGMRipAudioCodecClass parent_class;
};

static gint ogmrip_vorbis_run (OGMJobSpawn *spawn);

gchar **
ogmrip_vorbis_command (OGMRipAudioCodec *audio, gboolean header, const gchar *input)
{
  GPtrArray *argv;
  const gchar *output;
  gint quality;

  quality = ogmrip_audio_codec_get_quality (audio);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup (PROGRAM));

  if (!header)
  {
    g_ptr_array_add (argv, g_strdup ("-r"));
    g_ptr_array_add (argv, g_strdup ("-R"));
    g_ptr_array_add (argv, g_strdup_printf ("%d",
          ogmrip_audio_codec_get_sample_rate (audio)));
    g_ptr_array_add (argv, g_strdup ("-C"));
    g_ptr_array_add (argv, g_strdup_printf ("%d",
          ogmrip_audio_codec_get_channels (audio) + 1));
  }

  g_ptr_array_add (argv, g_strdup ("-q"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", quality));
  g_ptr_array_add (argv, g_strdup ("-o"));

  output = ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (audio)));
  g_ptr_array_add (argv, g_strdup (output));

  g_ptr_array_add (argv, g_strdup (input));
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

G_DEFINE_TYPE (OGMRipVorbis, ogmrip_vorbis, OGMRIP_TYPE_AUDIO_CODEC)

static void
ogmrip_vorbis_class_init (OGMRipVorbisClass *klass)
{
  OGMJobSpawnClass *spawn_class;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);

  spawn_class->run = ogmrip_vorbis_run;
}

static void
ogmrip_vorbis_init (OGMRipVorbis *vorbis)
{
}

static gint
ogmrip_vorbis_run (OGMJobSpawn *spawn)
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
    ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_mplayer_wav_watch, spawn, TRUE, FALSE, FALSE);
    ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
    g_object_unref (child);

    argv = ogmrip_vorbis_command (OGMRIP_AUDIO_CODEC (spawn), FALSE, fifo);
    if (argv)
    {
      child = ogmjob_exec_newv (argv);
      ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
      g_object_unref (child);

      result = OGMJOB_SPAWN_CLASS (ogmrip_vorbis_parent_class)->run (spawn);
    }
  }

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), pipeline);

  if (g_file_test (fifo, G_FILE_TEST_EXISTS))
    g_unlink (fifo);
  g_free (fifo);

  return result;
}

gboolean
ogmrip_init_plugin (GError **error)
{
  gboolean have_mplayer, have_oggenc;
  gchar *fullname;

  have_mplayer = ogmrip_check_mplayer ();

  fullname = g_find_program_in_path (PROGRAM);
  have_oggenc = fullname != NULL;
  g_free (fullname);

  if (!have_mplayer && !have_oggenc)
  {
    // g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("MPlayer and OggEnc are missing"));
    return FALSE;
  }

  if (!have_mplayer)
  {
    // g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("MPlayer is missing"));
    return FALSE;
  }

  if (!have_oggenc)
  {
    // g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("OggEnc is missing"));
    return FALSE;
  }

  ogmrip_type_register_codec (NULL, OGMRIP_TYPE_VORBIS,
      "vorbis", N_("Ogg Vorbis"), OGMRIP_FORMAT_VORBIS);

  return TRUE;
}

