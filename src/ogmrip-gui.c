/* OGMRip - A media encoder for GNOME
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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
#include "ogmrip-gui.h"
#include "ogmrip-helper.h"
#include "ogmrip-options-dialog.h"
#include "ogmrip-pref-dialog.h"
#include "ogmrip-settings.h"

#include <ogmrip-mplayer.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <stdlib.h>

#ifdef HAVE_ENCHANT_SUPPORT
#include "ogmrip-spell-dialog.h"
#endif /* HAVE_ENCHANT_SUPPORT */

#include <stdlib.h>

#ifdef G_ENABLE_DEBUG
static gboolean debug = TRUE;
#else
static gboolean debug = FALSE;
#endif

#define OGMRIP_MENU_FILE "ogmrip"  G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-menu.ui"
#define OGMRIP_UI_FILE   "ogmrip"  G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-main-window.ui"
#define OGMRIP_ICON_FILE "pixmaps" G_DIR_SEPARATOR_S "ogmrip.png"

#define OGMRIP_DEFAULT_FILE_NAME "movie"

typedef struct
{
  OGMRipMedia *media;
  gboolean prepared;

  GtkWidget *window;

  GtkWidget *title_entry;
  GtkWidget *title_chooser;
  GtkWidget *length_label;

  GtkWidget *audio_list;
  GtkWidget *subp_list;

  GtkWidget *relative_check;
  GtkWidget *angle_spin;

  GAction *load_action;
  GAction *extract_action;

  GtkTreeModel *chapter_store;

  OGMRipEncodingManager *manager;
  OGMRipPlayer *player;
} OGMRipData;

GSettings *settings;

static void
gtk_container_clear (GtkContainer *container)
{
  GList *list, *link;

  g_return_if_fail (GTK_IS_CONTAINER (container));

  list = gtk_container_get_children (container);
  for (link = list; link; link = link->next)
    gtk_container_remove (container, GTK_WIDGET (link->data));
  g_list_free (list);
}

#ifdef HAVE_ENCHANT_SUPPORT
/*
 * Performs spell checking
 */
static void
ogmrip_gui_spell_check (OGMRipData *data, const gchar *filename, gint lang)
{
  GtkWidget *dialog;

  GIOChannel *input = NULL, *output = NULL;
  GIOStatus status;

  gboolean retval = FALSE;
  gchar *text, *corrected;
  gchar *new_file;

  new_file = ogmrip_fs_mktemp ("sub.XXXXXX", NULL);
  if (!new_file)
    goto spell_check_cleanup;

  input = g_io_channel_new_file (filename, "r", NULL);
  if (!input)
    goto spell_check_cleanup;

  output = g_io_channel_new_file (new_file, "w", NULL);
  if (!output)
    goto spell_check_cleanup;

  dialog = ogmrip_spell_dialog_new (ogmrip_language_to_iso639_1 (lang));
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
    g_rename (new_file, filename);
  else
    g_unlink (new_file);

  g_free (new_file);
}
#endif /* HAVE_ENCHANT_SUPPORT */

static void
ogmrip_gui_load_progress_cb (OGMRipMedia *media, gdouble percent, gpointer pbar)
{
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), percent);
}

static gboolean
ogmrip_gui_open_media (OGMRipData *data, OGMRipMedia *media)
{
  GtkWidget *dialog, *area, *box, *label, *pbar;
  GCancellable *cancellable;
  gboolean res;

  dialog = gtk_dialog_new_with_buttons (_("Opening media"),
      GTK_WINDOW (data->window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (box), 6);
  gtk_container_add (GTK_CONTAINER (area), box);

  label = gtk_label_new (_("Opening media"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, -1.0);
  gtk_container_add (GTK_CONTAINER (box), label);

  pbar = gtk_progress_bar_new ();
  gtk_widget_set_size_request (pbar, 300, -1);
  gtk_container_add (GTK_CONTAINER (box), pbar);

  gtk_widget_show_all (dialog);

  cancellable = g_cancellable_new ();
  g_signal_connect_swapped (dialog, "response",
      G_CALLBACK (g_cancellable_cancel), cancellable);

  res = ogmrip_media_open (media, cancellable, ogmrip_gui_load_progress_cb, pbar, NULL);

  g_object_unref (cancellable);
  gtk_widget_destroy (dialog);

  if (!res && !g_cancellable_is_cancelled (cancellable))
    ogmrip_run_error_dialog (GTK_WINDOW (data->window), NULL, _("Could not open the media"));

  return res;
}

/*
 * Loads a media
 */
static void
ogmrip_gui_load_media (OGMRipData *data, OGMRipMedia *media)
{
  if (ogmrip_gui_open_media (data, media))
  {
    gint nvid;
    const gchar *label = NULL;

    if (data->media)
    {
      ogmrip_media_close (data->media);
      g_object_unref (data->media);
    }
    data->media = g_object_ref (media);

    ogmrip_title_chooser_set_media (OGMRIP_TITLE_CHOOSER (data->title_chooser), media);

    nvid = ogmrip_media_get_n_titles (media);
    if (nvid > 0)
      label = ogmrip_media_get_label (media);

    gtk_entry_set_text (GTK_ENTRY (data->title_entry),
        label ? label : _("Untitled"));

    g_simple_action_set_enabled (G_SIMPLE_ACTION (data->extract_action), data->prepared);
  }
}

static gboolean
ogmrip_gui_load_path (OGMRipData *data, const gchar *path)
{
  OGMRipMedia *media;

  media = ogmrip_media_new (path);
  if (!media)
  {
    ogmrip_run_error_dialog (GTK_WINDOW (data->window), NULL, _("Could not open the media"));
    return FALSE;
  }

  ogmrip_gui_load_media (data, media);
  g_object_unref (media);

  return TRUE;
}

/*
 * Cleans an encoding
 */
static void
ogmrip_gui_clean (OGMRipData *data, OGMRipEncoding *encoding, gboolean error)
{
  gboolean temporary, log, copy = FALSE;

  temporary = !g_settings_get_boolean (settings, OGMRIP_SETTINGS_KEEP_TMP);
  log = !error && !g_settings_get_boolean (settings, OGMRIP_SETTINGS_LOG_OUTPUT);

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
          _("Do you want to remove the copy of the media?"));
      after = gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES ?
        OGMRIP_AFTER_ENC_REMOVE : OGMRIP_AFTER_ENC_KEEP;
      gtk_widget_destroy (dialog);
    }

    copy = after == OGMRIP_AFTER_ENC_REMOVE;
  }

  ogmrip_encoding_clean (encoding, temporary, copy, log);

  g_object_unref (encoding);
}

