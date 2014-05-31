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

#include "ogmrip-type-chooser-widget.h"

#include <ogmrip-encode.h>

static gboolean
ogmrip_type_chooser_widget_filter (GtkTreeModel *model, GtkTreeIter *iter, GtkTreeModelFilter *filter)
{
  GType *container, gtype;

  container = g_object_get_data (G_OBJECT (filter), "filter-type");
  if (!container)
    return TRUE;

  gtype = ogmrip_type_store_get_gtype (OGMRIP_TYPE_STORE (model), iter);

  if (gtype == G_TYPE_NONE)
    return FALSE;

  if (gtype == OGMRIP_TYPE_HARDSUB)
    return TRUE;

  return ogmrip_container_contains (*container, ogmrip_codec_format (gtype));
}

void
ogmrip_type_chooser_widget_construct (GtkComboBox *chooser, GType gtype)
{
  OGMRipTypeStore *store;
  GtkTreeModel *filter;
  GtkCellRenderer *cell;

  g_return_if_fail (GTK_IS_COMBO_BOX (chooser));

  store = ogmrip_type_store_new (gtype);
  filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (store), NULL);
  g_object_unref (store);

  gtk_combo_box_set_model (chooser, GTK_TREE_MODEL (filter));
  g_object_unref (filter);

  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter),
      (GtkTreeModelFilterVisibleFunc) ogmrip_type_chooser_widget_filter, filter, NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser),
      cell, "text", OGMRIP_TYPE_STORE_DESCRIPTION_COLUMN, NULL);
}

GType
ogmrip_type_chooser_widget_get_active (GtkComboBox *chooser)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GType gtype;

  g_return_val_if_fail (GTK_IS_COMBO_BOX (chooser), G_TYPE_NONE);

  if (!gtk_combo_box_get_active_iter (chooser, &iter))
    return G_TYPE_NONE;

  model = gtk_combo_box_get_model (chooser);
  gtk_tree_model_get (model, &iter, OGMRIP_TYPE_STORE_TYPE_COLUMN, &gtype, -1);

  return gtype;
}

void
ogmrip_type_chooser_widget_set_active (GtkComboBox *chooser, GType gtype)
{
  g_return_if_fail (GTK_IS_COMBO_BOX (chooser));

  if (gtype == G_TYPE_NONE)
    gtk_combo_box_set_active (chooser, -1);
  else
  {
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_combo_box_get_model (chooser);
    if (gtk_tree_model_get_iter_first (model, &iter))
    {
      GType current_gtype;

      do
      {
        gtk_tree_model_get (model, &iter, OGMRIP_TYPE_STORE_TYPE_COLUMN, &current_gtype, -1);
        if (current_gtype == gtype)
        {
          gtk_combo_box_set_active_iter (chooser, &iter);
          break;
        }
      }
      while (gtk_tree_model_iter_next (model, &iter));

      if (gtk_combo_box_get_active (chooser) < 0)
        gtk_combo_box_set_active (chooser, 0);
    }
  }
}

void
ogmrip_type_chooser_widget_set_filter (GtkComboBox *chooser, GType gtype)
{
  GtkTreeModel *filter;
  GType *container;

  g_return_if_fail (GTK_IS_COMBO_BOX (chooser));
  g_return_if_fail (g_type_is_a (gtype, OGMRIP_TYPE_CONTAINER));

  container = g_new0 (GType, 1);
  *container = gtype;

  filter = gtk_combo_box_get_model (chooser);
  g_object_set_data_full (G_OBJECT (filter), "filter-type", container, (GDestroyNotify) g_free);

  gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (filter));
}

