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

/**
 * SECTION:ogmrip-file
 * @title: Files
 * @short_description: Structures to manipulate media files
 * @include: ogmrip-file.h
 */

#include "ogmrip-file.h"
#include "ogmrip-enums.h"
#include "ogmrip-version.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <enca.h>

#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

struct _OGMRipFile
{
  gchar *filename;
  gboolean do_unlink;
  gint format;
  gint type;
  gint lang;
  gint ref;
  gint fd;
};

struct _OGMRipVideoFile
{
  OGMRipFile file;

  gint bitrate;
  gdouble length;
  gint width;
  gint height;
  gdouble fps;
  gdouble aspect;
};

struct _OGMRipAudioFile
{
  OGMRipFile file;

  gint  rate;
  gint  bitrate;
  gint channels;
  gdouble length;
};

struct _OGMRipSubpFile
{
  OGMRipFile file;

  gint charset;
};

typedef struct
{
  const gchar *fourcc;
  guint value;
  guint format;
} OGMRipFileConfig;

static gchar **
ogmrip_backend_identify_command (const gchar *filename, gboolean lavf)
{
  GPtrArray *argv;

  g_return_val_if_fail (filename != NULL, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("mplayer"));
  g_ptr_array_add (argv, g_strdup ("-nolirc"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));

  if (MPLAYER_CHECK_VERSION (1,0,3,0))
  {
    g_ptr_array_add (argv, g_strdup ("-noconfig"));
    g_ptr_array_add (argv, g_strdup ("all"));
  }

  g_ptr_array_add (argv, g_strdup ("-vo"));
  g_ptr_array_add (argv, g_strdup ("null"));
  g_ptr_array_add (argv, g_strdup ("-ao"));
  g_ptr_array_add (argv, g_strdup ("null"));
  g_ptr_array_add (argv, g_strdup ("-frames"));
  g_ptr_array_add (argv, g_strdup ("0"));

  if (lavf)
  {
    g_ptr_array_add (argv, g_strdup ("-demuxer"));
    g_ptr_array_add (argv, g_strdup ("lavf"));
  }

  g_ptr_array_add (argv, g_strdup ("-identify"));
  g_ptr_array_add (argv, g_strdup (filename));
  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gchar **
ogmrip_backend_identify_sub_command (const gchar *filename, gboolean vobsub)
{
  GPtrArray *argv;

  g_return_val_if_fail (filename != NULL, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("mplayer"));
  g_ptr_array_add (argv, g_strdup ("-nolirc"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));

  if (MPLAYER_CHECK_VERSION (1,0,3,0))
  {
    g_ptr_array_add (argv, g_strdup ("-noconfig"));
    g_ptr_array_add (argv, g_strdup ("all"));
  }

  g_ptr_array_add (argv, g_strdup ("-v"));
  g_ptr_array_add (argv, g_strdup ("/dev/zero"));
  g_ptr_array_add (argv, g_strdup ("-rawvideo"));
  g_ptr_array_add (argv, g_strdup ("pal:fps=25"));
  g_ptr_array_add (argv, g_strdup ("-demuxer"));
  g_ptr_array_add (argv, g_strdup ("rawvideo"));
  g_ptr_array_add (argv, g_strdup ("-vc"));
  g_ptr_array_add (argv, g_strdup ("null"));
  g_ptr_array_add (argv, g_strdup ("-vo"));
  g_ptr_array_add (argv, g_strdup ("null"));
  g_ptr_array_add (argv, g_strdup ("-frames"));
  g_ptr_array_add (argv, g_strdup ("0"));
  g_ptr_array_add (argv, g_strdup ("-noframedrop"));

  if (vobsub)
    g_ptr_array_add (argv, g_strdup ("-vobsub"));
  else
    g_ptr_array_add (argv, g_strdup ("-sub"));

  g_ptr_array_add (argv, g_strdup (filename));

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}
/*
static void
enca_set_dwim_options (EncaAnalyser analyser, const gchar *buffer, gsize len)
{
  const double mu = 0.005;
  const double m = 15.0;
  size_t sgnf;

  if (!len)
    sgnf = 1;
  else
    sgnf = ceil ((double) len / (len / m + 1.0 / mu));

  enca_set_significant (enca->analyser, sgnf);
  enca_set_filtering (enca->analyser, sgnf > 2);
}
*/
static gint
enca_analyse_file (EncaAnalyser analyser, const gchar *filename)
{
  FILE *stream;
  char buffer[0x10000];
  size_t len;

  EncaEncoding result;

  stream = fopen (filename, "rb");
  if (!stream)
    return -1;

  len = fread (buffer, 1, 0x10000, stream);
  if (!len && ferror (stream))
  {
    fclose (stream);
    return -1;
  }
  fclose (stream);

  /*
   * get file size
   *
   * if (file_size == len)
   *    enca_set_termination_strictness (enca->analyser, 1);
   *
   * enca_set_dwim_options (analyser, buffer, len);
   */
  
  result = enca_analyse_const (analyser, (const unsigned char *) buffer, len);

  return result.charset;
}

/**
 * OGMRIP_FILE_ERROR:
 *
 * Error domain for file operations. Errors in this domain will be from the
 * #OGMRipFileError enumeration. See #GError for information on error domains.
 */

GQuark
ogmrip_file_error_quark (void)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_static_string ("ogmrip-file-error-quark");

  return quark;
}

