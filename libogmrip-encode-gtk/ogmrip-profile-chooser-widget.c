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

#include "ogmrip-profile-chooser-widget.h"

struct _OGMRipProfileChooserWidgetPriv
{
  OGMRipProfileStore *store;
};

enum
{
  PROP_0,
  PROP_PROFILE
};

G_DEFINE_TYPE_WITH_PRIVATE (OGMRipProfileChooserWidget, ogmrip_profile_chooser_widget, GTK_TYPE_COMBO_BOX);

static void
ogmrip_profile_chooser_widget_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_PROFILE:
      g_value_set_object (value, ogmrip_profile_chooser_widget_get_active (OGMRIP_PROFILE_CHOOSER_WIDGET (gobject)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_profile_chooser_widget_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_PROFILE:
      ogmrip_profile_chooser_widget_set_active (OGMRIP_PROFILE_CHOOSER_WIDGET (gobject), g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_profile_chooser_widget_dispose (GObject *gobject)
{
  OGMRipProfileChooserWidget *chooser = OGMRIP_PROFILE_CHOOSER_WIDGET (gobject);

  if (chooser->priv->store)
  {
    g_object_unref (chooser->priv->store);
    chooser->priv->store = NULL;
  }

  G_OBJECT_CLASS (ogmrip_profile_chooser_widget_parent_class)->dispose (gobject);
}

static void
ogmrip_profile_chooser_widget_changed (GtkComboBox *combo_box)
{
  g_object_notify (G_OBJECT (combo_box), "profile");
}

static void
ogmrip_profile_chooser_widget_class_init (OGMRipProfileChooserWidgetClass *klass)
{
  GObjectClass *gobject_class;
  GtkComboBoxClass *combo_box_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_profile_chooser_widget_get_property;
  gobject_class->set_property = ogmrip_profile_chooser_widget_set_property;
  gobject_class->dispose      = ogmrip_profile_chooser_widget_dispose;

  combo_box_class = GTK_COMBO_BOX_CLASS (klass);
  combo_box_class->changed = ogmrip_profile_chooser_widget_changed;

  g_object_class_install_property (gobject_class, PROP_PROFILE,
      g_param_spec_object ("profile", "Profile property", "Profile property",
        OGMRIP_TYPE_PROFILE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_profile_chooser_widget_init (OGMRipProfileChooserWidget *chooser)
{
  GtkCellRenderer *cell;

  chooser->priv = ogmrip_profile_chooser_widget_get_instance_private (chooser);

  chooser->priv->store = ogmrip_profile_store_new (NULL, TRUE);
  gtk_combo_box_set_model (GTK_COMBO_BOX (chooser), GTK_TREE_MODEL (chooser->priv->store));

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser), cell, "markup", OGMRIP_PROFILE_STORE_INFO_COLUMN, NULL);
}

GtkWidget *
ogmrip_profile_chooser_widget_new (void)
{
  return g_object_new (OGMRIP_TYPE_PROFILE_CHOOSER_WIDGET, NULL);
}

OGMRipProfile *
ogmrip_profile_chooser_widget_get_active (OGMRipProfileChooserWidget *chooser)
{
  GtkTreeIter iter;

  g_return_val_if_fail (OGMRIP_IS_PROFILE_CHOOSER_WIDGET (chooser), NULL);

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (chooser), &iter))
    return NULL;

  return ogmrip_profile_store_get_profile (chooser->priv->store, &iter);
}

void
ogmrip_profile_chooser_widget_set_active (OGMRipProfileChooserWidget *chooser, OGMRipProfile *profile)
{
  g_return_if_fail (OGMRIP_IS_PROFILE_CHOOSER_WIDGET (chooser));
  g_return_if_fail (profile == NULL || OGMRIP_IS_PROFILE (profile));

  if (!profile)
    gtk_combo_box_set_active (GTK_COMBO_BOX (chooser), -1);
  else
  {
    GtkTreeIter iter;

    if (ogmrip_profile_store_get_iter (chooser->priv->store, &iter, profile))
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (chooser), &iter);
    else
      gtk_combo_box_set_active (GTK_COMBO_BOX (chooser), -1);
  }
}

