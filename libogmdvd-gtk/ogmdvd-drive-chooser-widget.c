/* OGMDvd - A wrapper library around libdvdread
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
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

/**
 * SECTION:ogmdvd-drive-chooser-widget
 * @title: OGMDvdDriveChooserWidget
 * @include: ogmdvd-drive-chooser-widget.h
 * @short_description: DVD drive chooser widget that can be embedded in other widgets
 */

#include "ogmdvd-drive-chooser-widget.h"

#include <ogmdvd.h>
#include <glib/gi18n.h>

#define OGMDVD_DRIVE_CHOOSER_WIDGET_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMDVD_TYPE_DRIVE_CHOOSER_WIDGET, OGMDvdDriveChooserWidgetPriv))

enum
{
  TEXT_COLUMN,
  TYPE_COLUMN,
  DEVICE_COLUMN,
  DRIVE_COLUMN,
  N_COLUMNS
};

enum
{
  FILE_SEP_ROW = OGMDVD_DEVICE_DIR + 1,
  SEL_SEP_ROW,
  FILE_SEL_ROW,
  DIR_SEL_ROW
};

struct _OGMDvdDriveChooserWidgetPriv
{
  GtkTreeRowReference *last_row;
};

static void     ogmdvd_drive_chooser_init              (OGMDvdDriveChooserInterface *iface);
static void     ogmdvd_drive_chooser_widget_changed    (GtkComboBox                 *combo_box);
static gchar *  ogmdvd_drive_chooser_widget_get_device (OGMDvdDriveChooser          *chooser,
                                                        OGMDvdDeviceType            *type);
static void     ogmdvd_drive_chooser_widget_fill       (OGMDvdDriveChooserWidget    *chooser);
static gboolean ogmdvd_drive_chooser_widget_sep_func   (GtkTreeModel                *model,
                                                        GtkTreeIter                 *iter,
                                                        gpointer                    data);

G_DEFINE_TYPE_WITH_CODE (OGMDvdDriveChooserWidget, ogmdvd_drive_chooser_widget, GTK_TYPE_COMBO_BOX,
    G_IMPLEMENT_INTERFACE (OGMDVD_TYPE_DRIVE_CHOOSER, ogmdvd_drive_chooser_init))

static void
ogmdvd_drive_chooser_widget_class_init (OGMDvdDriveChooserWidgetClass *klass)
{
  GtkComboBoxClass *combo_box_class;

  combo_box_class = (GtkComboBoxClass *) klass;
  combo_box_class->changed = ogmdvd_drive_chooser_widget_changed;

  g_type_class_add_private (klass, sizeof (OGMDvdDriveChooserWidgetPriv));
}

static void
ogmdvd_drive_chooser_init (OGMDvdDriveChooserInterface  *iface)
{
  iface->get_device = ogmdvd_drive_chooser_widget_get_device;
}

static void
ogmdvd_drive_chooser_widget_init (OGMDvdDriveChooserWidget *chooser)
{
  GtkCellRenderer *cell;
  GtkListStore *store;

  chooser->priv = OGMDVD_DRIVE_CHOOSER_WIDGET_GET_PRIVATE (chooser);

  store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_POINTER);
  gtk_combo_box_set_model (GTK_COMBO_BOX (chooser), GTK_TREE_MODEL (store));
  g_object_unref (store);

  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (chooser),
      ogmdvd_drive_chooser_widget_sep_func, NULL, NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser), cell, "markup", TEXT_COLUMN, NULL);

  ogmdvd_drive_chooser_widget_fill (chooser);
}