static gboolean
ogmrip_file_construct (OGMRipFile *file, const gchar *filename)
{
  file->fd = g_open (filename, O_RDONLY, 0);
  if (file->fd < 0)
    return FALSE;

  file->filename = g_strdup (filename);
  file->ref ++;

  return TRUE;
}

/**
 * ogmrip_file_ref:
 * @file: an #OGMRipFile
 *
 * Increments the reference count of an #OGMRipFile.
 *
 * Returns: @file
 */
OGMRipFile *
ogmrip_file_ref (OGMRipFile *file)
{
  g_return_val_if_fail (file != NULL, NULL);

  file->ref ++;

  return file;
}

/**
 * ogmrip_file_unref:
 * @file: an #OGMRipFile
 *
 * Decrements the reference count of an #OGMRipFile.
 */
void
ogmrip_file_unref (OGMRipFile *file)
{
  g_return_if_fail (file != NULL);

  if (file->ref > 0)
  {
    file->ref --;

    if (file->ref == 0)
    {
      close (file->fd);

      if (file->do_unlink)
        g_unlink (file->filename);

      g_free (file->filename);
      g_free (file);
    }
  }
}

/**
 * ogmrip_file_set_unlink_on_unref:
 * @file: An #OGMRipFile
 * @do_unlink: Whether the file will be closed on the final unref of @file
 *
 * Sets whether the file will be unlinked when @file receives its final unref
 * and is destroyed.
 */
void
ogmrip_file_set_unlink_on_unref (OGMRipFile *file, gboolean do_unlink)
{
  g_return_if_fail (file != NULL);

  file->do_unlink = do_unlink;
}

/**
 * ogmrip_file_get_unlink_on_unref:
 * @file: An #OGMRipFile
 *
 * Gets whether the file will be unlinked when @file receives its final unref
 * and is destroyed.
 *
 * Returns: Whether the channel will be closed on the final unref
 */
gboolean
ogmrip_file_get_unlink_on_unref (OGMRipFile *file)
{
  g_return_val_if_fail (file != NULL, FALSE);

  return file->do_unlink;
}

/**
 * ogmrip_file_get_type:
 * @file: An #OGMRipFile
 *
 * Gets the type of a file.
 *
 * Returns: An #OGMRipFileType, or -1
 */
gint
ogmrip_file_get_type (OGMRipFile *file)
{
  g_return_val_if_fail (file != NULL, -1);

  return file->type;
}

/**
 * ogmrip_file_get_format:
 * @file: An #OGMRipFile
 *
 * Gets the format of a file.
 *
 * Returns: An #OGMRipFormatType, or -1
 */
gint
ogmrip_file_get_format (OGMRipFile *file)
{
  g_return_val_if_fail (file != NULL, -1);

  return file->format;
}

/**
 * ogmrip_file_get_size:
 * @file: An #OGMRipFile
 *
 * Gets the size of a file in bytes.
 *
 * Returns: The file size, or -1
 */
gint64
ogmrip_file_get_size (OGMRipFile *file)
{
  struct stat buf;
  guint64 size = 0;

  g_return_val_if_fail (file != NULL, -1);

  if (fstat (file->fd, &buf) == 0)
    size = (guint64) buf.st_size;

  return size;
}

/**
 * ogmrip_file_get_filename:
 * @file: An #OGMRipFile
 *
 * Gets the filename of a file.
 *
 * Returns: The filename, or NULL
 */
