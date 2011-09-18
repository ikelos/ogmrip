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
  PROP_OVERHEAD
};

static void ogmrip_avi_get_property (GObject     *gobject,
                                     guint       property_id,
                                     GValue      *value,
                                     GParamSpec  *pspec);
static gint ogmrip_avi_run          (OGMJobSpawn *spawn);

static void
ogmrip_avi_foreach_file (OGMRipContainer *avi, OGMRipFile *file, GPtrArray *argv)
{
  if (OGMRIP_IS_VIDEO_STREAM (file) || OGMRIP_IS_AUDIO_STREAM (file))
  {
    const gchar *filename;
    struct stat buf;

    filename = ogmrip_file_get_path (file);

    if (g_stat (filename, &buf) == 0 && buf.st_size > 0)
    {
      if (OGMRIP_IS_VIDEO_STREAM (file))
      {
        g_ptr_array_add (argv, g_strdup ("-n"));
        g_ptr_array_add (argv, g_strdup ("-i"));
      }

      g_ptr_array_add (argv, g_strdup (filename));
    }
  }
}

static gchar **
ogmrip_avi_command (OGMRipContainer *avi, GError **error)
{
  GPtrArray *argv;
  const gchar *output, *fourcc;
  guint tsize, tnumber;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("avibox"));

  output = ogmrip_container_get_output (avi);
  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (output));

  ogmrip_container_foreach_file (avi,
      (OGMRipContainerFunc) ogmrip_avi_foreach_file, argv);

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

G_DEFINE_DYNAMIC_TYPE (OGMRipAvi, ogmrip_avi, OGMRIP_TYPE_CONTAINER)

static void
ogmrip_avi_class_init (OGMRipAviClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_avi_get_property;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmrip_avi_run;

  g_object_class_install_property (gobject_class, PROP_OVERHEAD,
      g_param_spec_uint ("overhead", "overhead", "overhead",
        0, G_MAXUINT, AVI_OVERHEAD, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_avi_class_finalize (OGMRipAviClass *klass)
{
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
    case PROP_OVERHEAD:
      g_value_set_uint (value, AVI_OVERHEAD);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_avi_foreach_subp (OGMRipContainer *avi, OGMRipFile *file, OGMJobSpawn *queue)
{
  OGMJobSpawn *child;
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

  ogmrip_container_foreach_file (OGMRIP_CONTAINER (spawn), 
      (OGMRipContainerFunc) ogmrip_avi_foreach_subp, queue);

  result = OGMJOB_SPAWN_CLASS (ogmrip_avi_parent_class)->run (spawn);

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), queue);

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
  {
    ogmrip_avi_register_type (G_TYPE_MODULE (module));
    ogmrip_type_register_container (module,
        OGMRIP_TYPE_AVI, "avi", N_("Audio-Video Interlace (AVI)"), formats);
  }
}

