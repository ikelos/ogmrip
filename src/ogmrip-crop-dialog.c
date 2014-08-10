/* OGMRip - A media encoder for GNOME
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-crop-dialog.h"
#include "ogmrip-helper.h"

#include <ogmrip-job.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#define OGMRIP_UI_RES  "/org/ogmrip/ogmrip-crop-dialog.ui"
#define OGMRIP_UI_ROOT "root"

#define SCALE_FACTOR 2 / 3

struct _OGMRipCropDialogPriv
{
  OGMRipTitle *title;

  GtkWidget *left_spin;
  GtkWidget *right_spin;
  GtkWidget *top_spin;
  GtkWidget *bottom_spin;

  GtkWidget *frame_image;
  GtkWidget *frame_scale;
  GtkWidget *frame_label;

  GdkPixbuf *pixbuf;

  gulong length;
  guint rate_numerator;
  guint rate_denominator;

  guint raw_width;
  guint raw_height;

  gboolean deint;
};

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_LEFT,
  PROP_TOP,
  PROP_RIGHT,
  PROP_BOTTOM
};

static void
ogmrip_crop_dialog_crop_frame (OGMRipCropDialog *dialog)
{
  if (dialog->priv->pixbuf)
  {
    GdkPixbuf *pixbuf;
    gint left, top, right, bottom, w, h;

    left = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->left_spin)) * SCALE_FACTOR;
    top = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->top_spin)) * SCALE_FACTOR;
    right = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->right_spin)) * SCALE_FACTOR;
    bottom = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->bottom_spin)) * SCALE_FACTOR;

    w = gdk_pixbuf_get_width (dialog->priv->pixbuf)- left - right;
    h = gdk_pixbuf_get_height (dialog->priv->pixbuf) - top - bottom;

    pixbuf = gdk_pixbuf_new_subpixbuf (dialog->priv->pixbuf, left, top, w, h);
    gtk_image_set_from_pixbuf (GTK_IMAGE (dialog->priv->frame_image), pixbuf);
    g_object_unref (pixbuf);
  }
}

static void
ogmrip_crop_dialog_grab_frame (OGMRipCropDialog *dialog, gulong frame)
{
  GFile *file;

  file = ogmrip_title_grab_frame (dialog->priv->title, frame, NULL, NULL);
  if (file)
  {
    gchar *filename;

    if (dialog->priv->pixbuf)
      g_object_unref (dialog->priv->pixbuf);

    filename = g_file_get_path (file);
    dialog->priv->pixbuf = gdk_pixbuf_new_from_file_at_size (filename,
        dialog->priv->raw_width * SCALE_FACTOR, dialog->priv->raw_height * SCALE_FACTOR, NULL);
    g_free (filename);

    g_file_delete (file, NULL, NULL);
    g_object_unref (file);

    if (dialog->priv->pixbuf)
      ogmrip_crop_dialog_crop_frame (dialog);
  }
}

static void
ogmrip_crop_dialog_spin_value_changed (OGMRipCropDialog *dialog)
{
  ogmrip_crop_dialog_crop_frame (dialog);
}

static void
ogmrip_crop_dialog_scale_value_changed (OGMRipCropDialog *dialog, GtkWidget *scale)
{
  gulong frame;
  gchar *text;

  frame = (gulong) gtk_range_get_value (GTK_RANGE (scale));
  text = g_strdup_printf (_("Frame %lu of %lu"), frame, dialog->priv->length);
  gtk_label_set_text (GTK_LABEL (dialog->priv->frame_label), text);
  g_free (text);
}

static gboolean
ogmrip_crop_dialog_scale_button_released (OGMRipCropDialog *dialog, GdkEventButton *event, GtkWidget *scale)
{
  if (!event || event->button == 1)
  {
    gulong frame;

    frame = (guint) gtk_range_get_value (GTK_RANGE (scale));
    ogmrip_crop_dialog_grab_frame (dialog, frame);
  }

  return FALSE;
}

static void g_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (OGMRipCropDialog, ogmrip_crop_dialog, GTK_TYPE_DIALOG,
    G_ADD_PRIVATE (OGMRipCropDialog)
    G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, g_initable_iface_init))

static void
ogmrip_crop_dialog_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipCropDialog *dialog = OGMRIP_CROP_DIALOG (gobject);

  switch (property_id)
  {
    case PROP_TITLE:
      g_value_set_object (value, dialog->priv->title);
      break;
    case PROP_LEFT:
      g_value_set_uint (value, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->left_spin)));
      break;
    case PROP_TOP:
      g_value_set_uint (value, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->top_spin)));
      break;
    case PROP_RIGHT:
      g_value_set_uint (value, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->right_spin)));
      break;
    case PROP_BOTTOM:
      g_value_set_uint (value, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->bottom_spin)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_crop_dialog_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipCropDialog *dialog = OGMRIP_CROP_DIALOG (gobject);

  switch (property_id)
  {
    case PROP_TITLE:
      dialog->priv->title = g_value_dup_object (value);
      break;
    case PROP_LEFT:
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->left_spin), (gdouble) g_value_get_uint (value));
      break;
    case PROP_TOP:
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->top_spin), (gdouble) g_value_get_uint (value));
      break;
    case PROP_RIGHT:
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->right_spin), (gdouble) g_value_get_uint (value));
      break;
    case PROP_BOTTOM:
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->bottom_spin), (gdouble) g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_crop_dialog_constructed (GObject *gobject)
{
  OGMRipCropDialog *dialog = OGMRIP_CROP_DIALOG (gobject);
  gdouble framerate;
  gint32 frame;

  ogmrip_video_stream_get_resolution (ogmrip_title_get_video_stream (dialog->priv->title),
      &dialog->priv->raw_width, &dialog->priv->raw_height);
  ogmrip_video_stream_get_framerate (ogmrip_title_get_video_stream (dialog->priv->title),
      &dialog->priv->rate_numerator, &dialog->priv->rate_denominator);

  framerate = dialog->priv->rate_numerator / (gdouble) dialog->priv->rate_denominator;
  dialog->priv->length = ogmrip_title_get_length (dialog->priv->title, NULL) * framerate;

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (dialog->priv->left_spin),   0.0, (gdouble) dialog->priv->raw_width);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (dialog->priv->right_spin),  0.0, (gdouble) dialog->priv->raw_width);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (dialog->priv->top_spin),    0.0, (gdouble) dialog->priv->raw_height);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (dialog->priv->bottom_spin), 0.0, (gdouble) dialog->priv->raw_height);

  gtk_range_set_range (GTK_RANGE (dialog->priv->frame_scale), 1.0, dialog->priv->length);
  gtk_range_set_increments (GTK_RANGE (dialog->priv->frame_scale), 1.0, dialog->priv->length / 25);

  frame = g_random_int_range (1, dialog->priv->length);
  gtk_range_set_value (GTK_RANGE (dialog->priv->frame_scale), frame);

  ogmrip_crop_dialog_scale_button_released (dialog, NULL, dialog->priv->frame_scale);

  G_OBJECT_CLASS (ogmrip_crop_dialog_parent_class)->constructed (gobject);
}

static void
ogmrip_crop_dialog_dispose (GObject *gobject)
{
  OGMRipCropDialog *dialog = OGMRIP_CROP_DIALOG (gobject);

  if (dialog->priv->title)
  {
    g_object_unref (dialog->priv->title);
    dialog->priv->title = NULL;
  }

  if (dialog->priv->pixbuf)
  {
    g_object_unref (dialog->priv->pixbuf);
    dialog->priv->pixbuf = NULL;
  }

  G_OBJECT_CLASS (ogmrip_crop_dialog_parent_class)->dispose (gobject);
}

static void
ogmrip_crop_dialog_class_init (OGMRipCropDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = ogmrip_crop_dialog_get_property;
  gobject_class->set_property = ogmrip_crop_dialog_set_property;
  gobject_class->constructed = ogmrip_crop_dialog_constructed;
  gobject_class->dispose = ogmrip_crop_dialog_dispose;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TITLE,
      g_param_spec_object ("title", "title", "title",
        OGMRIP_TYPE_TITLE, G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_LEFT,
      g_param_spec_uint ("crop-left", "crop-left", "crop-left",
        0, G_MAXUINT, 0, G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TOP,
      g_param_spec_uint ("crop-top", "crop-top", "crop-top",
        0, G_MAXUINT, 0, G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_RIGHT,
      g_param_spec_uint ("crop-right", "crop-right", "crop-right",
        0, G_MAXUINT, 0, G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_BOTTOM,
      g_param_spec_uint ("crop-bottom", "crop-bottom", "crop-bottom",
        0, G_MAXUINT, 0, G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), OGMRIP_UI_RES);

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipCropDialog, left_spin);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipCropDialog, top_spin);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipCropDialog, right_spin);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipCropDialog, bottom_spin);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipCropDialog, frame_image);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipCropDialog, frame_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipCropDialog, frame_scale);
}

static void
ogmrip_crop_dialog_init (OGMRipCropDialog *dialog)
{
  gtk_widget_init_template (GTK_WIDGET (dialog));

  dialog->priv = ogmrip_crop_dialog_get_instance_private (dialog);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  g_signal_connect_swapped (dialog->priv->left_spin, "value-changed",
      G_CALLBACK (ogmrip_crop_dialog_spin_value_changed), dialog);

  g_signal_connect_swapped (dialog->priv->top_spin, "value-changed",
      G_CALLBACK (ogmrip_crop_dialog_spin_value_changed), dialog);

  g_signal_connect_swapped (dialog->priv->right_spin, "value-changed",
      G_CALLBACK (ogmrip_crop_dialog_spin_value_changed), dialog);

  g_signal_connect_swapped (dialog->priv->bottom_spin, "value-changed",
      G_CALLBACK (ogmrip_crop_dialog_spin_value_changed), dialog);

  g_signal_connect_swapped (dialog->priv->frame_scale, "value-changed",
      G_CALLBACK (ogmrip_crop_dialog_scale_value_changed), dialog);

  g_signal_connect_swapped (dialog->priv->frame_scale, "button-release-event",
      G_CALLBACK (ogmrip_crop_dialog_scale_button_released), dialog);
}

static gboolean
ogmrip_crop_dialog_initable_init (GInitable *initable, GCancellable *cancellable, GError **error)
{
  if (!OGMRIP_CROP_DIALOG (initable)->priv->title)
    return FALSE;

  return TRUE;
}

static void
g_initable_iface_init (GInitableIface *iface)
{
  iface->init = ogmrip_crop_dialog_initable_init;
}

GtkWidget *
ogmrip_crop_dialog_new (OGMRipTitle *title, guint left, guint top, guint right, guint bottom)
{
  return g_object_new (OGMRIP_TYPE_CROP_DIALOG, "title", title,
      "crop-left", left, "crop-top", top, "crop-right", right, "crop-bottom", bottom, NULL);
}

void
ogmrip_crop_dialog_get_crop (OGMRipCropDialog *dialog, guint *left, guint *top, guint *right, guint *bottom)
{
  g_return_if_fail (OGMRIP_IS_CROP_DIALOG (dialog));
  g_return_if_fail (left && top && right && bottom);

  *left = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->left_spin));
  *top = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->top_spin));
  *right = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->right_spin));
  *bottom = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->bottom_spin));
}

void
ogmrip_crop_dialog_set_deinterlacer (OGMRipCropDialog *dialog, gboolean deint)
{
  g_return_if_fail (OGMRIP_IS_CROP_DIALOG (dialog));

  if (dialog->priv->deint != deint)
  {
    gint32 frame;

    dialog->priv->deint = deint;
    frame = g_random_int_range (1, dialog->priv->length);
    gtk_range_set_value (GTK_RANGE (dialog->priv->frame_scale), frame);
  }
}

