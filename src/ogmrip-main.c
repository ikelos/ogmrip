/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-job.h"
#include "ogmrip-dvd.h"
#include "ogmrip-dvd-gtk.h"
#include "ogmrip-encode-gtk.h"
#include "ogmrip-media-gtk.h"
#include "ogmrip-settings.h"

#include "ogmrip-options-dialog.h"
#include "ogmrip-pref-dialog.h"

#ifdef HAVE_ENCHANT_SUPPORT
#include "ogmrip-spell-dialog.h"
#endif /* HAVE_ENCHANT_SUPPORT */

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <stdlib.h>

#include <libxml/tree.h>

#ifdef HAVE_LIBNOTIFY_SUPPORT
#include <libnotify/notify.h>
#endif /* HAVE_LIBNOTIFY_SUPPORT */

#define OGMRIP_UI_FILE    "ogmrip"  G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-ui.xml"
#define OGMRIP_GLADE_FILE "ogmrip"  G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-main.glade"
#define OGMRIP_ICON_FILE  "pixmaps" G_DIR_SEPARATOR_S "ogmrip.png"

#define OGMRIP_DEFAULT_FILE_NAME "movie"

typedef struct
{
  OGMRipMedia *media;

  GtkWidget *window;

  GtkWidget *title_entry;
  GtkWidget *title_chooser;
  GtkWidget *length_label;

  GtkWidget *audio_list;
  GtkWidget *subp_list;

  GtkWidget *relative_check;
  GtkWidget *angle_spin;

  GtkAction *extract_action;

  GtkTreeModel *chapter_store;

  OGMRipEncodingManager *manager;
  OGMRipPlayer *player;
} OGMRipData;

extern GSettings *settings;

#ifdef HAVE_ENCHANT_SUPPORT
/*
 * Performs spell checking
 */
