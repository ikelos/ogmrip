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
 * SECTION:ogmdvd-title-chooser-widget
 * @title: OGMDvdTitleChooserWidget
 * @include: ogmdvd-title-chooser-widget.h
 * @short_description: DVD title chooser widget that can be embedded in other widgets
 */

#include "ogmdvd-title-chooser-widget.h"

#include <glib/gi18n.h>

#define OGMDVD_TITLE_CHOOSER_WIDGET_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMDVD_TYPE_TITLE_CHOOSER_WIDGET, OGMDvdTitleChooserWidgetPriv))

enum
{
  PROP_0,
  PROP_DISC,
  PROP_TITLE
};

enum
{
  TEXT_COLUMN,
  NR_COLUMN,
  NUM_COLUMNS
};

struct _OGMDvdTitleChooserWidgetPriv
{
  OGMDvdDisc *disc;
  OGMDvdTitle *title;
};

/*
 * GObject funcs
 */

static void ogmdvd_title_chooser_init                (OGMDvdTitleChooserIface  *iface);
static void ogmdvd_title_chooser_widget_dispose      (GObject                  *object);
static void ogmdvd_title_chooser_widget_get_property (GObject                  *gobject,
                                                      guint                    property_id,
                                                      GValue                   *value,
                                                      GParamSpec               *pspec);
static void ogmdvd_title_chooser_widget_set_property (GObject                  *gobject,
                                                      guint                    property_id,
                                                      const GValue             *value,
                                                      GParamSpec               *pspec);

/*
 * OGMDvdTitleChooser funcs
 */

static void          ogmdvd_title_chooser_widget_set_disc   (OGMDvdTitleChooser *chooser,
                                                             OGMDvdDisc         *disc);
static OGMDvdDisc *  ogmdvd_title_chooser_widget_get_disc   (OGMDvdTitleChooser *chooser);
static OGMDvdTitle * ogmdvd_title_chooser_widget_get_active (OGMDvdTitleChooser *chooser);

G_DEFINE_TYPE_WITH_CODE (OGMDvdTitleChooserWidget, ogmdvd_title_chooser_widget, GTK_TYPE_COMBO_BOX,
    G_IMPLEMENT_INTERFACE (OGMDVD_TYPE_TITLE_CHOOSER, ogmdvd_title_chooser_init))

static void
ogmdvd_title_chooser_widget_class_init (OGMDvdTitleChooserWidgetClass *klass)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *) klass;
  object_class->dispose = ogmdvd_title_chooser_widget_dispose;
  object_class->get_property = ogmdvd_title_chooser_widget_get_property;
  object_class->set_property = ogmdvd_title_chooser_widget_set_property;

  g_object_class_override_property (object_class, PROP_DISC, "disc");
  g_object_class_override_property (object_class, PROP_TITLE, "title");

  g_type_class_add_private (klass, sizeof (OGMDvdTitleChooserWidgetPriv));
}

static void
ogmdvd_title_chooser_init (OGMDvdTitleChooserIface *iface)
{
  iface->set_disc = ogmdvd_title_chooser_widget_set_disc;
  iface->get_disc = ogmdvd_title_chooser_widget_get_disc;
  iface->get_active = ogmdvd_title_chooser_widget_get_active;
}

static void
ogmdvd_title_chooser_widget_init (OGMDvdTitleChooserWidget *chooser)
{
  GtkCellRenderer *cell;
  GtkListStore *store;

  chooser->priv = OGMDVD_TITLE_CHOOSER_WIDGET_GET_PRIVATE (chooser);

  store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_INT);
  gtk_combo_box_set_model (GTK_COMBO_BOX (chooser), GTK_TREE_MODEL (store));
  g_object_unref (store);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser), cell, "text", TEXT_COLUMN, NULL);
}

static void
ogmdvd_title_chooser_widget_dispose (GObject *object)
{
  OGMDvdTitleChooserWidget *chooser;

  chooser = OGMDVD_TITLE_CHOOSER_WIDGET (object);

  if (chooser->priv->disc)
    ogmdvd_disc_unref (chooser->priv->disc);
  chooser->priv->disc = NULL;

  if (chooser->priv->title)
    ogmdvd_title_unref (chooser->priv->title);
  chooser->priv->title = NULL;

  (*G_OBJECT_CLASS (ogmdvd_title_chooser_widget_parent_class)->dispose) (object);
}