static void
ogmdvd_drive_chooser_widget_add_device (GtkComboBox *combo_box, const gchar *device, gboolean file)
{
  GtkTreeModel *model;
  GtkTreeIter sibling, iter;
  gint type;

  model = gtk_combo_box_get_model (combo_box);
  if (gtk_tree_model_get_iter_first (model, &sibling))
  {
    do
    {
      gtk_tree_model_get (model, &sibling, TYPE_COLUMN, &type, -1);
      if (type != OGMDVD_DEVICE_BLOCK && type != OGMDVD_DEVICE_NONE)
        break;
    }
    while (gtk_tree_model_iter_next (model, &sibling));

    if (type == SEL_SEP_ROW)
    {
      gtk_list_store_insert_before (GTK_LIST_STORE (model), &iter, &sibling);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          TEXT_COLUMN, NULL, TYPE_COLUMN, FILE_SEP_ROW, -1);

      gtk_list_store_insert_before (GTK_LIST_STORE (model), &iter, &sibling);
    }
    else if (type == FILE_SEP_ROW)
    {
      if (gtk_tree_model_iter_next (model, &sibling))
      {
        gtk_tree_model_get (model, &sibling, TYPE_COLUMN, &type, -1);
        if (type == OGMDVD_DEVICE_FILE || type == OGMDVD_DEVICE_DIR)
          iter = sibling;
      }
    }
  }

  if (gtk_list_store_iter_is_valid (GTK_LIST_STORE (model), &iter))
  {
    OGMDvdDisc *disc;

    disc = ogmdvd_disc_new (device, NULL);
    if (disc)
    {
      gchar *title, *text;

      title = g_markup_escape_text (ogmdvd_disc_get_label (disc), -1);
      text = g_strdup_printf ("<b>%s</b>\n%s", title, device);
      g_free (title);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter, TEXT_COLUMN, text,
          TYPE_COLUMN, file ? OGMDVD_DEVICE_FILE : OGMDVD_DEVICE_DIR, DEVICE_COLUMN, device, DRIVE_COLUMN, NULL, -1);
      g_free (text);

      gtk_combo_box_set_active_iter (combo_box, &iter);
    }
  }
}

static GtkWidget *
gtk_widget_get_transient_window (GtkWidget *widget)
{
  GtkWidget *parent;

  parent = gtk_widget_get_parent (widget);
  while (parent)
  {
    widget = parent;
    parent = gtk_widget_get_parent (widget);
  }

  if (!GTK_IS_WINDOW (widget))
    return NULL;

  return widget;
}

static gchar *
ogmdvd_drive_chooser_widget_select_file (GtkComboBox *combo_box, gboolean file)
{
  GtkWidget *dialog, *parent;
  GdkPixbuf *pixbuf;

  gchar *device = NULL;

  dialog = gtk_file_chooser_dialog_new (file ? _("Select an ISO file") : _("Select a DVD structure"),
      NULL, file ? GTK_FILE_CHOOSER_ACTION_OPEN : GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

  parent = gtk_widget_get_transient_window (GTK_WIDGET (combo_box));
  if (parent)
  {
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_gravity (GTK_WINDOW (dialog), GDK_GRAVITY_CENTER);
    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  }

#if GTK_CHECK_VERSION(3,0,0)
  pixbuf = gtk_widget_render_icon_pixbuf (GTK_WIDGET (dialog), GTK_STOCK_OPEN, GTK_ICON_SIZE_DIALOG);
#else
  pixbuf = gtk_widget_render_icon (GTK_WIDGET (dialog), GTK_STOCK_OPEN, GTK_ICON_SIZE_DIALOG, NULL);
#endif
  if (pixbuf)
  {
    gtk_window_set_icon (GTK_WINDOW (dialog), pixbuf);
    g_object_unref (pixbuf);
  }

  if (file)
  {
    GtkFileFilter *filter;

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("ISO images"));
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
    gtk_file_filter_add_mime_type (filter, "application/x-cd-image");
  }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    gchar *dir;

    gtk_widget_hide (dialog);

    dir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    if (dir)
    {
      gchar *basename;

      basename = g_path_get_basename (dir);
      if (g_ascii_strcasecmp (basename, "video_ts") == 0)
        device = g_path_get_dirname (dir);
      else
        device = g_strdup (dir);
      g_free (basename);
    }
    g_free (dir);
  }

  gtk_widget_destroy (dialog);

  return device;
}

static void
ogmdvd_drive_chooser_widget_changed (GtkComboBox *combo_box)
{
  OGMDvdDriveChooserWidget *chooser;

  GtkTreeModel *model;
  GtkTreeIter iter;

  gchar *device = NULL;
  gint type;

  chooser = OGMDVD_DRIVE_CHOOSER_WIDGET (combo_box);
  model = gtk_combo_box_get_model (combo_box);

  if (gtk_combo_box_get_active_iter (combo_box, &iter))
  {
    gtk_tree_model_get (model, &iter, TYPE_COLUMN, &type, -1);
    switch (type)
    {
      case OGMDVD_DEVICE_BLOCK:
      case OGMDVD_DEVICE_FILE:
      case OGMDVD_DEVICE_DIR:
        gtk_tree_model_get (model, &iter, DEVICE_COLUMN, &device, -1);
        break;
      case FILE_SEL_ROW:
      case DIR_SEL_ROW:
        device = ogmdvd_drive_chooser_widget_select_file (combo_box, type == FILE_SEL_ROW);
        if (device)
          ogmdvd_drive_chooser_widget_add_device (combo_box, device, type == FILE_SEL_ROW);
        else
        {
          if (!chooser->priv->last_row)
            gtk_combo_box_set_active (combo_box, 0);
          else
          {
            GtkTreePath *path;
            GtkTreeIter prev;

            path = gtk_tree_row_reference_get_path (chooser->priv->last_row);
            if (gtk_tree_model_get_iter (model, &prev, path))
              gtk_combo_box_set_active_iter (combo_box, &prev);
            else
              gtk_combo_box_set_active (combo_box, 0);
            gtk_tree_path_free (path);
          }
        }
        break;
      default:
        break;
    }

    if (type <= OGMDVD_DEVICE_DIR)
    {
      GtkTreePath *path;

      if (chooser->priv->last_row)
        gtk_tree_row_reference_free (chooser->priv->last_row);

      path = gtk_tree_model_get_path (model, &iter);
      chooser->priv->last_row = gtk_tree_row_reference_new (model, path);
      gtk_tree_path_free (path);
    }
  }

  g_signal_emit_by_name (combo_box, "device-changed", device, type <= OGMDVD_DEVICE_DIR ? type : OGMDVD_DEVICE_NONE);

  if (device)
    g_free (device);
}