gchar *
ogmrip_file_get_filename (OGMRipFile *file)
{
  g_return_val_if_fail (file != NULL, NULL);

  return g_strdup (file->filename);
}

static void
ogmrip_file_detect_charset (OGMRipFile *file)
{
  EncaAnalyser analyser = NULL;

  size_t i, nlang;
  const char **langv;
  int code;

  langv = enca_get_languages (&nlang);
  for (i = 0; i < nlang - 1; i++)
  {
    code = (langv[i][0] << 8) | langv[i][1];
    if (code == file->lang)
      analyser = enca_analyser_alloc (langv[i]);
  }

  if (!analyser)
    analyser = enca_analyser_alloc ("__");

  if (analyser)
  {
    /*
    enca_set_threshold (analyser, 1.38);
    enca_set_multibyte (analyser, 1);
    enca_set_ambiguity (analyser, 1);
    enca_set_garbage_test (analyser, 1);
    */
    OGMRIP_SUBP_FILE (file)->charset =
      enca_analyse_file (analyser, file->filename);
    enca_analyser_free (analyser);
  }
}

/**
 * ogmrip_file_set_language:
 * @file: An #OGMRipFile
 * @lang: A language code
 *
 * Sets the language of a file.
 */
void
ogmrip_file_set_language (OGMRipFile *file, gint lang)
{
  g_return_if_fail (file != NULL);

  if (file->lang != lang)
  {
    file->lang = lang;
    if (file->type == OGMRIP_FILE_TYPE_SUBP)
      ogmrip_file_detect_charset (file);
  }
}

/**
 * ogmrip_file_get_language:
 * @file: An #OGMRipFile
 *
 * Gets the language of a file.
 *
 * Returns: A language code, or -1
 */
gint
ogmrip_file_get_language (OGMRipFile *file)
{
  g_return_val_if_fail (file != NULL, -1);

  return file->lang;
}

OGMRipFileConfig audio_config[] =
{
  { "MP3 ", 0x55,    OGMRIP_FORMAT_MP3    },
  { ".mp3", 0,       OGMRIP_FORMAT_MP3    },
  { "LAME", 0,       OGMRIP_FORMAT_MP3    },
  { "mp4a", 0,       OGMRIP_FORMAT_AAC    },
  { "MP4A", 0,       OGMRIP_FORMAT_AAC    },
  { "AAC ", 0xff,    OGMRIP_FORMAT_AAC    },
  { "AAC ", 0x706d,  OGMRIP_FORMAT_AAC    },
  { "AAC ", 0x4143,  OGMRIP_FORMAT_AAC    },
  { "AAC ", 0xa106,  OGMRIP_FORMAT_AAC    },
  { "AACP", 0,       OGMRIP_FORMAT_AAC    },
  { "vrbs", 0x566f,  OGMRIP_FORMAT_VORBIS },
  { "dnet", 0,       OGMRIP_FORMAT_AC3    },
  { "PCM ", 0x01,    OGMRIP_FORMAT_PCM    },
  { "MP12", 0x50,    OGMRIP_FORMAT_MP12   },
  { "AC-3", 0x2000,  OGMRIP_FORMAT_AC3    },
  { "ac-3", 0,       OGMRIP_FORMAT_AC3    },
  { "DTS ", 0x2001,  OGMRIP_FORMAT_DTS    },
  { "fLaC", 0xF1AC,  OGMRIP_FORMAT_FLAC   },
  { "LPCM", 0x10001, OGMRIP_FORMAT_LPCM   },
  { NULL,   0,       0                    }
};

/**
 * ogmrip_audio_file_new:
 * @filename: A filename
 * @error: A location to return an error of type #OGMRIP_FILE_ERROR
 *
 * Creates a new #OGMRipAudioFile from au audio file.
 *
 * Returns: The new #OGMRipAudioFile
 */
