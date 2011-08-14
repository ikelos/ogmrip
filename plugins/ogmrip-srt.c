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

#include "ogmrip-fs.h"
#include "ogmrip-mplayer.h"
#include "ogmrip-plugin.h"
#include "ogmrip-subp-codec.h"
#include "ogmrip-version.h"

#include "ogmjob-queue.h"
#include "ogmjob-exec.h"

#include <unistd.h>
#include <string.h>
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

GType ogmrip_srt_get_type (void);
static gint ogmrip_srt_run (OGMJobSpawn *spawn);

static gboolean use_gocr      = FALSE;
static gboolean use_ocrad     = FALSE;
static gboolean use_tesseract = FALSE;

static gdouble
ogmrip_subp2pgm_watch (OGMJobExec *exec, const gchar *buffer, OGMRipSrt *srt)
{
  guint files;

  if (sscanf (buffer, "%u files generated", &files) == 1)
  {
    srt->files = files;
    srt->index = 0;
  }

  return -1.0;
}

static gdouble
ogmrip_gocr_watch (OGMJobExec *exec, const gchar *buffer, OGMRipSrt *srt)
{
  if (strncmp (buffer, "Elapsed time:", 13) == 0)
  {
    srt->index ++;

    return 0.98 + 0.02 * (srt->index + 1) / (gdouble) srt->files;
  }

  return -1.0;
}

static gdouble
ogmrip_ocrad_watch (OGMJobExec *exec, const gchar *buffer, OGMRipSrt *srt)
{
  if (strncmp (buffer, "number of text blocks =", 23) == 0)
  {
    srt->index ++;

    return 0.98 + 0.02 * (srt->index + 1) / (gdouble) srt->files;
  }

  return -1.0;
}

static gdouble
ogmrip_tesseract_watch (OGMJobExec *exec, const gchar *buffer, OGMRipSrt *srt)
{
  if (strncmp (buffer, "Tesseract Open Source OCR Engine", 32) == 0)
  {
    srt->index ++;

    return 0.98 + 0.02 * (srt->index + 1) / (gdouble) srt->files;
  }

  return -1.0;
}

