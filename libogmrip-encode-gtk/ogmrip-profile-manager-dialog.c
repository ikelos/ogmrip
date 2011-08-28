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

#include "ogmrip-profile-manager-dialog.h"
#include "ogmrip-profile-editor-dialog.h"
#include "ogmrip-profile-store.h"
#include "ogmrip-error-dialog.h"

#include "ogmrip-helper.h"

#include <ogmrip.h>

#include <glib/gi18n-lib.h>

#define OGMRIP_GLADE_FILE "ogmrip" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-profiles.glade"
#define OGMRIP_GLADE_ROOT "root"

#define OGMRIP_PROFILE_MANAGER_DIALOG_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_PROFILE_MANAGER_DIALOG, OGMRipProfileManagerDialogPriv))

struct _OGMRipProfileManagerDialogPriv
{
  OGMRipProfileEngine *engine;
  OGMRipProfileStore *store;
  GtkTreeSelection *selection;
};

static void ogmrip_profile_manager_dialog_dispose (GObject *gobject);

static void
ogmrip_profile_manager_dialog_profile_name_changed (GtkTextBuffer *buffer, GtkWidget *dialog)
{
  GtkTextIter start, end;
  gchar *text;

  gtk_text_buffer_get_bounds (buffer, &start, &end);
  text = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
      GTK_RESPONSE_ACCEPT, text != NULL && text[0] != '\0');

  g_free (text);
}

static gchar *
ogmrip_profile_manager_dialog_run_profile_dialog (GtkWindow *parent, const gchar *old_name)
{
  GtkWidget *dialog, *area, *vbox, *label, *frame, *entry;
  GtkTextBuffer *buffer;

  gchar *new_name = NULL;

  dialog = gtk_dialog_new_with_buttons (old_name ? _("Rename profile") : _("New profile"), NULL,
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
      GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT, FALSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  gtk_window_set_default_size (GTK_WINDOW (dialog), 250, 125);
  gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (area), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("_Profile name:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  entry = gtk_text_view_new ();
  gtk_text_view_set_accepts_tab (GTK_TEXT_VIEW (entry), FALSE);
  gtk_container_add (GTK_CONTAINER (frame), entry);
  gtk_widget_show (entry);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (entry));
  g_signal_connect (buffer, "changed",
      G_CALLBACK (ogmrip_profile_manager_dialog_profile_name_changed), dialog);

  if (old_name)
    gtk_text_buffer_set_text (buffer, old_name, -1);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    GtkTextIter start, end;

    gtk_text_buffer_get_bounds (buffer, &start, &end);
    new_name = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
  }
  gtk_widget_destroy (dialog);

  return new_name;
}

static void
ogmrip_profile_manager_dialog_new_button_clicked (OGMRipProfileManagerDialog *dialog)
{
  gchar *name;

  name = ogmrip_profile_manager_dialog_run_profile_dialog (GTK_WINDOW (dialog), NULL);
  if (name)
  {
    OGMRipProfile *profile;

    profile = ogmrip_profile_new (NULL);

    g_settings_set_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME, name);
    g_free (name);

    ogmrip_profile_engine_add (dialog->priv->engine, profile);
    g_object_unref (profile);
  }
}

static void
ogmrip_profile_manager_dialog_copy_button_clicked (OGMRipProfileManagerDialog *dialog)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (dialog->priv->selection, &model, &iter))
  {
    OGMRipProfile *profile, *new_profile;
    gchar *name, *new_name;

    profile = ogmrip_profile_store_get_profile (OGMRIP_PROFILE_STORE (model), &iter);

    name = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME);
    new_name = g_strconcat (_("Copy of"), " ", name, NULL);
    g_free (name);

    new_profile = ogmrip_profile_copy (profile, NULL);
    g_settings_set_string (G_SETTINGS (new_profile), OGMRIP_PROFILE_NAME, new_name);
    g_free (new_name);

    ogmrip_profile_engine_add (dialog->priv->engine, new_profile);
    g_object_unref (new_profile);
  }
}

