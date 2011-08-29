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

/**
 * SECTION:ogmrip-helper
 * @title: Helper
 * @include: ogmrip-source-chooser.h
 * @short_description: A list of helper functions
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-helper.h"
#include "ogmrip-profile-store.h"

#include <glib/gi18n-lib.h>

#include <stdlib.h>
#include <locale.h>

#include <ogmdvd.h>

extern const gchar *ogmdvd_languages[][3];
extern const guint  ogmdvd_nlanguages;

guint
g_settings_get_uint (GSettings *settings, const gchar *key)
{
  guint val;

  g_return_val_if_fail (G_IS_SETTINGS (settings), 0);
  g_return_val_if_fail (key != NULL, 0);

  g_settings_get (settings, key, "u", &val);

  return val;
}

void
gtk_container_clear (GtkContainer *container)
{
  GList *list, *link;

  g_return_if_fail (GTK_IS_CONTAINER (container));

  list = gtk_container_get_children (container);
  for (link = list; link; link = link->next)
    gtk_container_remove (container, GTK_WIDGET (link->data));
  g_list_free (list);
}

static void
gtk_dialog_response_accept (GtkDialog *dialog)
{
  gtk_dialog_response (dialog, GTK_RESPONSE_ACCEPT);
}

enum
{
  COL_CODEC_DESCRIPTION,
  COL_CODEC_NAME,
  COL_CODEC_TYPE,
  COL_CODEC_VISIBLE,
  N_CODEC_COLUMNS
};

static void
ogmrip_codec_chooser_construct (GtkComboBox *chooser)
{
  GtkListStore *store;
  GtkTreeModel *filter;
  GtkCellRenderer *cell;

  g_return_if_fail (GTK_IS_COMBO_BOX (chooser));

  store = gtk_list_store_new (N_CODEC_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_GTYPE, G_TYPE_BOOLEAN);
  filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (store), NULL);
  g_object_unref (store);

  gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (filter), COL_CODEC_VISIBLE);
  gtk_combo_box_set_model (chooser, filter);
  g_object_unref (filter);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser), cell, "text", COL_CODEC_DESCRIPTION, NULL);
}

static void
ogmrip_combo_box_append_item (GType type, const gchar *name, const gchar *description, GtkTreeModel *model)
{
  GtkTreeIter iter;

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      COL_CODEC_DESCRIPTION, gettext (description), COL_CODEC_NAME, name, COL_CODEC_TYPE, type, COL_CODEC_VISIBLE, TRUE, -1);
}

/**
 * ogmrip_container_chooser_construct:
 * @chooser: A #GtkComboBox
 *
 * Configures @chooser to store containers.
 */
void
ogmrip_container_chooser_construct (GtkComboBox *chooser)
{
  GtkTreeModel *filter, *model;

  g_return_if_fail (GTK_IS_COMBO_BOX (chooser));

  ogmrip_codec_chooser_construct (chooser);

  filter = gtk_combo_box_get_model (chooser);
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));
  gtk_list_store_clear (GTK_LIST_STORE (model));

  ogmrip_plugin_foreach_container ((OGMRipPluginFunc) ogmrip_combo_box_append_item, model);

  gtk_widget_set_sensitive (GTK_WIDGET (chooser), ogmrip_plugin_get_n_containers () > 0);
}

/**
 * ogmrip_video_codec_chooser_construct:
 * @chooser: A #GtkComboBox
 *
 * Configures @chooser to store video codecs.
 */
void
ogmrip_video_codec_chooser_construct (GtkComboBox *chooser)
{
  GtkTreeModel *filter, *model;

  g_return_if_fail (GTK_IS_COMBO_BOX (chooser));

  ogmrip_codec_chooser_construct (chooser);

  filter = gtk_combo_box_get_model (chooser);
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));
  gtk_list_store_clear (GTK_LIST_STORE (model));

  ogmrip_plugin_foreach_video_codec ((OGMRipPluginFunc) ogmrip_combo_box_append_item, model);

  gtk_widget_set_sensitive (GTK_WIDGET (chooser), ogmrip_plugin_get_n_video_codecs () > 0);
}

void
ogmrip_video_codec_chooser_filter (GtkComboBox *chooser, GType container)
{
  GtkTreeModel *filter, *model;
  GtkTreeIter iter;
  GType type;

  filter = gtk_combo_box_get_model (chooser);
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));

  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    do
    {
      gtk_tree_model_get (model, &iter, COL_CODEC_TYPE, &type, -1);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          COL_CODEC_VISIBLE, ogmrip_plugin_can_contain_video (container, type), -1);
    }
    while (gtk_tree_model_iter_next (model, &iter));
  }
}

/**
 * ogmrip_audio_codec_chooser_construct:
 * @chooser: A #GtkComboBox
 *
 * Configures @chooser to store audio codecs.
 */
void
ogmrip_audio_codec_chooser_construct (GtkComboBox *chooser)
{
  GtkTreeModel *filter, *model;

  g_return_if_fail (GTK_IS_COMBO_BOX (chooser));

  ogmrip_codec_chooser_construct (chooser);

  filter = gtk_combo_box_get_model (chooser);
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));
  gtk_list_store_clear (GTK_LIST_STORE (model));

  ogmrip_plugin_foreach_audio_codec ((OGMRipPluginFunc) ogmrip_combo_box_append_item, model);

  gtk_widget_set_sensitive (GTK_WIDGET (chooser), ogmrip_plugin_get_n_audio_codecs () > 0);
}

