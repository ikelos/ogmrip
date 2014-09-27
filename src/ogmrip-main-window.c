/* OGMRip - A media encoder for GNOME
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-main-window.h"
#include "ogmrip-options-dialog.h"
#include "ogmrip-settings.h"
#include "ogmrip-helper.h"

#ifdef HAVE_ENCHANT_SUPPORT
#include "ogmrip-spell-dialog.h"
#endif /* HAVE_ENCHANT_SUPPORT */

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#define OGMRIP_UI_RES   "/org/ogmrip/ogmrip-main-window.ui"
#define OGMRIP_ICON_RES "/org/ogmrip/ogmrip.png"
#define OGMRIP_MENU_RES "/org/ogmrip/ogmrip-menu.ui"

struct _OGMRipMainWindowPriv
{
  GtkWidget *title_entry;
  GtkWidget *title_chooser;
  GtkWidget *audio_list;
  GtkWidget *subp_list;
  GtkWidget *chapter_view;
  GtkWidget *load_button;
  GtkWidget *extract_button;
  GtkWidget *play_button;
  GtkWidget *angle_spin;
  GtkWidget *length_label;
  GtkWidget *relative_check;
  GtkTreeModel *chapter_store;

  GAction *load_action;
  GAction *extract_action;

  OGMRipMedia *media;
  OGMRipPlayer *player;
  OGMRipEncodingManager *manager;

  gboolean prepared;
};

extern GSettings *settings;

#ifdef HAVE_ENCHANT_SUPPORT
static void
ogmrip_main_window_spell_check (OGMRipMainWindow *window, const gchar *filename, gint lang)
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
    ogmrip_run_error_dialog (GTK_WINDOW (window), NULL,
        "<big><b>%s</b></big>\n\n%s", _("Could not create dictionary"), _("Spell will not be checked."));
    goto spell_check_cleanup;
  }

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
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
gtk_container_clear (GtkContainer *container)
{
  GList *list, *link;

  g_return_if_fail (GTK_IS_CONTAINER (container));

  list = gtk_container_get_children (container);
  for (link = list; link; link = link->next)
    gtk_container_remove (container, GTK_WIDGET (link->data));
  g_list_free (list);
}

static void
ogmrip_media_open_progress_cb (OGMRipMedia *media, gdouble percent, gpointer pbar)
{
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), percent);
}

static void
ogmrip_encoding_run_cb (OGMRipEncoding *encoding, OGMJobSpawn *spawn, OGMRipProgressDialog *dialog)
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
ogmrip_encoding_progress_cb (OGMRipEncoding *encoding, OGMJobSpawn *spawn, gdouble fraction, OGMRipProgressDialog *dialog)
{
  ogmrip_progress_dialog_set_fraction (dialog, fraction);
}

#ifdef HAVE_ENCHANT_SUPPORT
static void
ogmrip_encoding_complete_cb (OGMRipEncoding *encoding, OGMJobSpawn *spawn, OGMRipEncodingStatus status, OGMRipMainWindow *window)
{
  if (status == OGMRIP_ENCODING_SUCCESS &&
      spawn && OGMRIP_IS_SUBP_CODEC (spawn) &&
      ogmrip_codec_format (G_OBJECT_TYPE (spawn)) != OGMRIP_FORMAT_VOBSUB)
  {
    OGMRipProfile *profile;
    gboolean spell_check;

    profile = ogmrip_encoding_get_profile (encoding);
    ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_SPELL_CHECK, "b", &spell_check);

    if (spell_check)
      ogmrip_main_window_spell_check (window,
          ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (spawn))),
          ogmrip_subp_codec_get_language (OGMRIP_SUBP_CODEC (spawn)));
  }
}
#endif

static void
ogmrip_progress_dialog_response_cb (GtkWidget *parent, gint response_id, OGMRipEncoding *encoding)
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

G_DEFINE_TYPE_WITH_PRIVATE (OGMRipMainWindow, ogmrip_main_window, GTK_TYPE_APPLICATION_WINDOW)

static void
ogmrip_main_window_add_audio_chooser (OGMRipMainWindow *window)
{
  GtkWidget *chooser;
  OGMRipTitle *title;

  title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (window->priv->title_chooser));

  chooser = ogmrip_audio_chooser_widget_new ();
  ogmrip_source_chooser_set_title (OGMRIP_SOURCE_CHOOSER (chooser), title);
  gtk_container_add (GTK_CONTAINER (window->priv->audio_list), chooser);
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
      G_CALLBACK (ogmrip_main_window_add_audio_chooser), window);
  g_signal_connect_swapped (chooser, "remove-clicked",
      G_CALLBACK (gtk_container_remove), window->priv->audio_list);
}

