/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-encoding-manager-dialog.h"
#include "ogmrip-profile-keys.h"
#include "ogmrip-error-dialog.h"

#include <glib/gi18n-lib.h>

#define OGMRIP_GLADE_FILE "ogmrip" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-queue.glade"
#define OGMRIP_GLADE_ROOT "root"

#define gtk_builder_get_widget(builder, name) \
    (GtkWidget *) gtk_builder_get_object ((builder), (name))

enum
{
  PROP_0,
  PROP_MANAGER
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

struct _OGMRipEncodingManagerDialogPriv
{
  OGMRipEncodingManager *manager;

  GtkListStore *store;
  GtkTreeSelection *selection;
};

static void ogmrip_encoding_manager_dialog_dispose      (GObject      *gobject);
static void ogmrip_encoding_manager_dialog_get_property (GObject      *gobject,
                                                         guint        property_id,
                                                         GValue       *value,
                                                         GParamSpec   *pspec);
static void ogmrip_encoding_manager_dialog_set_property (GObject      *gobject,
                                                         guint        property_id,
                                                         const GValue *value,
                                                         GParamSpec   *pspec);

static gboolean
ogmrip_encoding_manager_dialog_get_iter (OGMRipEncodingManagerDialog *dialog, OGMRipEncoding *encoding, GtkTreeIter *iter)
{
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->priv->store), iter))
  {
    OGMRipEncoding *e;

    do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->store), iter, COL_ENCODING, &e, -1);
      g_object_unref (e);

      if (e == encoding)
        return TRUE;
    }
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->priv->store), iter));
  }

  return FALSE;
}

static void
ogmrip_encoding_manager_dialog_add_encoding (OGMRipEncodingManagerDialog *dialog, OGMRipEncoding *encoding)
{
  OGMRipProfile *profile;
  GtkTreeIter iter;
  gchar *str;

  profile = ogmrip_encoding_get_profile (encoding);
  str = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME);

  gtk_list_store_append (dialog->priv->store, &iter);
  gtk_list_store_set (dialog->priv->store, &iter,
      COL_NAME, ogmrip_container_get_label (ogmrip_encoding_get_container (encoding)),
      COL_TITLE, ogmrip_title_get_nr (ogmrip_encoding_get_title (encoding)) + 1,
      COL_PROFILE, str, COL_ENCODING, encoding, -1);

  g_free (str);
}

static void
ogmrip_encoding_manager_dialog_remove_encoding (OGMRipEncodingManagerDialog *dialog, OGMRipEncoding *encoding)
{
  GtkTreeIter iter;

  if (ogmrip_encoding_manager_dialog_get_iter (dialog, encoding, &iter))
    gtk_list_store_remove (dialog->priv->store, &iter);
}

static void
ogmrip_encoding_manager_dialog_move_encoding (OGMRipEncodingManagerDialog *dialog, OGMRipEncoding *encoding, OGMRipDirection direction)
{
  GtkTreeIter iter;

  if (ogmrip_encoding_manager_dialog_get_iter (dialog, encoding, &iter))
  {
    GtkTreeIter position = iter;

    switch (direction)
    {
      case OGMRIP_DIRECTION_TOP:
        gtk_list_store_move_after (dialog->priv->store, &iter, NULL);
        break;
      case OGMRIP_DIRECTION_UP:
        if (gtk_tree_model_iter_previous (GTK_TREE_MODEL (dialog->priv->store), &position))
          gtk_list_store_move_before (dialog->priv->store, &iter, &position);
        break;
      case OGMRIP_DIRECTION_DOWN:
        if (gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->priv->store), &position))
          gtk_list_store_move_after (dialog->priv->store, &iter, &position);
        break;
      case OGMRIP_DIRECTION_BOTTOM:
        gtk_list_store_move_before (dialog->priv->store, &iter, NULL);
        break;
    }
  }
}

