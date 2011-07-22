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

#include "ogmrip-queue-dialog.h"
#include "ogmrip-options-dialog.h"

#include "ogmrip-settings.h"

#include <glib/gi18n.h>

#define OGMRIP_GLADE_FILE "ogmrip" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-queue.glade"
#define OGMRIP_GLADE_ROOT "root"

#define OGMRIP_QUEUE_DIALOG_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_QUEUE_DIALOG, OGMRipQueueDialogPriv))

enum
{
  ADD,
  REMOVE,
  IMPORT,
  EXPORT,
  LAST_SIGNAL
};

enum
{
  COL_PIXBUF,
  COL_NAME,
  COL_TITLE,
  COL_PROFILE,
  COL_ENCODING,
  COL_STRIKE,
  COL_LAST
};

struct _OGMRipQueueDialogPriv
{
  GtkWidget *treeview;
  GtkListStore *store;
  GtkTreeSelection *selection;

  GtkWidget *popup;
  GtkAction *clear_action;
  GtkAction *remove_action;
  GtkAction *import_action;
  GtkAction *export_action;

  GtkWidget *top_button;
  GtkWidget *bottom_button;
  GtkWidget *up_button;
  GtkWidget *down_button;
};

static void ogmrip_queue_dialog_dispose (GObject *gobject);
/*
extern OGMRipSettings *settings;
*/
static int signals[LAST_SIGNAL] = { 0 };

static gboolean
ogmrip_queue_dialog_find_encoding (OGMRipQueueDialog *dialog, OGMRipEncoding *encoding, GtkTreeIter *iter)
{
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->priv->store), iter))
  {
    OGMRipEncoding *encoding2;
    do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->store), iter, COL_ENCODING, &encoding2, -1);
      if (encoding2 && encoding2 == encoding)
        return TRUE;
    }
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->priv->store), iter));
  }

  return FALSE;
}

static void
ogmrip_queue_dialog_update (OGMRipQueueDialog *dialog, OGMRipEncoding *encoding, GtkTreeIter *iter)
{
  gboolean has_prev = FALSE, has_next = FALSE, can_remove = FALSE;

  can_remove = !OGMRIP_ENCODING_IS_RUNNING (encoding);

  if (!OGMRIP_ENCODING_IS_EXTRACTED (encoding))
  {
    GtkTreeIter next_iter = *iter, prev_iter = *iter;

    has_next = gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->priv->store), &next_iter);

    if (gtk_tree_model_iter_prev (GTK_TREE_MODEL (dialog->priv->store), &prev_iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->store), &prev_iter, COL_ENCODING, &encoding, -1);
      has_prev = !OGMRIP_ENCODING_IS_EXTRACTED (encoding);
    }
  }

  gtk_widget_set_sensitive (dialog->priv->up_button, has_prev);
  gtk_widget_set_sensitive (dialog->priv->top_button, has_prev);

  gtk_widget_set_sensitive (dialog->priv->down_button, has_next);
  gtk_widget_set_sensitive (dialog->priv->bottom_button, has_next);

  gtk_action_set_sensitive (dialog->priv->remove_action, can_remove);
}

static void
ogmrip_queue_dialog_encoding_run (OGMRipQueueDialog *dialog, OGMRipEncoding *encoding)
{
  GtkTreeIter iter;

  if (ogmrip_queue_dialog_find_encoding (dialog, encoding, &iter))
  {
    gtk_list_store_set (dialog->priv->store, &iter, COL_PIXBUF, GTK_STOCK_EXECUTE, -1);

    ogmrip_queue_dialog_update (dialog, encoding, &iter);
  }
}

static void
ogmrip_queue_dialog_encoding_completed (OGMRipQueueDialog *dialog, OGMJobResultType result, OGMRipEncoding *encoding)
{
  GtkTreeIter iter;

  if (ogmrip_queue_dialog_find_encoding (dialog, encoding, &iter))
  {
    gtk_list_store_set (dialog->priv->store, &iter, COL_PIXBUF, NULL, -1);

    ogmrip_queue_dialog_update (dialog, encoding, &iter);
  }
}

