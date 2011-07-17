/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-gtk.h"

#include "ogmrip-profiles.h"
#include "ogmrip-settings.h"

#include "ogmrip-options-dialog.h"
#include "ogmrip-pref-dialog.h"
#include "ogmrip-profiles-dialog.h"
#include "ogmrip-progress-dialog.h"
#include "ogmrip-queue-dialog.h"
#include "ogmrip-update-dialog.h"

#include "ogmrip-audio-options.h"
#include "ogmrip-subp-options.h"

#ifdef HAVE_ENCHANT_SUPPORT
#include "ogmrip-spell-dialog.h"
#endif /* HAVE_ENCHANT_SUPPORT */

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <libxml/tree.h>

#include <stdlib.h>

#ifdef HAVE_LIBNOTIFY_SUPPORT
#include <libnotify/notify.h>
#endif /* HAVE_LIBNOTIFY_SUPPORT */

#ifdef HAVE_DBUS_SUPPORT
#include <gdk/gdkx.h>
#include <dbus/dbus-glib.h>
#endif /* HAVE_DBUS_SUPPORT */

#define OGMRIP_UI_FILE      "ogmrip"  G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-ui.xml"
#define OGMRIP_GLADE_FILE   "ogmrip"  G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-main.glade"
#define OGMRIP_ICON_FILE    "pixmaps" G_DIR_SEPARATOR_S "ogmrip.png"

#define OGMRIP_DEFAULT_FILE_NAME "movie"

typedef struct
{
  OGMDvdDisc *disc;

  GtkWidget *window;

  GtkWidget *pref_dialog;
  GtkWidget *options_dialog;
  GtkWidget *progress_dialog;
  GtkWidget *profiles_dialog;
  GtkWidget *queue_dialog;

  GtkWidget *title_entry;
  GtkWidget *title_chooser;
  GtkWidget *length_label;
  GtkWidget *relative_check;
  GtkWidget *angle_spin;

  GtkWidget *audio_list;
  GtkWidget *subp_list;

  GtkWidget *chapter_list;

  GtkWidget *play_button;
  GtkAction *extract_action;
  GtkWidget *extract_button;
  GtkAction *import_chap_action;
  GtkAction *export_chap_action;

  gboolean encoding;

  GString *warnings;
} OGMRipData;

extern GSettings *settings;

static void ogmrip_main_load (OGMRipData *data, const gchar *path);

static int
ogmrip_main_load_logfile (GtkTextBuffer *buffer, const gchar *filename)
{
  FILE *f;
  gchar text[1024];

  GtkTextIter iter;

  f = fopen (filename, "r");
  if (!f)
    return -1;

  gtk_text_buffer_get_start_iter (buffer, &iter);

  while (!feof (f))
  {
    size_t n;

    n = fread (text, 1, 1024, f);
    if (ferror (f))
    {
      fclose (f);
      return -1;
    }

    gtk_text_buffer_insert (buffer, &iter, text, n);
  }

  return 0;
}

static void
ogmrip_main_error_message (OGMRipData *data, OGMRipEncoding *encoding, GError *error)
{
  GtkWidget *dialog;

  dialog = ogmrip_message_dialog_new (GTK_WINDOW (data->window), GTK_MESSAGE_ERROR, "<big><b>%s</b></big>", error->message);
  g_object_set (dialog, "use-markup", TRUE, NULL);

  if (error->domain == OGMRIP_ENCODING_ERROR)
  {
    switch (error->code)
    {
      case OGMRIP_ENCODING_ERROR_UNKNOWN:
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s\n\n%s",
            _("Please, check http://ogmrip.sourceforge.net to see if this is a known issue."),
            _("You really should join the log file if you open a bug report or ask questions on the forum."));
        break;
      case OGMRIP_ENCODING_ERROR_CONTAINER:
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
            _("Please, choose some others."));
      case OGMRIP_ENCODING_ERROR_AUDIO:
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
            _("Please, choose one audio stream only."));
        break;
      case OGMRIP_ENCODING_ERROR_SUBP:
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
            _("Please, choose one subtitles stream only."));
        break;
      case OGMRIP_ENCODING_ERROR_FATAL:
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s\n%s",
            _("This is an unexpected error and should not happen."),
            _("Please, fill a bug report at http://ogmrip.sourceforge.net."));
        break;
      default:
        break;
    }
  }

  if (encoding)
  {
    const gchar *logfile;

    logfile = ogmrip_encoding_get_logfile (encoding);
    if (logfile && g_file_test (logfile, G_FILE_TEST_EXISTS))
    {
      GtkWidget *area, *expander, *swin, *textview;

      area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

      expander = gtk_expander_new_with_mnemonic ("_Show the log file");
      gtk_expander_set_expanded (GTK_EXPANDER (expander), FALSE);
      gtk_container_add (GTK_CONTAINER (area), expander);
      gtk_widget_show (expander);

      swin = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
      gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (swin), GTK_SHADOW_IN);
      gtk_container_add (GTK_CONTAINER (expander), swin);
      gtk_widget_show (swin);

      textview = gtk_text_view_new ();
      gtk_text_view_set_editable (GTK_TEXT_VIEW (textview), FALSE);
      gtk_container_add (GTK_CONTAINER (swin), textview);
      gtk_widget_show (textview);

      gtk_widget_set_size_request (textview, 700, 400);

      ogmrip_main_load_logfile (gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview)), logfile);
    }
  }

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

#ifdef HAVE_ENCHANT_SUPPORT
/*
 * Performs spell checking
 */
static gboolean
ogmrip_main_spell_check (OGMRipData *data, const gchar *filename, gint lang)
{
  gboolean retval = FALSE;
  gchar *text, *corrected;
  gchar *new_file = NULL;

  GtkWidget *checker = NULL;
  GIOChannel *input = NULL, *output = NULL;
  GIOStatus status = G_IO_STATUS_NORMAL;

  checker = ogmrip_spell_dialog_new (ogmdvd_get_language_iso639_1 (lang));
  if (!checker)
  {
    ogmrip_message_dialog (GTK_WINDOW (data->window), GTK_MESSAGE_ERROR, "<big><b>%s</b></big>\n\n%s",
        _("Could not create dictionary"), _("Spell will not be checked."));
    goto spell_check_cleanup;
  }

  input = g_io_channel_new_file (filename, "r", NULL);
  if (!input)
    goto spell_check_cleanup;

  new_file = ogmrip_fs_mktemp ("sub.XXXXXX", NULL);
  if (!new_file)
    goto spell_check_cleanup;

  output = g_io_channel_new_file (new_file, "w", NULL);
  if (!output)
    goto spell_check_cleanup;

  gtk_window_set_parent (GTK_WINDOW (checker), GTK_WINDOW (data->window));
  gtk_widget_show (checker);

  do
  {
    status = g_io_channel_read_line (input, &text, NULL, NULL, NULL);
    if (status == G_IO_STATUS_NORMAL)
    {
      retval = ogmrip_spell_dialog_check_text (OGMRIP_SPELL_DIALOG (checker), text, &corrected);
      if (retval)
      {
        do
        {
          status = g_io_channel_write_chars (output, corrected ? corrected : text, -1, NULL, NULL);
        }
        while (status == G_IO_STATUS_AGAIN);

        g_free (corrected);
      }
      g_free (text);
    }
  } 
  while (retval == TRUE && (status == G_IO_STATUS_NORMAL || status == G_IO_STATUS_AGAIN));

  retval &= status == G_IO_STATUS_EOF;

spell_check_cleanup:
  if (checker)
    gtk_widget_destroy (checker);

  if (output)
  {
    g_io_channel_shutdown (output, TRUE, NULL);
    g_io_channel_unref (output);
  }

  if (input)
  {
    g_io_channel_shutdown (input, TRUE, NULL);
    g_io_channel_unref (input);
  }

  if (retval)
    retval = ogmrip_fs_rename (new_file, filename, NULL);
  else
    g_unlink (new_file);

  g_free (new_file);

  return retval;
}
#endif /* HAVE_ENCHANT_SUPPORT */

#ifdef HAVE_DBUS_SUPPORT
#define PM_DBUS_SERVICE           "org.gnome.SessionManager"
#define PM_DBUS_INHIBIT_PATH      "/org/gnome/SessionManager"
#define PM_DBUS_INHIBIT_INTERFACE "org.gnome.SessionManager"

static gint
ogmrip_main_dbus_inhibit (OGMRipData *data)
{
  GError *error = NULL;

  DBusGConnection *conn;
  DBusGProxy *proxy;
  gboolean res;
  guint cookie;

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!conn)
  {
    g_warning ("Couldn't get a DBUS connection: %s", error->message);
    g_error_free (error);
    return -1;
  }

  proxy = dbus_g_proxy_new_for_name (conn,
      PM_DBUS_SERVICE, PM_DBUS_INHIBIT_PATH, PM_DBUS_INHIBIT_INTERFACE);

  if (proxy == NULL)
  {
    g_warning ("Could not get DBUS proxy: %s", PM_DBUS_SERVICE);
    return -1;
  }

  res = dbus_g_proxy_call (proxy, "Inhibit", &error,
      G_TYPE_STRING, "Brasero",
      G_TYPE_UINT, GDK_WINDOW_XID (gtk_widget_get_window (data->window)),
      G_TYPE_STRING, "Encoding",
      G_TYPE_UINT, 1 | 4,
      G_TYPE_INVALID,
      G_TYPE_UINT, &cookie,
      G_TYPE_INVALID);

  if (!res)
  {
    g_warning ("Failed to inhibit the system from suspending: %s", error->message);
    g_error_free (error);
    cookie = -1;
  }

  g_object_unref (G_OBJECT (proxy));
  dbus_g_connection_unref (conn);

  return cookie;
}

static void
ogmrip_main_dbus_uninhibit (OGMRipData *data, guint cookie)
{
  GError *error = NULL;

  DBusGConnection *conn;
  DBusGProxy *proxy;
  gboolean res;

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!conn)
  {
    g_warning ("Couldn't get a DBUS connection: %s", error->message);
    g_error_free (error);
    return;
  }

  proxy = dbus_g_proxy_new_for_name (conn,
      PM_DBUS_SERVICE, PM_DBUS_INHIBIT_PATH, PM_DBUS_INHIBIT_INTERFACE);

  if (proxy == NULL)
  {
    g_warning ("Could not get DBUS proxy: %s", PM_DBUS_SERVICE);
    dbus_g_connection_unref (conn);
    return;
  }

  res = dbus_g_proxy_call (proxy, "Uninhibit", &error,
      G_TYPE_UINT, cookie,
      G_TYPE_INVALID,
      G_TYPE_INVALID);

  if (!res)
  {
    g_warning ("Failed to restore the system power manager: %s", error->message);
    g_error_free (error);
  }

  g_object_unref (G_OBJECT (proxy));
  dbus_g_connection_unref (conn);
}
#endif /* HAVE_DBUS_SUPPORT */

static void
ogmrip_main_append_warning (OGMRipData *data, const gchar *format, ...)
{
  va_list args;
  gchar *str;

  va_start (args, format);
  str = g_strdup_vprintf (format, args);
  va_end (args);

  if (!data->warnings)
    data->warnings = g_string_new (str);
  else
  {
    g_string_append_c (data->warnings, '\n');
    g_string_append (data->warnings, str);
  }

  g_free (str);
}

/*
 * Creates a dialog to ask what to do with a DVD backup
 */
