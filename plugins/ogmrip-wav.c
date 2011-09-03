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

#include <stdio.h>
#include <glib/gi18n-lib.h>

#define OGMRIP_TYPE_WAV          (ogmrip_wav_get_type ())
#define OGMRIP_WAV(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_WAV, OGMRipWav))
#define OGMRIP_WAV_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_WAV, OGMRipWavClass))
#define OGMRIP_IS_WAV(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_WAV))
#define OGMRIP_IS_WAV_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_WAV))

typedef struct _OGMRipWav      OGMRipWav;
typedef struct _OGMRipWavClass OGMRipWavClass;

struct _OGMRipWav
{
  OGMRipAudioCodec parent_instance;

  gboolean header;
};

struct _OGMRipWavClass
{
  OGMRipAudioCodecClass parent_class;
};

static gint ogmrip_wav_run (OGMJobSpawn *spawn);

G_DEFINE_TYPE (OGMRipWav, ogmrip_wav, OGMRIP_TYPE_AUDIO_CODEC)

static gchar **
ogmrip_wav_command (OGMRipAudioCodec *audio, gboolean header, const gchar *input, const gchar *output)
{
  GPtrArray *argv;

  argv = ogmrip_mplayer_wav_command (audio, TRUE, output);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static void
ogmrip_wav_class_init (OGMRipWavClass *klass)
{
  OGMJobSpawnClass *spawn_class;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);

  spawn_class->run = ogmrip_wav_run;
}

static void
ogmrip_wav_init (OGMRipWav *wav)
{
  wav->header = TRUE;
}

static gint
ogmrip_wav_run (OGMJobSpawn *spawn)
{
  OGMJobSpawn *child;
  gchar **argv;
  gint result;

  argv = ogmrip_wav_command (OGMRIP_AUDIO_CODEC (spawn), 
      OGMRIP_WAV (spawn)->header, NULL, NULL);
  if (!argv)
    return OGMJOB_RESULT_ERROR;

  child = ogmjob_exec_newv (argv);
  ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_mplayer_wav_watch, spawn, TRUE, FALSE, FALSE);
  ogmjob_container_add (OGMJOB_CONTAINER (spawn), child);
  g_object_unref (child);

  result = OGMJOB_SPAWN_CLASS (ogmrip_wav_parent_class)->run (spawn);

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), child);

  return result;
}

static OGMRipAudioPlugin wav_plugin =
{
  NULL,
  G_TYPE_NONE,
  "wav",
  N_("Wave (uncompressed PCM)"),
  OGMRIP_FORMAT_PCM
};

OGMRipAudioPlugin *
ogmrip_init_plugin (GError **error)
{
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (!ogmrip_check_mplayer ())
  {
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("MPlayer is missing"));
    return NULL;
  }

  wav_plugin.type = OGMRIP_TYPE_WAV;

  return &wav_plugin;
}

