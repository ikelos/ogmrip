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

#include "ogmrip-pref-dialog.h"

#include "ogmrip-helper.h"
#include "ogmrip-settings.h"

#include <glib/gi18n.h>

#define OGMRIP_UI_RES  "/org/ogmrip/ogmrip-pref-dialog.ui"
#define OGMRIP_UI_ROOT "root"

struct _OGMRipPrefDialogPriv
{
  GtkWidget *general_page;
  GtkWidget *audio_lang_chooser;
  GtkWidget *subp_lang_chooser;
  GtkWidget *chapters_lang_chooser;
  GtkWidget *output_dir_chooser;
  GtkWidget *tmp_dir_chooser;
  GtkWidget *filename_combo;
  GtkWidget *after_enc_combo;
  GtkWidget *threads_spin;
  GtkWidget *copy_dvd_check;
  GtkWidget *keep_tmp_check;
  GtkWidget *log_check;
};

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

G_DEFINE_TYPE_WITH_PRIVATE (OGMRipPrefDialog, ogmrip_pref_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_pref_dialog_class_init (OGMRipPrefDialogClass *klass)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), OGMRIP_UI_RES);

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipPrefDialog, general_page);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipPrefDialog, audio_lang_chooser);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipPrefDialog, subp_lang_chooser);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipPrefDialog, chapters_lang_chooser);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipPrefDialog, output_dir_chooser);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipPrefDialog, tmp_dir_chooser);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipPrefDialog, filename_combo);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipPrefDialog, after_enc_combo);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipPrefDialog, threads_spin);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipPrefDialog, copy_dvd_check);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipPrefDialog, keep_tmp_check);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipPrefDialog, log_check);
}

static void
ogmrip_pref_dialog_init (OGMRipPrefDialog *dialog)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  g_type_ensure (OGMRIP_TYPE_LANGUAGE_CHOOSER_WIDGET);

  gtk_widget_init_template (GTK_WIDGET (dialog));

  dialog->priv = ogmrip_pref_dialog_get_instance_private (dialog);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (dialog->priv->audio_lang_chooser));
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      OGMRIP_LANGUAGE_STORE_NAME_COLUMN, _("No favorite audio language"),
      OGMRIP_LANGUAGE_STORE_CODE_COLUMN, 0, -1);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (dialog->priv->subp_lang_chooser));
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      OGMRIP_LANGUAGE_STORE_NAME_COLUMN, _("No favorite subtitle language"),
      OGMRIP_LANGUAGE_STORE_CODE_COLUMN, 0, -1);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (dialog->priv->chapters_lang_chooser));
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      OGMRIP_LANGUAGE_STORE_NAME_COLUMN, _("No chapters language"),
      OGMRIP_LANGUAGE_STORE_CODE_COLUMN, 0, -1);

  ogmrip_pref_dialog_folder_setting_changed (dialog->priv->output_dir_chooser, OGMRIP_SETTINGS_OUTPUT_DIR);

  g_signal_connect (dialog->priv->output_dir_chooser, "current-folder-changed",
      G_CALLBACK (ogmrip_pref_dialog_folder_chooser_changed), OGMRIP_SETTINGS_OUTPUT_DIR);
  g_signal_connect_swapped (settings, "changed::" OGMRIP_SETTINGS_OUTPUT_DIR,
      G_CALLBACK (ogmrip_pref_dialog_folder_setting_changed), dialog->priv->output_dir_chooser);
  g_signal_connect (dialog->priv->output_dir_chooser, "destroy",
      G_CALLBACK (ogmrip_pref_dialog_disconnect), ogmrip_pref_dialog_folder_setting_changed);

  ogmrip_pref_dialog_folder_setting_changed (dialog->priv->tmp_dir_chooser, OGMRIP_SETTINGS_TMP_DIR);

  g_signal_connect (dialog->priv->tmp_dir_chooser, "current-folder-changed",
      G_CALLBACK (ogmrip_pref_dialog_folder_chooser_changed), OGMRIP_SETTINGS_TMP_DIR);
  g_signal_connect_swapped (settings, "changed::" OGMRIP_SETTINGS_TMP_DIR,
      G_CALLBACK (ogmrip_pref_dialog_folder_setting_changed), dialog->priv->tmp_dir_chooser);
  g_signal_connect (dialog->priv->tmp_dir_chooser, "destroy",
      G_CALLBACK (ogmrip_pref_dialog_disconnect), ogmrip_pref_dialog_folder_setting_changed);

  g_settings_bind (settings, OGMRIP_SETTINGS_PREF_AUDIO,
                   dialog->priv->audio_lang_chooser, "language",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_SETTINGS_PREF_SUBP,
                   dialog->priv->subp_lang_chooser, "language",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_SETTINGS_CHAPTER_LANG,
                   dialog->priv->chapters_lang_chooser, "language",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_SETTINGS_FILENAME,
                   dialog->priv->filename_combo, "active",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_SETTINGS_COPY_DVD,
                   dialog->priv->copy_dvd_check, "active",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_SETTINGS_AFTER_ENC,
                   dialog->priv->after_enc_combo, "active",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_SETTINGS_THREADS,
                   dialog->priv->threads_spin, "value",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_SETTINGS_KEEP_TMP,
                   dialog->priv->keep_tmp_check, "active",
                   G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_SETTINGS_LOG_OUTPUT,
                   dialog->priv->log_check, "active",
                   G_SETTINGS_BIND_DEFAULT);
}

GtkWidget *
ogmrip_pref_dialog_new (void)
{
  return g_object_new (OGMRIP_TYPE_PREF_DIALOG, NULL);
}