/*
 * Internal signals
 */

static void
ogmdvd_drive_chooser_widget_medium_removed (OGMDvdDriveChooserWidget *chooser, OGMDvdDrive *drive1)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint type;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    OGMDvdDrive *drive2;

    do
    {
      gtk_tree_model_get (model, &iter, TYPE_COLUMN, &type, DRIVE_COLUMN, &drive2, -1);
      if (type == OGMDVD_DEVICE_BLOCK && drive1 == drive2)
      {
        gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
        break;
      }
    }
    while (gtk_tree_model_iter_next (model, &iter));
  }

  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    gtk_tree_model_get (model, &iter, TYPE_COLUMN, &type, -1);
    if (type != OGMDVD_DEVICE_BLOCK && type != OGMDVD_DEVICE_NONE)
    {
      GtkTreeIter it;

      gtk_list_store_insert_before (GTK_LIST_STORE (model), &it, &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &it,
          TEXT_COLUMN, _("<b>No DVD</b>\nNo device"), TYPE_COLUMN, OGMDVD_DEVICE_NONE, -1);
    }
  }

  if (gtk_combo_box_get_active (GTK_COMBO_BOX (chooser)) == -1)
    gtk_combo_box_set_active (GTK_COMBO_BOX (chooser), 0);
}

static void
ogmdvd_drive_chooser_widget_medium_added (OGMDvdDriveChooserWidget *chooser, OGMDvdDrive *drive)
{
  OGMDvdDisc *disc;

  disc = ogmdvd_disc_new (ogmdvd_drive_get_device (drive), NULL);
  if (!disc)
    ogmdvd_drive_chooser_widget_medium_removed (chooser, drive);
  else
  {
    GtkTreeModel *model;
    GtkTreeIter sibling, iter;
    gchar *text, *name, *title;

    name = ogmdvd_drive_get_name (drive);
    if (!name)
      name = g_strdup (_("Unknown Drive"));

    title = g_markup_escape_text (ogmdvd_disc_get_label (disc), -1);
    text = g_strdup_printf ("<b>%s</b>\n%s", title, name);
    g_free (title);

    g_free (name);

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
    if (gtk_tree_model_get_iter_first (model, &sibling))
    {
      gint type;

      do
      {
        gtk_tree_model_get (model, &sibling, TYPE_COLUMN, &type, -1);
        if (type != OGMDVD_DEVICE_BLOCK)
          break;
      }
      while (gtk_tree_model_iter_next (model, &sibling));

      if (type == OGMDVD_DEVICE_NONE)
        iter = sibling;
      else
        gtk_list_store_insert_before (GTK_LIST_STORE (model), &iter, &sibling);
    }
    else
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);

    gtk_list_store_set (GTK_LIST_STORE (model), &iter, TEXT_COLUMN, text, TYPE_COLUMN, OGMDVD_DEVICE_BLOCK,
        DEVICE_COLUMN, ogmdvd_drive_get_device (drive), DRIVE_COLUMN, drive, -1);
    g_free (text);

    ogmdvd_disc_unref (disc);

    if (gtk_combo_box_get_active (GTK_COMBO_BOX (chooser)) == -1)
      gtk_combo_box_set_active (GTK_COMBO_BOX (chooser), 0);
    else
    {
      if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (chooser), &sibling))
      {
        GtkTreePath *path1, *path2;

        path1 = gtk_tree_model_get_path (model, &iter);
        path2 = gtk_tree_model_get_path (model, &sibling);

        if (path1 && path2 && gtk_tree_path_compare (path1, path2) == 0)
          ogmdvd_drive_chooser_widget_changed (GTK_COMBO_BOX (chooser));

        if (path1)
          gtk_tree_path_free (path1);

        if (path2)
          gtk_tree_path_free (path2);
      }
    }
  }
}

