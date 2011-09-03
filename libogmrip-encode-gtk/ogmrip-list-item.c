/* OGMRip - A library for DVD ripping and encoding
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

/**
 * SECTION:ogmrip-list-item
 * @title: OGMRipListItem
 * @include: ogmrip-list-item.h
 * @short_description: List item
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-list-item.h"

#include <glib/gi18n-lib.h>

#define OGMRIP_LIST_ITEM_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_LIST_ITEM, OGMRipListItemPriv))

struct _OGMRipListItemPriv
{
  GtkWidget *add_button;
  GtkWidget *remove_button;
  GtkSizeGroup *group;
  gchar *add_tooltip;
  gchar *remove_tooltip;
};

enum
{
  PROP_0,
  PROP_ADD_TOOLTIP,
  PROP_REMOVE_TOOLTIP
};

enum
{
  ADD,
  REMOVE,
  N_SIGNALS
};

static void ogmrip_list_item_constructed  (GObject      *gobject);
static void ogmrip_list_item_finalize     (GObject      *gobject);
static void ogmrip_list_item_get_property (GObject      *gobject,
                                           guint        param_id,
                                           GValue       *value,
                                           GParamSpec   *pspec);
static void ogmrip_list_item_set_property (GObject      *gobject,
                                           guint        param_id,
                                           const GValue *value,
                                           GParamSpec   *pspec);

static int signals[N_SIGNALS] = { 0 };

static void
ogmrip_list_item_add_button_clicked_cb (OGMRipListItem *item)
{
  gtk_widget_hide (item->priv->add_button);
  gtk_widget_show (item->priv->remove_button);

  g_signal_emit (G_OBJECT (item), signals[ADD], 0);
}

static void
ogmrip_list_item_rem_button_clicked_cb (OGMRipListItem *item)
{
  gtk_widget_show (item->priv->add_button);
  gtk_widget_hide (item->priv->remove_button);

  g_signal_emit (G_OBJECT (item), signals[REMOVE], 0);
}

G_DEFINE_TYPE (OGMRipListItem, ogmrip_list_item, GTK_TYPE_HBOX)

static void
ogmrip_list_item_class_init (OGMRipListItemClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_list_item_constructed;
  gobject_class->finalize = ogmrip_list_item_finalize;
  gobject_class->get_property = ogmrip_list_item_get_property;
  gobject_class->set_property = ogmrip_list_item_set_property;

  g_object_class_install_property (gobject_class, PROP_ADD_TOOLTIP,
      g_param_spec_string ("add-tooltip", "add-tooltip", "add-tooltip",
        NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_REMOVE_TOOLTIP,
      g_param_spec_string ("remove-tooltip", "remove-tooltip", "remove-tooltip",
        NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  signals[ADD] = g_signal_new ("add-clicked", G_TYPE_FROM_CLASS (klass), 
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipListItemClass, add_clicked), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  signals[REMOVE] = g_signal_new ("remove-clicked", G_TYPE_FROM_CLASS (klass), 
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipListItemClass, remove_clicked), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  g_type_class_add_private (klass, sizeof (OGMRipListItemPriv));
}

static void
ogmrip_list_item_init (OGMRipListItem *item)
{
  item->priv = OGMRIP_LIST_ITEM_GET_PRIVATE (item);

  item->priv->group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
}

static void
ogmrip_list_item_constructed (GObject *gobject)
{
  OGMRipListItem *item = OGMRIP_LIST_ITEM (gobject);
  GtkWidget *hbox, *image;

  if (!item->priv->add_tooltip)
    item->priv->add_tooltip = g_strdup (_("Add an item"));

  if (!item->priv->remove_tooltip)
    item->priv->remove_tooltip = g_strdup (_("Remove the item"));

  hbox = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_end (GTK_BOX (item), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  item->priv->add_button = gtk_button_new ();
  gtk_widget_set_no_show_all (item->priv->add_button, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), item->priv->add_button, FALSE, FALSE, 0);
  gtk_widget_set_tooltip_text (item->priv->add_button, item->priv->add_tooltip);
  gtk_widget_show (item->priv->add_button);

  gtk_size_group_add_widget (item->priv->group, item->priv->add_button);

  g_signal_connect_swapped (item->priv->add_button, "clicked",
      G_CALLBACK (ogmrip_list_item_add_button_clicked_cb), item);

  image = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (item->priv->add_button), image);
  gtk_widget_show (image);

  item->priv->remove_button = gtk_button_new ();
  gtk_widget_set_no_show_all (item->priv->remove_button, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), item->priv->remove_button, FALSE, FALSE, 0);
  gtk_widget_set_tooltip_text (item->priv->remove_button, item->priv->remove_tooltip);

  gtk_size_group_add_widget (item->priv->group, item->priv->remove_button);

  g_signal_connect_swapped (item->priv->remove_button, "clicked",
      G_CALLBACK (ogmrip_list_item_rem_button_clicked_cb), item);

  image = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (item->priv->remove_button), image);
  gtk_widget_show (image);

  G_OBJECT_CLASS (ogmrip_list_item_parent_class)->constructed (gobject);
}

static void
ogmrip_list_item_finalize (GObject *gobject)
{
  OGMRipListItem *list = OGMRIP_LIST_ITEM (gobject);

  g_free (list->priv->add_tooltip);
  g_free (list->priv->remove_tooltip);

  G_OBJECT_CLASS (ogmrip_list_item_parent_class)->finalize (gobject);
}

static void
ogmrip_list_item_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  OGMRipListItem *list = OGMRIP_LIST_ITEM (gobject);

  switch (prop_id)
  {
    case PROP_ADD_TOOLTIP:
      g_value_set_string (value, list->priv->add_tooltip);
      break;
    case PROP_REMOVE_TOOLTIP:
      break;
      g_value_set_string (value, list->priv->remove_tooltip);
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_list_item_set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipListItem *list = OGMRIP_LIST_ITEM (gobject);

  switch (prop_id)
  {
    case PROP_ADD_TOOLTIP:
      list->priv->add_tooltip = g_value_dup_string (value);
      break;
    case PROP_REMOVE_TOOLTIP:
      list->priv->remove_tooltip = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

GtkWidget *
ogmrip_list_item_new (void)
{
  return g_object_new (OGMRIP_TYPE_LIST_ITEM, NULL);
}

GtkSizeGroup *
ogmrip_list_item_get_size_group (OGMRipListItem *list)
{
  g_return_val_if_fail (OGMRIP_IS_LIST_ITEM (list), NULL);

  return list->priv->group;
}