static void
ogmrip_queue_dialog_encoding_task_completed (OGMRipQueueDialog *dialog, OGMRipEncodingTask *task, OGMRipEncoding *encoding)
{
  GtkTreeIter iter;

  if (task->type == OGMRIP_TASK_MERGE && task->detail.result == OGMJOB_RESULT_SUCCESS &&
      ogmrip_queue_dialog_find_encoding (dialog, encoding, &iter))
    gtk_list_store_set (dialog->priv->store, &iter, COL_STRIKE, TRUE, -1);
}

static void
ogmrip_queue_dialog_remove_iter (OGMRipQueueDialog *dialog, GtkTreeIter *iter, OGMRipEncoding *encoding)
{
  if (!encoding)
    gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->store), iter, COL_ENCODING, &encoding, -1);

  gtk_list_store_remove (dialog->priv->store, iter);
  if (gtk_tree_selection_get_selected (dialog->priv->selection, NULL, NULL))
    g_signal_emit_by_name (dialog->priv->selection, "changed");
  else
  {
    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->priv->store), iter))
      gtk_tree_selection_select_iter (dialog->priv->selection, iter);
    else
      g_signal_emit_by_name (dialog->priv->selection, "changed");
  }

  g_signal_handlers_disconnect_by_func (encoding, ogmrip_queue_dialog_encoding_run, dialog);
  g_signal_handlers_disconnect_by_func (encoding, ogmrip_queue_dialog_encoding_completed, dialog);
  g_signal_handlers_disconnect_by_func (encoding, ogmrip_queue_dialog_encoding_task_completed, dialog);

  g_signal_emit (dialog, signals[REMOVE], 0, encoding);
}
/*
static void
ogmrip_queue_dialog_profile_changed (OGMRipQueueDialog *dialog, OGMRipOptionsDialog *options_dialog)
{
  GtkTreeRowReference *reference;
  GtkTreeIter iter;

  reference = g_object_get_data (G_OBJECT (options_dialog), "__row_reference__");
  if (reference && gtk_tree_row_reference_get_iter (reference, &iter))
  {
    OGMRipEncoding *encoding;
    const gchar *section;
    gchar *name;

    gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->store), &iter, COL_ENCODING, &encoding, -1);
    section = ogmrip_encoding_get_profile (encoding);

    ogmrip_settings_get (settings, section, OGMRIP_GCONF_PROFILE_NAME, &name, NULL);
    gtk_list_store_set (dialog->priv->store, &iter, COL_PROFILE, name, -1);
    g_free (name);
  }
}
*/
static void
ogmrip_queue_dialog_list_row_activated (OGMRipQueueDialog *parent, GtkTreePath *path, GtkTreeViewColumn *column)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter (GTK_TREE_MODEL (parent->priv->store), &iter, path))
  {
    OGMRipEncoding *encoding;

    gtk_tree_model_get (GTK_TREE_MODEL (parent->priv->store), &iter,
        COL_ENCODING, &encoding, -1);

    if (!OGMRIP_ENCODING_IS_EXTRACTED (encoding))
    {
      GtkWidget *dialog;
/*
      GType container;
      GSList *sections, *section;
      gint n_audio, n_subp;
      gchar *name;
*/
      dialog = ogmrip_options_dialog_new (encoding, OGMRIP_OPTIONS_DIALOG_EDIT);
      gtk_window_set_parent (GTK_WINDOW (dialog), GTK_WINDOW (parent));

      /*
       * TODO ajout des profils compatibles
       */
/*
      n_audio = ogmrip_encoding_get_n_audio_streams (encoding);
      n_subp = ogmrip_encoding_get_n_subp_streams (encoding);

      sections = ogmrip_settings_get_subsections (settings, OGMRIP_GCONF_PROFILES);
      for (section = sections; section; section = section->next)
      {
        if (ogmrip_settings_has_section (settings, section->data) &&
            ogmrip_profiles_check_profile (section->data, NULL))
        {
          ogmrip_settings_get (settings, section->data, OGMRIP_GCONF_CONTAINER_FORMAT, &name, NULL);
          container = ogmrip_gconf_get_container_type (section->data, name);
          g_free (name);

          if (ogmrip_plugin_get_container_max_audio (container) >= n_audio &&
              ogmrip_plugin_get_container_max_subp (container) >= n_subp)
          {
            ogmrip_settings_get (settings, section->data, "name", &name, NULL);
            ogmrip_options_dialog_add_profile (OGMRIP_OPTIONS_DIALOG (dialog), section->data, name);
            g_free (name);
          }
        }
        g_free (section->data);
      }
      g_slist_free (sections);

      g_signal_connect_swapped (dialog, "profile-changed", G_CALLBACK (ogmrip_queue_dialog_profile_changed), parent);
*/
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
    }
  }
}