OGMRipFile *
ogmrip_audio_file_new (const gchar *filename, GError **error)
{
  OGMRipAudioFile *audio;

  GError *tmp_error = NULL;
  gboolean result, is_video = FALSE;
  gint i, j, rate = -1, bitrate = -1, format = -1, channels = -1;
  gchar **argv, *output, **lines;
  gdouble length = -1.0;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  g_return_val_if_fail (g_file_test (filename, G_FILE_TEST_IS_REGULAR), NULL);

  argv = ogmrip_backend_identify_command (filename, TRUE);
  if (!argv)
    return NULL;

  result = g_spawn_sync (NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL,
      NULL, NULL, &output, NULL, NULL, &tmp_error);
  g_strfreev (argv);

  if (!result)
  {
    g_propagate_error (error, tmp_error);
    return NULL;
  }

  lines = g_strsplit (output, "\n", 0);
  g_free (output);

  if (!lines)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_UNKNOWN,
        _("Unknown error while identifying '%s'"), filename);
    return NULL;
  }

  for (i = 0; lines[i]; i++)
  {
    errno = 0;

    if (g_str_has_prefix (lines[i], "ID_AUDIO_BITRATE="))
      bitrate = strtoul (lines[i] + 17, NULL, 10);
    else if (g_str_has_prefix (lines[i], "ID_AUDIO_RATE="))
      rate = strtoul (lines[i] + 14, NULL, 10);
    else if (g_str_has_prefix (lines[i], "ID_LENGTH="))
      length = strtod (lines[i] + 10, NULL);
    else if (g_str_has_prefix (lines[i], "ID_AUDIO_NCH="))
      channels = strtoul (lines[i] + 13, NULL, 10);
    else if (g_str_has_prefix (lines[i], "ID_AUDIO_FORMAT="))
    {
      gint fourcc = 0, value;

      if (strlen (lines[i] + 16) == 4)
      {
        fourcc  = (lines[i][19] << 24) & 0xff000000;
        fourcc |= (lines[i][18] << 16) & 0x00ff0000;
        fourcc |= (lines[i][17] <<  8) & 0x0000ff00;
        fourcc |= (lines[i][16] <<  0) & 0x000000ff;
      }

      value = strtoul (lines[i] + 16, NULL, 10);

      for (j = 0; audio_config[j].fourcc; j ++)
      {
        if (value && value == audio_config[j].value)
        {
          format = audio_config[j].format;
          break;
        }

        if (fourcc && g_str_has_prefix (lines[i] + 16, audio_config[j].fourcc))
        {
          format = audio_config[j].format;
          break;
        }
      }
    }
    else if (g_str_has_prefix (lines[i], "ID_VIDEO"))
      is_video = TRUE;

    if (errno != 0)
    {
      g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_RANGE,
          _("Cannot identify file '%s': %s"), filename, g_strerror (errno));
      g_strfreev (lines);
      return NULL;
    }
  }

  g_strfreev (lines);


  if (bitrate < 0)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_BITRATE,
        _("Cannot get bitrate of file '%s'"), filename);
    return NULL;
  }

  if (rate < 0)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_RATE,
        _("Cannot get rate of file '%s'"), filename);
    return NULL;
  }

  if (length < 0.0)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_LENGTH,
        _("Cannot get length of file '%s'"), filename);
    return NULL;
  }

  if (format < 0)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_FORMAT,
        _("Cannot get format of file '%s'"), filename);
    return NULL;
  }

  if (channels < 0)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_FORMAT,
        _("Cannot get number of channels of file '%s'"), filename);
    return NULL;
  }

  if (is_video)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_FORMAT,
        _("File '%s' contains video tracks"), filename);
    return NULL;
  }

  if (format != 0x01   && format != 0x55 && format != 0x2000 && format != 0x2001 &&
      format != 0x706d && format != 0xff && format != 0x566f)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_FORMAT,
        _("Format of file '%s' is not supported"), filename);
    return NULL;
  }

  audio = g_new0 (OGMRipAudioFile, 1);
  OGMRIP_FILE (audio)->type = OGMRIP_FILE_TYPE_AUDIO;
  OGMRIP_FILE (audio)->lang = -1;

  if (!ogmrip_file_construct (OGMRIP_FILE (audio), filename))
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_UNKNOWN,
        _("Unknown error while opening '%s': %s"), filename, g_strerror (errno));
    g_free (audio);
    return NULL;
  }

  switch (format)
  {
    case 0x01:
      OGMRIP_FILE (audio)->format = OGMRIP_FORMAT_PCM;
      break;
    case 0x55:
      OGMRIP_FILE (audio)->format = OGMRIP_FORMAT_MP3;
      break;
    case 0x2000:
      OGMRIP_FILE (audio)->format = OGMRIP_FORMAT_AC3;
      break;
    case 0x2001:
      OGMRIP_FILE (audio)->format = OGMRIP_FORMAT_DTS;
      break;
    case 0x566f:
      OGMRIP_FILE (audio)->format = OGMRIP_FORMAT_VORBIS;
      break;
    case 0x706d:
    case 0xff:
      OGMRIP_FILE (audio)->format = OGMRIP_FORMAT_AAC;
      break;
    default:
      g_assert_not_reached ();
  }

  audio->rate = rate;
  audio->length = length;
  audio->bitrate = bitrate;
  audio->channels = channels;

  return OGMRIP_FILE (audio);
}

