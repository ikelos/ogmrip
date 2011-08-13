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

#include "ogmrip-encoding-manager-dialog.h"
#include "ogmrip-options-dialog.h"

#include "ogmrip-settings.h"

#include <glib/gi18n.h>

#define OGMRIP_GLADE_FILE "ogmrip" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-encoding_manager.glade"
#define OGMRIP_GLADE_ROOT "root"

#define OGMRIP_ENCODING_MANAGER_DIALOG_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_ENCODING_MANAGER_DIALOG, OGMRipEncodingManagerDialogPriv))

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

static void ogmrip_encoding_manager_dialog_dispose (GObject *gobject);

static const GtkActionEntry action_entries[] =
{
  { "Clear",  GTK_STOCK_CLEAR,  NULL,          NULL, NULL, NULL },
  { "Remove", GTK_STOCK_REMOVE, NULL,          NULL, NULL, NULL },
  { "Import", GTK_STOCK_OPEN,   N_("_Import"), NULL, NULL, NULL },
  { "Export", GTK_STOCK_SAVE,   N_("_Export"), NULL, NULL, NULL }
};

static const gchar *ui_description =
"<ui>"
"  <popup name='Popup'>"
"    <menuitem action='Import'/>"
"    <menuitem action='Export'/>"
"    <separator/>"
"    <menuitem action='Remove'/>"
"    <menuitem action='Clear'/>"
"  </popup>"
"</ui>";

G_DEFINE_TYPE (OGMRipEncodingManagerDialog, ogmrip_encoding_manager_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_encoding_manager_dialog_class_init (OGMRipEncodingManagerDialogClass *klass)
{
  GObjectClass *gobject_class;
  
  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_encoding_manager_dialog_dispose;

  g_type_class_add_private (klass, sizeof (OGMRipEncodingManagerDialogPriv));
}

static void
ogmrip_encoding_manager_dialog_init (OGMRipEncodingManagerDialog *dialog)
{
  GError *error = NULL;

  GtkWidget *area, *widget;
  GtkBuilder *builder;

  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  dialog->priv = OGMRIP_ENCODING_MANAGER_DIALOG_GET_PRIVATE (dialog);

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

  gtk_window_set_title (GTK_WINDOW (dialog), _("Encoding EncodingManager"));
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

  dialog->priv->remove_action = gtk_action_group_get_action (action_group, "Remove");
  gtk_action_set_sensitive (dialog->priv->remove_action, FALSE);

  dialog->priv->import_action = gtk_action_group_get_action (action_group, "Import");
  gtk_action_set_visible (dialog->priv->import_action, FALSE);

  dialog->priv->export_action = gtk_action_group_get_action (action_group, "Export");
  gtk_action_set_sensitive (dialog->priv->export_action, FALSE);
  gtk_action_set_visible (dialog->priv->export_action, FALSE);

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  widget = gtk_builder_get_widget (builder, OGMRIP_GLADE_ROOT);
  gtk_box_pack_start (GTK_BOX (area), widget, TRUE, TRUE, 0);
  gtk_widget_show (widget);

  dialog->priv->treeview = gtk_builder_get_widget (builder, "treeview");

  dialog->priv->store = gtk_list_store_new (COL_LAST, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT,
      G_TYPE_STRING, OGMRIP_TYPE_ENCODING, G_TYPE_BOOLEAN);
  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->priv->treeview), GTK_TREE_MODEL (dialog->priv->store));

  dialog->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->treeview));

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

  dialog->priv->bottom_button = gtk_builder_get_widget (builder, "bottom-button");
  gtk_widget_set_sensitive (dialog->priv->bottom_button, FALSE);

  dialog->priv->up_button = gtk_builder_get_widget (builder, "up-button");
  gtk_widget_set_sensitive (dialog->priv->up_button, FALSE);

  dialog->priv->down_button = gtk_builder_get_widget (builder, "down-button");
  gtk_widget_set_sensitive (dialog->priv->down_button, FALSE);

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

  G_OBJECT_CLASS (ogmrip_encoding_manager_dialog_parent_class)->dispose (gobject);
}

GtkWidget *
ogmrip_encoding_manager_dialog_new (void)
{
  return g_object_new (OGMRIP_TYPE_ENCODING_MANAGER_DIALOG, NULL);
}

