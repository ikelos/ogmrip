/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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
 * SECTION:ogmrip-media-chooser-widget
 * @title: OGMRipMediaChooserWidget
 * @include: ogmrip-media-chooser-widget.h
 * @short_description: Media chooser widget that can be embedded in other widgets
 */

#include "ogmrip-media-chooser-widget.h"

#include <ogmrip-base.h>
#include <ogmrip-media.h>

#include <glib/gi18n-lib.h>

#define OGMRIP_MEDIA_CHOOSER_WIDGET_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_MEDIA_CHOOSER_WIDGET, OGMRipMediaChooserWidgetPriv))

enum
{
  TEXT_COLUMN,
  TYPE_COLUMN,
  MEDIA_COLUMN,
  VOLUME_COLUMN,
  N_COLUMNS
};

enum
{
  NONE_ROW,
  DEVICE_ROW,
  FILE_ROW,
  DIR_ROW,
  FILE_SEP_ROW,
  SEL_SEP_ROW,
  FILE_SEL_ROW,
  DIR_SEL_ROW
};

struct _OGMRipMediaChooserWidgetPriv
{
  GVolumeMonitor *monitor;
  GtkTreeRowReference *last_row;
};

static void ogmrip_media_chooser_init (OGMRipMediaChooserInterface *iface);
static void ogmrip_media_chooser_widget_dispose (GObject     *gobject);
static void ogmrip_media_chooser_widget_changed (GtkComboBox *combo);

static OGMRipMedia *
ogmrip_media_chooser_widget_get_media (OGMRipMediaChooser *chooser)
{
  OGMRipMedia *media;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint type;

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (chooser), &iter))
    return NULL;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));

  gtk_tree_model_get (model, &iter, TYPE_COLUMN, &type, -1);
  if (type != DEVICE_ROW && type != FILE_ROW && type != DIR_ROW)
    return NULL;

  gtk_tree_model_get (model, &iter, MEDIA_COLUMN, &media, -1);
  if (media)
    g_object_unref (media);

  return media;
}

static gboolean
ogmrip_media_chooser_widget_sep_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  gint type = NONE_ROW;

  gtk_tree_model_get (model, iter, TYPE_COLUMN, &type, -1);

  return (type == FILE_SEP_ROW || type == SEL_SEP_ROW);
}

static void
ogmrip_media_chooser_widget_volume_removed (OGMRipMediaChooserWidget *chooser, GVolume *volume1)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint type;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    GVolume *volume2;

    do
    {
      gtk_tree_model_get (model, &iter, TYPE_COLUMN, &type, VOLUME_COLUMN, &volume2, -1);
      if (volume2)
        g_object_unref (volume2);

      if (type == DEVICE_ROW && volume1 == volume2)
      {
        g_signal_handlers_disconnect_by_func (volume1,
            ogmrip_media_chooser_widget_volume_removed, chooser);
        gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
        break;
      }
    }
    while (gtk_tree_model_iter_next (model, &iter));
  }

  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    gtk_tree_model_get (model, &iter, TYPE_COLUMN, &type, -1);
    if (type != DEVICE_ROW && type != NONE_ROW)
    {
      GtkTreeIter it;

      gtk_list_store_insert_before (GTK_LIST_STORE (model), &it, &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &it,
          TEXT_COLUMN, _("<b>No media</b>\nNo drive"), TYPE_COLUMN, NONE_ROW, -1);
    }
  }

  if (gtk_combo_box_get_active (GTK_COMBO_BOX (chooser)) == -1)
    gtk_combo_box_set_active (GTK_COMBO_BOX (chooser), 0);
}

static void
ogmrip_media_chooser_widget_volume_added (OGMRipMediaChooserWidget *chooser, GVolume *volume, GVolumeMonitor *monitor)
{
  GDrive *drive;

  drive = g_volume_get_drive (volume);
  if (drive && g_drive_has_media (drive))
  {
    OGMRipMedia *media;
    gchar *str;

    str = g_volume_get_identifier (volume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
    media = ogmrip_media_new (str);
    g_free (str);

    if (!media)
      ogmrip_media_chooser_widget_volume_removed (chooser, volume);
    else
    {
      GtkTreeModel *model;
      GtkTreeIter sibling, iter;
      gchar *text, *name, *title;

      name = g_volume_get_name (volume);
      if (!name)
        name = g_strdup (_("Unknown media"));

      title = g_markup_escape_text (ogmrip_media_get_label (media), -1);
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
          if (type != DEVICE_ROW)
            break;
        }
        while (gtk_tree_model_iter_next (model, &sibling));

        if (type == NONE_ROW)
          iter = sibling;
        else
          gtk_list_store_insert_before (GTK_LIST_STORE (model), &iter, &sibling);
      }
      else
        gtk_list_store_append (GTK_LIST_STORE (model), &iter);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter, TEXT_COLUMN, text,
          TYPE_COLUMN, DEVICE_ROW, MEDIA_COLUMN, media, VOLUME_COLUMN, volume, -1);
      g_free (text);

      g_object_unref (media);

      g_signal_connect_swapped (volume, "removed",
          G_CALLBACK (ogmrip_media_chooser_widget_volume_removed), chooser);

      if (gtk_combo_box_get_active (GTK_COMBO_BOX (chooser)) >= 0)
      {
        if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (chooser), &sibling))
        {
          GtkTreePath *path1, *path2;

          path1 = gtk_tree_model_get_path (model, &iter);
          path2 = gtk_tree_model_get_path (model, &sibling);

          if (path1 && path2 && gtk_tree_path_compare (path1, path2) == 0)
            ogmrip_media_chooser_widget_changed (GTK_COMBO_BOX (chooser));

          if (path1)
            gtk_tree_path_free (path1);

          if (path2)
            gtk_tree_path_free (path2);
        }
      }
    }
  }

  if (drive)
    g_object_unref (drive);
}