static void
ogmrip_main_spell_check (OGMRipData *data, const gchar *filename, gint lang)
{
  GtkWidget *dialog;

  GIOChannel *input = NULL, *output = NULL;
  GIOStatus status;

  gboolean retval = FALSE;
  gchar *text, *corrected;
  gchar *new_file;

  input = g_io_channel_new_file (filename, "r", NULL);
  if (!input)
    goto spell_check_cleanup;

  new_file = ogmrip_fs_mktemp ("sub.XXXXXX", NULL);
  if (!new_file)
    goto spell_check_cleanup;

  output = g_io_channel_new_file (new_file, "w", NULL);
  if (!output)
    goto spell_check_cleanup;

  dialog = ogmrip_spell_dialog_new (ogmrip_language_get_iso639_1 (lang));
  if (!dialog)
  {
    ogmrip_run_error_dialog (GTK_WINDOW (data->window), NULL,
        "<big><b>%s</b></big>\n\n%s", _("Could not create dictionary"), _("Spell will not be checked."));
    goto spell_check_cleanup;
  }

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->window));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  gtk_window_present (GTK_WINDOW (dialog));

  do
  {
    status = g_io_channel_read_line (input, &text, NULL, NULL, NULL);
    if (status == G_IO_STATUS_NORMAL)
    {
      retval = ogmrip_spell_dialog_check_text (OGMRIP_SPELL_DIALOG (dialog), text, &corrected);
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

  gtk_widget_destroy (dialog);

  retval &= status == G_IO_STATUS_EOF;

spell_check_cleanup:
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
}
#endif /* HAVE_ENCHANT_SUPPORT */

#define PM_DBUS_SERVICE           "org.gnome.SessionManager"
#define PM_DBUS_INHIBIT_PATH      "/org/gnome/SessionManager"
#define PM_DBUS_INHIBIT_INTERFACE "org.gnome.SessionManager"

static gint
ogmrip_main_dbus_inhibit (OGMRipData *data)
{
  GError *error = NULL;

  GDBusConnection *conn;
  GVariant *res;
  guint cookie;

  conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  if (!conn)
  {
    g_warning ("Couldn't get a DBUS connection: %s", error->message);
    g_error_free (error);
    return -1;
  }

  res = g_dbus_connection_call_sync (conn,
      PM_DBUS_SERVICE,
      PM_DBUS_INHIBIT_PATH,
      PM_DBUS_INHIBIT_INTERFACE,
      "Inhibit",
      g_variant_new("(susu)",
        g_get_application_name (),
        0,
        "Encoding",
        1 | 4),
      G_VARIANT_TYPE ("(u)"),
      G_DBUS_CALL_FLAGS_NONE,
      -1,
      NULL,
      &error);

  if (res)
  {
    g_variant_get (res, "(u)", &cookie);
    g_variant_unref (res);
  }
  else
  {
    g_warning ("Failed to inhibit the system from suspending: %s", error->message);
    g_error_free (error);
    cookie = -1;
  }

  return cookie;
}

static void
ogmrip_main_dbus_uninhibit (OGMRipData *data, guint cookie)
{
  GError *error = NULL;

  GDBusConnection *conn;
  GVariant *res;

  conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  if (!conn)
  {
    g_warning ("Couldn't get a DBUS connection: %s", error->message);
    g_error_free (error);
    return;
  }

  res = g_dbus_connection_call_sync (conn,
      PM_DBUS_SERVICE,
      PM_DBUS_INHIBIT_PATH,
      PM_DBUS_INHIBIT_INTERFACE,
      "Uninhibit",
      g_variant_new("(u)",
        cookie),
      NULL,
      G_DBUS_CALL_FLAGS_NONE,
      -1,
      NULL,
      &error);

  if (res)
    g_variant_unref (res);
  else
  {
    g_warning ("Failed to restore the system power manager: %s", error->message);
    g_error_free (error);
  }
}

/*
 * Loads a media
 */
static void
ogmrip_main_load (OGMRipData *data, const gchar *path)
{
  GError *error = NULL;
  OGMRipMedia *disc;

  disc = ogmdvd_disc_new (path, &error);
  if (!disc)
  {
    ogmrip_run_error_dialog (GTK_WINDOW (data->window), error, ("Could not open the DVD"));
    g_clear_error (&error);
  }
  else
  {
    const gchar *label = NULL;
    gint nvid;

    if (data->media)
      g_object_unref (data->media);
    data->media = disc;

    ogmrip_title_chooser_set_media (OGMRIP_TITLE_CHOOSER (data->title_chooser), disc);

    nvid = ogmrip_media_get_n_titles (disc);
    if (nvid > 0)
      label = ogmrip_media_get_label (disc);

    gtk_entry_set_text (GTK_ENTRY (data->title_entry), label ? label : "");
  }
}

/*
 * Cleans an encoding
 */
static void
ogmrip_main_clean (OGMRipData *data, OGMRipEncoding *encoding, gboolean error)
{
  if (!g_settings_get_boolean (settings, OGMRIP_SETTINGS_KEEP_TMP))
  {
    OGMRipCodec *codec;
    GList *list, *link;

    codec = ogmrip_encoding_get_video_codec (encoding);
    g_unlink (ogmrip_file_get_path (ogmrip_codec_get_output (codec)));

    list = ogmrip_encoding_get_audio_codecs (encoding);
    for (link = list; link; link = link->next)
      g_unlink (ogmrip_file_get_path (ogmrip_codec_get_output (link->data)));
    g_list_free (list);

    list = ogmrip_encoding_get_subp_codecs (encoding);
    for (link = list; link; link = link->next)
      g_unlink (ogmrip_file_get_path (ogmrip_codec_get_output (link->data)));
    g_list_free (list);

    list = ogmrip_encoding_get_chapters (encoding);
    for (link = list; link; link = link->next)
      g_unlink (ogmrip_file_get_path (ogmrip_codec_get_output (link->data)));
    g_list_free (list);
  }

  if (!error && !g_settings_get_boolean (settings, OGMRIP_SETTINGS_LOG_OUTPUT))
  {
    const gchar *filename;

    filename = ogmrip_encoding_get_log_file (encoding);
    if (filename)
      g_unlink (filename);
  }

  if (ogmrip_encoding_get_copy (encoding))
  {
    guint after;

    after = g_settings_get_uint (settings, OGMRIP_SETTINGS_AFTER_ENC);
    if (after == OGMRIP_AFTER_ENC_ASK)
    {
      GtkWidget *dialog;

      dialog = gtk_message_dialog_new (GTK_WINDOW (data->window), 
          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
          GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
          _("Do you want to remove the copy of the DVD ?"));
      after = gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES ?
        OGMRIP_AFTER_ENC_REMOVE : OGMRIP_AFTER_ENC_KEEP;
      gtk_widget_destroy (dialog);
    }

    if (after == OGMRIP_AFTER_ENC_REMOVE)
    {
      OGMRipTitle *title;
      const gchar *uri;

      title = ogmrip_encoding_get_title (encoding);
      uri = ogmrip_media_get_uri (ogmrip_title_get_media (title));
      if (!g_str_has_prefix (uri, "dvd://"))
        g_warning ("Unknown scheme for '%s'", uri);
      else
        ogmrip_fs_rmdir (uri + 6, TRUE, NULL);
    }
  }
}

/*
 * Displays the error message
 */
static void
ogmrip_main_display_error (OGMRipData *data, OGMRipEncoding *encoding, GError *error)
{
  GtkWidget *dialog, *area, *widget;
  const gchar *filename;

  dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (data->window),
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "<b>%s</b>", _("Encoding failed"));

  if (error)
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
  else
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", _("Unknown error"));

  filename = ogmrip_encoding_get_log_file (encoding);
  if (filename)
  {
    GFileInputStream *stream;
    GtkTextBuffer *buffer;
    GtkTextIter iter;
    GFile *file;

    gchar text[2048];
    gssize n;

    area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

    widget = gtk_expander_new_with_mnemonic ("_Show the log file");
    gtk_expander_set_expanded (GTK_EXPANDER (widget), FALSE);
    gtk_container_add (GTK_CONTAINER (area), widget);
    gtk_widget_show (widget);

    area = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (area), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (area), GTK_SHADOW_IN);
    gtk_container_add (GTK_CONTAINER (widget), area);
    gtk_widget_show (area);

    widget = gtk_text_view_new ();
    gtk_text_view_set_editable (GTK_TEXT_VIEW (widget), FALSE);
    gtk_container_add (GTK_CONTAINER (area), widget);
    gtk_widget_show (widget);

    gtk_widget_set_size_request (widget, 700, 400);

    file = g_file_new_for_path (filename);
    stream = g_file_read (file, NULL, NULL);
    g_object_unref (file);

    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
    gtk_text_buffer_get_start_iter (buffer, &iter);

    while ((n = g_input_stream_read (G_INPUT_STREAM (stream), text, 2048, NULL, NULL)) > 0)
      gtk_text_buffer_insert (buffer, &iter, text, n);

    g_object_unref (stream);
  }

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
ogmrip_main_encoding_spawn_run (OGMRipEncoding *encoding, OGMJobSpawn *spawn, OGMRipProgressDialog *dialog)
{
  OGMRipStream *stream;
  gchar *message;

  if (spawn)
  {
    if (OGMRIP_IS_VIDEO_CODEC (spawn))
      ogmrip_progress_dialog_set_message (dialog, _("Encoding video title"));
    else if (OGMRIP_IS_CHAPTERS (spawn))
      ogmrip_progress_dialog_set_message (dialog, _("Extracting chapters information"));
    else if (OGMRIP_IS_CONTAINER (spawn))
      ogmrip_progress_dialog_set_message (dialog, _("Merging audio and video streams"));
    else if (OGMRIP_IS_COPY (spawn))
      ogmrip_progress_dialog_set_message (dialog, _("DVD backup"));
    else if (OGMRIP_IS_ANALYZE (spawn))
      ogmrip_progress_dialog_set_message (dialog, _("Analyzing video stream"));
    else if (OGMRIP_IS_TEST (spawn))
      ogmrip_progress_dialog_set_message (dialog, _("Running the compressibility test"));
    else if (OGMRIP_IS_AUDIO_CODEC (spawn))
    {
      stream = ogmrip_codec_get_input (OGMRIP_CODEC (spawn));
      message = g_strdup_printf (_("Extracting audio stream %d"),
          ogmrip_audio_stream_get_nr (OGMRIP_AUDIO_STREAM (stream)) + 1);
      ogmrip_progress_dialog_set_message (dialog, message);
      g_free (message);
    }
    else if (OGMRIP_IS_SUBP_CODEC (spawn))
    {
      stream = ogmrip_codec_get_input (OGMRIP_CODEC (spawn));
      message = g_strdup_printf (_("Extracting subtitle stream %d"),
          ogmrip_subp_stream_get_nr (OGMRIP_SUBP_STREAM (stream)) + 1);
      ogmrip_progress_dialog_set_message (dialog, message);
      g_free (message);
    }
  }

  ogmrip_progress_dialog_set_fraction (dialog, 0.0);
}

static void
ogmrip_main_encoding_spawn_progress (OGMRipEncoding *encoding, OGMJobSpawn *spawn, gdouble fraction, OGMRipProgressDialog *dialog)
{
  ogmrip_progress_dialog_set_fraction (dialog, fraction);
}

static void
ogmrip_main_progress_dialog_response (GtkWidget *parent, gint response_id, OGMRipEncoding *encoding)
{
  GtkWidget *dialog;

  switch (response_id)
  {
    case OGMRIP_RESPONSE_SUSPEND:
      ogmrip_encoding_suspend (encoding);
      break;
    case OGMRIP_RESPONSE_RESUME:
      ogmrip_encoding_resume (encoding);
      break;
    default:
      dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
          GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
          _("Are you sure you want to cancel the encoding process?"));
      if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
        ogmrip_encoding_cancel (encoding);
      gtk_widget_destroy (dialog);
      break;
  }
}

/*
 * Encodes an encoding
 */