static GtkWidget *
ogmrip_main_update_remove_dialog_new (OGMRipData *data)
{
  GtkWidget *dialog, *button, *image;

  dialog = gtk_message_dialog_new (GTK_WINDOW (data->window), 
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
      _("Do you want to remove the copy of the DVD,\n"
        "keep it on the hard drive, or\n"
        "keep it and update the GUI ?"));
  gtk_window_set_icon_from_stock (GTK_WINDOW (dialog), GTK_STOCK_DIALOG_QUESTION);

  button = gtk_button_new_with_mnemonic (_("_Remove"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, OGMRIP_AFTER_ENC_REMOVE);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_DELETE, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);

  button = gtk_button_new_with_mnemonic (_("_Keep"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, OGMRIP_AFTER_ENC_KEEP);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_SAVE, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);

  button = gtk_button_new_with_mnemonic (_("_Update"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, OGMRIP_AFTER_ENC_UPDATE);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  return dialog;
}

/*
 * Logs a profile entry
 */
static void
ogmrip_main_dump_profile_entry (OGMRipProfile *profile, gchar *key)
{
  GVariant *variant;

  variant = g_settings_get_value (G_SETTINGS (profile), key);
/*
  if (!g_str_equal (key, "name"))
  {
    switch (value.g_type)
    {
      case G_TYPE_INT:
        ogmjob_log_printf ("%s = %d\n", key, g_value_get_int (&value));
        break;
      case G_TYPE_UINT:
        ogmjob_log_printf ("%s = %u\n", key, g_value_get_uint (&value));
        break;
      case G_TYPE_LONG:
        ogmjob_log_printf ("%s = %ld\n", key, g_value_get_long (&value));
        break;
      case G_TYPE_ULONG:
        ogmjob_log_printf ("%s = %lu\n", key, g_value_get_ulong (&value));
        break;
      case G_TYPE_FLOAT:
        ogmjob_log_printf ("%s = %f\n", key, g_value_get_float (&value));
        break;
      case G_TYPE_DOUBLE:
        ogmjob_log_printf ("%s = %lf\n", key, g_value_get_double (&value));
        break;
      case G_TYPE_STRING:
        ogmjob_log_printf ("%s = %s\n", key, g_value_get_string (&value));
        break;
      case G_TYPE_BOOLEAN:
        ogmjob_log_printf ("%s = %s\n", key, g_value_get_boolean (&value) ? "true" : "false");
        break;
      default:
        g_warning ("Unknown type %s", g_type_name (value.g_type));
        break;
    }
  }
*/
  g_variant_unref (variant);

  g_free (key);
}

/*
 * Logs the profile
 */
static void
ogmrip_main_dump_profile (OGMRipProfile *profile)
{
  gchar **keys;
  guint i;

  /*
   * TODO
   */
  ogmjob_log_printf ("\nProfile: %s\n", "toto" /*profile*/);
  ogmjob_log_printf ("--------\n\n");

  keys = g_settings_list_keys (G_SETTINGS (profile));
  for (i = 0; keys[i]; i ++)
    ogmrip_main_dump_profile_entry (profile, keys[i]);
  g_free (keys);

  ogmjob_log_printf ("\n");
}

/*
 * Add audio streams and files to the encoding
 */
static gboolean
ogmrip_main_add_encoding_audio (OGMRipData *data, OGMRipEncoding *encoding, GtkWidget *chooser, GError **error)
{
  OGMRipSource *source;
  gint type;

  source = ogmrip_source_chooser_get_active (OGMRIP_SOURCE_CHOOSER (chooser), &type);

  if (type == OGMRIP_SOURCE_FILE)
    return ogmrip_encoding_add_audio_file (encoding, OGMRIP_FILE (source), error);

  if (type == OGMRIP_SOURCE_STREAM)
  {
    OGMRipAudioOptions *options;
    gboolean new_options;

    options = g_object_get_data (G_OBJECT (chooser), "__audio_options__");
    new_options = options == NULL;

    if (new_options)
    {
      options = g_new0 (OGMRipAudioOptions, 1);
      ogmrip_audio_options_init (options);

      options->language = ogmdvd_audio_stream_get_language (OGMDVD_AUDIO_STREAM (source));
    }

    if (options->defaults)
    {
      OGMRipProfile *profile;

      profile = ogmrip_encoding_get_profile (encoding);

      options->codec = ogmrip_profile_get_audio_codec_type (profile, NULL);
      ogmrip_profile_get (profile, OGMRIP_PROFILE_AUDIO, OGMRIP_PROFILE_CHANNELS, "u", &options->channels);
      ogmrip_profile_get (profile, OGMRIP_PROFILE_AUDIO, OGMRIP_PROFILE_NORMALIZE, "b", &options->normalize);
      ogmrip_profile_get (profile, OGMRIP_PROFILE_AUDIO, OGMRIP_PROFILE_QUALITY, "u", &options->quality);
      ogmrip_profile_get (profile, OGMRIP_PROFILE_AUDIO, OGMRIP_PROFILE_SAMPLERATE, "u", &options->srate);
    }

    if (!ogmrip_encoding_add_audio_stream (encoding, OGMDVD_AUDIO_STREAM (source), options, error))
    {
      if (new_options)
      {
        ogmrip_audio_options_reset (options);
        g_free (options);
      }

      return FALSE;
    }

    return TRUE;
  }

  return FALSE;
}

/*
 * Add subp streams and files to the encoding
 */
static gboolean
ogmrip_main_add_encoding_subp (OGMRipData *data, OGMRipEncoding *encoding, GtkWidget *chooser, GError **error)
{
  OGMRipSource *source;
  gint type;

  source = ogmrip_source_chooser_get_active (OGMRIP_SOURCE_CHOOSER (chooser), &type);

  if (type == OGMRIP_SOURCE_FILE)
    return ogmrip_encoding_add_subp_file (encoding, OGMRIP_FILE (source), error);

  if (type == OGMRIP_SOURCE_STREAM)
  {
    OGMRipSubpOptions *options;
    gboolean new_options;

    options = g_object_get_data (G_OBJECT (chooser), "__subp_options__");
    new_options = options == NULL;

    if (new_options)
    {
      options = g_new0 (OGMRipSubpOptions, 1);
      ogmrip_subp_options_init (options);

      options->language = ogmdvd_subp_stream_get_language (OGMDVD_SUBP_STREAM (source));
    }

    if (options->defaults)
    {
      OGMRipProfile *profile;

      profile = ogmrip_encoding_get_profile (encoding);

      options->codec = ogmrip_profile_get_subp_codec_type (profile, NULL);
      ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_FORCED_ONLY, "b", &options->forced_subs);
      ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_SPELL_CHECK, "b", &options->spell);
      ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_CHARACTER_SET, "u", &options->charset);
      ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_NEWLINE_STYLE, "u", &options->newline);
    }

    if (!ogmrip_encoding_add_subp_stream (encoding, OGMDVD_SUBP_STREAM (source), options, error))
    {
      if (new_options)
      {
        ogmrip_subp_options_reset (options);
        g_free (options);
      }

      return FALSE;
    }

    return TRUE;
  }

  return FALSE;
}

static gchar *
ogmrip_main_encoding_get_filename (OGMRipEncoding *encoding, const gchar *outdir, gint format)
{
  GType container_type = G_TYPE_NONE, video_codec_type = G_TYPE_NONE, audio_codec_type = G_TYPE_NONE;
  gchar basename[FILENAME_MAX], utf8name[FILENAME_MAX];
  gchar *filename, *ext;

  OGMRipAudioOptions options;
  const gchar *label, *lang = "Undetermined";

  if (ogmrip_encoding_get_nth_audio_options (encoding, 0, &options))
  {
    lang = ogmdvd_get_language_label (options.language);
    audio_codec_type = options.codec;

    ogmrip_audio_options_reset (&options);
  }
  else
  {
    OGMRipFile *file;

    file = ogmrip_encoding_get_nth_audio_file (encoding, 0);
    if (file)
      lang = ogmdvd_get_language_label (ogmrip_file_get_language (file));
  }

  container_type = ogmrip_encoding_get_container_type (encoding);
  video_codec_type = ogmrip_encoding_get_video_codec_type (encoding);
  label = ogmrip_encoding_get_label (encoding);

  if (video_codec_type == G_TYPE_NONE)
    format = 1;

  switch (format)
  {
    case 0:
      strncpy (basename, label, FILENAME_MAX);
      break;
    case 1:
      snprintf (basename, FILENAME_MAX, "%s - %s", label, lang);
      break;
    case 2:
      snprintf (basename, FILENAME_MAX, "%s - %s - %s", label, lang,
          ogmrip_plugin_get_video_codec_name (video_codec_type));
      break;
    case 3:
      if (audio_codec_type != G_TYPE_NONE)
        snprintf (basename, FILENAME_MAX, "%s - %s - %s %s", label, lang,
            ogmrip_plugin_get_video_codec_name (video_codec_type),
            ogmrip_plugin_get_audio_codec_name (audio_codec_type));
      else
        snprintf (basename, FILENAME_MAX, "%s - %s - %s", label, lang,
            ogmrip_plugin_get_video_codec_name (video_codec_type));
      break;
    default:
      strncpy (basename, OGMRIP_DEFAULT_FILE_NAME, FILENAME_MAX);
      break;
  }

  filename = g_build_filename (outdir, basename, NULL);

  ext = g_utf8_strdown (ogmrip_plugin_get_container_name (container_type), -1);
  snprintf (utf8name, FILENAME_MAX, "%s.%s", filename, ext);
  g_free (ext);

  g_free (filename);

  filename = g_filename_from_utf8 (utf8name, -1, NULL, NULL, NULL);

  return filename;
}

static void
ogmrip_main_set_encoding_filename (OGMRipEncoding *encoding)
{
  gchar *filename, *outdir;
  gint format;

  outdir = g_settings_get_string (settings, OGMRIP_SETTINGS_OUTPUT_DIR);
  g_settings_get (settings, OGMRIP_SETTINGS_FILENAME, "u", &format);

  filename = ogmrip_main_encoding_get_filename (encoding, outdir, format);
  g_free (outdir);

  ogmrip_encoding_set_filename (encoding, filename);
  g_free (filename);
}

static void
ogmrip_main_encoding_run (OGMRipData *data, OGMRipEncoding *encoding)
{
  OGMRipProfile *profile;

  static const gchar *fourcc[] =
  {
    NULL,
    "XVID",
    "DIVX",
    "DX50",
    "FMP4"
  };

  profile = ogmrip_encoding_get_profile (encoding);
  if (profile)
  {
    gint fcc;

    ogmrip_main_dump_profile (profile);
/*
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "chapters-lang", OGMRIP_GCONF_GENERAL, OGMRIP_GCONF_CHAPTER_LANG);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "bpp", profile, OGMRIP_GCONF_VIDEO_BPP);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "deblock", profile, OGMRIP_GCONF_VIDEO_DEBLOCK);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "dering", profile, OGMRIP_GCONF_VIDEO_DERING);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "denoise", profile, OGMRIP_GCONF_VIDEO_DENOISE);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "expand", profile, OGMRIP_GCONF_VIDEO_EXPAND);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "max-width", profile, OGMRIP_GCONF_VIDEO_MAX_WIDTH);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "max-height", profile, OGMRIP_GCONF_VIDEO_MAX_HEIGHT);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "min-width", profile, OGMRIP_GCONF_VIDEO_MIN_WIDTH);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "min-height", profile, OGMRIP_GCONF_VIDEO_MIN_HEIGHT);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "passes", profile, OGMRIP_GCONF_VIDEO_PASSES);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "preset", profile, OGMRIP_GCONF_VIDEO_PRESET);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "qpel", profile, OGMRIP_GCONF_VIDEO_QPEL);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "scaler", profile, OGMRIP_GCONF_VIDEO_SCALER);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "trellis", profile, OGMRIP_GCONF_VIDEO_TRELLIS);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "turbo", profile, OGMRIP_GCONF_VIDEO_TURBO);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "can-crop", profile, OGMRIP_GCONF_VIDEO_CAN_CROP);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "can-scale", profile, OGMRIP_GCONF_VIDEO_CAN_SCALE);
*/
    ogmrip_profile_get (profile, OGMRIP_PROFILE_GENERAL, OGMRIP_PROFILE_FOURCC, "b", &fcc);
    ogmrip_encoding_set_fourcc (encoding, fcc >= 0 && fcc <= 4 ? fourcc[fcc] : NULL);
  }

  ogmrip_progress_dialog_set_encoding (OGMRIP_PROGRESS_DIALOG (data->progress_dialog), encoding);
}

static void
ogmrip_main_encoding_completed (OGMRipData *data, OGMJobResultType result, OGMRipEncoding *encoding)
{
  gboolean log_output = FALSE;

  log_output = g_settings_get_boolean (settings, OGMRIP_SETTINGS_LOG_OUTPUT);

  if (result != OGMJOB_RESULT_ERROR && !log_output)
  {
    const gchar *logfile;

    logfile = ogmrip_encoding_get_logfile (encoding);
    if (logfile && g_file_test (logfile, G_FILE_TEST_EXISTS))
      g_unlink (logfile);
  }
}

