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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-subp-chooser-widget.h"
#include "ogmrip-subp-options-dialog.h"

#include <ogmrip-file-gtk.h>

#include <glib/gi18n-lib.h>

#define OGMRIP_SUBP_CHOOSER_WIDGET_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_SUBP_CHOOSER_WIDGET, OGMRipSubpChooserWidgetPriv))

struct _OGMRipSubpChooserWidgetPriv
{
  GtkWidget *chooser;
  GtkWidget *button;
  GtkWidget *dialog;
};

enum
{
  PROP_0,
  PROP_TITLE
};

static void ogmrip_source_chooser_init (OGMRipSourceChooserInterface *iface);
static void ogmrip_subp_chooser_widget_get_property (GObject         *gobject,
                                                     guint           property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);
static void ogmrip_subp_chooser_widget_set_property (GObject         *gobject,
                                                     guint           property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void ogmrip_subp_chooser_widget_destroy      (GtkWidget       *widget);

static OGMRipStream *
ogmrip_subp_chooser_widget_get_active (OGMRipSourceChooser *chooser)
{
  OGMRipSubpChooserWidget *widget = OGMRIP_SUBP_CHOOSER_WIDGET (chooser);

  return ogmrip_source_chooser_get_active (OGMRIP_SOURCE_CHOOSER (widget->priv->chooser));
}

static void
ogmrip_subp_chooser_widget_set_active (OGMRipSourceChooser *chooser, OGMRipStream *stream)
{
  OGMRipSubpChooserWidget *widget = OGMRIP_SUBP_CHOOSER_WIDGET (chooser);

  ogmrip_source_chooser_set_active (OGMRIP_SOURCE_CHOOSER (widget->priv->chooser), stream);
}

static void
ogmrip_subp_chooser_widget_select_language (OGMRipSourceChooser *chooser, gint language)
{
  OGMRipSubpChooserWidget *widget = OGMRIP_SUBP_CHOOSER_WIDGET (chooser);

  return ogmrip_source_chooser_select_language (OGMRIP_SOURCE_CHOOSER (widget->priv->chooser), language);
}

static void
ogmrip_subp_chooser_widget_combo_changed (OGMRipSubpChooserWidget *widget)
{
  OGMRipStream *stream;

  stream = ogmrip_source_chooser_get_active (OGMRIP_SOURCE_CHOOSER (widget->priv->chooser));
  gtk_widget_set_sensitive (widget->priv->button, stream && !OGMRIP_IS_FILE (stream));

  if (stream && !OGMRIP_IS_FILE (stream))
  {
    ogmrip_subp_options_dialog_set_label (OGMRIP_SUBP_OPTIONS_DIALOG (widget->priv->dialog),
        ogmrip_subp_stream_get_label (OGMRIP_SUBP_STREAM (stream)));
    ogmrip_subp_options_dialog_set_language (OGMRIP_SUBP_OPTIONS_DIALOG (widget->priv->dialog),
        ogmrip_subp_stream_get_language (OGMRIP_SUBP_STREAM (stream)));
  }
}

static void
ogmrip_subp_chooser_widget_combo_title_notified (OGMRipSubpChooserWidget *widget)
{
  OGMRipTitle *title;

  ogmrip_source_chooser_widget_add_subp_stream (OGMRIP_SOURCE_CHOOSER_WIDGET (widget->priv->chooser), NULL);

  title = ogmrip_source_chooser_get_title (OGMRIP_SOURCE_CHOOSER (widget->priv->chooser));
  if (title)
  {
    OGMRipSubpStream *stream;
    gint i, n;

    n = ogmrip_title_get_n_subp_streams (title);
    for (i = 0; i < n; i ++)
    {
      stream = ogmrip_title_get_nth_subp_stream (title, i);
      if (stream)
        ogmrip_source_chooser_widget_add_subp_stream (OGMRIP_SOURCE_CHOOSER_WIDGET (widget->priv->chooser), stream);
    }
  }
}

static void
ogmrip_subp_chooser_widget_button_clicked (OGMRipSubpChooserWidget *widget)
{
  GtkWidget *toplevel;

   toplevel = gtk_widget_get_toplevel (GTK_WIDGET (widget));
   if (gtk_widget_is_toplevel (toplevel) && GTK_IS_WINDOW (toplevel))
     gtk_window_set_transient_for (GTK_WINDOW (widget->priv->dialog), GTK_WINDOW (toplevel));

   gtk_dialog_run (GTK_DIALOG (widget->priv->dialog));
   gtk_widget_hide (widget->priv->dialog);
}

G_DEFINE_TYPE_WITH_CODE (OGMRipSubpChooserWidget, ogmrip_subp_chooser_widget, OGMRIP_TYPE_LIST_ITEM,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_SOURCE_CHOOSER, ogmrip_source_chooser_init))

static void
ogmrip_subp_chooser_widget_class_init (OGMRipSubpChooserWidgetClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_subp_chooser_widget_get_property;
  gobject_class->set_property = ogmrip_subp_chooser_widget_set_property;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->destroy = ogmrip_subp_chooser_widget_destroy;

  g_object_class_override_property (gobject_class, PROP_TITLE, "title");

  g_type_class_add_private (klass, sizeof (OGMRipSubpChooserWidgetPriv));
}