gboolean
ogmrip_queue_dialog_list_button_pressed (OGMRipQueueDialog *dialog, GdkEventButton *event, GtkWidget *treeview)
{
  if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
  {
    gtk_menu_popup (GTK_MENU (dialog->priv->popup), NULL, NULL,
        NULL, NULL, event->button, gdk_event_get_time ((GdkEvent*) event));

    return TRUE;
  }

  return FALSE;
}

gboolean
ogmrip_queue_dialog_list_popup_menu (OGMRipQueueDialog *dialog, GtkWidget *treeview)
{
  gtk_menu_popup (GTK_MENU (dialog->priv->popup), NULL, NULL,
      NULL, NULL, 0, gdk_event_get_time (NULL));

  return TRUE;
}

static void
ogmrip_queue_dialog_list_selection_changed (OGMRipQueueDialog *dialog)
{
  OGMRipEncoding *encoding = NULL;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (dialog->priv->selection, NULL, &iter))
  {
    gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->store), &iter, COL_ENCODING, &encoding, -1);

    if (encoding)
      ogmrip_queue_dialog_update (dialog, encoding, &iter);
  }

  gtk_action_set_sensitive (dialog->priv->export_action, encoding != NULL);
}

static void
ogmrip_queue_dialog_top_button_clicked (OGMRipQueueDialog *dialog)
{
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (dialog->priv->selection, NULL, &iter))
  {
    gtk_list_store_move_after (dialog->priv->store, &iter, NULL);
    g_signal_emit_by_name (dialog->priv->selection, "changed");
  }
}

static void
ogmrip_queue_dialog_bottom_button_clicked (OGMRipQueueDialog *dialog)
{
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (dialog->priv->selection, NULL, &iter))
  {
    gtk_list_store_move_before (dialog->priv->store, &iter, NULL);
    g_signal_emit_by_name (dialog->priv->selection, "changed");
  }
}

static void
ogmrip_queue_dialog_up_button_clicked (OGMRipQueueDialog *dialog)
{
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (dialog->priv->selection, NULL, &iter))
  {
    GtkTreeIter position = iter;

    if (gtk_tree_model_iter_prev (GTK_TREE_MODEL (dialog->priv->store), &position))
    {
      gtk_list_store_move_before (dialog->priv->store, &iter, &position);
      g_signal_emit_by_name (dialog->priv->selection, "changed");
    }
  }
}

static void
ogmrip_queue_dialog_down_button_clicked (OGMRipQueueDialog *dialog)
{
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (dialog->priv->selection, NULL, &iter))
  {
    GtkTreeIter position = iter;

    if (gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->priv->store), &position))
    {
      gtk_list_store_move_after (dialog->priv->store, &iter, &position);
      g_signal_emit_by_name (dialog->priv->selection, "changed");
    }
  }
}