/**
 * ogmrip_audio_file_get_sample_rate:
 * @audio: An #OGMRipAudioFile
 *
 * Gets the sample rate of an audio file.
 *
 * Returns: The sample rate, or -1
 */
gint
ogmrip_audio_file_get_sample_rate (OGMRipAudioFile *audio)
{
  g_return_val_if_fail (audio != NULL, -1);

  return audio->rate;
}

/**
 * ogmrip_audio_file_get_bitrate:
 * @audio: An #OGMRipAudioFile
 *
 * Gets the bitrate of an audio file.
 *
 * Returns: The bitrate, or -1
 */
gint
ogmrip_audio_file_get_bitrate (OGMRipAudioFile *audio)
{
  g_return_val_if_fail (audio != NULL, -1);

  return audio->bitrate;
}

/**
 * ogmrip_audio_file_get_length:
 * @audio: An #OGMRipAudioFile
 *
 * Gets the length in seconds of an audio file.
 *
 * Returns: The length, or -1.0
 */
gdouble
ogmrip_audio_file_get_length (OGMRipAudioFile *audio)
{
  g_return_val_if_fail (audio != NULL, -1.0);

  return audio->length;
}

/**
 * ogmrip_audio_file_get_samples_per_frame:
 * @audio: An #OGMRipAudioFile
 *
 * Gets the number of samples per frame of an audio file.
 *
 * Returns: The number of samples per frame, or -1
 */
gint
ogmrip_audio_file_get_samples_per_frame (OGMRipAudioFile *audio)
{
  g_return_val_if_fail (audio != NULL, -1);

  switch (audio->file.format)
  {
    case OGMRIP_FORMAT_MP3:
      return 1152;
      break;
    case OGMRIP_FORMAT_AC3:
    case OGMRIP_FORMAT_DTS:
      return 1536;
      break;
  }

  return 1024;
}

/**
 * ogmrip_audio_file_get_channels:
 * @audio: An #OGMRipAudioFile
 *
 * Gets the number of channels of an audio file.
 *
 * Returns: an #OGMDvdAudioChannels, or -1
 */
gint
ogmrip_audio_file_get_channels (OGMRipAudioFile *audio)
{
  g_return_val_if_fail (audio != NULL, -1);

  return audio->channels - 1;
}

static OGMRipSubpFile *
ogmrip_subp_file_new_sub (const gchar *filename, GError **error)
{
  OGMRipSubpFile *subp;

  GError *tmp_error = NULL;
  gchar **argv, *output, **lines;
  gint i, j, format = -1;
  gboolean result;

  gchar *sr[] = 
  {
    "microdvd", "subrip", "subviewer", "sami", "vplayer",
    "rt", "ssa", "pjs", "mpsub", "aqt", "subviewer 2.0",
    "subrip 0.9", "jacosub", "mpl2", NULL
  };

  argv = ogmrip_backend_identify_sub_command (filename, FALSE);
  if (!argv)
    return NULL;

  result = g_spawn_sync (NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL,
      NULL, NULL, &output, NULL, NULL, &tmp_error);
  g_strfreev (argv);

  if (!result)
  {
    g_propagate_error (error, tmp_error);
    return NULL;
  }

  lines = g_strsplit (output, "\n", 0);
  g_free (output);

  if (!lines)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_UNKNOWN,
        _("Unknown error while identifying '%s'"), filename);
    return NULL;
  }

  for (i = 0; lines[i] && format == -1; i++)
  {
    if (g_str_has_prefix (lines[i], "SUB: ") &&
        g_str_has_prefix (lines[i] + 5, "Detected subtitle file format: "))
    {
      for (j = 0; sr[j] && format < 0; j++)
        if (strcmp (lines[i] + 36, sr[j]) == 0)
          format = OGMRIP_FORMAT_MICRODVD + j;
    }
  }

  g_strfreev (lines);

  if (format < 0)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_FORMAT, 
        _("Cannot get format of file '%s'"), filename);
    return NULL;
  }

  subp = g_new0 (OGMRipSubpFile, 1);
  OGMRIP_FILE (subp)->type = OGMRIP_FILE_TYPE_SUBP;
  OGMRIP_FILE (subp)->format = format;
  OGMRIP_FILE (subp)->lang = -1;

  if (!ogmrip_file_construct (OGMRIP_FILE (subp), filename))
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_UNKNOWN,
        _("Unknown error while opening '%s': %s"), filename, g_strerror (errno));
    g_free (subp);
    return NULL;
  }

  subp->charset = -1;

  return subp;
}