static void
ogmrip_main_encode (OGMRipData *data, OGMRipEncoding *encoding)
{
  if (ogmrip_open_title (GTK_WINDOW (data->window), ogmrip_encoding_get_title (encoding)))
  {
    GError *error = NULL;
    GtkWidget *dialog;
    gint result, cookie;

    cookie = ogmrip_main_dbus_inhibit (data);

    dialog = ogmrip_progress_dialog_new (GTK_WINDOW (data->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, TRUE);
    ogmrip_progress_dialog_set_title (OGMRIP_PROGRESS_DIALOG (dialog),
        ogmrip_container_get_label (ogmrip_encoding_get_container (encoding)));

    g_signal_connect (dialog, "response",
        G_CALLBACK (ogmrip_main_progress_dialog_response), encoding);
    g_signal_connect (dialog, "delete-event",
        G_CALLBACK (gtk_true), NULL);

    gtk_window_present (GTK_WINDOW (dialog));

    g_signal_connect (encoding, "run",
        G_CALLBACK (ogmrip_main_encoding_spawn_run), dialog);
    g_signal_connect (encoding, "progress",
        G_CALLBACK (ogmrip_main_encoding_spawn_progress), dialog);

    result = ogmrip_encoding_encode (encoding, &error);

    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_main_encoding_spawn_run, dialog);
    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_main_encoding_spawn_progress, dialog);

    gtk_widget_destroy (dialog);

    ogmrip_main_clean (data, encoding, result == OGMJOB_RESULT_ERROR);

    if (cookie >= 0)
      ogmrip_main_dbus_uninhibit (data, cookie);

    ogmrip_title_close (ogmrip_encoding_get_title (encoding));

    if (result == OGMJOB_RESULT_ERROR || error != NULL)
    {
      ogmrip_main_display_error (data, encoding, error);
      g_clear_error (&error);
    }
  }
}

/*
 * Imports a simple chapter file
 */
static gboolean
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
        ogmrip_chapter_store_set_label (OGMRIP_CHAPTER_STORE (data->chapter_store), chap - 1, str + 1);
      }
    }
  }

  fclose (file);

  return TRUE;
}

/*
 * Imports a matroska chapter file
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
          ogmrip_chapter_store_set_label (OGMRIP_CHAPTER_STORE (data->chapter_store), chap, (gchar *) str);
          chap ++;
        }
      }
    }
  }

  xmlFreeDoc (doc);

  return TRUE;
}

/*
 * Exports to a simple chapter file
 */
static void
ogmrip_main_export_simple_chapters (OGMRipData *data, const gchar *filename)
{
  guint start_chap;
  gint end_chap;

  if (ogmrip_chapter_store_get_selection (OGMRIP_CHAPTER_STORE (data->chapter_store), &start_chap, &end_chap))
  {
    GError *error = NULL;
    OGMRipTitle *title;
    OGMRipCodec *chapters;
    gchar *label;
    guint i;

    title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (data->title_chooser));

    chapters = ogmrip_chapters_new (ogmrip_title_get_video_stream (title));
    ogmrip_codec_set_chapters (OGMRIP_CODEC (chapters), start_chap, end_chap);

    for (i = 0; ; i++)
    {
      label = ogmrip_chapter_store_get_label (OGMRIP_CHAPTER_STORE (data->chapter_store), i);
      if (!label)
        break;
      ogmrip_chapters_set_label (OGMRIP_CHAPTERS (chapters), i, label);
      g_free (label);
    }

    if (ogmjob_spawn_run (OGMJOB_SPAWN (chapters), &error) != OGMJOB_RESULT_SUCCESS)
    {
/*
      ogmrip_message_dialog (GTK_WINDOW (data->window), GTK_MESSAGE_ERROR, "<b>%s</b>\n\n%s",
          _("Could not export the chapters"), error->message);
*/
      g_clear_error (&error);
    }
  }
}

/*
 * Adds an audio chooser
 */
static void
ogmrip_main_add_audio_chooser (OGMRipData *data)
{
  GtkWidget *chooser;
  OGMRipTitle *title;

  title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (data->title_chooser));

  chooser = ogmrip_audio_chooser_widget_new ();
  ogmrip_source_chooser_set_title (OGMRIP_SOURCE_CHOOSER (chooser), title);
  gtk_container_add (GTK_CONTAINER (data->audio_list), chooser);
  gtk_widget_show (chooser);

  if (title)
  {
    ogmrip_source_chooser_select_language (OGMRIP_SOURCE_CHOOSER (chooser),
        g_settings_get_uint (settings, OGMRIP_SETTINGS_PREF_AUDIO));

    if (!ogmrip_source_chooser_get_active (OGMRIP_SOURCE_CHOOSER (chooser)))
    {
      OGMRipAudioStream *stream = NULL;

      if (ogmrip_title_get_n_audio_streams (title) > 0)
        stream = ogmrip_title_get_nth_audio_stream (title, 0);

      ogmrip_source_chooser_set_active (OGMRIP_SOURCE_CHOOSER (chooser), stream ? OGMRIP_STREAM (stream) : NULL);
    }
  }

  g_signal_connect_swapped (chooser, "add-clicked",
      G_CALLBACK (ogmrip_main_add_audio_chooser), data);
  g_signal_connect_swapped (chooser, "remove-clicked",
      G_CALLBACK (gtk_container_remove), data->audio_list);
}

/*
 * Adds a subp chooser
 */
static void
ogmrip_main_add_subp_chooser (OGMRipData *data)
{
  GtkWidget *chooser;
  OGMRipTitle *title;

  title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (data->title_chooser));

  chooser = ogmrip_subp_chooser_widget_new ();
  ogmrip_source_chooser_set_title (OGMRIP_SOURCE_CHOOSER (chooser), title);
  gtk_container_add (GTK_CONTAINER (data->subp_list), chooser);
  gtk_widget_show (chooser);

  if (title)
  {
    ogmrip_source_chooser_select_language (OGMRIP_SOURCE_CHOOSER (chooser),
        g_settings_get_uint (settings, OGMRIP_SETTINGS_PREF_SUBP));

    if (!ogmrip_source_chooser_get_active (OGMRIP_SOURCE_CHOOSER (chooser)))
      ogmrip_source_chooser_set_active (OGMRIP_SOURCE_CHOOSER (chooser), NULL);
  }

  g_signal_connect_swapped (chooser, "add-clicked",
      G_CALLBACK (ogmrip_main_add_subp_chooser), data);
  g_signal_connect_swapped (chooser, "remove-clicked",
      G_CALLBACK (gtk_container_remove), data->subp_list);
}

static const gchar *fourcc[] =
{
  NULL,
  "XVID",
  "DIVX",
  "DX50",
  "FMP4"
};

/*
 * Creates a container
 */
static OGMRipContainer *
ogmrip_main_create_container (OGMRipData *data, OGMRipProfile *profile)
{
  OGMRipContainer *container;
  GSettings *settings;
  GType type;
  gchar *name;

  ogmrip_profile_get (profile, OGMRIP_PROFILE_GENERAL, OGMRIP_PROFILE_CONTAINER, "s", &name);
  type = ogmrip_type_from_name (name);
  g_free (name);

  g_assert (type != G_TYPE_NONE);

  container = g_object_new (type, NULL);
  ogmrip_container_set_label (container,
      gtk_entry_get_text (GTK_ENTRY (data->title_entry)));

  settings = ogmrip_profile_get_child (profile, OGMRIP_PROFILE_GENERAL);

  ogmrip_container_set_fourcc (container,
      fourcc[g_settings_get_uint (settings, OGMRIP_PROFILE_FOURCC)]);
  ogmrip_container_set_split (container,
      g_settings_get_uint (settings, OGMRIP_PROFILE_TARGET_NUMBER),
      g_settings_get_uint (settings, OGMRIP_PROFILE_TARGET_SIZE));

  g_object_unref (settings);

  return container;
}

