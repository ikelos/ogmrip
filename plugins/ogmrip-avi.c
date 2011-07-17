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
#include "ogmrip-plugin.h"
#include "ogmrip-fs.h"
#include "ogmrip-version.h"

#include "ogmjob-exec.h"
#include "ogmjob-queue.h"

#include <string.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#define AVI_OVERHEAD 24

#define OGMRIP_TYPE_AVI          (ogmrip_avi_get_type ())
#define OGMRIP_AVI(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_AVI, OGMRipAvi))
#define OGMRIP_AVI_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_AVI, OGMRipAviClass))
#define OGMRIP_IS_AVI(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_AVI))
#define OGMRIP_IS_AVI_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_AVI))

typedef struct _OGMRipAvi      OGMRipAvi;
typedef struct _OGMRipAviClass OGMRipAviClass;

struct _OGMRipAvi
{
  OGMRipContainer parent_instance;
};

struct _OGMRipAviClass
{
  OGMRipContainerClass parent_class;
};

GType ogmrip_avi_get_type (void);
static gint ogmrip_avi_run (OGMJobSpawn *spawn);
static gint ogmrip_avi_get_overhead (OGMRipContainer *container);

static void
ogmrip_avi_append_audio_file (OGMRipContainer *avi, const gchar *filename, GPtrArray *argv)
{
  struct stat buf;

  if (g_stat (filename, &buf) == 0 && buf.st_size > 0)
    g_ptr_array_add (argv, g_strdup (filename));
}

static void
ogmrip_avi_foreach_audio (OGMRipContainer *avi, 
    OGMRipCodec *codec, guint demuxer, gint language, GPtrArray *argv)
{
  const gchar *input;

  input = ogmrip_codec_get_output (codec);
  ogmrip_avi_append_audio_file (avi, input, argv);
}

static void
ogmrip_avi_foreach_file (OGMRipContainer *avi, OGMRipFile *file, GPtrArray *argv)
{
  if (ogmrip_file_get_type (file) == OGMRIP_FILE_TYPE_AUDIO)
  {
    gchar *filename;

    filename = ogmrip_file_get_filename (file);
    if (filename)
    {
      ogmrip_avi_append_audio_file (avi, filename, argv);
      g_free (filename);
    }
  }
}