static void
ogmrip_main_encoding_task_completed (OGMRipData *data, OGMRipEncodingTask *task, OGMRipEncoding *encoding)
{
  if (task->detail.result == OGMJOB_RESULT_SUCCESS)
  {
    if (task->type == OGMRIP_TASK_AUDIO)
    {
      OGMDvdAudioStream *stream;
      const gchar *output;
      struct stat buf;

      stream = ogmrip_audio_codec_get_dvd_audio_stream (OGMRIP_AUDIO_CODEC (task->spawn));

      output = ogmrip_codec_get_output (OGMRIP_CODEC (task->spawn));
      if (g_stat (output, &buf) == 0 && buf.st_size == 0)
        ogmrip_main_append_warning (data, _("Audio stream %d seems to be empty. It has not been merged."),
            ogmdvd_stream_get_nr (OGMDVD_STREAM (stream)) + 1);
    }
    else if (task->type == OGMRIP_TASK_SUBP)
    {
      OGMDvdSubpStream *stream;
      const gchar *output;
      struct stat buf;

      gboolean do_merge;

      output = ogmrip_codec_get_output (OGMRIP_CODEC (task->spawn));

      if (ogmrip_plugin_get_subp_codec_format (G_OBJECT_TYPE (task->spawn)) == OGMRIP_FORMAT_VOBSUB)
      {
        gchar *filename;

        filename = g_strconcat (output, ".idx", NULL);
        do_merge = (g_stat (filename, &buf) == 0 && buf.st_size != 0);

        if (do_merge)
        {
          g_free (filename);
          filename = g_strconcat (output, ".sub", NULL);
          do_merge = (g_stat (filename, &buf) == 0 && buf.st_size != 0);
        }
        g_free (filename);
      }
      else
        do_merge = (g_stat (output, &buf) == 0 && buf.st_size != 0);

      stream = ogmrip_subp_codec_get_dvd_subp_stream (OGMRIP_SUBP_CODEC (task->spawn));

      if (!do_merge)
        ogmrip_main_append_warning (data, _("Subtitle stream %d seems to be empty. It has not been merged."),
            ogmdvd_stream_get_nr (OGMDVD_STREAM (stream)) + 1);

#ifdef HAVE_ENCHANT_SUPPORT
      if (ogmrip_plugin_get_subp_codec_text (G_OBJECT_TYPE (task->spawn)))
      {
        OGMRipSubpOptions *options = (OGMRipSubpOptions *) task->options;

        if (options && options->spell && options->language > 0)
        {
          const gchar *filename;

          filename = ogmrip_codec_get_output (OGMRIP_CODEC (task->spawn));

          ogmrip_main_spell_check (data, filename, options->language);
        }
      }
#endif /* HAVE_ENCHANT_SUPPORT */
    }
  }
}
/*
static void
ogmrip_main_encoding_options_notified (OGMRipSettings *settings, const gchar *section, const gchar *key, const GValue *value, OGMRipEncoding *encoding)
{
  GError *error = NULL;

  if (OGMRIP_ENCODING_IS_RUNNING (encoding))
    return;

  if (g_str_equal (key, OGMRIP_GCONF_KEEP_TMP))
    ogmrip_encoding_set_keep_tmp_files (encoding, g_value_get_boolean (value));
  else if (g_str_equal (key, OGMRIP_GCONF_COPY_DVD))
    ogmrip_encoding_set_copy_dvd (encoding, g_value_get_boolean (value));
  else if (g_str_equal (key, OGMRIP_GCONF_CONTAINER_ENSURE_SYNC))
    ogmrip_encoding_set_ensure_sync (encoding, g_value_get_boolean (value));
  else if (g_str_equal (key, OGMRIP_GCONF_CONTAINER_TNUMBER))
    ogmrip_encoding_set_target_number (encoding, g_value_get_int (value));
  else if (g_str_equal (key, OGMRIP_GCONF_CONTAINER_TSIZE))
    ogmrip_encoding_set_target_size (encoding, g_value_get_int (value));
  else if (g_str_equal (key, OGMRIP_GCONF_VIDEO_ENCODING))
    ogmrip_encoding_set_method (encoding, g_value_get_int (value));
  else if (g_str_equal (key, OGMRIP_GCONF_VIDEO_BITRATE))
    ogmrip_encoding_set_bitrate (encoding, g_value_get_int (value));
  else if (g_str_equal (key, OGMRIP_GCONF_VIDEO_QUANTIZER))
    ogmrip_encoding_set_quantizer (encoding, g_value_get_double (value));
  else if (g_str_equal (key, OGMRIP_GCONF_THREADS))
    ogmrip_encoding_set_threads (encoding, g_value_get_int (value));
  else if (g_str_equal (key, OGMRIP_GCONF_CONTAINER_FORMAT))
  {
    /?*
     * TODO what if the container and the codec are not compatible ?
     *?/
    if (ogmrip_encoding_set_container_type (encoding,
          ogmrip_plugin_get_container_by_name (g_value_get_string (value)), &error))
      ogmrip_main_set_encoding_filename (encoding);
    else
    {
      ogmrip_message_dialog (NULL, GTK_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }
  }
  else if (g_str_equal (key, OGMRIP_GCONF_VIDEO_CODEC))
  {
    GType old_codec;

    old_codec = ogmrip_encoding_get_video_codec_type (encoding);

    if (ogmrip_encoding_set_video_codec_type (encoding,
          ogmrip_plugin_get_video_codec_by_name (g_value_get_string (value)), &error))
      ogmrip_main_set_encoding_filename (encoding);
    else
    {
      if (old_codec != G_TYPE_NONE)
        ogmrip_settings_set (settings, section, key, ogmrip_plugin_get_video_codec_name (old_codec), NULL);

      ogmrip_message_dialog (NULL, GTK_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }
  }
  else if (g_str_equal (key, OGMRIP_GCONF_VIDEO_ASPECT))
  {
    switch (g_value_get_int (value))
    {
      case OGMDVD_DISPLAY_ASPECT_4_3:
        ogmrip_encoding_set_aspect_ratio (encoding, 4, 3);
        break;
      case OGMDVD_DISPLAY_ASPECT_16_9:
        ogmrip_encoding_set_aspect_ratio (encoding, 16, 9);
        break;
      default:
        ogmrip_encoding_set_aspect_ratio (encoding, 0, 0);
        break;
    }
  }
  else if (g_str_equal (key, OGMRIP_GCONF_OUTPUT_DIR) || g_str_equal (key, OGMRIP_GCONF_FILENAME))
    ogmrip_main_set_encoding_filename (encoding);
}

static void
ogmrip_main_encoding_audio_notified (OGMRipSettings *settings, const gchar *section, const gchar *key, const GValue *value, OGMRipEncoding *encoding)
{
  GError *error = NULL;
  OGMRipAudioOptions options;
  guint i, n;

  if (OGMRIP_ENCODING_IS_RUNNING (encoding))
    return;

  n = ogmrip_encoding_get_n_audio_streams (encoding);
  for (i = 0; i < n; i ++)
  {
    if (ogmrip_encoding_get_nth_audio_options (encoding, i, &options))
    {
      if (options.defaults)
      {
        GType old_codec = G_TYPE_NONE;

        if (g_str_equal (key, OGMRIP_GCONF_AUDIO_CHANNELS))
          options.channels = g_value_get_int (value);
        else if (g_str_equal (key, OGMRIP_GCONF_AUDIO_NORMALIZE))
          options.normalize = g_value_get_boolean (value);
        else if (g_str_equal (key, OGMRIP_GCONF_AUDIO_QUALITY))
          options.quality = g_value_get_int (value);
        else if (g_str_equal (key, OGMRIP_GCONF_AUDIO_SRATE))
          options.srate = g_value_get_int (value);
        else if (g_str_equal (key, OGMRIP_GCONF_AUDIO_CODEC))
        {
          old_codec = options.codec;
          options.codec = ogmrip_plugin_get_audio_codec_by_name (g_value_get_string (value));
        }

        if (!ogmrip_encoding_set_nth_audio_options (encoding, i, &options, &error))
        {
          if (old_codec != G_TYPE_NONE)
            ogmrip_settings_set (settings, section, key, ogmrip_plugin_get_audio_codec_name (old_codec), NULL);

          ogmrip_message_dialog (NULL, GTK_MESSAGE_ERROR, error->message);
          g_error_free (error);
          break;
        }
      }

      ogmrip_audio_options_reset (&options);
    }
  }

  ogmrip_main_set_encoding_filename (encoding);
}

static void
ogmrip_main_encoding_subp_notified (OGMRipSettings *settings, const gchar *section, const gchar *key, const GValue *value, OGMRipEncoding *encoding)
{
  GError *error = NULL;
  OGMRipSubpOptions options;
  guint i, n;

  if (OGMRIP_ENCODING_IS_RUNNING (encoding))
    return;

  n = ogmrip_encoding_get_n_subp_streams (encoding);
  for (i = 0; i < n; i ++)
  {
    if (ogmrip_encoding_get_nth_subp_options (encoding, i, &options))
    {
      if (options.defaults)
      {
        GType old_codec = G_TYPE_NONE;

        if (g_str_equal (key, OGMRIP_GCONF_FORCED_SUBS))
          options.forced_subs = g_value_get_boolean (value);
        else if (g_str_equal (key, OGMRIP_GCONF_SPELL_CHECK))
          options.spell = g_value_get_boolean (value);
        else if (g_str_equal (key, OGMRIP_GCONF_SUBP_CHARSET))
          options.charset = g_value_get_int (value);
        else if (g_str_equal (key, OGMRIP_GCONF_SUBP_NEWLINE))
          options.newline = g_value_get_int (value);
        else if (g_str_equal (key, OGMRIP_GCONF_SUBP_CODEC))
        {
          old_codec = options.codec;
          options.codec = ogmrip_plugin_get_subp_codec_by_name (g_value_get_string (value));
        }

        if (!ogmrip_encoding_set_nth_subp_options (encoding, i, &options, &error))
        {
          if (old_codec != G_TYPE_NONE)
            ogmrip_settings_set (settings, section, key, ogmrip_plugin_get_subp_codec_name (old_codec), NULL);

          ogmrip_message_dialog (NULL, GTK_MESSAGE_ERROR, error->message);
          g_error_free (error);
        }
      }

      ogmrip_subp_options_reset (&options);
    }
  }
}
*/
static void
ogmrip_main_set_encoding_notifications (OGMRipEncoding *encoding, OGMRipProfile *profile)
{
/*
  gpointer ptr;
  gulong handler = 0;

  ptr = g_object_get_data (G_OBJECT (encoding), "__ogmrip_container_handler__");

  if (ptr)
    ogmrip_settings_remove_notify (settings, (gulong) ptr);

  if (profile)
    handler = ogmrip_settings_add_notify_while_alive (settings, profile, OGMRIP_GCONF_CONTAINER,
        (OGMRipNotifyFunc) ogmrip_main_encoding_options_notified, encoding, G_OBJECT (encoding));

  g_object_set_data (G_OBJECT (encoding), "__ogmrip_container_handler__", (gpointer) handler);

  ptr = g_object_get_data (G_OBJECT (encoding), "__ogmrip_video_handler__");
  if (ptr)
    ogmrip_settings_remove_notify (settings, (gulong) ptr);

  if (profile)
    handler = ogmrip_settings_add_notify_while_alive (settings, profile, OGMRIP_GCONF_VIDEO,
        (OGMRipNotifyFunc) ogmrip_main_encoding_options_notified, encoding, G_OBJECT (encoding));

  g_object_set_data (G_OBJECT (encoding), "__ogmrip_video_handler__", (gpointer) handler);

  ptr = g_object_get_data (G_OBJECT (encoding), "__ogmrip_audio_handler__");
  if (ptr)
    ogmrip_settings_remove_notify (settings, (gulong) ptr);

  if (profile)
    handler = ogmrip_settings_add_notify_while_alive (settings, profile, OGMRIP_GCONF_AUDIO,
        (OGMRipNotifyFunc) ogmrip_main_encoding_audio_notified, encoding, G_OBJECT (encoding));

  g_object_set_data (G_OBJECT (encoding), "__ogmrip_audio_handler__", (gpointer) handler);

  ptr = g_object_get_data (G_OBJECT (encoding), "__ogmrip_subp_handler__");
  if (ptr)
    ogmrip_settings_remove_notify (settings, (gulong) ptr);

  if (profile)
    handler = ogmrip_settings_add_notify_while_alive (settings, profile, OGMRIP_GCONF_SUBP,
        (OGMRipNotifyFunc) ogmrip_main_encoding_subp_notified, encoding, G_OBJECT (encoding));

  g_object_set_data (G_OBJECT (encoding), "__ogmrip_subp_handler__", (gpointer) handler);
*/
}

gboolean
ogmrip_main_set_encoding_profile (OGMRipEncoding *encoding, OGMRipProfile *profile, GError **error)
{
  if (profile)
  {
    OGMRipAudioOptions old_audio_options, new_audio_options;
    OGMRipSubpOptions old_subp_options, new_subp_options;

    GType container_type, video_codec_type;
    gint i, n, bitrate/*, aspect*/;

    video_codec_type = ogmrip_profile_get_video_codec_type (profile, NULL);
    ogmrip_encoding_set_video_codec_type (encoding, video_codec_type, NULL);
/*
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "threads", OGMRIP_GCONF_ADVANCED, OGMRIP_GCONF_THREADS);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "keep-tmp-files", OGMRIP_GCONF_ADVANCED, OGMRIP_GCONF_KEEP_TMP);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "copy-dvd", OGMRIP_GCONF_ADVANCED, OGMRIP_GCONF_COPY_DVD);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "auto-subp", OGMRIP_GCONF_ADVANCED, OGMRIP_GCONF_AUTO_SUBP);

    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "ensure-sync", profile, OGMRIP_GCONF_CONTAINER_ENSURE_SYNC);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "target-number", profile, OGMRIP_GCONF_CONTAINER_TNUMBER);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "target-size", profile, OGMRIP_GCONF_CONTAINER_TSIZE);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "method", profile, OGMRIP_GCONF_VIDEO_ENCODING);
    ogmrip_settings_set_property_from_key (settings, G_OBJECT (encoding),
        "quantizer", profile, OGMRIP_GCONF_VIDEO_QUANTIZER);
*/
    ogmrip_profile_get (profile, OGMRIP_PROFILE_VIDEO, OGMRIP_PROFILE_BITRATE, "u", &bitrate);
/*
    ogmrip_profile_get (profile, OGMRIP_PROFILE_VIDEO, OGMRIP_PROFILE_ASPECT, "u", &aspect);
*/
    ogmrip_encoding_set_bitrate (encoding, bitrate * 1000);
/*
    switch (aspect)
    {
      case OGMDVD_DISPLAY_ASPECT_4_3:
        ogmrip_encoding_set_aspect_ratio (encoding, 4, 3);
        break;
      case OGMDVD_DISPLAY_ASPECT_16_9:
        ogmrip_encoding_set_aspect_ratio (encoding, 16, 9);
        break;
      default:
        ogmrip_encoding_set_aspect_ratio (encoding, 0, 0);
        break;
    }
*/
    n = ogmrip_encoding_get_n_audio_streams (encoding);
    for (i = 0; i < n; i ++)
    {
      if (ogmrip_encoding_get_nth_audio_options (encoding, i, &old_audio_options))
      {
        if (old_audio_options.defaults)
        {
          new_audio_options = old_audio_options;

          ogmrip_profile_get (profile, OGMRIP_PROFILE_AUDIO, OGMRIP_PROFILE_QUALITY, "u", &new_audio_options.quality);
          ogmrip_profile_get (profile, OGMRIP_PROFILE_AUDIO, OGMRIP_PROFILE_CHANNELS, "u", &new_audio_options.channels);
          ogmrip_profile_get (profile, OGMRIP_PROFILE_AUDIO, OGMRIP_PROFILE_SAMPLERATE, "u", &new_audio_options.srate);
          ogmrip_profile_get (profile, OGMRIP_PROFILE_AUDIO, OGMRIP_PROFILE_NORMALIZE, "b", &new_audio_options.normalize);

          new_audio_options.codec = ogmrip_profile_get_audio_codec_type (profile, NULL);

          if (!ogmrip_encoding_set_nth_audio_options (encoding, i, &new_audio_options, error))
            ogmrip_encoding_set_nth_audio_options (encoding, i, &old_audio_options, error);
        }

        ogmrip_audio_options_reset (&old_audio_options);
      }
    }

    n = ogmrip_encoding_get_n_subp_streams (encoding);
    for (i = 0; i < n; i ++)
    {
      if (ogmrip_encoding_get_nth_subp_options (encoding, i, &old_subp_options))
      {
        if (old_subp_options.defaults)
        {
          new_subp_options = old_subp_options;

          ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_CHARACTER_SET, "u", &new_subp_options.charset);
          ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_NEWLINE_STYLE, "u", &new_subp_options.newline);
          ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_FORCED_ONLY, "b", &new_subp_options.forced_subs);
          ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_SPELL_CHECK, "b", &new_subp_options.spell);

          new_subp_options.codec = ogmrip_profile_get_subp_codec_type (profile, NULL);


          if (!ogmrip_encoding_set_nth_subp_options (encoding, i, &new_subp_options, error))
            ogmrip_encoding_set_nth_subp_options (encoding, i, &old_subp_options, error);
        }

        ogmrip_subp_options_reset (&old_subp_options);
      }
    }

    container_type = ogmrip_profile_get_container_type (profile, NULL);
    if (!ogmrip_encoding_set_container_type (encoding, container_type, error))
      return FALSE;
  }

  ogmrip_main_set_encoding_notifications (encoding, profile);

  return TRUE;
}

/*
 * Returns a new OGMRipEncoding
 */