static void
ogmdvd_title_chooser_widget_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMDvdTitleChooser *chooser;

  chooser = OGMDVD_TITLE_CHOOSER (gobject);

  switch (property_id) 
  {
    case PROP_DISC:
      g_value_set_pointer (value, ogmdvd_title_chooser_widget_get_disc (chooser));
      break;
    case PROP_TITLE:
      g_value_set_pointer (value, ogmdvd_title_chooser_widget_get_active (chooser));
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmdvd_title_chooser_widget_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMDvdTitleChooser *chooser;

  chooser = OGMDVD_TITLE_CHOOSER (gobject);

  switch (property_id) 
  {
    case PROP_DISC:
      ogmdvd_title_chooser_widget_set_disc (chooser, g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

/*
 * OGMDvdTitleChooser funcs
 */

static void
ogmdvd_title_chooser_widget_set_disc (OGMDvdTitleChooser *chooser, OGMDvdDisc *disc)
{
  OGMDvdTitleChooserWidget *widget;

  g_return_if_fail (OGMDVD_IS_TITLE_CHOOSER_WIDGET (chooser));

  widget = OGMDVD_TITLE_CHOOSER_WIDGET (chooser);

  if (widget->priv->disc != disc)
  {
    GtkTreeModel *model;
    GtkTreeIter iter;

    OGMDvdTitle *title;
    OGMDvdTime time_;

    gint vid, nvid, format, aspect;
    glong length, longest;
    gchar *str, *str_time;

    if (widget->priv->title)
      ogmdvd_title_unref (widget->priv->title);
    widget->priv->title = NULL;

    if (disc)
      ogmdvd_disc_ref (disc);
    if (widget->priv->disc)
      ogmdvd_disc_unref (widget->priv->disc);
    widget->priv->disc = disc;

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
    gtk_list_store_clear (GTK_LIST_STORE (model));

    if (!disc)
      gtk_combo_box_set_active (GTK_COMBO_BOX (widget), -1);
    else
    {
      nvid = ogmdvd_disc_get_n_titles (disc);
      for (vid = 0, longest = 0; vid < nvid; vid++)
      {
        title = ogmdvd_disc_get_nth_title (disc, vid);
        if (title)
        {
          format = ogmdvd_title_get_video_format (title);
          aspect = ogmdvd_title_get_display_aspect (title);
          length = ogmdvd_title_get_length (title, &time_);

          if (time_.hour > 0)
            str_time = g_strdup_printf ("%02d:%02d %s", time_.hour, time_.min, _("hours"));
          else if (time_.min > 0)
            str_time = g_strdup_printf ("%02d:%02d %s", time_.min, time_.sec, _("minutes"));
          else
            str_time = g_strdup_printf ("%02d %s", time_.sec, _("seconds"));

          str = g_strdup_printf ("%s %02d (%s, %s, %s)", _("Title"), vid + 1, str_time,
              ogmdvd_get_video_format_label (format), 
              ogmdvd_get_display_aspect_label (aspect)); 
          g_free (str_time);

          gtk_list_store_append (GTK_LIST_STORE (model), &iter);
          gtk_list_store_set (GTK_LIST_STORE (model), &iter, TEXT_COLUMN, str, NR_COLUMN, vid, -1);
          g_free (str);

          ogmdvd_title_unref (title);

          if (length > longest)
          {
            longest = length;
            gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
          }
        }
      }
    }
  }
}

static OGMDvdDisc *
ogmdvd_title_chooser_widget_get_disc (OGMDvdTitleChooser *chooser)
{
  g_return_val_if_fail (OGMDVD_IS_TITLE_CHOOSER_WIDGET (chooser), NULL);

  return OGMDVD_TITLE_CHOOSER_WIDGET (chooser)->priv->disc;
}

static OGMDvdTitle *
ogmdvd_title_chooser_widget_get_active (OGMDvdTitleChooser *chooser)
{
  OGMDvdTitleChooserWidget *widget;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint nr;

  g_return_val_if_fail (OGMDVD_IS_TITLE_CHOOSER_WIDGET (chooser), NULL);

  widget = OGMDVD_TITLE_CHOOSER_WIDGET (chooser);

  if (!widget->priv->disc)
    return NULL;

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter))
    return NULL;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
  gtk_tree_model_get (model, &iter, NR_COLUMN, &nr, -1);

  if (!widget->priv->title)
    widget->priv->title = ogmdvd_disc_get_nth_title (widget->priv->disc, nr);
  else
  {
    if (ogmdvd_title_get_nr (widget->priv->title) != nr)
    {
      ogmdvd_title_unref (widget->priv->title);
      widget->priv->title = ogmdvd_disc_get_nth_title (widget->priv->disc, nr);
    }
  }

  return widget->priv->title;
}

/**
 * ogmdvd_title_chooser_widget_new:
 *
 * Creates a new #OGMDvdTitleChooserWidget.
 *
 * Returns: The new #OGMDvdTitleChooserWidget
 */
GtkWidget *
ogmdvd_title_chooser_widget_new (void)
{
  return g_object_new (OGMDVD_TYPE_TITLE_CHOOSER_WIDGET, NULL);
}

