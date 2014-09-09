/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2014 Olivier Rolland <billl@users.sourceforge.net>
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
 * SECTION:ogmrip-title-chooser-widget
 * @title: OGMRipTitleChooserWidget
 * @include: ogmrip-title-chooser-widget.h
 * @short_description: Title chooser widget that can be embedded in other widgets
 */

#include "ogmrip-title-chooser-widget.h"

#include <glib/gi18n-lib.h>

enum
{
  PROP_0,
  PROP_MEDIA
};

enum
{
  TEXT_COLUMN,
  ID_COLUMN,
  NUM_COLUMNS
};

struct _OGMRipTitleChooserWidgetPriv
{
  OGMRipMedia *media;
};

static void ogmrip_title_chooser_init (OGMRipTitleChooserInterface *iface);

static void
ogmrip_title_chooser_widget_set_media (OGMRipTitleChooserWidget *chooser, OGMRipMedia *media)
{
  GtkTreeModel *model;

  if (media)
    g_object_ref (media);
  if (chooser->priv->media)
    g_object_unref (chooser->priv->media);
  chooser->priv->media = media;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
  gtk_list_store_clear (GTK_LIST_STORE (model));

  if (!media)
    gtk_combo_box_set_active (GTK_COMBO_BOX (chooser), -1);
  else
  {
    GtkTreeIter iter;
    GList *list, *link;

    OGMRipVideoStream *stream;
    OGMRipTime time_;
    GString *string;

    gint standard;
    glong length, longest = 0;
    guint num, denom;

    list = ogmrip_media_get_titles (media);
    for (link = list; link; link = link->next)
    {
      stream = ogmrip_title_get_video_stream (link->data);

      string = g_string_new (NULL);
      g_string_printf (string, "%s %02d", _("Title"), ogmrip_title_get_id (link->data) + 1);

      length = ogmrip_title_get_length (link->data, &time_);
      if (length > 0)
      {
        if (time_.hour > 0)
          g_string_append_printf (string, " (%02lu:%02lu %s", time_.hour, time_.min, _("hours"));
        else if (time_.min > 0)
          g_string_append_printf (string, " (%02lu:%02lu %s", time_.min, time_.sec, _("minutes"));
        else
          g_string_append_printf (string, " (%02lu %s", time_.sec, _("seconds"));

        standard = ogmrip_video_stream_get_standard (stream);
        if (standard != OGMRIP_STANDARD_UNDEFINED)
          g_string_append_printf (string, ", %s", ogmrip_standard_get_label (standard));

        ogmrip_video_stream_get_aspect_ratio (stream, &num, &denom);
        if (num > 0 && denom > 0)
        {
          if (denom == 1000)
          {
            gchar aspect[G_ASCII_DTOSTR_BUF_SIZE];

            g_string_append_printf (string, ", %s:1", g_ascii_formatd (aspect, G_ASCII_DTOSTR_BUF_SIZE, "%.02lf", num / 1000.));
          }
          else
            g_string_append_printf (string, ", %u/%u", num, denom);
        }

        g_string_append_c (string, ')');
      }

      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter, TEXT_COLUMN, string->str,
          ID_COLUMN, ogmrip_title_get_id (link->data), -1);
      g_string_free (string, TRUE);

      if (length <= 0 || length > longest)
      {
        longest = length;
        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (chooser), &iter);
      }
    }
    g_list_free (list);
  }
}

static OGMRipTitle *
ogmrip_title_chooser_widget_get_active (OGMRipTitleChooser *chooser)
{
  OGMRipTitleChooserWidget *widget = OGMRIP_TITLE_CHOOSER_WIDGET (chooser);
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint id;

  if (!widget->priv->media)
    return NULL;

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter))
    return NULL;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
  gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);

  return ogmrip_media_get_title (widget->priv->media, id);
}

static void
ogmrip_title_chooser_widget_set_active (OGMRipTitleChooser *chooser, OGMRipTitle *title)
{
  OGMRipTitleChooserWidget *widget = OGMRIP_TITLE_CHOOSER_WIDGET (chooser);
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint id1, id2;

  id1 = ogmrip_title_get_id (title);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    do
    {
      gtk_tree_model_get (model, &iter, ID_COLUMN, &id2, -1);
    }
    while (id1 != id2 && gtk_tree_model_iter_next (model, &iter));

    if (id2 == id1)
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
  }
}

G_DEFINE_TYPE_WITH_CODE (OGMRipTitleChooserWidget, ogmrip_title_chooser_widget, GTK_TYPE_COMBO_BOX,
    G_ADD_PRIVATE (OGMRipTitleChooserWidget)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_TITLE_CHOOSER, ogmrip_title_chooser_init))

static void
ogmrip_title_chooser_widget_dispose (GObject *object)
{
  OGMRipTitleChooserWidget *chooser = OGMRIP_TITLE_CHOOSER_WIDGET (object);

  g_clear_object (&chooser->priv->media);

  G_OBJECT_CLASS (ogmrip_title_chooser_widget_parent_class)->dispose (object);
}

static void
ogmrip_title_chooser_widget_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipTitleChooserWidget *chooser = OGMRIP_TITLE_CHOOSER_WIDGET (gobject);

  switch (property_id) 
  {
    case PROP_MEDIA:
      g_value_set_object (value, chooser->priv->media);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_title_chooser_widget_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipTitleChooserWidget *chooser = OGMRIP_TITLE_CHOOSER_WIDGET (gobject);

  switch (property_id) 
  {
    case PROP_MEDIA:
      ogmrip_title_chooser_widget_set_media (chooser, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_title_chooser_widget_class_init (OGMRipTitleChooserWidgetClass *klass)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *) klass;
  object_class->dispose = ogmrip_title_chooser_widget_dispose;
  object_class->get_property = ogmrip_title_chooser_widget_get_property;
  object_class->set_property = ogmrip_title_chooser_widget_set_property;

  g_object_class_override_property (object_class, PROP_MEDIA, "media");
}

static void
ogmrip_title_chooser_init (OGMRipTitleChooserInterface *iface)
{
  iface->get_active = ogmrip_title_chooser_widget_get_active;
  iface->set_active = ogmrip_title_chooser_widget_set_active;
}

static void
ogmrip_title_chooser_widget_init (OGMRipTitleChooserWidget *chooser)
{
  GtkCellRenderer *cell;
  GtkListStore *store;

  chooser->priv = ogmrip_title_chooser_widget_get_instance_private (chooser);

  store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_INT);
  gtk_combo_box_set_model (GTK_COMBO_BOX (chooser), GTK_TREE_MODEL (store));
  g_object_unref (store);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser), cell, "text", TEXT_COLUMN, NULL);
}

/**
 * ogmrip_title_chooser_widget_new:
 *
 * Creates a new #OGMRipTitleChooserWidget.
 *
 * Returns: The new #OGMRipTitleChooserWidget
 */
GtkWidget *
ogmrip_title_chooser_widget_new (void)
{
  return g_object_new (OGMRIP_TYPE_TITLE_CHOOSER_WIDGET, NULL);
}