/*
 * Displays the error message
 */
static void
ogmrip_gui_display_error (OGMRipData *data, OGMRipEncoding *encoding, GError *error)
{
  GtkWidget *dialog, *area, *expander, *widget;
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
    GtkTextIter iter;
    GFile *file;

    area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

    expander = gtk_expander_new_with_mnemonic ("_Show the log file");
    gtk_expander_set_expanded (GTK_EXPANDER (expander), FALSE);
    gtk_container_add (GTK_CONTAINER (area), expander);

    g_object_bind_property (expander, "expanded", dialog, "resizable", G_BINDING_DEFAULT);

    area = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (area), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (area), GTK_SHADOW_IN);
    gtk_container_add (GTK_CONTAINER (expander), area);
    gtk_widget_set_vexpand (area, TRUE);
    gtk_widget_show (area);

    widget = gtk_text_view_new ();
    gtk_text_view_set_editable (GTK_TEXT_VIEW (widget), FALSE);
    gtk_container_add (GTK_CONTAINER (area), widget);
    gtk_widget_show (widget);

    file = g_file_new_for_path (filename);

    stream = g_file_read (file, NULL, NULL);
    if (stream)
    {
      GtkTextBuffer *buffer;
      gchar text[2048];
      gssize n;

      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
      gtk_text_buffer_get_start_iter (buffer, &iter);

      while ((n = g_input_stream_read (G_INPUT_STREAM (stream), text, 2048, NULL, NULL)) > 0)
        gtk_text_buffer_insert (buffer, &iter, text, n);

      gtk_widget_set_visible (expander, gtk_text_buffer_get_char_count (buffer) > 0);

      g_object_unref (stream);
    }

    g_object_unref (file);
  }

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
ogmrip_gui_encoding_spawn_run (OGMRipEncoding *encoding, OGMJobSpawn *spawn, OGMRipProgressDialog *dialog)
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
      ogmrip_progress_dialog_set_message (dialog, _("Copying media"));
    else if (OGMRIP_IS_ANALYZE (spawn))
      ogmrip_progress_dialog_set_message (dialog, _("Analyzing video stream"));
    else if (OGMRIP_IS_TEST (spawn))
      ogmrip_progress_dialog_set_message (dialog, _("Running the compressibility test"));
    else if (OGMRIP_IS_AUDIO_CODEC (spawn))
    {
      stream = ogmrip_codec_get_input (OGMRIP_CODEC (spawn));
      message = g_strdup_printf (_("Extracting audio stream %d"),
          ogmrip_stream_get_id (stream) + 1);
      ogmrip_progress_dialog_set_message (dialog, message);
      g_free (message);
    }
    else if (OGMRIP_IS_SUBP_CODEC (spawn))
    {
      stream = ogmrip_codec_get_input (OGMRIP_CODEC (spawn));
      message = g_strdup_printf (_("Extracting subtitle stream %d"),
          ogmrip_stream_get_id (stream) + 1);
      ogmrip_progress_dialog_set_message (dialog, message);
      g_free (message);
    }
  }

  ogmrip_progress_dialog_set_fraction (dialog, -1.0);
}

static void
ogmrip_gui_encoding_spawn_progress (OGMRipEncoding *encoding, OGMJobSpawn *spawn, gdouble fraction, OGMRipProgressDialog *dialog)
{
  ogmrip_progress_dialog_set_fraction (dialog, fraction);
}

static void
ogmrip_gui_progress_dialog_response (GtkWidget *parent, gint response_id, OGMRipEncoding *encoding)
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
      {
        GCancellable *cancellable;

        cancellable = g_object_get_data (G_OBJECT (encoding), "cancellable");
        if (cancellable)
          g_cancellable_cancel (cancellable);
      }
      gtk_widget_destroy (dialog);
      break;
  }
}

static gboolean
ogmrip_gui_delete_output (OGMRipData *data, OGMRipEncoding *encoding, GCancellable *cancellable, GError **error)
{
  GtkWidget *dialog;
  GFile *output;

  gboolean response;
  gchar *filename;

  output = ogmrip_container_get_output (ogmrip_encoding_get_container (encoding));

  filename = g_file_get_parse_name (output);
  dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (data->window),
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, _("A file named '%s' already exists."), filename);
  g_free (filename);

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), _("Do you want to replace it?"));

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  if (response != GTK_RESPONSE_YES)
    return FALSE;

  if (!g_file_delete (output, cancellable, error))
    return FALSE;

  return TRUE;
}

/*
 * Encodes an encoding
 */