static void
ogmrip_main_window_add_subp_chooser (OGMRipMainWindow *window)
{
  GtkWidget *chooser;
  OGMRipTitle *title;

  title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (window->priv->title_chooser));

  chooser = ogmrip_subp_chooser_widget_new ();
  ogmrip_source_chooser_set_title (OGMRIP_SOURCE_CHOOSER (chooser), title);
  gtk_container_add (GTK_CONTAINER (window->priv->subp_list), chooser);
  gtk_widget_show (chooser);

  if (title)
  {
    ogmrip_source_chooser_select_language (OGMRIP_SOURCE_CHOOSER (chooser),
        g_settings_get_uint (settings, OGMRIP_SETTINGS_PREF_SUBP));

    if (!ogmrip_source_chooser_get_active (OGMRIP_SOURCE_CHOOSER (chooser)))
      ogmrip_source_chooser_set_active (OGMRIP_SOURCE_CHOOSER (chooser), NULL);
  }

  g_signal_connect_swapped (chooser, "add-clicked",
      G_CALLBACK (ogmrip_main_window_add_subp_chooser), window);
  g_signal_connect_swapped (chooser, "remove-clicked",
      G_CALLBACK (gtk_container_remove), window->priv->subp_list);
}

static gboolean
ogmrip_main_window_open_media (OGMRipMainWindow *window, OGMRipMedia *media)
{
  GtkWidget *dialog, *area, *box, *label, *pbar;
  GCancellable *cancellable;
  gboolean res;

  dialog = gtk_dialog_new_with_buttons (_("Opening media"),
      GTK_WINDOW (window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
      _("_Cancel"), GTK_RESPONSE_CANCEL,
      NULL);
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

  res = ogmrip_media_open (media, cancellable, ogmrip_media_open_progress_cb, pbar, NULL);

  gtk_widget_destroy (dialog);

  if (!res && !g_cancellable_is_cancelled (cancellable))
    ogmrip_run_error_dialog (GTK_WINDOW (window), NULL, _("Could not open the media"));

  g_object_unref (cancellable);

  return res;
}

static void
ogmrip_main_window_load_media (OGMRipMainWindow *window, OGMRipMedia *media)
{
  if (ogmrip_main_window_open_media (window, media))
  {
    gint nvid;
    const gchar *label = NULL;

    if (window->priv->media)
    {
      ogmrip_media_close (window->priv->media);
      g_object_unref (window->priv->media);
    }
    window->priv->media = g_object_ref (media);

    ogmrip_title_chooser_set_media (OGMRIP_TITLE_CHOOSER (window->priv->title_chooser), media);

    nvid = ogmrip_media_get_n_titles (media);
    if (nvid > 0)
      label = ogmrip_media_get_label (media);

    gtk_entry_set_text (GTK_ENTRY (window->priv->title_entry),
        label ? label : _("Untitled"));

    g_simple_action_set_enabled (G_SIMPLE_ACTION (window->priv->extract_action), window->priv->prepared);
  }
}

static void
ogmrip_main_window_run_error_dialog (OGMRipMainWindow *window, OGMRipEncoding *encoding, GError *error)
{
  GtkWidget *dialog, *area, *expander, *widget;
  const gchar *filename;

  dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (window),
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

static gboolean
ogmrip_main_window_run_replace_dialog (OGMRipMainWindow *window, OGMRipEncoding *encoding, GCancellable *cancellable, GError **error)
{
  GtkWidget *dialog;
  GFile *output;

  gboolean response;
  gchar *filename;

  output = ogmrip_container_get_output (ogmrip_encoding_get_container (encoding));

  filename = g_file_get_parse_name (output);
  dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (window),
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

static void
ogmrip_main_window_clean (OGMRipMainWindow *window, OGMRipEncoding *encoding, gboolean error)
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

      dialog = gtk_message_dialog_new (GTK_WINDOW (window),
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

void
ogmrip_main_window_encode (OGMRipMainWindow *window, OGMRipEncoding *encoding)
{
  GError *error = NULL;
  gboolean result = FALSE;

  if (ogmrip_open_title (GTK_WINDOW (window), ogmrip_encoding_get_title (encoding), &error))
  {
    GCancellable *cancellable;
    GtkWidget *dialog;
    guint cookie;

    cookie = gtk_application_inhibit (gtk_window_get_application (GTK_WINDOW (window)),
        GTK_WINDOW (window), GTK_APPLICATION_INHIBIT_SUSPEND, _("Encoding a movie"));

    dialog = ogmrip_progress_dialog_new (GTK_WINDOW (window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, TRUE);
    ogmrip_progress_dialog_set_title (OGMRIP_PROGRESS_DIALOG (dialog),
        ogmrip_container_get_label (ogmrip_encoding_get_container (encoding)));

    g_signal_connect (dialog, "response",
        G_CALLBACK (ogmrip_progress_dialog_response_cb), encoding);

    g_signal_connect (dialog, "delete-event",
        G_CALLBACK (gtk_true), NULL);

    gtk_window_present (GTK_WINDOW (dialog));

    g_signal_connect (encoding, "run",
        G_CALLBACK (ogmrip_encoding_run_cb), dialog);
    g_signal_connect (encoding, "progress",
        G_CALLBACK (ogmrip_encoding_progress_cb), dialog);

#ifdef HAVE_ENCHANT_SUPPORT
    g_signal_connect (encoding, "complete",
        G_CALLBACK (ogmrip_encoding_complete_cb), window);
#endif

    cancellable = g_cancellable_new ();
    g_object_set_data_full (G_OBJECT (encoding), "cancellable", cancellable, g_object_unref);

    result = ogmrip_encoding_encode (encoding, cancellable, &error);
    if (!result && error && error->code == G_IO_ERROR_EXISTS)
    {
      gtk_widget_hide (dialog);

      g_clear_error (&error);

      if (ogmrip_main_window_run_replace_dialog (window, encoding, cancellable, &error))
      {
        gtk_widget_show (dialog);
        result = ogmrip_encoding_encode (encoding, cancellable, &error);
      }
      else if (!error)
        result = TRUE;
    }

    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_encoding_run_cb, dialog);
    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_encoding_progress_cb, dialog);

#ifdef HAVE_ENCHANT_SUPPORT
    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_encoding_complete_cb, window);
#endif

    gtk_widget_destroy (dialog);

    if (cookie > 0)
      gtk_application_uninhibit (gtk_window_get_application (GTK_WINDOW (window)), cookie);

    ogmrip_title_close (ogmrip_encoding_get_title (encoding));
  }

  if (!result && (!error || error->code != G_IO_ERROR_CANCELLED))
    ogmrip_main_window_run_error_dialog (window, encoding, error);

  ogmrip_main_window_clean (window, encoding, !result && (!error || error->code != G_IO_ERROR_CANCELLED));

  if (error)
    g_error_free (error);
}

static void
ogmrip_main_window_test (OGMRipMainWindow *window, OGMRipEncoding *encoding, guint *width, guint *height)
{
  GError *error = NULL;
  gboolean result = FALSE;

  if (ogmrip_open_title (GTK_WINDOW (window), ogmrip_encoding_get_title (encoding), &error))
  {
    GCancellable *cancellable;
    GtkWidget *dialog;
    guint cookie;

    cookie = gtk_application_inhibit (gtk_window_get_application (GTK_WINDOW (window)),
        GTK_WINDOW (window), GTK_APPLICATION_INHIBIT_SUSPEND, NULL);

    dialog = ogmrip_progress_dialog_new (GTK_WINDOW (window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, TRUE);
    ogmrip_progress_dialog_set_title (OGMRIP_PROGRESS_DIALOG (dialog),
        ogmrip_container_get_label (ogmrip_encoding_get_container (encoding)));

    g_signal_connect (dialog, "response",
        G_CALLBACK (ogmrip_progress_dialog_response_cb), encoding);
    g_signal_connect (dialog, "delete-event",
        G_CALLBACK (gtk_true), NULL);

    gtk_window_present (GTK_WINDOW (dialog));

    g_signal_connect (encoding, "run",
        G_CALLBACK (ogmrip_encoding_run_cb), dialog);
    g_signal_connect (encoding, "progress",
        G_CALLBACK (ogmrip_encoding_progress_cb), dialog);

    cancellable = g_cancellable_new ();
    g_object_set_data_full (G_OBJECT (encoding), "cancellable", cancellable, g_object_unref);

    result = ogmrip_encoding_test (encoding, cancellable, &error);

    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_encoding_run_cb, dialog);
    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_encoding_progress_cb, dialog);

    gtk_widget_destroy (dialog);

    if (cookie > 0)
      gtk_application_uninhibit (gtk_window_get_application (GTK_WINDOW (window)), cookie);

    ogmrip_title_close (ogmrip_encoding_get_title (encoding));

    if (result)
    {
      OGMRipCodec *codec;

      codec = ogmrip_encoding_get_video_codec (encoding);
      ogmrip_video_codec_get_scale_size (OGMRIP_VIDEO_CODEC (codec), width, height);
    }
  }

  if (!result && (!error || error->code == G_IO_ERROR_CANCELLED))
    ogmrip_main_window_run_error_dialog (window, encoding, error);

  ogmrip_main_window_clean (window, encoding, !result && (!error || error->code == G_IO_ERROR_CANCELLED));

  if (error)
    g_error_free (error);

  ogmrip_encoding_clear (encoding);
}

static OGMRipContainer *
ogmrip_main_window_create_container (OGMRipMainWindow *window, OGMRipProfile *profile)
{
  OGMRipContainer *container;

  container = ogmrip_container_new_from_profile (profile);
  ogmrip_container_set_label (container,
      gtk_entry_get_text (GTK_ENTRY (window->priv->title_entry)));

  return container;
}

static OGMRipCodec *
ogmrip_main_window_create_video_codec (OGMRipMainWindow *window, OGMRipProfile *profile,
    OGMRipVideoStream *stream, guint start_chap, gint end_chap)
{
  OGMRipCodec *codec;

  codec = ogmrip_video_codec_new_from_profile (stream, profile);
  if (!codec)
    return NULL;

  ogmrip_codec_set_autoclean (codec, FALSE);
  ogmrip_codec_set_chapters (codec, start_chap, end_chap);

  return codec;
}

static const gint sample_rate[] =
{ 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000 };

static OGMRipCodec *
ogmrip_main_window_create_audio_codec (OGMRipMainWindow *window, OGMRipProfile *profile,
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

  ogmrip_codec_set_autoclean (codec, FALSE);
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

static OGMRipCodec *
ogmrip_main_window_create_subp_codec (OGMRipMainWindow *window, OGMRipProfile *profile,
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

  ogmrip_codec_set_autoclean (codec, FALSE);
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

static OGMRipCodec *
ogmrip_main_window_create_chapters_codec (OGMRipMainWindow *window, OGMRipVideoStream *stream, guint start_chap, gint end_chap)
{
  OGMRipCodec *codec;
  guint last_chap, chap;
  gchar *label;

  if (!ogmrip_chapter_store_get_editable (OGMRIP_CHAPTER_STORE (window->priv->chapter_store)))
    return NULL;

  codec = ogmrip_chapters_new (stream);
  if (!codec)
    return NULL;

  ogmrip_chapters_set_language (OGMRIP_CHAPTERS (codec),
      g_settings_get_uint (settings, OGMRIP_SETTINGS_CHAPTER_LANG));

  ogmrip_codec_set_autoclean (codec, FALSE);
  ogmrip_codec_set_chapters (codec, start_chap, end_chap);

  ogmrip_codec_get_chapters (codec, &start_chap, &last_chap);
  for (chap = start_chap; chap <= last_chap; chap ++)
  {
    label = ogmrip_chapter_store_get_label (OGMRIP_CHAPTER_STORE (window->priv->chapter_store), chap);
    if (!label)
      break;

    ogmrip_chapters_set_label (OGMRIP_CHAPTERS (codec), chap, label);
    g_free (label);
  }

  return codec;
}

static void
ogmrip_main_window_set_filename (OGMRipMainWindow *window, OGMRipEncoding *encoding)
{
  GFile *file;
  GString *filename;
  OGMRipContainer *container;
  OGMRipCodec *codec;
  gchar *path, *str;
  guint format;

  format = g_settings_get_uint (settings, OGMRIP_SETTINGS_FILENAME);

  filename = g_string_new (gtk_entry_get_text (GTK_ENTRY (window->priv->title_entry)));

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

static gboolean
ogmrip_main_window_run_options_dialog (OGMRipMainWindow *window, OGMRipEncoding *encoding, guint width, guint height, GError **error)
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

  title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (window->priv->title_chooser));

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
        gtk_widget_is_sensitive (window->priv->relative_check) &
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (window->priv->relative_check)));

    dialog = ogmrip_options_dialog_new (encoding);
  }

  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
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

  container = ogmrip_main_window_create_container (window, profile);
  status = ogmrip_encoding_set_container (encoding, container, error);
  g_object_unref (container);

  if (!status)
  {
    gtk_widget_destroy (dialog);
    g_object_unref (encoding);
    return FALSE;
  }

  ogmrip_chapter_store_get_selection (OGMRIP_CHAPTER_STORE (window->priv->chapter_store), &start_chap, &end_chap);

  vcodec = ogmrip_main_window_create_video_codec (window, profile,
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

  list = gtk_container_get_children (GTK_CONTAINER (window->priv->audio_list));
  for (link = list; link && status; link = link->next)
  {
    stream = ogmrip_source_chooser_get_active (link->data);
    if (stream)
    {
      if (OGMRIP_IS_FILE (stream))
        status = ogmrip_encoding_add_file (encoding, OGMRIP_FILE (stream), error);
      else
      {
        codec = ogmrip_main_window_create_audio_codec (window, profile,
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

  list = gtk_container_get_children (GTK_CONTAINER (window->priv->subp_list));
  for (link = list; link && status; link = link->next)
  {
    stream = ogmrip_source_chooser_get_active (link->data);
    if (stream)
    {
      if (OGMRIP_IS_FILE (stream))
        status = ogmrip_encoding_add_file (encoding, OGMRIP_FILE (stream), error);
      else
      {
        codec = ogmrip_main_window_create_subp_codec (window, profile,
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

  codec = ogmrip_main_window_create_chapters_codec (window,
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

  ogmrip_main_window_set_filename (window, encoding);

  if (response == OGMRIP_RESPONSE_EXTRACT)
    ogmrip_main_window_encode (window, encoding);
  else if (response == OGMRIP_RESPONSE_ENQUEUE)
  {
    ogmrip_encoding_manager_add (window->priv->manager, encoding);
    g_object_unref (encoding);
  }
  else if (response == OGMRIP_RESPONSE_TEST)
  {
    guint width, height;

    ogmrip_main_window_test (window, encoding, &width, &height);
    status = ogmrip_main_window_run_options_dialog (window, encoding, width, height, error);
  }

  return status;
}

static gboolean
ogmrip_main_window_import_simple_chapters (OGMRipMainWindow *window, GFile *gfile)
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
        ogmrip_chapter_store_set_label (OGMRIP_CHAPTER_STORE (window->priv->chapter_store), chap - 1, str + 1);
      }
    }
  }

  fclose (file);

  return TRUE;
}

static gboolean
ogmrip_main_window_import_matroska_chapters (OGMRipMainWindow *window, GFile *file)
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
              ogmrip_chapter_store_set_label (OGMRIP_CHAPTER_STORE (window->priv->chapter_store), chap ++, str);
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

static void
ogmrip_main_window_export_simple_chapters (OGMRipMainWindow *window, const gchar *filename)
{
  guint start_chap;
  gint end_chap;

  if (ogmrip_chapter_store_get_selection (OGMRIP_CHAPTER_STORE (window->priv->chapter_store), &start_chap, &end_chap))
  {
    GError *error = NULL;
    OGMRipTitle *title;
    OGMRipCodec *chapters;
    gchar *label;
    guint i;

    title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (window->priv->title_chooser));

    chapters = ogmrip_chapters_new (ogmrip_title_get_video_stream (title));
    ogmrip_codec_set_chapters (OGMRIP_CODEC (chapters), start_chap, end_chap);

    for (i = 0; ; i++)
    {
      label = ogmrip_chapter_store_get_label (OGMRIP_CHAPTER_STORE (window->priv->chapter_store), i);
      if (!label)
        break;
      ogmrip_chapters_set_label (OGMRIP_CHAPTERS (chapters), i, label);
      g_free (label);
    }

    if (!ogmjob_task_run (OGMJOB_TASK (chapters), NULL, &error))
    {
      GtkWidget *dialog;

      dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (window),
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

static void
ogmrip_main_window_eject_activated (OGMRipMainWindow *window, GtkWidget *dialog)
{
  if (window->priv->media)
  {
    OGMRipMedia *media;

    media = ogmrip_media_chooser_get_media (OGMRIP_MEDIA_CHOOSER (dialog));
    if (media)
    {
      const gchar *uri;

      uri = ogmrip_media_get_uri (window->priv->media);
      if ((g_str_has_prefix (uri, "dvd://") || g_str_has_prefix (uri, "br://")) &&
          g_str_equal (ogmrip_media_get_uri (media), uri))
      {
        g_object_unref (window->priv->media);
        window->priv->media = NULL;

        ogmrip_title_chooser_set_media (OGMRIP_TITLE_CHOOSER (window->priv->title_chooser), NULL);
        gtk_entry_set_text (GTK_ENTRY (window->priv->title_entry), "");
      }
    }
  }
}

static void
ogmrip_main_window_load_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipMainWindow *window = data;
  GtkWidget *dialog;

  dialog = ogmrip_media_chooser_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  g_signal_connect_swapped (dialog, "eject",
      G_CALLBACK (ogmrip_main_window_eject_activated), window);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    OGMRipMedia *media;

    gtk_widget_hide (dialog);

    media = ogmrip_media_chooser_get_media (OGMRIP_MEDIA_CHOOSER (dialog));
    if (media)
      ogmrip_main_window_load_media (data, media);
  }
  gtk_widget_destroy (dialog);
}

static void
ogmrip_main_window_extract_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipMainWindow *window = data;
  GError *error = NULL;

  if (!ogmrip_main_window_run_options_dialog (window, NULL, 0, 0, &error))
  {
    ogmrip_run_error_dialog (GTK_WINDOW (window), error, _("Cannot encode media"));
    g_error_free (error);
  }
}

static void
ogmrip_main_window_open_chapters_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipMainWindow *window = data;

  GtkWidget *dialog;
  GtkFileFilter *filter;

  dialog = gtk_file_chooser_dialog_new (_("Select a chapters file"),
      GTK_WINDOW (window), GTK_FILE_CHOOSER_ACTION_OPEN,
      _("_Cancel"), GTK_RESPONSE_CANCEL,
      _("_Open"), GTK_RESPONSE_OK,
      NULL);

  filter = gtk_file_filter_new ();
  gtk_file_filter_add_mime_type (filter, "text/*");
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    GFile *file;

    file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
    if (file)
    {
      if (!ogmrip_main_window_import_matroska_chapters (window, file))
        if (!ogmrip_main_window_import_simple_chapters (window, file))
        {
          gchar *path;

          path = g_file_get_parse_name (file);
          ogmrip_run_error_dialog (GTK_WINDOW (window), NULL,
              _("Could not open the chapters file '%s'"), path);
          g_free (path);
        }
      g_object_unref (file);
    }
  }
  gtk_widget_destroy (dialog);
}

static void
ogmrip_main_window_save_chapters_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipMainWindow *window = data;
  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new (_("Select a file"),
      GTK_WINDOW (window), GTK_FILE_CHOOSER_ACTION_SAVE,
      _("_Cancel"), GTK_RESPONSE_CANCEL,
      _("_Save"), GTK_RESPONSE_OK,
      NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    gchar *filename;

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    if (filename)
    {
      ogmrip_main_window_export_simple_chapters (window, filename);
      g_free (filename);
    }
  }
  gtk_widget_destroy (dialog);
}

static void
ogmrip_main_window_select_all_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipMainWindow *window = data;

  ogmrip_chapter_store_select_all (OGMRIP_CHAPTER_STORE (window->priv->chapter_store));
}