static void
ogmrip_queue_dialog_clear_action_activated (OGMRipQueueDialog *dialog)
{
  GtkTreeIter iter;
  OGMRipEncoding *encoding;

  while (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->priv->store), &iter))
  {
    gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->store), &iter, COL_ENCODING, &encoding, -1);

    if (!OGMRIP_ENCODING_IS_EXTRACTED (encoding))
      break;

    ogmrip_queue_dialog_remove_iter (dialog, &iter, encoding);
  }
}

static void
ogmrip_queue_dialog_remove_action_activated (OGMRipQueueDialog *dialog)
{
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (dialog->priv->selection, NULL, &iter))
    ogmrip_queue_dialog_remove_iter (dialog, &iter, NULL);
}

static void
ogmrip_queue_dialog_import_action_activated (OGMRipQueueDialog *parent)
{
  OGMRipEncoding *encoding = NULL;
  GtkWidget *dialog;
  gint response;

  GError *error = NULL;
  gchar *filename = NULL;

  dialog = gtk_file_chooser_dialog_new (_("Load encoding"),
      GTK_WINDOW (parent), GTK_FILE_CHOOSER_ACTION_OPEN,
      GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
      NULL);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response == GTK_RESPONSE_ACCEPT)
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
  gtk_widget_destroy (dialog);

  while (response == GTK_RESPONSE_ACCEPT)
  {
    encoding = ogmrip_encoding_new_from_file (filename, &error);
    if (encoding)
      break;

    if (!g_error_matches (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_ID))
      break;

    g_clear_error (&error);

    dialog = ogmrip_load_dvd_dialog_new (GTK_WINDOW (parent), NULL, NULL, FALSE);
    response = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  }

  if (filename)
    g_free (filename);

  if (encoding)
  {
    g_signal_emit (parent, signals[IMPORT], 0, encoding);

    ogmrip_queue_dialog_add_encoding (parent, encoding);
  }
  else
  {
    dialog = ogmrip_message_dialog_new (GTK_WINDOW (dialog), GTK_MESSAGE_ERROR, _("Cannot load encoding from '%s'"), filename);
    if (error)
    {
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
      g_error_free (error);
    }
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  }
}

static void
ogmrip_queue_dialog_export_action_activated (OGMRipQueueDialog *parent)
{
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (parent->priv->selection, NULL, &iter))
  {
    OGMRipEncoding *encoding;

    gtk_tree_model_get (GTK_TREE_MODEL (parent->priv->store), &iter, COL_ENCODING, &encoding, -1);

    if (encoding)
    {
      GtkWidget *dialog;

      dialog = gtk_file_chooser_dialog_new (_("Save encoding"),
          GTK_WINDOW (parent), GTK_FILE_CHOOSER_ACTION_SAVE,
          GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
          GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
          NULL);
      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

      if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
      {
        gchar *filename;

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        ogmrip_encoding_dump (encoding, filename);
        g_free (filename);

        g_signal_emit (parent, signals[EXPORT], 0, encoding);
      }
      gtk_widget_destroy (dialog);
    }
  }
}


G_DEFINE_TYPE (OGMRipQueueDialog, ogmrip_queue_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_queue_dialog_class_init (OGMRipQueueDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = ogmrip_queue_dialog_dispose;

  signals[ADD] = g_signal_new ("add-encoding", G_TYPE_FROM_CLASS (klass), 
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipQueueDialogClass, add_encoding), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, OGMRIP_TYPE_ENCODING);

  signals[REMOVE] = g_signal_new ("remove-encoding", G_TYPE_FROM_CLASS (klass), 
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipQueueDialogClass, remove_encoding), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, OGMRIP_TYPE_ENCODING);

  signals[IMPORT] = g_signal_new ("import-encoding", G_TYPE_FROM_CLASS (klass), 
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipQueueDialogClass, import_encoding), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, OGMRIP_TYPE_ENCODING);

  signals[EXPORT] = g_signal_new ("export-encoding", G_TYPE_FROM_CLASS (klass), 
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipQueueDialogClass, export_encoding), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, OGMRIP_TYPE_ENCODING);

  g_type_class_add_private (klass, sizeof (OGMRipQueueDialogPriv));
}

