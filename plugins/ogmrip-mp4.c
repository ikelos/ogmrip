/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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
#include <ogmrip-mplayer.h>
#include <ogmrip-module.h>

#include <errno.h>
#include <stdlib.h>

#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#define MP4BOX "MP4Box"

#define OGMRIP_TYPE_MP4           (ogmrip_mp4_get_type ())
#define OGMRIP_MP4(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MP4, OGMRipMp4))
#define OGMRIP_MP4_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_MP4, OGMRipMp4Class))
#define OGMRIP_IS_MP4(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_MP4))
#define OGMRIP_IS_MP4_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_MP4))
#define OGMRIP_MP4_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_MP4, OGMRipMp4Class))

typedef struct _OGMRipMp4      OGMRipMp4;
typedef struct _OGMRipMp4Class OGMRipMp4Class;

struct _OGMRipMp4
{
  OGMRipContainer parent_instance;

  OGMRipFile *video;
  gint nvobsub;
  gint naudio;
  gint nsubp;

  guint nstreams;
  guint streams;
  guint old_percent;

  guint nsplits;
  guint splits;
  guint split_percent;
};

struct _OGMRipMp4Class
{
  OGMRipContainerClass parent_class;
};

GType           ogmrip_mp4_get_type (void);
static gboolean ogmrip_mp4_run      (OGMJobTask   *task,
                                     GCancellable *cancellable,
                                     GError       **error);

static void
ogmrip_mp4_append_audio_file (OGMRipContainer *mp4, OGMRipFile *file, GPtrArray *argv)
{
  const gchar *filename;
  struct stat buf;

  filename = ogmrip_file_get_path (file);
  if (g_stat (filename, &buf) == 0 && buf.st_size > 0)
  {
    const gchar *fmt;

    switch (ogmrip_stream_get_format (OGMRIP_STREAM (file)))
    {
      case OGMRIP_FORMAT_AAC:
        fmt = "aac";
        break;
      case OGMRIP_FORMAT_MP3:
        fmt = "mp3";
        break;
      case OGMRIP_FORMAT_VORBIS:
        fmt = "ogg";
        break;
      case OGMRIP_FORMAT_AC3:
      case OGMRIP_FORMAT_COPY:
        fmt = "ac3";
        break;
      default:
        fmt = NULL;
        break;
    }

    if (fmt)
    {
      gint language;
      const gchar *iso639_2 = NULL;

      g_ptr_array_add (argv, g_strdup ("-add"));

      language = ogmrip_audio_stream_get_language (OGMRIP_AUDIO_STREAM (file));
      if (language > -1)
        iso639_2 = ogmrip_language_get_iso639_2 (language);
      if (iso639_2)
        g_ptr_array_add (argv, g_strdup_printf ("%s:fmt=%s:lang=%s:group=1:#audio", filename, fmt, iso639_2));
      else
        g_ptr_array_add (argv, g_strdup_printf ("%s:fmt=%s:group=1:#audio", filename, fmt));
    }
  }
}

static void
ogmrip_mp4_append_subp_file (OGMRipContainer *mp4, OGMRipFile *file, GPtrArray *argv)
{
  const gchar *filename;
  struct stat buf;

  filename = ogmrip_file_get_path (file);
  if (g_stat (filename, &buf) == 0 && buf.st_size > 0)
  {
    const gchar *fmt;

    switch (ogmrip_stream_get_format (OGMRIP_STREAM (file)))
    {
      case OGMRIP_FORMAT_SRT:
        fmt = "srt";
        break;
      case OGMRIP_FORMAT_VOBSUB:
        fmt = "vobsub";
        break;
      default:
        fmt = NULL;
        break;
    }

    if (fmt)
    {
      gint language;
      const gchar *iso639_2 = NULL;

      g_ptr_array_add (argv, g_strdup ("-add"));

      language = ogmrip_subp_stream_get_language (OGMRIP_SUBP_STREAM (file));
      if (language > -1)
        iso639_2 = ogmrip_language_get_iso639_2 (language);
      if (iso639_2)
        g_ptr_array_add (argv, g_strdup_printf ("%s:fmt=%s:lang=%s", filename, fmt, iso639_2));
      else
        g_ptr_array_add (argv, g_strdup_printf ("%s:fmt=%s", filename, fmt));
    }
  }
}

static void
ogmrip_mp4_append_chapters_file (OGMRipContainer *mp4, OGMRipFile *file, GPtrArray *argv)
{
  const gchar *filename;
  struct stat buf;

  filename = ogmrip_file_get_path (file);
  if (g_stat (filename, &buf) == 0 && buf.st_size > 0)
  {
    g_ptr_array_add (argv, g_strdup ("-chap"));
    g_ptr_array_add (argv, g_strdup (filename));
  }
}