G_DEFINE_TYPE_WITH_CODE (OGMRipMediaChooserWidget, ogmrip_media_chooser_widget, GTK_TYPE_COMBO_BOX,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_MEDIA_CHOOSER, ogmrip_media_chooser_init))

static void
ogmrip_media_chooser_widget_class_init (OGMRipMediaChooserWidgetClass *klass)
{
  GObjectClass *gobject_class;
  GtkComboBoxClass *combo_box_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_media_chooser_widget_dispose;

  combo_box_class = GTK_COMBO_BOX_CLASS (klass);
  combo_box_class->changed = ogmrip_media_chooser_widget_changed;

  g_type_class_add_private (klass, sizeof (OGMRipMediaChooserWidgetPriv));
}

static void
ogmrip_media_chooser_init (OGMRipMediaChooserInterface  *iface)
{
  iface->get_media = ogmrip_media_chooser_widget_get_media;
}

static void
ogmrip_media_chooser_widget_init (OGMRipMediaChooserWidget *chooser)
{
  GtkCellRenderer *cell;
  GtkListStore *store;
  GtkTreeIter iter;

  GList *volumes, *volume;

  chooser->priv = OGMRIP_MEDIA_CHOOSER_WIDGET_GET_PRIVATE (chooser);

  store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_INT, OGMRIP_TYPE_MEDIA, G_TYPE_VOLUME);
  gtk_combo_box_set_model (GTK_COMBO_BOX (chooser), GTK_TREE_MODEL (store));
  g_object_unref (store);

  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (chooser),
      ogmrip_media_chooser_widget_sep_func, NULL, NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser), cell, "markup", TEXT_COLUMN, NULL);

  chooser->priv->monitor = g_volume_monitor_get ();
  g_signal_connect_swapped (chooser->priv->monitor, "volume-added",
      G_CALLBACK (ogmrip_media_chooser_widget_volume_added), chooser);

  volumes = g_volume_monitor_get_volumes (chooser->priv->monitor);
  for (volume = volumes; volume; volume = volume->next)
    ogmrip_media_chooser_widget_volume_added (chooser, volume->data, chooser->priv->monitor);

  g_list_foreach (volumes, (GFunc) g_object_unref, NULL);
  g_list_free (volumes);

  if (gtk_tree_model_iter_n_children (GTK_TREE_MODEL (store), NULL) == 0)
  {
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        TEXT_COLUMN, _("<b>No media</b>\nNo drive"), TYPE_COLUMN, NONE_ROW, -1);
  }

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
      TEXT_COLUMN, NULL, TYPE_COLUMN, SEL_SEP_ROW, -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
      TEXT_COLUMN, _("Select a media directory..."), TYPE_COLUMN, DIR_SEL_ROW, -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
      TEXT_COLUMN, _("Select a media file..."), TYPE_COLUMN, FILE_SEL_ROW, -1);
}

static void
ogmrip_media_chooser_widget_dispose (GObject *gobject)
{
  OGMRipMediaChooserWidget *chooser = OGMRIP_MEDIA_CHOOSER_WIDGET (gobject);

  if (chooser->priv->monitor)
  {
    g_signal_handlers_disconnect_by_func (chooser->priv->monitor,
        ogmrip_media_chooser_widget_volume_added, chooser);
    g_object_unref (chooser->priv->monitor);
    chooser->priv->monitor = NULL;
  }

  if (chooser->priv->last_row)
  {
    gtk_tree_row_reference_free (chooser->priv->last_row);
    chooser->priv->last_row = NULL;
  }

  G_OBJECT_CLASS (ogmrip_media_chooser_widget_parent_class)->dispose (gobject);
}

