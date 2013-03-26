/* OGMRipAvi - An AVI plugin for OGMRip
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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
#include <ogmrip-module.h>

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

enum
{
  PROP_0,
  PROP_NVIDEO,
  PROP_NAUDIO,
  PROP_NSUBP
};

static void     ogmrip_avi_get_property (GObject      *gobject,
                                         guint        property_id,
                                         GValue       *value,
                                         GParamSpec   *pspec);
static gboolean ogmrip_avi_run          (OGMJobTask   *task,
                                         GCancellable *cancellable,
                                         GError       **error);

static void
ogmrip_avi_add_file (OGMRipContainer *avi, OGMRipFile *file, gboolean video, GPtrArray *argv)
{
  const gchar *filename;
  struct stat buf;

  filename = ogmrip_file_get_path (file);

  if (g_stat (filename, &buf) == 0 && buf.st_size > 0)
  {
    if (video)
    {
      g_ptr_array_add (argv, g_strdup ("-n"));
      g_ptr_array_add (argv, g_strdup ("-i"));
    }
    g_ptr_array_add (argv, g_strdup (filename));
  }
}

static void
ogmrip_avi_add_video_file (OGMRipContainer *avi, GPtrArray *argv)
{
  GList *list, *link;

  list = ogmrip_container_get_files (avi);
  for (link = list; link; link = link->next)
    if (OGMRIP_IS_VIDEO_STREAM (link->data))
      break;

  if (link)
    ogmrip_avi_add_file (avi, link->data, TRUE, argv);

  g_list_free (list);
}

static void
ogmrip_avi_foreach_audio_file (OGMRipContainer *avi, OGMRipFile *file, GPtrArray *argv)
{
  if (OGMRIP_IS_AUDIO_STREAM (file))
    ogmrip_avi_add_file (avi, file, FALSE, argv);
}

static gchar **
ogmrip_avi_command (OGMRipContainer *avi, GError **error)
{
  GPtrArray *argv;
  const gchar *fourcc;
  guint tsize, tnumber;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("avibox"));

  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_file_get_path (ogmrip_container_get_output (avi)));

  ogmrip_avi_add_video_file (avi, argv);

  ogmrip_container_foreach_file (avi,
      (OGMRipContainerFunc) ogmrip_avi_foreach_audio_file, argv);

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

static gboolean
ogmrip_avi_watch (OGMJobTask *task, const gchar *buffer, OGMRipContainer *matroska, GError **error)
{
  gulong frames, total;
  guint percent;
  gchar *str;

  str = strrchr (buffer, ':');
  if (str && sscanf (str, ": %06lu-%06lu frames written (%u%%)", &frames, &total, &percent) == 3)
    ogmjob_task_set_progress (task, percent / 100.0);

  return TRUE;
}

static gchar **
ogmrip_copy_command (OGMRipContainer *container, const gchar *input, const gchar *ext)
{
  GPtrArray *argv;
  gchar *filename, *output;

  filename = g_file_get_path (ogmrip_container_get_output (container));
  output = ogmrip_fs_set_extension (filename, ext);
  g_free (filename);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("cp"));
  g_ptr_array_add (argv, g_strdup ("-f"));
  g_ptr_array_add (argv, g_strdup (input));
  g_ptr_array_add (argv, output);
  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gint
ogmrip_avi_get_overhead (OGMRipContainer *container)
{
  return AVI_OVERHEAD;
}

G_DEFINE_TYPE (OGMRipAvi, ogmrip_avi, OGMRIP_TYPE_CONTAINER)

static void
ogmrip_avi_class_init (OGMRipAviClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobTaskClass *task_class;
  OGMRipContainerClass *container_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_avi_get_property;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_avi_run;

  container_class = OGMRIP_CONTAINER_CLASS (klass);
  container_class->get_overhead = ogmrip_avi_get_overhead;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_NAUDIO, 
        g_param_spec_uint ("naudio", "Number of audio streams property", "Get number of audio streams", 
           0, 8, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_NSUBP, 
        g_param_spec_uint ("nsubp", "Number of subp streams property", "Get number of subp streams", 
           0, 1, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_avi_init (OGMRipAvi *avi)
{
}

static void
ogmrip_avi_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_NAUDIO:
      G_OBJECT_CLASS (ogmrip_avi_parent_class)->get_property (gobject, property_id, value, pspec);
      break;
    case PROP_NSUBP:
      G_OBJECT_CLASS (ogmrip_avi_parent_class)->get_property (gobject, property_id, value, pspec);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_avi_foreach_subp (OGMRipContainer *avi, OGMRipFile *file, OGMJobTask *queue)
{
  OGMJobTask *child;
  const gchar *filename;
  gchar *input, **argv;
  gint format;

  filename = ogmrip_file_get_path (file);
  format = ogmrip_stream_get_format (OGMRIP_STREAM (file));

  if (format == OGMRIP_FORMAT_SRT)
  {
    argv = ogmrip_copy_command (avi, filename, "srt");
    if (argv)
    {
      child = ogmjob_spawn_newv (argv);
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
      child = ogmjob_spawn_newv (argv);
      ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
      g_object_unref (child);
    }

    input = g_strconcat (filename, ".idx", NULL);
    argv = ogmrip_copy_command (avi, input, "idx");
    g_free (input);

    if (argv)
    {
      child = ogmjob_spawn_newv (argv);
      ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
      g_object_unref (child);
    }
  }
}

static gboolean
ogmrip_avi_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobTask *queue, *child;
  gchar **argv;
  gboolean result;

  queue = ogmjob_queue_new ();
  ogmjob_container_add (OGMJOB_CONTAINER (task), queue);
  g_object_unref (queue);

  argv = ogmrip_avi_command (OGMRIP_CONTAINER (task), error);
  if (!argv)
    return FALSE;

  child = ogmjob_spawn_newv (argv);
  ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (child), (OGMJobWatch) ogmrip_avi_watch, task);
  ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
  g_object_unref (child);

  ogmrip_container_foreach_file (OGMRIP_CONTAINER (task), 
      (OGMRipContainerFunc) ogmrip_avi_foreach_subp, queue);

  result = OGMJOB_TASK_CLASS (ogmrip_avi_parent_class)->run (task, cancellable, error);

  ogmjob_container_remove (OGMJOB_CONTAINER (task), queue);

  return result;
}

static OGMRipFormat formats[] = 
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

void
ogmrip_module_load (OGMRipModule *module)
{
  gboolean have_avibox = FALSE;
  gchar *fullname;

  fullname = g_find_program_in_path ("avibox");
  have_avibox = fullname != NULL;
  g_free (fullname);

  if (!have_avibox)
    g_warning (_("avibox is missing"));
  else
    ogmrip_register_container (OGMRIP_TYPE_AVI,
        "avi", _("Audio-Video Interlace (AVI)"), formats, NULL);
}

