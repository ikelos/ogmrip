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

#include <ogmjob.h>
#include <ogmrip.h>

#include <math.h>
#include <unistd.h>
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

GType ogmrip_ogg_get_type (void);
static gint ogmrip_ogg_run (OGMJobSpawn *spawn);

static gdouble
ogmrip_ogg_merge_watch (OGMJobExec *exec, const gchar *buffer, OGMRipContainer *ogg)
{
  gulong frames, total;
  guint percent, steps;

  ogmrip_container_get_split (ogg, &steps, NULL);
  steps = steps > 1 ? 2 : 1;

  if (sscanf (buffer, "progress: %lu/%lu frames (%u%%)", &frames, &total, &percent) == 3)
    return percent / (steps * 100.0);

  return -1.0;
}

static gdouble
ogmrip_ogg_split_watch (OGMJobExec *exec, const gchar *buffer, OGMRipContainer *ogg)
{
  gulong frames, total;
  guint percent;

  if (sscanf (buffer, "Processing bytes %lu/%lu (%u%%)", &frames, &total, &percent) == 3)
    return 0.5 + percent / 400.0;
  else if (sscanf (buffer, "Processing frame %lu/%lu (%u%%)", &frames, &total, &percent) == 3)
    return 0.5 + percent / 400.0;

  return -1.0;
}

static gchar *
ogmrip_ogg_get_sync (OGMRipContainer *container)
{
/*
  OGMDvdTitle *title;
  guint num, denom;
  gint start_delay;
  gchar *buf;

  start_delay = ogmrip_container_get_start_delay (container);
  if (start_delay <= 0)
    return NULL;

  title = NULL;
  if (ogmdvd_title_get_telecine (title) || ogmdvd_title_get_progressive (title))
  {
    num = 24000;
    denom = 1001;
  }
  else
  {
    OGMDvdVideoStream *stream;

    stream = ogmdvd_title_get_video_stream (title);
    ogmdvd_video_stream_get_framerate (OGMDVD_VIDEO_STREAM (stream), &num, &denom);
  }

  buf = g_new0 (gchar, G_ASCII_DTOSTR_BUF_SIZE);
  g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%.0f", (start_delay * denom * 1000) / (gdouble) num);

  return buf;
*/
  return NULL;
}

static void
ogmrip_ogg_merge_append_audio_file (OGMRipContainer *ogg, 
    const char *filename, gint language, GPtrArray *argv)
{
  struct stat buf;

  if (g_stat (filename, &buf) == 0 && buf.st_size > 0)
  {
    gchar *sync;

    if (language > -1)
    {
      g_ptr_array_add (argv, g_strdup ("-c"));
      g_ptr_array_add (argv, g_strdup_printf ("LANGUAGE=%s", 
            g_strdup (ogmdvd_get_language_label (language))));
    }

    sync = ogmrip_ogg_get_sync (ogg);
    if (sync)
    {
      g_ptr_array_add (argv, g_strdup ("--sync"));
      g_ptr_array_add (argv, g_strdup (sync));
      g_free (sync);
    }

    g_ptr_array_add (argv, g_strdup ("--novideo"));
    g_ptr_array_add (argv, g_strdup ("--notext"));

    g_ptr_array_add (argv, g_strdup (filename));
  }
}

static void
ogmrip_ogg_merge_append_subp_file (OGMRipContainer *ogg, 
    const gchar *filename, gint language, GPtrArray *argv)
{
  struct stat buf;

  if (g_stat (filename, &buf) == 0 && buf.st_size > 0)
  {
    if (language > -1)
    {
      g_ptr_array_add (argv, g_strdup ("-c"));
      g_ptr_array_add (argv, g_strdup_printf ("LANGUAGE=%s",
            g_strdup (ogmdvd_get_language_label (language))));
    }

    g_ptr_array_add (argv, g_strdup ("--novideo"));
    g_ptr_array_add (argv, g_strdup ("--noaudio"));

    g_ptr_array_add (argv, g_strdup (filename));
  }
}

#if (defined(GLIB_SIZEOF_SIZE_T) && GLIB_SIZEOF_SIZE_T == 4)
static void
ogmrip_ogg_merge_append_chapters_file (OGMRipContainer *ogg, 
    const gchar *filename, gint language, GPtrArray *argv)
{
  struct stat buf;

  if (g_stat (filename, &buf) == 0 && buf.st_size > 0)
  {
    if (language > -1)
    {
      g_ptr_array_add (argv, g_strdup ("-c"));
      g_ptr_array_add (argv, g_strdup_printf ("LANGUAGE=%s",
            g_strdup (ogmdvd_get_language_label (language))));
    }

    g_ptr_array_add (argv, g_strdup ("--novideo"));
    g_ptr_array_add (argv, g_strdup ("--noaudio"));

    g_ptr_array_add (argv, g_strdup (filename));
  }
}
#endif