static OGMRipSubpFile *
ogmrip_subp_file_new_vobsub (const gchar *filename, GError **error)
{
  OGMRipSubpFile *subp;
  gchar **argv, *output, **lines;
  gint format = OGMRIP_FORMAT_VOBSUB;

  GError *tmp_error = NULL;
  gint i;

  argv = ogmrip_backend_identify_sub_command (filename, TRUE);
  if (!argv)
    return NULL;

  if (!g_spawn_sync (NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL,
        NULL, NULL, NULL, &output, NULL, &tmp_error))
  {
    g_propagate_error (error, tmp_error);
    g_strfreev (argv);
    return NULL;
  }

  lines = g_strsplit (output, "\n", 0);
  g_free (output);

  if (!lines)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_UNKNOWN,
        _("Unknown error while identifying '%s'"), filename);
    return NULL;
  }

  for (i = 0; lines[i] && format != -1; i++)
  {
    if (g_str_has_prefix (lines[i], "VobSub: ") &&
        (g_str_has_prefix (lines[i] + 8, "Can't open IDX file") ||
         g_str_has_prefix (lines[i] + 8, "Can't open SUB file")))
    {
      format = -1;
      break;
    }
  }

  g_strfreev (lines);

  if (format < 0)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_FORMAT, 
        _("Cannot get format of file '%s'"), filename);
    return NULL;
  }

  subp = g_new0 (OGMRipSubpFile, 1);
  OGMRIP_FILE (subp)->type = OGMRIP_FILE_TYPE_SUBP;
  OGMRIP_FILE (subp)->format = format;
  OGMRIP_FILE (subp)->lang = -1;

  if (!ogmrip_file_construct (OGMRIP_FILE (subp), filename))
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_UNKNOWN,
        _("Unknown error while opening '%s': %s"), filename, g_strerror (errno));
    g_free (subp);
    return NULL;
  }

  subp->charset = -1;

  return subp;
}

/**
 * ogmrip_subp_file_new:
 * @filename: A filename
 * @error: A location to return an error of type #OGMRIP_FILE_ERROR
 *
 * Creates a new #OGMRipSubpFile from a subtitle file.
 *
 * Returns: The new #OGMRipSubpFile
 */
OGMRipFile *
ogmrip_subp_file_new (const gchar *filename, GError **error)
{
  GError *tmp_error = NULL;
  OGMRipSubpFile *subp = NULL;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  g_return_val_if_fail (g_file_test (filename, G_FILE_TEST_IS_REGULAR), NULL);

  if (g_str_has_suffix (filename, ".idx"))
  {
    gchar *basename;

    basename = g_strndup (filename, strlen (filename) - 4);
    subp = ogmrip_subp_file_new_vobsub (filename, &tmp_error);
    g_free (basename);
  }

  if (!subp)
  {
    g_clear_error (&tmp_error);
    subp = ogmrip_subp_file_new_sub (filename, &tmp_error);
  }

  if (!subp && tmp_error)
    g_propagate_error (error, tmp_error);

  return OGMRIP_FILE (subp);
}

/**
 * ogmrip_subp_file_get_charset:
 * @subp: An #OGMRipSubpFile
 *
 * Gets the character set of a subtitle file.
 *
 * Returns: The #OGMRipCharset, or -1
 */
gint
ogmrip_subp_file_get_charset (OGMRipSubpFile *subp)
{
  g_return_val_if_fail (subp != NULL, -1);

  return subp->charset;
}

