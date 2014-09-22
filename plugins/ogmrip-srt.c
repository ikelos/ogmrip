/* OGMRipSrt - An SRT plugin for OGMRip
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
#include <ogmrip-mplayer.h>
#include <ogmrip-module.h>

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_SRT          (ogmrip_srt_get_type ())
#define OGMRIP_SRT(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_SRT, OGMRipSrt))
#define OGMRIP_SRT_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_SRT, OGMRipSrtClass))
#define OGMRIP_IS_SRT(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_SRT))
#define OGMRIP_IS_SRT_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_SRT))

typedef struct _OGMRipSrt      OGMRipSrt;
typedef struct _OGMRipSrtClass OGMRipSrtClass;

struct _OGMRipSrt
{
  OGMRipSubpCodec parent_instance;

  guint files;
  guint index;

  gboolean is_valid_lang;
};

struct _OGMRipSrtClass
{
  OGMRipSubpCodecClass parent_class;
};

GType           ogmrip_srt_get_type (void);
static gboolean ogmrip_srt_run      (OGMJobTask   *task,
                                     GCancellable *cancellable,
                                     GError       **error);

static gboolean use_gocr      = FALSE;
static gboolean use_ocrad     = FALSE;
static gboolean use_tesseract = FALSE;

static gdouble
ogmrip_subp2pgm_watch (OGMJobSpawn *spawn, const gchar *buffer, OGMRipSrt *srt)
{
  guint files;

  if (sscanf (buffer, "%u files generated", &files) == 1)
  {
    srt->files = files;
    srt->index = 0;
  }

  return -1.0;
}

static gboolean
ogmrip_gocr_watch (OGMJobTask *task, const gchar *buffer, OGMRipSrt *srt, GError **error)
{
  if (strncmp (buffer, "Elapsed time:", 13) == 0)
  {
    srt->index ++;

    ogmjob_task_set_progress (OGMJOB_TASK (srt),
        0.98 + 0.02 * (srt->index + 1) / (gdouble) srt->files);
  }

  return TRUE;
}

static gboolean
ogmrip_ocrad_watch (OGMJobTask *task, const gchar *buffer, OGMRipSrt *srt, GError **error)
{
  if (strncmp (buffer, "number of text blocks =", 23) == 0)
  {
    srt->index ++;

    ogmjob_task_set_progress (OGMJOB_TASK (srt),
        0.98 + 0.02 * (srt->index + 1) / (gdouble) srt->files);
  }

  return TRUE;
}

static gboolean
ogmrip_tesseract_watch (OGMJobTask *task, const gchar *buffer, OGMRipSrt *srt, GError **error)
{
  if (strncmp (buffer, "Tesseract Open Source OCR Engine", 32) == 0)
  {
    srt->index ++;

    ogmjob_task_set_progress (OGMJOB_TASK (srt),
        0.98 + 0.02 * (srt->index + 1) / (gdouble) srt->files);
  }

  return TRUE;
}

static OGMJobTask *
ogmrip_subp2pgm_command (OGMRipSubpCodec *subp, const gchar *input)
{
  OGMJobTask *task;
  GPtrArray *argv;

  argv = g_ptr_array_new_full (20, g_free);

  if (use_tesseract)
    g_ptr_array_add (argv, g_strdup ("subp2tiff"));
  else
    g_ptr_array_add (argv, g_strdup ("subp2pgm"));

  if (ogmrip_subp_codec_get_forced_only (subp))
    g_ptr_array_add (argv, g_strdup ("--forced"));

  g_ptr_array_add (argv, g_strdup ("--normalize"));

  g_ptr_array_add (argv, g_strdup (input));
  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  ogmjob_spawn_set_watch (OGMJOB_SPAWN (task), OGMJOB_STREAM_OUTPUT,
      (OGMJobWatch) ogmrip_subp2pgm_watch, subp, NULL);

  return task;
}

static OGMJobTask *
ogmrip_gocr_command (OGMRipSubpCodec *subp, const gchar *input)
{
  OGMJobTask *task;
  GPtrArray *argv;

  argv = g_ptr_array_new_full (20, g_free);
  g_ptr_array_add (argv, g_strdup ("gocr"));
  g_ptr_array_add (argv, g_strdup ("-v"));
  g_ptr_array_add (argv, g_strdup ("1"));
  g_ptr_array_add (argv, g_strdup ("-f"));

  switch (ogmrip_subp_codec_get_charset (subp))
  {
    case OGMRIP_CHARSET_UTF8:
      g_ptr_array_add (argv, g_strdup ("UTF8"));
      break;
    case OGMRIP_CHARSET_ISO8859_1:
      g_ptr_array_add (argv, g_strdup ("ISO8859_1"));
      break;
    case OGMRIP_CHARSET_ASCII:
      g_ptr_array_add (argv, g_strdup ("ASCII"));
      break;
  }

  g_ptr_array_add (argv, g_strdup ("-m"));
  g_ptr_array_add (argv, g_strdup ("4"));
  g_ptr_array_add (argv, g_strdup ("-m"));
  g_ptr_array_add (argv, g_strdup ("64"));
  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strconcat (input, ".txt", NULL));
  g_ptr_array_add (argv, g_strdup (input));
  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  ogmjob_spawn_set_watch (OGMJOB_SPAWN (task), OGMJOB_STREAM_ERROR,
      (OGMJobWatch) ogmrip_gocr_watch, subp, NULL);

  return task;
}

static OGMJobTask *
ogmrip_ocrad_command (OGMRipSubpCodec *subp, const gchar *input)
{
  OGMJobTask *task;
  GPtrArray *argv;

  argv = g_ptr_array_new_full (20, g_free);
  g_ptr_array_add (argv, g_strdup ("ocrad"));
  g_ptr_array_add (argv, g_strdup ("-v"));
  g_ptr_array_add (argv, g_strdup ("-f"));
  g_ptr_array_add (argv, g_strdup ("-F"));

  switch (ogmrip_subp_codec_get_charset (subp))
  {
    case OGMRIP_CHARSET_UTF8:
      g_ptr_array_add (argv, g_strdup ("utf8"));
      break;
    case OGMRIP_CHARSET_ISO8859_1:
    case OGMRIP_CHARSET_ASCII:
      g_ptr_array_add (argv, g_strdup ("byte"));
      break;
  }

  g_ptr_array_add (argv, g_strdup ("-l"));
  g_ptr_array_add (argv, g_strdup ("0"));
  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strconcat (input, ".txt", NULL));
  g_ptr_array_add (argv, g_strdup (input));
  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  ogmjob_spawn_set_watch (OGMJOB_SPAWN (task), OGMJOB_STREAM_ERROR,
      (OGMJobWatch) ogmrip_ocrad_watch, subp, NULL);

  return task;
}

static OGMJobTask *
ogmrip_tesseract_command (OGMRipSubpCodec *subp, const gchar *input, gboolean lang)
{
  OGMJobTask *task;
  GPtrArray *argv;

  argv = g_ptr_array_new_full (20, g_free);
  g_ptr_array_add (argv, g_strdup ("tesseract"));
  g_ptr_array_add (argv, g_strdup (input));
  g_ptr_array_add (argv, g_strdup (input));

  if (lang && OGMRIP_SRT (subp)->is_valid_lang)
  {
    OGMRipStream *stream;
    const gchar *language;

    stream = ogmrip_codec_get_input (OGMRIP_CODEC (subp));
    language = ogmrip_language_to_iso639_2 (ogmrip_subp_stream_get_language (OGMRIP_SUBP_STREAM (stream)));

    if (g_str_equal (language, "und"))
      OGMRIP_SRT (subp)->is_valid_lang = FALSE;
    else
    {
      if (g_str_equal (language, "fre"))
        language = "fra";
      else if (g_str_equal (language, "ger"))
        language = "deu";

      g_ptr_array_add (argv, g_strdup ("-l"));
      g_ptr_array_add (argv, g_strdup (language));
    }
  }

  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  ogmjob_spawn_set_watch (OGMJOB_SPAWN (task), OGMJOB_STREAM_ERROR,
      (OGMJobWatch) ogmrip_tesseract_watch, subp, NULL);

  return task;
}

static OGMJobTask *
ogmrip_srt_command (OGMRipSubpCodec *subp, const gchar *input)
{
  OGMJobTask *task;

  GPtrArray *argv;
  const gchar *output;

  output = ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (subp)));

  argv = g_ptr_array_new_full (20, g_free);
  g_ptr_array_add (argv, g_strdup ("subptools"));
  g_ptr_array_add (argv, g_strdup ("-s"));

  g_ptr_array_add (argv, g_strdup ("-t"));
  g_ptr_array_add (argv, g_strdup ("srt"));

  switch (ogmrip_subp_codec_get_newline (OGMRIP_SUBP_CODEC (subp)))
  {
    case OGMRIP_NEWLINE_LF:
      g_ptr_array_add (argv, g_strdup ("-n"));
      g_ptr_array_add (argv, g_strdup ("lf"));
      break;
    case OGMRIP_NEWLINE_CR_LF:
      g_ptr_array_add (argv, g_strdup ("-n"));
      g_ptr_array_add (argv, g_strdup ("cr+lf"));
      break;
    case OGMRIP_NEWLINE_CR:
      g_ptr_array_add (argv, g_strdup ("-n"));
      g_ptr_array_add (argv, g_strdup ("cr"));
      break;
    default:
      break;
  }

  g_ptr_array_add (argv, g_strdup ("-i"));
  g_ptr_array_add (argv, g_strdup (input));
  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (output));
  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  return task;
}

static OGMJobTask *
ogmrip_srt_ocr (OGMJobTask *task, const gchar *filename, gboolean lang)
{
  OGMJobTask *child;

  if (use_tesseract)
    child = ogmrip_tesseract_command (OGMRIP_SUBP_CODEC (task), filename, lang);
  else if (use_ocrad)
    child = ogmrip_ocrad_command (OGMRIP_SUBP_CODEC (task), filename);
  else
    child = ogmrip_gocr_command (OGMRIP_SUBP_CODEC (task), filename);

  return child;
}


G_DEFINE_TYPE (OGMRipSrt, ogmrip_srt, OGMRIP_TYPE_SUBP_CODEC)

static void
ogmrip_srt_class_init (OGMRipSrtClass *klass)
{
  OGMJobTaskClass *task_class;

  task_class = OGMJOB_TASK_CLASS (klass);

  task_class->run = ogmrip_srt_run;
}

static void
ogmrip_srt_init (OGMRipSrt *srt)
{
  srt->is_valid_lang = TRUE;
}

static gboolean
ogmrip_srt_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobTask *child;
  GPatternSpec *pspec;
  GDir *dir;

  gboolean result = FALSE, have_sub_files = FALSE;
  gchar *pattern, *str, *tmp_file, *xml_file;
  const gchar *name;
  gint fd;

  fd = ogmrip_fs_open_tmp ("sub.XXXXXX", &tmp_file, error);
  if (fd < 0)
    return FALSE;
  g_unlink (tmp_file);
  close (fd);

  xml_file = g_strconcat (tmp_file, ".xml", NULL);

  child = ogmrip_subp_encoder_new (OGMRIP_SUBP_CODEC (task), OGMRIP_ENCODER_VOBSUB, NULL, tmp_file);
  ogmjob_container_add (OGMJOB_CONTAINER (task), child);
  g_object_unref (child);

  result = OGMJOB_TASK_CLASS (ogmrip_srt_parent_class)->run (task, cancellable, error);

  ogmjob_container_remove (OGMJOB_CONTAINER (task), child);

  if (result)
  {
    child = ogmrip_subp2pgm_command (OGMRIP_SUBP_CODEC (task), tmp_file);
    result = ogmjob_task_run (child, cancellable, error);
    g_object_unref (child);
  }

  if (result)
  {
    dir = g_dir_open (ogmrip_fs_get_tmp_dir (), 0, NULL);
    if (dir)
    {
      gchar *basename;

      basename = g_path_get_basename (tmp_file);
      if (use_tesseract)
        pattern = g_strconcat (basename, "*.tif", NULL);
      else
        pattern = g_strconcat (basename, "*.pgm", NULL);
      pspec = g_pattern_spec_new (pattern);
      g_free (basename);
      g_free (pattern);

      while ((name = g_dir_read_name (dir)))
      {
        if (g_pattern_match (pspec, strlen (name), name, NULL))
        {
          str = g_build_filename (ogmrip_fs_get_tmp_dir (), name, NULL);
          if ((child = ogmrip_srt_ocr (task, str, TRUE)))
          {
            result = ogmjob_task_run (child, cancellable, error);
            g_object_unref (child);

            if (!result)
            {
              if (!use_tesseract || !OGMRIP_SRT (task)->is_valid_lang)
                break;

              OGMRIP_SRT (task)->is_valid_lang = FALSE;

              if ((child = ogmrip_srt_ocr (task, str, FALSE)))
              {
                result = ogmjob_task_run (child, cancellable, error);
                g_object_unref (child);

                if (!result)
                  break;
              }
            }

            have_sub_files = TRUE;
          }
          g_unlink (str);
          g_free (str);
        }
      }

      g_pattern_spec_free (pspec);
      g_dir_close (dir);
    }
  }

  if (result)
  {
    if (have_sub_files && g_file_test (xml_file, G_FILE_TEST_EXISTS))
    {
      child = ogmrip_srt_command (OGMRIP_SUBP_CODEC (task), xml_file);
      result = ogmjob_task_run (child, cancellable, error);
      g_object_unref (child);
    }
  }

  g_unlink (xml_file);
  g_free (xml_file);

  xml_file = g_strconcat (tmp_file, ".idx", NULL);
  g_unlink (xml_file);
  g_free (xml_file);

  xml_file = g_strconcat (tmp_file, ".sub", NULL);
  g_unlink (xml_file);
  g_free (xml_file);

  dir = g_dir_open (ogmrip_fs_get_tmp_dir (), 0, NULL);
  if (dir)
  {
    gchar *basename;

    basename = g_path_get_basename (tmp_file);
    if (use_tesseract)
      pattern = g_strconcat (basename, "*.tif.txt", NULL);
    else
      pattern = g_strconcat (basename, "*.pgm.txt", NULL);
    pspec = g_pattern_spec_new (pattern);
    g_free (basename);
    g_free (pattern);

    while ((name = g_dir_read_name (dir)))
    {
      if (g_pattern_match (pspec, strlen (name), name, NULL))
      {
        str = g_build_filename (ogmrip_fs_get_tmp_dir (), name, NULL);
        g_unlink (str);
        g_free (str);
      }
    }

    g_pattern_spec_free (pspec);
    g_dir_close (dir);
  }

  g_free (tmp_file);

  return result;
}

void
ogmrip_module_load (OGMRipModule *module)
{
#if defined(HAVE_GOCR_SUPPORT) || defined(HAVE_OCRAD_SUPPORT) || defined(HAVE_TESSERACT_SUPPORT)
  gchar *fullname;
#endif

  if (!ogmrip_check_mencoder ())
  {
    g_warning (_("MEncoder is missing"));
    return;
  }

#ifdef HAVE_TESSERACT_SUPPORT
  fullname = g_find_program_in_path ("tesseract");
  use_tesseract = fullname != NULL;
  g_free (fullname);

  if (use_tesseract)
  {
    fullname = g_find_program_in_path ("subp2tiff");
    use_tesseract = fullname != NULL;
    g_free (fullname);
  }
#endif

#ifdef HAVE_GOCR_SUPPORT
  if (!use_tesseract)
  {
    fullname = g_find_program_in_path ("gocr");
    use_gocr = fullname != NULL;
    g_free (fullname);
  }
#endif

#ifdef HAVE_OCRAD_SUPPORT
  if (!use_gocr && !use_tesseract)
  {
    fullname = g_find_program_in_path ("ocrad");
    use_ocrad = fullname != NULL;
    g_free (fullname);
  }
#endif

  if (!use_gocr && !use_ocrad && !use_tesseract)
  {
    g_warning (_("GOCR, Ocrad and Tesseract are missing"));
    return;
  }

  ogmrip_register_codec (OGMRIP_TYPE_SRT,
      "srt", _("SRT text"), OGMRIP_FORMAT_SRT, NULL);
}