/*
 * Creates a video codec
 */
static OGMRipCodec *
ogmrip_main_create_video_codec (OGMRipData *data, OGMRipProfile *profile,
    OGMRipVideoStream *stream, guint start_chap, gint end_chap)
{
  OGMRipCodec *codec;
  GSettings *settings;
  GType type;
  guint w, h, m;
  gchar *name;

  ogmrip_profile_get (profile, OGMRIP_PROFILE_VIDEO, OGMRIP_PROFILE_CODEC, "s", &name);
  type = ogmrip_type_from_name (name);
  g_free (name);

  if (type == G_TYPE_NONE)
    return NULL;

  codec = g_object_new (type, "input", stream, NULL);

  ogmrip_codec_set_chapters (codec, start_chap, end_chap);

  settings = ogmrip_profile_get_child (profile, OGMRIP_PROFILE_VIDEO);

  ogmrip_video_codec_set_angle (OGMRIP_VIDEO_CODEC (codec),
      gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->angle_spin)));
  ogmrip_video_codec_set_passes (OGMRIP_VIDEO_CODEC (codec),
      g_settings_get_uint (settings, OGMRIP_PROFILE_PASSES));
  ogmrip_video_codec_set_scaler (OGMRIP_VIDEO_CODEC (codec),
      g_settings_get_uint (settings, OGMRIP_PROFILE_SCALER));
  ogmrip_video_codec_set_turbo (OGMRIP_VIDEO_CODEC (codec),
      g_settings_get_boolean (settings, OGMRIP_PROFILE_TURBO));
  ogmrip_video_codec_set_denoise (OGMRIP_VIDEO_CODEC (codec),
      g_settings_get_boolean (settings, OGMRIP_PROFILE_DENOISE));
  ogmrip_video_codec_set_deblock (OGMRIP_VIDEO_CODEC (codec),
      g_settings_get_boolean (settings, OGMRIP_PROFILE_DEBLOCK));
  ogmrip_video_codec_set_dering (OGMRIP_VIDEO_CODEC (codec),
      g_settings_get_boolean (settings, OGMRIP_PROFILE_DERING));
  ogmrip_video_codec_set_quality (OGMRIP_VIDEO_CODEC (codec),
      g_settings_get_uint (settings, OGMRIP_PROFILE_QUALITY));

  w = g_settings_get_uint (settings, OGMRIP_PROFILE_MIN_WIDTH);
  h = g_settings_get_uint (settings, OGMRIP_PROFILE_MIN_HEIGHT);

  if (w > 0 && h > 0)
    ogmrip_video_codec_set_min_size (OGMRIP_VIDEO_CODEC (codec), w, h);

  w = g_settings_get_uint (settings, OGMRIP_PROFILE_MAX_WIDTH);
  h = g_settings_get_uint (settings, OGMRIP_PROFILE_MAX_HEIGHT);

  if (w > 0 && h > 0)
    ogmrip_video_codec_set_max_size (OGMRIP_VIDEO_CODEC (codec), w, h,
        g_settings_get_boolean (settings, OGMRIP_PROFILE_EXPAND));

  ogmrip_profile_get (profile, OGMRIP_PROFILE_GENERAL, OGMRIP_PROFILE_ENCODING_METHOD, "u", &m);
  if (m == OGMRIP_ENCODING_BITRATE)
    ogmrip_video_codec_set_bitrate (OGMRIP_VIDEO_CODEC (codec),
        g_settings_get_uint (settings, OGMRIP_PROFILE_BITRATE));
  else if (m == OGMRIP_ENCODING_QUANTIZER)
    ogmrip_video_codec_set_quantizer (OGMRIP_VIDEO_CODEC (codec),
        g_settings_get_double (settings, OGMRIP_PROFILE_QUANTIZER));

  g_object_unref (settings);

  return codec;
}

static const gint sample_rate[] =
{ 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000 };

/*
 * Creates an audio codec
 */
static OGMRipCodec *
ogmrip_main_create_audio_codec (OGMRipData *data, OGMRipProfile *profile,
    OGMRipSourceChooser *chooser, OGMRipAudioStream *stream, guint start_chap, gint end_chap)
{
  OGMRipAudioOptions *options;
  OGMRipCodec *codec;
  GType type;

  options = ogmrip_audio_chooser_widget_get_options (OGMRIP_AUDIO_CHOOSER_WIDGET (chooser));
  if (options)
    type = ogmrip_audio_options_get_codec (options);
  else
  {
    gchar *name;

    ogmrip_profile_get (profile, OGMRIP_PROFILE_AUDIO, OGMRIP_PROFILE_CODEC, "s", &name);
    type = ogmrip_type_from_name (name);
    g_free (name);
  }

  if (type == G_TYPE_NONE)
    return NULL;

  codec = g_object_new (type, "input", stream, NULL);
  if (!codec)
    return NULL;

  ogmrip_codec_set_chapters (codec, start_chap, end_chap);
  ogmrip_audio_codec_set_label (OGMRIP_AUDIO_CODEC (codec),
      ogmrip_audio_chooser_widget_get_label (OGMRIP_AUDIO_CHOOSER_WIDGET (chooser)));
  ogmrip_audio_codec_set_language (OGMRIP_AUDIO_CODEC (codec),
      ogmrip_audio_chooser_widget_get_language (OGMRIP_AUDIO_CHOOSER_WIDGET (chooser)));

  if (options)
  {
    ogmrip_audio_codec_set_channels (OGMRIP_AUDIO_CODEC (codec),
        ogmrip_audio_options_get_channels (options));
    ogmrip_audio_codec_set_normalize (OGMRIP_AUDIO_CODEC (codec),
        ogmrip_audio_options_get_normalize (options));
    ogmrip_audio_codec_set_quality (OGMRIP_AUDIO_CODEC (codec),
        ogmrip_audio_options_get_quality (options));
    ogmrip_audio_codec_set_sample_rate (OGMRIP_AUDIO_CODEC (codec),
        sample_rate[ogmrip_audio_options_get_sample_rate (options)]);
  }
  else
  {
    GSettings *settings;

    settings = ogmrip_profile_get_child (profile, OGMRIP_PROFILE_AUDIO);

    ogmrip_audio_codec_set_channels (OGMRIP_AUDIO_CODEC (codec),
        g_settings_get_uint (settings, OGMRIP_PROFILE_CHANNELS));
    ogmrip_audio_codec_set_normalize (OGMRIP_AUDIO_CODEC (codec),
        g_settings_get_boolean (settings, OGMRIP_PROFILE_NORMALIZE));
    ogmrip_audio_codec_set_quality (OGMRIP_AUDIO_CODEC (codec),
        g_settings_get_uint (settings, OGMRIP_PROFILE_QUALITY));
    ogmrip_audio_codec_set_sample_rate (OGMRIP_AUDIO_CODEC (codec),
        sample_rate[g_settings_get_uint (settings, OGMRIP_PROFILE_SAMPLERATE)]);

    g_object_unref (settings);
  }

  return codec;
}

/*
 * Creates a subp codec
 */