static OGMRipEncoding *
ogmrip_main_new_encoding (OGMRipData *data, GError **error)
{
  OGMRipEncoding *encoding;
  OGMRipProfileEngine *engine;
  OGMRipProfile *profile;
  OGMDvdTitle *title;
  GtkWidget *chooser;

  gchar *str;
  guint start_chap, i, n;
  gint end_chap;

  title = ogmdvd_title_chooser_get_active (OGMDVD_TITLE_CHOOSER (data->title_chooser));
  if (!title)
  {
    g_set_error (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_UNKNOWN, _("Could not open the DVD"));
    return NULL;
  }

  engine = ogmrip_profile_engine_get_default ();

  str = g_settings_get_string (settings, OGMRIP_SETTINGS_PROFILE);
  profile = ogmrip_profile_engine_get (engine, str);
  g_free (str);

  if (!profile)
  {
    g_set_error (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_UNKNOWN, "<b>%s</b>\n\n%s",
        _("No available profile"), _("You must create at least one profile before you can encode."));
    return NULL;
  }

  encoding = ogmrip_encoding_new (title, OGMRIP_DEFAULT_FILE_NAME ".avi");
  ogmrip_encoding_set_profile (encoding, profile);

  ogmrip_encoding_set_label (encoding, gtk_entry_get_text (GTK_ENTRY (data->title_entry)));
  ogmrip_encoding_set_angle (encoding, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->angle_spin)));

  ogmrip_chapter_list_get_selected (OGMRIP_CHAPTER_LIST (data->chapter_list), &start_chap, &end_chap);
  ogmrip_encoding_set_chapters (encoding, start_chap, end_chap);

  for (i = 0; ; i++)
  {
    str = ogmdvd_chapter_list_get_label (OGMDVD_CHAPTER_LIST (data->chapter_list), i);
    if (!str)
      break;
    ogmrip_encoding_set_chapter_label (encoding, i, str);
    g_free (str);
  }

  ogmrip_encoding_set_relative (encoding,
      gtk_widget_is_sensitive (data->relative_check) &
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->relative_check)));

  g_signal_connect_swapped (encoding, "run", G_CALLBACK (ogmrip_main_encoding_run), data);
  g_signal_connect_swapped (encoding, "complete", G_CALLBACK (ogmrip_main_encoding_completed), data);
  g_signal_connect_swapped (encoding, "task::complete", G_CALLBACK (ogmrip_main_encoding_task_completed), data);

  if (!ogmrip_main_set_encoding_profile (encoding, profile, error))
  {
    g_object_unref (encoding);
    return NULL;
  }
/*
  ogmrip_settings_add_notify_while_alive (settings, OGMRIP_GCONF_ROOT, "general",
      (OGMRipNotifyFunc) ogmrip_main_encoding_options_notified, encoding, G_OBJECT (encoding));
  ogmrip_settings_add_notify_while_alive (settings, OGMRIP_GCONF_ROOT, "advanced",
      (OGMRipNotifyFunc) ogmrip_main_encoding_options_notified, encoding, G_OBJECT (encoding));
*/
  n = ogmrip_chooser_list_length (OGMRIP_CHOOSER_LIST (data->audio_list));
  for (i = 0; i < n; i ++)
  {
    chooser = ogmrip_chooser_list_nth (OGMRIP_CHOOSER_LIST (data->audio_list), i);
    if (!ogmrip_main_add_encoding_audio (data, encoding, chooser, error))
    {
      g_object_unref (encoding);
      return NULL;
    }
  }

  n = ogmrip_chooser_list_length (OGMRIP_CHOOSER_LIST (data->subp_list));
  for (i = 0; i < n; i ++)
  {
    chooser = ogmrip_chooser_list_nth (OGMRIP_CHOOSER_LIST (data->subp_list), i);
    if (!ogmrip_main_add_encoding_subp (data, encoding, chooser, error))
    {
      g_object_unref (encoding);
      return NULL;
    }
  }

  ogmrip_main_set_encoding_filename (encoding);

  return encoding;
}

static gboolean
ogmrip_main_check_encoding (OGMRipEncoding *encoding, OGMRipData *data)
{
  if (!ogmrip_encoding_check_filename (encoding, NULL))
  {
    const gchar *filename;
    gint response;

    filename = ogmrip_encoding_get_filename (encoding);

    response = ogmrip_message_dialog (GTK_WINDOW (data->window), GTK_MESSAGE_QUESTION,
        _("A file named '%s' already exists.\nDo you want to replace it?"), filename);

    if (response == GTK_RESPONSE_NO)
      return FALSE;

    g_unlink (filename);
  }

  return TRUE;
}

static void ogmrip_main_progress_dialog_construct (OGMRipData *data);

/*
 * Tests an encoding
 */
/*
static gboolean
ogmrip_main_test_encoding (OGMRipEncoding *encoding, OGMRipData *data)
{
  GError *error = NULL;
  GtkWidget *dialog = NULL;
  gint result = OGMJOB_RESULT_ERROR;

  ogmrip_main_progress_dialog_construct (data);
  ogmrip_progress_dialog_can_quit (OGMRIP_PROGRESS_DIALOG (data->progress_dialog), FALSE);
  gtk_widget_show (data->progress_dialog);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (data->queue_dialog), GTK_RESPONSE_ACCEPT, FALSE);

  ogmrip_options_dialog_set_response_sensitive (OGMRIP_OPTIONS_DIALOG (data->options_dialog), OGMRIP_RESPONSE_EXTRACT, FALSE);
  ogmrip_options_dialog_set_response_sensitive (OGMRIP_OPTIONS_DIALOG (data->options_dialog), OGMRIP_RESPONSE_TEST, FALSE);

  while (TRUE)
  {
    result = ogmrip_encoding_test (encoding, &error);
    if (result != OGMJOB_RESULT_ERROR)
      break;

    if (!error)
      break;

    if (!g_error_matches (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_ID))
      break;

    g_clear_error (&error);

    if (!dialog)
      dialog = ogmrip_load_dvd_dialog_new (GTK_WINDOW (data->progress_dialog),
          ogmdvd_title_get_disc (ogmrip_encoding_get_title (encoding)), ogmrip_encoding_get_label (encoding), FALSE);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_ACCEPT)
    {
      result = OGMJOB_RESULT_CANCEL;
      break;
    }
  }

  if (dialog)
    gtk_widget_destroy (dialog);

  if (result == OGMJOB_RESULT_SUCCESS)
  {
    guint scale_w, scale_h;

    ogmrip_encoding_get_scale (encoding, &scale_w, &scale_h);
    ogmrip_options_dialog_set_scale (OGMRIP_OPTIONS_DIALOG (data->options_dialog), OGMRIP_OPTIONS_MANUAL, scale_w, scale_h);
    gtk_window_present (GTK_WINDOW (data->options_dialog));
  }

  gtk_dialog_set_response_sensitive (GTK_DIALOG (data->queue_dialog), GTK_RESPONSE_ACCEPT, TRUE);

  ogmrip_options_dialog_set_response_sensitive (OGMRIP_OPTIONS_DIALOG (data->options_dialog), OGMRIP_RESPONSE_EXTRACT, TRUE);
  ogmrip_options_dialog_set_response_sensitive (OGMRIP_OPTIONS_DIALOG (data->options_dialog), OGMRIP_RESPONSE_TEST, TRUE);

  if (data->progress_dialog)
    gtk_widget_destroy (data->progress_dialog);
  data->progress_dialog = NULL;

  if (result == OGMJOB_RESULT_ERROR || error)
  {
    if (!error)
      error = g_error_new (OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_UNKNOWN, _("Unknown error"));

    ogmrip_main_error_message (data, encoding, error);
    g_error_free (error);

    return FALSE;
  }

  if (result == OGMJOB_RESULT_SUCCESS)
    ogmrip_message_dialog (GTK_WINDOW (data->options_dialog), GTK_MESSAGE_INFO,
        "<big><b>%s</b></big>\n\n%s", _("The compressibility test completed successfully."),
        _("The scaling parameters have been adjusted to optimize the quality."));

  return result == OGMJOB_RESULT_SUCCESS;
}
*/
/*
 * Loads a DVD disk or a DVD structure
 */
static void
ogmrip_main_load (OGMRipData *data, const gchar *path)
{
  OGMDvdDisc *disc;
  GError *error = NULL;

  disc = ogmdvd_disc_new (path, &error);
  if (!disc)
  {
    if (error)
    {
      ogmrip_message_dialog (GTK_WINDOW (data->window), GTK_MESSAGE_ERROR, "<big><b>%s</b></big>\n\n%s", 
          _("Could not open the DVD"), _(error->message));
      g_error_free (error);
    }
    else
      ogmrip_message_dialog (GTK_WINDOW (data->window), GTK_MESSAGE_ERROR, "<big><b>%s</b></big>\n\n%s",
          _("Could not read the DVD"), _("Unknown error"));
  }
  else
  {
    const gchar *label = NULL;
    gint nvid;

    if (data->disc)
      ogmdvd_disc_unref (data->disc);
    data->disc = disc;

    ogmdvd_title_chooser_set_disc (OGMDVD_TITLE_CHOOSER (data->title_chooser), disc);

    nvid = ogmdvd_disc_get_n_titles (disc);
    if (nvid > 0)
      label = ogmdvd_disc_get_label (disc);

    gtk_widget_set_sensitive (data->title_chooser, nvid > 0);
    gtk_widget_set_sensitive (data->title_entry, nvid > 0);
    gtk_widget_set_sensitive (data->play_button, nvid > 0);
    gtk_widget_set_sensitive (data->extract_button, nvid > 0);
    gtk_action_set_sensitive (data->extract_action, nvid > 0);
    gtk_action_set_sensitive (data->import_chap_action, nvid > 0);
    gtk_action_set_sensitive (data->export_chap_action, nvid > 0);

    gtk_entry_set_text (GTK_ENTRY (data->title_entry), label ? label : "");
  }
}

static void
ogmrip_main_load_from_encoding (OGMRipData *data, OGMRipEncoding *encoding)
{
  OGMDvdTitle *title;

  title = ogmrip_encoding_get_title (encoding);
  ogmrip_main_load (data, ogmdvd_disc_get_device (ogmdvd_title_get_disc (title)));
}

/*
 * Opens a simple chapter file
 */
gboolean
ogmrip_main_import_simple_chapters (OGMRipData *data, const gchar *filename)
{
  FILE *file;
  gchar buf[201], *str;
  gint chap;

  file = fopen (filename, "r");
  if (!file)
    return FALSE;

  while (!feof (file))
  {
    if (fgets (buf, 200, file) != NULL)
    {
      if (sscanf (buf, "CHAPTER%02dNAME=", &chap) == 1 && chap > 0)
      {
        str = g_strstrip (strchr (buf, '='));
        ogmdvd_chapter_list_set_label (OGMDVD_CHAPTER_LIST (data->chapter_list), chap - 1, str + 1);
      }
    }
  }

  fclose (file);

  return TRUE;
}

/*
 * Opens a matroska chapter file
 */
static gboolean
ogmrip_main_import_matroska_chapters (OGMRipData *data, const gchar *filename)
{
  xmlDoc *doc;
  xmlNode *node, *child;
  xmlChar *str;

  gint chap = 0;

  doc = xmlParseFile (filename);
  if (!doc)
    return FALSE;

  node = xmlDocGetRootElement (doc);
  if (!node)
  {
    xmlFreeDoc (doc);
    return FALSE;
  }

  if (!xmlStrEqual (node->name, (xmlChar *) "Chapters"))
  {
    xmlFreeDoc (doc);
    return FALSE;
  }

  for (node = node->children; node; node = node->next)
    if (xmlStrEqual (node->name, (xmlChar *) "EditionEntry"))
      break;

  if (!node)
  {
    xmlFreeDoc (doc);
    return FALSE;
  }

  for (node = node->children; node; node = node->next)
  {
    if (xmlStrEqual (node->name, (xmlChar *) "ChapterAtom"))
    {
      for (child = node->children; child; child = child->next)
        if (xmlStrEqual (child->name, (xmlChar *) "ChapterDisplay"))
          break;

      if (child)
      {
        for (child = child->children; child; child = child->next)
          if (xmlStrEqual (child->name, (xmlChar *) "ChapterString"))
            break;

        if (child)
        {
          str = xmlNodeGetContent (child);
          ogmdvd_chapter_list_set_label (OGMDVD_CHAPTER_LIST (data->chapter_list), chap, (gchar *) str);
          chap ++;
        }
      }
    }
  }

  xmlFreeDoc (doc);

  return TRUE;
}

/*
 * Saves chapter info in simple format
 */
void
ogmrip_main_export_simple_chapters (OGMRipData *data, const gchar *filename)
{
  guint start_chap;
  gint end_chap;

  if (ogmrip_chapter_list_get_selected (OGMRIP_CHAPTER_LIST (data->chapter_list), &start_chap, &end_chap))
  {
    OGMDvdTitle *title;
    OGMJobSpawn *spawn;
    GError *error = NULL;

    gchar *label;
    gint i;

    title = ogmdvd_title_chooser_get_active (OGMDVD_TITLE_CHOOSER (data->title_chooser));
    spawn = ogmrip_chapters_new (title, filename);
    ogmdvd_title_unref (title);

    ogmrip_codec_set_chapters (OGMRIP_CODEC (spawn), start_chap, end_chap);

    for (i = 0; ; i++)
    {
      label = ogmdvd_chapter_list_get_label (OGMDVD_CHAPTER_LIST (data->chapter_list), i);
      if (!label)
        break;
      ogmrip_chapters_set_label (OGMRIP_CHAPTERS (spawn), i, label);
      g_free (label);
    }

    ogmrip_codec_set_unlink_on_unref (OGMRIP_CODEC (spawn), FALSE);
    ogmrip_codec_set_chapters (OGMRIP_CODEC (spawn), start_chap, end_chap);

    if (ogmjob_spawn_run (spawn, &error) != OGMJOB_RESULT_SUCCESS)
    {
      if (!error)
        error = g_error_new (OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_UNKNOWN,
            _("Unknown error while exporting the chapters"));

      ogmrip_main_error_message (data, NULL, error);
      g_error_free (error);
/*
      if (error)
      {
        ogmrip_message_dialog (GTK_WINDOW (data->window), GTK_MESSAGE_ERROR, "<b>%s</b>\n\n%s",
            _("Could not export the chapters"), error->message);
        g_error_free (error);
      }
      else
        ogmrip_message_dialog (GTK_WINDOW (data->window), GTK_MESSAGE_ERROR,
            "<b>%s</b>\n\n%s", _("Unknown error while exporting the chapters"),
            _("Please, check http://ogmrip.sf.net to see if this is a known issue."));
*/
    }
  }
}

/*
 * Clear the GUI
 */
static void
ogmrip_main_clear (OGMRipData *data)
{
  ogmdvd_title_chooser_set_disc (OGMDVD_TITLE_CHOOSER (data->title_chooser), NULL);

  gtk_widget_set_sensitive (data->title_chooser, FALSE);
  gtk_widget_set_sensitive (data->title_entry, FALSE);
  gtk_widget_set_sensitive (data->play_button, FALSE);
  gtk_widget_set_sensitive (data->extract_button, FALSE);
  gtk_action_set_sensitive (data->extract_action, FALSE);
  gtk_action_set_sensitive (data->import_chap_action, FALSE);
  gtk_action_set_sensitive (data->export_chap_action, FALSE);

  gtk_entry_set_text (GTK_ENTRY (data->title_entry), "");
}

