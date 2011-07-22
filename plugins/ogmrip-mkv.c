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
#include "ogmrip-fs.h"
#include "ogmrip-version.h"
#include "ogmrip-plugin.h"
#include "ogmjob-exec.h"

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

GType ogmrip_matroska_get_type (void);
static gint ogmrip_matroska_run (OGMJobSpawn *spawn);
static gint ogmrip_matroska_get_overhead (OGMRipContainer *container);

static gint major_version = 0;
static gint minor_version = 0;

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

static gchar *
ogmrip_matroska_get_sync (OGMRipContainer *container)
{
  guint start_delay;

  start_delay = ogmrip_container_get_start_delay (container);
  if (start_delay > 0)
  {
    OGMRipVideoCodec *video;
    guint num, denom;
    gchar *buf;

    video = ogmrip_container_get_video (container);
    if (ogmrip_codec_get_telecine (OGMRIP_CODEC (video)) || ogmrip_codec_get_progressive (OGMRIP_CODEC (video)))
    {
      num = 24000;
      denom = 1001;
    }
    else
    {
      OGMDvdStream *stream;

      stream = ogmrip_codec_get_input (OGMRIP_CODEC (video));
      ogmdvd_video_stream_get_framerate (OGMDVD_VIDEO_STREAM (stream), &num, &denom);
    }

    buf = g_new0 (gchar, G_ASCII_DTOSTR_BUF_SIZE);
    g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%.0f", (start_delay * denom * 1000) / (gdouble) num);

    return buf;
  }

  return NULL;
}

static void
ogmrip_matroska_append_audio_file (OGMRipContainer *matroska, const gchar *filename,
    const gchar *label, gint language, GPtrArray *argv)
{
  struct stat buf;

  if (g_stat (filename, &buf) == 0 && buf.st_size > 0)
  {
    gchar *sync;

    if (language > -1)
    {
      const gchar *iso639_2;

      iso639_2 = ogmdvd_get_language_iso639_2 (language);
      if (iso639_2)
      {
        g_ptr_array_add (argv, g_strdup ("--language"));
        g_ptr_array_add (argv, g_strconcat ("0:", iso639_2, NULL));
      }
    }

    if (label)
    {
      g_ptr_array_add (argv, g_strdup ("--track-name"));
      g_ptr_array_add (argv, g_strconcat ("0:", label, NULL));
    }

    sync = ogmrip_matroska_get_sync (matroska);
    if (sync)
    {
      g_ptr_array_add (argv, g_strdup ("--sync"));
      g_ptr_array_add (argv, g_strdup_printf ("0:%s", sync));
      g_free (sync);
    }

    g_ptr_array_add (argv, g_strdup ("-D"));
    g_ptr_array_add (argv, g_strdup ("-S"));

    g_ptr_array_add (argv, g_strdup (filename));
  }
}

static void
ogmrip_matroska_append_subp_file (OGMRipContainer *matroska, const gchar *filename,
    const gchar *label, gint demuxer, gint charset, gint language, GPtrArray *argv)
{
  gchar *real_filename;
  gboolean do_merge;
  struct stat buf;

  if (demuxer == OGMRIP_SUBP_DEMUXER_VOBSUB)
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
    if (language > -1)
    {
      const gchar *iso639_2;

      iso639_2 = ogmdvd_get_language_iso639_2 (language);
      if (iso639_2)
      {
        g_ptr_array_add (argv, g_strdup ("--language"));
        g_ptr_array_add (argv, g_strconcat ("0:", iso639_2, NULL));
      }
    }

    if (label)
    {
      g_ptr_array_add (argv, g_strdup ("--track-name"));
      g_ptr_array_add (argv, g_strconcat ("0:", label, NULL));
    }

    switch (charset)
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
    }

    g_ptr_array_add (argv, g_strdup ("-s"));
    g_ptr_array_add (argv, g_strdup ("0"));
    g_ptr_array_add (argv, g_strdup ("-D"));
    g_ptr_array_add (argv, g_strdup ("-A"));

    g_ptr_array_add (argv, real_filename);
  }
}

static void
ogmrip_matroska_foreach_audio (OGMRipContainer *matroska, 
    OGMRipCodec *codec, guint demuxer, gint language, GPtrArray *argv)
{
  const gchar *input, *label;

  input = ogmrip_codec_get_output (codec);
  label = ogmrip_audio_codec_get_label (OGMRIP_AUDIO_CODEC (codec));

  ogmrip_matroska_append_audio_file (matroska, input, label, language, argv);
}

static void
ogmrip_matroska_foreach_subp (OGMRipContainer *matroska, 
    OGMRipCodec *codec, guint demuxer, gint language, GPtrArray *argv)
{
  const gchar *input, *label;
  gint charset;

  input = ogmrip_codec_get_output (codec);
  label = ogmrip_subp_codec_get_label (OGMRIP_SUBP_CODEC (codec));
  charset = ogmrip_subp_codec_get_charset (OGMRIP_SUBP_CODEC (codec));

  ogmrip_matroska_append_subp_file (matroska, input, label, demuxer, charset, language, argv);
}

