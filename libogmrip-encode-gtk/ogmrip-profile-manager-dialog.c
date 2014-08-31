/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-profile-manager-dialog.h"
#include "ogmrip-profile-editor-dialog.h"
#include "ogmrip-profile-view.h"
#include "ogmrip-error-dialog.h"

#include <ogmrip-encode.h>

#include <glib/gi18n-lib.h>

#define OGMRIP_UI_RES   "/org/ogmrip/ogmrip-profile-manager-dialog.ui"
#define OGMRIP_MENU_RES "/org/ogmrip/ogmrip-profile-manager-menu.ui"

struct _OGMRipProfileManagerDialogPriv
{
  OGMRipProfileEngine *engine;

  GtkWidget *toolbar;

  GtkWidget *profile_swin;
  GtkWidget *profile_view;
  GtkTreeSelection *selection;

  GtkWidget *add_button;
  GtkWidget *remove_button;
  GtkWidget *import_button;

  GtkWidget *name_dialog;
  GtkWidget *name_entry;
  GtkTextBuffer *desc_buffer;

  GAction *remove_action;
};

static gboolean
gtk_widget_button_press_cb (GtkWidget *widget, GdkEventButton *event, GtkWidget *menu)
{
  GtkTreePath *path;

  if (event->button != 3 || event->type != GDK_BUTTON_PRESS)
    return FALSE;

  if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW (widget)))
    return FALSE;

  if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL))
    return FALSE;

  gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)), path);

  gtk_tree_path_free (path);

  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, event->time);

  return TRUE;
}

static gboolean
gtk_widget_popup_menu_cb (GtkWidget *widget, GtkWidget *menu)
{
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());

  return TRUE;
}

static void
gtk_widget_set_popup_menu (GtkWidget *widget, GMenuModel *model)
{
  GtkWidget *menu;

  menu = gtk_menu_new_from_model (G_MENU_MODEL (model));
  gtk_menu_attach_to_widget (GTK_MENU (menu), widget, NULL);

  g_signal_connect (widget, "button-press-event",
      G_CALLBACK (gtk_widget_button_press_cb), menu);
  g_signal_connect (widget, "popup-menu",
      G_CALLBACK (gtk_widget_popup_menu_cb), menu);
}

static void
ogmrip_profile_manager_dialog_profile_desc_changed (GtkTextBuffer *buffer, GtkWidget *dialog)
{
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
      GTK_RESPONSE_OK, gtk_text_buffer_get_char_count (buffer) > 0);
}

static void
ogmrip_profile_manager_dialog_add_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipProfileManagerDialog *dialog = data;
  GtkWidget *button;

  gtk_window_set_title (GTK_WINDOW (dialog->priv->name_dialog), _("New profile"));

  button = gtk_dialog_get_widget_for_response (GTK_DIALOG (dialog->priv->name_dialog), GTK_RESPONSE_OK);
  gtk_button_set_label (GTK_BUTTON (button), _("Create"));

  if (gtk_dialog_run (GTK_DIALOG (dialog->priv->name_dialog)) == GTK_RESPONSE_OK)
  {
    OGMRipProfile *profile;
    GtkTextIter start, end;
    const gchar *name;
    gchar *desc;

    profile = ogmrip_profile_new (NULL);
    ogmrip_profile_engine_add (dialog->priv->engine, profile);
    g_object_unref (profile);

    name = gtk_entry_get_text (GTK_ENTRY (dialog->priv->name_entry));
    g_settings_set_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME, name);

    gtk_text_buffer_get_bounds (dialog->priv->desc_buffer, &start, &end);
    desc = gtk_text_buffer_get_text (dialog->priv->desc_buffer, &start, &end, TRUE);
    g_settings_set_string (G_SETTINGS (profile), OGMRIP_PROFILE_DESCRIPTION, desc);
    g_free (desc);
  }

  gtk_widget_hide (dialog->priv->name_dialog);
}

static void
ogmrip_profile_manager_dialog_remove_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipProfileManagerDialog *parent = data;
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
ogmrip_profile_manager_dialog_edit_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipProfileManagerDialog *dialog = data;
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (dialog->priv->selection, &model, &iter))
  {
    OGMRipProfile *profile;
    GtkWidget *editor;

    profile = ogmrip_profile_store_get_profile (OGMRIP_PROFILE_STORE (model), &iter);

    editor = ogmrip_profile_editor_dialog_new (profile);
    gtk_window_set_transient_for (GTK_WINDOW (editor), GTK_WINDOW (dialog));
    gtk_window_set_destroy_with_parent (GTK_WINDOW (editor), FALSE);

    gtk_dialog_run (GTK_DIALOG (editor));

    gtk_widget_destroy (editor);
  }
}

