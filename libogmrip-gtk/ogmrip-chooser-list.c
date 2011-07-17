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

/**
 * SECTION:ogmrip-chooser-list
 * @title: OGMRipChooserList
 * @include: ogmrip-chooser-list.h
 * @short_description: A widget that displays a list of source choosers
 */

#include "ogmrip-chooser-list.h"
#include "ogmrip-helper.h"

#include <glib/gi18n-lib.h>

#define OGMRIP_CHOOSER_LIST_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_CHOOSER_LIST, OGMRipChooserListPriv))

enum
{
  MORE_CLICKED,
  LAST_SIGNAL
};

struct _OGMRipChooserListPriv
{
  GType child_type;
  gint max;
};

typedef struct
{
  GtkWidget *chooser;
  GtkWidget *add_button;
  GtkWidget *rem_button;
  GtkWidget *more_button;
  GtkWidget *dialog;
} OGMRipChooserListItem;

static void ogmrip_chooser_list_dispose         (GObject      *gobject);
static void ogmrip_chooser_list_show            (GtkWidget    *widget);
static void ogmrip_chooser_list_add_internal    (GtkContainer *container,
                                                 GtkWidget    *widget);
static void ogmrip_chooser_list_remove_internal (GtkContainer *container,
                                                 GtkWidget    *widget);

static int signals[LAST_SIGNAL] = { 0 };

static void
ogmrip_chooser_list_chooser_changed (GtkWidget *chooser, OGMRipChooserListItem *item)
{
  OGMRipSource *source;
  OGMRipSourceType type;

  source = ogmrip_source_chooser_get_active (OGMRIP_SOURCE_CHOOSER (chooser), &type);
  gtk_widget_set_sensitive (item->more_button, type == OGMRIP_SOURCE_STREAM);
}

static GtkWidget *
ogmrip_chooser_list_child_new (OGMRipChooserList *list, GtkWidget *chooser)
{
  OGMRipChooserListItem *item;
  GtkWidget *child, *hbox, *image;

  child = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (child), chooser, TRUE, TRUE, 0);
  gtk_widget_show (child);

  item = g_new0 (OGMRipChooserListItem, 1);
  item->chooser = chooser;

  g_object_set_data_full (G_OBJECT (child), "__ogmrip_chooser_list_item__",
      item, (GDestroyNotify) g_free);

  hbox = gtk_hbox_new (TRUE, 6);
  gtk_box_pack_start (GTK_BOX (child), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  item->more_button = gtk_button_new_with_label ("...");
  gtk_box_pack_start (GTK_BOX (hbox), item->more_button, TRUE, TRUE, 0);
  gtk_widget_set_tooltip_text (item->more_button, _("More options"));
  gtk_widget_show (item->more_button);

  item->add_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), item->add_button, TRUE, TRUE, 0);
  gtk_widget_set_tooltip_text (item->add_button, _("Add a stream"));
  gtk_widget_show (item->add_button);

  image = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (item->add_button), image);
  gtk_widget_show (image);

  item->rem_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), item->rem_button, TRUE, TRUE, 0);
  gtk_widget_set_tooltip_text (item->rem_button, _("Remove the stream"));

  image = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (item->rem_button), image);
  gtk_widget_show (image);

  g_signal_connect (item->chooser, "changed",
      G_CALLBACK (ogmrip_chooser_list_chooser_changed), item);

  return child;
}

static GtkWidget *
ogmrip_chooser_list_child_get_add_button (GtkWidget *child)
{
  OGMRipChooserListItem *item;

  item = g_object_get_data (G_OBJECT (child), "__ogmrip_chooser_list_item__");
  
  return item->add_button;
}

static GtkWidget *
ogmrip_chooser_list_child_get_rem_button (GtkWidget *child)
{
  OGMRipChooserListItem *item;

  item = g_object_get_data (G_OBJECT (child), "__ogmrip_chooser_list_item__");

  return item->rem_button;
}

static GtkWidget *
ogmrip_chooser_list_child_get_more_button (GtkWidget *child)
{
  OGMRipChooserListItem *item;

  item = g_object_get_data (G_OBJECT (child), "__ogmrip_chooser_list_item__");
  
  return item->more_button;
}

