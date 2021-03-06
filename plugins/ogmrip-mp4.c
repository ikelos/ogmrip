/* OGMRipMp4 - An MP4 plugin for OGMRip
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

#include "ogmrip-mp4.h"

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

  guint format;
  gboolean hint;
};

struct _OGMRipMp4Class
{
  OGMRipContainerClass parent_class;
};

enum
{
  PROP_0,
  PROP_FORMAT,
  PROP_HINT
};

enum
{
  FORMAT_STANDARD,
  FORMAT_ISMA,
  FORMAT_IPOD,
  FORMAT_PSP
};

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
        iso639_2 = ogmrip_language_to_iso639_2 (language);
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
        iso639_2 = ogmrip_language_to_iso639_2 (language);
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

static OGMJobTask *
ogmrip_mp4_create_command (OGMRipContainer *container, const gchar *input)
{
  OGMRipMp4 *mp4 = OGMRIP_MP4 (container);

  OGMJobTask *task;
  GPtrArray *argv;
  const gchar *label, *fmt = NULL;
  gchar fps[8];

  switch (ogmrip_stream_get_format (OGMRIP_STREAM (mp4->video)))
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

  argv = g_ptr_array_new_full (20, g_free);
  g_ptr_array_add (argv, g_strdup (MP4BOX));

  switch (mp4->format)
  {
    case FORMAT_IPOD:
      g_ptr_array_add (argv, g_strdup ("-ipod"));
      break;
    case FORMAT_PSP:
      break;
    case FORMAT_ISMA:
      if (mp4->naudio <= 1 && mp4->nsubp < 1)
        g_ptr_array_add (argv, g_strdup ("-isma"));
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  if (mp4->hint)
    g_ptr_array_add (argv, g_strdup ("-hint"));

  g_ptr_array_add (argv, g_strdup ("-nodrop"));
  g_ptr_array_add (argv, g_strdup ("-new"));

  g_ptr_array_add (argv, g_strdup ("-brand"));
  g_ptr_array_add (argv, g_strdup ("mp42"));

  g_ptr_array_add (argv, g_strdup ("-tmp"));
  g_ptr_array_add (argv, g_strdup (ogmrip_fs_get_tmp_dir ()));

  label = ogmrip_container_get_label (container);
  if (label)
  {
    g_ptr_array_add (argv, g_strdup ("-itags"));
    g_ptr_array_add (argv, g_strdup_printf ("name=%s", label));
  }

  if (fmt)
  {
    guint num, denom;

    if (!input)
      input = ogmrip_file_get_path (mp4->video);

    ogmrip_video_stream_get_framerate (OGMRIP_VIDEO_STREAM (mp4->video), &num, &denom);
    g_ascii_formatd (fps, 8, "%.3f", num / (gdouble) denom);

    g_ptr_array_add (argv, g_strdup ("-add"));
    g_ptr_array_add (argv, g_strdup_printf ("%s:fmt=%s:fps=%s#video", input, fmt, fps));
  }

  ogmrip_container_foreach_file (container, 
      (OGMRipContainerFunc) ogmrip_mp4_append_file, argv);

  g_ptr_array_add (argv, g_file_get_path (ogmrip_container_get_output (container)));

  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  ogmjob_spawn_set_watch (OGMJOB_SPAWN (task), OGMJOB_STREAM_OUTPUT,
      (OGMJobWatch) ogmrip_mp4_create_watch, mp4, NULL);

  return task;
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

static OGMJobTask *
ogmrip_mp4_split_command (OGMRipContainer *mp4)
{
  OGMJobTask *task;
  GPtrArray *argv;
  guint tsize;

  argv = g_ptr_array_new_full (20, g_free);
  g_ptr_array_add (argv, g_strdup (MP4BOX));

  g_ptr_array_add (argv, g_strdup ("-tmp"));
  g_ptr_array_add (argv, g_strdup (ogmrip_fs_get_tmp_dir ()));

  ogmrip_container_get_split (OGMRIP_CONTAINER (mp4), NULL, &tsize);
  g_ptr_array_add (argv, g_strdup ("-splits"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", tsize));

  g_ptr_array_add (argv, g_file_get_path (ogmrip_container_get_output (mp4)));

  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  ogmjob_spawn_set_watch (OGMJOB_SPAWN (task), OGMJOB_STREAM_OUTPUT,
      (OGMJobWatch) ogmrip_mp4_split_watch, mp4, NULL);

  return task;
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

static void
ogmrip_mp4_configure (OGMRipConfigurable *configurable, OGMRipProfile *profile)
{
  GSettings *settings;

  settings = ogmrip_profile_get_child (profile, "mp4");
  if (settings)
  {
    g_settings_bind (settings, "format", configurable, OGMRIP_MP4_PROP_FORMAT,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "hint", configurable, OGMRIP_MP4_PROP_HINT,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_object_unref (settings);
  }
}

static void
ogmrip_configurable_iface_init (OGMRipConfigurableInterface *iface)
{
  iface->configure = ogmrip_mp4_configure;
}

G_DEFINE_TYPE_EXTENDED (OGMRipMp4, ogmrip_mp4, OGMRIP_TYPE_CONTAINER, 0,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_CONFIGURABLE, ogmrip_configurable_iface_init))

static void
ogmrip_mp4_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipMp4 *mp4 = OGMRIP_MP4 (gobject);

  switch (property_id) 
  {
    case PROP_FORMAT:
      g_value_set_uint (value, mp4->format);
      break;
    case PROP_HINT:
      g_value_set_boolean (value, mp4->hint);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_mp4_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipMp4 *mp4 = OGMRIP_MP4 (gobject);

  switch (property_id) 
  {
    case PROP_FORMAT:
      mp4->format = g_value_get_uint (value);
      break;
    case PROP_HINT:
      mp4->hint = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static gboolean
ogmrip_mp4_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMRipMp4 *mp4 = OGMRIP_MP4 (task);
  OGMJobTask *queue, *child;

  gboolean result = FALSE;
  gchar *filename;

  filename = ogmrip_fs_mktemp ("video.XXXXXX", error);
  if (!filename)
    return FALSE;

  ogmrip_container_get_split (OGMRIP_CONTAINER (task), &mp4->nsplits, NULL);

  mp4->naudio = mp4->nsubp = mp4->nvobsub = 0;
  ogmrip_container_foreach_file (OGMRIP_CONTAINER (task),
      (OGMRipContainerFunc) ogmrip_mp4_get_info, NULL);

  queue = ogmjob_queue_new ();
  ogmjob_container_add (OGMJOB_CONTAINER (task), queue);
  g_object_unref (queue);

  child = ogmrip_video_extractor_new (OGMRIP_CONTAINER (task), mp4->video, filename);
  ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
  g_object_unref (child);

  mp4->old_percent = 0;
  mp4->nstreams = 2 + mp4->naudio + mp4->nvobsub;
  mp4->streams = 0;

  child = ogmrip_mp4_create_command (OGMRIP_CONTAINER (task), filename);
  ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
  g_object_unref (child);

  if (mp4->nsplits > 1)
  {
    mp4->split_percent = 0;
    mp4->splits = 0;

    child = ogmrip_mp4_split_command (OGMRIP_CONTAINER (task));
    ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
    g_object_unref (child);
  }

  result = OGMJOB_TASK_CLASS (ogmrip_mp4_parent_class)->run (task, cancellable, error);

  ogmjob_container_remove (OGMJOB_CONTAINER (task), queue);

  g_unlink (filename);
  g_free (filename);

  if (mp4->nsplits > 1)
    g_file_delete (ogmrip_container_get_output (OGMRIP_CONTAINER (task)), NULL, NULL);

  return result;
}

static void
ogmrip_mp4_class_init (OGMRipMp4Class *klass)
{
  GObjectClass *gobject_class;
  OGMJobTaskClass *task_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_mp4_get_property;
  gobject_class->set_property = ogmrip_mp4_set_property;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_mp4_run;

  g_object_class_install_property (gobject_class, PROP_HINT,
      g_param_spec_boolean (OGMRIP_MP4_PROP_HINT, "hint property", "Set hint",
        OGMRIP_MP4_DEFAULT_HINT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FORMAT,
      g_param_spec_uint (OGMRIP_MP4_PROP_FORMAT, "Format property", "Set format",
        0, 3, OGMRIP_MP4_DEFAULT_FORMAT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_mp4_init (OGMRipMp4 *mp4)
{
  mp4->format = OGMRIP_MP4_DEFAULT_FORMAT;
  mp4->hint = OGMRIP_MP4_DEFAULT_HINT;
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

static gboolean
ogmrip_mp4_get_version (const gchar *str, gint *major_version, gint *minor_version, gint *micro_version)
{
  gchar *end;

  if (!g_str_has_prefix (str, "MP4Box - GPAC version "))
    return FALSE;

  errno = 0;

  *major_version = strtoul (str + 22, &end, 10);
  if (errno || *end != '.')
    return FALSE;

  *minor_version = strtoul (end + 1, NULL, 10);
  if (errno || *end != '.')
    return FALSE;

  *micro_version = strtoul (end + 1, NULL, 10);
  if (errno)
    return FALSE;

  return TRUE;
}

void
ogmrip_module_load (OGMRipModule *module)
{
  gchar *output, *error;
  gint major_version = 0, minor_version = 0, micro_version = 0;

  if (!g_spawn_command_line_sync (MP4BOX " -version", &output, &error, NULL, NULL))
  {
    g_warning (_("MP4Box is missing"));
    return;
  }

  if (ogmrip_mp4_get_version (output, &major_version, &minor_version, &micro_version) ||
      ogmrip_mp4_get_version (error,  &major_version, &minor_version, &micro_version))
  {
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
  }

  g_free (output);
  g_free (error);

  ogmrip_register_container (OGMRIP_TYPE_MP4,
      "mp4", _("Mpeg-4 Media (MP4)"), formats,
      "schema-id", "org.ogmrip.mp4", NULL);
}