static void
ogmrip_encoding_manager_dialog_set_manager (OGMRipEncodingManagerDialog *dialog, OGMRipEncodingManager *manager)
{
  GList *list, *link;

  dialog->priv->manager = g_object_ref (manager);

  list = ogmrip_encoding_manager_get_list (manager);
  for (link = list; link; link = link->next)
    ogmrip_encoding_manager_dialog_add_encoding (dialog, link->data);
  g_list_free (list);

  g_signal_connect_swapped (manager, "add",
      G_CALLBACK (ogmrip_encoding_manager_dialog_add_encoding), dialog);
  g_signal_connect_swapped (manager, "remove",
      G_CALLBACK (ogmrip_encoding_manager_dialog_remove_encoding), dialog);
  g_signal_connect_swapped (manager, "move",
      G_CALLBACK (ogmrip_encoding_manager_dialog_move_encoding), dialog);
}

static void
ogmrip_encoding_manager_dialog_set_top_button_sensitivity (GtkTreeSelection *selection, GtkWidget *button)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  gtk_widget_set_sensitive (button,
      gtk_tree_selection_get_selected (selection, &model, &iter) &&
      gtk_tree_model_iter_previous (model, &iter));
}

static void
ogmrip_encoding_manager_dialog_set_down_button_sensitivity (GtkTreeSelection *selection, GtkWidget *button)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  gtk_widget_set_sensitive (button,
      gtk_tree_selection_get_selected (selection, &model, &iter) &&
      gtk_tree_model_iter_next (model, &iter));
}

static void
ogmrip_encoding_manager_dialog_set_clear_button_sensitivity (GtkTreeSelection *selection, GtkWidget *button)
{
  GtkTreeModel *model;

  gtk_tree_selection_get_selected (selection, &model, NULL);
  gtk_widget_set_sensitive (button, gtk_tree_model_iter_n_children (model, NULL) > 0);
}

static void
ogmrip_encoding_manager_dialog_set_button_sensitivity (GtkTreeSelection *selection, GtkWidget *button)
{
  gtk_widget_set_sensitive (button, gtk_tree_selection_get_selected (selection, NULL, NULL));
}

static OGMRipEncoding *
ogmrip_encoding_manager_dialog_get_active (OGMRipEncodingManagerDialog *dialog)
{
  OGMRipEncoding *encoding;
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (!gtk_tree_selection_get_selected (dialog->priv->selection, &model, &iter))
    return NULL;

  gtk_tree_model_get (model, &iter, COL_ENCODING, &encoding, -1);
  g_object_unref (encoding);

  return encoding;
}

static void
ogmrip_encoding_manager_dialog_clear_clicked (OGMRipEncodingManagerDialog *dialog)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->priv->store), &iter))
  {
    OGMRipEncoding *encoding;
    gboolean strike, valid;

    g_signal_handlers_block_by_func (dialog->priv->manager,
        ogmrip_encoding_manager_dialog_remove_encoding, dialog);

    do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->store), &iter,
          COL_ENCODING, &encoding, COL_STRIKE, &strike, -1);
      g_object_unref (encoding);

      if (!strike)
        valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->priv->store), &iter);
      else
      {
        valid = gtk_list_store_remove (dialog->priv->store, &iter);
        ogmrip_encoding_manager_remove (dialog->priv->manager, encoding);
      }
    }
    while (valid);

    g_signal_handlers_unblock_by_func (dialog->priv->manager,
        ogmrip_encoding_manager_dialog_remove_encoding, dialog);
  }
}