static void
ogmrip_mp4_append_file (OGMRipContainer *mp4, OGMRipFile *file, GPtrArray *argv)
{
  if (OGMRIP_IS_AUDIO_STREAM (file))
    ogmrip_mp4_append_audio_file (mp4, file, argv);
  else if (OGMRIP_IS_SUBP_STREAM (file))
    ogmrip_mp4_append_subp_file (mp4, file, argv);
  else if (OGMRIP_IS_CHAPTERS_STREAM (file))
    ogmrip_mp4_append_chapters_file (mp4, file, argv);
}

static gchar **
ogmrip_mp4box_extract_command (const gchar *input)
{
  GPtrArray *argv;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup (MP4BOX));
  g_ptr_array_add (argv, g_strdup ("-aviraw"));
  g_ptr_array_add (argv, g_strdup ("video"));
  g_ptr_array_add (argv, g_strdup (input));

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gboolean
ogmrip_mp4box_extract_watch (OGMJobTask *task, const gchar *buffer, OGMRipContainer *mp4, GError **error)
{
  gchar *sep;
  guint percent;

  if ((sep = strrchr (buffer, '(')) && sscanf (sep, "(%u/100)", &percent) == 1)
    ogmjob_task_set_progress (task, percent / 100.0);

  return TRUE;
}