G_DEFINE_TYPE (OGMRipChooserList, ogmrip_chooser_list, GTK_TYPE_VBOX)

static void
ogmrip_chooser_list_class_init (OGMRipChooserListClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  gobject_class = G_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);
  container_class = GTK_CONTAINER_CLASS (klass);

  gobject_class->dispose = ogmrip_chooser_list_dispose;
  widget_class->show = ogmrip_chooser_list_show;
  container_class->add = ogmrip_chooser_list_add_internal;
  container_class->remove = ogmrip_chooser_list_remove_internal;

  /**
   * OGMRipChooserList::more-clicked
   * @list: the widget that received the signal
   * @chooser: the selected source chooser
   *
   * Emitted each time a 'more' button is clicked
   */
  signals[MORE_CLICKED] = g_signal_new ("more-clicked", G_TYPE_FROM_CLASS (gobject_class), 
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipChooserListClass, more_clicked), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, OGMRIP_TYPE_SOURCE_CHOOSER);

  g_type_class_add_private (klass, sizeof (OGMRipChooserListPriv));
}

static void
ogmrip_chooser_list_init (OGMRipChooserList *list)
{
  list->priv = OGMRIP_CHOOSER_LIST_GET_PRIVATE (list);

  list->priv->max = -1;

  gtk_box_set_spacing (GTK_BOX (list), 6);
}

static void
ogmrip_chooser_list_add_clicked (OGMRipChooserList *list, GtkWidget *button)
{
  GtkWidget *child;

  child = g_object_new (list->priv->child_type, NULL);
  gtk_container_add (GTK_CONTAINER (list), child);
  gtk_widget_show (child);
}

static void
ogmrip_chooser_list_remove_clicked (OGMRipChooserList *list, GtkWidget *button)
{
  gtk_container_remove (GTK_CONTAINER (list),
      gtk_widget_get_parent (gtk_widget_get_parent (button)));
}

static void
ogmrip_chooser_list_more_clicked (OGMRipChooserList *list, GtkWidget *button)
{
  OGMRipChooserListItem *item;
  GtkWidget *parent;

  parent = gtk_widget_get_parent (gtk_widget_get_parent (button));

  item = g_object_get_data (G_OBJECT (parent), "__ogmrip_chooser_list_item__");
  if (item)
    g_signal_emit (G_OBJECT (list), signals[MORE_CLICKED], 0, item->chooser);
}

static void
ogmrip_chooser_list_dispose (GObject *gobject)
{
  OGMRipChooserList *list;
  
  list = OGMRIP_CHOOSER_LIST (gobject);
/*
  if (list->priv->client)
    g_object_unref (list->priv->client);
  list->priv->client = NULL;
*/
  G_OBJECT_CLASS (ogmrip_chooser_list_parent_class)->dispose (gobject);
}

static void
ogmrip_chooser_list_show (GtkWidget *widget)
{
  if (widget)
  {
    GtkWidget *parent;

    parent = gtk_widget_get_parent (widget);
    if (parent)
      gtk_widget_show (parent);
  }

  (*GTK_WIDGET_CLASS (ogmrip_chooser_list_parent_class)->show) (widget);
}

static void
ogmrip_chooser_list_add_internal (GtkContainer *container, GtkWidget *chooser)
{
  OGMRipChooserList *list;
  guint len;

  list = OGMRIP_CHOOSER_LIST (container);

  if (G_TYPE_CHECK_INSTANCE_TYPE (chooser, list->priv->child_type))
  {
    GList *children;

    children = gtk_container_get_children (container);
    len = g_list_length (children);
    g_list_free (children);

    if (list->priv->max < 0 || len < list->priv->max)
    {
      GtkWidget *child, *button;

      child = gtk_box_get_nth_child (GTK_BOX (container), -1);
      if (child)
      {
        button = ogmrip_chooser_list_child_get_add_button (child);
        gtk_widget_hide (button);

        button = ogmrip_chooser_list_child_get_rem_button (child);
        gtk_widget_show (button);
      }

      child = ogmrip_chooser_list_child_new (list, chooser);

      button = ogmrip_chooser_list_child_get_add_button (child);
      g_signal_connect_swapped (button, "clicked", 
          G_CALLBACK (ogmrip_chooser_list_add_clicked), list);
      if (list->priv->max > 0 && len + 1 == list->priv->max)
        gtk_widget_set_sensitive (button, FALSE);

      button = ogmrip_chooser_list_child_get_rem_button (child);
      g_signal_connect_swapped (button, "clicked", 
          G_CALLBACK (ogmrip_chooser_list_remove_clicked), list);

      button = ogmrip_chooser_list_child_get_more_button (child);
      g_signal_connect_swapped (button, "clicked", 
          G_CALLBACK (ogmrip_chooser_list_more_clicked), list);

      (*GTK_CONTAINER_CLASS (ogmrip_chooser_list_parent_class)->add) (container, child);
    }
  }
}