static void
ogmrip_queue_dialog_init (OGMRipQueueDialog *dialog)
{
  const GtkActionEntry action_entries[] =
  {
    { "Clear",  GTK_STOCK_CLEAR,  NULL,          NULL, NULL, NULL },
    { "Remove", GTK_STOCK_REMOVE, NULL,          NULL, NULL, NULL },
    { "Import", GTK_STOCK_OPEN,   N_("_Import"), NULL, NULL, NULL },
    { "Export", GTK_STOCK_SAVE,   N_("_Export"), NULL, NULL, NULL }
  };

  const gchar *ui_description =
  "<ui>"
  "  <popup name='Popup'>"
  "    <menuitem action='Import'/>"
  "    <menuitem action='Export'/>"
  "    <separator/>"
  "    <menuitem action='Remove'/>"
  "    <menuitem action='Clear'/>"
  "  </popup>"
  "</ui>";

  GError *error = NULL;

  GtkWidget *area, *widget;
  GtkBuilder *builder;

  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  dialog->priv = OGMRIP_QUEUE_DIALOG_GET_PRIVATE (dialog);

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_GLADE_FILE, &error))
  {
    g_warning ("Couldn't load builder file: %s", error->message);
    g_object_unref (builder);
    g_error_free (error);
    return;
  }

  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_EXECUTE, GTK_RESPONSE_ACCEPT);

  gtk_window_set_title (GTK_WINDOW (dialog), _("Encoding Queue"));
  gtk_window_set_default_size (GTK_WINDOW (dialog), 500, 300);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT, FALSE);
  gtk_window_set_icon_from_stock (GTK_WINDOW (dialog), GTK_STOCK_PROPERTIES);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, action_entries, G_N_ELEMENTS (action_entries), NULL);

  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL);

  dialog->priv->popup = gtk_ui_manager_get_widget (ui_manager, "/Popup");

  dialog->priv->clear_action = gtk_action_group_get_action (action_group, "Clear");
  gtk_action_set_sensitive (dialog->priv->clear_action, FALSE);
  g_signal_connect_swapped (dialog->priv->clear_action, "activate",
      G_CALLBACK (ogmrip_queue_dialog_clear_action_activated), dialog);

  dialog->priv->remove_action = gtk_action_group_get_action (action_group, "Remove");
  gtk_action_set_sensitive (dialog->priv->remove_action, FALSE);
  g_signal_connect_swapped (dialog->priv->remove_action, "activate",
      G_CALLBACK (ogmrip_queue_dialog_remove_action_activated), dialog);

  dialog->priv->import_action = gtk_action_group_get_action (action_group, "Import");
  g_signal_connect_swapped (dialog->priv->import_action, "activate",
      G_CALLBACK (ogmrip_queue_dialog_import_action_activated), dialog);
  gtk_action_set_visible (dialog->priv->import_action, FALSE);

  dialog->priv->export_action = gtk_action_group_get_action (action_group, "Export");
  gtk_action_set_sensitive (dialog->priv->export_action, FALSE);
  g_signal_connect_swapped (dialog->priv->export_action, "activate",
      G_CALLBACK (ogmrip_queue_dialog_export_action_activated), dialog);
  gtk_action_set_visible (dialog->priv->export_action, FALSE);

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  widget = gtk_builder_get_widget (builder, OGMRIP_GLADE_ROOT);
  gtk_box_pack_start (GTK_BOX (area), widget, TRUE, TRUE, 0);
  gtk_widget_show (widget);

  dialog->priv->treeview = gtk_builder_get_widget (builder, "treeview");
  g_signal_connect_swapped (dialog->priv->treeview, "row-activated",
      G_CALLBACK (ogmrip_queue_dialog_list_row_activated), dialog);
  g_signal_connect_swapped (dialog->priv->treeview, "button-press-event",
      G_CALLBACK (ogmrip_queue_dialog_list_button_pressed), dialog);
  g_signal_connect_swapped (dialog->priv->treeview, "popup-menu",
      G_CALLBACK (ogmrip_queue_dialog_list_popup_menu), dialog);

  dialog->priv->store = gtk_list_store_new (COL_LAST, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT,
      G_TYPE_STRING, OGMRIP_TYPE_ENCODING, G_TYPE_BOOLEAN);
  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->priv->treeview), GTK_TREE_MODEL (dialog->priv->store));

  dialog->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->treeview));
  g_signal_connect_swapped (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_queue_dialog_list_selection_changed), dialog);

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Run"), renderer, "stock-id", COL_PIXBUF, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->priv->treeview), column);
  g_object_set (renderer, "stock-size", GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Name"), renderer, "text", COL_NAME, "strikethrough", COL_STRIKE, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->priv->treeview), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Title"), renderer, "text", COL_TITLE, "strikethrough", COL_STRIKE, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->priv->treeview), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Profile"), renderer, "markup", COL_PROFILE, "strikethrough", COL_STRIKE, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->priv->treeview), column);

  dialog->priv->top_button = gtk_builder_get_widget (builder, "top-button");
  gtk_widget_set_sensitive (dialog->priv->top_button, FALSE);
  g_signal_connect_swapped (dialog->priv->top_button, "clicked",
      G_CALLBACK (ogmrip_queue_dialog_top_button_clicked), dialog);

  dialog->priv->bottom_button = gtk_builder_get_widget (builder, "bottom-button");
  gtk_widget_set_sensitive (dialog->priv->bottom_button, FALSE);
  g_signal_connect_swapped (dialog->priv->bottom_button, "clicked",
      G_CALLBACK (ogmrip_queue_dialog_bottom_button_clicked), dialog);

  dialog->priv->up_button = gtk_builder_get_widget (builder, "up-button");
  gtk_widget_set_sensitive (dialog->priv->up_button, FALSE);
  g_signal_connect_swapped (dialog->priv->up_button, "clicked",
      G_CALLBACK (ogmrip_queue_dialog_up_button_clicked), dialog);

  dialog->priv->down_button = gtk_builder_get_widget (builder, "down-button");
  gtk_widget_set_sensitive (dialog->priv->down_button, FALSE);
  g_signal_connect_swapped (dialog->priv->down_button, "clicked",
      G_CALLBACK (ogmrip_queue_dialog_down_button_clicked), dialog);

  g_object_unref (builder);
}