static void
ogmrip_main_window_deselect_all_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipMainWindow *window = data;

  ogmrip_chapter_store_deselect_all (OGMRIP_CHAPTER_STORE (window->priv->chapter_store));
}

static void
ogmrip_main_window_close_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  gtk_widget_destroy (data);
}

static void
ogmrip_main_window_title_chooser_changed (OGMRipMainWindow *window)
{
  OGMRipTitle *title;
  gint angles;

  title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (window->priv->title_chooser));
  gtk_widget_set_sensitive (window->priv->title_chooser, title != NULL);
  gtk_widget_set_sensitive (window->priv->relative_check, title != NULL);

  gtk_container_clear (GTK_CONTAINER (window->priv->audio_list));
  ogmrip_main_window_add_audio_chooser (window);

  gtk_container_clear (GTK_CONTAINER (window->priv->subp_list));
  ogmrip_main_window_add_subp_chooser (window);

  ogmrip_chapter_store_set_title (OGMRIP_CHAPTER_STORE (window->priv->chapter_store), title);
  ogmrip_chapter_store_select_all (OGMRIP_CHAPTER_STORE (window->priv->chapter_store));

  angles = title ? ogmrip_title_get_n_angles (title) : 1;
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (window->priv->angle_spin), 1);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (window->priv->angle_spin), 1, angles);
  gtk_widget_set_visible (gtk_widget_get_parent (window->priv->angle_spin), angles > 1);
}

