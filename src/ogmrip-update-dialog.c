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

#include "ogmrip-update-dialog.h"

#include "ogmrip-helper.h"
#include "ogmrip-settings.h"

#include <glib/gi18n.h>

#define OGMRIP_GLADE_FILE "ogmrip" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-update.glade"
#define OGMRIP_GLADE_ROOT "root"

#define OGMRIP_UPDATE_DIALOG_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_UPDATE_DIALOG, OGMRipUpdateDialogPriv))

struct _OGMRipUpdateDialogPriv
{
  GtkListStore *list;
};

enum
{
  COL_UPDATE,
  COL_NAME,
  COL_PROFILE,
  COL_VERSION,
  COL_LAST
};

static void
ogmrip_update_dialog_select_all_clicked (GtkTreeModel *model)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    do
    {
      gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_UPDATE, TRUE, -1);
    }
    while (gtk_tree_model_iter_next (model, &iter));
  }
}

static void
ogmrip_update_dialog_deselect_all_clicked (GtkTreeModel *model)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    do
    {
      gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_UPDATE, FALSE, -1);
    }
    while (gtk_tree_model_iter_next (model, &iter));
  }
}

static void
ogmrip_update_dialog_cell_toggled (GtkTreeModel *model, gchar *path)
{
  GtkTreeIter iter;
  gboolean update;

  gtk_tree_model_get_iter_from_string (model, &iter, path);

  gtk_tree_model_get (model, &iter, COL_UPDATE, &update, -1);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_UPDATE, update ? FALSE : TRUE, -1);
}

G_DEFINE_TYPE (OGMRipUpdateDialog, ogmrip_update_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_update_dialog_class_init (OGMRipUpdateDialogClass *klass)
{
  g_type_class_add_private (klass, sizeof (OGMRipUpdateDialogPriv));
}

static void
ogmrip_update_dialog_init (OGMRipUpdateDialog *dialog)
{
  GError *error = NULL;

  GtkWidget *area, *widget;
  GtkBuilder *builder;

  GtkCellRenderer *cell;
  GtkTreeViewColumn *column;

  dialog->priv = OGMRIP_UPDATE_DIALOG_GET_PRIVATE (dialog);

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_GLADE_FILE, &error))
    g_error ("Couldn't load builder file: %s", error->message);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
      GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
      GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
      NULL);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 450, 350);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Update profiles"));

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  widget = gtk_builder_get_widget (builder, OGMRIP_GLADE_ROOT);
  gtk_container_add (GTK_CONTAINER (area), widget);
  gtk_widget_show (widget);

  widget = gtk_builder_get_widget (builder, "treeview");

  dialog->priv->list = gtk_list_store_new (COL_LAST, G_TYPE_BOOLEAN, G_TYPE_STRING, OGMRIP_TYPE_PROFILE, G_TYPE_STRING);
  gtk_tree_view_set_model (GTK_TREE_VIEW (widget), GTK_TREE_MODEL (dialog->priv->list));
  g_object_unref (dialog->priv->list);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  cell = gtk_cell_renderer_toggle_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell, "active", COL_UPDATE, NULL);
  g_object_set (cell, "activatable", TRUE, NULL);

  g_signal_connect_swapped (cell, "toggled", G_CALLBACK (ogmrip_update_dialog_cell_toggled), dialog->priv->list);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell, "markup", COL_NAME, NULL);

  widget = gtk_builder_get_widget (builder, "select-all-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_update_dialog_select_all_clicked), dialog->priv->list);

  widget = gtk_builder_get_widget (builder, "deselect-all-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_update_dialog_deselect_all_clicked), dialog->priv->list);

  g_object_unref (builder);
}

GtkWidget *
ogmrip_update_dialog_new (void)
{
  return g_object_new (OGMRIP_TYPE_UPDATE_DIALOG, NULL);
}

void
ogmrip_update_dialog_add_profile (OGMRipUpdateDialog *dialog, OGMRipProfile *profile)
{
  GtkTreeIter iter;
  gchar *name, *version;

  g_return_if_fail (OGMRIP_IS_UPDATE_DIALOG (dialog));

  name = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME);
  version = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_VERSION);

  gtk_list_store_append (dialog->priv->list, &iter);
  gtk_list_store_set (dialog->priv->list, &iter, COL_UPDATE, TRUE,
      COL_NAME, name, COL_PROFILE, profile, COL_VERSION, version, -1);

  g_free (version);
  g_free (name);
}

GList *
ogmrip_update_dialog_get_profiles (OGMRipUpdateDialog *dialog)
{
  GList *list = NULL;

  GtkTreeIter iter;
  OGMRipProfile *profile;
  gboolean update;
  gchar *version;

  g_return_val_if_fail (OGMRIP_IS_UPDATE_DIALOG (dialog), NULL);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->priv->list), &iter))
  {
    do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->list), &iter, COL_UPDATE, &update, COL_PROFILE, &profile, -1);
      if (update)
        version = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_VERSION);
      else
        gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->list), &iter, COL_VERSION, &version, -1);

      if (version)
        list = g_list_prepend (list, g_object_ref (profile));

      g_free (version);
      g_object_unref (profile);
    }
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->priv->list), &iter));
  }

  return list;
}

