/* OGMRipFlac - A Flac plugin for OGMRip
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

#include <ogmrip-base.h>
#include <ogmrip-encode.h>
#include <ogmrip-mplayer.h>
#include <ogmrip-module.h>

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#define OGMRIP_TYPE_FLAC          (ogmrip_flac_get_type ())
#define OGMRIP_FLAC(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_FLAC, OGMRipFlac))
#define OGMRIP_FLAC_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_FLAC, OGMRipFlacClass))
#define OGMRIP_IS_FLAC(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_FLAC))
#define OGMRIP_IS_FLAC_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_FLAC))

typedef struct _OGMRipFlac      OGMRipFlac;
typedef struct _OGMRipFlacClass OGMRipFlacClass;

struct _OGMRipFlac
{
  OGMRipAudioCodec parent_instance;
};

struct _OGMRipFlacClass
{
  OGMRipAudioCodecClass parent_class;
};

static OGMJobTask *
ogmrip_flac_command (OGMRipAudioCodec *audio, gboolean header, const gchar *input)
{
  OGMJobTask *task;

  GPtrArray *argv;
  const gchar *output;
  gint quality;

  output = ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (audio)));

  argv = g_ptr_array_new_full (20, g_free);
  g_ptr_array_add (argv, g_strdup ("flac"));

  g_ptr_array_add (argv, g_strdup_printf ("--force"));
  g_ptr_array_add (argv, g_strdup_printf ("--ogg"));

  if (!header)
  {
    g_ptr_array_add (argv, g_strdup ("--force-raw-format"));
    g_ptr_array_add (argv, g_strdup ("--endian=little"));
    g_ptr_array_add (argv, g_strdup_printf ("--sample-rate=%d",
          ogmrip_audio_codec_get_sample_rate (audio)));
  }

  quality = ogmrip_audio_codec_get_quality (audio);
  g_ptr_array_add (argv, g_strdup_printf ("--compression-level-%d", CLAMP (quality, 0, 8)));
  if (quality > 8)
    g_ptr_array_add (argv, g_strdup ("--exhaustive-model-search"));

  g_ptr_array_add (argv, g_strdup_printf ("-o"));
  g_ptr_array_add (argv, g_strdup (output));

  g_ptr_array_add (argv, g_strdup (input));
  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  return task;
}

G_DEFINE_TYPE (OGMRipFlac, ogmrip_flac, OGMRIP_TYPE_AUDIO_CODEC)

static gboolean
ogmrip_flac_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobTask *pipeline;
  OGMJobTask *child;

  gchar *fifo;
  gboolean result;

  fifo = ogmrip_fs_mkftemp ("fifo.XXXXXX", error);
  if (!fifo)
    return FALSE;

  pipeline = ogmjob_pipeline_new ();
  ogmjob_container_add (OGMJOB_CONTAINER (task), pipeline);
  g_object_unref (pipeline);

  child = ogmrip_audio_encoder_new (OGMRIP_AUDIO_CODEC (task), OGMRIP_ENCODER_WAV, NULL, fifo);
  ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
  g_object_unref (child);

  child = ogmrip_flac_command (OGMRIP_AUDIO_CODEC (task), TRUE, fifo);
  ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
  g_object_unref (child);

  result = OGMJOB_TASK_CLASS (ogmrip_flac_parent_class)->run (task, cancellable, error);

  ogmjob_container_remove (OGMJOB_CONTAINER (task), pipeline);

  g_unlink (fifo);
  g_free (fifo);

  return result;
}

static void
ogmrip_flac_class_init (OGMRipFlacClass *klass)
{
  OGMJobTaskClass *task_class;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_flac_run;
}

static void
ogmrip_flac_init (OGMRipFlac *flac)
{
}

void
ogmrip_module_load (OGMRipModule *module)
{
  gboolean have_mplayer, have_flac;
  gchar *fullname;

  have_mplayer = ogmrip_check_mplayer ();

  fullname = g_find_program_in_path ("flac");
  have_flac = fullname != NULL;
  g_free (fullname);

  if (!have_mplayer && !have_flac)
  {
    g_warning (_("MPlayer and FLAC are missing"));
    return;
  }

  if (!have_mplayer)
  {
    g_warning (_("MPlayer is missing"));
    return;
  }

  if (!have_flac)
  {
    g_warning (_("FLAC is missing"));
    return;
  }

  ogmrip_register_codec (OGMRIP_TYPE_FLAC,
      "flac", _("Free Lossless Audio Codec (FLAC)"), OGMRIP_FORMAT_FLAC, NULL);
}

