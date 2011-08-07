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

#include "ogmjob.h"
#include "ogmrip-gtk.h"
#include "ogmrip-settings.h"

#include "ogmrip-options-dialog.h"
#include "ogmrip-pref-dialog.h"
#include "ogmrip-profiles-dialog.h"
#include "ogmrip-queue-dialog.h"

#ifdef HAVE_ENCHANT_SUPPORT
#include "ogmrip-spell-dialog.h"
#endif /* HAVE_ENCHANT_SUPPORT */

#include <glib/gi18n.h>
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

  GtkWidget *title_entry;
  GtkWidget *title_chooser;
  GtkWidget *length_label;

  GtkWidget *audio_list;
  GtkWidget *subp_list;
  GtkWidget *chapter_list;

  GtkWidget *relative_check;
  GtkWidget *angle_spin;

  GtkAction *extract_action;

  OGMRipPlayer *player;
} OGMRipData;

extern GSettings *settings;

/*
 * Loads a media
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

    gtk_entry_set_text (GTK_ENTRY (data->title_entry), label ? label : "");
  }
}

/*
 * Adds an audio chooser
 */
static void
ogmrip_main_add_audio_chooser (OGMRipData *data)
{
  GtkWidget *chooser;
  OGMDvdTitle *title;

  chooser = ogmrip_audio_chooser_widget_new ();
  gtk_container_add (GTK_CONTAINER (data->audio_list), chooser);
  gtk_widget_show (chooser);

  g_signal_connect_swapped (chooser, "add-clicked",
      G_CALLBACK (ogmrip_main_add_audio_chooser), data);
  g_signal_connect_swapped (chooser, "remove-clicked",
      G_CALLBACK (gtk_container_remove), data->audio_list);

  title = ogmdvd_title_chooser_get_active (OGMDVD_TITLE_CHOOSER (data->title_chooser));
  ogmrip_source_chooser_set_title (OGMRIP_SOURCE_CHOOSER (chooser), title);

  if (title)
  {
    gint lang;

    g_settings_get (settings, OGMRIP_SETTINGS_PREF_AUDIO, "u", &lang);
    ogmrip_source_chooser_select_language (OGMRIP_SOURCE_CHOOSER (chooser), lang);

    if (gtk_combo_box_get_active (GTK_COMBO_BOX (chooser)) < 0)
    {
      if (ogmdvd_title_get_n_audio_streams (title) > 0)
        gtk_combo_box_set_active (GTK_COMBO_BOX (chooser), 1);
      else
        gtk_combo_box_set_active (GTK_COMBO_BOX (chooser), 0);
    }
  }
}

/*
 * Adds a subp chooser
 */
static void
ogmrip_main_add_subp_chooser (OGMRipData *data)
{
  GtkWidget *chooser;
  OGMDvdTitle *title;

  chooser = ogmrip_subp_chooser_widget_new ();
  gtk_container_add (GTK_CONTAINER (data->subp_list), chooser);
  gtk_widget_show (chooser);

  g_signal_connect_swapped (chooser, "add-clicked",
      G_CALLBACK (ogmrip_main_add_subp_chooser), data);
  g_signal_connect_swapped (chooser, "remove-clicked",
      G_CALLBACK (gtk_container_remove), data->subp_list);

  title = ogmdvd_title_chooser_get_active (OGMDVD_TITLE_CHOOSER (data->title_chooser));
  ogmrip_source_chooser_set_title (OGMRIP_SOURCE_CHOOSER (chooser), title);

  if (title)
  {
    gint lang;

    g_settings_get (settings, OGMRIP_SETTINGS_PREF_SUBP, "u", &lang);
    ogmrip_source_chooser_select_language (OGMRIP_SOURCE_CHOOSER (chooser), lang);

    if (gtk_combo_box_get_active (GTK_COMBO_BOX (chooser)) < 0)
      gtk_combo_box_set_active (GTK_COMBO_BOX (chooser), 0);
  }
}

/*
 * Add audio streams and files to the encoding
 */
/*
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
*/
/*
 * Add subp streams and files to the encoding
 */
/*
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
*/
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

        ogmdvd_title_chooser_set_disc (OGMDVD_TITLE_CHOOSER (data->title_chooser), NULL);
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

