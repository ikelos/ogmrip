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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#define PROGRAM "mkvmerge"
#define MKV_OVERHEAD 4

#define OGMRIP_TYPE_MATROSKA          (ogmrip_matroska_get_type ())
#define OGMRIP_MATROSKA(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MATROSKA, OGMRipMatroska))
#define OGMRIP_MATROSKA_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_MATROSKA, OGMRipMatroskaClass))
#define OGMRIP_IS_MATROSKA(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_MATROSKA))
#define OGMRIP_IS_MATROSKA_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_MATROSKA))

typedef struct _OGMRipMatroska      OGMRipMatroska;
typedef struct _OGMRipMatroskaClass OGMRipMatroskaClass;

struct _OGMRipMatroska
{
  OGMRipContainer parent_instance;
};

struct _OGMRipMatroskaClass
{
  OGMRipContainerClass parent_class;
};

enum
{
  PROP_0,
  PROP_OVERHEAD
};

static void ogmrip_matroska_get_property (GObject     *gobject,
                                          guint       property_id,
                                          GValue      *value,
                                          GParamSpec  *pspec);
static gint ogmrip_matroska_run          (OGMJobSpawn *spawn);

static gdouble
ogmrip_matroska_watch (OGMJobExec *exec, const gchar *buffer, OGMRipContainer *matroska)
{
  gulong frames, total;
  guint percent;

  if (sscanf (buffer, "progress: %lu/%lu frames (%u%%)", &frames, &total, &percent) == 3)
    return percent / 100.0;
  else if (sscanf (buffer, "Progress: %u%%", &percent) == 1)
    return percent / 100.0;

  return -1.0;
}

static void
ogmrip_matroska_append_video_file (OGMRipContainer *matroska, OGMRipFile *file, GPtrArray *argv)
{
  g_ptr_array_add (argv, g_strdup ("--command-line-charset"));
  g_ptr_array_add (argv, g_strdup ("UTF-8"));

  g_ptr_array_add (argv, g_strdup ("-d"));
  g_ptr_array_add (argv, g_strdup ("0"));
  g_ptr_array_add (argv, g_strdup ("-A"));
  g_ptr_array_add (argv, g_strdup ("-S"));
  g_ptr_array_add (argv, g_strdup (ogmrip_file_get_path (file)));
}

static void
ogmrip_matroska_append_audio_file (OGMRipContainer *matroska, OGMRipFile *file, GPtrArray *argv)
{
  const gchar *filename;
  struct stat buf;

  filename = ogmrip_file_get_path (file);
  if (g_stat (filename, &buf) == 0 && buf.st_size > 0)
  {
    gint language;
    const gchar *label;
    glong sync;

    language = ogmrip_audio_stream_get_language (OGMRIP_AUDIO_STREAM (file));
    if (language > -1)
    {
      const gchar *iso639_2;

      iso639_2 = ogmrip_language_get_iso639_2 (language);
      if (iso639_2)
      {
        g_ptr_array_add (argv, g_strdup ("--language"));
        g_ptr_array_add (argv, g_strconcat ("0:", iso639_2, NULL));
      }
    }

    label = ogmrip_audio_stream_get_label (OGMRIP_AUDIO_STREAM (file));
    if (label)
    {
      g_ptr_array_add (argv, g_strdup ("--track-name"));
      g_ptr_array_add (argv, g_strconcat ("0:", label, NULL));
    }

    sync = ogmrip_container_get_sync (matroska);
    if (sync)
    {
      g_ptr_array_add (argv, g_strdup ("--sync"));
      g_ptr_array_add (argv, g_strdup_printf ("0:%ld", sync));
    }

    g_ptr_array_add (argv, g_strdup ("-D"));
    g_ptr_array_add (argv, g_strdup ("-S"));

    g_ptr_array_add (argv, g_strdup (filename));
  }
}