static void
ogmrip_matroska_foreach_chapters (OGMRipContainer *matroska, 
    OGMRipCodec *codec, guint demuxer, gint language, GPtrArray *argv)
{
  const gchar *input;
  struct stat buf;

  input = ogmrip_codec_get_output (codec);
  if (g_stat (input, &buf) == 0 && buf.st_size > 0)
  {
    if (language > -1)
    {
      const gchar *iso639_2;

      iso639_2 = ogmdvd_get_language_iso639_2 (language);
      if (iso639_2)
      {
        g_ptr_array_add (argv, g_strdup ("--chapter-language"));
        g_ptr_array_add (argv, g_strdup (iso639_2));
      }
    }

    g_ptr_array_add (argv, g_strdup ("--chapter-charset"));
    g_ptr_array_add (argv, g_strdup ("UTF-8"));
    g_ptr_array_add (argv, g_strdup ("--chapters"));

    g_ptr_array_add (argv, g_strdup (input));
  }
}

static void
ogmrip_matroska_foreach_file (OGMRipContainer *matroska, OGMRipFile *file, GPtrArray *argv)
{
  gchar *filename;

  filename = ogmrip_file_get_filename (file);
  if (filename)
  {
    gint charset, lang;

    lang = ogmrip_file_get_language (file);

    switch (ogmrip_file_get_type (file))
    {
      case OGMRIP_FILE_TYPE_AUDIO:
        ogmrip_matroska_append_audio_file (matroska, filename, NULL, lang, argv);
        break;
      case OGMRIP_FILE_TYPE_SUBP:
        charset = ogmrip_subp_file_get_charset (OGMRIP_SUBP_FILE (file));
        ogmrip_matroska_append_subp_file (matroska, filename, NULL, OGMRIP_SUBP_DEMUXER_AUTO, charset, lang, argv);
        break;
      default:
        g_assert_not_reached ();
        break;
    }
  }
  g_free (filename);
}

gchar **
ogmrip_matroska_command (OGMRipContainer *matroska)
{
  GPtrArray *argv;
  OGMRipVideoCodec *video;
  const gchar *output, *label, *filename, *fourcc;
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

  if ((video = ogmrip_container_get_video (matroska)))
  {
    if (major_version == 1)
    {
      if (ogmrip_plugin_get_video_codec_format (G_TYPE_FROM_INSTANCE (video)) == OGMRIP_FORMAT_H264)
      {
        /*
         * Option to merge h264 streams
         */
        g_ptr_array_add (argv, g_strdup ("--engage"));
        g_ptr_array_add (argv, g_strdup ("allow_avc_in_vfw_mode"));
      }
    }

    g_ptr_array_add (argv, g_strdup ("--command-line-charset"));
    g_ptr_array_add (argv, g_strdup ("UTF-8"));

    filename = ogmrip_codec_get_output (OGMRIP_CODEC (video));

    g_ptr_array_add (argv, g_strdup ("-d"));
    g_ptr_array_add (argv, g_strdup ("0"));
    g_ptr_array_add (argv, g_strdup ("-A"));
    g_ptr_array_add (argv, g_strdup ("-S"));
    g_ptr_array_add (argv, g_strdup (filename));
  }

  ogmrip_container_foreach_audio (matroska, 
      (OGMRipContainerCodecFunc) ogmrip_matroska_foreach_audio, argv);
  ogmrip_container_foreach_subp (matroska, 
      (OGMRipContainerCodecFunc) ogmrip_matroska_foreach_subp, argv);
  ogmrip_container_foreach_chapters (matroska, 
      (OGMRipContainerCodecFunc) ogmrip_matroska_foreach_chapters, argv);
  ogmrip_container_foreach_file (matroska, 
      (OGMRipContainerFileFunc) ogmrip_matroska_foreach_file, argv);

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
  OGMJobSpawnClass *spawn_class;
  OGMRipContainerClass *container_class;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmrip_matroska_run;

  container_class = OGMRIP_CONTAINER_CLASS (klass);
  container_class->get_overhead = ogmrip_matroska_get_overhead;
}

static void
ogmrip_matroska_init (OGMRipMatroska *matroska)
{
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
   * If mkvmerge resturns 1, it's only a warning
   */
  if (ogmjob_exec_get_status (OGMJOB_EXEC (child)) == 1)
    result = OGMJOB_RESULT_SUCCESS;

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), child);

  return result;
}

static gint
ogmrip_matroska_get_overhead (OGMRipContainer *container)
{
  return MKV_OVERHEAD;
}

static OGMRipContainerPlugin mkv_plugin =
{
  NULL,
  G_TYPE_NONE,
  "mkv",
  N_("Matroska Media (MKV)"),
  FALSE,
  TRUE,
  G_MAXINT,
  G_MAXINT,
  NULL
};

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

OGMRipContainerPlugin *
ogmrip_init_plugin (GError **error)
{
  gchar *output;
  guint i = 0;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (!g_spawn_command_line_sync ("mkvmerge --version", &output, NULL, NULL, NULL))
  {
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("mkvmerge is missing"));
    return NULL;
  }

  if (strncmp (output, "mkvmerge v", 10) == 0)
  {
    gchar *end;

    errno = 0;
    major_version = strtoul (output + 10, &end, 10);
    if (!errno && *end == '.')
      minor_version = strtoul (end + 1, NULL, 10);
  }

  g_free (output);

  if (!g_spawn_command_line_sync ("mkvmerge --list-types", &output, NULL, NULL, NULL))
  {
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("mkvmerge is missing"));
    return NULL;
  }

  while (formats[i] != -1)
    i++;

  if (strstr (output, " drc ") || strstr (output, " Dirac "))
    formats[i++] = OGMRIP_FORMAT_DIRAC;

  if (strstr (output, " ivf ") || strstr (output, " IVF "))
    formats[i++] = OGMRIP_FORMAT_VP8;

  g_free (output);

  mkv_plugin.type = OGMRIP_TYPE_MATROSKA;
  mkv_plugin.formats = formats;

  return &mkv_plugin;
}