static void
ogmrip_gui_encode (OGMRipData *data, OGMRipEncoding *encoding)
{
  GError *error = NULL;
  gboolean result = FALSE;

  if (ogmrip_open_title (GTK_WINDOW (data->window), ogmrip_encoding_get_title (encoding), &error))
  {
    GCancellable *cancellable;
    GtkWidget *dialog;
    guint cookie;

    cookie = gtk_application_inhibit (gtk_window_get_application (GTK_WINDOW (data->window)),
        GTK_WINDOW (data->window), GTK_APPLICATION_INHIBIT_SUSPEND, _("Encoding a movie"));

    dialog = ogmrip_progress_dialog_new (GTK_WINDOW (data->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, TRUE);
    ogmrip_progress_dialog_set_title (OGMRIP_PROGRESS_DIALOG (dialog),
        ogmrip_container_get_label (ogmrip_encoding_get_container (encoding)));

    g_signal_connect (dialog, "response",
        G_CALLBACK (ogmrip_gui_progress_dialog_response), encoding);
    g_signal_connect (dialog, "delete-event",
        G_CALLBACK (gtk_true), NULL);

    gtk_window_present (GTK_WINDOW (dialog));

    g_signal_connect (encoding, "run",
        G_CALLBACK (ogmrip_gui_encoding_spawn_run), dialog);
    g_signal_connect (encoding, "progress",
        G_CALLBACK (ogmrip_gui_encoding_spawn_progress), dialog);

    cancellable = g_cancellable_new ();
    g_object_set_data_full (G_OBJECT (encoding), "cancellable", cancellable, g_object_unref);

    result = ogmrip_encoding_encode (encoding, cancellable, &error);
    if (!result && error && error->code == G_IO_ERROR_EXISTS)
    {
      gtk_widget_hide (dialog);

      g_clear_error (&error);
      if (ogmrip_gui_delete_output (data, encoding, cancellable, &error))
      {
        gtk_widget_show (dialog);
        result = ogmrip_encoding_encode (encoding, cancellable, &error);
      }
      else if (!error)
        result = TRUE;
    }

    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_gui_encoding_spawn_run, dialog);
    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_gui_encoding_spawn_progress, dialog);

    gtk_widget_destroy (dialog);

    if (cookie > 0)
      gtk_application_uninhibit (gtk_window_get_application (GTK_WINDOW (data->window)), cookie);

    ogmrip_title_close (ogmrip_encoding_get_title (encoding));
  }

  if (!result && (!error || error->code != G_IO_ERROR_CANCELLED))
    ogmrip_gui_display_error (data, encoding, error);

  ogmrip_gui_clean (data, encoding, !result && (!error || error->code != G_IO_ERROR_CANCELLED));

  if (error)
    g_error_free (error);
}

/*
 * Tests an encoding
 */
static void
ogmrip_gui_test (OGMRipData *data, OGMRipEncoding *encoding, guint *width, guint *height)
{
  GError *error = NULL;
  gboolean result = FALSE;

  if (ogmrip_open_title (GTK_WINDOW (data->window), ogmrip_encoding_get_title (encoding), &error))
  {
    GCancellable *cancellable;
    GtkWidget *dialog;
    guint cookie;

    cookie = gtk_application_inhibit (gtk_window_get_application (GTK_WINDOW (data->window)),
        GTK_WINDOW (data->window), GTK_APPLICATION_INHIBIT_SUSPEND, NULL);

    dialog = ogmrip_progress_dialog_new (GTK_WINDOW (data->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, TRUE);
    ogmrip_progress_dialog_set_title (OGMRIP_PROGRESS_DIALOG (dialog),
        ogmrip_container_get_label (ogmrip_encoding_get_container (encoding)));

    g_signal_connect (dialog, "response",
        G_CALLBACK (ogmrip_gui_progress_dialog_response), encoding);
    g_signal_connect (dialog, "delete-event",
        G_CALLBACK (gtk_true), NULL);

    gtk_window_present (GTK_WINDOW (dialog));

    g_signal_connect (encoding, "run",
        G_CALLBACK (ogmrip_gui_encoding_spawn_run), dialog);
    g_signal_connect (encoding, "progress",
        G_CALLBACK (ogmrip_gui_encoding_spawn_progress), dialog);

    cancellable = g_cancellable_new ();
    g_object_set_data_full (G_OBJECT (encoding), "cancellable", cancellable, g_object_unref);

    result = ogmrip_encoding_test (encoding, cancellable, &error);

    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_gui_encoding_spawn_run, dialog);
    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_gui_encoding_spawn_progress, dialog);

    gtk_widget_destroy (dialog);

    if (cookie > 0)
      gtk_application_uninhibit (gtk_window_get_application (GTK_WINDOW (data->window)), cookie);

    ogmrip_title_close (ogmrip_encoding_get_title (encoding));

    if (result)
    {
      OGMRipCodec *codec;

      codec = ogmrip_encoding_get_video_codec (encoding);
      ogmrip_video_codec_get_scale_size (OGMRIP_VIDEO_CODEC (codec), width, height);
    }
  }

  if (!result && (!error || error->code == G_IO_ERROR_CANCELLED))
    ogmrip_gui_display_error (data, encoding, error);

  ogmrip_gui_clean (data, encoding, !result && (!error || error->code == G_IO_ERROR_CANCELLED));

  if (error)
    g_error_free (error);

  ogmrip_encoding_clear (encoding);
}

/*
 * Imports a simple chapter file
 */
static gboolean
ogmrip_gui_import_simple_chapters (OGMRipData *data, GFile *gfile)
{
  FILE *file;
  gchar buf[201], *str;
  gint chap;

  str = g_file_get_path (gfile);
  file = fopen (str, "r");
  g_free (str);

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
ogmrip_gui_import_matroska_chapters (OGMRipData *data, GFile *file)
{
  OGMRipXML *xml;

  gint chap = 0;
  gchar *str;

  xml = ogmrip_xml_new_from_file (file, NULL);
  if (!xml)
    return FALSE;

  if (!g_str_equal (ogmrip_xml_get_name (xml), "Chapters"))
  {
    ogmrip_xml_free (xml);
    return FALSE;
  }

  if (!ogmrip_xml_children (xml))
  {
    ogmrip_xml_free (xml);
    return FALSE;
  }

  do
  {
    if (g_str_equal (ogmrip_xml_get_name (xml), "EditionEntry"))
      break;
  }
  while (ogmrip_xml_next (xml));

  if (!ogmrip_xml_children (xml))
  {
    ogmrip_xml_free (xml);
    return FALSE;
  }

  do
  {
    if (g_str_equal (ogmrip_xml_get_name (xml), "ChapterAtom"))
    {
      if (ogmrip_xml_children (xml))
      {
        do
        {
          if (g_str_equal (ogmrip_xml_get_name (xml), "ChapterDisplay"))
            break;
        }
        while (ogmrip_xml_next (xml));

        if (ogmrip_xml_children (xml))
        {
          do
          {
            if (g_str_equal (ogmrip_xml_get_name (xml), "ChapterString"))
            {
              str = ogmrip_xml_get_string (xml, NULL);
              ogmrip_chapter_store_set_label (OGMRIP_CHAPTER_STORE (data->chapter_store), chap ++, str);
              g_free (str);
            }
          }
          while (ogmrip_xml_next (xml));

          ogmrip_xml_parent (xml);
        }

        ogmrip_xml_parent (xml);
      }
    }
  }
  while (ogmrip_xml_next (xml));

  ogmrip_xml_free (xml);

  return TRUE;
}

/*
 * Exports to a simple chapter file
 */
static void
ogmrip_gui_export_simple_chapters (OGMRipData *data, const gchar *filename)
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

    if (!ogmjob_task_run (OGMJOB_TASK (chapters), NULL, &error))
    {
      GtkWidget *dialog;

      dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (data->window),
          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
          GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "<b>%s</b>", _("Could not export the chapters"));

      if (error)
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);

      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);

      g_clear_error (&error);
    }
  }
}

