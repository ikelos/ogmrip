/* OGMRip - A media encoder for GNOME
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-application.h"
#include "ogmrip-cli.h"
#include "ogmrip-settings.h"

#include <ogmrip-encode.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <errno.h>
#include <stdlib.h>

#define OGMRIP_CLI_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_CLI, OGMRipCliPriv))

struct _OGMRipCliPriv
{
  GCancellable *cancellable;
};

enum
{
  NONE,
  LIST,
  ENCODE,
  PROFILES
};

static gint     action   = NONE;

/*
 * Common options
 */
static gboolean help     = FALSE;
static gboolean debug    = FALSE;
static gchar    *input   = NULL;
static gint     ntitle   = -1;

/*
 * List options
 */
static gboolean video    = FALSE;
static gboolean audio    = FALSE;
static gboolean chapters = FALSE;
static gboolean angles   = FALSE;
static gboolean palette  = FALSE;
static gboolean subp     = FALSE;
static gboolean all      = FALSE;

/*
 * Encoding options
 */
static gchar    *output    = NULL;
static gchar    *preset    = NULL;
static gboolean aid[100];
static gboolean sid[100];
static GSList   *alang     = NULL;
static GSList   *slang     = NULL;
static gboolean relative   = FALSE;
static gint     start_chap = 0;
static gint     end_chap   = -1;

static gint     naid       = 0;
static gint     nsid       = 0;

extern GSettings *settings;

static gboolean
parse_input (const gchar *name, const gchar *value, gpointer data, GError **error)
{
  struct stat buf;

  if (g_stat (value, &buf) < 0)
  {
    g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
        _("Failed to stat '%s': %s"), value, g_strerror (errno));
    return FALSE;
  }

  if (g_path_is_absolute (value))
    input = g_strdup (value);
  else
  {
    gchar *cwd, *dirname, *basename;

    cwd = g_get_current_dir ();

    dirname = g_path_get_dirname (value);
    g_chdir (dirname);
    g_free (dirname);

    dirname = g_get_current_dir ();
    g_chdir (cwd);
    g_free (cwd);

    basename = g_path_get_basename (value);
    input = g_build_filename (dirname, basename, NULL);
    g_free (basename);
  }

  return TRUE;
}

static gboolean
parse_alang (const gchar *str, gchar **endptr, GError **error)
{
  guint code;

  if (str[0] == '\0' || str[1] == '\0' || (str[2] != ',' && str[2] != '\0'))
    return FALSE;

  code = (str[0] << 8) | str[1];
  alang = g_slist_append (alang, GUINT_TO_POINTER (code));

  if (endptr)
    *endptr = (gchar *) str + 2;

  return TRUE;
}

static gboolean
parse_aid (const gchar *name, const gchar *value, gpointer data, GError **error)
{
  gchar *str = (gchar *) value, *endptr;
  guint id;

  if (!value || strlen (value) == 0)
    return FALSE;

  while (TRUE)
  {
    errno = 0;
    endptr = NULL;

    id = strtoul (str, &endptr, 10);

    if (errno == 0)
    {
      if (id > 0 && id < 100)
      {
        if (!aid[id - 1])
          naid ++;

        aid[id - 1] = TRUE;
      }
      else
      {
        errno = 1;
        if (str == endptr && parse_alang (str, &endptr, error))
          errno = 0;
      }

      if (*endptr != '\0' && *endptr != ',')
        errno = 1;
    }

    if (errno)
    {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
          _("Cannot parse value '%s' for %s"), value, name);
      return FALSE;
    }

    str = endptr;

    if (*str == '\0')
      break;
    str ++;
  }

  return TRUE;
}

static gboolean
parse_slang (const gchar *str, gchar **endptr, GError **error)
{
  guint code;

  if (str[0] == '\0' || str[1] == '\0' || (str[2] != ',' && str[2] != '\0'))
    return FALSE;

  code = (str[0] << 8) | str[1];
  slang = g_slist_append (slang, GUINT_TO_POINTER (code));

  if (endptr)
    *endptr = (gchar *) str + 2;

  return TRUE;
}