/*
 * Events
 */

static void
ogmrip_main_pref_dialog_construct (OGMRipData *data)
{
  data->pref_dialog = ogmrip_pref_dialog_new ();
  gtk_window_set_parent (GTK_WINDOW (data->pref_dialog), GTK_WINDOW (data->window));

  g_signal_connect (data->pref_dialog, "destroy", G_CALLBACK (gtk_widget_destroyed), &data->pref_dialog);
  g_signal_connect (data->pref_dialog, "response", G_CALLBACK (gtk_widget_hide), NULL);
  g_signal_connect (data->pref_dialog, "delete-event", G_CALLBACK (gtk_true), NULL);
}
/*
static void ogmrip_main_options_dialog_edit_clicked (OGMRipData *data);
static void ogmrip_main_options_dialog_profile_changed (OGMRipData *data);
static void ogmrip_main_options_dialog_responsed    (OGMRipData *data,
                                                     gint       response);
*/
static void ogmrip_main_chooser_list_changed        (OGMRipData *data);

static void
ogmrip_main_options_dialog_construct (OGMRipData *data)
{
/*
  data->options_dialog = ogmrip_options_dialog_new (OGMRIP_OPTIONS_DIALOG_CREATE);

  gtk_window_set_parent (GTK_WINDOW (data->options_dialog), GTK_WINDOW (data->window));

  g_signal_connect (data->options_dialog, "destroy",
      G_CALLBACK (gtk_widget_destroyed), &data->options_dialog);
  g_signal_connect (data->options_dialog, "delete-event",
      G_CALLBACK (gtk_true), NULL);

  g_signal_connect_swapped (data->options_dialog, "edit-clicked",
      G_CALLBACK (ogmrip_main_options_dialog_edit_clicked), data);
  g_signal_connect_swapped (data->options_dialog, "profile-changed",
      G_CALLBACK (ogmrip_main_options_dialog_profile_changed), data);
  g_signal_connect_swapped (data->options_dialog, "response",
      G_CALLBACK (ogmrip_main_options_dialog_responsed), data);

  g_signal_connect_swapped (data->audio_list, "add",
      G_CALLBACK (ogmrip_main_chooser_list_changed), data);
  g_signal_connect_swapped (data->audio_list, "remove",
      G_CALLBACK (ogmrip_main_chooser_list_changed), data);

  g_signal_connect_swapped (data->subp_list, "add",
      G_CALLBACK (ogmrip_main_chooser_list_changed), data);
  g_signal_connect_swapped (data->subp_list, "remove",
      G_CALLBACK (ogmrip_main_chooser_list_changed), data);
*/
}
/*
static void ogmrip_main_profiles_dialog_new_profile    (OGMRipData    *data,
                                                        OGMRipProfile *profile,
                                                        const gchar   *name);
static void ogmrip_main_profiles_dialog_remove_profile (OGMRipData    *data,
                                                        OGMRipProfile *profile,
                                                        const gchar   *name);
static void ogmrip_main_profiles_dialog_rename_profile (OGMRipData    *data,
                                                        OGMRipProfile *profile,
                                                        const gchar   *name);
*/
static void
ogmrip_main_profiles_dialog_construct (OGMRipData *data)
{
  data->profiles_dialog = ogmrip_profiles_dialog_new ();
  gtk_window_set_parent (GTK_WINDOW (data->profiles_dialog), GTK_WINDOW (data->window));

  g_signal_connect (data->profiles_dialog, "destroy",
      G_CALLBACK (gtk_widget_destroyed), &data->profiles_dialog);
  g_signal_connect (data->profiles_dialog, "response",
      G_CALLBACK (gtk_widget_hide), NULL);
  g_signal_connect (data->profiles_dialog, "delete-event",
      G_CALLBACK (gtk_true), NULL);
/*
  g_signal_connect_swapped (data->profiles_dialog, "new-profile",
      G_CALLBACK (ogmrip_main_profiles_dialog_new_profile), data);
  g_signal_connect_swapped (data->profiles_dialog, "remove-profile",
    G_CALLBACK (ogmrip_main_profiles_dialog_remove_profile), data);
  g_signal_connect_swapped (data->profiles_dialog, "rename-profile",
      G_CALLBACK (ogmrip_main_profiles_dialog_rename_profile), data);
*/
}

static void ogmrip_main_queue_dialog_encoding_imported (OGMRipData     *data,
                                                        OGMRipEncoding *encoding);
static void ogmrip_main_queue_dialog_responsed         (OGMRipData     *data,
                                                        gint           response);

static void
ogmrip_main_queue_dialog_construct (OGMRipData *data)
{
  data->queue_dialog = ogmrip_queue_dialog_new ();
  gtk_window_set_parent (GTK_WINDOW (data->queue_dialog), GTK_WINDOW (data->window));

  g_signal_connect (data->queue_dialog, "destroy",
      G_CALLBACK (gtk_widget_destroyed), &data->queue_dialog);
  g_signal_connect (data->queue_dialog, "delete-event",
      G_CALLBACK (gtk_true), NULL);

  g_signal_connect_swapped (data->queue_dialog, "import-encoding",
      G_CALLBACK (ogmrip_main_queue_dialog_encoding_imported), data);

  g_signal_connect_swapped (data->queue_dialog, "response",
      G_CALLBACK (ogmrip_main_queue_dialog_responsed), data);
}

static void ogmrip_main_progress_dialog_responsed (OGMRipData *data,
                                                   gint       response);

static void
ogmrip_main_progress_dialog_construct (OGMRipData *data)
{
  data->progress_dialog = ogmrip_progress_dialog_new ();
  gtk_window_set_parent (GTK_WINDOW (data->progress_dialog), GTK_WINDOW (data->window));

  g_signal_connect (data->progress_dialog, "destroy",
      G_CALLBACK (gtk_widget_destroyed), &data->progress_dialog);
  g_signal_connect (data->progress_dialog, "delete-event",
      G_CALLBACK (gtk_true), NULL);

  g_signal_connect_swapped (data->progress_dialog, "response",
      G_CALLBACK (ogmrip_main_progress_dialog_responsed), data);
}
/*
static void
ogmrip_main_options_dialog_profile_changed (OGMRipData *data)
{
  OGMRipEncoding *encoding;

  encoding = ogmrip_options_dialog_get_encoding (OGMRIP_OPTIONS_DIALOG (data->options_dialog));
  if (encoding && !OGMRIP_ENCODING_IS_RUNNING (encoding))
  {
    OGMRipProfile *profile;

    profile = ogmrip_encoding_get_profile (encoding);
    ogmrip_main_set_encoding_profile (encoding, profile, NULL);
    ogmrip_main_set_encoding_filename (encoding);
  }
}

static void
ogmrip_main_options_dialog_edit_clicked (OGMRipData *data)
{
  OGMRipProfile *profile;

  profile = ogmrip_options_dialog_get_active_profile (OGMRIP_OPTIONS_DIALOG (data->options_dialog));
  ogmrip_profiles_dialog_set_active (OGMRIP_PROFILES_DIALOG (data->profiles_dialog), profile);

  gtk_window_present (GTK_WINDOW (data->profiles_dialog));
}
*/
static gboolean
ogmrip_main_manager_add_encoding (OGMRipEncoding *encoding, OGMRipEncodingManager *queue)
{
  ogmrip_encoding_manager_add (queue, encoding);

  return TRUE;
}

static gboolean
ogmrip_main_manager_ask_encoding (OGMRipEncoding *encoding, GtkDialog *dialog)
{
  if (OGMRIP_ENCODING_IS_BACKUPED (encoding))
  {
    gint after_enc;

    after_enc = gtk_dialog_run (dialog);

    if (after_enc == OGMRIP_AFTER_ENC_REMOVE)
      ogmrip_encoding_cleanup (encoding);
    else if (after_enc == OGMRIP_AFTER_ENC_UPDATE)
    {
      OGMRipData *data;

      data = g_object_get_data (G_OBJECT (dialog), "__data__");

      ogmrip_main_load_from_encoding (data, encoding);
    }
  }

  return TRUE;
}

static gboolean
ogmrip_main_manager_find_encoding (OGMRipEncoding *encoding)
{
  return !OGMRIP_ENCODING_IS_BACKUPED (encoding);
}

static void
ogmrip_main_options_dialog_extract_responsed (OGMRipData *data, OGMRipEncoding *encoding)
{
  GError *error = NULL;
  gboolean do_quit = FALSE;

  GtkWidget *dialog;
  OGMRipEncodingManager *manager;
  gint after_enc, result, response;

  ogmrip_main_progress_dialog_construct (data);
  gtk_widget_show (data->progress_dialog);

  gtk_dialog_set_response_visible (GTK_DIALOG (data->options_dialog), OGMRIP_RESPONSE_EXTRACT, FALSE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (data->queue_dialog), GTK_RESPONSE_ACCEPT, FALSE);

  manager = ogmrip_encoding_manager_new ();

  g_settings_get (settings, OGMRIP_SETTINGS_AFTER_ENC, "u", &after_enc);

  switch (after_enc)
  {
    case OGMRIP_AFTER_ENC_REMOVE:
      ogmrip_encoding_manager_set_cleanup (manager, OGMRIP_CLEANUP_REMOVE_ALL);
      break;
    case OGMRIP_AFTER_ENC_KEEP:
    case OGMRIP_AFTER_ENC_ASK:
      ogmrip_encoding_manager_set_cleanup (manager, OGMRIP_CLEANUP_KEEP_ALL);
      break;
    case OGMRIP_AFTER_ENC_UPDATE:
      ogmrip_encoding_manager_set_cleanup (manager, OGMRIP_CLEANUP_KEEP_LAST);
      break;
  }

  g_signal_connect_swapped_while_alive (data->queue_dialog, "add-encoding",
      G_CALLBACK (ogmrip_encoding_manager_add), manager);
  g_signal_connect_swapped_while_alive (data->queue_dialog, "remove-encoding",
      G_CALLBACK (ogmrip_encoding_manager_remove), manager);

  if (encoding)
    ogmrip_encoding_manager_add (manager, encoding);
  else
    ogmrip_queue_dialog_foreach_encoding (OGMRIP_QUEUE_DIALOG (data->queue_dialog),
        (OGMRipEncodingFunc) ogmrip_main_manager_add_encoding, manager);

  result = OGMJOB_RESULT_CANCEL;

  if (ogmrip_encoding_manager_foreach (manager, (OGMRipEncodingFunc) ogmrip_main_check_encoding, data))
  {
#ifdef HAVE_DBUS_SUPPORT
    gint cookie;

    cookie = ogmrip_main_dbus_inhibit (data);
#endif /* HAVE_DBUS_SUPPORT */

    while (TRUE)
    {
      result = ogmrip_encoding_manager_run (manager, &error);
      if (result != OGMJOB_RESULT_ERROR)
        break;

      if (error == NULL)
        g_set_error (&error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_UNKNOWN, _("Unknown error"));

      if (!g_error_matches (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_ID))
        break;

      g_clear_error (&error);

      encoding = ogmrip_encoding_manager_find (manager, (OGMRipEncodingFunc) ogmrip_main_manager_find_encoding, NULL);
      if (!encoding)
        break;

      dialog = ogmrip_load_dvd_dialog_new (GTK_WINDOW (data->progress_dialog),
          ogmdvd_title_get_disc (ogmrip_encoding_get_title (encoding)), ogmrip_encoding_get_label (encoding), FALSE);

      response = gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);

      if (response != GTK_RESPONSE_ACCEPT)
      {
        result = OGMJOB_RESULT_CANCEL;
        break;
      }
    }

#ifdef HAVE_DBUS_SUPPORT
    if (cookie >= 0)
      ogmrip_main_dbus_uninhibit (data, cookie);
#endif /* HAVE_DBUS_SUPPORT */
  }

  if (result == OGMJOB_RESULT_SUCCESS)
  {
    switch (after_enc)
    {
      case OGMRIP_AFTER_ENC_ASK:
        dialog = ogmrip_main_update_remove_dialog_new (data);
        g_object_set_data (G_OBJECT (dialog), "__data__", data);
        ogmrip_encoding_manager_foreach (manager,
            (OGMRipEncodingFunc) ogmrip_main_manager_ask_encoding, dialog);
        gtk_widget_destroy (dialog);
        break;
      case OGMRIP_AFTER_ENC_UPDATE:
        encoding = ogmrip_encoding_manager_nth (manager, -1);
        if (encoding)
          ogmrip_main_load_from_encoding (data, encoding);
        break;
      default:
        break;
    }
  }

  g_object_unref (manager);

  gtk_dialog_set_response_visible (GTK_DIALOG (data->options_dialog), OGMRIP_RESPONSE_EXTRACT, TRUE);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (data->queue_dialog),
      GTK_RESPONSE_ACCEPT, result != OGMJOB_RESULT_SUCCESS);

  encoding = NULL;
  if (data->progress_dialog)
  {
    encoding = ogmrip_progress_dialog_get_encoding (OGMRIP_PROGRESS_DIALOG (data->progress_dialog));
    if (encoding)
      g_object_ref (encoding);

    do_quit = ogmrip_progress_dialog_get_quit (OGMRIP_PROGRESS_DIALOG (data->progress_dialog));
    gtk_widget_destroy (data->progress_dialog);
  }
  data->progress_dialog = NULL;

  gtk_window_set_title (GTK_WINDOW (data->window), "OGMRip");

  if (error)
  {
    ogmrip_main_error_message (data, encoding, error);
    g_error_free (error);
  }
  else if (data->warnings)
  {
    gchar *str;

    str = g_strdup_printf ("<b>%s</b>\n\n", _("The DVD has been successfully encoded, but..."));
    g_string_prepend (data->warnings, str);
    g_free (str);

    ogmrip_message_dialog (GTK_WINDOW (data->window), GTK_MESSAGE_WARNING, data->warnings->str);

    g_string_free (data->warnings, TRUE);
    data->warnings = NULL;
  }

  if (encoding)
    g_object_unref (encoding);

  if (result == OGMJOB_RESULT_SUCCESS && do_quit)
    gtk_widget_destroy (data->window);
}

/*
 * When the options dialog emits a response
 */