static void
ogmrip_profile_manager_dialog_edit_button_clicked (OGMRipProfileManagerDialog *parent)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (parent->priv->selection, &model, &iter))
  {
    OGMRipProfile *profile;
    GtkWidget *dialog;

    profile = ogmrip_profile_store_get_profile (OGMRIP_PROFILE_STORE (model), &iter);

    dialog = ogmrip_profile_editor_dialog_new (profile);
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));
    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), FALSE);

    gtk_dialog_run (GTK_DIALOG (dialog));

    gtk_widget_destroy (dialog);
  }
}

static void
ogmrip_profile_manager_dialog_remove_button_clicked (OGMRipProfileManagerDialog *parent)
{
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (gtk_tree_selection_get_selected (parent->priv->selection, &model, &iter))
  {
    OGMRipProfile *profile;
    GtkWidget *dialog;
    gchar *name;

    profile = ogmrip_profile_store_get_profile (OGMRIP_PROFILE_STORE (model), &iter);

    dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (parent),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, _("Delete profile ?"));
    gtk_window_set_title (GTK_WINDOW (dialog), _("Delete profile ?"));

    name = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME);
    gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog), "%s", name);
    g_free (name);

    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
      ogmrip_profile_engine_remove (parent->priv->engine, profile);
    gtk_widget_destroy (dialog);
  }
}

static void
ogmrip_profile_manager_dialog_rename_button_clicked (OGMRipProfileManagerDialog *parent)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (parent->priv->selection, &model, &iter))
  {
    OGMRipProfile *profile;
    gchar *old_name, *new_name;

    profile = ogmrip_profile_store_get_profile (OGMRIP_PROFILE_STORE (model), &iter);

    old_name = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME);

    new_name = ogmrip_profile_manager_dialog_run_profile_dialog (GTK_WINDOW (parent), old_name);
    if (new_name && !g_str_equal (new_name, old_name))
      g_settings_set_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME, new_name);

    g_free (new_name);
    g_free (old_name);
  }
}

static void
ogmrip_profile_manager_dialog_export_button_clicked (OGMRipProfileManagerDialog *parent)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (parent->priv->selection, &model, &iter))
  {
    OGMRipProfile *profile;

    profile = ogmrip_profile_store_get_profile (OGMRIP_PROFILE_STORE (model), &iter);
    if (profile)
    {
      GtkWidget *dialog;

      dialog = gtk_file_chooser_dialog_new (_("Export Profile"),
          GTK_WINDOW (parent), GTK_FILE_CHOOSER_ACTION_SAVE,
          GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
          GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
          NULL);
      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

      if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
      {
        GError *error = NULL;
        GFile *file;

        file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
        if (!ogmrip_profile_export (profile, file, &error))
        {
          ogmrip_run_error_dialog (GTK_WINDOW (dialog), error, _("Could not export the profile"));
          g_clear_error (&error);
        }
        g_object_unref (file);
      }
      gtk_widget_destroy (dialog);
    }
  }
}

static void
ogmrip_profile_manager_dialog_import_button_clicked (OGMRipProfileManagerDialog *parent)
{
  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new (_("Select profile to import"),
      GTK_WINDOW (parent), GTK_FILE_CHOOSER_ACTION_OPEN,
      GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
      NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    GError *error = NULL;
    OGMRipProfile *profile;
    GFile *file;

    gtk_widget_hide (dialog);

    file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));

    profile = ogmrip_profile_new_from_file (file, NULL);
    if (profile)
    {
      ogmrip_profile_engine_add (parent->priv->engine, profile);
      g_object_unref (profile);
    }
    else
    {
      gtk_widget_destroy (dialog);
      dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
          GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Cannot load profile"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), 
          "%s", error ? error->message : _("Unknown error"));
      g_clear_error (&error);

      gtk_dialog_run (GTK_DIALOG (dialog));
    }

    g_object_unref (file);
  }
  gtk_widget_destroy (dialog);
}