static gboolean
parse_sid (const gchar *name, const gchar *value, gpointer data, GError **error)
{
  gchar *str = (gchar *) value, *endptr;
  guint id;

  if (!value || strlen (value) == 0)
    return FALSE;

  while (TRUE)
  {
    errno = 0;
    endptr = NULL;

    id = strtoul (str, &endptr, 10);

    if (errno == 0)
    {
      if (id > 0 && id < 100)
      {
        if (!sid[id - 1])
          nsid ++;

        sid[id - 1] = TRUE;
      }
      else
      {
        errno = 1;
        if (str == endptr && parse_slang (str, &endptr, error))
          errno = 0;
      }

      if (*endptr != '\0' && *endptr != ',')
        errno = 1;
    }

    if (errno)
    {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
          _("Cannot parse value '%s' for %s"), value, name);
      return FALSE;
    }

    str = endptr;

    if (*str == '\0')
      break;
    str ++;
  }

  return TRUE;
}

static gboolean
parse_chap (const gchar *name, const gchar *value, gpointer data, GError **error)
{
  gchar *str;

  if (!value || strlen (value) == 0)
    return FALSE;

  errno = 0;
  start_chap = strtoul (value, &str, 10);

  if (!errno && *str == '-')
    end_chap = strtoul (str + 1, &str, 10);

  if (errno == ERANGE || *str != '\0')
  {
    g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
        _("Cannot parse value '%s' for %s"), value, name);
    return FALSE;
  }

  if (end_chap != -1 && start_chap > end_chap)
  {
    gint swap;

    swap = start_chap;
    start_chap = end_chap;
    end_chap = swap;
  }

  return TRUE;
}

static void
ogmrip_cli_video_info (OGMRipTitle *title)
{
  OGMRipVideoStream *stream;
  guint width, height, num, denom;
  gint val;

  stream = ogmrip_title_get_video_stream (title);

  val = ogmrip_stream_get_format (OGMRIP_STREAM (stream));
  g_print ("\t%s: %s, ", _("Format"), ogmrip_format_get_label (val));

  ogmrip_video_stream_get_aspect_ratio (stream, &num, &denom);
  g_print ("%s: %u:%u, ", _("Aspect ratio"), num, denom);

  ogmrip_video_stream_get_resolution (stream, &width, &height);
  g_print ("%s: %dx%d\n", _("Resolution"), width, height);
}

static void
ogmrip_cli_palette_info (OGMRipTitle *title)
{
  const guint *palette;

  palette = ogmrip_title_get_palette (title);
  if (palette)
  {
    guint i;

    g_print ("\t%s:", ("Palette"));
    for (i = 0; i < 16; i++)
      printf (" %06x", palette[i]);
    printf ("\n");
  }
}

static void
ogmrip_cli_audio_info (OGMRipTitle *title)
{
  GList *list, *link;
  gint val;

  list = ogmrip_title_get_audio_streams (title);
  for (link = list; link; link = link->next)
  {
    g_print ("\t%s: %02d, ", _("Audio"), ogmrip_stream_get_id (link->data) + 1);

    val = ogmrip_audio_stream_get_language (link->data);
    g_print ("%s: %s, ", _("Language"), ogmrip_language_to_iso639_1 (val));

    val = ogmrip_stream_get_format (link->data);
    g_print ("%s: %s, ", _("Format"), ogmrip_format_get_label (val));

    g_print ("%s: 48 kHz, ", _("Frequency"));

    val = ogmrip_audio_stream_get_quantization (link->data);
    if (val != OGMRIP_QUANTIZATION_UNDEFINED)
      g_print ("%s: %s, ", _("Quantization"), ogmrip_quantization_get_label (val));

    val = ogmrip_audio_stream_get_channels (link->data);
    g_print ("%s: %s", _("Channels"), ogmrip_channels_get_label (val));

    val = ogmrip_audio_stream_get_content (link->data);
    if (val != OGMRIP_AUDIO_CONTENT_UNDEFINED)
      g_print (", %s: %s", _("Content"), ogmrip_audio_content_get_label (val));

    g_print ("\n");
  }
  g_list_free (list);
}