static gchar **
ogmrip_subp2pgm_command (OGMRipSubpCodec *subp, const gchar *input)
{
  GPtrArray *argv;

  argv = g_ptr_array_new ();

  if (use_tesseract)
    g_ptr_array_add (argv, g_strdup ("subp2tiff"));
  else
    g_ptr_array_add (argv, g_strdup ("subp2pgm"));

  if (ogmrip_subp_codec_get_forced_only (subp))
    g_ptr_array_add (argv, g_strdup ("--forced"));

  g_ptr_array_add (argv, g_strdup ("--normalize"));

  g_ptr_array_add (argv, g_strdup (input));
  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gchar **
ogmrip_gocr_command (OGMRipSubpCodec *subp, const gchar *input)
{
  GPtrArray *argv;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("gocr"));
  g_ptr_array_add (argv, g_strdup ("-v"));
  g_ptr_array_add (argv, g_strdup ("1"));
  g_ptr_array_add (argv, g_strdup ("-f"));

  switch (ogmrip_subp_codec_get_character_set (subp))
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

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gchar **
ogmrip_ocrad_command (OGMRipSubpCodec *subp, const gchar *input)
{
  GPtrArray *argv;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("ocrad"));
  g_ptr_array_add (argv, g_strdup ("-v"));
  g_ptr_array_add (argv, g_strdup ("-f"));
  g_ptr_array_add (argv, g_strdup ("-F"));

  switch (ogmrip_subp_codec_get_character_set (subp))
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

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gchar **
ogmrip_tesseract_command (OGMRipSubpCodec *subp, const gchar *input, gboolean lang)
{
  GPtrArray *argv;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("tesseract"));
  g_ptr_array_add (argv, g_strdup (input));
  g_ptr_array_add (argv, g_strdup (input));

  if (lang && OGMRIP_SRT (subp)->is_valid_lang)
  {
    OGMDvdStream *stream;
    const gchar *language;

    stream = ogmrip_codec_get_input (OGMRIP_CODEC (subp));
    language = ogmdvd_get_language_iso639_2 (ogmdvd_subp_stream_get_language (OGMDVD_SUBP_STREAM (stream)));

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

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gchar **
ogmrip_srt_command (OGMRipSubpCodec *subp, const gchar *input, const gchar *output)
{
  GPtrArray *argv;

  if (!output)
    output = ogmrip_codec_get_output (OGMRIP_CODEC (subp));

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("subptools"));
  g_ptr_array_add (argv, g_strdup ("-s"));

  g_ptr_array_add (argv, g_strdup ("-t"));
  g_ptr_array_add (argv, g_strdup ("srt"));

  switch (ogmrip_subp_codec_get_newline_style (OGMRIP_SUBP_CODEC (subp)))
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

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gchar **
ogmrip_vobsub_command (OGMRipSubpCodec *subp, const gchar *input, const gchar *output)
{
  GPtrArray *argv;

  argv = ogmrip_mencoder_vobsub_command (subp, output);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static OGMJobSpawn *
ogmrip_srt_ocr (OGMJobSpawn *spawn, const gchar *filename, gboolean lang)
{
  OGMJobSpawn *child;
  gchar **argv;

  if (use_tesseract)
    argv = ogmrip_tesseract_command (OGMRIP_SUBP_CODEC (spawn), filename, lang);
  else if (use_ocrad)
    argv = ogmrip_ocrad_command (OGMRIP_SUBP_CODEC (spawn), filename);
  else
    argv = ogmrip_gocr_command (OGMRIP_SUBP_CODEC (spawn), filename);

  if (!argv)
    return NULL;

  child = ogmjob_exec_newv (argv);

  if (use_tesseract)
    ogmjob_exec_add_watch_full (OGMJOB_EXEC (child),
        (OGMJobWatch) ogmrip_tesseract_watch, spawn, FALSE, TRUE, TRUE);
  else if (use_ocrad)
    ogmjob_exec_add_watch_full (OGMJOB_EXEC (child),
        (OGMJobWatch) ogmrip_ocrad_watch, spawn, FALSE, TRUE, TRUE);
  else
    ogmjob_exec_add_watch_full (OGMJOB_EXEC (child),
        (OGMJobWatch) ogmrip_gocr_watch, spawn, FALSE, TRUE, TRUE);

  return child;
}


G_DEFINE_TYPE (OGMRipSrt, ogmrip_srt, OGMRIP_TYPE_SUBP_CODEC)

static void
ogmrip_srt_class_init (OGMRipSrtClass *klass)
{
  OGMJobSpawnClass *spawn_class;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);

  spawn_class->run = ogmrip_srt_run;
}

static void
ogmrip_srt_init (OGMRipSrt *srt)
{
  srt->is_valid_lang = TRUE;
}

static gint
ogmrip_srt_run (OGMJobSpawn *spawn)
{
  OGMJobSpawn *child;
  GPatternSpec *pspec;
  GDir *dir;

  gboolean have_sub_files = FALSE;
  gchar **argv, *pattern, *str, *tmp_file, *xml_file;
  const gchar *name;
  gint result, fd;

  result = OGMJOB_RESULT_ERROR;

  fd = ogmrip_fs_open_tmp ("sub.XXXXXX", &tmp_file, NULL);
  if (fd < 0)
    return OGMJOB_RESULT_ERROR;
  g_unlink (tmp_file);
  close (fd);

  xml_file = g_strconcat (tmp_file, ".xml", NULL);

  argv = ogmrip_vobsub_command (OGMRIP_SUBP_CODEC (spawn), NULL, tmp_file);
  if (argv)
  {
    child = ogmjob_exec_newv (argv);
    ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_mencoder_vobsub_watch, spawn, TRUE, FALSE, FALSE);
    ogmjob_container_add (OGMJOB_CONTAINER (spawn), child);
    g_object_unref (child);

    result = OGMJOB_SPAWN_CLASS (ogmrip_srt_parent_class)->run (spawn);

    ogmjob_container_remove (OGMJOB_CONTAINER (spawn), child);
  }

  if (result == OGMJOB_RESULT_SUCCESS)
  {
    result = OGMJOB_RESULT_ERROR;

    argv = ogmrip_subp2pgm_command (OGMRIP_SUBP_CODEC (spawn), tmp_file);
    if (argv)
    {
      child = ogmjob_exec_newv (argv);
      ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_subp2pgm_watch, spawn, TRUE, FALSE, FALSE);
      result = ogmjob_spawn_run (child, NULL);
      g_object_unref (child);
    }
  }

  if (result == OGMJOB_RESULT_SUCCESS)
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
          if ((child = ogmrip_srt_ocr (spawn, str, TRUE)))
          {
            result = ogmjob_spawn_run (child, NULL);
            g_object_unref (child);

            if (result != OGMJOB_RESULT_SUCCESS)
            {
              if (!use_tesseract || !OGMRIP_SRT (spawn)->is_valid_lang)
                break;

              OGMRIP_SRT (spawn)->is_valid_lang = FALSE;

              if ((child = ogmrip_srt_ocr (spawn, str, FALSE)))
              {
                result = ogmjob_spawn_run (child, NULL);
                g_object_unref (child);

                if (result != OGMJOB_RESULT_SUCCESS)
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

  if (result == OGMJOB_RESULT_SUCCESS)
  {
    if (have_sub_files && g_file_test (xml_file, G_FILE_TEST_EXISTS))
    {
      result = OGMJOB_RESULT_ERROR;

      argv = ogmrip_srt_command (OGMRIP_SUBP_CODEC (spawn), xml_file, NULL);
      if (argv)
      {
        child = ogmjob_exec_newv (argv);
        result = ogmjob_spawn_run (child, NULL);
        g_object_unref (child);
      }
    }
  }

  if (g_file_test (xml_file, G_FILE_TEST_EXISTS))
    g_unlink (xml_file);
  g_free (xml_file);

  xml_file = g_strconcat (tmp_file, ".idx", NULL);
  if (g_file_test (xml_file, G_FILE_TEST_EXISTS))
    g_unlink (xml_file);
  g_free (xml_file);

  xml_file = g_strconcat (tmp_file, ".sub", NULL);
  if (g_file_test (xml_file, G_FILE_TEST_EXISTS))
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

static OGMRipSubpPlugin srt_plugin =
{
  NULL,
  G_TYPE_NONE,
  "srt",
  N_("SRT text"),
  OGMRIP_FORMAT_SRT,
  TRUE
};

OGMRipSubpPlugin *
ogmrip_init_plugin (GError **error)
{
#if defined(HAVE_GOCR_SUPPORT) || defined(HAVE_OCRAD_SUPPORT) || defined(HAVE_TESSERACT_SUPPORT)
  gchar *fullname;
#endif

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (!ogmrip_check_mencoder ())
  {
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("MEncoder is missing"));
    return NULL;
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
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("GOCR, Ocrad and Tesseract are missing"));
    return NULL;
  }

  srt_plugin.type = OGMRIP_TYPE_SRT;

  return &srt_plugin;
}

