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

#include "ogmrip-pref-dialog.h"

#include "ogmrip-helper.h"
#include "ogmrip-settings.h"

#include <glib/gi18n.h>

#define OGMRIP_UI_FILE "ogmrip" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-pref-dialog.ui"
#define OGMRIP_UI_ROOT "root"

extern GSettings *settings;

static void
ogmrip_pref_dialog_folder_setting_changed (GtkWidget *chooser, const gchar *key);

static void
ogmrip_pref_dialog_folder_chooser_changed (GtkWidget *chooser, const gchar *key)
{
  gchar *path;

  g_signal_handlers_block_by_func (settings,
      ogmrip_pref_dialog_folder_setting_changed, chooser);

  path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
  g_settings_set_string (settings, key, path);
  g_free (path);

  g_signal_handlers_unblock_by_func (settings,
      ogmrip_pref_dialog_folder_setting_changed, chooser);
}

static void
ogmrip_pref_dialog_folder_setting_changed (GtkWidget *chooser, const gchar *key)
{
  gchar *path;

  g_signal_handlers_block_by_func (chooser,
      ogmrip_pref_dialog_folder_chooser_changed, (gpointer) key);

  path = g_settings_get_string (settings, key);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), path);
  g_free (path);

  g_signal_handlers_unblock_by_func (chooser,
      ogmrip_pref_dialog_folder_chooser_changed, (gpointer) key);
}

static void
ogmrip_pref_dialog_lang_setting_changed (GtkWidget *chooser, const gchar *key);

static void
ogmrip_pref_dialog_lang_chooser_changed (GtkWidget *chooser, const gchar *key)
{
  gint lang;

  g_signal_handlers_block_by_func (settings,
      ogmrip_pref_dialog_lang_setting_changed, chooser);

  lang = ogmrip_language_chooser_widget_get_active (OGMRIP_LANGUAGE_CHOOSER_WIDGET (chooser));
  g_settings_set (settings, key, "u", lang);

  g_signal_handlers_unblock_by_func (settings,
      ogmrip_pref_dialog_lang_setting_changed, chooser);
}

static void
ogmrip_pref_dialog_lang_setting_changed (GtkWidget *chooser, const gchar *key)
{
  g_signal_handlers_block_by_func (chooser,
      ogmrip_pref_dialog_lang_chooser_changed, (gpointer) key);

  ogmrip_language_chooser_widget_set_active (OGMRIP_LANGUAGE_CHOOSER_WIDGET (chooser),
      g_settings_get_uint (settings, key));

  g_signal_handlers_unblock_by_func (chooser,
      ogmrip_pref_dialog_lang_chooser_changed, (gpointer) key);
}

static void
ogmrip_pref_dialog_disconnect (GtkWidget *widget, GCallback callback)
{
  g_signal_handlers_disconnect_by_func (settings, callback, widget);
}