static void
ogmrip_main_window_chapter_selection_changed (OGMRipMainWindow *window)
{
  gboolean sensitive;
  guint start_chap;
  gint end_chap;

  sensitive = ogmrip_chapter_store_get_selection (OGMRIP_CHAPTER_STORE (window->priv->chapter_store), &start_chap, &end_chap);
  if (!sensitive)
    gtk_label_set_text (GTK_LABEL (window->priv->length_label), "");
  else
  {
    OGMRipTitle *title;
    OGMRipTime time_;

    title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (window->priv->title_chooser));
    if (ogmrip_title_get_chapters_length (title, start_chap, end_chap, &time_) > 0)
    {
      gchar *str;

      str = g_strdup_printf ("%02lu:%02lu:%02lu", time_.hour, time_.min, time_.sec);
      gtk_label_set_text (GTK_LABEL (window->priv->length_label), str);
      g_free (str);
    }
  }

  g_simple_action_set_enabled (G_SIMPLE_ACTION (window->priv->extract_action), sensitive);

  gtk_widget_set_sensitive (window->priv->relative_check, sensitive && (start_chap > 0 || end_chap != -1));
}

static void
ogmrip_main_window_play_activated (OGMRipMainWindow *window)
{
  GError *error = NULL;

  OGMRipTitle *title;
  OGMRipStream *stream;
  GList *list, *link;

  guint start_chap;
  gint end_chap;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (window->priv->play_button)))
  {
    ogmrip_player_stop (window->priv->player);
    return;
  }

  title = ogmrip_title_chooser_get_active (OGMRIP_TITLE_CHOOSER (window->priv->title_chooser));
  ogmrip_player_set_title (window->priv->player, title);

  ogmrip_chapter_store_get_selection (OGMRIP_CHAPTER_STORE (window->priv->chapter_store), &start_chap, &end_chap);
  ogmrip_player_set_chapters (window->priv->player, start_chap, end_chap);

  list = gtk_container_get_children (GTK_CONTAINER (window->priv->audio_list));
  for (link = list; link; link = link->next)
  {
    stream = ogmrip_source_chooser_get_active (link->data);
    if (stream)
    {
      ogmrip_player_set_audio_stream (window->priv->player, OGMRIP_AUDIO_STREAM (stream));
      break;
    }
  }
  g_list_free (list);

  list = gtk_container_get_children (GTK_CONTAINER (window->priv->subp_list));
  for (link = list; link; link = link->next)
  {
    stream = ogmrip_source_chooser_get_active (link->data);
    if (stream)
    {
      ogmrip_player_set_subp_stream (window->priv->player, OGMRIP_SUBP_STREAM (stream));
      break;
    }
  }
  g_list_free (list);

  if (!ogmrip_player_play (window->priv->player, &error))
  {
    ogmrip_run_error_dialog (GTK_WINDOW (window), error, _("Can't play title"));
    g_clear_error (&error);
  }
}