static void
ogmrip_profile_manager_dialog_copy_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipProfileManagerDialog *dialog = data;
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
    ogmrip_profile_engine_add (dialog->priv->engine, new_profile);
    g_object_unref (new_profile);

    g_settings_set_string (G_SETTINGS (new_profile), OGMRIP_PROFILE_NAME, new_name);
    g_free (new_name);
  }
}

static void
ogmrip_profile_manager_dialog_rename_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipProfileManagerDialog *dialog = data;
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (dialog->priv->selection, &model, &iter))
  {
    OGMRipProfile *profile;
    GtkWidget *button;
    gchar *old_name, *old_desc;

    gtk_window_set_title (GTK_WINDOW (dialog->priv->name_dialog), _("Rename profile"));

    button = gtk_dialog_get_widget_for_response (GTK_DIALOG (dialog->priv->name_dialog), GTK_RESPONSE_OK);
    gtk_button_set_label (GTK_BUTTON (button), _("Re_name"));
    gtk_button_set_use_underline (GTK_BUTTON (button), TRUE);

    profile = ogmrip_profile_store_get_profile (OGMRIP_PROFILE_STORE (model), &iter);

    old_name = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME);
    gtk_entry_set_text (GTK_ENTRY (dialog->priv->name_entry), old_name);

    old_desc = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_DESCRIPTION);
    gtk_text_buffer_set_text (dialog->priv->desc_buffer, old_desc, -1);

    if (gtk_dialog_run (GTK_DIALOG (dialog->priv->name_dialog)) == GTK_RESPONSE_OK)
    {
      GtkTextIter start, end;
      const gchar *new_name;
      gchar *new_desc;

      new_name = gtk_entry_get_text (GTK_ENTRY (dialog->priv->name_entry));

      if (new_name && !g_str_equal (new_name, old_name))
        g_settings_set_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME, new_name);

      gtk_text_buffer_get_bounds (dialog->priv->desc_buffer, &start, &end);
      new_desc = gtk_text_buffer_get_text (dialog->priv->desc_buffer, &start, &end, TRUE);

      if (new_desc && !g_str_equal (new_desc, old_desc))
        g_settings_set_string (G_SETTINGS (profile), OGMRIP_PROFILE_DESCRIPTION, new_desc);

      g_free (new_desc);
    }

    g_free (old_name);
    g_free (old_desc);

    gtk_widget_hide (dialog->priv->name_dialog);
  }
}

static void
ogmrip_profile_manager_dialog_reset_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipProfileManagerDialog *dialog = data;
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (dialog->priv->selection, &model, &iter))
    ogmrip_profile_reset (ogmrip_profile_store_get_profile (OGMRIP_PROFILE_STORE (model), &iter));
}

static void
ogmrip_profile_manager_dialog_import_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipProfileManagerDialog *parent = data;
  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new (_("Select profile to import"),
      GTK_WINDOW (parent), GTK_FILE_CHOOSER_ACTION_OPEN,
      _("_Cancel"), GTK_RESPONSE_CANCEL,
      _("_Import"), GTK_RESPONSE_OK,
      NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
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
ogmrip_profile_manager_dialog_export_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipProfileManagerDialog *parent = data;
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
          _("_Cancel"), GTK_RESPONSE_CANCEL,
          _("_Export"), GTK_RESPONSE_OK,
          NULL);
      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

      if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
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
ogmrip_encoding_manager_dialog_selection_changed_cb (OGMRipProfileManagerDialog *dialog)
{
  g_simple_action_set_enabled (G_SIMPLE_ACTION (dialog->priv->remove_action),
      gtk_tree_selection_count_selected_rows (dialog->priv->selection) > 0);
}

static void
ogmrip_profile_manager_dialog_row_activated (OGMRipProfileManagerDialog *dialog)
{
  ogmrip_profile_manager_dialog_edit_activated (NULL, NULL, dialog);
}