static OGMRipCodec *
ogmrip_main_create_subp_codec (OGMRipData *data, OGMRipProfile *profile,
    OGMRipSourceChooser *chooser, OGMRipSubpStream *stream, guint start_chap, gint end_chap)
{
  OGMRipSubpOptions *options;
  OGMRipCodec *codec;
  GType type;

  options = ogmrip_subp_chooser_widget_get_options (OGMRIP_SUBP_CHOOSER_WIDGET (chooser));
  if (options)
    type = ogmrip_subp_options_get_codec (options);
  else
  {
    gchar *name;

    ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_CODEC, "s", &name);
    type = ogmrip_type_from_name (name);
    g_free (name);
  }

  if (type == G_TYPE_NONE)
    return NULL;

  codec = g_object_new (type, "input", stream, NULL);
  if (!codec)
    return NULL;

  ogmrip_codec_set_chapters (codec, start_chap, end_chap);
  ogmrip_subp_codec_set_label (OGMRIP_SUBP_CODEC (codec),
      ogmrip_subp_chooser_widget_get_label (OGMRIP_SUBP_CHOOSER_WIDGET (chooser)));
  ogmrip_subp_codec_set_language (OGMRIP_SUBP_CODEC (codec),
      ogmrip_subp_chooser_widget_get_language (OGMRIP_SUBP_CHOOSER_WIDGET (chooser)));

  if (options)
  {
    ogmrip_subp_codec_set_charset (OGMRIP_SUBP_CODEC (codec),
        ogmrip_subp_options_get_charset (options));
    ogmrip_subp_codec_set_newline (OGMRIP_SUBP_CODEC (codec),
        ogmrip_subp_options_get_newline (options));
    ogmrip_subp_codec_set_forced_only (OGMRIP_SUBP_CODEC (codec),
        ogmrip_subp_options_get_forced_only (options));
  }
  else
  {
    GSettings *settings;

    settings = ogmrip_profile_get_child (profile, OGMRIP_PROFILE_SUBP);

    ogmrip_subp_codec_set_charset (OGMRIP_SUBP_CODEC (codec),
        g_settings_get_uint (settings, OGMRIP_PROFILE_CHARACTER_SET));
    ogmrip_subp_codec_set_newline (OGMRIP_SUBP_CODEC (codec),
        g_settings_get_uint (settings, OGMRIP_PROFILE_NEWLINE_STYLE));
    ogmrip_subp_codec_set_forced_only (OGMRIP_SUBP_CODEC (codec),
        g_settings_get_boolean (settings, OGMRIP_PROFILE_FORCED_ONLY));

    g_object_unref (settings);
  }

  return codec;
}

/*
 * Creates a chapters codec
 */
static OGMRipCodec *
ogmrip_main_create_chapters_codec (OGMRipData *data, OGMRipVideoStream *stream, guint start_chap, gint end_chap)
{
  OGMRipCodec *codec;
  guint last_chap, chap;
  gchar *label;

  codec = ogmrip_chapters_new (stream);
  if (!codec)
    return NULL;

  ogmrip_chapters_set_language (OGMRIP_CHAPTERS (codec),
      g_settings_get_uint (settings, OGMRIP_SETTINGS_CHAPTER_LANG));

  ogmrip_codec_set_chapters (codec, start_chap, end_chap);

  ogmrip_codec_get_chapters (codec, &start_chap, &last_chap);
  for (chap = start_chap; chap <= last_chap; chap ++)
  {
    label = ogmrip_chapter_store_get_label (OGMRIP_CHAPTER_STORE (data->chapter_store), chap);
    if (!label)
      break;

    ogmrip_chapters_set_label (OGMRIP_CHAPTERS (codec), chap, label);
    g_free (label);
  }

  return codec;
}

/*
 * Sets the name of the output file
 */
static void
ogmrip_main_set_filename (OGMRipData *data, OGMRipEncoding *encoding)
{
  GString *filename;
  OGMRipContainer *container;
  OGMRipCodec *codec;
  gchar *path, *str;
  guint format;

  format = g_settings_get_uint (settings, OGMRIP_SETTINGS_FILENAME);

  filename = g_string_new (gtk_entry_get_text (GTK_ENTRY (data->title_entry)));

  if (format >= 1)
  {
    codec = ogmrip_encoding_get_nth_audio_codec (encoding, 0);
    if (codec)
    {
      gint lang;

      lang = ogmrip_audio_codec_get_language (OGMRIP_AUDIO_CODEC (codec));
      if (lang >= 0)
        g_string_append_printf (filename, " - %s", ogmrip_language_get_label (lang));
    }
  }

  if (format >= 2)
  {
    codec = ogmrip_encoding_get_video_codec (encoding);
    if (codec)
      g_string_append_printf (filename, " - %s", ogmrip_type_name (G_OBJECT_TYPE (codec)));
  }

  if (format >= 3)
  {
    codec = ogmrip_encoding_get_nth_audio_codec (encoding, 0);
    if (codec)
      g_string_append_printf (filename, " - %s", ogmrip_type_name (G_OBJECT_TYPE (codec)));
  }

  container = ogmrip_encoding_get_container (encoding);
  g_string_append_printf (filename, ".%s", ogmrip_type_name (G_OBJECT_TYPE (container)));

  path = g_settings_get_string (settings, OGMRIP_SETTINGS_OUTPUT_DIR);
  str = g_build_filename (path, filename->str, NULL);
  g_string_free (filename, TRUE);
  g_free (path);

  ogmrip_container_set_output (container, str);

  path = ogmrip_fs_set_extension (str, "log");
  g_free (str);

  ogmrip_encoding_set_log_file (encoding, path);
  g_free (path);
}

/*
 * When an encoding has completed
 */
static void
ogmrip_main_encoding_completed (OGMRipData *data, OGMJobSpawn *spawn, guint result, OGMRipEncoding *encoding)
{
  if (result == OGMJOB_RESULT_SUCCESS && spawn != NULL)
  {
    if (OGMRIP_IS_SUBP_CODEC (spawn))
    {
#ifdef HAVE_ENCHANT_SUPPORT
      if (ogmrip_codec_format (G_OBJECT_TYPE (spawn)) != OGMRIP_FORMAT_VOBSUB)
      {
        OGMRipProfile *profile;
        gboolean spell_check;

        profile = ogmrip_encoding_get_profile (encoding);
        ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_SPELL_CHECK, "b", &spell_check);

        if (spell_check)
          ogmrip_main_spell_check (data,
              ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (spawn))),
              ogmrip_subp_codec_get_language (OGMRIP_SUBP_CODEC (spawn)));
      }
#endif
    }
  }
}

/*
 * When the eject button is activated
 */