static OGMRipContainer *
ogmrip_main_create_container (OGMRipData *data, OGMRipProfile *profile)
{
  OGMRipContainer *container;
  GType type;

  type = ogmrip_profile_get_container_type (profile, NULL);
  g_assert (type != G_TYPE_NONE);

  container = g_object_new (type, NULL);
  ogmrip_container_set_label (container,
      gtk_entry_get_text (GTK_ENTRY (data->title_entry)));

  /*
   * TODO add chapters' names
   */

  return container;
}

static OGMRipCodec *
ogmrip_main_create_video_codec (OGMRipData *data, OGMRipProfile *profile, OGMDvdVideoStream *stream)
{
  GType type;

  type = ogmrip_profile_get_video_codec_type (profile, NULL);
  if (type == G_TYPE_NONE)
    return NULL;

  return g_object_new (type, "input", stream, NULL);
}

/*
 * When the extract button is activated
 */
static void
ogmrip_main_extract_activated (OGMRipData *data)
{
  GtkWidget *dialog;

  OGMDvdTitle *title;
  OGMRipEncoding *encoding;
  OGMRipProfile *profile;
  OGMRipContainer *container;
  OGMRipCodec *codec;

  guint start_chap;
  gint end_chap, response;
  GList *list, *link;

  title = ogmdvd_title_chooser_get_active (OGMDVD_TITLE_CHOOSER (data->title_chooser));

  encoding = ogmrip_encoding_new ();
  ogmrip_encoding_set_relative (encoding,
      gtk_widget_is_sensitive (data->relative_check) &
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->relative_check)));

  /*
   * TODO set filename
   */

  dialog = ogmrip_options_dialog_new (encoding, title);
  gtk_window_set_parent (GTK_WINDOW (dialog), GTK_WINDOW (data->window));

  response = gtk_dialog_run (GTK_DIALOG (dialog));

  profile = ogmrip_encoding_get_profile (encoding);

  container = ogmrip_main_create_container (data, profile);
  ogmrip_encoding_set_container (encoding, container);
  g_object_unref (container);

  ogmrip_chapter_list_get_selected (OGMRIP_CHAPTER_LIST (data->chapter_list), &start_chap, &end_chap);

  codec = ogmrip_main_create_video_codec (data, profile, ogmdvd_title_get_video_stream (title));
  if (codec)
  {
    guint x, y, w, h;

    ogmrip_codec_set_chapters (codec, start_chap, end_chap);

    ogmrip_video_codec_set_angle (OGMRIP_VIDEO_CODEC (codec),
        gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->angle_spin)));

    ogmrip_options_dialog_get_crop_size (OGMRIP_OPTIONS_DIALOG (dialog), &x, &y, &w, &h);
    ogmrip_video_codec_set_crop_size (OGMRIP_VIDEO_CODEC (codec), x, y, w, h);

    ogmrip_options_dialog_get_scale_size (OGMRIP_OPTIONS_DIALOG (dialog), &w, &h);
    ogmrip_video_codec_set_scale_size (OGMRIP_VIDEO_CODEC (codec), w, h);
  }

  list = gtk_container_get_children (GTK_CONTAINER (data->audio_list));
  for (link = list; link; link = link->next)
  {
    /*
     * TODO add audio streams and files
     */
  }
  g_list_free (list);

  list = gtk_container_get_children (GTK_CONTAINER (data->subp_list));
  for (link = list; link; link = link->next)
  {
    /*
     * TODO add subp streams and files
     */
  }

  gtk_widget_destroy (dialog);

  if (response == OGMRIP_RESPONSE_EXTRACT)
  {
    /*
     * TODO encode
     */
  }
  else if (response == OGMRIP_RESPONSE_ENQUEUE)
  {
    /*
     * TODO enqueue
     */
  }
}

/*
 * When the play button is activated
 */
