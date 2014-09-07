/* OGMRipWav - A WAV plugin for OGMRip
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

// #include <stdio.h>
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

static gboolean ogmrip_wav_run (OGMJobTask   *task,
                                GCancellable *cancellable,
                                GError       **error);

G_DEFINE_TYPE (OGMRipWav, ogmrip_wav, OGMRIP_TYPE_AUDIO_CODEC)

static void
ogmrip_wav_class_init (OGMRipWavClass *klass)
{
  OGMJobTaskClass *task_class;

  task_class = OGMJOB_TASK_CLASS (klass);

  task_class->run = ogmrip_wav_run;
}

static void
ogmrip_wav_init (OGMRipWav *wav)
{
  wav->header = TRUE;
}

static gboolean
ogmrip_wav_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobTask *child;
  gboolean result;

  child = ogmrip_audio_encoder_new (OGMRIP_AUDIO_CODEC (task), OGMRIP_ENCODER_WAV, NULL,
      ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (task))));
  ogmjob_container_add (OGMJOB_CONTAINER (task), child);
  g_object_unref (child);

  result = OGMJOB_TASK_CLASS (ogmrip_wav_parent_class)->run (task, cancellable, error);

  ogmjob_container_remove (OGMJOB_CONTAINER (task), child);

  return result;
}

void
ogmrip_module_load (OGMRipModule *module)
{
  if (!ogmrip_check_mplayer ())
    g_warning (_("MPlayer is missing"));
  else
    ogmrip_register_codec (OGMRIP_TYPE_WAV,
        "wav", _("Wave (uncompressed PCM)"), OGMRIP_FORMAT_PCM, NULL);
}