static void
ogmrip_main_eject_activated (OGMRipData *data, GtkWidget *dialog)
{
  if (data->media)
  {
    gchar *device;

    device = ogmdvd_drive_chooser_get_device (OGMDVD_DRIVE_CHOOSER (dialog), NULL);
    if (device)
    {
      const gchar *uri;

      uri = ogmrip_media_get_uri (data->media);
      if (!g_str_has_prefix (uri, "dvd://"))
        g_warning ("Unknown scheme for '%s'", uri);
      else if (g_str_equal (device, uri + 6))
      {
        g_object_unref (data->media);
        data->media = NULL;

        ogmrip_title_chooser_set_media (OGMRIP_TITLE_CHOOSER (data->title_chooser), NULL);
        gtk_entry_set_text (GTK_ENTRY (data->title_entry), "");
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
  GtkWidget *dialog;

  dialog = ogmdvd_drive_chooser_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->window));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

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

/*
 * When the extract button is activated
 */
static void
ogmrip_main_extract_activated (OGMRipData *data)
{
  GtkWidget *dialog;
  OGMRipTitle *title;

  OGMRipEncoding *encoding;
  OGMRipProfile *profile;
  OGMRipContainer *container;
  OGMRipCodec *vcodec, *codec;
  OGMRipStream *stream;

  guint start_chap;
  gint end_chap, response;

  GList *list, *link;

  title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (data->title_chooser));

  encoding = ogmrip_encoding_new (title);
  ogmrip_encoding_set_relative (encoding,
      gtk_widget_is_sensitive (data->relative_check) &
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->relative_check)));

  dialog = ogmrip_options_dialog_new (encoding);
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->window));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response != OGMRIP_RESPONSE_EXTRACT && response != OGMRIP_RESPONSE_ENQUEUE)
  {
    gtk_widget_destroy (dialog);
    g_object_unref (encoding);
    return;
  }

  profile = ogmrip_encoding_get_profile (encoding);

  ogmrip_encoding_set_copy (encoding,
      g_settings_get_boolean (settings, OGMRIP_SETTINGS_COPY_DVD));

  container = ogmrip_main_create_container (data, profile);
  ogmrip_encoding_set_container (encoding, container);
  g_object_unref (container);

  ogmrip_chapter_store_get_selection (OGMRIP_CHAPTER_STORE (data->chapter_store), &start_chap, &end_chap);

  vcodec = ogmrip_main_create_video_codec (data, profile,
      ogmrip_title_get_video_stream (title), start_chap, end_chap);

  if (vcodec)
  {
    guint x, y, w, h;
    glong t;
    gint d;

    ogmrip_options_dialog_get_crop_size (OGMRIP_OPTIONS_DIALOG (dialog), &x, &y, &w, &h);
    ogmrip_video_codec_set_crop_size (OGMRIP_VIDEO_CODEC (vcodec), x, y, w, h);

    ogmrip_options_dialog_get_scale_size (OGMRIP_OPTIONS_DIALOG (dialog), &w, &h);
    ogmrip_video_codec_set_scale_size (OGMRIP_VIDEO_CODEC (vcodec), w, h);

    d = ogmrip_options_dialog_get_deinterlacer (OGMRIP_OPTIONS_DIALOG (dialog));
    ogmrip_video_codec_set_deinterlacer (OGMRIP_VIDEO_CODEC (vcodec), d);

    ogmrip_encoding_set_video_codec (encoding, OGMRIP_VIDEO_CODEC (vcodec));
    g_object_unref (vcodec);

    t = g_settings_get_uint (settings, OGMRIP_SETTINGS_THREADS);
#ifdef HAVE_SYSCONF_NPROC
    if (!t)
      t = sysconf (_SC_NPROCESSORS_ONLN);
#endif
    ogmrip_video_codec_set_threads (OGMRIP_VIDEO_CODEC (vcodec), t ? t : 1);
  }

  gtk_widget_destroy (dialog);

  list = gtk_container_get_children (GTK_CONTAINER (data->audio_list));
  for (link = list; link; link = link->next)
  {
    stream = ogmrip_source_chooser_get_active (link->data);
    if (stream)
    {
      if (OGMRIP_IS_FILE (stream))
        ogmrip_encoding_add_file (encoding, OGMRIP_FILE (stream));
      else
      {
        codec = ogmrip_main_create_audio_codec (data, profile,
            OGMRIP_SOURCE_CHOOSER (link->data), OGMRIP_AUDIO_STREAM (stream), start_chap, end_chap);

        if (codec)
        {
          ogmrip_encoding_add_audio_codec (encoding, OGMRIP_AUDIO_CODEC (codec));
          g_object_unref (codec);
        }
      }
    }
  }
  g_list_free (list);

  list = gtk_container_get_children (GTK_CONTAINER (data->subp_list));
  for (link = list; link; link = link->next)
  {
    stream = ogmrip_source_chooser_get_active (link->data);
    if (stream)
    {
      if (OGMRIP_IS_FILE (stream))
        ogmrip_encoding_add_file (encoding, OGMRIP_FILE (stream));
      else
      {
        codec = ogmrip_main_create_subp_codec (data, profile,
            OGMRIP_SOURCE_CHOOSER (link->data), OGMRIP_SUBP_STREAM (stream), start_chap, end_chap);

        if (codec)
        {
          if (G_OBJECT_TYPE (codec) != OGMRIP_TYPE_HARDSUB)
            ogmrip_encoding_add_subp_codec (encoding, OGMRIP_SUBP_CODEC (codec));
          else if (vcodec)
            ogmrip_video_codec_set_hard_subp (OGMRIP_VIDEO_CODEC (vcodec), OGMRIP_SUBP_STREAM (stream),
                ogmrip_subp_codec_get_forced_only (OGMRIP_SUBP_CODEC (codec)));
          g_object_unref (codec);
        }
      }
    }
  }
  g_list_free (list);

  codec = ogmrip_main_create_chapters_codec (data,
      ogmrip_title_get_video_stream (title), start_chap, end_chap);

  if (codec)
  {
    ogmrip_encoding_add_chapters (encoding, OGMRIP_CHAPTERS (codec));
    g_object_unref (codec);
  }

  if (g_settings_get_boolean (settings, OGMRIP_SETTINGS_AUTO_SUBP) && 
      !ogmrip_encoding_get_nth_subp_codec (encoding, 0))
  {
    codec = ogmrip_encoding_get_nth_audio_codec (encoding, 0);
    if (codec)
    {
      list = ogmrip_title_get_subp_streams (ogmrip_encoding_get_title (encoding));
      for (link = list; link; link = link->next)
      {
        if (ogmrip_audio_codec_get_language (OGMRIP_AUDIO_CODEC (codec)) ==
            ogmrip_subp_stream_get_language (link->data))
        {
          codec = ogmrip_encoding_get_video_codec (encoding);
          ogmrip_video_codec_set_hard_subp (OGMRIP_VIDEO_CODEC (codec), link->data, TRUE);
          break;
        }
      }
      g_list_free (list);
    }
  }

  g_signal_connect_swapped (encoding, "complete",
      G_CALLBACK (ogmrip_main_encoding_completed), data);

  ogmrip_main_set_filename (data, encoding);

  if (response == OGMRIP_RESPONSE_EXTRACT)
    ogmrip_main_encode (data, encoding);
  else if (response == OGMRIP_RESPONSE_ENQUEUE)
  {
    ogmrip_encoding_manager_add (data->manager, encoding);
    g_object_unref (encoding);
  }
}

/*
 * When the play button is activated
 */