static void
ogmrip_cli_subp_info (OGMRipTitle *title)
{
  GList *list, *link;
  gint val;

  list = ogmrip_title_get_subp_streams (title);
  for (link = list; link; link = link->next)
  {
    g_print ("\t%s: %02d, ", _("Subtitle"), ogmrip_stream_get_id (link->data) + 1);

    val = ogmrip_subp_stream_get_language (link->data);
    g_print ("%s: %s, ", _("Language"), ogmrip_language_to_iso639_1 (val));

    val = ogmrip_stream_get_format (link->data);
    g_print ("%s: %s", _("Format"), ogmrip_format_get_label (val));

    val = ogmrip_subp_stream_get_content (link->data);
    if (val != OGMRIP_SUBP_CONTENT_UNDEFINED)
      g_print (", %s: %s", _("Content"), ogmrip_subp_content_get_label (val));

    g_print ("\n");
  }
  g_list_free (list);
}

static void
ogmrip_cli_chapters_info (OGMRipTitle *title)
{
  OGMRipTime length;
  gint chap, nchap;

  nchap = ogmrip_title_get_n_chapters (title);
  for (chap = 0; chap < nchap; chap ++)
  {
    if (ogmrip_title_get_chapters_length (title, chap, chap, &length) > 0.0)
      g_print ("\t\t%s: %02d, %s: %02lu:%02lu:%02lu\n",
          _("Chapter"), chap + 1, _("Length"), length.hour, length.min, length.sec);
  }
}

static gboolean
ogmrip_cli_title_info (OGMRipTitle *title)
{
  OGMRipTime length;
  gint num_chapters, num_audio, num_subp;

  ogmrip_title_get_length (title, &length);
  num_chapters = ogmrip_title_get_n_chapters (title);
  num_audio = ogmrip_title_get_n_audio_streams (title);
  num_subp = ogmrip_title_get_n_subp_streams (title);

  g_print ("%s: %02d, ", _("Title"), ogmrip_title_get_id (title) + 1);
  g_print ("%s: %02lu:%02lu:%02lu, ", _("Length"), length.hour, length.min, length.sec);
  g_print ("%s: %02d, ", _("Chapters"), num_chapters);
  g_print ("%s: %02d, ", _("Audio"), num_audio);
  g_print ("%s: %02d\n", _("Subpictures"), num_subp);

  if (video)
    ogmrip_cli_video_info (title);

  if (palette)
    ogmrip_cli_palette_info (title);

  if (audio)
    ogmrip_cli_audio_info (title);

  if (subp)
    ogmrip_cli_subp_info (title);

  if (angles)
    g_print ("\t%s: %d\n", _("Number of angles"), ogmrip_title_get_n_angles (title));

  if (chapters)
    ogmrip_cli_chapters_info (title);

  return TRUE;
}

static gboolean
ogmrip_cli_media_info (OGMRipMedia *media)
{
  GList *list, *link;
  gdouble length, longest_length = 0;
  gint longest_title = 1;

  list = ogmrip_media_get_titles (media);
  for (link = list; link; link = link->next)
  {
    ogmrip_cli_title_info (link->data);

    length = ogmrip_title_get_length (link->data, NULL);
    if (length > longest_length)
    {
      longest_length = length;
      longest_title = ogmrip_title_get_id (link->data);
    }
  }
  g_list_free (list);

  g_print ("%s: %d\n", _("Longest title"), longest_title);

  return TRUE;
}

static void
ogmrip_cli_media_open_cb (OGMRipMedia *media, gdouble percent, gpointer user_data)
{
  g_print ("\rOpening media: %d%%  ", (gint) (percent * 100));
}