/*
 * Adds an audio chooser
 */
static void
ogmrip_gui_add_audio_chooser (OGMRipData *data)
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
      GList *list;

      list = ogmrip_title_get_audio_streams (title);
      ogmrip_source_chooser_set_active (OGMRIP_SOURCE_CHOOSER (chooser), list ? list->data : NULL);
      g_list_free (list);
    }
  }

  g_signal_connect_swapped (chooser, "add-clicked",
      G_CALLBACK (ogmrip_gui_add_audio_chooser), data);
  g_signal_connect_swapped (chooser, "remove-clicked",
      G_CALLBACK (gtk_container_remove), data->audio_list);
}

/*
 * Adds a subp chooser
 */
static void
ogmrip_gui_add_subp_chooser (OGMRipData *data)
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
      G_CALLBACK (ogmrip_gui_add_subp_chooser), data);
  g_signal_connect_swapped (chooser, "remove-clicked",
      G_CALLBACK (gtk_container_remove), data->subp_list);
}

/*
 * Creates a container
 */
static OGMRipContainer *
ogmrip_gui_create_container (OGMRipData *data, OGMRipProfile *profile)
{
  OGMRipContainer *container;

  container = ogmrip_container_new_from_profile (profile);
  ogmrip_container_set_label (container,
      gtk_entry_get_text (GTK_ENTRY (data->title_entry)));

  return container;
}

/*
 * Creates a video codec
 */
static OGMRipCodec *
ogmrip_gui_create_video_codec (OGMRipData *data, OGMRipProfile *profile,
    OGMRipVideoStream *stream, guint start_chap, gint end_chap)
{
  OGMRipCodec *codec;

  codec = ogmrip_video_codec_new_from_profile (stream, profile);
  if (!codec)
    return NULL;

  ogmrip_codec_set_chapters (codec, start_chap, end_chap);

  return codec;
}

static const gint sample_rate[] =
{ 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000 };

/*
 * Creates an audio codec
 */
static OGMRipCodec *
ogmrip_gui_create_audio_codec (OGMRipData *data, OGMRipProfile *profile,
    OGMRipSourceChooser *chooser, OGMRipAudioStream *stream, guint start_chap, gint end_chap)
{
  OGMRipAudioOptions *options;
  OGMRipCodec *codec;
  GType type;

  options = ogmrip_audio_chooser_widget_get_options (OGMRIP_AUDIO_CHOOSER_WIDGET (chooser));
  if (!options)
    codec = ogmrip_audio_codec_new_from_profile (stream, profile);
  else
  {
    type = ogmrip_audio_options_get_codec (options);
    if (type == G_TYPE_NONE)
      return NULL;
    codec = ogmrip_audio_codec_new (type, stream);
  }

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

  return codec;
}

/*
 * Creates a subp codec
 */
static OGMRipCodec *
ogmrip_gui_create_subp_codec (OGMRipData *data, OGMRipProfile *profile,
    OGMRipSourceChooser *chooser, OGMRipSubpStream *stream, guint start_chap, gint end_chap)
{
  OGMRipSubpOptions *options;
  OGMRipCodec *codec;
  GType type;

  options = ogmrip_subp_chooser_widget_get_options (OGMRIP_SUBP_CHOOSER_WIDGET (chooser));
  if (!options)
    codec = ogmrip_subp_codec_new_from_profile (stream, profile);
  else
  {
    type = ogmrip_subp_options_get_codec (options);
    if (type == G_TYPE_NONE)
      return NULL;
    codec = ogmrip_subp_codec_new (type, stream);
  }

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

  return codec;
}

/*
 * Creates a chapters codec
 */
static OGMRipCodec *
ogmrip_gui_create_chapters_codec (OGMRipData *data, OGMRipVideoStream *stream, guint start_chap, gint end_chap)
{
  OGMRipCodec *codec;
  guint last_chap, chap;
  gchar *label;

  if (!ogmrip_chapter_store_get_editable (OGMRIP_CHAPTER_STORE (data->chapter_store)))
    return NULL;

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
ogmrip_gui_set_filename (OGMRipData *data, OGMRipEncoding *encoding)
{
  GFile *file;
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
        g_string_append_printf (filename, " - %s", ogmrip_language_to_name (lang));
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

  file = g_file_new_for_path (str);
  ogmrip_container_set_output (container, file);
  g_object_unref (file);

  path = ogmrip_fs_set_extension (str, "log");
  g_free (str);

  ogmrip_encoding_set_log_file (encoding, path);
  g_free (path);
}

/*
 * When an encoding has completed
 */
static void
ogmrip_gui_encoding_completed (OGMRipData *data, OGMJobSpawn *spawn, OGMRipEncodingStatus status, OGMRipEncoding *encoding)
{
  if (spawn != NULL && status == OGMRIP_ENCODING_SUCCESS)
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
          ogmrip_gui_spell_check (data,
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
ogmrip_gui_eject_activated (OGMRipData *data, GtkWidget *dialog)
{
  if (data->media)
  {
    OGMRipMedia *media;

    media = ogmrip_media_chooser_get_media (OGMRIP_MEDIA_CHOOSER (dialog));
    if (media)
    {
      const gchar *uri;

      uri = ogmrip_media_get_uri (data->media);
      if ((g_str_has_prefix (uri, "dvd://") || g_str_has_prefix (uri, "br://")) &&
          g_str_equal (ogmrip_media_get_uri (media), uri))
      {
        g_object_unref (data->media);
        data->media = NULL;

        ogmrip_title_chooser_set_media (OGMRIP_TITLE_CHOOSER (data->title_chooser), NULL);
        gtk_entry_set_text (GTK_ENTRY (data->title_entry), "");
      }
    }
  }
}

/*
 * When the load button is activated
 */
static void
ogmrip_gui_load_activated (OGMRipData *data)
{
  GtkWidget *dialog;

  dialog = ogmrip_media_chooser_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->window));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  g_signal_connect_swapped (dialog, "eject", G_CALLBACK (ogmrip_gui_eject_activated), data);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    OGMRipMedia *media;

    gtk_widget_hide (dialog);

    media = ogmrip_media_chooser_get_media (OGMRIP_MEDIA_CHOOSER (dialog));
    if (media)
      ogmrip_gui_load_media (data, media);
  }
  gtk_widget_destroy (dialog);
}