static void
ogmrip_chooser_list_remove_internal (GtkContainer *container, GtkWidget *child)
{
  GtkWidget *last;

  (*GTK_CONTAINER_CLASS (ogmrip_chooser_list_parent_class)->remove) (container, child);

  last = gtk_box_get_nth_child (GTK_BOX (container), -1);
  if (last)
  {
    GtkWidget *button;

    button = ogmrip_chooser_list_child_get_add_button (last);
    gtk_widget_show (button);

    button = ogmrip_chooser_list_child_get_rem_button (last);
    gtk_widget_hide (button);
  }
}

/**
 * ogmrip_chooser_list_new:
 * @type: the type of the children
 *
 * Creates a new #OGMRipChooserList.
 *
 * Returns: The new #OGMRipChooserList
 */
GtkWidget *
ogmrip_chooser_list_new (GType type)
{
  OGMRipChooserList *list;
/*
  g_return_val_if_fail (!G_TYPE_IS_INSTANTIATABLE (type), NULL);
  g_return_val_if_fail (g_type_is_a (type, OGMRIP_TYPE_SOURCE_CHOOSER), NULL);
*/
  list = g_object_new (OGMRIP_TYPE_CHOOSER_LIST, NULL);
  list->priv->child_type = type;

  return GTK_WIDGET (list);
}

/**
 * ogmrip_chooser_list_set_max:
 * @list: An #OGMRipChooserList
 * @max: the maximum number of children
 *
 * Creates a new #OGMRipChooserList.
 */
void
ogmrip_chooser_list_set_max (OGMRipChooserList *list, guint max)
{
  GList *link, *children;
  GtkWidget *button, *child;
  guint i;

  g_return_if_fail (OGMRIP_IS_CHOOSER_LIST (list));

  list->priv->max = MAX (max, 1);

  children = gtk_container_get_children (GTK_CONTAINER (list));
  for (i = 0, link = children; link; i ++, link = link->next)
  {
    child = link->data;

    if (i >= max)
      gtk_container_remove (GTK_CONTAINER (list), child);
    else
    {
      button = ogmrip_chooser_list_child_get_add_button (child);
      gtk_widget_set_sensitive (button, max < 0 || i < max - 1);
    }
  }
  g_list_free (children);
}

/**
 * ogmrip_chooser_list_get_max:
 * @list: An #OGMRipChooserList
 *
 * Returns the maximum number of children.
 *
 * Returns: the maximum number of children, or -1
 */
gint
ogmrip_chooser_list_get_max (OGMRipChooserList *list)
{
  g_return_val_if_fail (OGMRIP_IS_CHOOSER_LIST (list), -1);

  return list->priv->max;
}

/**
 * ogmrip_chooser_list_add:
 * @list: An #OGMRipChooserList
 * @chooser: A chooser to be placed inside @list
 *
 * Adds @chooser to @list.
 */
void
ogmrip_chooser_list_add (OGMRipChooserList *list, GtkWidget *chooser)
{
  g_return_if_fail (OGMRIP_CHOOSER_LIST (list));
  g_return_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (chooser, list->priv->child_type));

  gtk_container_add (GTK_CONTAINER (list), chooser);
}