static void
ogmrip_cli_list (OGMRipCli *cli)
{
  OGMRipMedia *media = NULL;
  gboolean retval;

  media = ogmrip_media_new (input);
  if (!media || !ogmrip_media_open (media, NULL, ogmrip_cli_media_open_cb, NULL, NULL))
  {
    g_printerr (_("Couldn't open media: '%s'"), input);
    g_printerr ("\n");
    goto cleanup;
  }
  g_print ("\rOpening media: 100%%\n");

  if (ntitle == -1)
    retval = ogmrip_cli_media_info (media);
  else
  {
    OGMRipTitle *title;

    if (ntitle < 1 || !(title = ogmrip_media_get_title (media, ntitle - 1)))
    {
      g_printerr (_("Title %d does not exist."), ntitle);
      g_printerr ("\n");
      goto cleanup;
    }

    retval = ogmrip_cli_title_info (title);
  }

  if (!retval)
  {
  }

cleanup:
  if (media)
    g_object_unref (media);
}

static OGMRipEncoding *
ogmrip_cli_create_encoding (OGMRipTitle *title, OGMRipProfile *profile)
{
  OGMRipEncoding *encoding;
  gboolean sync;
  guint method;

  encoding = ogmrip_encoding_new (title);
  ogmrip_encoding_set_test (encoding, FALSE);
  ogmrip_encoding_set_autocrop (encoding, TRUE);
  ogmrip_encoding_set_autoscale (encoding, TRUE);
  ogmrip_encoding_set_relative (encoding, relative);
  ogmrip_encoding_set_profile (encoding, profile);

  ogmrip_encoding_set_copy (encoding,
      g_settings_get_boolean (settings, OGMRIP_SETTINGS_COPY_DVD));

  ogmrip_profile_get (profile, OGMRIP_PROFILE_GENERAL,
      OGMRIP_PROFILE_ENCODING_METHOD, "u", &method);
  ogmrip_encoding_set_method (encoding, method);

  ogmrip_profile_get (profile, OGMRIP_PROFILE_GENERAL,
      OGMRIP_PROFILE_ENSURE_SYNC, "b", &sync);
  ogmrip_encoding_set_ensure_sync (encoding, sync);

  return encoding;
}

static gboolean
ogmrip_cli_set_container (OGMRipEncoding *encoding, OGMRipProfile *profile)
{
  GError *error = NULL;
  OGMRipContainer *container;
  gboolean status;
  GFile *file;

  container = ogmrip_container_new_from_profile (profile);
  status = ogmrip_encoding_set_container (encoding, container, &error);
  g_object_unref (container);

  file = g_file_new_for_path (output);
  ogmrip_container_set_output (container, file);
  g_object_unref (file);

  if (!status)
  {
    g_printerr ("%s\n", error->message);
    g_error_free (error);
    return FALSE;
  }

  return TRUE;
}

static gboolean
ogmrip_cli_set_video_codec (OGMRipEncoding *encoding, OGMRipProfile *profile, OGMRipTitle *title)
{
  GError *error = NULL;
  OGMRipVideoStream *stream;
  OGMRipCodec *codec;
  gboolean status;

  stream = ogmrip_title_get_video_stream (title);
  if (!stream)
  {
    g_printerr (_("Video stream does not exist."));
    g_printerr ("\n");
    return FALSE;
  }

  codec = ogmrip_video_codec_new_from_profile (stream, profile);
  ogmrip_codec_set_chapters (OGMRIP_CODEC (codec), start_chap, end_chap);
  /*
   * crop size, scale size, deinterlacer, angle
   */
  status = ogmrip_encoding_set_video_codec (encoding, OGMRIP_VIDEO_CODEC (codec), &error);
  g_object_unref (codec);

  if (!status)
  {
    g_printerr ("%s\n", error->message);
    g_error_free (error);
    return FALSE;
  }

  return TRUE;
}