static gchar **
ogmrip_mencoder_extract_command (const gchar *input, const gchar *output)
{
  GPtrArray *argv;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("mencoder"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-noskip"));

  g_ptr_array_add (argv, g_strdup ("-noconfig"));
  g_ptr_array_add (argv, g_strdup ("all"));

  g_ptr_array_add (argv, g_strdup ("-mc"));
  g_ptr_array_add (argv, g_strdup ("0"));

  g_ptr_array_add (argv, g_strdup ("-nosound"));

  if (ogmrip_check_mplayer_nosub ())
    g_ptr_array_add (argv, g_strdup ("-nosub"));

  g_ptr_array_add (argv, g_strdup ("-ovc"));
  g_ptr_array_add (argv, g_strdup ("copy"));

  g_ptr_array_add (argv, g_strdup ("-of"));
  g_ptr_array_add (argv, g_strdup ("lavf"));
  g_ptr_array_add (argv, g_strdup ("-lavfopts"));
  g_ptr_array_add (argv, g_strdup ("format=mp4"));

  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (output));

  g_ptr_array_add (argv, g_strdup (input));

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gchar **
ogmrip_mp4_create_command (OGMRipContainer *mp4, const gchar *input, const gchar *output)
{
  GPtrArray *argv;
  const gchar *label, *fmt = NULL;
  gchar fps[8];

  switch (ogmrip_stream_get_format (OGMRIP_STREAM (OGMRIP_MP4 (mp4)->video)))
  {
    case OGMRIP_FORMAT_MPEG4:
      fmt = "mpeg4-video";
      break;
    case OGMRIP_FORMAT_MPEG2:
      fmt = "mpeg2-video";
      break;
    case OGMRIP_FORMAT_H264:
      fmt = "h264";
      break;
    case OGMRIP_FORMAT_THEORA:
      fmt = "ogg";
      break;
    default:
      fmt = NULL;
      break;
  }

  if (!fmt)
    return NULL;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup (MP4BOX));

  if (OGMRIP_MP4 (mp4)->naudio <= 1 && OGMRIP_MP4 (mp4)->nsubp < 1)
    g_ptr_array_add (argv, g_strdup ("-isma"));

  g_ptr_array_add (argv, g_strdup ("-nodrop"));
  g_ptr_array_add (argv, g_strdup ("-new"));

  g_ptr_array_add (argv, g_strdup ("-brand"));
  g_ptr_array_add (argv, g_strdup ("mp42"));

  g_ptr_array_add (argv, g_strdup ("-tmp"));
  g_ptr_array_add (argv, g_strdup (ogmrip_fs_get_tmp_dir ()));

  label = ogmrip_container_get_label (mp4);
  if (label)
  {
    g_ptr_array_add (argv, g_strdup ("-itags"));
    g_ptr_array_add (argv, g_strdup_printf ("name=%s", label));
  }

  if (fmt)
  {
    guint num, denom;

    if (!input)
      input = ogmrip_file_get_path (OGMRIP_MP4 (mp4)->video);

    ogmrip_video_stream_get_framerate (OGMRIP_VIDEO_STREAM (OGMRIP_MP4 (mp4)->video), &num, &denom);
    g_ascii_formatd (fps, 8, "%.3f", num / (gdouble) denom);

    g_ptr_array_add (argv, g_strdup ("-add"));
    g_ptr_array_add (argv, g_strdup_printf ("%s:fmt=%s:fps=%s#video", input, fmt, fps));
  }

  ogmrip_container_foreach_file (mp4, 
      (OGMRipContainerFunc) ogmrip_mp4_append_file, argv);

  g_ptr_array_add (argv, g_strdup (output));

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gboolean
ogmrip_mp4_create_watch (OGMJobTask *task, const gchar *buffer, OGMRipMp4 *mp4, GError **error)
{
  guint percent;
  gchar *sep;

  if ((sep = strrchr (buffer, '(')) && sscanf (sep, "(%u/100)", &percent) == 1)
  {
    if (percent < mp4->old_percent)
      mp4->streams ++;

    mp4->old_percent = percent;

    ogmjob_task_set_progress (task, mp4->streams / (gdouble) mp4->nstreams + percent / (mp4->nstreams * 100.0));
  }

  return TRUE;
}

static gchar **
ogmrip_mp4_split_command (OGMRipContainer *mp4, const gchar *input)
{
  GPtrArray *argv;
  guint tsize;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup (MP4BOX));

  g_ptr_array_add (argv, g_strdup ("-tmp"));
  g_ptr_array_add (argv, g_strdup (ogmrip_fs_get_tmp_dir ()));

  ogmrip_container_get_split (OGMRIP_CONTAINER (mp4), NULL, &tsize);
  g_ptr_array_add (argv, g_strdup ("-splits"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", tsize));

  g_ptr_array_add (argv, g_strdup (input));

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gboolean
ogmrip_mp4_split_watch (OGMJobTask *task, const gchar *buffer, OGMRipMp4 *mp4, GError **error)
{
  gchar *sep;
  guint percent;

  if ((sep = strrchr (buffer, '(')) && sscanf (sep, "(%u/100)", &percent) == 1)
  {
    if (g_str_has_prefix (buffer, "Splitting:"))
    {
      mp4->split_percent = percent;

      ogmjob_task_set_progress (task, (percent + 100 * mp4->splits) / (100.0 * (mp4->nsplits + 1)));
    }
    else if (g_str_has_prefix (buffer, "ISO File Writing:"))
    {
      if (percent < mp4->split_percent)
        mp4->splits ++;

      ogmjob_task_set_progress (task, (percent + mp4->split_percent + 100 * mp4->splits) / (100.0 * (mp4->nsplits + 1)));
    }
  }

  return TRUE;
}

static gchar *
ogmrip_mp4_get_h264_filename (const gchar *input)
{
  gchar *dot, *filename;

  dot = strrchr (input, '.');

  filename = g_new0 (gchar, dot - input + 12);
  strncpy (filename, input, dot - input);
  strcat (filename, "_video.h264");

  return filename;
}

static void
ogmrip_mp4_get_info (OGMRipMp4 *mp4, OGMRipFile *file)
{
  if (OGMRIP_IS_VIDEO_STREAM (file))
    mp4->video = file;
  else if (OGMRIP_IS_AUDIO_STREAM (file))
    mp4->naudio ++;
  else if (OGMRIP_IS_SUBP_STREAM (file))
  {
    mp4->nsubp ++;
    if (ogmrip_stream_get_format (OGMRIP_STREAM (file)) == OGMRIP_FORMAT_VOBSUB)
      mp4->nvobsub ++;
  }
}

G_DEFINE_TYPE (OGMRipMp4, ogmrip_mp4, OGMRIP_TYPE_CONTAINER)

static void
ogmrip_mp4_class_init (OGMRipMp4Class *klass)
{
  OGMJobTaskClass *task_class;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_mp4_run;
}

static void
ogmrip_mp4_init (OGMRipMp4 *mp4)
{
}

static gboolean
ogmrip_mp4_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMRipMp4 *mp4 = OGMRIP_MP4 (task);
  OGMJobTask *queue, *child;

  gchar **argv, *filename = NULL;
  const gchar *output;

  gboolean result = FALSE;

  output = ogmrip_container_get_output (OGMRIP_CONTAINER (task));
  ogmrip_container_get_split (OGMRIP_CONTAINER (task), &mp4->nsplits, NULL);

  mp4->naudio = mp4->nsubp = mp4->nvobsub = 0;
  ogmrip_container_foreach_file (OGMRIP_CONTAINER (task),
      (OGMRipContainerFunc) ogmrip_mp4_get_info, NULL);

  queue = ogmjob_queue_new ();
  ogmjob_container_add (OGMJOB_CONTAINER (task), queue);
  g_object_unref (queue);

  if (ogmrip_stream_get_format (OGMRIP_STREAM (mp4->video)) == OGMRIP_FORMAT_H264)
  {
    gboolean global_header = FALSE;
/*
    if (g_object_class_find_property (G_OBJECT_GET_CLASS (video), "global_header"))
      g_object_get (video, "global_header", &global_header, NULL);
*/
    if (global_header)
    {
      filename = ogmrip_fs_mktemp ("video.XXXXXX", NULL);

      argv = ogmrip_mencoder_extract_command (ogmrip_file_get_path (mp4->video), filename);
      if (!argv)
      {
        g_free (filename);
        return FALSE;
      }

      child = ogmjob_spawn_newv (argv);
      ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (child), (OGMJobWatch) ogmrip_mencoder_container_watch, task);
    }
    else
    {
      argv = ogmrip_mp4box_extract_command (ogmrip_file_get_path (mp4->video));
      if (!argv)
        return FALSE;

      child = ogmjob_spawn_newv (argv);
      ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (child), (OGMJobWatch) ogmrip_mp4box_extract_watch, task);

      filename = ogmrip_mp4_get_h264_filename (ogmrip_file_get_path (mp4->video));
    }

    ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
    g_object_unref (child);
  }

  argv = ogmrip_mp4_create_command (OGMRIP_CONTAINER (task), filename, output);
  if (argv)
  {
    mp4->old_percent = 0;
    mp4->nstreams = 2 + mp4->naudio + mp4->nvobsub;
    mp4->streams = 0;

    child = ogmjob_spawn_newv (argv);
    ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (child), (OGMJobWatch) ogmrip_mp4_create_watch, task);
    ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
    g_object_unref (child);

    if (mp4->nsplits > 1 && result)
    {
      argv = ogmrip_mp4_split_command (OGMRIP_CONTAINER (task), output);
      if (argv)
      {
        mp4->split_percent = 0;
        mp4->splits = 0;

        child = ogmjob_spawn_newv (argv);
        ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (child), (OGMJobWatch) ogmrip_mp4_split_watch, task);
        ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
        g_object_unref (child);
      }
    }

    result = OGMJOB_TASK_CLASS (ogmrip_mp4_parent_class)->run (task, cancellable, error);
  }

  ogmjob_container_remove (OGMJOB_CONTAINER (task), queue);

  if (filename)
  {
    g_unlink (filename);
    g_free (filename);
  }

  if (mp4->nsplits > 1)
    g_unlink (output);

  return result;
}