static void
ogmrip_encoding_manager_dialog_remove_clicked (OGMRipEncodingManagerDialog *dialog)
{
  OGMRipEncoding *encoding;

  encoding = ogmrip_encoding_manager_dialog_get_active (dialog);
  if (encoding)
    ogmrip_encoding_manager_remove (dialog->priv->manager, encoding);
}
/*
static void
ogmrip_encoding_manager_dialog_import_activated (OGMRipEncodingManagerDialog *parent)
{
  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new (_("Import Encoding"),
      GTK_WINDOW (parent), GTK_FILE_CHOOSER_ACTION_OPEN,
      GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
      NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    GError *error = NULL;
    GFile *file;

    OGMRipEncoding *encoding;

    file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));

    encoding = ogmrip_encoding_new_from_file (file, &error);
    if (encoding)
    {
      ogmrip_encoding_manager_add (parent->priv->manager, encoding);
      g_object_unref (encoding);
    }
    else
    {
      ogmrip_run_error_dialog (GTK_WINDOW (dialog), error, _("Could not export the encoding"));
      g_clear_error (&error);
    }
    g_object_unref (file);
  }
  gtk_widget_destroy (dialog);
}

static void
ogmrip_encoding_manager_dialog_export_activated (OGMRipEncodingManagerDialog *parent)
{
  OGMRipEncoding *encoding;
  
  encoding = ogmrip_encoding_manager_dialog_get_active (parent);
  if (encoding)
  {
    GtkWidget *dialog;

    dialog = gtk_file_chooser_dialog_new (_("Export Encoding"),
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
      if (!ogmrip_encoding_export (encoding, file, &error))
      {
        ogmrip_run_error_dialog (GTK_WINDOW (dialog), error, _("Could not export the encoding"));
        g_clear_error (&error);
      }
      g_object_unref (file);
    }
    gtk_widget_destroy (dialog);
  }
}
*/
static void
ogmrip_encoding_manager_dialog_top_button_clicked (OGMRipEncodingManagerDialog *dialog)
{
  OGMRipEncoding *encoding;

  encoding = ogmrip_encoding_manager_dialog_get_active (dialog);
  if (encoding)
    ogmrip_encoding_manager_move (dialog->priv->manager, encoding, OGMRIP_DIRECTION_TOP);
}

static void
ogmrip_encoding_manager_dialog_up_button_clicked (OGMRipEncodingManagerDialog *dialog)
{
  OGMRipEncoding *encoding;

  encoding = ogmrip_encoding_manager_dialog_get_active (dialog);
  if (encoding)
    ogmrip_encoding_manager_move (dialog->priv->manager, encoding, OGMRIP_DIRECTION_UP);
}

static void
ogmrip_encoding_manager_dialog_down_button_clicked (OGMRipEncodingManagerDialog *dialog)
{
  OGMRipEncoding *encoding;

  encoding = ogmrip_encoding_manager_dialog_get_active (dialog);
  if (encoding)
    ogmrip_encoding_manager_move (dialog->priv->manager, encoding, OGMRIP_DIRECTION_DOWN);
}

static void
ogmrip_encoding_manager_dialog_bottom_button_clicked (OGMRipEncodingManagerDialog *dialog)
{
  OGMRipEncoding *encoding;

  encoding = ogmrip_encoding_manager_dialog_get_active (dialog);
  if (encoding)
    ogmrip_encoding_manager_move (dialog->priv->manager, encoding, OGMRIP_DIRECTION_BOTTOM);
}