G_DEFINE_TYPE (OGMRipPrefDialog, ogmrip_pref_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_pref_dialog_class_init (OGMRipPrefDialogClass *klass)
{
}

static void
ogmrip_pref_dialog_init (OGMRipPrefDialog *dialog)
{
  GError *error = NULL;

  GtkBuilder *builder;
  GtkWidget *area, *widget, *chooser;

  GtkTreeModel *model;
  GtkTreeIter iter;

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_UI_FILE, &error))
    g_error ("Couldn't load builder file: %s", error->message);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog), _("_Close"), GTK_RESPONSE_CLOSE, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Preferences"));

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  widget = gtk_builder_get_widget (builder, OGMRIP_UI_ROOT);
  gtk_container_add (GTK_CONTAINER (area), widget);
  gtk_widget_show (widget);

  /*
   * General
   */

  widget = gtk_builder_get_widget (builder, "output-dir-chooser");

  ogmrip_pref_dialog_folder_setting_changed (widget, OGMRIP_SETTINGS_OUTPUT_DIR);

  g_signal_connect (widget, "current-folder-changed",
      G_CALLBACK (ogmrip_pref_dialog_folder_chooser_changed), OGMRIP_SETTINGS_OUTPUT_DIR);
  g_signal_connect_swapped (settings, "changed::" OGMRIP_SETTINGS_OUTPUT_DIR,
      G_CALLBACK (ogmrip_pref_dialog_folder_setting_changed), widget);
  g_signal_connect (widget, "destroy",
      G_CALLBACK (ogmrip_pref_dialog_disconnect), ogmrip_pref_dialog_folder_setting_changed);

  gtk_file_chooser_set_action (GTK_FILE_CHOOSER (widget), 
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

  widget = gtk_builder_get_widget (builder, "filename-combo");
  g_settings_bind (settings, OGMRIP_SETTINGS_FILENAME, widget, "active", G_SETTINGS_BIND_DEFAULT);

  area = gtk_builder_get_widget (builder, "general-page");

  chooser = ogmrip_language_chooser_widget_new ();
  gtk_grid_attach (GTK_GRID (area), chooser, 1, 4, 1, 1);
  gtk_widget_set_hexpand (chooser, TRUE);
  gtk_widget_show (chooser);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      OGMRIP_LANGUAGE_STORE_NAME_COLUMN, _("No favorite audio language"),
      OGMRIP_LANGUAGE_STORE_CODE_COLUMN, 0, -1);

  ogmrip_pref_dialog_lang_setting_changed (chooser, OGMRIP_SETTINGS_PREF_AUDIO);

  g_signal_connect (chooser, "changed",
      G_CALLBACK (ogmrip_pref_dialog_lang_chooser_changed), OGMRIP_SETTINGS_PREF_AUDIO);
  g_signal_connect_swapped (settings, "changed::" OGMRIP_SETTINGS_PREF_AUDIO,
      G_CALLBACK (ogmrip_pref_dialog_lang_setting_changed), chooser);
  g_signal_connect (chooser, "destroy",
      G_CALLBACK (ogmrip_pref_dialog_disconnect), ogmrip_pref_dialog_lang_setting_changed);

  chooser = ogmrip_language_chooser_widget_new ();
  gtk_grid_attach (GTK_GRID (area), chooser, 1, 5, 1, 1);
  gtk_widget_set_hexpand (chooser, TRUE);
  gtk_widget_show (chooser);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      OGMRIP_LANGUAGE_STORE_NAME_COLUMN, _("No favorite subtitle language"),
      OGMRIP_LANGUAGE_STORE_CODE_COLUMN, 0, -1);

  ogmrip_pref_dialog_lang_setting_changed (chooser, OGMRIP_SETTINGS_PREF_SUBP);

  g_signal_connect (chooser, "changed",
      G_CALLBACK (ogmrip_pref_dialog_lang_chooser_changed), OGMRIP_SETTINGS_PREF_SUBP);
  g_signal_connect_swapped (settings, "changed::" OGMRIP_SETTINGS_PREF_SUBP,
      G_CALLBACK (ogmrip_pref_dialog_lang_setting_changed), chooser);
  g_signal_connect (chooser, "destroy",
      G_CALLBACK (ogmrip_pref_dialog_disconnect), ogmrip_pref_dialog_lang_setting_changed);

  chooser = ogmrip_language_chooser_widget_new ();
  gtk_grid_attach (GTK_GRID (area), chooser, 1, 6, 1, 1);
  gtk_widget_set_hexpand (chooser, TRUE);
  gtk_widget_show (chooser);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      OGMRIP_LANGUAGE_STORE_NAME_COLUMN, _("No chapters language"),
      OGMRIP_LANGUAGE_STORE_CODE_COLUMN, 0, -1);

  ogmrip_pref_dialog_lang_setting_changed (chooser, OGMRIP_SETTINGS_CHAPTER_LANG);

  g_signal_connect (chooser, "changed",
      G_CALLBACK (ogmrip_pref_dialog_lang_chooser_changed), OGMRIP_SETTINGS_CHAPTER_LANG);
  g_signal_connect_swapped (settings, "changed::" OGMRIP_SETTINGS_CHAPTER_LANG,
      G_CALLBACK (ogmrip_pref_dialog_lang_setting_changed), chooser);
  g_signal_connect (chooser, "destroy",
      G_CALLBACK (ogmrip_pref_dialog_disconnect), ogmrip_pref_dialog_lang_setting_changed);

  /*
   * Advanced
   */

  widget = gtk_builder_get_widget (builder, "copy-dvd-check");
  g_settings_bind (settings, OGMRIP_SETTINGS_COPY_DVD, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "after-enc-combo");
  g_settings_bind (settings, OGMRIP_SETTINGS_AFTER_ENC, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "threads-spin");
  g_settings_bind (settings, OGMRIP_SETTINGS_THREADS, widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "keep-tmp-check");
  g_settings_bind (settings, OGMRIP_SETTINGS_KEEP_TMP, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "log-check");
  g_settings_bind (settings, OGMRIP_SETTINGS_LOG_OUTPUT, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "tmp-dir-chooser");

  ogmrip_pref_dialog_folder_setting_changed (widget, OGMRIP_SETTINGS_TMP_DIR);

  g_signal_connect (widget, "current-folder-changed",
      G_CALLBACK (ogmrip_pref_dialog_folder_chooser_changed), OGMRIP_SETTINGS_TMP_DIR);
  g_signal_connect_swapped (settings, "changed::" OGMRIP_SETTINGS_TMP_DIR,
      G_CALLBACK (ogmrip_pref_dialog_folder_setting_changed), widget);
  g_signal_connect (widget, "destroy",
      G_CALLBACK (ogmrip_pref_dialog_disconnect), ogmrip_pref_dialog_folder_setting_changed);

  gtk_file_chooser_set_action (GTK_FILE_CHOOSER (widget), 
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

  g_object_unref (builder);
}

GtkWidget *
ogmrip_pref_dialog_new (void)
{
  return g_object_new (OGMRIP_TYPE_PREF_DIALOG, NULL);
}