static void
ogmrip_main_window_player_play (OGMRipMainWindow *window)
{
  GtkWidget *image;

  image = gtk_bin_get_child (GTK_BIN (window->priv->play_button));
  gtk_container_remove (GTK_CONTAINER (window->priv->play_button), image);

  image = gtk_image_new_from_icon_name ("media-playback-stop", GTK_ICON_SIZE_MENU);

  gtk_container_add (GTK_CONTAINER (window->priv->play_button), image);
  gtk_widget_show (image);
}

static void
ogmrip_main_window_player_stop (OGMRipMainWindow *window)
{
  GtkWidget *image;

  image = gtk_bin_get_child (GTK_BIN (window->priv->play_button));
  gtk_container_remove (GTK_CONTAINER (window->priv->play_button), image);

  image = gtk_image_new_from_icon_name ("media-playback-start", GTK_ICON_SIZE_MENU);

  gtk_container_add (GTK_CONTAINER (window->priv->play_button), image);
  gtk_widget_show (image);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (window->priv->play_button), FALSE);
}

static void
ogmrip_main_window_app_prepared (OGMRipMainWindow *window, OGMRipApplication *app)
{
  g_simple_action_set_enabled (G_SIMPLE_ACTION (window->priv->load_action), TRUE);
  g_simple_action_set_enabled (G_SIMPLE_ACTION (window->priv->extract_action), window->priv->media != NULL);

  window->priv->prepared = TRUE;
}