static gboolean
ogmrip_media_chooser_widget_add_media (GtkComboBox *combo, OGMRipMedia *media, gboolean file)
{
  GtkTreeModel *model;
  GtkTreeIter sibling, iter;

  gchar *title, *text;
  gint type;

  model = gtk_combo_box_get_model (combo);
  if (gtk_tree_model_get_iter_first (model, &sibling))
  {
    do
    {
      gtk_tree_model_get (model, &sibling, TYPE_COLUMN, &type, -1);
      if (type != DEVICE_ROW && type != NONE_ROW)
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
        if (type == FILE_ROW || type == DIR_ROW)
          iter = sibling;
      }
    }
  }

  if (!gtk_list_store_iter_is_valid (GTK_LIST_STORE (model), &iter))
    return FALSE;

  title = g_markup_escape_text (ogmrip_media_get_label (media), -1);
  text = g_strdup_printf ("<b>%s</b>\n%s", title, ogmrip_media_get_uri (media) + 6);
  g_free (title);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter, TEXT_COLUMN, text,
      TYPE_COLUMN, file ? FILE_ROW : DIR_ROW, MEDIA_COLUMN, media, VOLUME_COLUMN, NULL, -1);
  g_free (text);

  gtk_combo_box_set_active_iter (combo, &iter);

  return TRUE;
}

static OGMRipMedia *
ogmrip_media_chooser_widget_select_file (GtkComboBox *combo, gboolean file)
{
  OGMRipMedia *media = NULL;
  GtkWidget *dialog, *toplevel;

  dialog = gtk_file_chooser_dialog_new (file ? _("Select an media file") : _("Select a media directory"),
      NULL, file ? GTK_FILE_CHOOSER_ACTION_OPEN : GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (combo));
  if (gtk_widget_is_toplevel (toplevel) && GTK_IS_WINDOW (toplevel))
  {
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (toplevel));
    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  }

  if (file)
  {
    GtkFileFilter *filter;

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Supported files"));
    gtk_file_filter_add_mime_type (filter, "video/*");
    gtk_file_filter_add_mime_type (filter, "application/x-cd-image");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    gchar *path;

    gtk_widget_hide (dialog);

    path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    if (path)
    {
      media = ogmrip_media_new (path);
      g_free (path);
    }
  }

  gtk_widget_destroy (dialog);

  return media;
}

static void
ogmrip_media_chooser_widget_changed (GtkComboBox *combo)
{
  OGMRipMediaChooserWidget *chooser;
  OGMRipMedia *media = NULL;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint type;

  chooser = OGMRIP_MEDIA_CHOOSER_WIDGET (combo);
  model = gtk_combo_box_get_model (combo);

  if (gtk_combo_box_get_active_iter (combo, &iter))
  {
    gtk_tree_model_get (model, &iter, TYPE_COLUMN, &type, -1);
    switch (type)
    {
      case DEVICE_ROW:
      case FILE_ROW:
      case DIR_ROW:
        gtk_tree_model_get (model, &iter, MEDIA_COLUMN, &media, -1);
        break;
      case FILE_SEL_ROW:
      case DIR_SEL_ROW:
        media = ogmrip_media_chooser_widget_select_file (combo, type == FILE_SEL_ROW);
        if (!media || !ogmrip_media_chooser_widget_add_media (combo, media, type == FILE_SEL_ROW))
        {
          if (!chooser->priv->last_row)
            gtk_combo_box_set_active (combo, 0);
          else
          {
            GtkTreePath *path;
            GtkTreeIter prev;

            path = gtk_tree_row_reference_get_path (chooser->priv->last_row);
            if (gtk_tree_model_get_iter (model, &prev, path))
              gtk_combo_box_set_active_iter (combo, &prev);
            else
              gtk_combo_box_set_active (combo, 0);
            gtk_tree_path_free (path);
          }
        }
        break;
      default:
        break;
    }

    if (type <= DIR_ROW)
    {
      GtkTreePath *path;

      if (chooser->priv->last_row)
        gtk_tree_row_reference_free (chooser->priv->last_row);

      path = gtk_tree_model_get_path (model, &iter);
      chooser->priv->last_row = gtk_tree_row_reference_new (model, path);
      gtk_tree_path_free (path);
    }
  }

  g_signal_emit_by_name (combo, "media-changed", media);

  if (media)
    g_object_unref (media);
}

/**
 * ogmrip_media_chooser_widget_new:
 *
 * Creates a new #OGMRipMediaChooserWidget.
 *
 * Returns: The new #OGMRipMediaChooserWidget
 */
GtkWidget *
ogmrip_media_chooser_widget_new (void)
{
  return g_object_new (OGMRIP_TYPE_MEDIA_CHOOSER_WIDGET, NULL);
}

GVolume *
ogmrip_media_chooser_widget_get_volume (OGMRipMediaChooserWidget *chooser)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GVolume *volume;

  g_return_val_if_fail (OGMRIP_IS_MEDIA_CHOOSER_WIDGET (chooser), NULL);

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (chooser), &iter))
    return NULL;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
  gtk_tree_model_get (model, &iter, VOLUME_COLUMN, &volume, -1);
  if (volume)
    g_object_unref (volume);

  return volume;
}