static void
ogmrip_queue_dialog_dispose (GObject *gobject)
{
  OGMRipQueueDialog *dialog;

  dialog = OGMRIP_QUEUE_DIALOG (gobject);

  if (dialog->priv->store)
  {
    g_object_unref (dialog->priv->store);
    dialog->priv->store = NULL;
  }

  G_OBJECT_CLASS (ogmrip_queue_dialog_parent_class)->dispose (gobject);
}

GtkWidget *
ogmrip_queue_dialog_new (void)
{
  return g_object_new (OGMRIP_TYPE_QUEUE_DIALOG, NULL);
}

static gboolean
ogmrip_queue_dialog_check_filename (OGMRipEncoding *encoding1, OGMRipEncoding *encoding2)
{
  return g_str_equal (ogmrip_encoding_get_filename (encoding1),
      ogmrip_encoding_get_filename (encoding2)) != TRUE;
}

void
ogmrip_queue_dialog_add_encoding (OGMRipQueueDialog *dialog, OGMRipEncoding *encoding)
{
  GtkTreeIter iter;

  g_return_if_fail (OGMRIP_IS_QUEUE_DIALOG (dialog));
  g_return_if_fail (encoding != NULL);

  if (ogmrip_queue_dialog_find_encoding (dialog, encoding, &iter))
    gtk_tree_selection_select_iter (dialog->priv->selection, &iter);
  else
  {
    OGMDvdTitle *title;
    OGMRipProfile *profile;
    gchar *name;

    if (!ogmrip_queue_dialog_foreach_encoding (dialog,
          (OGMRipEncodingFunc) ogmrip_queue_dialog_check_filename, encoding))
    {
      GtkWindow *transient;

      transient = gtk_window_get_transient_for (GTK_WINDOW (dialog));

      if (ogmrip_message_dialog (transient, GTK_MESSAGE_QUESTION, "<big><b>%s</b></big>\n\n%s",
            _("This encoding will have the same output file name as another one."),
            _("Do you want to enqueue it anyway ?")) != GTK_RESPONSE_YES)
        return;
    }

    if (g_file_test (ogmrip_encoding_get_filename (encoding), G_FILE_TEST_EXISTS))
    {
      GtkWindow *transient;

      transient = gtk_window_get_transient_for (GTK_WINDOW (dialog));

      if (ogmrip_message_dialog (transient, GTK_MESSAGE_QUESTION, "<big><b>%s</b></big>\n\n%s",
            _("A file with the same name as the output file of the encoding already exists."),
            _("Do you want to enqueue it anyway ?")) != GTK_RESPONSE_YES)
        return;
    }

    profile = ogmrip_encoding_get_profile (encoding);
    name = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME);

    title = ogmrip_encoding_get_title (encoding);

    gtk_list_store_append (dialog->priv->store, &iter);
    gtk_list_store_set (dialog->priv->store, &iter,
        COL_NAME,  ogmrip_encoding_get_label (encoding),
        COL_TITLE, ogmdvd_title_get_nr (title) + 1,
        COL_PROFILE, name,
        COL_ENCODING, encoding,
        -1);

    g_free (name);

    g_signal_connect_swapped (encoding, "run",
        G_CALLBACK (ogmrip_queue_dialog_encoding_run), dialog);
    g_signal_connect_swapped (encoding, "complete",
        G_CALLBACK (ogmrip_queue_dialog_encoding_completed), dialog);
    g_signal_connect_swapped (encoding, "task::complete",
        G_CALLBACK (ogmrip_queue_dialog_encoding_task_completed), dialog);

    gtk_tree_selection_select_iter (dialog->priv->selection, &iter);

    gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT, TRUE);

    g_signal_emit (dialog, signals[ADD], 0, encoding);
  }
}

void
ogmrip_queue_dialog_remove_encoding (OGMRipQueueDialog *dialog, OGMRipEncoding *encoding)
{
  GtkTreeIter iter;

  g_return_if_fail (OGMRIP_IS_QUEUE_DIALOG (dialog));
  g_return_if_fail (encoding != NULL);

  if (ogmrip_queue_dialog_find_encoding (dialog, encoding, &iter) && !OGMRIP_ENCODING_IS_RUNNING (encoding))
    ogmrip_queue_dialog_remove_iter (dialog, &iter, NULL);
}

gboolean
ogmrip_queue_dialog_foreach_encoding (OGMRipQueueDialog *dialog, OGMRipEncodingFunc func, gpointer data)
{
  GtkTreeIter iter;

  g_return_val_if_fail (OGMRIP_IS_QUEUE_DIALOG (dialog), FALSE);
  g_return_val_if_fail (func != NULL, FALSE);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->priv->store), &iter))
  {
    OGMRipEncoding *encoding;
    do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->store), &iter, COL_ENCODING, &encoding, -1);
      if ((* func) (encoding, data) == FALSE)
        return FALSE;
    }
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->priv->store), &iter));
  }

  return TRUE;
}