static const OGMRipFileConfig video_config[] =
{
  { "XVID",       0, OGMRIP_FORMAT_MPEG4  },
  { "xvid",       0, OGMRIP_FORMAT_MPEG4  },
  { "XviD",       0, OGMRIP_FORMAT_MPEG4  },
  { "FMP4",       0, OGMRIP_FORMAT_MPEG4  },
  { "fmp4",       0, OGMRIP_FORMAT_MPEG4  },
  { "DIVX",       0, OGMRIP_FORMAT_MPEG4  },
  { "divx",       0, OGMRIP_FORMAT_MPEG4  },
  { "DX50",       0, OGMRIP_FORMAT_MPEG4  },
  { "dx50",       0, OGMRIP_FORMAT_MPEG4  },
  { "MP4V",       0, OGMRIP_FORMAT_MPEG4  },
  { "mp4v",       0, OGMRIP_FORMAT_MPEG4  },
  { "h264",       0, OGMRIP_FORMAT_H264   },
  { "H264",       0, OGMRIP_FORMAT_H264   },
  { "x264",       0, OGMRIP_FORMAT_H264   },
  { "X264",       0, OGMRIP_FORMAT_H264   },
  { "avc1",       0, OGMRIP_FORMAT_H264   },
  { "AVC1",       0, OGMRIP_FORMAT_H264   },
  { "davc",       0, OGMRIP_FORMAT_H264   },
  { "DAVC",       0, OGMRIP_FORMAT_H264   },
  { "theo",       0, OGMRIP_FORMAT_THEORA },
  { "Thra",       0, OGMRIP_FORMAT_THEORA },
  { "MJPG",       0, OGMRIP_FORMAT_MJPEG  },
  { "mpg1",       0, OGMRIP_FORMAT_MPEG1  },
  { "MPG1",       0, OGMRIP_FORMAT_MPEG1  },
  { "0x10000001", 0, OGMRIP_FORMAT_MPEG1  },
  { "mpg2",       0, OGMRIP_FORMAT_MPEG2  },
  { "MPG2",       0, OGMRIP_FORMAT_MPEG2  },
  { "0x10000002", 0, OGMRIP_FORMAT_MPEG2  },
  { "drac",       0, OGMRIP_FORMAT_DIRAC  },
  { "VP80",       0, OGMRIP_FORMAT_VP8    },
  { NULL,         0, 0                    }
};

/**
 * ogmrip_video_file_new:
 * @filename: A filename
 * @error: A location to return an error of type #OGMRIP_FILE_ERROR
 *
 * Creates a new #OGMRipVideoFile from a video file.
 *
 * Returns: The new #OGMRipVideoFile
 */