static gboolean
ogmrip_cli_add_audio_codec (OGMRipEncoding *encoding, OGMRipProfile *profile, OGMRipTitle *title)
{
  GError *error = NULL;
  OGMRipAudioStream *stream;
  OGMRipCodec *codec;
  gboolean status;
  gint i;

  for (i = 0; i < 100; i ++)
  {
    if (aid[i])
    {
      stream = ogmrip_title_get_audio_stream (title, i);
      if (!stream)
      {
        g_printerr (_("Audio stream %d does not exist."), i);
        g_printerr ("\n");
        return FALSE;
      }

      codec = ogmrip_audio_codec_new_from_profile (stream, profile);
      ogmrip_codec_set_chapters (OGMRIP_CODEC (codec), start_chap, end_chap);
      /*
       * label, language
       */
      status = ogmrip_encoding_add_audio_codec (encoding, OGMRIP_AUDIO_CODEC (codec), &error);
      g_object_unref (codec);

      if (!status)
      {
        g_printerr ("%s\n", error->message);
        g_error_free (error);
        return FALSE;
      }
    }
  }

  return TRUE;
}

static gboolean
ogmrip_cli_add_subp_codec (OGMRipEncoding *encoding, OGMRipProfile *profile, OGMRipTitle *title)
{
  GError *error = NULL;
  OGMRipSubpStream *stream;
  OGMRipCodec *codec;
  gboolean status;
  gint i;

  for (i = 0; i < 100; i ++)
  {
    if (sid[i])
    {
      stream = ogmrip_title_get_subp_stream (title, i);
      if (!stream)
        return FALSE;

      codec = ogmrip_subp_codec_new_from_profile (stream, profile);
      ogmrip_codec_set_chapters (OGMRIP_CODEC (codec), start_chap, end_chap);
      /*
       * label, language
       */
      status = ogmrip_encoding_add_subp_codec (encoding, OGMRIP_SUBP_CODEC (codec), NULL);
      g_object_unref (codec);

      if (!status)
      {
        g_printerr ("%s\n", error->message);
        return FALSE;
      }
    }
  }

  return TRUE;
}

static void
ogmrip_cli_encoding_progress (OGMJobSpawn *spawn, gdouble fraction)
{
  OGMRipStream *stream;
  gchar *message;

  if (OGMRIP_IS_VIDEO_CODEC (spawn))
    g_print ("%s", _("Encoding video title"));
  else if (OGMRIP_IS_CHAPTERS (spawn))
    g_print ("%s", _("Extracting chapters information"));
  else if (OGMRIP_IS_CONTAINER (spawn))
    g_print ("%s", _("Merging audio and video streams"));
  else if (OGMRIP_IS_COPY (spawn))
    g_print ("%s", _("Copying media"));
  else if (OGMRIP_IS_ANALYZE (spawn))
    g_print ("%s", _("Analyzing video stream"));
  else if (OGMRIP_IS_TEST (spawn))
    g_print ("%s", _("Running the compressibility test"));
  else if (OGMRIP_IS_AUDIO_CODEC (spawn))
  {
    stream = ogmrip_codec_get_input (OGMRIP_CODEC (spawn));
    message = g_strdup_printf (_("Extracting audio stream %d"),
        ogmrip_stream_get_id (stream) + 1);
    g_print ("%s", message);
    g_free (message);
  }
  else if (OGMRIP_IS_SUBP_CODEC (spawn))
  {
    stream = ogmrip_codec_get_input (OGMRIP_CODEC (spawn));
    message = g_strdup_printf (_("Extracting subtitle stream %d"),
        ogmrip_stream_get_id (stream) + 1);
    g_print ("%s", message);
    g_free (message);
  }

  g_print (": %d%%", (gint) (fraction * 100));
}

static void
ogmrip_cli_encoding_run_cb (OGMRipEncoding *encoding, OGMJobSpawn *spawn)
{
  if (spawn)
    ogmrip_cli_encoding_progress (spawn, 0.0);
}