G_DEFINE_TYPE_WITH_PRIVATE (OGMRipProfileManagerDialog, ogmrip_profile_manager_dialog, GTK_TYPE_DIALOG)

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

static void
ogmrip_profile_manager_dialog_class_init (OGMRipProfileManagerDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_profile_manager_dialog_dispose;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), OGMRIP_UI_RES);

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProfileManagerDialog, toolbar);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProfileManagerDialog, profile_swin);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProfileManagerDialog, add_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProfileManagerDialog, remove_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProfileManagerDialog, import_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProfileManagerDialog, name_dialog);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProfileManagerDialog, name_entry);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProfileManagerDialog, desc_buffer);
}

static GActionEntry entries[] =
{
  { "add",    ogmrip_profile_manager_dialog_add_activated,    NULL, NULL, NULL },
  { "remove", ogmrip_profile_manager_dialog_remove_activated, NULL, NULL, NULL },
  { "edit",   ogmrip_profile_manager_dialog_edit_activated,   NULL, NULL, NULL },
  { "copy",   ogmrip_profile_manager_dialog_copy_activated,   NULL, NULL, NULL },
  { "import", ogmrip_profile_manager_dialog_import_activated, NULL, NULL, NULL },
  { "export", ogmrip_profile_manager_dialog_export_activated, NULL, NULL, NULL },
  { "rename", ogmrip_profile_manager_dialog_rename_activated, NULL, NULL, NULL },
  { "reset",  ogmrip_profile_manager_dialog_reset_activated,  NULL, NULL, NULL }
};

static void
ogmrip_profile_manager_dialog_init (OGMRipProfileManagerDialog *dialog)
{
  GtkStyleContext *context;
  GSimpleActionGroup *group;
  GtkBuilder *builder;
  GObject *model;

  gtk_widget_init_template (GTK_WIDGET (dialog));

  dialog->priv = ogmrip_profile_manager_dialog_get_instance_private (dialog);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog->priv->name_dialog), GTK_RESPONSE_OK);

  dialog->priv->engine = ogmrip_profile_engine_get_default ();
  g_object_ref (dialog->priv->engine);

  context = gtk_widget_get_style_context (dialog->priv->profile_swin);
  gtk_style_context_set_junction_sides (context, GTK_JUNCTION_BOTTOM);

  context = gtk_widget_get_style_context (dialog->priv->toolbar);
  gtk_style_context_set_junction_sides (context, GTK_JUNCTION_TOP);
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_INLINE_TOOLBAR);

  dialog->priv->profile_view = ogmrip_profile_view_new ();
  gtk_container_add (GTK_CONTAINER (dialog->priv->profile_swin), dialog->priv->profile_view);
  gtk_widget_show (dialog->priv->profile_view);

  dialog->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->profile_view));

  g_signal_connect_swapped (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_encoding_manager_dialog_selection_changed_cb), dialog);

  g_signal_connect_swapped (dialog->priv->profile_view, "row-activated",
      G_CALLBACK (ogmrip_profile_manager_dialog_row_activated), dialog);

  group = g_simple_action_group_new ();
  gtk_widget_insert_action_group (GTK_WIDGET (dialog), "profile", G_ACTION_GROUP (group));
  g_object_unref (group);

  g_action_map_add_action_entries (G_ACTION_MAP (group),
      entries, G_N_ELEMENTS (entries), dialog);

  dialog->priv->remove_action = g_action_map_lookup_action (G_ACTION_MAP (group), "remove");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (dialog->priv->remove_action), FALSE);

  builder = gtk_builder_new_from_resource (OGMRIP_MENU_RES);

  model = gtk_builder_get_object (builder, "menu");
  gtk_widget_set_popup_menu (dialog->priv->profile_view, G_MENU_MODEL (model));

  g_object_unref (builder);

  gtk_actionable_set_action_name (GTK_ACTIONABLE (dialog->priv->add_button),    "profile.add");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (dialog->priv->remove_button), "profile.remove");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (dialog->priv->import_button), "profile.import");

  g_signal_connect (dialog->priv->desc_buffer, "changed",
      G_CALLBACK (ogmrip_profile_manager_dialog_profile_desc_changed), dialog->priv->name_dialog);
}

GtkWidget *
ogmrip_profile_manager_dialog_new (void)
{
  return g_object_new (OGMRIP_TYPE_PROFILE_MANAGER_DIALOG, NULL);
}