static OGMRipFormat formats[] = 
{
  OGMRIP_FORMAT_MPEG4,
  OGMRIP_FORMAT_MPEG2,
  OGMRIP_FORMAT_H264,
  OGMRIP_FORMAT_THEORA,
  OGMRIP_FORMAT_AAC,
  OGMRIP_FORMAT_MP3,
  OGMRIP_FORMAT_VORBIS,
  OGMRIP_FORMAT_SRT,
  OGMRIP_FORMAT_VOBSUB,
  -1,
  -1,
  -1
};

void
ogmrip_module_load (OGMRipModule *module)
{
  gchar *output;
  gint major_version = 0, minor_version = 0, micro_version = 0;

  if (!g_spawn_command_line_sync (MP4BOX " -version", &output, NULL, NULL, NULL))
  {
    g_warning (_("MP4Box is missing"));
    return;
  }

  if (g_str_has_prefix (output, "MP4Box - GPAC version "))
  {
    gchar *end;

    errno = 0;
    major_version = strtoul (output + 22, &end, 10);
    if (!errno && *end == '.')
      minor_version = strtoul (end + 1, NULL, 10);
    if (!errno && *end == '.')
      micro_version = strtoul (end + 1, NULL, 10);
  }
  g_free (output);

  if ((major_version > 0) ||
      (major_version == 0 && minor_version > 4) ||
      (major_version == 0 && minor_version == 4 && micro_version >= 5))
  {
    guint i = 0;

    while (formats[i] != -1)
      i++;

    formats[i] = OGMRIP_FORMAT_AC3;
    formats[i+1] = OGMRIP_FORMAT_COPY;
  }

  ogmrip_register_container (OGMRIP_TYPE_MP4,
      "mp4", _("Mpeg-4 Media (MP4)"), formats);
}