static void
ogmrip_cli_encoding_progress_cb (OGMRipEncoding *encoding, OGMJobSpawn *spawn, gdouble fraction)
{
  if (spawn)
  {
    g_print ("\r");
    ogmrip_cli_encoding_progress (spawn, fraction);
  }
}

static void
ogmrip_cli_encoding_complete_cb (OGMRipEncoding *encoding, OGMJobSpawn *spawn, OGMRipEncodingStatus status)
{
  if (spawn)
  {
    g_print ("\r");
    ogmrip_cli_encoding_progress (spawn, 1.0);
    g_print ("\n");
  }
}

static void
ogmrip_cli_encode (OGMRipCli *cli)
{
  OGMRipEncoding *encoding = NULL;
  OGMRipMedia *media = NULL;
  GError *error = NULL;

  OGMRipProfileEngine *engine;
  OGMRipProfile *profile;
  OGMRipTitle *title;

  engine = ogmrip_profile_engine_get_default ();

  if (!preset)
    preset = g_settings_get_string (settings, OGMRIP_SETTINGS_PROFILE);

  profile = ogmrip_profile_engine_get (engine, preset);
  if (!profile)
  {
    g_printerr (_("Profile '%s' does not exist."), preset);
    g_printerr ("\n");
    goto cleanup;
  }

  media = ogmrip_media_new (input);
  if (!media || !ogmrip_media_open (media, NULL, ogmrip_cli_media_open_cb, NULL, NULL))
  {
    g_printerr (_("Couldn't open media: '%s'"), input);
    g_printerr ("\n");
    goto cleanup;
  }

  if (ntitle < 1 || !(title = ogmrip_media_get_title (media, ntitle - 1)))
  {
    g_printerr (_("Title %d does not exist."), ntitle);
    g_printerr ("\n");
    goto cleanup;
  }

  encoding = ogmrip_cli_create_encoding (title, profile);

  if (!ogmrip_cli_set_container (encoding, profile))
    goto cleanup;

  if (!ogmrip_cli_set_video_codec (encoding, profile, title))
    goto cleanup;

  if (!ogmrip_cli_add_audio_codec (encoding, profile, title))
    goto cleanup;

  if (!ogmrip_cli_add_subp_codec (encoding, profile, title))
    goto cleanup;

  g_signal_connect (encoding, "run",
      G_CALLBACK (ogmrip_cli_encoding_run_cb), NULL);
  g_signal_connect (encoding, "progress",
      G_CALLBACK (ogmrip_cli_encoding_progress_cb), NULL);
  g_signal_connect (encoding, "complete",
      G_CALLBACK (ogmrip_cli_encoding_complete_cb), NULL);

  if (!ogmrip_title_open (title, NULL, NULL, NULL, &error))
  {
    g_printerr ("%s: %s", _("Cannot open title"), error->message);
    goto cleanup;
  }

  if (!ogmrip_encoding_encode (encoding, cli->priv->cancellable, &error))
    g_printerr ("%s: %s", _("Cannot encode title"), error->message);

  ogmrip_title_close (title);

cleanup:
  if (encoding)
  {
    ogmrip_encoding_clean (encoding, TRUE, TRUE, FALSE);
    g_object_unref (encoding);
  }

  if (media)
    g_object_unref (media);

  if (error)
    g_error_free (error);
}

static void
ogmrip_cli_prepare_cb (OGMRipCli *cli)
{
  switch (action)
  {
    case LIST:
      ogmrip_cli_list (cli);
      break;
    case ENCODE:
        ogmrip_cli_encode (cli);
      break;
    case NONE:
      g_assert_not_reached ();
      break;
  }

  g_application_release (G_APPLICATION (cli));
}

static void
ogmrip_cli_activate_cb (GApplication *app)
{
  g_application_hold (app);

  g_signal_connect (app, "prepare",
      G_CALLBACK (ogmrip_cli_prepare_cb), NULL);
}

static void
ogmrip_application_iface_init (OGMRipApplicationInterface *iface)
{
}