static void
ogmrip_profile_manager_dialog_set_button_sensitivity (GtkTreeSelection *selection, GtkWidget *button)
{
  gtk_widget_set_sensitive (button,
      gtk_tree_selection_get_selected (selection, NULL, NULL));
}

G_DEFINE_TYPE (OGMRipProfileManagerDialog, ogmrip_profile_manager_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_profile_manager_dialog_class_init (OGMRipProfileManagerDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_profile_manager_dialog_dispose;

  g_type_class_add_private (klass, sizeof (OGMRipProfileManagerDialogPriv));
}

static void
ogmrip_profile_manager_dialog_init (OGMRipProfileManagerDialog *dialog)
{
  GError *error = NULL;

  GtkWidget *area, *widget, *image;
  GtkBuilder *builder;

  GtkTreeViewColumn *column;
  GtkCellRenderer *cell;

  dialog->priv = OGMRIP_PROFILE_MANAGER_DIALOG_GET_PRIVATE (dialog);

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_GLADE_FILE, &error))
  {
    g_critical ("Couldn't load builder file: %s", error->message);
    return;
  }

  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 450, -1);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Edit Profiles"));

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  widget = gtk_builder_get_widget (builder, OGMRIP_GLADE_ROOT);
  gtk_container_add (GTK_CONTAINER (area), widget);
  gtk_widget_show (widget);

  widget = gtk_builder_get_widget (builder, "treeview");

  dialog->priv->store = ogmrip_profile_store_new (NULL);
  gtk_tree_view_set_model (GTK_TREE_VIEW (widget), GTK_TREE_MODEL (dialog->priv->store));

  column = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell, "markup",
      OGMRIP_PROFILE_STORE_NAME_COLUMN, NULL);

  dialog->priv->engine = ogmrip_profile_engine_get_default ();
  g_object_ref (dialog->priv->engine);

  dialog->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));

  widget = gtk_builder_get_widget (builder, "edit-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_profile_manager_dialog_edit_button_clicked), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_profile_manager_dialog_set_button_sensitivity), widget);

  widget = gtk_builder_get_widget (builder, "new-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_profile_manager_dialog_new_button_clicked), dialog);

  widget = gtk_builder_get_widget (builder, "delete-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_profile_manager_dialog_remove_button_clicked), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_profile_manager_dialog_set_button_sensitivity), widget);

  widget = gtk_builder_get_widget (builder, "copy-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_profile_manager_dialog_copy_button_clicked), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_profile_manager_dialog_set_button_sensitivity), widget);

  widget = gtk_builder_get_widget (builder, "rename-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_profile_manager_dialog_rename_button_clicked), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_profile_manager_dialog_set_button_sensitivity), widget);

  image = gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (widget), image);

  widget = gtk_builder_get_widget (builder, "export-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_profile_manager_dialog_export_button_clicked), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_profile_manager_dialog_set_button_sensitivity), widget);

  image = gtk_image_new_from_stock (GTK_STOCK_SAVE, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (widget), image);

  widget = gtk_builder_get_widget (builder, "import-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_profile_manager_dialog_import_button_clicked), dialog);

  image = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (widget), image);

  g_object_unref (builder);
}

static void
ogmrip_profile_manager_dialog_dispose (GObject *gobject)
{
  OGMRipProfileManagerDialog *dialog = OGMRIP_PROFILE_MANAGER_DIALOG (gobject);

  if (dialog->priv->engine)
  {
    g_object_unref (dialog->priv->engine);
    dialog->priv->engine = NULL;
  }

  G_OBJECT_CLASS (ogmrip_profile_manager_dialog_parent_class)->dispose (gobject);
}

GtkWidget *
ogmrip_profile_manager_dialog_new (void)
{
  return g_object_new (OGMRIP_TYPE_PROFILE_MANAGER_DIALOG, NULL);
}