static void
ogmrip_main_window_dispose (GObject *gobject)
{
  OGMRipMainWindow *window = OGMRIP_MAIN_WINDOW (gobject);

  g_clear_object (&window->priv->media);
  g_clear_object (&window->priv->player);
  g_clear_object (&window->priv->manager);

  G_OBJECT_CLASS (ogmrip_main_window_parent_class)->dispose (gobject);
}

static void
ogmrip_main_window_class_init (OGMRipMainWindowClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_main_window_dispose;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), OGMRIP_UI_RES);

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipMainWindow, title_entry);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipMainWindow, title_chooser);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipMainWindow, audio_list);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipMainWindow, subp_list);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipMainWindow, chapter_view);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipMainWindow, load_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipMainWindow, extract_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipMainWindow, play_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipMainWindow, angle_spin);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipMainWindow, length_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipMainWindow, relative_check);
}

static GActionEntry win_entries[] =
{
  { "load",          ogmrip_main_window_load_activated,          NULL, NULL, NULL },
  { "extract",       ogmrip_main_window_extract_activated,       NULL, NULL, NULL },
  { "open_chapters", ogmrip_main_window_open_chapters_activated, NULL, NULL, NULL },
  { "save_chapters", ogmrip_main_window_save_chapters_activated, NULL, NULL, NULL },
  { "select_all",    ogmrip_main_window_select_all_activated,    NULL, NULL, NULL },
  { "deselect_all",  ogmrip_main_window_deselect_all_activated,  NULL, NULL, NULL },
  { "close",         ogmrip_main_window_close_activated,         NULL, NULL, NULL }
};