static gchar **
ogmrip_avi_command (OGMRipContainer *avi, GError **error)
{
  GPtrArray *argv;
  OGMRipVideoCodec *video;
  const gchar *output, *filename, *fourcc;
  guint tsize, tnumber;

  g_return_val_if_fail (OGMRIP_IS_AVI (avi), NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("avibox"));

  output = ogmrip_container_get_output (avi);
  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (output));

  if ((video = ogmrip_container_get_video (avi)))
  {
    filename = ogmrip_codec_get_output (OGMRIP_CODEC (video));

    g_ptr_array_add (argv, g_strdup ("-n"));
    g_ptr_array_add (argv, g_strdup ("-i"));
    g_ptr_array_add (argv, g_strdup (filename));
  }

  ogmrip_container_foreach_audio (avi, 
      (OGMRipContainerCodecFunc) ogmrip_avi_foreach_audio, argv);
  ogmrip_container_foreach_file (avi, 
      (OGMRipContainerFileFunc) ogmrip_avi_foreach_file, argv);

  ogmrip_container_get_split (avi, &tnumber, &tsize);
  if (tnumber > 1)
  {
    g_ptr_array_add (argv, g_strdup ("-s"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", tsize));
  }

  fourcc = ogmrip_container_get_fourcc (avi);
  if (fourcc)
  {
    g_ptr_array_add (argv, g_strdup ("-f"));
    g_ptr_array_add (argv, g_strdup (fourcc));
  }

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gdouble
ogmrip_avi_watch (OGMJobExec *exec, const gchar *buffer, OGMRipContainer *matroska)
{
  gulong frames, total;
  guint percent;
  gchar *str;

  str = strrchr (buffer, ':');
  if (str && sscanf (str, ": %06lu-%06lu frames written (%u%%)", &frames, &total, &percent) == 3)
    return percent / 100.0;

  return -1.0;
}

static gchar **
ogmrip_copy_command (OGMRipContainer *container, const gchar *input, const gchar *ext)
{
  GPtrArray *argv;
  const gchar *filename;
  gchar *output;

  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (input != NULL, NULL);

  filename = ogmrip_container_get_output (container);
  output = ogmrip_fs_set_extension (filename, ext);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("cp"));
  g_ptr_array_add (argv, g_strdup ("-f"));
  g_ptr_array_add (argv, g_strdup (input));
  g_ptr_array_add (argv, output);
  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

G_DEFINE_TYPE (OGMRipAvi, ogmrip_avi, OGMRIP_TYPE_CONTAINER)

static void
ogmrip_avi_class_init (OGMRipAviClass *klass)
{
  OGMJobSpawnClass *spawn_class;
  OGMRipContainerClass *container_class;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmrip_avi_run;

  container_class = OGMRIP_CONTAINER_CLASS (klass);
  container_class->get_overhead = ogmrip_avi_get_overhead;
}

static void
ogmrip_avi_init (OGMRipAvi *avi)
{
}

static void
ogmrip_avi_foreach_subp (OGMRipContainer *avi, 
    OGMRipCodec *codec, guint demuxer, gint language, OGMJobContainer *queue)
{
  OGMJobSpawn *child;

  const gchar *filename;
  gchar *input, **argv = NULL;
  gint format;

  filename = ogmrip_codec_get_output (codec);

  format = ogmrip_plugin_get_subp_codec_format (G_TYPE_FROM_INSTANCE (codec));

  if (format == OGMRIP_FORMAT_SRT)
  {
    argv = ogmrip_copy_command (avi, filename, "srt");
    if (argv)
    {
      child = ogmjob_exec_newv (argv);
      ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
      g_object_unref (child);
    }
  }
  else if (format == OGMRIP_FORMAT_VOBSUB)
  {
    input = g_strconcat (filename, ".sub", NULL);
    argv = ogmrip_copy_command (avi, input, "sub");
    g_free (input);

    if (argv)
    {
      child = ogmjob_exec_newv (argv);
      ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
      g_object_unref (child);
    }

    input = g_strconcat (filename, ".idx", NULL);
    argv = ogmrip_copy_command (avi, input, "idx");
    g_free (input);

    if (argv)
    {
      child = ogmjob_exec_newv (argv);
      ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
      g_object_unref (child);
    }
  }
}

static gint
ogmrip_avi_run (OGMJobSpawn *spawn)
{
  GError *error = NULL;
  OGMJobSpawn *queue, *child;
  gchar **argv;
  gint result;

  result = OGMJOB_RESULT_ERROR;

  queue = ogmjob_queue_new ();
  ogmjob_container_add (OGMJOB_CONTAINER (spawn), queue);
  g_object_unref (queue);

  argv = ogmrip_avi_command (OGMRIP_CONTAINER (spawn), &error);
  if (!argv)
  {
    ogmjob_spawn_propagate_error (spawn, error);
    return OGMJOB_RESULT_ERROR;
  }

  child = ogmjob_exec_newv (argv);
  ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_avi_watch, spawn, TRUE, FALSE, FALSE);
  ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
  g_object_unref (child);

  ogmrip_container_foreach_subp (OGMRIP_CONTAINER (spawn), 
      (OGMRipContainerCodecFunc) ogmrip_avi_foreach_subp, queue);

  result = OGMJOB_SPAWN_CLASS (ogmrip_avi_parent_class)->run (spawn);

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), queue);

  return result;
}

static gint
ogmrip_avi_get_overhead (OGMRipContainer *container)
{
  return AVI_OVERHEAD;
}

static OGMRipContainerPlugin avi_plugin =
{
  NULL,
  G_TYPE_NONE,
  "avi",
  N_("Audio-Video Interlace (AVI)"),
  TRUE,
  TRUE,
  8,
  1,
  NULL
};

static gint formats[] = 
{
  OGMRIP_FORMAT_MPEG1,
  OGMRIP_FORMAT_MPEG2,
  OGMRIP_FORMAT_MPEG4,
  OGMRIP_FORMAT_H264,
  OGMRIP_FORMAT_DIRAC,
  OGMRIP_FORMAT_AC3,
  OGMRIP_FORMAT_COPY,
  OGMRIP_FORMAT_MP3,
  OGMRIP_FORMAT_SRT,
  OGMRIP_FORMAT_VOBSUB,
  -1
};

OGMRipContainerPlugin *
ogmrip_init_plugin (GError **error)
{
  gboolean have_avibox = FALSE;
  gchar *fullname;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  fullname = g_find_program_in_path ("avibox");
  have_avibox = fullname != NULL;
  g_free (fullname);

  if (!have_avibox)
    return NULL;

  avi_plugin.type = OGMRIP_TYPE_AVI;
  avi_plugin.formats = formats;

  return &avi_plugin;
}