static void
ogmrip_ogg_merge_foreach_file (OGMRipContainer *ogg, const gchar *filename,
    OGMRipFormatType format, const gchar *name, guint language, GPtrArray *argv)
{
  if (OGMRIP_IS_VIDEO_FORMAT (format))
  {
    g_ptr_array_add (argv, g_strdup ("--noaudio"));
    g_ptr_array_add (argv, g_strdup (filename));
  }
  else if (OGMRIP_IS_AUDIO_FORMAT (format))
    ogmrip_ogg_merge_append_audio_file (ogg, filename, language, argv);
  else if (OGMRIP_IS_SUBP_FORMAT (format))
    ogmrip_ogg_merge_append_subp_file (ogg, filename, language, argv);
#if (defined(GLIB_SIZEOF_SIZE_T) && GLIB_SIZEOF_SIZE_T == 4)
  /*
   * ogmmerge segfaults when merging chapters on platforms other than 32-bit
   */
  else if (OGMRIP_IS_CHAPTERS_FORMAT (format))
    ogmrip_ogg_merge_append_chapters_file (ogg,  filename, language, argv);
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
  OGMJobSpawnClass *spawn_class;
  OGMRipContainerClass *container_class;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  container_class = OGMRIP_CONTAINER_CLASS (klass);

  spawn_class->run = ogmrip_ogg_run;
}

static void
ogmrip_ogg_init (OGMRipOgg *ogg)
{
}

static gint
ogmrip_ogg_run (OGMJobSpawn *spawn)
{
  OGMJobSpawn *child;
  gchar **argv, *file;
  gint result, fd;
  guint tnumber;

  fd = 0;
  file = NULL;
  result = OGMJOB_RESULT_ERROR;

  ogmrip_container_get_split (OGMRIP_CONTAINER (spawn), &tnumber, NULL);
  if (tnumber > 1)
  {
    file = g_build_filename (ogmrip_fs_get_tmp_dir (), "merge.XXXXXX", NULL);

    fd = g_mkstemp (file);
    if (fd < 0)
    {
      g_free (file);
      return OGMJOB_RESULT_ERROR;
    }
  }

  argv = ogmrip_ogg_merge_command (OGMRIP_CONTAINER (spawn), file);
  if (argv)
  {
    child = ogmjob_exec_newv (argv);
    ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_ogg_merge_watch, spawn, TRUE, FALSE, FALSE);
    ogmjob_container_add (OGMJOB_CONTAINER (spawn), child);
    g_object_unref (child);

    result = OGMJOB_SPAWN_CLASS (ogmrip_ogg_parent_class)->run (spawn);

    ogmjob_container_remove (OGMJOB_CONTAINER (spawn), child);
  }

  if (tnumber > 1 && result == OGMJOB_RESULT_SUCCESS)
  {
    argv = ogmrip_ogg_split_command (OGMRIP_CONTAINER (spawn), file);
    if (argv)
    {
      child = ogmjob_exec_newv (argv);
      ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_ogg_split_watch, spawn, TRUE, FALSE, FALSE);
      ogmjob_container_add (OGMJOB_CONTAINER (spawn), child);
      g_object_unref (child);

      result = OGMJOB_SPAWN_CLASS (ogmrip_ogg_parent_class)->run (spawn);

      ogmjob_container_remove (OGMJOB_CONTAINER (spawn), child);
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

static OGMRipContainerPlugin ogg_plugin =
{
  NULL,
  G_TYPE_NONE,
  "ogm",
  N_("Ogg Media (OGM)"),
  FALSE,
  TRUE,
  G_MAXINT,
  G_MAXINT,
  NULL
};

static gint formats[] =
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

OGMRipContainerPlugin *
ogmrip_init_plugin (GError **error)
{
  gboolean have_ogmmerge, have_ogmsplit;
  gchar *fullname;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  fullname = g_find_program_in_path ("ogmmerge");
  have_ogmmerge = fullname != NULL;
  g_free (fullname);

  fullname = g_find_program_in_path ("ogmsplit");
  have_ogmsplit = fullname != NULL;
  g_free (fullname);

  ogg_plugin.type = OGMRIP_TYPE_OGG;
  ogg_plugin.formats = formats;

  if (have_ogmmerge && have_ogmsplit)
    return &ogg_plugin;

  if (!have_ogmmerge && !have_ogmsplit)
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("ogmmerge and ogmsplit are missing"));
  else if (!have_ogmmerge)
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("ogmmerge is missing"));
  else if (!have_ogmsplit)
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("ogmsplit is missing"));

  return NULL;
}