/**
 * ogmrip_chooser_list_remove:
 * @list: An #OGMRipChooserList
 * @chooser: Â current child of @list
 *
 * Removes @chooser from @list.
 */
void
ogmrip_chooser_list_remove (OGMRipChooserList *list, GtkWidget *chooser)
{
  GtkWidget *parent;

  g_return_if_fail (OGMRIP_CHOOSER_LIST (list));
  g_return_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (chooser, list->priv->child_type));

  parent = gtk_widget_get_parent (chooser);
  if (parent)
    gtk_container_remove (GTK_CONTAINER (list), parent);
}

/**
 * ogmrip_chooser_list_clear:
 * @list: An #OGMRipChooserList
 *
 * Removes all children of @list.
 */
void
ogmrip_chooser_list_clear (OGMRipChooserList *list)
{
  GList *link, *children;

  g_return_if_fail (OGMRIP_IS_CHOOSER_LIST (list));

  children = gtk_container_get_children (GTK_CONTAINER (list));
  for (link = children; link; link = link->next)
    gtk_container_remove (GTK_CONTAINER (list), GTK_WIDGET (link->data));
  g_list_free (children);
}

/**
 * ogmrip_chooser_list_foreach:
 * @list: An #OGMRipChooserList
 * @type: The type of the children
 * @func: A callback
 * @data: Callback user data
 *
 * Invokes @func on each non-internal @type child of @list.
 */
void
ogmrip_chooser_list_foreach (OGMRipChooserList *list, OGMRipSourceType type, GFunc func, gpointer data)
{
  GList *children, *link;
  gint source_type;

  OGMRipSource *source;
  OGMRipChooserListItem *item;

  g_return_if_fail (OGMRIP_IS_CHOOSER_LIST (list));

  children = gtk_container_get_children (GTK_CONTAINER (list));
  for (link = children; link; link = link->next)
  {
    item = g_object_get_data (G_OBJECT (link->data), "__ogmrip_chooser_list_item__");

    source = ogmrip_source_chooser_get_active (OGMRIP_SOURCE_CHOOSER (item->chooser), &source_type);

    if (type == OGMRIP_SOURCE_STREAM)
    {
      if (source_type == OGMRIP_SOURCE_STREAM)
        (* func) (item->chooser, data);
    }
    else if (type == OGMRIP_SOURCE_FILE)
    {
      if (source_type == OGMRIP_SOURCE_FILE)
        (* func) (item->chooser, data);
    }
  }
  g_list_free (children);
}

/**
 * ogmrip_chooser_list_length:
 * @list: An #OGMRipChooserList
 *
 * Returns the number of valid source choosers contained in @list.
 *
 * Returns: The number of valid choosers, or -1
 */
gint
ogmrip_chooser_list_length (OGMRipChooserList *list)
{
  GList *children, *link;
  gint n_children = 0;

  OGMRipChooserListItem *item;

  g_return_val_if_fail (OGMRIP_IS_CHOOSER_LIST (list), -1);

  children = gtk_container_get_children (GTK_CONTAINER (list));
  for (link = children; link; link = link->next)
  {
    item = g_object_get_data (G_OBJECT (link->data), "__ogmrip_chooser_list_item__");

    if (ogmrip_source_chooser_get_active (OGMRIP_SOURCE_CHOOSER (item->chooser), NULL))
      n_children ++;
  }
  g_list_free (children);

  return n_children;
}

/**
 * ogmrip_chooser_list_nth:
 * @list: An #OGMRipChooserList
 * @n: the position of the chooser, counting from 0
 *
 * Gets the chooser at the given position.
 *
 * Returns: the chooser, or NULL
 */
GtkWidget *
ogmrip_chooser_list_nth (OGMRipChooserList *list, guint n)
{
  OGMRipChooserListItem *item;
  GtkWidget *child;

  g_return_val_if_fail (OGMRIP_IS_CHOOSER_LIST (list), NULL);

  child = gtk_box_get_nth_child (GTK_BOX (list), n);
  if (!child)
    return NULL;

  item = g_object_get_data (G_OBJECT (child), "__ogmrip_chooser_list_item__");
  if (!item)
    return NULL;

  return item->chooser;
}