static gboolean
ogmrip_gui_run_options_dialog (OGMRipData *data, OGMRipEncoding *encoding, guint width, guint height, GError **error)
{
  GtkWidget *dialog;
  OGMRipTitle *title;

  OGMRipProfile *profile;
  OGMRipContainer *container;
  OGMRipCodec *vcodec, *codec;
  OGMRipStream *stream;

  guint start_chap, method;
  gint end_chap, response;
  gboolean status;

  GList *list, *link;

  title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (data->title_chooser));

  if (encoding)
  {
    ogmrip_encoding_clear_audio_codecs (encoding);
    ogmrip_encoding_clear_subp_codecs (encoding);
    ogmrip_encoding_clear_files (encoding);
    ogmrip_encoding_clear_chapters (encoding);

    dialog = ogmrip_options_dialog_new_at_scale (encoding, width, height);
  }
  else
  {
    encoding = ogmrip_encoding_new (title);
    ogmrip_encoding_set_relative (encoding,
        gtk_widget_is_sensitive (data->relative_check) &
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->relative_check)));

    dialog = ogmrip_options_dialog_new (encoding);
  }

  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->window));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response != OGMRIP_RESPONSE_EXTRACT && response != OGMRIP_RESPONSE_ENQUEUE && response != OGMRIP_RESPONSE_TEST)
  {
    gtk_widget_destroy (dialog);
    g_object_unref (encoding);
    return TRUE;
  }

  profile = ogmrip_encoding_get_profile (encoding);

  ogmrip_encoding_set_copy (encoding,
      g_settings_get_boolean (settings, OGMRIP_SETTINGS_COPY_DVD));

  ogmrip_profile_get (profile, OGMRIP_PROFILE_GENERAL, OGMRIP_PROFILE_ENCODING_METHOD, "u", &method);
  ogmrip_encoding_set_method (encoding, method);

  ogmrip_profile_get (profile, OGMRIP_PROFILE_GENERAL, OGMRIP_PROFILE_ENSURE_SYNC, "b", &status);
  ogmrip_encoding_set_ensure_sync (encoding, status);

  container = ogmrip_gui_create_container (data, profile);
  status = ogmrip_encoding_set_container (encoding, container, error);
  g_object_unref (container);

  if (!status)
  {
    gtk_widget_destroy (dialog);
    g_object_unref (encoding);
    return FALSE;
  }

  ogmrip_chapter_store_get_selection (OGMRIP_CHAPTER_STORE (data->chapter_store), &start_chap, &end_chap);

  vcodec = ogmrip_gui_create_video_codec (data, profile,
      ogmrip_title_get_video_stream (title), start_chap, end_chap);

  if (vcodec)
  {
    guint x, y, w, h;
    gint d;

    ogmrip_options_dialog_get_crop_size (OGMRIP_OPTIONS_DIALOG (dialog), &x, &y, &w, &h);
    ogmrip_video_codec_set_crop_size (OGMRIP_VIDEO_CODEC (vcodec), x, y, w, h);

    ogmrip_options_dialog_get_scale_size (OGMRIP_OPTIONS_DIALOG (dialog), &w, &h);
    ogmrip_video_codec_set_scale_size (OGMRIP_VIDEO_CODEC (vcodec), w, h);

    d = ogmrip_options_dialog_get_deinterlacer (OGMRIP_OPTIONS_DIALOG (dialog));
    ogmrip_video_codec_set_deinterlacer (OGMRIP_VIDEO_CODEC (vcodec), d);

    status = ogmrip_encoding_set_video_codec (encoding, OGMRIP_VIDEO_CODEC (vcodec), error);
    g_object_unref (vcodec);

    if (!status)
    {
      gtk_widget_destroy (dialog);
      g_object_unref (encoding);
      return FALSE;
    }
  }

  gtk_widget_destroy (dialog);

  list = gtk_container_get_children (GTK_CONTAINER (data->audio_list));
  for (link = list; link && status; link = link->next)
  {
    stream = ogmrip_source_chooser_get_active (link->data);
    if (stream)
    {
      if (OGMRIP_IS_FILE (stream))
        status = ogmrip_encoding_add_file (encoding, OGMRIP_FILE (stream), error);
      else
      {
        codec = ogmrip_gui_create_audio_codec (data, profile,
            OGMRIP_SOURCE_CHOOSER (link->data), OGMRIP_AUDIO_STREAM (stream), start_chap, end_chap);

        if (codec)
        {
          status = ogmrip_encoding_add_audio_codec (encoding, OGMRIP_AUDIO_CODEC (codec), error);
          g_object_unref (codec);
        }
      }
    }
  }
  g_list_free (list);

  if (!status)
  {
    g_object_unref (encoding);
    return FALSE;
  }

  list = gtk_container_get_children (GTK_CONTAINER (data->subp_list));
  for (link = list; link && status; link = link->next)
  {
    stream = ogmrip_source_chooser_get_active (link->data);
    if (stream)
    {
      if (OGMRIP_IS_FILE (stream))
        status = ogmrip_encoding_add_file (encoding, OGMRIP_FILE (stream), error);
      else
      {
        codec = ogmrip_gui_create_subp_codec (data, profile,
            OGMRIP_SOURCE_CHOOSER (link->data), OGMRIP_SUBP_STREAM (stream), start_chap, end_chap);

        if (codec)
        {
          if (G_OBJECT_TYPE (codec) != OGMRIP_TYPE_HARDSUB)
            status = ogmrip_encoding_add_subp_codec (encoding, OGMRIP_SUBP_CODEC (codec), error);
          else if (vcodec)
            ogmrip_video_codec_set_hard_subp (OGMRIP_VIDEO_CODEC (vcodec), OGMRIP_SUBP_STREAM (stream),
                ogmrip_subp_codec_get_forced_only (OGMRIP_SUBP_CODEC (codec)));
          g_object_unref (codec);
        }
      }
    }
  }
  g_list_free (list);

  if (!status)
  {
    g_object_unref (encoding);
    return FALSE;
  }

  codec = ogmrip_gui_create_chapters_codec (data,
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
      G_CALLBACK (ogmrip_gui_encoding_completed), data);

  ogmrip_gui_set_filename (data, encoding);

  if (response == OGMRIP_RESPONSE_EXTRACT)
    ogmrip_gui_encode (data, encoding);
  else if (response == OGMRIP_RESPONSE_ENQUEUE)
  {
    ogmrip_encoding_manager_add (data->manager, encoding);
    g_object_unref (encoding);
  }
  else if (response == OGMRIP_RESPONSE_TEST)
  {
    guint width, height;

    ogmrip_gui_test (data, encoding, &width, &height);
    status = ogmrip_gui_run_options_dialog (data, encoding, width, height, error);
  }

  return status;
}