static void
ogmrip_matroska_append_subp_file (OGMRipContainer *matroska, OGMRipFile *file, GPtrArray *argv)
{
  gint format;
  const gchar *filename;
  gchar *real_filename;
  gboolean do_merge;
  struct stat buf;

  filename = ogmrip_file_get_path (file);
  format = ogmrip_stream_get_format (OGMRIP_STREAM (file));

  if (format == OGMRIP_FORMAT_VOBSUB)
  {
    if (!g_str_has_suffix (filename, ".idx"))
    {
      real_filename = g_strconcat (filename, ".sub", NULL);
      do_merge = (g_stat (real_filename, &buf) == 0 && buf.st_size != 0);

      if (do_merge)
      {
        g_free (real_filename);
        real_filename = g_strconcat (filename, ".idx", NULL);
        do_merge = (g_stat (real_filename, &buf) == 0 && buf.st_size != 0);
      }
    }
    else
    {
      real_filename = ogmrip_fs_set_extension (filename, "sub");
      do_merge = (g_stat (real_filename, &buf) == 0 && buf.st_size != 0);

      if (do_merge)
      {
        g_free (real_filename);
        real_filename = g_strdup (filename);
        do_merge = (g_stat (real_filename, &buf) == 0 && buf.st_size != 0);
      }
    }
  }
  else
  {
    real_filename = g_strdup (filename);
    do_merge = (g_stat (real_filename, &buf) == 0 && buf.st_size != 0);
  }

  if (!do_merge)
    g_free (real_filename);
  else
  {
    const gchar *label;
    gint language;

    language = ogmrip_subp_stream_get_language (OGMRIP_SUBP_STREAM (file));
    if (language > -1)
    {
      const gchar *iso639_2;

      iso639_2 = ogmrip_language_get_iso639_2 (language);
      if (iso639_2)
      {
        g_ptr_array_add (argv, g_strdup ("--language"));
        g_ptr_array_add (argv, g_strconcat ("0:", iso639_2, NULL));
      }
    }

    label = ogmrip_subp_stream_get_label (OGMRIP_SUBP_STREAM (file));
    if (label)
    {
      g_ptr_array_add (argv, g_strdup ("--track-name"));
      g_ptr_array_add (argv, g_strconcat ("0:", label, NULL));
    }

    switch (ogmrip_subp_stream_get_charset (OGMRIP_SUBP_STREAM (file)))
    {
      case OGMRIP_CHARSET_UTF8:
        g_ptr_array_add (argv, g_strdup ("--sub-charset"));
        g_ptr_array_add (argv, g_strdup ("0:UTF-8"));
        break;
      case OGMRIP_CHARSET_ISO8859_1:
        g_ptr_array_add (argv, g_strdup ("--sub-charset"));
        g_ptr_array_add (argv, g_strdup ("0:ISO-8859-1"));
        break;
      case OGMRIP_CHARSET_ASCII:
        g_ptr_array_add (argv, g_strdup ("--sub-charset"));
        g_ptr_array_add (argv, g_strdup ("0:ASCII"));
        break;
      default:
        break;
    }

    g_ptr_array_add (argv, g_strdup ("-s"));
    g_ptr_array_add (argv, g_strdup ("0"));
    g_ptr_array_add (argv, g_strdup ("-D"));
    g_ptr_array_add (argv, g_strdup ("-A"));

    g_ptr_array_add (argv, real_filename);
  }
}

static void
ogmrip_matroska_append_chapters_file (OGMRipContainer *matroska, OGMRipFile *file, GPtrArray *argv)
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
      const gchar *iso639_2;

      iso639_2 = ogmrip_language_get_iso639_2 (language);
      if (iso639_2)
      {
        g_ptr_array_add (argv, g_strdup ("--chapter-language"));
        g_ptr_array_add (argv, g_strdup (iso639_2));
      }
    }

    g_ptr_array_add (argv, g_strdup ("--chapter-charset"));
    g_ptr_array_add (argv, g_strdup ("UTF-8"));
    g_ptr_array_add (argv, g_strdup ("--chapters"));

    g_ptr_array_add (argv, g_strdup (filename));
  }
}

static void
ogmrip_matroska_foreach_file (OGMRipContainer *matroska, OGMRipFile *file, GPtrArray *argv)
{
  if (OGMRIP_IS_VIDEO_STREAM (file))
    ogmrip_matroska_append_video_file (matroska, file, argv);
  else if (OGMRIP_IS_AUDIO_STREAM (file))
    ogmrip_matroska_append_audio_file (matroska, file, argv);
  else if (OGMRIP_IS_SUBP_STREAM (file))
    ogmrip_matroska_append_subp_file (matroska, file, argv);
  else if (OGMRIP_IS_CHAPTERS_STREAM (file))
    ogmrip_matroska_append_chapters_file (matroska, file, argv);
}