G_DEFINE_TYPE (OGMRipEncodingManagerDialog, ogmrip_encoding_manager_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_encoding_manager_dialog_class_init (OGMRipEncodingManagerDialogClass *klass)
{
  GObjectClass *gobject_class;
  
  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_encoding_manager_dialog_dispose;
  gobject_class->get_property = ogmrip_encoding_manager_dialog_get_property;
  gobject_class->set_property = ogmrip_encoding_manager_dialog_set_property;

  g_object_class_install_property (gobject_class, PROP_MANAGER,
      g_param_spec_object ("manager", "Encoding manager property", "Set encoding manager",
        OGMRIP_TYPE_ENCODING_MANAGER, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipEncodingManagerDialogPriv));
}

static void
ogmrip_encoding_manager_dialog_init (OGMRipEncodingManagerDialog *dialog)
{
  GError *error = NULL;

  GtkWidget *area, *widget;
  GtkBuilder *builder;

  GtkStyleContext *context;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog,
      OGMRIP_TYPE_ENCODING_MANAGER_DIALOG, OGMRipEncodingManagerDialogPriv);

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

  gtk_window_set_title (GTK_WINDOW (dialog), _("Encoding Manager"));
  gtk_window_set_default_size (GTK_WINDOW (dialog), 500, 300);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT, FALSE);

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  widget = gtk_builder_get_widget (builder, OGMRIP_GLADE_ROOT);
  gtk_container_add (GTK_CONTAINER (area), widget);
  gtk_widget_show (widget);

  widget = gtk_builder_get_widget (builder, "swin");
  context = gtk_widget_get_style_context (widget);
  gtk_style_context_set_junction_sides (context, GTK_JUNCTION_BOTTOM);

  widget = gtk_builder_get_widget (builder, "toolbar");
  context = gtk_widget_get_style_context (widget);
  gtk_style_context_set_junction_sides (context, GTK_JUNCTION_TOP);
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_INLINE_TOOLBAR);

  widget = gtk_builder_get_widget (builder, "treeview");

  dialog->priv->store = gtk_list_store_new (COL_LAST, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT,
      G_TYPE_STRING, OGMRIP_TYPE_ENCODING, G_TYPE_BOOLEAN);
  gtk_tree_view_set_model (GTK_TREE_VIEW (widget), GTK_TREE_MODEL (dialog->priv->store));

  dialog->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Run"), renderer, "stock-id", COL_PIXBUF, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);
  g_object_set (renderer, "stock-size", GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Name"), renderer, "text", COL_NAME, "strikethrough", COL_STRIKE, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Title"), renderer, "text", COL_TITLE, "strikethrough", COL_STRIKE, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Profile"), renderer, "markup", COL_PROFILE, "strikethrough", COL_STRIKE, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  widget = gtk_builder_get_widget (builder, "clear-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_encoding_manager_dialog_clear_clicked), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_encoding_manager_dialog_set_clear_button_sensitivity), widget);

  widget = gtk_builder_get_widget (builder, "remove-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_encoding_manager_dialog_remove_clicked), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_encoding_manager_dialog_set_button_sensitivity), widget);
/*
  action = gtk_action_group_get_action (action_group, "Import");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_encoding_manager_dialog_import_activated), dialog);

  action = gtk_action_group_get_action (action_group, "Export");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_encoding_manager_dialog_export_activated), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_encoding_manager_dialog_set_action_sensitivity), action);
  gtk_action_set_sensitive (action, FALSE);
*/
  widget = gtk_builder_get_widget (builder, "top-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_encoding_manager_dialog_top_button_clicked), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_encoding_manager_dialog_set_top_button_sensitivity), widget);

  widget = gtk_builder_get_widget (builder, "bottom-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_encoding_manager_dialog_bottom_button_clicked), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_encoding_manager_dialog_set_top_button_sensitivity), widget);

  widget = gtk_builder_get_widget (builder, "up-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_encoding_manager_dialog_up_button_clicked), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_encoding_manager_dialog_set_down_button_sensitivity), widget);

  widget = gtk_builder_get_widget (builder, "down-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_encoding_manager_dialog_down_button_clicked), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_encoding_manager_dialog_set_down_button_sensitivity), widget);

  g_object_unref (builder);
}

static void
ogmrip_encoding_manager_dialog_dispose (GObject *gobject)
{
  OGMRipEncodingManagerDialog *dialog;

  dialog = OGMRIP_ENCODING_MANAGER_DIALOG (gobject);

  if (dialog->priv->store)
  {
    g_object_unref (dialog->priv->store);
    dialog->priv->store = NULL;
  }

  if (dialog->priv->manager)
  {
    g_signal_handlers_disconnect_by_func (dialog->priv->manager,
        ogmrip_encoding_manager_dialog_add_encoding, dialog);
    g_signal_handlers_disconnect_by_func (dialog->priv->manager,
        ogmrip_encoding_manager_dialog_remove_encoding, dialog);
    g_signal_handlers_disconnect_by_func (dialog->priv->manager,
        ogmrip_encoding_manager_dialog_move_encoding, dialog);
    g_object_unref (dialog->priv->manager);
    dialog->priv->manager = NULL;
  }

  G_OBJECT_CLASS (ogmrip_encoding_manager_dialog_parent_class)->dispose (gobject);
}

static void
ogmrip_encoding_manager_dialog_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_MANAGER:
      g_value_set_object (value, OGMRIP_ENCODING_MANAGER_DIALOG (gobject)->priv->manager);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_encoding_manager_dialog_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_MANAGER:
      ogmrip_encoding_manager_dialog_set_manager (OGMRIP_ENCODING_MANAGER_DIALOG (gobject), g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

GtkWidget *
ogmrip_encoding_manager_dialog_new (OGMRipEncodingManager *manager)
{
  return g_object_new (OGMRIP_TYPE_ENCODING_MANAGER_DIALOG, "manager", manager, NULL);
}