static void
ogmrip_main_play_activated (OGMRipData *data, GtkWidget *button)
{
  GError *error = NULL;

  OGMRipTitle *title;
  OGMRipStream *stream;
  GList *list, *link;

  guint start_chap;
  gint end_chap;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
  {
    ogmrip_player_stop (data->player);
    return;
  }

  title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (data->title_chooser));
  ogmrip_player_set_title (data->player, title);

  ogmrip_chapter_store_get_selection (OGMRIP_CHAPTER_STORE (data->chapter_store), &start_chap, &end_chap);
  ogmrip_player_set_chapters (data->player, start_chap, end_chap);

  list = gtk_container_get_children (GTK_CONTAINER (data->audio_list));
  for (link = list; link; link = link->next)
  {
    stream = ogmrip_source_chooser_get_active (link->data);
    if (stream)
    {
      ogmrip_player_set_audio_stream (data->player, OGMRIP_AUDIO_STREAM (stream));
      break;
    }
  }
  g_list_free (list);

  list = gtk_container_get_children (GTK_CONTAINER (data->subp_list));
  for (link = list; link; link = link->next)
  {
    stream = ogmrip_source_chooser_get_active (link->data);
    if (stream)
    {
      ogmrip_player_set_subp_stream (data->player, OGMRIP_SUBP_STREAM (stream));
      break;
    }
  }
  g_list_free (list);

  if (!ogmrip_player_play (data->player, &error))
  {
    ogmrip_run_error_dialog (GTK_WINDOW (data->window), error, _("Can't play DVD title"));
    g_clear_error (&error);
  }
}

/*
 * When the import menu item is activated
 */
static void
ogmrip_main_import_chapters_activated (OGMRipData *data)
{
  GtkWidget *dialog;
  GtkFileFilter *filter;

  dialog = gtk_file_chooser_dialog_new (_("Select a chapters file"),
      GTK_WINDOW (data->window), GTK_FILE_CHOOSER_ACTION_OPEN,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

  filter = gtk_file_filter_new ();
  gtk_file_filter_add_mime_type (filter, "text/*");
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    gchar *filename;

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    if (filename)
    {
      if (!ogmrip_main_import_matroska_chapters (data, filename))
        if (!ogmrip_main_import_simple_chapters (data, filename))
          ogmrip_run_error_dialog (GTK_WINDOW (data->window), NULL,
              _("Could not open the chapters file '%s'"), filename);
      g_free (filename);
    }
  }
  gtk_widget_destroy (dialog);
}

/*
 * When the export menu item is activated
 */
static void
ogmrip_main_export_chapters_activated (OGMRipData *data)
{
  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new (_("Select a file"),
      GTK_WINDOW (data->window), GTK_FILE_CHOOSER_ACTION_SAVE,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    gchar *filename;

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    if (filename)
    {
      ogmrip_main_export_simple_chapters (data, filename);
      g_free (filename);
    }
  }
  gtk_widget_destroy (dialog);
}

/*
 * When the preferences menu item is activated
 */
static void
ogmrip_main_pref_activated (OGMRipData *data)
{
  GtkWidget *dialog;

  dialog = ogmrip_pref_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->window));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/*
 * When the profiles menu item is activated
 */
static void
ogmrip_main_profiles_activated (OGMRipData *data)
{
  GtkWidget *dialog;

  dialog = ogmrip_profile_manager_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->window));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/*
 * When the encodings menu item is activated
 */
static void
ogmrip_main_encodings_activated (OGMRipData *data)
{
  GtkWidget *dialog;

  dialog = ogmrip_encoding_manager_dialog_new (data->manager);
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->window));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
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
      "copyright", "(c) 2004-2011 Olivier Rolland",
      "website", "http://ogmrip.sourceforge.net",
      "translator-credits", translator_credits,
      "documenters", documenters,
      "authors", authors,
      "logo", icon,
      NULL);
}

/*
 * When the active title changed
 */
static void
ogmrip_main_title_chooser_changed (OGMRipData *data)
{
  OGMRipTitle *title;
  gint angles;

  title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (data->title_chooser));
  gtk_widget_set_sensitive (data->title_chooser, title != NULL);

  gtk_container_clear (GTK_CONTAINER (data->audio_list));
  ogmrip_main_add_audio_chooser (data);

  gtk_container_clear (GTK_CONTAINER (data->subp_list));
  ogmrip_main_add_subp_chooser (data);

  ogmrip_chapter_store_set_title (OGMRIP_CHAPTER_STORE (data->chapter_store), title);
  ogmrip_chapter_store_select_all (OGMRIP_CHAPTER_STORE (data->chapter_store));

  angles = title ? ogmrip_title_get_n_angles (title) : 1;
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->angle_spin), 1);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (data->angle_spin), 1, angles);
  gtk_widget_set_visible (data->angle_spin, angles > 1);
}

/*
 * When the chapters selection changed
 */
static void
ogmrip_main_chapter_selection_changed (OGMRipData *data)
{
  gboolean sensitive;
  guint start_chap;
  gint end_chap;

  sensitive = ogmrip_chapter_store_get_selection (OGMRIP_CHAPTER_STORE (data->chapter_store), &start_chap, &end_chap);
  if (!sensitive)
    gtk_label_set_text (GTK_LABEL (data->length_label), "");
  else
  {
    OGMRipTitle *title;
    OGMRipTime time_;

    title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (data->title_chooser));
    if (ogmrip_title_get_chapters_length (title, start_chap, end_chap, &time_) > 0)
    {
      gchar *str;

      str = g_strdup_printf ("%02lu:%02lu:%02lu", time_.hour, time_.min, time_.sec);
      gtk_label_set_text (GTK_LABEL (data->length_label), str);
      g_free (str);
    }
  }

  gtk_action_set_sensitive (data->extract_action, sensitive); 
  gtk_widget_set_sensitive (data->relative_check, sensitive && (start_chap > 0 || end_chap != -1));
}

/*
 * When the main window receives the delete event
 */
static gboolean
ogmrip_main_delete_event (OGMRipData *data)
{
  return FALSE;
}

/*
 * When the main window is destroyed
 */
static void
ogmrip_main_destroyed (OGMRipData *data)
{
  if (data->media)
  {
    g_object_unref (data->media);
    data->media = NULL;
  }

  if (data->player)
  {
    g_object_unref (data->player);
    data->player = NULL;
  }
  
  if (data->manager)
  {
    g_object_unref (data->manager);
    data->manager = NULL;
  }

  g_free (data);
}

/*
 * When a menu item is selected
 */
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

/*
 * When the play signal is emitted
 */
static void
ogmrip_main_player_play (OGMRipPlayer *player, GtkWidget *button)
{
  GtkWidget *image;

  image = gtk_bin_get_child (GTK_BIN (button));
  gtk_container_remove (GTK_CONTAINER (button), image);

  image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_MENU);

  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);
}

/*
 * When the stop signal is emitted
 */
static void
ogmrip_main_player_stop (OGMRipPlayer *player, GtkWidget *button)
{
  GtkWidget *image;

  image = gtk_bin_get_child (GTK_BIN (button));
  gtk_container_remove (GTK_CONTAINER (button), image);

  image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_MENU);

  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
}