/*
static void
ogmrip_main_options_dialog_responsed (OGMRipData *data, gint response)
{
  if (response != OGMRIP_RESPONSE_TEST)
    gtk_widget_hide (data->options_dialog);

  if (response >= 0)
  {
    OGMRipEncoding *encoding;

    encoding = ogmrip_options_dialog_get_encoding (OGMRIP_OPTIONS_DIALOG (data->options_dialog));
    if (encoding)
    {
      switch (response)
      {
        case OGMRIP_RESPONSE_EXTRACT:
          ogmrip_main_options_dialog_extract_responsed (data, encoding);
          break;
        case OGMRIP_RESPONSE_TEST:
          ogmrip_main_test_encoding (encoding, data);
          break;
        case OGMRIP_RESPONSE_ENQUEUE:
          ogmrip_queue_dialog_add_encoding (OGMRIP_QUEUE_DIALOG (data->queue_dialog), encoding);
          gtk_window_present (GTK_WINDOW (data->queue_dialog));
          break;
        default:
          break;
      }
    }
  }
}
*/
/*
 * When an encoding is imported
 */
static void
ogmrip_main_queue_dialog_encoding_imported (OGMRipData *data, OGMRipEncoding *encoding)
{
  OGMRipProfile *profile;
/*
  ogmrip_settings_add_notify_while_alive (settings, OGMRIP_GCONF_ROOT, "general",
      (OGMRipNotifyFunc) ogmrip_main_encoding_options_notified, encoding, G_OBJECT (encoding));
  ogmrip_settings_add_notify_while_alive (settings, OGMRIP_GCONF_ROOT, "advanced",
      (OGMRipNotifyFunc) ogmrip_main_encoding_options_notified, encoding, G_OBJECT (encoding));
*/
  profile = ogmrip_encoding_get_profile (encoding);
  ogmrip_main_set_encoding_notifications (encoding, profile);

  g_signal_connect_swapped (encoding, "run", G_CALLBACK (ogmrip_main_encoding_run), data);
  g_signal_connect_swapped (encoding, "complete", G_CALLBACK (ogmrip_main_encoding_completed), data);
  g_signal_connect_swapped (encoding, "task::complete", G_CALLBACK (ogmrip_main_encoding_task_completed), data);
}

/*
 * When the queue dialog emits a response
 */
static void
ogmrip_main_queue_dialog_responsed (OGMRipData *data, gint response)
{
  if (response == GTK_RESPONSE_ACCEPT)
    ogmrip_main_options_dialog_extract_responsed (data, NULL);
  else
    gtk_widget_hide (data->queue_dialog);
}

/*
 * When the progress dialog emits a response
 */
static void
ogmrip_main_progress_dialog_responsed (OGMRipData *data, gint response)
{
  OGMRipProgressDialog *dialog;
  OGMRipEncoding *encoding;

  dialog = OGMRIP_PROGRESS_DIALOG (data->progress_dialog);
  encoding = ogmrip_progress_dialog_get_encoding (dialog);

  if (encoding)
  {
    switch (response)
    {
      case OGMRIP_RESPONSE_SUSPEND:
        gtk_dialog_set_response_visible (GTK_DIALOG (dialog), OGMRIP_RESPONSE_SUSPEND, FALSE);
        gtk_dialog_set_response_visible (GTK_DIALOG (dialog), OGMRIP_RESPONSE_RESUME, TRUE);
        ogmrip_encoding_suspend (encoding);
        break;
      case OGMRIP_RESPONSE_RESUME:
        gtk_dialog_set_response_visible (GTK_DIALOG (dialog), OGMRIP_RESPONSE_SUSPEND, TRUE);
        gtk_dialog_set_response_visible (GTK_DIALOG (dialog), OGMRIP_RESPONSE_RESUME, FALSE);
        ogmrip_encoding_resume (encoding);
        break;
      case GTK_RESPONSE_NONE:
        break;
      default:
        response = ogmrip_message_dialog (GTK_WINDOW (dialog), GTK_MESSAGE_QUESTION,
            _("Are you sure you want to cancel the encoding process?"));
        if (response == GTK_RESPONSE_YES)
          ogmrip_encoding_cancel (encoding);
        break;
    }
  }
}
/*
static void
ogmrip_main_profiles_dialog_new_profile (OGMRipData *data, OGMRipProfile *profile, const gchar *name)
{
  ogmrip_options_dialog_add_profile (OGMRIP_OPTIONS_DIALOG (data->options_dialog), profile, name);
}

static void
ogmrip_main_profiles_dialog_remove_profile (OGMRipData *data, OGMRipProfile *profile, const gchar *name)
{
  ogmrip_options_dialog_remove_profile (OGMRIP_OPTIONS_DIALOG (data->options_dialog), profile);
}

static void
ogmrip_main_profiles_dialog_rename_profile (OGMRipData *data, OGMRipProfile *profile, const gchar *new_name)
{
  ogmrip_options_dialog_rename_profile (OGMRIP_OPTIONS_DIALOG (data->options_dialog), profile, new_name);
}
*/
/*
 * When the extract button is activated
 */
static void
ogmrip_main_extract_activated (OGMRipData *data)
{
  OGMRipEncoding *encoding;
  GError *error = NULL;

  encoding = ogmrip_main_new_encoding (data, &error);
  if (encoding)
  {
/*
    ogmrip_options_dialog_set_encoding (OGMRIP_OPTIONS_DIALOG (data->options_dialog), encoding);
*/
    g_object_unref (encoding);

    gtk_window_present (GTK_WINDOW (data->options_dialog));
  }
  else if (error)
  {
    ogmrip_message_dialog (GTK_WINDOW (data->window), GTK_MESSAGE_ERROR, error->message);
    g_error_free (error);
  }
}

static void
ogmrip_main_play_cb (OGMRipData *data)
{
  GtkWidget *image;

  image = gtk_bin_get_child (GTK_BIN (data->play_button));
  gtk_container_remove (GTK_CONTAINER (data->play_button), image);

  image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_MENU);

  gtk_container_add (GTK_CONTAINER (data->play_button), image);
  gtk_widget_show (image);
}

static void
ogmrip_main_stop_cb (OGMRipData *data)
{
  GtkWidget *image;

  image = gtk_bin_get_child (GTK_BIN (data->play_button));
  gtk_container_remove (GTK_CONTAINER (data->play_button), image);

  image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_MENU);

  gtk_container_add (GTK_CONTAINER (data->play_button), image);
  gtk_widget_show (image);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->play_button), FALSE);
}

/*
 * When the play button is activated
 */
static void
ogmrip_main_play_activated (OGMRipData *data)
{
  static OGMRipPlayer *player = NULL;

  if (!player)
  {
    player = ogmrip_player_new ();

    g_signal_connect_swapped (player, "play",
        G_CALLBACK (ogmrip_main_play_cb), data);
    g_signal_connect_swapped (player, "stop",
        G_CALLBACK (ogmrip_main_stop_cb), data);
  }

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->play_button)))
  {
    GError *error = NULL;
    GtkWidget *chooser;
    OGMDvdTitle *title;

    guint start_chap;
    gint end_chap;

    title = ogmdvd_title_chooser_get_active (OGMDVD_TITLE_CHOOSER (data->title_chooser));

    chooser = ogmrip_chooser_list_nth (OGMRIP_CHOOSER_LIST (data->audio_list), 0);
    if (chooser)
    {
      OGMRipSource *source;
      gint source_type;

      source = ogmrip_source_chooser_get_active (OGMRIP_SOURCE_CHOOSER (chooser), &source_type);
      if (source)
      {
        if (source_type == OGMRIP_SOURCE_STREAM)
          ogmrip_player_set_audio_stream (player, OGMDVD_AUDIO_STREAM (source));
      }
    }

    chooser = ogmrip_chooser_list_nth (OGMRIP_CHOOSER_LIST (data->subp_list), 0);
    if (chooser)
    {
      OGMRipSource *source;
      gint source_type;

      source = ogmrip_source_chooser_get_active (OGMRIP_SOURCE_CHOOSER (chooser), &source_type);
      if (source)
      {
        if (source_type == OGMRIP_SOURCE_STREAM)
          ogmrip_player_set_subp_stream (player, OGMDVD_SUBP_STREAM (source));
      }
    }

    ogmrip_chapter_list_get_selected (OGMRIP_CHAPTER_LIST (data->chapter_list), &start_chap, &end_chap);
    ogmrip_player_set_chapters (player, start_chap, end_chap);

    ogmrip_player_set_title (player, title);
    if (!ogmrip_player_play (player, &error))
    {
      ogmrip_message_dialog (GTK_WINDOW (data->window), GTK_MESSAGE_ERROR,
          "<big><b>%s</b></big>\n\n%s", _("Can't play DVD title"), error->message);
      g_error_free (error);
    }
  }
  else
    ogmrip_player_stop (player);
}

/*
 * When the eject button is activated
 */
static void
ogmrip_main_eject_activated (OGMRipData *data, GtkWidget *dialog)
{
  if (data->disc)
  {
    gchar *device;

    device = ogmdvd_drive_chooser_get_device (OGMDVD_DRIVE_CHOOSER (dialog), NULL);
    if (device)
    {
      if (g_str_equal (device, ogmdvd_disc_get_device (data->disc)))
      {
        ogmdvd_disc_unref (data->disc);
        data->disc = NULL;

        ogmrip_main_clear (data);
      }
      g_free (device);
    }
  }
}

/*
 * When the load button is activated
 */
static void
ogmrip_main_load_activated (OGMRipData *data)
{
  if (!data->encoding)
  {
    GtkWidget *dialog;

    dialog = ogmdvd_drive_chooser_dialog_new ();
    gtk_window_set_icon_from_stock (GTK_WINDOW (dialog), GTK_STOCK_REFRESH);
    gtk_window_set_parent (GTK_WINDOW (dialog), GTK_WINDOW (data->window));

    g_signal_connect_swapped (dialog, "eject", G_CALLBACK (ogmrip_main_eject_activated), data);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      gchar *device;

      gtk_widget_hide (dialog);

      device = ogmdvd_drive_chooser_get_device (OGMDVD_DRIVE_CHOOSER (dialog), NULL);
      if (device)
      {
        ogmrip_main_load (data, device);
        g_free (device);
      }
    }
    gtk_widget_destroy (dialog);
  }
}

/*
 * When the import chapters menu item is activated
 */
static void
ogmrip_main_import_chapters_activated (OGMRipData *data)
{
  if (!data->encoding)
  {
    GtkWidget *dialog;
    GtkFileFilter *filter;

    dialog = gtk_file_chooser_dialog_new (_("Select a chapters file"), 
        GTK_WINDOW (data->window), GTK_FILE_CHOOSER_ACTION_OPEN, 
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
    gtk_window_set_icon_from_stock (GTK_WINDOW (dialog), GTK_STOCK_OPEN);
    gtk_window_set_parent (GTK_WINDOW (dialog), GTK_WINDOW (data->window));

    filter = gtk_file_filter_new ();
    gtk_file_filter_add_mime_type (filter, "text/*");
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      gchar *filename;

      gtk_widget_hide (dialog);

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      if (filename)
        if (!ogmrip_main_import_matroska_chapters (data, filename))
          if (!ogmrip_main_import_simple_chapters (data, filename))
            ogmrip_message_dialog (GTK_WINDOW (data->window), GTK_MESSAGE_ERROR,
                _("Could not open the chapters file '%s'"), filename);
      g_free (filename);
    }
    gtk_widget_destroy (dialog);
  }
}

/*
 * When the export chapters menu item is activated
 */
static void
ogmrip_main_export_chapters_activated (OGMRipData *data)
{
  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new (_("Select a file"), 
      GTK_WINDOW (data->window), GTK_FILE_CHOOSER_ACTION_SAVE, 
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  gtk_window_set_icon_from_stock (GTK_WINDOW (dialog), GTK_STOCK_SAVE);
  gtk_window_set_parent (GTK_WINDOW (dialog), GTK_WINDOW (data->window));

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    gchar *filename;

    gtk_widget_hide (dialog);

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    if (filename)
      ogmrip_main_export_simple_chapters (data, filename);
    g_free (filename);
  }
  gtk_widget_destroy (dialog);
}

/*
 * When the select all menu item is activated
 */
static void
ogmrip_main_select_all_activated (OGMRipData *data)
{
  OGMDvdTitle *title;
  OGMDvdTime time_;

  ogmrip_chapter_list_select_all (OGMRIP_CHAPTER_LIST (data->chapter_list));

  title = ogmdvd_title_chooser_get_active (OGMDVD_TITLE_CHOOSER (data->title_chooser));
  if (title)
  {
    if (ogmdvd_title_get_length (title, &time_) > 0)
    {
      gchar *str;

      str = g_strdup_printf ("%02d:%02d:%02d", time_.hour, time_.min, time_.sec);
      gtk_label_set_text (GTK_LABEL (data->length_label), str);
      g_free (str);
    }

    gtk_widget_set_sensitive (data->play_button, TRUE);
    gtk_widget_set_sensitive (data->extract_button, TRUE);
    gtk_widget_set_sensitive (data->relative_check, FALSE);
    gtk_action_set_sensitive (data->extract_action, TRUE);
  }
}

/*
 * When the deselect all menu item is activated
 */
static void
ogmrip_main_deselect_all_activated (OGMRipData *data)
{
  ogmrip_chapter_list_deselect_all (OGMRIP_CHAPTER_LIST (data->chapter_list));

  gtk_label_set_text (GTK_LABEL (data->length_label), "");
  gtk_widget_set_sensitive (data->play_button, FALSE);
  gtk_widget_set_sensitive (data->extract_button, FALSE);
  gtk_widget_set_sensitive (data->relative_check, FALSE);
  gtk_action_set_sensitive (data->extract_action, FALSE);
}

/*
 * When the preferences menu item is activated
 */
static void
ogmrip_main_pref_activated (OGMRipData *data)
{
  gtk_window_present (GTK_WINDOW (data->pref_dialog));
}

/*
 * When the profiles menu item is activated
 */
static void
ogmrip_main_profiles_activated (OGMRipData *data)
{
/*
  gchar *profile, *section;

  ogmrip_settings_get (settings, OGMRIP_GCONF_GENERAL, OGMRIP_GCONF_PROFILE, &profile, NULL);
  section = ogmrip_settings_build_section (settings, OGMRIP_GCONF_PROFILES, profile, NULL);
  g_free (profile);

  ogmrip_profiles_dialog_set_active (OGMRIP_PROFILES_DIALOG (data->profiles_dialog), section);
  g_free (section);
*/
  gtk_window_present (GTK_WINDOW (data->profiles_dialog));
}

/*
 * When the encodings menu item is activated
 */