gchar **
ogmrip_matroska_command (OGMRipContainer *matroska)
{
  GPtrArray *argv;
  const gchar *output, *label, *fourcc;
  guint tsize, tnumber;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup (PROGRAM));

  output = ogmrip_container_get_output (matroska);
  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (output));

  fourcc = ogmrip_container_get_fourcc (matroska);
  if (fourcc)
  {
    g_ptr_array_add (argv, g_strdup ("--fourcc"));
    g_ptr_array_add (argv, g_strconcat ("0:", fourcc, NULL));
  }

  ogmrip_container_foreach_file (matroska,
      (OGMRipContainerFunc) ogmrip_matroska_foreach_file, argv);

  label = ogmrip_container_get_label (matroska);
  if (label)
  {
    g_ptr_array_add (argv, g_strdup ("--title"));
    g_ptr_array_add (argv, g_strdup_printf ("%s", label));
  }

  ogmrip_container_get_split (matroska, &tnumber, &tsize);
  if (tnumber > 1)
  {
    g_ptr_array_add (argv, g_strdup ("--split"));
    g_ptr_array_add (argv, g_strdup_printf ("%dM", tsize));
  }

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

G_DEFINE_TYPE (OGMRipMatroska, ogmrip_matroska, OGMRIP_TYPE_CONTAINER)

static void
ogmrip_matroska_class_init (OGMRipMatroskaClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_matroska_get_property;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmrip_matroska_run;

  g_object_class_install_property (gobject_class, PROP_OVERHEAD, 
        g_param_spec_uint ("overhead", "Overhead property", "Get overhead", 
           0, G_MAXUINT, MKV_OVERHEAD, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_matroska_init (OGMRipMatroska *matroska)
{
}

static void
ogmrip_matroska_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_OVERHEAD:
      g_value_set_uint (value, MKV_OVERHEAD);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static gint
ogmrip_matroska_run (OGMJobSpawn *spawn)
{
  OGMJobSpawn *child;
  gchar **argv;
  gint result;

  argv = ogmrip_matroska_command (OGMRIP_CONTAINER (spawn));
  if (!argv)
    return OGMJOB_RESULT_ERROR;

  child = ogmjob_exec_newv (argv);
  ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_matroska_watch, spawn, TRUE, FALSE, FALSE);
  ogmjob_container_add (OGMJOB_CONTAINER (spawn), child);
  g_object_unref (child);

  result = OGMJOB_SPAWN_CLASS (ogmrip_matroska_parent_class)->run (spawn);

  /*
   * If mkvmerge returns 1, it's only a warning
   */
  if (ogmjob_exec_get_status (OGMJOB_EXEC (child)) == 1)
    result = OGMJOB_RESULT_SUCCESS;

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), child);

  return result;
}

static gint formats[] =
{
  OGMRIP_FORMAT_MPEG1,
  OGMRIP_FORMAT_MPEG2,
  OGMRIP_FORMAT_MPEG4,
  OGMRIP_FORMAT_H264,
  OGMRIP_FORMAT_THEORA,
  OGMRIP_FORMAT_AC3,
  OGMRIP_FORMAT_DTS,
  OGMRIP_FORMAT_COPY,
  OGMRIP_FORMAT_AAC,
  OGMRIP_FORMAT_MP3,
  OGMRIP_FORMAT_VORBIS,
  OGMRIP_FORMAT_PCM,
  OGMRIP_FORMAT_SRT,
  OGMRIP_FORMAT_VOBSUB,
  OGMRIP_FORMAT_SSA,
  OGMRIP_FORMAT_FLAC,
  -1,
  -1,
  -1
};

gboolean
ogmrip_init_plugin (GError **error)
{
  gchar *output;
  guint i = 0;

  if (!g_spawn_command_line_sync ("mkvmerge --list-types", &output, NULL, NULL, NULL))
  {
    // g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("mkvmerge is missing"));
    return FALSE;
  }

  while (formats[i] != -1)
    i++;

  if (strstr (output, " drc ") || strstr (output, " Dirac "))
    formats[i++] = OGMRIP_FORMAT_DIRAC;

  if (strstr (output, " ivf ") || strstr (output, " IVF "))
    formats[i++] = OGMRIP_FORMAT_VP8;

  g_free (output);

  ogmrip_type_register_container (NULL, OGMRIP_TYPE_MATROSKA,
      "mkv", N_("Matroska Media (MKV)"), formats);

  return TRUE;
}