/*
 * When the extract button is activated
 */
static void
ogmrip_gui_extract_activated (OGMRipData *data)
{
  GError *error = NULL;

  if (!ogmrip_gui_run_options_dialog (data, NULL, 0, 0, &error))
  {
    ogmrip_run_error_dialog (GTK_WINDOW (data->window), error, _("Cannot encode media"));
    g_error_free (error);
  }
}

/*
 * When the play button is activated
 */
static void
ogmrip_gui_play_activated (OGMRipData *data, GtkWidget *button)
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
    ogmrip_run_error_dialog (GTK_WINDOW (data->window), error, _("Can't play title"));
    g_clear_error (&error);
  }
}

/*
 * When the import menu item is activated
 */
static void
ogmrip_gui_import_chapters_activated (OGMRipData *data)
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
    GFile *file;

    file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
    if (file)
    {
      if (!ogmrip_gui_import_matroska_chapters (data, file))
        if (!ogmrip_gui_import_simple_chapters (data, file))
        {
          gchar *path;

          path = g_file_get_parse_name (file);
          ogmrip_run_error_dialog (GTK_WINDOW (data->window), NULL,
              _("Could not open the chapters file '%s'"), path);
          g_free (path);
        }
      g_object_unref (file);
    }
  }
  gtk_widget_destroy (dialog);
}

/*
 * When the export menu item is activated
 */
static void
ogmrip_gui_export_chapters_activated (OGMRipData *data)
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
      ogmrip_gui_export_simple_chapters (data, filename);
      g_free (filename);
    }
  }
  gtk_widget_destroy (dialog);
}

/*
 * When the encoding manager dialog sends the response signal
 */
static void
ogmrip_gui_encodings_responsed (OGMRipData *data, gint response_id, GtkWidget *dialog)
{
  if (response_id != GTK_RESPONSE_ACCEPT)
    gtk_widget_destroy (dialog);
  else
  {
    GSList *list, *link;

    list = ogmrip_encoding_manager_get_list (data->manager);
    for (link = list; link; link = link->next)
      if (ogmrip_encoding_manager_get_status (data->manager, link->data) != OGMRIP_ENCODING_SUCCESS)
        ogmrip_gui_encode (data, g_object_ref (link->data));
    g_slist_free (list);
  }
}

/*
 * When the encodings menu item is activated
 */
static void
ogmrip_gui_encodings_activated (OGMRipData *data)
{
  GtkWidget *dialog;

  dialog = ogmrip_encoding_manager_dialog_new (data->manager);
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->window));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  g_signal_connect_swapped (dialog, "response",
      G_CALLBACK (ogmrip_gui_encodings_responsed), data);

  gtk_window_present (GTK_WINDOW (dialog));
}

/*
 * When the active title changed
 */
static void
ogmrip_gui_title_chooser_changed (OGMRipData *data)
{
  OGMRipTitle *title;
  gint angles;

  title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (data->title_chooser));
  gtk_widget_set_sensitive (data->title_chooser, title != NULL);

  gtk_container_clear (GTK_CONTAINER (data->audio_list));
  ogmrip_gui_add_audio_chooser (data);

  gtk_container_clear (GTK_CONTAINER (data->subp_list));
  ogmrip_gui_add_subp_chooser (data);

  ogmrip_chapter_store_set_title (OGMRIP_CHAPTER_STORE (data->chapter_store), title);
  ogmrip_chapter_store_select_all (OGMRIP_CHAPTER_STORE (data->chapter_store));

  angles = title ? ogmrip_title_get_n_angles (title) : 1;
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->angle_spin), 1);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (data->angle_spin), 1, angles);
  gtk_widget_set_visible (gtk_widget_get_parent (data->angle_spin), angles > 1);
}

/*
 * When the chapters selection changed
 */
static void
ogmrip_gui_chapter_selection_changed (OGMRipData *data)
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

  g_simple_action_set_enabled (G_SIMPLE_ACTION (data->extract_action), sensitive);
  gtk_widget_set_sensitive (data->relative_check, sensitive && (start_chap > 0 || end_chap != -1));
}

/*
 * When the main window receives the delete event
 */
static gboolean
ogmrip_gui_delete_event (OGMRipData *data)
{
  return FALSE;
}

/*
 * When the main window is destroyed
 */
static void
ogmrip_gui_destroyed (OGMRipData *data)
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
 * When the play signal is emitted
 */
static void
ogmrip_gui_player_play (OGMRipPlayer *player, GtkWidget *button)
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
ogmrip_gui_player_stop (OGMRipPlayer *player, GtkWidget *button)
{
  GtkWidget *image;

  image = gtk_bin_get_child (GTK_BIN (button));
  gtk_container_remove (GTK_CONTAINER (button), image);

  image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_MENU);

  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
}