static void
ogmrip_subp_chooser_widget_init (OGMRipSubpChooserWidget *widget)
{
  GtkWidget *dialog;
  GtkSizeGroup *group;

  widget->priv = OGMRIP_SUBP_CHOOSER_WIDGET_GET_PRIVATE (widget);

  dialog = ogmrip_subp_file_chooser_dialog_new ();

  gtk_box_set_spacing (GTK_BOX (widget), 6);

  widget->priv->chooser = ogmrip_source_chooser_widget_new_with_dialog (OGMRIP_FILE_CHOOSER_DIALOG (dialog));
  gtk_box_pack_start (GTK_BOX (widget), widget->priv->chooser, TRUE, TRUE, 0);
  gtk_widget_show (widget->priv->chooser);

  g_signal_connect_swapped (widget->priv->chooser, "changed",
      G_CALLBACK (ogmrip_subp_chooser_widget_combo_changed), widget);
  g_signal_connect_swapped (widget->priv->chooser, "notify::title",
      G_CALLBACK (ogmrip_subp_chooser_widget_combo_title_notified), widget);

  widget->priv->button = gtk_button_new_with_label ("...");
  gtk_box_pack_start (GTK_BOX (widget), widget->priv->button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (widget->priv->button, FALSE);
  gtk_widget_show (widget->priv->button);

  group = ogmrip_list_item_get_size_group (OGMRIP_LIST_ITEM (widget));
  gtk_size_group_add_widget (group, widget->priv->button);

  g_signal_connect_swapped (widget->priv->button, "clicked",
      G_CALLBACK (ogmrip_subp_chooser_widget_button_clicked), widget);

  widget->priv->dialog = ogmrip_subp_options_dialog_new ();
  g_signal_connect (widget->priv->dialog, "delete-event",
      G_CALLBACK (gtk_widget_hide_on_delete), NULL);
}

static void
ogmrip_source_chooser_init (OGMRipSourceChooserInterface *iface)
{
  iface->get_active = ogmrip_subp_chooser_widget_get_active;
  iface->set_active = ogmrip_subp_chooser_widget_set_active;
  iface->select_language = ogmrip_subp_chooser_widget_select_language;
}

static void
ogmrip_subp_chooser_widget_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipSubpChooserWidget *widget = OGMRIP_SUBP_CHOOSER_WIDGET (gobject);

  switch (property_id) 
  {
    case PROP_TITLE:
      g_value_set_object (value,
          ogmrip_source_chooser_get_title (OGMRIP_SOURCE_CHOOSER (widget->priv->chooser)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_subp_chooser_widget_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipSubpChooserWidget *widget = OGMRIP_SUBP_CHOOSER_WIDGET (gobject);

  switch (property_id) 
  {
    case PROP_TITLE:
      ogmrip_source_chooser_set_title (OGMRIP_SOURCE_CHOOSER (widget->priv->chooser), g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_subp_chooser_widget_destroy (GtkWidget *widget)
{
  OGMRipSubpChooserWidget *chooser = OGMRIP_SUBP_CHOOSER_WIDGET (widget);

  if (chooser->priv->dialog)
  {
    gtk_widget_destroy (chooser->priv->dialog);
    chooser->priv->dialog = NULL;
  }

  GTK_WIDGET_CLASS (ogmrip_subp_chooser_widget_parent_class)->destroy (widget);
}

GtkWidget *
ogmrip_subp_chooser_widget_new (void)
{
  return g_object_new (OGMRIP_TYPE_SUBP_CHOOSER_WIDGET, NULL);
}

OGMRipSubpOptions *
ogmrip_subp_chooser_widget_get_options (OGMRipSubpChooserWidget *chooser)
{
  g_return_val_if_fail (OGMRIP_IS_SUBP_CHOOSER_WIDGET (chooser), NULL);

  if (ogmrip_subp_options_dialog_get_use_defaults (OGMRIP_SUBP_OPTIONS_DIALOG (chooser->priv->dialog)))
    return NULL;

  return OGMRIP_SUBP_OPTIONS (chooser->priv->dialog);
}

const gchar *
ogmrip_subp_chooser_widget_get_label (OGMRipSubpChooserWidget *chooser)
{
  g_return_val_if_fail (OGMRIP_IS_SUBP_CHOOSER_WIDGET (chooser), NULL);

  return ogmrip_subp_options_dialog_get_label (OGMRIP_SUBP_OPTIONS_DIALOG (chooser->priv->dialog));
}

gint
ogmrip_subp_chooser_widget_get_language (OGMRipSubpChooserWidget *chooser)
{
  g_return_val_if_fail (OGMRIP_IS_SUBP_CHOOSER_WIDGET (chooser), -1);

  return ogmrip_subp_options_dialog_get_language (OGMRIP_SUBP_OPTIONS_DIALOG (chooser->priv->dialog));
}

