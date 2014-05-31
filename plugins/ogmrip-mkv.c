/* OGMRipMkv - A Matroska plugin for OGMRip
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
#include <ogmrip-module.h>

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#define MKVMERGE "mkvmerge"
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

  gboolean webm;
};

struct _OGMRipMatroskaClass
{
  OGMRipContainerClass parent_class;
};

typedef struct _OGMRipWebm      OGMRipWebm;
typedef struct _OGMRipWebmClass OGMRipWebmClass;

struct _OGMRipWebm
{
  OGMRipMatroska parent_instance;
};

struct _OGMRipWebmClass
{
  OGMRipMatroskaClass parent_class;
};

static gboolean ogmrip_matroska_run (OGMJobTask   *task,
                                     GCancellable *cancellable,
                                     GError       **error);

G_DEFINE_TYPE (OGMRipMatroska, ogmrip_matroska, OGMRIP_TYPE_CONTAINER)

static gboolean
ogmrip_matroska_watch (OGMJobTask *task, const gchar *buffer, OGMRipContainer *matroska, GError **error)
{
  gulong frames, total;
  guint percent;

  if (sscanf (buffer, "progress: %lu/%lu frames (%u%%)", &frames, &total, &percent) == 3)
    ogmjob_task_set_progress (task, percent / 100.0);
  else if (sscanf (buffer, "Progress: %u%%", &percent) == 1)
    ogmjob_task_set_progress (task, percent / 100.0);

  return TRUE;
}

static void
ogmrip_matroska_append_video_file (OGMRipContainer *matroska, OGMRipFile *file, GPtrArray *argv)
{
  g_ptr_array_add (argv, g_strdup ("--command-line-charset"));
  g_ptr_array_add (argv, g_strdup ("UTF-8"));

  g_ptr_array_add (argv, g_strdup ("-d"));

  if (ogmrip_stream_get_format (OGMRIP_STREAM (file)) == OGMRIP_FORMAT_VP8)
    g_ptr_array_add (argv, g_strdup ("1"));
  else
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

      iso639_2 = ogmrip_language_to_iso639_2 (language);
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
  const gchar *filename;
  gchar *real_filename;
  gboolean do_merge;
  struct stat buf;

  filename = ogmrip_file_get_path (file);

  if (ogmrip_stream_get_format (OGMRIP_STREAM (file)) == OGMRIP_FORMAT_VOBSUB)
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

      iso639_2 = ogmrip_language_to_iso639_2 (language);
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

      iso639_2 = ogmrip_language_to_iso639_2 (language);
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
  else if (OGMRIP_IS_CHAPTERS_STREAM (file) && OGMRIP_MATROSKA (matroska)->webm == FALSE)
    ogmrip_matroska_append_chapters_file (matroska, file, argv);
}

static OGMJobTask *
ogmrip_matroska_command (OGMRipContainer *matroska)
{
  OGMJobTask *task;

  GPtrArray *argv;
  const gchar *label, *fourcc;
  guint tsize, tnumber;

  argv = g_ptr_array_new_full (20, g_free);
  g_ptr_array_add (argv, g_strdup (MKVMERGE));

  if (OGMRIP_MATROSKA (matroska)->webm)
    g_ptr_array_add (argv, g_strdup ("--webm"));

  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_file_get_path (ogmrip_container_get_output (matroska)));

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

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (task),
      (OGMJobWatch) ogmrip_matroska_watch, matroska);

  return task;
}

static gint
ogmrip_matroska_get_overhead (OGMRipContainer *container)
{
  return MKV_OVERHEAD;
}

static void
ogmrip_matroska_class_init (OGMRipMatroskaClass *klass)
{
  OGMJobTaskClass *task_class;
  OGMRipContainerClass *container_class;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_matroska_run;

  container_class = OGMRIP_CONTAINER_CLASS (klass);
  container_class->get_overhead = ogmrip_matroska_get_overhead;
}

static void
ogmrip_matroska_init (OGMRipMatroska *matroska)
{
}

static gboolean
ogmrip_matroska_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobTask *child;
  gboolean result;

  child = ogmrip_matroska_command (OGMRIP_CONTAINER (task));
  ogmjob_container_add (OGMJOB_CONTAINER (task), child);
  g_object_unref (child);

  result = OGMJOB_TASK_CLASS (ogmrip_matroska_parent_class)->run (task, cancellable, error);

  /*
   * If mkvmerge returns 1, it's only a warning
   */
  if (!result)
  {
    gint status;

    status = ogmjob_spawn_get_status (OGMJOB_SPAWN (child));
    if (WIFEXITED (status) && WEXITSTATUS (status) == 1)
      result = TRUE;
  }

  ogmjob_container_remove (OGMJOB_CONTAINER (task), child);

  return result;
}

G_DEFINE_TYPE (OGMRipWebm, ogmrip_webm, OGMRIP_TYPE_MATROSKA)

static void
ogmrip_webm_class_init (OGMRipWebmClass *klass)
{
}

static void
ogmrip_webm_init (OGMRipWebm *webm)
{
  OGMRIP_MATROSKA (webm)->webm = TRUE;
}

static gint mkv_formats[] =
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

static gint webm_formats[] =
{
  OGMRIP_FORMAT_VORBIS,
  OGMRIP_FORMAT_VP8,
  -1
};

void
ogmrip_module_load (OGMRipModule *module)
{
  gchar *output;

  if (!g_spawn_command_line_sync (MKVMERGE " --list-types", &output, NULL, NULL, NULL))
    g_warning (_("mkvmerge is missing"));
  else
  {
    gboolean have_webm;
    guint i = 0;

    while (mkv_formats[i] != -1)
      i++;

    if (strstr (output, " drc ") || strstr (output, " Dirac "))
      mkv_formats[i++] = OGMRIP_FORMAT_DIRAC;

    if (strstr (output, " ivf ") || strstr (output, " IVF "))
      mkv_formats[i++] = OGMRIP_FORMAT_VP8;

    have_webm = strstr (output, " webm ") != NULL ||
      strstr (output, " WebM ") != NULL;

    g_free (output);

    ogmrip_register_container (OGMRIP_TYPE_MATROSKA,
        "mkv", _("Matroska Media (MKV)"), mkv_formats, NULL);

    if (have_webm)
      ogmrip_register_container (ogmrip_webm_get_type (),
          "webm", _("WebM Media (webm)"), webm_formats, NULL);
  }
}