static void
ogmrip_gui_app_prepared (OGMRipData *data, GApplication *app)
{
  g_simple_action_set_enabled (G_SIMPLE_ACTION (data->load_action), TRUE);
  g_simple_action_set_enabled (G_SIMPLE_ACTION (data->extract_action), data->media != NULL);

  data->prepared = TRUE;
}

static GActionEntry win_entries[] =
{
  { "load",          NULL, NULL, NULL, NULL },
  { "extract",       NULL, NULL, NULL, NULL },
  { "open_chapters", NULL, NULL, NULL, NULL },
  { "save_chapters", NULL, NULL, NULL, NULL },
  { "select_all",    NULL, NULL, NULL, NULL },
  { "deselect_all",  NULL, NULL, NULL, NULL },
  { "encodings",     NULL, NULL, NULL, NULL },
  { "profiles",      NULL, NULL, NULL, NULL },
  { "preferences",   NULL, NULL, NULL, NULL },
  { "about",         NULL, NULL, NULL, NULL },
  { "close",         NULL, NULL, NULL, NULL }
};

static OGMRipData *
ogmrip_gui_create (GApplication *app)
{
  GError *error = NULL;

  OGMRipData *data;
  GtkWidget *widget, *child;
  GtkBuilder *builder;
  GAction *action;

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_UI_FILE, &error))
    g_error ("Couldn't load builder file: %s", error->message);

  data = g_new0 (OGMRipData, 1);
  g_signal_connect_swapped (app, "prepare",
      G_CALLBACK (ogmrip_gui_app_prepared), data);

  data->window = gtk_builder_get_widget (builder, "main-window");
  gtk_window_set_default_size (GTK_WINDOW (data->window), 350, 500);
  gtk_window_set_icon_from_file (GTK_WINDOW (data->window),
      OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_ICON_FILE, NULL);
  gtk_application_add_window (GTK_APPLICATION (app), GTK_WINDOW (data->window));

  g_signal_connect_swapped (data->window, "delete-event",
      G_CALLBACK (ogmrip_gui_delete_event), data);
  g_signal_connect_swapped (data->window, "destroy",
      G_CALLBACK (ogmrip_gui_destroyed), data);

  g_action_map_add_action_entries (G_ACTION_MAP (data->window),
      win_entries, G_N_ELEMENTS (win_entries), NULL);

  action = g_action_map_lookup_action (G_ACTION_MAP (data->window), "close");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (gtk_widget_destroy), data->window);

  action = g_action_map_lookup_action (G_ACTION_MAP (data->window), "encodings");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_gui_encodings_activated), data);

  widget = gtk_builder_get_widget (builder, "hbox");

  data->title_chooser = ogmrip_title_chooser_widget_new ();
  gtk_grid_attach (GTK_GRID (widget), data->title_chooser, -1, 0, 1, 1);
  gtk_widget_set_hexpand (data->title_chooser, TRUE);
  gtk_widget_show (data->title_chooser);

  g_signal_connect_swapped (data->title_chooser, "changed",
      G_CALLBACK (ogmrip_gui_title_chooser_changed), data);

  action = g_action_map_lookup_action (G_ACTION_MAP (data->window), "open_chapters");
  g_object_bind_property (data->title_chooser, "sensitive",
      action, "enabled", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_gui_import_chapters_activated), data);

  action = g_action_map_lookup_action (G_ACTION_MAP (data->window), "save_chapters");
  g_object_bind_property (data->title_chooser, "sensitive",
      action, "enabled", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_gui_export_chapters_activated), data);

  data->load_action = g_action_map_lookup_action (G_ACTION_MAP (data->window), "load");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (data->load_action), FALSE);

  g_signal_connect_swapped (data->load_action, "activate",
      G_CALLBACK (ogmrip_gui_load_activated), data);

  widget = gtk_builder_get_widget (builder, "load-button");
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (widget), "win.load");

  data->extract_action = g_action_map_lookup_action (G_ACTION_MAP (data->window), "extract");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (data->extract_action), FALSE);

  g_signal_connect_swapped (data->extract_action, "activate",
      G_CALLBACK (ogmrip_gui_extract_activated), data);

  widget = gtk_builder_get_widget (builder, "extract-button");
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (widget), "win.extract");

  widget = gtk_builder_get_widget (builder, "play-button");
  g_object_bind_property (data->extract_action, "enabled",
      widget, "sensitive", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (widget, "toggled",
      G_CALLBACK (ogmrip_gui_play_activated), data);

  data->player = ogmrip_player_new ();
  g_signal_connect (data->player, "play",
      G_CALLBACK (ogmrip_gui_player_play), widget);
  g_signal_connect (data->player, "stop",
      G_CALLBACK (ogmrip_gui_player_stop), widget);

  data->audio_list = gtk_builder_get_widget (builder, "audio-list");
  g_object_bind_property (data->title_chooser, "sensitive",
      data->audio_list, "sensitive", G_BINDING_SYNC_CREATE);

  data->subp_list = gtk_builder_get_widget (builder, "subp-list");
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
      G_CALLBACK (ogmrip_gui_chapter_selection_changed), data);

  action = g_action_map_lookup_action (G_ACTION_MAP (data->window), "select_all");
  g_object_bind_property (data->title_chooser, "sensitive",
      action, "enabled", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_chapter_store_select_all), data->chapter_store);

  action = g_action_map_lookup_action (G_ACTION_MAP (data->window), "deselect_all");
  g_object_bind_property (data->title_chooser, "sensitive",
      action, "enabled", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_chapter_store_deselect_all), data->chapter_store);

  data->relative_check = gtk_builder_get_widget (builder, "relative-check");
  data->angle_spin = gtk_builder_get_widget (builder, "angle-spin");

  data->manager = ogmrip_encoding_manager_new ();

  g_object_unref (builder);

  ogmrip_gui_title_chooser_changed (data);

  return data;
}

/*
 * When the profiles menu item is activated
 */
static void
ogmrip_gui_profiles_activated (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  GtkWidget *dialog;
  
  dialog = ogmrip_profile_manager_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog),
      gtk_application_get_active_window (app));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/*
 * When the preferences menu item is activated
 */
