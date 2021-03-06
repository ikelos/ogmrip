/* OGMRipAac - An AAC plugin for OGMRip
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

#define FAAC "faac"

#define OGMRIP_TYPE_AAC          (ogmrip_aac_get_type ())
#define OGMRIP_AAC(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_AAC, OGMRipAac))
#define OGMRIP_AAC_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_AAC, OGMRipAacClass))
#define OGMRIP_IS_AAC(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_AAC))
#define OGMRIP_IS_AAC_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_AAC))

typedef struct _OGMRipAac      OGMRipAac;
typedef struct _OGMRipAacClass OGMRipAacClass;

struct _OGMRipAac
{
  OGMRipAudioCodec parent_instance;
};

struct _OGMRipAacClass
{
  OGMRipAudioCodecClass parent_class;
};

GType ogmrip_aac_get_type (void);

static OGMJobTask *
ogmrip_aac_command (OGMRipAudioCodec *audio, gboolean header, const gchar *input)
{
  OGMJobTask *task;

  GPtrArray *argv;
  const gchar *output;
  gint quality;

  quality = ogmrip_audio_codec_get_quality (audio);

  argv = g_ptr_array_new_full (20, g_free);
  g_ptr_array_add (argv, g_strdup (FAAC));

  if (!header)
  {
    g_ptr_array_add (argv, g_strdup ("-P"));
    g_ptr_array_add (argv, g_strdup ("-R"));
    g_ptr_array_add (argv, g_strdup_printf ("%d",
          ogmrip_audio_codec_get_sample_rate (audio)));
    g_ptr_array_add (argv, g_strdup ("-C"));
    g_ptr_array_add (argv, g_strdup_printf ("%d",
          ogmrip_audio_codec_get_channels (audio) + 1));
    g_ptr_array_add (argv, g_strdup ("-X"));
  }

  g_ptr_array_add (argv, g_strdup ("-q"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", quality * 49 + 10));
  g_ptr_array_add (argv, g_strdup ("--mpeg-vers"));
  g_ptr_array_add (argv, g_strdup ("4"));
  g_ptr_array_add (argv, g_strdup ("-o"));

  output = ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (audio)));
  g_ptr_array_add (argv, g_strdup (output));

  g_ptr_array_add (argv, g_strdup (input));
  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  return task;
}

G_DEFINE_TYPE (OGMRipAac, ogmrip_aac, OGMRIP_TYPE_AUDIO_CODEC);

static gboolean
ogmrip_aac_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
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

  child = ogmrip_audio_encoder_new (OGMRIP_AUDIO_CODEC (task), OGMRIP_ENCODER_PCM, NULL, fifo);
  ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
  g_object_unref (child);

  child = ogmrip_aac_command (OGMRIP_AUDIO_CODEC (task), FALSE, fifo);
  ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
  g_object_unref (child);

  result = OGMJOB_TASK_CLASS (ogmrip_aac_parent_class)->run (task, cancellable, error);

  ogmjob_container_remove (OGMJOB_CONTAINER (task), pipeline);

  g_unlink (fifo);
  g_free (fifo);

  return result;
}

static void
ogmrip_aac_class_init (OGMRipAacClass *klass)
{
  OGMJobTaskClass *task_class;

  task_class = OGMJOB_TASK_CLASS (klass);

  task_class->run = ogmrip_aac_run;
}

static void
ogmrip_aac_init (OGMRipAac *aac)
{
}

void
ogmrip_module_load (OGMRipModule *module)
{
  gboolean have_mplayer, have_faac;
  gchar *fullname;

  have_mplayer = ogmrip_check_mplayer ();

  fullname = g_find_program_in_path (FAAC);
  have_faac = fullname != NULL;
  g_free (fullname);

  if (!have_mplayer && !have_faac)
  {
    g_warning (_("MPlayer and FAAC are missing"));
    return;
  }

  if (!have_mplayer)
  {
    g_warning (_("MPlayer is missing"));
    return;
  }

  if (!have_faac)
  {
    g_warning (_("FAAC is missing"));
    return;
  }

  ogmrip_register_codec (OGMRIP_TYPE_AAC,
      "aac", _("Advanced Audio Coding (AAC)"), OGMRIP_FORMAT_AAC, NULL);
}

