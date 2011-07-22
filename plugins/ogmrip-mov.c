/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-container.h"
#include "ogmrip-mplayer.h"
#include "ogmrip-version.h"
#include "ogmrip-plugin.h"
#include "ogmjob-exec.h"

#include <string.h>
#include <glib/gi18n-lib.h>

#define OGMRIP_TYPE_MOV          (ogmrip_mov_get_type ())
#define OGMRIP_MOV(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MOV, OGMRipMov))
#define OGMRIP_MOV_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_MOV, OGMRipMovClass))
#define OGMRIP_IS_MOV(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_MOV))
#define OGMRIP_IS_MOV_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_MOV))

typedef struct _OGMRipMov      OGMRipMov;
typedef struct _OGMRipMovClass OGMRipMovClass;

struct _OGMRipMov
{
  OGMRipContainer parent_instance;
};

struct _OGMRipMovClass
{
  OGMRipContainerClass parent_class;
};

GType ogmrip_mov_get_type (void);
static gint ogmrip_mov_run (OGMJobSpawn *spawn);

gchar **
ogmrip_mov_command (OGMRipContainer *mov, GError **error)
{
  GPtrArray *argv;
  OGMRipVideoCodec *video;
  const gchar *output, *filename;

  video = ogmrip_container_get_video (mov);
  if (!video)
  {
    g_set_error (error, 0, 0, _("An MOV file must contain a video stream."));
    return NULL;
  }

  argv = ogmrip_mencoder_container_command (mov);

  g_ptr_array_add (argv, g_strdup ("-of"));
  g_ptr_array_add (argv, g_strdup ("lavf"));
  g_ptr_array_add (argv, g_strdup ("-lavfopts"));

  if (MPLAYER_CHECK_VERSION (1,0,2,0))
    g_ptr_array_add (argv, g_strdup ("format=mov"));
  else
    g_ptr_array_add (argv, g_strdup ("format=mov:i_certify_that_my_video_stream_does_not_use_b_frames"));

  output = ogmrip_container_get_output (mov);
  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (output));

  filename = ogmrip_codec_get_output (OGMRIP_CODEC (video));
  g_ptr_array_add (argv, g_strdup (filename));

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

G_DEFINE_TYPE (OGMRipMov, ogmrip_mov, OGMRIP_TYPE_CONTAINER)

static void
ogmrip_mov_class_init (OGMRipMovClass *klass)
{
  OGMJobSpawnClass *spawn_class;
  OGMRipContainerClass *container_class;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  container_class = OGMRIP_CONTAINER_CLASS (klass);

  spawn_class->run = ogmrip_mov_run;
}

static void
ogmrip_mov_init (OGMRipMov *mov)
{
}

static gint
ogmrip_mov_run (OGMJobSpawn *spawn)
{
  GError *error = NULL;
  OGMJobSpawn *child;
  gchar **argv;
  gint result;

  argv = ogmrip_mov_command (OGMRIP_CONTAINER (spawn), &error);
  if (!argv)
  {
    ogmjob_spawn_propagate_error (spawn, error);
    return OGMJOB_RESULT_ERROR;
  }

  child = ogmjob_exec_newv (argv);
  ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_mencoder_container_watch, spawn, TRUE, FALSE, FALSE);
  ogmjob_container_add (OGMJOB_CONTAINER (spawn), child);
  g_object_unref (child);

  result = OGMJOB_SPAWN_CLASS (ogmrip_mov_parent_class)->run (spawn);

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), child);

  return result;
}

static OGMRipContainerPlugin mov_plugin =
{
  NULL,
  G_TYPE_NONE,
  "mov",
  N_("QuickTime Media (MOV)"),
  TRUE,
  FALSE,
  1,
  1,
  NULL
};

static gint formats[] =
{
  OGMRIP_FORMAT_MPEG4,
  OGMRIP_FORMAT_H264,
  OGMRIP_FORMAT_THEORA,
  OGMRIP_FORMAT_AAC,
  OGMRIP_FORMAT_AC3,
  OGMRIP_FORMAT_DTS,
  OGMRIP_FORMAT_COPY,
  OGMRIP_FORMAT_MP3,
  OGMRIP_FORMAT_VORBIS,
  OGMRIP_FORMAT_PCM,
  OGMRIP_FORMAT_SRT,
  OGMRIP_FORMAT_VOBSUB,
  -1
};

OGMRipContainerPlugin *
ogmrip_init_plugin (GError **error)
{
  gchar *output;
  gboolean match;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (!ogmrip_check_mencoder ())
  {
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("MEncoder is missing"));
    return NULL;
  }

  if (!g_spawn_command_line_sync ("mencoder -of help", &output, NULL, NULL, NULL))
    return NULL;

  match = g_regex_match_simple ("^ *lavf *- .*$", output, G_REGEX_MULTILINE, 0);
  g_free (output);

  if (!match)
  {
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("MEncoder is build without lavf support"));
    return NULL;
  }

  mov_plugin.type = OGMRIP_TYPE_MOV;
  mov_plugin.formats = formats;

  return &mov_plugin;
}