static void
ogmrip_gui_pref_activated (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  GtkWidget *dialog;

  dialog = ogmrip_pref_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog),
      gtk_application_get_active_window (app));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/*
 * When the about menu item is activated
 */
static void
ogmrip_gui_about_activated (GSimpleAction *action, GVariant *parameter, gpointer app)
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

  gtk_show_about_dialog (
      gtk_application_get_active_window (app),
      "name", PACKAGE_NAME,
      "version", PACKAGE_VERSION,
      "comments", _("A Media Encoder for GNOME"),
      "copyright", "(c) 2004-2012 Olivier Rolland",
      "website", "http://ogmrip.sourceforge.net",
      "translator-credits", translator_credits,
      "documenters", documenters,
      "authors", authors,
      "logo", icon,
      NULL);
}

/*
 * When the quit menu item is activated
 */
static void
ogmrip_gui_quit_activated (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  GList *list;

  while (1)
  {
    list = gtk_application_get_windows (app);
    if (!list)
      break;
    gtk_widget_destroy (list->data);
  }
}

static GActionEntry app_entries[] =
{
  { "profiles",    ogmrip_gui_profiles_activated, NULL, NULL, NULL },
  { "preferences", ogmrip_gui_pref_activated,     NULL, NULL, NULL },
  { "about",       ogmrip_gui_about_activated,    NULL, NULL, NULL },
  { "quit",        ogmrip_gui_quit_activated,     NULL, NULL, NULL }
};

static void
ogmrip_gui_startup_cb (GApplication *app)
{
  GError *error = NULL;
  GtkBuilder *builder;
  GObject *menu;

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_MENU_FILE, &error))
    g_error ("Couldn't load builder file: %s", error->message);

  g_action_map_add_action_entries (G_ACTION_MAP (app),
      app_entries, G_N_ELEMENTS (app_entries), app);

  menu = gtk_builder_get_object (builder, "app-menu");
  gtk_application_set_app_menu (GTK_APPLICATION (app), G_MENU_MODEL (menu));

  menu = gtk_builder_get_object (builder, "win-menu");
  gtk_application_set_menubar (GTK_APPLICATION (app), G_MENU_MODEL (menu));

  g_object_unref (builder);
}

static void
ogmrip_gui_activate_cb (GApplication *app)
{
  OGMRipData *data;

  data = ogmrip_gui_create (app);

  g_application_hold (app);
  g_signal_connect_swapped (data->window, "destroy",
      G_CALLBACK (g_application_release), app);

  gtk_window_present (GTK_WINDOW (data->window));
}

static void
ogmrip_gui_open_cb (GApplication *app, GFile **files, gint n_files, const gchar *hint)
{
  OGMRipData *data;
  gchar *filename;
  gint i;

  for (i = 0; i < n_files; i ++)
  {
    data = ogmrip_gui_create (app);

    filename = g_file_get_path (files[i]);
    if (!g_file_test (filename, G_FILE_TEST_EXISTS) ||
        !ogmrip_gui_load_path (data, filename))
      gtk_widget_destroy (data->window);
    else
    {
      g_application_hold (app);
      g_signal_connect_swapped (data->window, "destroy",
          G_CALLBACK (g_application_release), app);
      gtk_window_present (GTK_WINDOW (data->window));
    }
    g_free (filename);
  }
}

static void
ogmrip_application_iface_init (OGMRipApplicationInterface *iface)
{
}

G_DEFINE_TYPE_WITH_CODE (OGMRipGui, ogmrip_gui, GTK_TYPE_APPLICATION,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_APPLICATION, ogmrip_application_iface_init));

static GOptionEntry opts[] =
{
  { "debug", 0,  0, G_OPTION_ARG_NONE, &debug, "Enable debug messages", NULL },
  { NULL,    0,  0, 0,                 NULL,   NULL,                    NULL }
};

static gboolean
ogmrip_gui_local_cmdline (GApplication *application, gchar ***argv, gint *status)
{
  GError *error = NULL;
  GOptionContext *context;
  gboolean retval;
  gint argc;

  context = g_option_context_new ("[<PATH>]");
  g_option_context_add_main_entries (context, opts, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  argc = g_strv_length (*argv);
  retval = g_option_context_parse (context, &argc, argv, &error);
  g_option_context_free (context);

  if (!retval)
  {
    g_print ("option parsing failed: %s\n", error->message);
    g_error_free (error);
    *status = 1;

    return TRUE;
  }

  if (!g_application_register (application, NULL, &error))
  {
    g_critical ("%s", error->message);
    g_error_free (error);
    *status = 1;

    return TRUE;
  }

  if (argc <= 1)
    g_application_activate (application);
  else
  {
    GFile **files;
    gint i;

    files = g_new (GFile *, argc - 1);

    for (i = 0; i < argc - 1; i ++)
      files[i] = g_file_new_for_commandline_arg ((*argv)[i + 1]);

    g_application_open (application, files, argc - 1, "");

    for (i = 0; i < argc - 1; i ++)
      g_object_unref (files[i]);
    g_free (files);
  }

  if (debug)
    ogmrip_log_set_print_stdout (TRUE);

  *status = 0;

  return TRUE;
}

static void
ogmrip_gui_class_init (OGMRipGuiClass *klass)
{
  GApplicationClass *application_class;

  application_class = G_APPLICATION_CLASS (klass);
  application_class->local_command_line = ogmrip_gui_local_cmdline;
}

static void
ogmrip_gui_init (OGMRipGui *gui)
{
}

GApplication *
ogmrip_gui_new (const gchar *app_id)
{
  GApplication *app;

  g_return_val_if_fail (g_application_id_is_valid (app_id), NULL);

  app = g_object_new (OGMRIP_TYPE_GUI,
      "application-id", app_id,
      "flags", G_APPLICATION_HANDLES_OPEN,
      NULL);

  g_signal_connect (app, "startup",
      G_CALLBACK (ogmrip_gui_startup_cb), NULL);
  g_signal_connect (app, "activate",
      G_CALLBACK (ogmrip_gui_activate_cb), NULL);
  g_signal_connect (app, "open",
      G_CALLBACK (ogmrip_gui_open_cb), NULL);

  return app;
}