/*
 * Internal functions
 */

typedef struct
{
  OGMDvdDrive *drive;
  gulong handler;
} OGMRipDisconnector;

static void
ogmrip_signal_disconnector (gpointer data, GObject *gobject)
{
  OGMRipDisconnector *disconnector = data;

  g_signal_handler_disconnect (disconnector->drive, disconnector->handler);

  g_free (disconnector);
}

static void
ogmdvd_drive_chooser_widget_fill (OGMDvdDriveChooserWidget *chooser)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GSList *drives, *drive;

  OGMDvdMonitor *monitor;
  OGMRipDisconnector *disconnector;

  monitor = ogmdvd_monitor_get_default ();
  drives = ogmdvd_monitor_get_drives (monitor);
  g_object_unref (monitor);

  for (drive = drives; drive; drive = drive->next)
  {
    if (ogmdvd_drive_get_drive_type (OGMDVD_DRIVE (drive->data)) & OGMDVD_DRIVE_DVD)
    {
      disconnector = g_new0 (OGMRipDisconnector, 1);

      disconnector->drive = drive->data;
      disconnector->handler = g_signal_connect_swapped (drive->data, "medium-added",
          G_CALLBACK (ogmdvd_drive_chooser_widget_medium_added), chooser);

      g_object_weak_ref (G_OBJECT (chooser), ogmrip_signal_disconnector, disconnector);

      disconnector = g_new0 (OGMRipDisconnector, 1);

      disconnector->drive = drive->data;
      disconnector->handler = g_signal_connect_swapped (drive->data, "medium-removed", 
          G_CALLBACK (ogmdvd_drive_chooser_widget_medium_removed), chooser);

      g_object_weak_ref (G_OBJECT (chooser), ogmrip_signal_disconnector, disconnector);
/*
      ogmdvd_drive_chooser_widget_medium_added (chooser, OGMDVD_DRIVE (drive->data));
*/
    }
  }

  g_slist_foreach (drives, (GFunc) g_object_unref, NULL);
  g_slist_free (drives);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));

  if (gtk_tree_model_iter_n_children (model, NULL) == 0)
  {
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
        TEXT_COLUMN, _("<b>No DVD</b>\nNo device"), TYPE_COLUMN, OGMDVD_DEVICE_NONE, -1);
  }

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      TEXT_COLUMN, NULL, TYPE_COLUMN, SEL_SEP_ROW, -1);

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      TEXT_COLUMN, _("Select a DVD structure..."), TYPE_COLUMN, DIR_SEL_ROW, -1);

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      TEXT_COLUMN, _("Select an ISO file..."), TYPE_COLUMN, FILE_SEL_ROW, -1);

  gtk_combo_box_set_active (GTK_COMBO_BOX (chooser), 0);
}

static gboolean
ogmdvd_drive_chooser_widget_sep_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  gint type = OGMDVD_DEVICE_NONE;

  gtk_tree_model_get (model, iter, TYPE_COLUMN, &type, -1);

  return (type == FILE_SEP_ROW || type == SEL_SEP_ROW);
}

static gchar *
ogmdvd_drive_chooser_widget_get_device (OGMDvdDriveChooser *chooser, OGMDvdDeviceType *type)
{
  const gchar *device;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint atype;

  g_return_val_if_fail (OGMDVD_IS_DRIVE_CHOOSER_WIDGET (chooser), NULL);

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (chooser), &iter))
    return NULL;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));

  gtk_tree_model_get (model, &iter, TYPE_COLUMN, &atype, -1);
  if (atype != OGMDVD_DEVICE_BLOCK && atype != OGMDVD_DEVICE_FILE && atype != OGMDVD_DEVICE_DIR)
    return NULL;

  gtk_tree_model_get (model, &iter, DEVICE_COLUMN, &device, -1);

  if (type)
    *type = atype;

  if (!device)
    return NULL;

  return g_strdup (device);
}

/**
 * ogmdvd_drive_chooser_widget_new:
 *
 * Creates a new #OGMDvdDriveChooserWidget.
 *
 * Returns: The new #OGMDvdDriveChooserWidget
 */
GtkWidget *
ogmdvd_drive_chooser_widget_new (void)
{
  GtkWidget *widget;

  widget = g_object_new (OGMDVD_TYPE_DRIVE_CHOOSER_WIDGET, NULL);

  return widget;
}

