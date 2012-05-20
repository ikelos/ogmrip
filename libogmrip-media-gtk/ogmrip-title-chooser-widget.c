/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2012 Olivier Rolland <billl@users.sourceforge.net>
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
 * @short_description: DVD title chooser widget that can be embedded in other widgets
 */

#include "ogmrip-title-chooser-widget.h"

#include <glib/gi18n-lib.h>

#define OGMRIP_TITLE_CHOOSER_WIDGET_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_TITLE_CHOOSER_WIDGET, OGMRipTitleChooserWidgetPriv))

enum
{
  PROP_0,
  PROP_MEDIA
};

enum
{
  TEXT_COLUMN,
  NR_COLUMN,
  NUM_COLUMNS
};

struct _OGMRipTitleChooserWidgetPriv
{
  OGMRipMedia *media;
};

static void ogmrip_title_chooser_init                (OGMRipTitleChooserInterface *iface);
static void ogmrip_title_chooser_widget_dispose      (GObject                     *object);
static void ogmrip_title_chooser_widget_get_property (GObject                     *gobject,
                                                      guint                       property_id,
                                                      GValue                      *value,
                                                      GParamSpec                  *pspec);
static void ogmrip_title_chooser_widget_set_property (GObject                     *gobject,
                                                      guint                       property_id,
                                                      const GValue                *value,
                                                      GParamSpec                  *pspec);


static void
ogmrip_title_chooser_widget_set_disc (OGMRipTitleChooserWidget *chooser, OGMRipMedia *media)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  OGMRipTitle *title;
  OGMRipTime time_;
  GString *string;

  gint vid, nvid, standard, aspect;
  glong length, longest;

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
    OGMRipVideoStream *stream;

    nvid = ogmrip_media_get_n_titles (media);
    for (vid = 0, longest = 0; vid < nvid; vid++)
    {
      title = ogmrip_media_get_nth_title (media, vid);
      if (title)
      {
        stream = ogmrip_title_get_video_stream (title);

        string = g_string_new (NULL);
        g_string_printf (string, "%s %02d", _("Title"), vid + 1);

        length = ogmrip_title_get_length (title, &time_);
        if (time_.hour > 0)
          g_string_append_printf (string, " (%02lu:%02lu %s", time_.hour, time_.min, _("hours"));
        else if (time_.min > 0)
          g_string_append_printf (string, " (%02lu:%02lu %s", time_.min, time_.sec, _("minutes"));
        else
          g_string_append_printf (string, " (%02lu %s", time_.sec, _("seconds"));

        standard = ogmrip_video_stream_get_standard (stream);
        if (standard != OGMRIP_STANDARD_UNDEFINED)
          g_string_append_printf (string, ", %s", ogmrip_standard_get_label (standard));

        aspect = ogmrip_video_stream_get_aspect (stream);
        if (aspect != OGMRIP_ASPECT_UNDEFINED)
          g_string_append_printf (string, ", %s", ogmrip_aspect_get_label (aspect));

        g_string_append_c (string, ')');

        gtk_list_store_append (GTK_LIST_STORE (model), &iter);
        gtk_list_store_set (GTK_LIST_STORE (model), &iter, TEXT_COLUMN, string->str, NR_COLUMN, vid, -1);
        g_string_free (string, TRUE);

        if (length > longest)
        {
          longest = length;
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (chooser), &iter);
        }
      }
    }
  }
}

static OGMRipTitle *
ogmrip_title_chooser_widget_get_active (OGMRipTitleChooser *chooser)
{
  OGMRipTitleChooserWidget *widget = OGMRIP_TITLE_CHOOSER_WIDGET (chooser);
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint nr;

  if (!widget->priv->media)
    return NULL;

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter))
    return NULL;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
  gtk_tree_model_get (model, &iter, NR_COLUMN, &nr, -1);

  return ogmrip_media_get_nth_title (widget->priv->media, nr);
}

static void
ogmrip_title_chooser_widget_set_active (OGMRipTitleChooser *chooser, OGMRipTitle *title)
{
  OGMRipTitleChooserWidget *widget = OGMRIP_TITLE_CHOOSER_WIDGET (chooser);
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint nr1, nr2;

  nr1 = ogmrip_title_get_nr (title);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    do
    {
      gtk_tree_model_get (model, &iter, NR_COLUMN, &nr2, -1);
    }
    while (nr1 != nr2 && gtk_tree_model_iter_next (model, &iter));

    if (nr2 == nr1)
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
  }
}

G_DEFINE_TYPE_WITH_CODE (OGMRipTitleChooserWidget, ogmrip_title_chooser_widget, GTK_TYPE_COMBO_BOX,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_TITLE_CHOOSER, ogmrip_title_chooser_init))

static void
ogmrip_title_chooser_widget_class_init (OGMRipTitleChooserWidgetClass *klass)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *) klass;
  object_class->dispose = ogmrip_title_chooser_widget_dispose;
  object_class->get_property = ogmrip_title_chooser_widget_get_property;
  object_class->set_property = ogmrip_title_chooser_widget_set_property;

  g_object_class_override_property (object_class, PROP_MEDIA, "media");

  g_type_class_add_private (klass, sizeof (OGMRipTitleChooserWidgetPriv));
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

  chooser->priv = OGMRIP_TITLE_CHOOSER_WIDGET_GET_PRIVATE (chooser);

  store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_INT);
  gtk_combo_box_set_model (GTK_COMBO_BOX (chooser), GTK_TREE_MODEL (store));
  g_object_unref (store);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser), cell, "text", TEXT_COLUMN, NULL);
}

static void
ogmrip_title_chooser_widget_dispose (GObject *object)
{
  OGMRipTitleChooserWidget *chooser;

  chooser = OGMRIP_TITLE_CHOOSER_WIDGET (object);

  if (chooser->priv->media)
  {
    g_object_unref (chooser->priv->media);
    chooser->priv->media = NULL;
  }

  (*G_OBJECT_CLASS (ogmrip_title_chooser_widget_parent_class)->dispose) (object);
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
      ogmrip_title_chooser_widget_set_disc (chooser, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
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