static void
ogmrip_main_play_activated (OGMRipData *data, GtkWidget *button)
{
  GError *error = NULL;

  OGMDvdTitle *title;
  guint start_chap;
  gint end_chap;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
  {
    ogmrip_player_stop (data->player);
    return;
  }

  title = ogmdvd_title_chooser_get_active (OGMDVD_TITLE_CHOOSER (data->title_chooser));
  ogmrip_player_set_title (data->player, title);

  ogmrip_chapter_list_get_selected (OGMRIP_CHAPTER_LIST (data->chapter_list), &start_chap, &end_chap);
  ogmrip_player_set_chapters (data->player, start_chap, end_chap);

  /*
   * TODO set audio and subp streams
   */

  if (!ogmrip_player_play (data->player, &error))
  {
    ogmrip_message_dialog (GTK_WINDOW (data->window), GTK_MESSAGE_ERROR,
        "<big><b>%s</b></big>\n\n%s", _("Can't play DVD title"), error->message);
    g_error_free (error);
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
      /*
       * TODO import chapters
       */

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
      /*
       * TODO export chapters
       */

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
  gtk_window_set_parent (GTK_WINDOW (dialog), GTK_WINDOW (data->window));

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

  dialog = ogmrip_profiles_dialog_new ();
  gtk_window_set_parent (GTK_WINDOW (dialog), GTK_WINDOW (data->window));

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

  dialog = ogmrip_queue_dialog_new ();
  gtk_window_set_parent (GTK_WINDOW (dialog), GTK_WINDOW (data->window));

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
      "copyright", "(c) 2004-2010 Olivier Rolland",
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
  OGMDvdTitle *title;
  gint angles;

  title = ogmdvd_title_chooser_get_active (OGMDVD_TITLE_CHOOSER (data->title_chooser));
  gtk_widget_set_sensitive (data->title_chooser, title != NULL);

  gtk_container_clear (GTK_CONTAINER (data->audio_list));
  ogmrip_main_add_audio_chooser (data);

  gtk_container_clear (GTK_CONTAINER (data->subp_list));
  ogmrip_main_add_subp_chooser (data);

  ogmdvd_chapter_list_set_title (OGMDVD_CHAPTER_LIST (data->chapter_list), title);
  ogmrip_chapter_list_select_all (OGMRIP_CHAPTER_LIST (data->chapter_list));

  angles = title ? ogmdvd_title_get_n_angles (title) : 1;
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

  sensitive = ogmrip_chapter_list_get_selected (OGMRIP_CHAPTER_LIST (data->chapter_list), &start_chap, &end_chap);
  if (!sensitive)
    gtk_label_set_text (GTK_LABEL (data->length_label), "");
  else
  {
    OGMDvdTitle *title;
    OGMDvdTime time_;

    title = ogmdvd_title_chooser_get_active (OGMDVD_TITLE_CHOOSER (data->title_chooser));
    if (ogmdvd_title_get_chapters_length (title, start_chap, end_chap, &time_) > 0)
    {
      gchar *str;

      str = g_strdup_printf ("%02d:%02d:%02d", time_.hour, time_.min, time_.sec);
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
  if (data->disc)
  {
    ogmdvd_disc_unref (data->disc);
    data->disc = NULL;
  }

  if (data->player)
  {
    g_object_unref (data->player);
    data->player = NULL;
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

  data->title_chooser = ogmdvd_title_chooser_widget_new ();
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

  data->chapter_list = ogmrip_chapter_list_new ();
  gtk_container_add (GTK_CONTAINER (widget), data->chapter_list);
  gtk_widget_show (data->chapter_list);

  g_object_bind_property (data->title_chooser, "sensitive",
      data->chapter_list, "sensitive", G_BINDING_SYNC_CREATE);
  g_signal_connect_swapped (data->chapter_list, "selection-changed", 
      G_CALLBACK (ogmrip_main_chapter_selection_changed), data);

  action = gtk_action_group_get_action (action_group, "SelectAll");
  g_object_bind_property (data->title_chooser, "sensitive",
      action, "sensitive", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_chapter_list_select_all), data->chapter_list);

  action = gtk_action_group_get_action (action_group, "DeselectAll");
  g_object_bind_property (data->title_chooser, "sensitive",
      action, "sensitive", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_chapter_list_deselect_all), data->chapter_list);

  data->relative_check = gtk_builder_get_widget (builder, "relative-check");
  data->angle_spin = gtk_builder_get_widget (builder, "angle-spin");

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

  ogmrip_settings_init ();

  ogmrip_plugin_init ();
  ogmrip_options_plugin_init ();

#ifdef HAVE_LIBNOTIFY_SUPPORT
  notify_init (PACKAGE_NAME);
#endif /* HAVE_LIBNOTIFY_SUPPORT */

  engine = ogmrip_profile_engine_get_default ();
  g_settings_bind (settings, OGMRIP_SETTINGS_PROFILES,
      engine, "profiles", G_SETTINGS_BIND_DEFAULT);
}

static void
ogmrip_uninit (void)
{
  OGMRipProfileEngine *engine;

  engine = ogmrip_profile_engine_get_default ();
  g_object_unref (engine);

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

