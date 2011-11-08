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

#define OGMRIP_TYPE_OGG          (ogmrip_ogg_get_type ())
#define OGMRIP_OGG(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_OGG, OGMRipOgg))
#define OGMRIP_OGG_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_OGG, OGMRipOggClass))
#define OGMRIP_IS_OGG(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_OGG))
#define OGMRIP_IS_OGG_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_OGG))

typedef struct _OGMRipOgg      OGMRipOgg;
typedef struct _OGMRipOggClass OGMRipOggClass;

struct _OGMRipOgg
{
  OGMRipContainer parent_instance;
};

struct _OGMRipOggClass
{
  OGMRipContainerClass parent_class;
};

static gboolean ogmrip_ogg_run (OGMJobTask   *task,
                                GCancellable *cancellable,
                                GError       **error);

static gboolean
ogmrip_ogg_merge_watch (OGMJobTask *task, const gchar *buffer, OGMRipContainer *ogg, GError **error)
{
  gulong frames, total;
  guint percent, steps;

  ogmrip_container_get_split (ogg, &steps, NULL);
  steps = steps > 1 ? 2 : 1;

  if (sscanf (buffer, "progress: %lu/%lu frames (%u%%)", &frames, &total, &percent) == 3)
    ogmjob_task_set_progress (task, percent / (steps * 100.0));

  return TRUE;
}

static gboolean
ogmrip_ogg_split_watch (OGMJobTask *task, const gchar *buffer, OGMRipContainer *ogg, GError **error)
{
  gulong frames, total;
  guint percent;

  if (sscanf (buffer, "Processing bytes %lu/%lu (%u%%)", &frames, &total, &percent) == 3)
    ogmjob_task_set_progress (task, 0.5 + percent / 400.0);
  else if (sscanf (buffer, "Processing frame %lu/%lu (%u%%)", &frames, &total, &percent) == 3)
    ogmjob_task_set_progress (task, 0.5 + percent / 400.0);

  return TRUE;
}

static void
ogmrip_ogg_merge_append_video_file (OGMRipContainer *ogg, OGMRipFile *file, GPtrArray *argv)
{
  g_ptr_array_add (argv, g_strdup ("--noaudio"));
  g_ptr_array_add (argv, g_strdup (ogmrip_file_get_path (file)));
}

static void
ogmrip_ogg_merge_append_audio_file (OGMRipContainer *ogg, OGMRipFile *file, GPtrArray *argv)
{
  const gchar *filename;
  struct stat buf;

  filename = ogmrip_file_get_path (file);
  if (g_stat (filename, &buf) == 0 && buf.st_size > 0)
  {
    gint language;
    glong sync;

    language = ogmrip_audio_stream_get_language (OGMRIP_AUDIO_STREAM (file));
    if (language > -1)
    {
      g_ptr_array_add (argv, g_strdup ("-c"));
      g_ptr_array_add (argv, g_strdup_printf ("LANGUAGE=%s", 
            g_strdup (ogmrip_language_get_label (language))));
    }

    sync = ogmrip_container_get_sync (ogg);
    if (sync)
    {
      g_ptr_array_add (argv, g_strdup ("--sync"));
      g_ptr_array_add (argv, g_strdup_printf ("%ld", sync));
    }

    g_ptr_array_add (argv, g_strdup ("--novideo"));
    g_ptr_array_add (argv, g_strdup ("--notext"));

    g_ptr_array_add (argv, g_strdup (filename));
  }
}

static void
ogmrip_ogg_merge_append_subp_file (OGMRipContainer *ogg, OGMRipFile *file, GPtrArray *argv)
{
  const gchar *filename;
  struct stat buf;

  filename = ogmrip_file_get_path (file);
  if (g_stat (filename, &buf) == 0 && buf.st_size > 0)
  {
    gint language;

    language = ogmrip_subp_stream_get_language (OGMRIP_SUBP_STREAM (file));
    if (language > -1)
    {
      g_ptr_array_add (argv, g_strdup ("-c"));
      g_ptr_array_add (argv, g_strdup_printf ("LANGUAGE=%s",
            g_strdup (ogmrip_language_get_label (language))));
    }

    g_ptr_array_add (argv, g_strdup ("--novideo"));
    g_ptr_array_add (argv, g_strdup ("--noaudio"));

    g_ptr_array_add (argv, g_strdup (filename));
  }
}

#if (defined(GLIB_SIZEOF_SIZE_T) && GLIB_SIZEOF_SIZE_T == 4)
static void
ogmrip_ogg_merge_append_chapter_file (OGMRipContainer *ogg, OGMRipFile *file, GPtrArray *argv)
{
  const gchar *filename;
  struct stat buf;

  filename = ogmrip_file_get_path (file);
  if (g_stat (filename, &buf) == 0 && buf.st_size > 0)
  {
    gint language;

    language = ogmrip_chapters_stream_get_language (OGMRIP_CHAPTERS_STREAM (file));
    if (language > -1)
    {
      g_ptr_array_add (argv, g_strdup ("-c"));
      g_ptr_array_add (argv, g_strdup_printf ("LANGUAGE=%s",
            g_strdup (ogmrip_language_get_label (language))));
    }

    g_ptr_array_add (argv, g_strdup ("--novideo"));
    g_ptr_array_add (argv, g_strdup ("--noaudio"));

    g_ptr_array_add (argv, g_strdup (filename));
  }
}
#endif