static void
ogmrip_main_window_init (OGMRipMainWindow *window)
{
  GError *error = NULL;

  GtkWidget *hbar, *button, *image;

  GtkBuilder *builder;
  GObject *menu;

  GAction *action;
  GdkPixbuf *icon;

  g_type_ensure (OGMRIP_TYPE_CHAPTER_VIEW);
  g_type_ensure (OGMRIP_TYPE_TITLE_CHOOSER_WIDGET);

  gtk_widget_init_template (GTK_WIDGET (window));

  window->priv = ogmrip_main_window_get_instance_private (window);

  gtk_application_window_set_show_menubar (GTK_APPLICATION_WINDOW (window), FALSE);

  icon = gdk_pixbuf_new_from_resource (OGMRIP_ICON_RES, NULL);
  if (icon)
  {
    gtk_window_set_icon (GTK_WINDOW (window), icon);
    g_object_unref (icon);
  }

  hbar = gtk_header_bar_new ();
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (hbar), TRUE);
  gtk_window_set_titlebar (GTK_WINDOW (window), hbar);
  gtk_widget_show (hbar);

  gtk_window_set_title (GTK_WINDOW (window), "OGMRip");

  button = gtk_menu_button_new ();
  gtk_menu_button_set_use_popover (GTK_MENU_BUTTON (button), TRUE);
  gtk_header_bar_pack_end (GTK_HEADER_BAR (hbar), button);
  gtk_widget_show (button);

  image = gtk_image_new_from_icon_name ("emblem-system-symbolic", GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_resource (builder, OGMRIP_MENU_RES, &error))
    g_error ("Couldn't load builder file: %s", error->message);
  menu = gtk_builder_get_object (builder, "win-menu");
  g_object_unref (builder);

  gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (button), G_MENU_MODEL (menu));

  g_signal_connect_swapped (window->priv->title_chooser, "changed",
      G_CALLBACK (ogmrip_main_window_title_chooser_changed), window);

  g_action_map_add_action_entries (G_ACTION_MAP (window),
      win_entries, G_N_ELEMENTS (win_entries), window);

  action = g_action_map_lookup_action (G_ACTION_MAP (window), "open_chapters");
  g_object_bind_property (window->priv->title_chooser, "sensitive",
      action, "enabled", G_BINDING_SYNC_CREATE);

  action = g_action_map_lookup_action (G_ACTION_MAP (window), "save_chapters");
  g_object_bind_property (window->priv->title_chooser, "sensitive",
      action, "enabled", G_BINDING_SYNC_CREATE);

  window->priv->load_action = g_action_map_lookup_action (G_ACTION_MAP (window), "load");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (window->priv->load_action), FALSE);

  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (window->priv->load_button), "win.load");

  window->priv->extract_action = g_action_map_lookup_action (G_ACTION_MAP (window), "extract");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (window->priv->extract_action), FALSE);

  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (window->priv->extract_button), "win.extract");

  g_object_bind_property (window->priv->extract_action, "enabled",
      window->priv->play_button, "sensitive", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (window->priv->play_button, "toggled",
      G_CALLBACK (ogmrip_main_window_play_activated), window);

  window->priv->player = ogmrip_player_new ();
  g_signal_connect_swapped (window->priv->player, "play",
      G_CALLBACK (ogmrip_main_window_player_play), window);
  g_signal_connect_swapped (window->priv->player, "stop",
      G_CALLBACK (ogmrip_main_window_player_stop), window);

  g_object_bind_property (window->priv->title_chooser, "sensitive",
      window->priv->audio_list, "sensitive", G_BINDING_SYNC_CREATE);

  g_object_bind_property (window->priv->title_chooser, "sensitive",
      window->priv->subp_list, "sensitive", G_BINDING_SYNC_CREATE);

  g_object_bind_property (window->priv->title_chooser, "sensitive",
      window->priv->title_entry, "sensitive", G_BINDING_SYNC_CREATE);

  g_object_bind_property (window->priv->title_chooser, "sensitive",
      window->priv->chapter_view, "sensitive", G_BINDING_SYNC_CREATE);

  window->priv->chapter_store = gtk_tree_view_get_model (GTK_TREE_VIEW (window->priv->chapter_view));

  g_signal_connect_swapped (window->priv->chapter_store, "selection-changed",
      G_CALLBACK (ogmrip_main_window_chapter_selection_changed), window);

  action = g_action_map_lookup_action (G_ACTION_MAP (window), "select_all");
  g_object_bind_property (window->priv->title_chooser, "sensitive",
      action, "enabled", G_BINDING_SYNC_CREATE);

  action = g_action_map_lookup_action (G_ACTION_MAP (window), "deselect_all");
  g_object_bind_property (window->priv->title_chooser, "sensitive",
      action, "enabled", G_BINDING_SYNC_CREATE);

  g_signal_emit_by_name (window->priv->title_chooser, "changed");
}

GtkWidget *
ogmrip_main_window_new (OGMRipApplication *application, OGMRipEncodingManager *manager)
{
  OGMRipMainWindow *window;

  g_return_val_if_fail (OGMRIP_IS_APPLICATION (application), NULL);

  window = g_object_new (OGMRIP_TYPE_MAIN_WINDOW, NULL);
  gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (application));

  window->priv->manager = g_object_ref (manager);

  if (ogmrip_application_get_is_prepared (application))
    ogmrip_main_window_app_prepared (window, application);
  else
    g_signal_connect_swapped (application, "prepare",
        G_CALLBACK (ogmrip_main_window_app_prepared), window);

  return GTK_WIDGET (window);
}

gboolean
ogmrip_main_window_load_path (OGMRipMainWindow *window, const gchar *path)
{
  OGMRipMedia *media;

  media = ogmrip_media_new (path);
  if (!media)
  {
    ogmrip_run_error_dialog (GTK_WINDOW (window), NULL, _("Could not open the media"));
    return FALSE;
  }

  ogmrip_main_window_load_media (window, media);
  g_object_unref (media);

  return TRUE;
}
