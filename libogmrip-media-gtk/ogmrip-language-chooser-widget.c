/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-language-chooser-widget.h"
#include "ogmrip-language-store.h"

struct _OGMRipLanguageChooserWidgetPriv
{
  OGMRipLanguageStore *store;
};

static void ogmrip_language_chooser_widget_dispose (GObject *gobject);

G_DEFINE_TYPE (OGMRipLanguageChooserWidget, ogmrip_language_chooser_widget, GTK_TYPE_COMBO_BOX);

static void
ogmrip_language_chooser_widget_class_init (OGMRipLanguageChooserWidgetClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_language_chooser_widget_dispose;

  g_type_class_add_private (klass, sizeof (OGMRipLanguageChooserWidgetPriv));
}

static void
ogmrip_language_chooser_widget_init (OGMRipLanguageChooserWidget *chooser)
{
  GtkCellRenderer *cell;

  chooser->priv = G_TYPE_INSTANCE_GET_PRIVATE (chooser, OGMRIP_TYPE_LANGUAGE_CHOOSER_WIDGET, OGMRipLanguageChooserWidgetPriv);

  chooser->priv->store = ogmrip_language_store_new ();
  gtk_combo_box_set_model (GTK_COMBO_BOX (chooser), GTK_TREE_MODEL (chooser->priv->store));

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser), cell, "text", OGMRIP_LANGUAGE_STORE_NAME_COLUMN, NULL);
}

static void
ogmrip_language_chooser_widget_dispose (GObject *gobject)
{
  OGMRipLanguageChooserWidget *chooser = OGMRIP_LANGUAGE_CHOOSER_WIDGET (gobject);

  if (chooser->priv->store)
  {
    g_object_unref (chooser->priv->store);
    chooser->priv->store = NULL;
  }

  G_OBJECT_CLASS (ogmrip_language_chooser_widget_parent_class)->dispose (gobject);
}

GtkWidget *
ogmrip_language_chooser_widget_new (void)
{
  return g_object_new (OGMRIP_TYPE_LANGUAGE_CHOOSER_WIDGET, NULL);
}

guint
ogmrip_language_chooser_widget_get_active (OGMRipLanguageChooserWidget *chooser)
{
  GtkTreeIter iter;
  guint code;

  g_return_val_if_fail (OGMRIP_IS_LANGUAGE_CHOOSER_WIDGET (chooser), 0);

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (chooser), &iter))
    return 0;

  gtk_tree_model_get (GTK_TREE_MODEL (chooser->priv->store), &iter,
      OGMRIP_LANGUAGE_STORE_CODE_COLUMN, &code, -1);

  return code;
}

void
ogmrip_language_chooser_widget_set_active (OGMRipLanguageChooserWidget *chooser, guint code)
{
  GtkTreeIter iter;

  g_return_if_fail (OGMRIP_IS_LANGUAGE_CHOOSER_WIDGET (chooser));

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (chooser->priv->store), &iter))
  {
    guint current_code;

    do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (chooser->priv->store), &iter,
          OGMRIP_LANGUAGE_STORE_CODE_COLUMN, &current_code, -1);

      if (current_code == code)
      {
        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (chooser), &iter);
        break;
      }
    }
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL (chooser->priv->store), &iter));
  }
}