OGMRipFile *
ogmrip_video_file_new (const gchar *filename, GError **error)
{
  OGMRipVideoFile *video;

  GError *tmp_error = NULL;
  gint i, j, bitrate = -1, format = -1, width = -1, height = -1;
  gdouble fps = -1.0, aspect = -1.0, length = -1.0;
  gchar **argv, *output, **lines;
  gboolean result;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  g_return_val_if_fail (g_file_test (filename, G_FILE_TEST_IS_REGULAR), NULL);

  argv = ogmrip_backend_identify_command (filename, FALSE);
  if (!argv)
    return NULL;

  result = g_spawn_sync (NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL,
      NULL, NULL, &output, NULL, NULL, &tmp_error);
  g_strfreev (argv);

  if (!result)
  {
    g_propagate_error (error, tmp_error);
    return NULL;
  }

  lines = g_strsplit (output, "\n", 0);
  g_free (output);

  if (!lines)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_UNKNOWN,
        _("Unknown error while identifying '%s'"), filename);
    return NULL;
  }

  for (i = 0; lines[i]; i++)
  {
    errno = 0;

    if (g_str_has_prefix (lines[i], "ID_VIDEO_BITRATE="))
      bitrate = strtoul (lines[i] + 17, NULL, 10);
    else if (g_str_has_prefix (lines[i], "ID_LENGTH="))
      length = strtod (lines[i] + 10, NULL);
    else if (g_str_has_prefix (lines[i], "ID_VIDEO_WIDTH="))
      width = strtoul (lines[i] + 15, NULL, 10);
    else if (g_str_has_prefix (lines[i], "ID_VIDEO_HEIGHT="))
      height = strtoul (lines[i] + 16, NULL, 10);
    else if (g_str_has_prefix (lines[i], "ID_VIDEO_FPS="))
      fps = strtod (lines[i] + 13, NULL);
    else if (g_str_has_prefix (lines[i], "ID_VIDEO_ASPECT="))
      aspect = strtod (lines[i] + 16, NULL);
    else if (g_str_has_prefix (lines[i], "ID_VIDEO_FORMAT="))
    {
      for (j = 0; video_config[j].fourcc; j ++)
      {
        if (g_str_has_prefix (lines[i] + 16, video_config[j].fourcc))
        {
          format = video_config[j].format;
          break;
        }
      }
    }

    if (errno != 0)
    {
      g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_RANGE,
          _("Cannot identify file '%s': %s"), filename, g_strerror (errno));
      g_strfreev (lines);
      return NULL;
    }
  }

  g_strfreev (lines);


  if (bitrate < 0)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_BITRATE,
        _("Cannot get bitrate of file '%s'"), filename);
    return NULL;
  }

  if (length < 0.0)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_LENGTH,
        _("Cannot get length of file '%s'"), filename);
    return NULL;
  }

  if (format < 0)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_FORMAT,
        _("Cannot get format of file '%s'"), filename);
    return NULL;
  }

  if (width < 0)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_WIDTH,
        _("Cannot get width of video file '%s'"), filename);
    return NULL;
  }

  if (height < 0)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_HEIGHT,
        _("Cannot get height of video file '%s'"), filename);
    return NULL;
  }

  if (aspect < 0.0)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_ASPECT,
        _("Cannot get aspect ratio of video file '%s'"), filename);
    return NULL;
  }

  if (fps < 0.0)
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_FPS,
        _("Cannot get frame rate of video file '%s'"), filename);
    return NULL;
  }

  video = g_new0 (OGMRipVideoFile, 1);
  OGMRIP_FILE (video)->type = OGMRIP_FILE_TYPE_VIDEO;
  OGMRIP_FILE (video)->format = format;

  if (!ogmrip_file_construct (OGMRIP_FILE (video), filename))
  {
    g_set_error (error, OGMRIP_FILE_ERROR, OGMRIP_FILE_ERROR_UNKNOWN,
        _("Unknown error while opening '%s': %s"), filename, g_strerror (errno));
    g_free (video);
    return NULL;
  }

  video->length = length;
  video->bitrate = bitrate;
  video->width = width;
  video->height = height;
  video->aspect = aspect;
  video->fps = fps;

  return OGMRIP_FILE (video);
}

/**
 * ogmrip_video_file_get_bitrate:
 * @video: An #OGMRipVideoFile
 *
 * Gets the bitrate of a video file.
 *
 * Returns: The bitrate, or -1
 */
gint
ogmrip_video_file_get_bitrate (OGMRipVideoFile *video)
{
  g_return_val_if_fail (video != NULL, -1);

  return video->bitrate;
}

/**
 * ogmrip_video_file_get_length:
 * @video: An #OGMRipVideoFile
 *
 * Gets the length in seconds of a video file.
 *
 * Returns: The length, or -1.0
 */
gdouble
ogmrip_video_file_get_length (OGMRipVideoFile *video)
{
  g_return_val_if_fail (video != NULL, -1.0);

  return video->length;
}

/**
 * ogmrip_video_file_get_size:
 * @video: An #OGMRipVideoFile
 * @width: A pointer to store the width, or NULL
 * @height: A pointer to store the height, or NULL
 *
 * Gets the dimension of a video file.
 */
void
ogmrip_video_file_get_size (OGMRipVideoFile *video, guint *width, guint *height)
{
  g_return_if_fail (video != NULL);

  if (width)
    *width = video->width;

  if (height)
    *height = video->height;
}

/**
 * ogmrip_video_file_get_framerate:
 * @video: An #OGMRipVideoFile
 *
 * Gets the framerate of a video file.
 *
 * Returns: The framerate, or -1
 */
gdouble
ogmrip_video_file_get_framerate (OGMRipVideoFile *video)
{
  g_return_val_if_fail (video != NULL, -1.0);

  return video->fps;
}

/**
 * ogmrip_video_file_get_aspect_ratio:
 * @video: An #OGMRipVideoFile
 *
 * Gets the aspect ratio of a video file.
 *
 * Returns: The aspect ratio, or -1
 */
gdouble
ogmrip_video_file_get_aspect_ratio (OGMRipVideoFile *video)
{
  g_return_val_if_fail (video != NULL, -1.0);

  return video->aspect;
}