G_DEFINE_TYPE_WITH_CODE (OGMRipCli, ogmrip_cli, G_TYPE_APPLICATION,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_APPLICATION, ogmrip_application_iface_init));

static const GOptionEntry main_opts[] =
{
  { "help",  'h', 0, G_OPTION_ARG_NONE, &help,  N_("Show help options"),     NULL },
  { "debug", 'd', 0, G_OPTION_ARG_NONE, &debug, N_("Enable debug messages"), NULL },
  { NULL,     0,  0, 0,                 NULL,   NULL,                        NULL }
};

static const GOptionEntry list_opts[] =
{
  { "input",     'i', 0, G_OPTION_ARG_CALLBACK,  parse_input, N_("Specify the media path"),            "<path>" },
  { "title",     't', 0, G_OPTION_ARG_INT,       &ntitle,     N_("Specify the title (default: none)"), "<id>"   },
  { "video",     'v', 0, G_OPTION_ARG_NONE,      &video,      N_("Show video info"),                   NULL     },
  { "audio",     'a', 0, G_OPTION_ARG_NONE,      &audio,      N_("Show audio info"),                   NULL     },
  { "chapters",  'c', 0, G_OPTION_ARG_NONE,      &chapters,   N_("Show chapters info"),                NULL     },
  { "angles",    'g', 0, G_OPTION_ARG_NONE,      &angles,     N_("Show angles info"),                  NULL     },
  { "palette",   'p', 0, G_OPTION_ARG_NONE,      &palette,    N_("Show palette info"),                 NULL     },
  { "subtitles", 's', 0, G_OPTION_ARG_NONE,      &subp,       N_("Show subtitles info"),               NULL     },
  { "all",       'x', 0, G_OPTION_ARG_NONE,      &all,        N_("Show all info"),                     NULL     },
  { NULL,         0,  0, 0,                      NULL,        NULL,                                    NULL     }
};

static const GOptionEntry encode_opts[] =
{
  { "input",    'i', 0, G_OPTION_ARG_CALLBACK,  parse_input, N_("Specify the media path"),                            "<path>"          },
  { "output",   'o', 0, G_OPTION_ARG_FILENAME,  &output,     N_("Specify the output file"),                           "<file>"          },
  { "title",    't', 0, G_OPTION_ARG_INT,       &ntitle,     N_("Specify the title (default: none)"),                 "<id>"            },
  { "audio",    'a', 0, G_OPTION_ARG_CALLBACK,  parse_aid,   N_("Select the audio id to encode (default: none)"),     "<id>[,<id>]*"    },
  { "sub",      's', 0, G_OPTION_ARG_CALLBACK,  parse_sid,   N_("Select the subtitle id to extract (default: none)"), "<id>[,<id>]*"    },
  { "chapters", 'c', 0, G_OPTION_ARG_CALLBACK,  parse_chap,  N_("Specify which chapters to extract. (default: all)"), "<start>[-<end>]" },
  { "relative", 'r', 0, G_OPTION_ARG_NONE,      &relative,   N_("Extract selected chapters in relative mode"),        NULL              },
  { "profile",  'p', 0, G_OPTION_ARG_STRING,    &preset,     N_("Specify the encoding profile"),                      "<profile>"       },
  { NULL,        0,  0, 0,                      NULL,        NULL,                                                    NULL              }
};

static void
ogmrip_cli_usage (GOptionContext *context)
{
  if (context)
  {
    gchar *str;

    str = g_option_context_get_help (context, FALSE, NULL);
    g_print ("%s", str);
    g_free (str);
  }
  else
  {
    g_print ("%s\n", _("Usage:"));
    g_print ("  %s <%s> [%s...]\n\n", g_get_prgname (), _("COMMAND"), _("OPTION"));
    g_print ("%s:\n", _("Commands"));
    g_print ("  list   - %s\n", _("lists the content of a media"));
    g_print ("  encode - %s\n", _("encodes selected title, audio streams and subtitles"));
    g_print ("\n");
    g_print ("  %s '%s <%s> --help' %s\n\n", _("Try"), g_get_prgname (), _("COMMAND"), _("to get a list of available options"));
  }
}