static void
ogmrip_main_encodings_activated (OGMRipData *data)
{
  gtk_window_present (GTK_WINDOW (data->queue_dialog));
}

/*
 * When the about menu item is activated
 */
static void
ogmrip_main_about_activated (OGMRipData *data)
{
  static GdkPixbuf *icon = NULL;

  const gchar *authors[] = 
  { 
    "Olivier Rolland <billl@users.sourceforge.net>", 
    NULL 
  };
  gchar *translator_credits = _("translator-credits");

  const gchar *documenters[] = 
  { 
    "Olivier Rolland <billl@users.sourceforge.net>",
    NULL 
  };

  if (!icon)
    icon = gdk_pixbuf_new_from_file (OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_ICON_FILE, NULL);

  if (g_str_equal (translator_credits, "translator-credits"))
    translator_credits = NULL;

  gtk_show_about_dialog (GTK_WINDOW (data->window), 
      "name", PACKAGE_NAME,
      "version", PACKAGE_VERSION,
      "comments", _("A DVD Encoder for GNOME"),
      "copyright", "(c) 2004-2010 Olivier Rolland",
      "website", "http://ogmrip.sourceforge.net",
      "translator-credits", translator_credits,
      "documenters", documenters,
      "authors", authors,
      "logo", icon,
      NULL);
}

static void
ogmrip_main_title_chooser_changed (OGMRipData *data)
{
  GtkWidget *audio_chooser;
  GtkWidget *subp_chooser;

  OGMDvdTitle *title;
  OGMDvdTime time_;
  gchar *str;
/*
  ogmrip_options_dialog_set_encoding (OGMRIP_OPTIONS_DIALOG (data->options_dialog), NULL);
*/
  ogmrip_chooser_list_clear (OGMRIP_CHOOSER_LIST (data->audio_list));

  audio_chooser = ogmrip_audio_chooser_widget_new ();
  gtk_container_add (GTK_CONTAINER (data->audio_list), audio_chooser);
  gtk_widget_show (audio_chooser);

  gtk_widget_set_sensitive (data->audio_list, data->disc != NULL);

  ogmrip_chooser_list_clear (OGMRIP_CHOOSER_LIST (data->subp_list));

  subp_chooser = ogmrip_subtitle_chooser_widget_new ();
  gtk_container_add (GTK_CONTAINER (data->subp_list), subp_chooser);
  gtk_widget_show (subp_chooser);

  gtk_widget_set_sensitive (data->subp_list, data->disc != NULL);

  ogmdvd_chapter_list_clear (OGMDVD_CHAPTER_LIST (data->chapter_list));

  gtk_label_set_text (GTK_LABEL (data->length_label), "");

  gtk_widget_hide (gtk_widget_get_parent (data->angle_spin));

  if (data->disc)
  {
    gint pref, angles;

    title = ogmdvd_title_chooser_get_active (OGMDVD_TITLE_CHOOSER (data->title_chooser));
    if (title)
    {
      if (ogmdvd_title_get_length (title, &time_) > 0)
      {
        str = g_strdup_printf ("%02d:%02d:%02d", time_.hour, time_.min, time_.sec);
        gtk_label_set_text (GTK_LABEL (data->length_label), str);
        g_free (str);
      }

      g_settings_get (settings, OGMRIP_SETTINGS_PREF_AUDIO, "u", &pref);
      ogmrip_source_chooser_set_title (OGMRIP_SOURCE_CHOOSER (audio_chooser), title);
      ogmrip_source_chooser_select_language (OGMRIP_SOURCE_CHOOSER (audio_chooser), pref);

      if (gtk_combo_box_get_active (GTK_COMBO_BOX (audio_chooser)) == 0)
        gtk_combo_box_set_active (GTK_COMBO_BOX (audio_chooser), 1);

      g_settings_get (settings, OGMRIP_SETTINGS_PREF_SUBP, "u", &pref);
      ogmrip_source_chooser_set_title (OGMRIP_SOURCE_CHOOSER (subp_chooser), title);
      ogmrip_source_chooser_select_language (OGMRIP_SOURCE_CHOOSER (subp_chooser), pref);

      ogmdvd_chapter_list_set_title (OGMDVD_CHAPTER_LIST (data->chapter_list), title);
      ogmrip_main_select_all_activated (data);

      angles = ogmdvd_title_get_n_angles (title);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->angle_spin), 1);
      gtk_spin_button_set_range (GTK_SPIN_BUTTON (data->angle_spin), 1, angles);

      if (angles > 1)
        gtk_widget_show (gtk_widget_get_parent (data->angle_spin));
    }

    gtk_widget_set_sensitive (data->relative_check, FALSE);
  }
}

static void
ogmrip_main_chooser_list_changed (OGMRipData *data)
{
  static gint n_audio_prec = -1, n_subp_prec = -1;

  if (data->options_dialog)
  {
/*
    GType container, video_codec, subp_codec;
    GSList *sections, *section;

    gchar *name, *selected_profile;
*/
    gint n_audio, n_subp;

    n_audio = ogmrip_chooser_list_length (OGMRIP_CHOOSER_LIST (data->audio_list));
    n_subp = ogmrip_chooser_list_length (OGMRIP_CHOOSER_LIST (data->subp_list));

    if (n_audio != n_audio_prec || n_subp != n_subp_prec)
    {
/*
      ogmrip_settings_block (settings, OGMRIP_GCONF_GENERAL, OGMRIP_GCONF_PROFILE);

      selected_profile = ogmrip_options_dialog_get_active_profile (OGMRIP_OPTIONS_DIALOG (data->options_dialog));
      if (!selected_profile)
      {
        g_settings_get (settings, OGMRIP_GCONF_GENERAL, OGMRIP_GCONF_PROFILE, &name, NULL);
        selected_profile = ogmrip_settings_build_section (settings, OGMRIP_GCONF_PROFILES, name, NULL);
        g_free (name);
      }

      ogmrip_options_dialog_clear_profiles (OGMRIP_OPTIONS_DIALOG (data->options_dialog));

      sections = ogmrip_settings_get_subsections (settings, OGMRIP_GCONF_PROFILES);
      for (section = sections; section; section = section->next)
      {
        if (ogmrip_settings_has_section (settings, section->data) &&
            ogmrip_profiles_check_profile (section->data, NULL))
        {
          container = ogmrip_gconf_get_container_type (section->data, NULL);
          video_codec = ogmrip_gconf_get_video_codec_type (section->data, NULL);
          subp_codec = ogmrip_gconf_get_subp_codec_type (section->data, NULL);

          if ((video_codec == G_TYPE_NONE && n_audio + n_subp > 0) ||
              (video_codec != G_TYPE_NONE &&
               ogmrip_plugin_get_container_max_audio (container) >= n_audio &&
               ogmrip_plugin_get_container_max_subp (container) >= n_subp &&
               (subp_codec != OGMRIP_TYPE_HARDSUB || n_subp <= 1)))
          {
            ogmrip_settings_get (settings, section->data, "name", &name, NULL);
            ogmrip_options_dialog_add_profile (OGMRIP_OPTIONS_DIALOG (data->options_dialog), section->data, name);
            g_free (name);
          }
        }

        g_free (section->data);
      }
      g_slist_free (sections);

      ogmrip_options_dialog_set_active_profile (OGMRIP_OPTIONS_DIALOG (data->options_dialog), selected_profile);
      g_free (selected_profile);

      ogmrip_settings_unblock (settings, OGMRIP_GCONF_GENERAL, OGMRIP_GCONF_PROFILE);
*/
      n_audio_prec = n_audio;
      n_subp_prec = n_subp;
    }
  }
}

static void
ogmrip_main_audio_chooser_added (OGMRipData *data, OGMRipSourceChooser *chooser)
{
  OGMDvdTitle *title;

  title = ogmdvd_title_chooser_get_active (OGMDVD_TITLE_CHOOSER (data->title_chooser));
  if (title)
    ogmrip_source_chooser_set_title (OGMRIP_SOURCE_CHOOSER (chooser), title);

  g_signal_connect_swapped (chooser, "changed", 
      G_CALLBACK (ogmrip_main_chooser_list_changed), data);
}

static void
ogmrip_main_audio_chooser_removed (OGMRipData *data, OGMRipSourceChooser *chooser)
{
  GtkWidget *widget;

  widget = gtk_box_get_nth_child (GTK_BOX (chooser), 0);
  g_signal_handlers_disconnect_by_func (widget,
      ogmrip_main_chooser_list_changed, data);
}

static void
ogmrip_audio_options_free (OGMRipAudioOptions *options)
{
  ogmrip_audio_options_reset (options);
  g_free (options);
}

static void
ogmrip_main_audio_chooser_more_clicked (OGMRipData *data, OGMRipSourceChooser *chooser, OGMRipChooserList *list)
{
  GtkWidget *dialog;
  OGMRipAudioOptions *options;

  dialog = ogmrip_audio_options_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->window));

  options = g_object_get_data (G_OBJECT (chooser), "__audio_options__");
  if (!options)
  {
    OGMRipSource *source;

    source = ogmrip_source_chooser_get_active (OGMRIP_SOURCE_CHOOSER (chooser), NULL);

    options = g_new0 (OGMRipAudioOptions, 1);
    ogmrip_audio_options_init (options);

    options->language = ogmdvd_audio_stream_get_language (OGMDVD_AUDIO_STREAM (source));

    g_object_set_data_full (G_OBJECT (chooser), "__audio_options__",
        options, (GDestroyNotify) ogmrip_audio_options_free);
  }

  ogmrip_audio_options_dialog_set_options (OGMRIP_AUDIO_OPTIONS_DIALOG (dialog), options);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide (dialog);

  ogmrip_audio_options_dialog_get_options (OGMRIP_AUDIO_OPTIONS_DIALOG (dialog), options);

  gtk_widget_destroy (dialog);
}

static void
ogmrip_main_subp_chooser_added (OGMRipData *data, OGMRipSourceChooser *chooser)
{
  OGMDvdTitle *title;

  title = ogmdvd_title_chooser_get_active (OGMDVD_TITLE_CHOOSER (data->title_chooser));
  if (title)
    ogmrip_source_chooser_set_title (OGMRIP_SOURCE_CHOOSER (chooser), title);

  g_signal_connect_swapped (chooser, "changed", 
      G_CALLBACK (ogmrip_main_chooser_list_changed), data);
}

static void
ogmrip_main_subp_chooser_removed (OGMRipData *data, OGMRipSourceChooser *chooser)
{
  GtkWidget *widget;

  widget = gtk_box_get_nth_child (GTK_BOX (chooser), 0);
  g_signal_handlers_disconnect_by_func (widget,
      ogmrip_main_chooser_list_changed, data);
}

static void
ogmrip_subp_options_free (OGMRipSubpOptions *options)
{
  ogmrip_subp_options_reset (options);
  g_free (options);
}

static void
ogmrip_main_subp_chooser_more_clicked (OGMRipData *data, OGMRipSourceChooser *chooser, OGMRipChooserList *list)
{
  GtkWidget *dialog;
  OGMRipSubpOptions *options;

  dialog = ogmrip_subp_options_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->window));

  options = g_object_get_data (G_OBJECT (chooser), "__subp_options__");
  if (!options)
  {
    OGMRipSource *source;

    source = ogmrip_source_chooser_get_active (OGMRIP_SOURCE_CHOOSER (chooser), NULL);

    options = g_new0 (OGMRipSubpOptions, 1);
    ogmrip_subp_options_init (options);

    options->language = ogmdvd_subp_stream_get_language (OGMDVD_SUBP_STREAM (source));

    g_object_set_data_full (G_OBJECT (chooser), "__subp_options__",
        options, (GDestroyNotify) ogmrip_subp_options_free);
  }

  ogmrip_subp_options_dialog_set_options (OGMRIP_SUBP_OPTIONS_DIALOG (dialog), options);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide (dialog);

  ogmrip_subp_options_dialog_get_options (OGMRIP_SUBP_OPTIONS_DIALOG (dialog), options);

  gtk_widget_destroy (dialog);
}

static void
ogmrip_main_chapter_selection_changed (OGMRipData *data)
{
  OGMDvdTitle *title;
  OGMDvdTime time_;

  guint start_chap;
  gint end_chap;

  gboolean sensitive;
  gchar *str;

  sensitive = ogmrip_chapter_list_get_selected (OGMRIP_CHAPTER_LIST (data->chapter_list), &start_chap, &end_chap);
  if (!sensitive)
    gtk_label_set_text (GTK_LABEL (data->length_label), "");
  else
  {
    title = ogmdvd_title_chooser_get_active (OGMDVD_TITLE_CHOOSER (data->title_chooser));
    if (ogmdvd_title_get_chapters_length (title, start_chap, end_chap, &time_) > 0)
    {
      str = g_strdup_printf ("%02d:%02d:%02d", time_.hour, time_.min, time_.sec);
      gtk_label_set_text (GTK_LABEL (data->length_label), str);
      g_free (str);
    }
  }

  gtk_widget_set_sensitive (data->play_button, sensitive);
  gtk_widget_set_sensitive (data->extract_button, sensitive);
  gtk_action_set_sensitive (data->extract_action, sensitive);

  gtk_widget_set_sensitive (data->relative_check, sensitive && (start_chap > 0 || end_chap != -1));
}

/*
 * When the main window receives the delete event
 */
static gboolean
ogmrip_main_delete_event (OGMRipData *data)
{
  if (data->encoding)
    return TRUE;

  return FALSE;
}

/*
 * When the main window is destroyed
 */
static void
ogmrip_main_destroyed (OGMRipData *data)
{
  g_signal_handlers_disconnect_by_func (data->audio_list,
      ogmrip_main_chooser_list_changed, data);
  ogmrip_chooser_list_clear (OGMRIP_CHOOSER_LIST (data->audio_list));

  g_signal_handlers_disconnect_by_func (data->subp_list,
      ogmrip_main_chooser_list_changed, data);
  ogmrip_chooser_list_clear (OGMRIP_CHOOSER_LIST (data->subp_list));

  if (data->disc)
    ogmdvd_disc_unref (data->disc);
  data->disc = NULL;

  if (data->pref_dialog)
    gtk_widget_destroy (data->pref_dialog);
  data->pref_dialog = NULL;

  if (data->options_dialog)
    gtk_widget_destroy (data->options_dialog);
  data->options_dialog = NULL;

  if (data->profiles_dialog)
    gtk_widget_destroy (data->profiles_dialog);
  data->profiles_dialog = NULL;

  if (data->queue_dialog)
    gtk_widget_destroy (data->queue_dialog);
  data->queue_dialog = NULL;

  g_free (data);
}