void
ogmrip_audio_codec_chooser_filter (GtkComboBox *chooser, GType container)
{
  GtkTreeModel *filter, *model;
  GtkTreeIter iter;
  GType type;

  filter = gtk_combo_box_get_model (chooser);
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));

  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    do
    {
      gtk_tree_model_get (model, &iter, COL_CODEC_TYPE, &type, -1);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          COL_CODEC_VISIBLE, ogmrip_plugin_can_contain_audio (container, type), -1);
    }
    while (gtk_tree_model_iter_next (model, &iter));
  }
}

/**
 * ogmrip_subp_codec_chooser_construct
 * @chooser: A #GtkComboBox
 *
 * Configures @chooser to store subp codecs.
 */
void
ogmrip_subp_codec_chooser_construct (GtkComboBox *chooser)
{
  GtkTreeModel *filter, *model;

  g_return_if_fail (GTK_IS_COMBO_BOX (chooser));

  ogmrip_codec_chooser_construct (chooser);

  filter = gtk_combo_box_get_model (chooser);
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));
  gtk_list_store_clear (GTK_LIST_STORE (model));

  ogmrip_plugin_foreach_subp_codec ((OGMRipPluginFunc) ogmrip_combo_box_append_item, model);

  gtk_widget_set_sensitive (GTK_WIDGET (chooser), ogmrip_plugin_get_n_subp_codecs () > 0);
}

void
ogmrip_subp_codec_chooser_filter (GtkComboBox *chooser, GType container)
{
  GtkTreeModel *filter, *model;
  GtkTreeIter iter;
  GType type;

  filter = gtk_combo_box_get_model (chooser);
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));

  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    do
    {
      gtk_tree_model_get (model, &iter, COL_CODEC_TYPE, &type, -1);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          COL_CODEC_VISIBLE, ogmrip_plugin_can_contain_subp (container, type), -1);
    }
    while (gtk_tree_model_iter_next (model, &iter));
  }
}

/**
 * ogmrip_codec_chooser_set_active:
 * @chooser: A #GtkComboBox
 * @name: The name of the codec
 *
 * Selects the codec with the given @name.
 */
void
ogmrip_codec_chooser_set_active (GtkComboBox *chooser, const char *name)
{
  if (!name)
    gtk_combo_box_set_active (chooser, -1);
  else
  {
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_combo_box_get_model (chooser);
    if (gtk_tree_model_get_iter_first (model, &iter))
    {
      gchar *str;

      do
      {
        gtk_tree_model_get (model, &iter, COL_CODEC_NAME, &str, -1);
        if (g_str_equal (str, name))
        {
          gtk_combo_box_set_active_iter (chooser, &iter);
          g_free (str);
          break;
        }
        g_free (str);
      }
      while (gtk_tree_model_iter_next (model, &iter));

      if (gtk_combo_box_get_active (chooser) < 0)
        gtk_combo_box_set_active (chooser, 0);
    }
  }
}

/**
 * ogmrip_codec_chooser_get_active:
 * @chooser: A #GtkComboBox
 *
 * Returns the selected codec.
 *
 * Returns: the name of the codec, or NULL
 */
gchar *
ogmrip_codec_chooser_get_active (GtkComboBox *chooser)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *name;

  g_return_val_if_fail (GTK_IS_COMBO_BOX (chooser), NULL);

  if (!gtk_combo_box_get_active_iter (chooser, &iter))
    return NULL;

  model = gtk_combo_box_get_model (chooser);
  gtk_tree_model_get (model, &iter, COL_CODEC_NAME, &name, -1);

  return name;
}

GType
ogmrip_codec_chooser_get_active_type (GtkComboBox *chooser)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GType type;

  g_return_val_if_fail (GTK_IS_COMBO_BOX (chooser), G_TYPE_NONE);

  if (!gtk_combo_box_get_active_iter (chooser, &iter))
    return G_TYPE_NONE;

  model = gtk_combo_box_get_model (chooser);
  gtk_tree_model_get (model, &iter, COL_CODEC_TYPE, &type, -1);

  return type;
}

static gboolean
ogmrip_drive_eject_idle (OGMDvdDrive *drive)
{
  ogmdvd_drive_eject (drive);

  return FALSE;
}

gboolean
ogmrip_open_title (GtkWindow *parent, OGMRipTitle *title)
{
  GError *error = NULL;
  OGMDvdMonitor *monitor;
  OGMDvdDrive *drive;
  GtkWidget *dialog;
  const gchar *uri;

  g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), FALSE);
  g_return_val_if_fail (OGMRIP_IS_TITLE (title), FALSE);

  if (ogmrip_title_is_open (title))
    return TRUE;

  if (ogmrip_title_open (title, &error))
    return TRUE;

  uri = ogmrip_media_get_uri (ogmrip_title_get_media (title));

  monitor = ogmdvd_monitor_get_default ();
  drive = ogmdvd_monitor_get_drive (monitor, uri + 6);
  g_object_unref (monitor);

  if (!drive)
    return FALSE;

  dialog = gtk_message_dialog_new_with_markup (parent,
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_INFO, GTK_BUTTONS_CANCEL, "<b>%s</b>\n\n%s",
      ogmrip_media_get_label (ogmrip_title_get_media (title)),
      _("Please insert the DVD required to encode this title."));

  g_signal_connect (dialog, "delete-event",
      G_CALLBACK (gtk_widget_hide_on_delete), NULL);
  g_signal_connect_swapped (drive, "medium-added",
      G_CALLBACK (gtk_dialog_response_accept), dialog);

  do
  {
    g_clear_error (&error);
    if (ogmrip_title_open (title, &error))
      break;

    if (error && error->code != OGMDVD_DISC_ERROR_ID)
      break;

    g_idle_add ((GSourceFunc) ogmrip_drive_eject_idle, drive);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_CANCEL)
      break;
  }
  while (1);

  g_object_unref (drive);
  gtk_widget_destroy (dialog);
  g_clear_error (&error);

  return ogmrip_title_is_open (title);
}