/*
 * When a menu item is deselected
 */
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
  gtk_window_set_icon_from_file (GTK_WINDOW (data->window),
      OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_ICON_FILE, NULL);

  g_signal_connect_swapped (data->window, "delete-event",
      G_CALLBACK (ogmrip_main_delete_event), data);
  g_signal_connect_swapped (data->window, "destroy",
      G_CALLBACK (ogmrip_main_destroyed), data);
  g_signal_connect (data->window, "destroy",
      G_CALLBACK (gtk_main_quit), NULL);

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group,
      action_entries, G_N_ELEMENTS (action_entries), NULL);

  ui_manager = gtk_ui_manager_new ();

  widget = gtk_builder_get_widget (builder, "statusbar");
  g_signal_connect (ui_manager, "connect-proxy", G_CALLBACK (ogmrip_main_connect_proxy), widget);

  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_file (ui_manager,
      OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_UI_FILE, NULL);

  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (data->window), accel_group);

  child = gtk_bin_get_child (GTK_BIN (data->window));

  widget = gtk_ui_manager_get_widget (ui_manager, "/Menubar");
  gtk_box_pack_start (GTK_BOX (child), widget, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (child), widget, 0);

  action = gtk_action_group_get_action (action_group, "Quit");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (gtk_widget_destroy), data->window);

  action = gtk_action_group_get_action (action_group, "Preferences");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_main_pref_activated), data);

  action = gtk_action_group_get_action (action_group, "Profiles");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_main_profiles_activated), data);

  action = gtk_action_group_get_action (action_group, "Encodings");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_main_encodings_activated), data);

  action = gtk_action_group_get_action (action_group, "About");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_main_about_activated), data);

  widget = gtk_builder_get_widget (builder, "hbox");

  data->title_chooser = ogmrip_title_chooser_widget_new ();
  gtk_box_pack_start (GTK_BOX (widget), data->title_chooser, TRUE, TRUE, 0);
  gtk_widget_show (data->title_chooser);

  g_signal_connect_swapped (data->title_chooser, "changed",
      G_CALLBACK (ogmrip_main_title_chooser_changed), data);

  action = gtk_action_group_get_action (action_group, "OpenChapters");
  g_object_bind_property (data->title_chooser, "sensitive",
      action, "sensitive", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_main_import_chapters_activated), data);

  action = gtk_action_group_get_action (action_group, "SaveChapters");
  g_object_bind_property (data->title_chooser, "sensitive",
      action, "sensitive", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_main_export_chapters_activated), data);

  action =  gtk_action_group_get_action (action_group, "Load");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_main_load_activated), data);

  widget = gtk_builder_get_widget (builder, "load-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_main_load_activated), data);

  data->extract_action =  gtk_action_group_get_action (action_group, "Extract");
  g_signal_connect_swapped (data->extract_action, "activate",
      G_CALLBACK (ogmrip_main_extract_activated), data);

  widget = gtk_builder_get_widget (builder, "extract-button");
  g_object_bind_property (data->extract_action, "sensitive",
      widget, "sensitive", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_main_extract_activated), data);

  widget = gtk_builder_get_widget (builder, "play-button");
  g_object_bind_property (data->extract_action, "sensitive",
      widget, "sensitive", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (widget, "toggled",
      G_CALLBACK (ogmrip_main_play_activated), data);

  data->player = ogmrip_player_new ();
  g_signal_connect (data->player, "play",
      G_CALLBACK (ogmrip_main_player_play), widget);
  g_signal_connect (data->player, "stop",
      G_CALLBACK (ogmrip_main_player_stop), widget);

  widget = gtk_builder_get_widget (builder, "table");

  data->audio_list = gtk_vbox_new (TRUE, 6);
  gtk_table_attach (GTK_TABLE (widget), data->audio_list, 1, 3, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_widget_show (data->audio_list);

  g_object_bind_property (data->title_chooser, "sensitive",
      data->audio_list, "sensitive", G_BINDING_SYNC_CREATE);

  data->subp_list = gtk_vbox_new (TRUE, 6);
  gtk_table_attach (GTK_TABLE (widget), data->subp_list, 1, 3, 3, 4, GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_widget_show (data->subp_list);

  g_object_bind_property (data->title_chooser, "sensitive",
      data->subp_list, "sensitive", G_BINDING_SYNC_CREATE);

  data->length_label = gtk_builder_get_widget (builder, "length-label");

  data->title_entry = gtk_builder_get_widget (builder, "title-entry");
  g_object_bind_property (data->title_chooser, "sensitive",
      data->title_entry, "sensitive", G_BINDING_SYNC_CREATE);

  widget = gtk_builder_get_widget (builder, "scrolledwindow");

  child = ogmrip_chapter_view_new ();
  gtk_container_add (GTK_CONTAINER (widget), child);
  gtk_widget_show (child);

  g_object_bind_property (data->title_chooser, "sensitive",
      child, "sensitive", G_BINDING_SYNC_CREATE);

  data->chapter_store = gtk_tree_view_get_model (GTK_TREE_VIEW (child));

  g_signal_connect_swapped (data->chapter_store, "selection-changed", 
      G_CALLBACK (ogmrip_main_chapter_selection_changed), data);

  action = gtk_action_group_get_action (action_group, "SelectAll");
  g_object_bind_property (data->title_chooser, "sensitive",
      action, "sensitive", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_chapter_store_select_all), data->chapter_store);

  action = gtk_action_group_get_action (action_group, "DeselectAll");
  g_object_bind_property (data->title_chooser, "sensitive",
      action, "sensitive", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_chapter_store_deselect_all), data->chapter_store);

  data->relative_check = gtk_builder_get_widget (builder, "relative-check");
  data->angle_spin = gtk_builder_get_widget (builder, "angle-spin");

  data->manager = ogmrip_encoding_manager_new ();

  g_object_unref (builder);

  ogmrip_main_title_chooser_changed (data);

  return data;
}

#ifdef G_ENABLE_DEBUG
static gboolean debug = TRUE;
#else
static gboolean debug = FALSE;
#endif

static void
ogmrip_init (void)
{
  OGMRipProfileEngine *engine;
  gchar *path;

  ogmrip_settings_init ();

  // ogmrip_plugin_init ();
  // ogmrip_options_plugin_init ();

#ifdef HAVE_LIBNOTIFY_SUPPORT
  notify_init (PACKAGE_NAME);
#endif /* HAVE_LIBNOTIFY_SUPPORT */

  engine = ogmrip_profile_engine_get_default ();

  path = g_build_filename (OGMRIP_DATA_DIR, "ogmrip", "profiles", NULL);
  ogmrip_profile_engine_add_path (engine, path);
  g_free (path);

  path = g_build_filename (g_get_user_data_dir (), "ogmrip", "profiles", NULL);
  ogmrip_profile_engine_add_path (engine, path);
  g_free (path);
}

static void
ogmrip_uninit (void)
{
  OGMRipProfileEngine *engine;

  engine = ogmrip_profile_engine_get_default ();
  g_object_unref (engine);

  ogmrip_settings_uninit ();

  // ogmrip_options_plugin_uninit ();
  // ogmrip_plugin_uninit ();
}

static GOptionEntry opts[] =
{
  { "debug", 0,  0, G_OPTION_ARG_NONE, &debug, "Enable debug messages", NULL },
  { NULL,    0,  0, 0,                 NULL,   NULL,                    NULL }
};

int
main (int argc, char *argv[])
{
  OGMRipData *data;

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */

  if (!gtk_init_with_args (&argc, &argv, "<DVD DEVICE>", opts, GETTEXT_PACKAGE, NULL))
    return EXIT_FAILURE;

  if (debug)
    ogmjob_log_set_print_stdout (TRUE);

  ogmrip_init ();

  data = ogmrip_main_new ();

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

  gtk_window_present (GTK_WINDOW (data->window));

  gtk_main ();

  ogmrip_uninit ();

  return EXIT_SUCCESS;
}