static void
ogmrip_main_item_selected (GtkWidget *item, GtkWidget *statusbar)
{
  gchar *hint;

  hint = g_object_get_data (G_OBJECT (item), "__menu_hint__");
  if (hint)
  {
    guint context_id;

    context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), "__menu_hint__");
    gtk_statusbar_push (GTK_STATUSBAR (statusbar), context_id, hint);
  }
}

static void
ogmrip_main_item_deselected (GtkWidget *item, GtkWidget *statusbar)
{
  guint context_id;

  context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), "__menu_hint__");
  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), context_id);
}

static void
ogmrip_main_connect_proxy (GtkUIManager *uimanager, GtkAction *action, GtkWidget *proxy, GtkWidget *statusbar)
{
  if (GTK_IS_MENU_ITEM (proxy))
  {
    gchar *hint;

    g_object_get (action, "tooltip", &hint, NULL);
    if (hint)
    {
      g_object_set_data_full (G_OBJECT (proxy), "__menu_hint__", hint, (GDestroyNotify) g_free);

      g_signal_connect (proxy, "select", G_CALLBACK (ogmrip_main_item_selected), statusbar);
      g_signal_connect (proxy, "deselect", G_CALLBACK (ogmrip_main_item_deselected), statusbar);
    }
  }
}

static OGMRipData *
ogmrip_main_new (void)
{
  GtkActionEntry action_entries[] =
  {
    { "FileMenu",     NULL,                  N_("_File"),               NULL,          NULL,                             NULL },
    { "Load",         GTK_STOCK_CDROM,       N_("_Load"),               "<ctl>L",      N_("Load a DVD disk, an ISO file, or a DVD structure"), NULL },
    { "Extract",      GTK_STOCK_CONVERT,     N_("E_xtract"),            "<ctl>Return", N_("Extract selected streams"),   NULL },
    { "OpenChapters", NULL,                  N_("_Import Chapters..."), NULL,          N_("Import chapter information"), NULL },
    { "SaveChapters", NULL,                  N_("_Export Chapters..."), NULL,          N_("Export chapter information"), NULL },
    { "Quit",         GTK_STOCK_QUIT,        NULL,                      NULL,          N_("Exit OGMRip"),                NULL },
    { "EditMenu",     NULL,                  N_("_Edit"),               NULL,          NULL,                             NULL },
    { "SelectAll",    NULL,                  N_("Select _All"),         "<ctl>A",      N_("Select all chapters"),        NULL },
    { "DeselectAll",  NULL,                  N_("_Deselect All"),       "<ctl>D",      N_("Deselect all chapters"),      NULL },
    { "Profiles",     NULL,                  N_("Pro_files"),           "<ctl>F",      N_("Edit the profiles"),          NULL },
    { "Preferences",  GTK_STOCK_PREFERENCES, NULL,                      NULL,          N_("Edit the preferences"),       NULL },
    { "Encodings",    NULL,                  N_("_Encodings"),          "<ctl>E",      N_("Edit the encodings"),         NULL },
    { "HelpMenu",     NULL,                  N_("_Help"),               NULL,          NULL,                             NULL },
    { "About",        GTK_STOCK_ABOUT,       N_("_About"),              NULL,          N_("About OGMRip"),               NULL },
  };

  GError *error = NULL;

  OGMRipData *data;
  GtkWidget *widget, *child;
  GtkBuilder *builder;

  GtkAction *action;
  GtkActionGroup *action_group;
  GtkAccelGroup *accel_group;
  GtkUIManager *ui_manager;

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_GLADE_FILE, &error))
  {
    g_warning ("Couldn't load builder file: %s", error->message);
    g_object_unref (builder);
    g_error_free (error);
    return NULL;
  }

  data = g_new0 (OGMRipData, 1);

  data->window = gtk_builder_get_widget (builder, "main-window");
  gtk_window_set_default_size (GTK_WINDOW (data->window), 350, 500);
  gtk_window_set_icon_from_file (GTK_WINDOW (data->window), OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_ICON_FILE, NULL);

  g_signal_connect_swapped (data->window, "delete-event", G_CALLBACK (ogmrip_main_delete_event), data);
  g_signal_connect_swapped (data->window, "destroy", G_CALLBACK (ogmrip_main_destroyed), data);
  g_signal_connect (data->window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, action_entries, G_N_ELEMENTS (action_entries), NULL);

  ui_manager = gtk_ui_manager_new ();

  widget = gtk_builder_get_widget (builder, "statusbar");
  g_signal_connect (ui_manager, "connect-proxy", G_CALLBACK (ogmrip_main_connect_proxy), widget);

  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_file (ui_manager, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_UI_FILE, NULL);

  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (data->window), accel_group);

  child = gtk_bin_get_child (GTK_BIN (data->window));

  widget = gtk_ui_manager_get_widget (ui_manager, "/Menubar");
  gtk_box_pack_start (GTK_BOX (child), widget, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (child), widget, 0);

  action =  gtk_action_group_get_action (action_group, "Load");
  g_signal_connect_swapped (action, "activate", G_CALLBACK (ogmrip_main_load_activated), data);

  data->import_chap_action =  gtk_action_group_get_action (action_group, "OpenChapters");
  g_signal_connect_swapped (data->import_chap_action, "activate", G_CALLBACK (ogmrip_main_import_chapters_activated), data);
  gtk_action_set_sensitive (data->import_chap_action, FALSE);

  data->export_chap_action =  gtk_action_group_get_action (action_group, "SaveChapters");
  g_signal_connect_swapped (data->export_chap_action, "activate", G_CALLBACK (ogmrip_main_export_chapters_activated), data);
  gtk_action_set_sensitive (data->export_chap_action, FALSE);

  action =  gtk_action_group_get_action (action_group, "Quit");
  g_signal_connect_swapped (action, "activate", G_CALLBACK (gtk_widget_destroy), data->window);

  action =  gtk_action_group_get_action (action_group, "Preferences");
  g_signal_connect_swapped (action, "activate", G_CALLBACK (ogmrip_main_pref_activated), data);

  action =  gtk_action_group_get_action (action_group, "Profiles");
  g_signal_connect_swapped (action, "activate", G_CALLBACK (ogmrip_main_profiles_activated), data);

  action =  gtk_action_group_get_action (action_group, "Encodings");
  g_signal_connect_swapped (action, "activate", G_CALLBACK (ogmrip_main_encodings_activated), data);

  action =  gtk_action_group_get_action (action_group, "SelectAll");
  g_signal_connect_swapped (action, "activate", G_CALLBACK (ogmrip_main_select_all_activated), data);

  action =  gtk_action_group_get_action (action_group, "DeselectAll");
  g_signal_connect_swapped (action, "activate", G_CALLBACK (ogmrip_main_deselect_all_activated), data);

  action =  gtk_action_group_get_action (action_group, "About");
  g_signal_connect_swapped (action, "activate", G_CALLBACK (ogmrip_main_about_activated), data);

  data->extract_action =  gtk_action_group_get_action (action_group, "Extract");
  g_signal_connect_swapped (data->extract_action, "activate", G_CALLBACK (ogmrip_main_extract_activated), data);
  gtk_action_set_sensitive (data->extract_action, FALSE);

  widget = gtk_builder_get_widget (builder, "load-button");
  g_signal_connect_swapped (widget, "clicked", G_CALLBACK (ogmrip_main_load_activated), data);

  data->extract_button = gtk_builder_get_widget (builder, "extract-button");
  g_signal_connect_swapped (data->extract_button, "clicked", G_CALLBACK (ogmrip_main_extract_activated), data);

  data->title_chooser = gtk_builder_get_widget (builder, "title-chooser");
  gtk_widget_set_sensitive (data->title_chooser, FALSE);
  gtk_widget_show (data->title_chooser);

  g_signal_connect_swapped (data->title_chooser, "changed", G_CALLBACK (ogmrip_main_title_chooser_changed), data);

  data->angle_spin = gtk_builder_get_widget (builder, "angle-spin");

  data->play_button = gtk_builder_get_widget (builder, "play-button");
  g_signal_connect_swapped (data->play_button, "toggled", G_CALLBACK (ogmrip_main_play_activated), data);

  widget = gtk_builder_get_widget (builder, "table");

  data->audio_list = ogmrip_chooser_list_new (OGMRIP_TYPE_AUDIO_CHOOSER_WIDGET);
  gtk_table_attach (GTK_TABLE (widget), data->audio_list, 1, 3, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_widget_set_sensitive (data->audio_list, FALSE);
  gtk_widget_show (data->audio_list);

  g_signal_connect_swapped (data->audio_list, "add",
      G_CALLBACK (ogmrip_main_audio_chooser_added), data);
  g_signal_connect_swapped (data->audio_list, "remove",
      G_CALLBACK (ogmrip_main_audio_chooser_removed), data);
  g_signal_connect_swapped (data->audio_list, "more-clicked",
      G_CALLBACK (ogmrip_main_audio_chooser_more_clicked), data);

  data->subp_list = ogmrip_chooser_list_new (OGMRIP_TYPE_SUBTITLE_CHOOSER_WIDGET);
  gtk_table_attach (GTK_TABLE (widget), data->subp_list, 1, 3, 3, 4, GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_widget_set_sensitive (data->subp_list, FALSE);
  gtk_widget_show (data->subp_list);

  g_signal_connect_swapped (data->subp_list, "add",
      G_CALLBACK (ogmrip_main_subp_chooser_added), data);
  g_signal_connect_swapped (data->subp_list, "remove",
      G_CALLBACK (ogmrip_main_subp_chooser_removed), data);
  g_signal_connect_swapped (data->subp_list, "more-clicked",
      G_CALLBACK (ogmrip_main_subp_chooser_more_clicked), data);

  widget = ogmrip_audio_chooser_widget_new ();
  gtk_container_add (GTK_CONTAINER (data->audio_list), widget);
  gtk_widget_show (widget);

  widget = ogmrip_subtitle_chooser_widget_new ();
  gtk_container_add (GTK_CONTAINER (data->subp_list), widget);
  gtk_widget_show (widget);

  data->length_label = gtk_builder_get_widget (builder, "length-label");
  data->relative_check = gtk_builder_get_widget (builder, "relative-check");
  data->title_entry = gtk_builder_get_widget (builder, "title-entry");

  widget = gtk_builder_get_widget (builder, "scrolledwindow");

  data->chapter_list = ogmrip_chapter_list_new ();
  gtk_container_add (GTK_CONTAINER (widget), data->chapter_list);
  gtk_widget_show (data->chapter_list);

  g_signal_connect_swapped (data->chapter_list, "selection-changed", 
      G_CALLBACK (ogmrip_main_chapter_selection_changed), data);

  g_object_unref (builder);

  return data;
}

static gboolean
ogmrip_check_profiles (OGMRipData *data)
{
  GList *list, *link;

  list = ogmrip_profiles_check_updates (NULL, ogmrip_get_system_profiles_dir (), NULL);
  list = ogmrip_profiles_check_updates (list, ogmrip_get_user_profiles_dir (), NULL);

  if (list)
  {
    gint response;
    GtkWidget *dialog;

    dialog = ogmrip_update_dialog_new ();
    gtk_window_set_parent (GTK_WINDOW (dialog), GTK_WINDOW (data->window));

    for (link = list; link; link = link->next)
    {
      ogmrip_update_dialog_add_profile (OGMRIP_UPDATE_DIALOG (dialog), link->data);
      g_free (link->data);
    }

    g_list_free (list);

    response = gtk_dialog_run (GTK_DIALOG (dialog));

    if (response == GTK_RESPONSE_ACCEPT)
    {
      gchar **strv;

      list = ogmrip_update_dialog_get_profiles (OGMRIP_UPDATE_DIALOG (dialog));

      for (link = list; link; link = link->next)
      {
        strv = g_strsplit_set (link->data, "@", 2);
/*
        if (strv[1])
          ogmrip_settings_set (settings, strv[0], "version", strv[1], NULL);
*/
        g_strfreev (strv);

        g_free (link->data);
      }

      g_list_free (list);
    }

    gtk_widget_destroy (dialog);
  }

  ogmrip_profiles_import_all (ogmrip_get_system_profiles_dir (), NULL);
  ogmrip_profiles_import_all (ogmrip_get_user_profiles_dir (), NULL);

  gtk_main_quit ();

  return FALSE;
}

#ifdef G_ENABLE_DEBUG
static gboolean debug = TRUE;
#else
static gboolean debug = FALSE;
#endif

static void
ogmrip_init (void)
{
  ogmrip_settings_init ();

  ogmrip_plugin_init ();
  ogmrip_options_plugin_init ();

#ifdef HAVE_LIBNOTIFY_SUPPORT
  notify_init (PACKAGE_NAME);
#endif /* HAVE_LIBNOTIFY_SUPPORT */
}

static void
ogmrip_uninit (void)
{
  ogmrip_settings_uninit ();

  ogmrip_options_plugin_uninit ();
  ogmrip_plugin_uninit ();
}

int
main (int argc, char *argv[])
{
  OGMRipData *data;

  GOptionEntry opts[] =
  {
    { "debug", 0,  0, G_OPTION_ARG_NONE, &debug, "Enable debug messages", NULL },
    { NULL,    0,  0, 0,                 NULL,  NULL,                     NULL }
  };

  if (!gtk_init_with_args (&argc, &argv, "<DVD DEVICE>", opts, GETTEXT_PACKAGE, NULL))
    return EXIT_FAILURE;

  if (debug)
    ogmjob_log_set_print_stdout (TRUE);

  ogmrip_init ();

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */

  data = ogmrip_main_new ();

  g_idle_add ((GSourceFunc) ogmrip_check_profiles, data);

  gtk_main ();

  ogmrip_main_pref_dialog_construct (data);
  ogmrip_main_options_dialog_construct (data);
  ogmrip_main_profiles_dialog_construct (data);
  ogmrip_main_queue_dialog_construct (data);

  if (argc > 1)
  {
    if (g_file_test (argv[1], G_FILE_TEST_EXISTS))
    {
      gchar *filename;

      filename = ogmrip_fs_get_full_path (argv[1]);
      ogmrip_main_load (data, filename);
      g_free (filename);
    }
  }

  gtk_widget_show (data->window);

  gtk_main ();

  ogmrip_uninit ();

  return EXIT_SUCCESS;
}

