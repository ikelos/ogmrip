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

#include <ogmrip-base.h>
#include <ogmrip-encode.h>
#include <ogmrip-mplayer.h>
#include <ogmrip-module.h>

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#define OGGENC "oggenc"

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

static gboolean ogmrip_vorbis_run (OGMJobTask   *task,
                                   GCancellable *cancellable,
                                   GError       **error);

gchar **
ogmrip_vorbis_command (OGMRipAudioCodec *audio, gboolean header, const gchar *input)
{
  GPtrArray *argv;
  const gchar *output;
  gint quality;

  quality = ogmrip_audio_codec_get_quality (audio);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup (OGGENC));

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
  OGMJobTaskClass *task_class;

  task_class = OGMJOB_TASK_CLASS (klass);

  task_class->run = ogmrip_vorbis_run;
}

static void
ogmrip_vorbis_init (OGMRipVorbis *vorbis)
{
}

static gboolean
ogmrip_vorbis_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobTask *pipeline;
  OGMJobTask *child;
  gchar **argv, *fifo;
  gboolean result;

  result = FALSE;

  fifo = ogmrip_fs_mkftemp ("fifo.XXXXXX", error);
  if (!fifo)
    return FALSE;

  pipeline = ogmjob_pipeline_new ();
  ogmjob_container_add (OGMJOB_CONTAINER (task), pipeline);
  g_object_unref (pipeline);

  argv = ogmrip_wav_command (OGMRIP_AUDIO_CODEC (task), FALSE, fifo);
  if (argv)
  {
    child = ogmjob_spawn_newv (argv);
    ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (child), (OGMJobWatch) ogmrip_mplayer_wav_watch, task);
    ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
    g_object_unref (child);

    argv = ogmrip_vorbis_command (OGMRIP_AUDIO_CODEC (task), FALSE, fifo);
    if (argv)
    {
      child = ogmjob_spawn_newv (argv);
      ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
      g_object_unref (child);

      result = OGMJOB_TASK_CLASS (ogmrip_vorbis_parent_class)->run (task, cancellable, error);
    }
  }

  ogmjob_container_remove (OGMJOB_CONTAINER (task), pipeline);

  if (g_file_test (fifo, G_FILE_TEST_EXISTS))
    g_unlink (fifo);
  g_free (fifo);

  return result;
}

void
ogmrip_module_load (OGMRipModule *module)
{
  gboolean have_mplayer, have_oggenc;
  gchar *fullname;

  have_mplayer = ogmrip_check_mplayer ();

  fullname = g_find_program_in_path (OGGENC);
  have_oggenc = fullname != NULL;
  g_free (fullname);

  if (!have_mplayer && !have_oggenc)
  {
    g_warning (_("MPlayer and OggEnc are missing"));
    return;
  }

  if (!have_mplayer)
  {
    g_warning (_("MPlayer is missing"));
    return;
  }

  if (!have_oggenc)
  {
    g_warning (_("OggEnc is missing"));
    return;
  }

  ogmrip_register_codec (OGMRIP_TYPE_VORBIS,
      "vorbis", _("Ogg Vorbis"), OGMRIP_FORMAT_VORBIS, NULL);
}

