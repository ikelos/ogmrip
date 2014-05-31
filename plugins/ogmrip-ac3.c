/* OGMRipAc3 - An AC3 plugin for OGMRip
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

#define AFTEN "aften"

#define OGMRIP_TYPE_AC3          (ogmrip_ac3_get_type ())
#define OGMRIP_AC3(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_AC3, OGMRipAc3))
#define OGMRIP_AC3_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_AC3, OGMRipAc3Class))
#define OGMRIP_IS_AC3(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_AC3))
#define OGMRIP_IS_AC3_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_AC3))

typedef struct _OGMRipAc3      OGMRipAc3;
typedef struct _OGMRipAc3Class OGMRipAc3Class;

struct _OGMRipAc3
{
  OGMRipAudioCodec parent_instance;
};

struct _OGMRipAc3Class
{
  OGMRipAudioCodecClass parent_class;
};

static gboolean ogmrip_ac3_run (OGMJobTask   *task,
                                GCancellable *cancellable,
                                GError       **error);

static const guint16 a52_bitratetab[] =
{
   32,  40,  48,  56,  64,  80,  96, 112, 128, 160,
  192, 224, 256, 320, 384, 448, 512, 576, 640,   0
};

static OGMJobTask *
ogmrip_aften_command (OGMRipAudioCodec *audio, const gchar *input)
{
  OGMJobTask *task;
  OGMRipStream *stream;

  GPtrArray *argv;
  const gchar *output;
  gint i, bitrate;

  argv = g_ptr_array_new_full (20, g_free);
  g_ptr_array_add (argv, g_strdup (AFTEN));

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (audio));

  bitrate = ogmrip_audio_stream_get_bitrate (OGMRIP_AUDIO_STREAM (stream)) / 1000;
  bitrate = 128 + ((bitrate - 128) * ogmrip_audio_codec_get_quality (audio) / 10);

  for (i = 0; a52_bitratetab[i]; i++)
    if (a52_bitratetab[i] > bitrate)
      break;

  bitrate = a52_bitratetab[i] ? a52_bitratetab[i] : 640;

  g_ptr_array_add (argv, g_strdup ("-b"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", bitrate));

  g_ptr_array_add (argv, g_strdup (input));

  output = ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (audio)));
  g_ptr_array_add (argv, g_strdup (output));

  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  return task;
}

G_DEFINE_TYPE (OGMRipAc3, ogmrip_ac3, OGMRIP_TYPE_AUDIO_CODEC);

static void
ogmrip_ac3_class_init (OGMRipAc3Class *klass)
{
  OGMJobTaskClass *task_class;

  task_class = OGMJOB_TASK_CLASS (klass);

  task_class->run = ogmrip_ac3_run;
}

static void
ogmrip_ac3_init (OGMRipAc3 *ac3)
{
}

static gboolean
ogmrip_ac3_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
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

  child = ogmrip_aften_command (OGMRIP_AUDIO_CODEC (task), fifo);
  ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
  g_object_unref (child);

  result = OGMJOB_TASK_CLASS (ogmrip_ac3_parent_class)->run (task, cancellable, error);

  ogmjob_container_remove (OGMJOB_CONTAINER (task), pipeline);

  g_unlink (fifo);
  g_free (fifo);

  return result;
}

void
ogmrip_module_load (OGMRipModule *module)
{
  gboolean have_mplayer, have_aften;
  gchar *fullname;

  have_mplayer = ogmrip_check_mplayer ();

  fullname = g_find_program_in_path (AFTEN);
  have_aften = fullname != NULL;
  g_free (fullname);

  if (!have_mplayer && !have_aften)
  {
    g_warning (_("MPlayer and aften are missing"));
    return;
  }

  if (!have_mplayer)
  {
    g_warning (_("MPlayer is missing"));
    return;
  }

  if (!have_aften)
  {
    g_warning (_("aften is missing"));
    return;
  }

  ogmrip_register_codec (OGMRIP_TYPE_AC3,
      "ac3", _("Dolby Digital (AC3)"), OGMRIP_FORMAT_AC3, NULL);
}