static void
ogmrip_cli_dispose (GObject *gobject)
{
  OGMRipCli *cli = OGMRIP_CLI (gobject);

  if (cli->priv->cancellable)
  {
    g_object_unref (cli->priv->cancellable);
    cli->priv->cancellable = NULL;
  }

  G_OBJECT_CLASS (ogmrip_cli_parent_class)->dispose (gobject);
}

static gboolean
ogmrip_cli_local_cmdline (GApplication *cli, gchar ***argv, gint *status)
{
  GError *error = NULL;
  GOptionContext *context;
  gboolean retval;
  gint argc;

  context = g_option_context_new (NULL);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_add_main_entries (context, main_opts, GETTEXT_PACKAGE);

  argc = g_strv_length (*argv);

  if (argc > 1)
  {
    if (g_str_equal ((*argv)[1], "list"))
    {
      action = LIST;
      g_set_prgname ("ogmrip list");
      g_option_context_add_main_entries (context, list_opts, GETTEXT_PACKAGE);
    }
    else if (g_str_equal ((*argv)[1], "encode"))
    {
      action = ENCODE;
      g_set_prgname ("ogmrip encode");
      g_option_context_add_main_entries (context, encode_opts, GETTEXT_PACKAGE);
    }
  }

  retval = g_option_context_parse (context, &argc, argv, &error);

  if (!retval)
  {
    g_printerr (_("Option parsing failed: %s"), error->message);
    g_printerr ("\n");
    g_error_free (error);
    *status = 1;

    goto cleanup;
  }

  if (action == NONE)
  {
    ogmrip_cli_usage (NULL);

    *status = help ? 0 : 1;

    goto cleanup;
  }

  if (help)
  {
    ogmrip_cli_usage (context);

    *status = 0;

    goto cleanup;
  }

  if (!input)
  {
    g_printerr ("%s\n", _("No media specified"));

    *status = 1;

    goto cleanup;
  }

  if (action == ENCODE && !output)
  {
    g_printerr ("%s\n", _("No output file speficied"));

    *status = 1;

    goto cleanup;
  }

  if (all)
    video = audio = chapters = angles = palette = subp = TRUE;

  if (!g_application_register (cli, NULL, &error))
  {
    g_critical ("%s", error->message);
    g_error_free (error);
    *status = 1;

    goto cleanup;
  }

  g_application_activate (cli);

  if (debug)
    ogmrip_log_set_print_stdout (TRUE);

  *status = 0;

cleanup:
  g_option_context_free (context);

  return TRUE;
}

static void
ogmrip_cli_class_init (OGMRipCliClass *klass)
{
  GObjectClass *gobject_class;
  GApplicationClass *cli_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_cli_dispose;

  cli_class = G_APPLICATION_CLASS (klass);
  cli_class->local_command_line = ogmrip_cli_local_cmdline;

  g_type_class_add_private (klass, sizeof (OGMRipCliPriv));
}

static void
ogmrip_cli_init (OGMRipCli *app)
{
  app->priv = OGMRIP_CLI_GET_PRIVATE (app);

  app->priv->cancellable = g_cancellable_new ();
}

GApplication *
ogmrip_cli_new (const gchar *app_id)
{
  GApplication *app;

  g_return_val_if_fail (g_application_id_is_valid (app_id), NULL);

  g_type_init ();

  app = g_object_new (OGMRIP_TYPE_CLI,
      "application-id", app_id,
      NULL);

  g_signal_connect (app, "activate",
      G_CALLBACK (ogmrip_cli_activate_cb), NULL);

  return app;
}

void
ogmrip_cli_cancel (OGMRipCli *cli)
{
  g_return_if_fail (OGMRIP_IS_CLI (cli));

  if (cli->priv->cancellable)
    g_cancellable_cancel (cli->priv->cancellable);
}