static void
ogmrip_ogg_merge_foreach_file (OGMRipContainer *ogg, OGMRipFile *file, GPtrArray *argv)
{
  if (OGMRIP_IS_VIDEO_STREAM (file))
    ogmrip_ogg_merge_append_video_file (ogg, file, argv);
  else if (OGMRIP_IS_AUDIO_STREAM (file))
    ogmrip_ogg_merge_append_audio_file (ogg, file, argv);
  else if (OGMRIP_IS_SUBP_STREAM (file))
    ogmrip_ogg_merge_append_subp_file (ogg, file, argv);
#if (defined(GLIB_SIZEOF_SIZE_T) && GLIB_SIZEOF_SIZE_T == 4)
  /*
   * ogmmerge segfaults when merging chapters on platforms other than 32-bit
   */
  else if (OGMRIP_IS_CHAPTERS_STREAM (file))
    ogmrip_ogg_merge_append_chapter_file (ogg, file, argv);
#endif

}

static gchar **
ogmrip_ogg_merge_command (OGMRipContainer *ogg, const gchar *output)
{
  GPtrArray *argv;
  const gchar *label, *fourcc;

  if (!output)
    output = ogmrip_container_get_output (ogg);
  g_return_val_if_fail (output != NULL, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("ogmmerge"));

  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (output));

  fourcc = ogmrip_container_get_fourcc (ogg);
  if (fourcc)
  {
    g_ptr_array_add (argv, g_strdup ("--fourcc"));
    g_ptr_array_add (argv, g_strdup (fourcc));
  }

  label = ogmrip_container_get_label (ogg);
  if (label)
  {
    g_ptr_array_add (argv, g_strdup ("-c"));
    g_ptr_array_add (argv, g_strdup_printf ("TITLE=%s", label));
  }

  ogmrip_container_foreach_file (ogg,
      (OGMRipContainerFunc) ogmrip_ogg_merge_foreach_file, argv);

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gchar **
ogmrip_ogg_split_command (OGMRipContainer *ogg, const gchar *input)
{
  GPtrArray *argv;
  const gchar *output;
  guint tsize;

  output = ogmrip_container_get_output (ogg);
  ogmrip_container_get_split (OGMRIP_CONTAINER (ogg), NULL, &tsize);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("ogmsplit"));
  g_ptr_array_add (argv, g_strdup ("--frontend"));
  g_ptr_array_add (argv, g_strdup ("-s"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", tsize));
  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (output));
  g_ptr_array_add (argv, g_strdup (input));
  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

G_DEFINE_TYPE (OGMRipOgg, ogmrip_ogg, OGMRIP_TYPE_CONTAINER)

static void
ogmrip_ogg_class_init (OGMRipOggClass *klass)
{
  OGMJobTaskClass *task_class;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_ogg_run;
}

static void
ogmrip_ogg_init (OGMRipOgg *ogg)
{
}

static gboolean
ogmrip_ogg_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobTask *child;
  gchar **argv, *file;
  gboolean result;
  guint tnumber;
  gint fd;

  fd = 0;
  file = NULL;
  result = FALSE;

  ogmrip_container_get_split (OGMRIP_CONTAINER (task), &tnumber, NULL);
  if (tnumber > 1)
  {
    file = g_build_filename (ogmrip_fs_get_tmp_dir (), "merge.XXXXXX", NULL);

    fd = g_mkstemp (file);
    if (fd < 0)
    {
      g_free (file);
      return FALSE;
    }
  }

  argv = ogmrip_ogg_merge_command (OGMRIP_CONTAINER (task), file);
  if (argv)
  {
    child = ogmjob_spawn_newv (argv);
    ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (child), (OGMJobWatch) ogmrip_ogg_merge_watch, task);
    ogmjob_container_add (OGMJOB_CONTAINER (task), child);
    g_object_unref (child);

    result = OGMJOB_TASK_CLASS (ogmrip_ogg_parent_class)->run (task, cancellable, error);

    ogmjob_container_remove (OGMJOB_CONTAINER (task), child);
  }

  if (tnumber > 1 && result)
  {
    argv = ogmrip_ogg_split_command (OGMRIP_CONTAINER (task), file);
    if (argv)
    {
      child = ogmjob_spawn_newv (argv);
      ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (child), (OGMJobWatch) ogmrip_ogg_split_watch, task);
      ogmjob_container_add (OGMJOB_CONTAINER (task), child);
      g_object_unref (child);

      result = OGMJOB_TASK_CLASS (ogmrip_ogg_parent_class)->run (task, cancellable, error);

      ogmjob_container_remove (OGMJOB_CONTAINER (task), child);
    }
  }

  if (file)
  {
    g_unlink (file);
    g_free (file);
  }

  if (fd)
    close (fd);

  return result;
}

static OGMRipFormat formats[] =
{
  OGMRIP_FORMAT_MPEG4,
  OGMRIP_FORMAT_H264,
  OGMRIP_FORMAT_AC3,
  OGMRIP_FORMAT_COPY,
  OGMRIP_FORMAT_MP3,
  OGMRIP_FORMAT_VORBIS,
  OGMRIP_FORMAT_PCM,
  OGMRIP_FORMAT_SRT,
  -1
};

void
ogmrip_module_load (OGMRipModule *module)
{
  gboolean have_ogmmerge, have_ogmsplit;
  gchar *fullname;

  fullname = g_find_program_in_path ("ogmmerge");
  have_ogmmerge = fullname != NULL;
  g_free (fullname);

  fullname = g_find_program_in_path ("ogmsplit");
  have_ogmsplit = fullname != NULL;
  g_free (fullname);

  if (!have_ogmmerge && !have_ogmsplit)
  {
    g_warning (_("ogmmerge and ogmsplit are missing"));
    return;
  }

  if (!have_ogmmerge)
  {
    g_warning (_("ogmmerge is missing"));
    return;
  }

  if (!have_ogmsplit)
  {
    g_warning (_("ogmsplit is missing"));
    return;
  }

  ogmrip_register_container (OGMRIP_TYPE_OGG,
      "ogm", _("Ogg Media (OGM)"), formats);
}

