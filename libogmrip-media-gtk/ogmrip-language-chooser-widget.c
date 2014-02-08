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

#include "ogmrip-language-chooser-widget.h"

struct _OGMRipLanguageChooserWidgetPriv
{
  OGMRipLanguageStore *store;
};

enum
{
  PROP_0,
  PROP_LANG
};

G_DEFINE_TYPE_WITH_PRIVATE (OGMRipLanguageChooserWidget, ogmrip_language_chooser_widget, GTK_TYPE_COMBO_BOX);

static void
ogmrip_language_chooser_widget_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_LANG:
      g_value_set_uint (value, ogmrip_language_chooser_widget_get_active (OGMRIP_LANGUAGE_CHOOSER_WIDGET (gobject)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_language_chooser_widget_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_LANG:
      ogmrip_language_chooser_widget_set_active (OGMRIP_LANGUAGE_CHOOSER_WIDGET (gobject), g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
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

static void
ogmrip_language_chooser_widget_changed (GtkComboBox *combo_box)
{
  g_object_notify (G_OBJECT (combo_box), "language");
}

static void
ogmrip_language_chooser_widget_class_init (OGMRipLanguageChooserWidgetClass *klass)
{
  GObjectClass *gobject_class;
  GtkComboBoxClass *combo_box_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_language_chooser_widget_get_property;
  gobject_class->set_property = ogmrip_language_chooser_widget_set_property;
  gobject_class->dispose      = ogmrip_language_chooser_widget_dispose;

  combo_box_class = GTK_COMBO_BOX_CLASS (klass);
  combo_box_class->changed = ogmrip_language_chooser_widget_changed;

  g_object_class_install_property (gobject_class, PROP_LANG,
      g_param_spec_uint ("language", "Language property", "Language property",
        0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_language_chooser_widget_init (OGMRipLanguageChooserWidget *chooser)
{
  GtkCellRenderer *cell;

  chooser->priv = ogmrip_language_chooser_widget_get_instance_private (chooser);

  chooser->priv->store = ogmrip_language_store_new ();
  gtk_combo_box_set_model (GTK_COMBO_BOX (chooser), GTK_TREE_MODEL (chooser->priv->store));

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser), cell, "text", OGMRIP_LANGUAGE_STORE_NAME_COLUMN, NULL);
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

