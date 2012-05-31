/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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
#include <ogmrip-mplayer.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#define OGMRIP_GLADE_FILE "ogmrip" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-crop.glade"
#define OGMRIP_GLADE_ROOT "root"

#define SCALE_FACTOR 2 / 3

#define OGMRIP_CROP_DIALOG_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_CROP_DIALOG, OGMRipCropDialogPriv))

struct _OGMRipCropDialogPriv
{
  OGMRipTitle *title;

  GtkWidget *left_spin;
  GtkWidget *right_spin;
  GtkWidget *top_spin;
  GtkWidget *bottom_spin;

  GtkWidget *image;
  GtkWidget *scale;
  GtkWidget *label;
  GtkWidget *aspect;

  GdkPixbuf *pixbuf;

  gulong length;
  guint rate_numerator;
  guint rate_denominator;

  guint raw_width;
  guint raw_height;

  gboolean deint;

};

static void ogmrip_crop_dialog_dispose (GObject *gobject);

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
    gtk_image_set_from_pixbuf (GTK_IMAGE (dialog->priv->image), pixbuf);
    g_object_unref (pixbuf);
  }
}

static void
ogmrip_crop_dialog_grab_frame (OGMRipCropDialog *dialog, gulong frame)
{
  OGMJobTask *task;
  GPtrArray *argv;
  guint position;

  position = (guint) (frame * dialog->priv->rate_denominator / (gdouble) dialog->priv->rate_numerator);
  argv = ogmrip_mplayer_grab_frame_command (dialog->priv->title, position, dialog->priv->deint);

  task = ogmjob_spawn_newv ((gchar **) g_ptr_array_free (argv, FALSE));
  if (ogmjob_task_run (task, NULL, NULL))
  {
    gchar *filename;

    if (MPLAYER_CHECK_VERSION (1,0,0,8))
      filename = g_build_filename (ogmrip_fs_get_tmp_dir (), "00000001.jpg", NULL);
    else
    {
      filename = g_build_filename (ogmrip_fs_get_tmp_dir (), "00000001.jpg", NULL);
      g_unlink (filename);
      g_free (filename);

      filename = g_build_filename (ogmrip_fs_get_tmp_dir (), "00000003.jpg", NULL);
      g_unlink (filename);
      g_free (filename);

      filename = g_build_filename (ogmrip_fs_get_tmp_dir (), "00000002.jpg", NULL);
    }

    if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    {
      if (dialog->priv->pixbuf)
        g_object_unref (dialog->priv->pixbuf);
      dialog->priv->pixbuf = gdk_pixbuf_new_from_file_at_size (filename, 
          dialog->priv->raw_width * SCALE_FACTOR, dialog->priv->raw_height * SCALE_FACTOR, NULL);
      g_unlink (filename);
    }
    g_free (filename);

    if (dialog->priv->pixbuf)
      ogmrip_crop_dialog_crop_frame (dialog);
  }
  g_object_unref (task);
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
  gtk_label_set_text (GTK_LABEL (dialog->priv->label), text);
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

G_DEFINE_TYPE (OGMRipCropDialog, ogmrip_crop_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_crop_dialog_class_init (OGMRipCropDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = ogmrip_crop_dialog_dispose;

  g_type_class_add_private (klass, sizeof (OGMRipCropDialogPriv));
}

static void
ogmrip_crop_dialog_init (OGMRipCropDialog *dialog)
{
  GError *error = NULL;

  GtkBuilder *builder;
  GtkWidget *area, *root;

  dialog->priv = OGMRIP_CROP_DIALOG_GET_PRIVATE (dialog);

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_GLADE_FILE, &error))
    g_error ("Couldn't load builder file: %s", error->message);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_OK, GTK_RESPONSE_OK,
      NULL);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Cropping"));

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  root = gtk_builder_get_widget (builder, OGMRIP_GLADE_ROOT);
  gtk_container_add (GTK_CONTAINER (area), root);
  gtk_widget_show (root);

  dialog->priv->left_spin = gtk_builder_get_widget (builder, "left-spin");
  g_signal_connect_swapped (dialog->priv->left_spin, "value-changed",
      G_CALLBACK (ogmrip_crop_dialog_spin_value_changed), dialog);

  dialog->priv->top_spin = gtk_builder_get_widget (builder, "top-spin");
  g_signal_connect_swapped (dialog->priv->top_spin, "value-changed",
      G_CALLBACK (ogmrip_crop_dialog_spin_value_changed), dialog);

  dialog->priv->right_spin = gtk_builder_get_widget (builder, "right-spin");
  g_signal_connect_swapped (dialog->priv->right_spin, "value-changed",
      G_CALLBACK (ogmrip_crop_dialog_spin_value_changed), dialog);

  dialog->priv->bottom_spin = gtk_builder_get_widget (builder, "bottom-spin");
  g_signal_connect_swapped (dialog->priv->bottom_spin, "value-changed",
      G_CALLBACK (ogmrip_crop_dialog_spin_value_changed), dialog);

  dialog->priv->image = gtk_builder_get_widget (builder, "frame-image");
  dialog->priv->label = gtk_builder_get_widget (builder, "frame-label");

  dialog->priv->scale = gtk_builder_get_widget (builder, "frame-scale");
  g_signal_connect_swapped (dialog->priv->scale, "value-changed",
      G_CALLBACK (ogmrip_crop_dialog_scale_value_changed), dialog);
  g_signal_connect_swapped (dialog->priv->scale, "button-release-event",
      G_CALLBACK (ogmrip_crop_dialog_scale_button_released), dialog);

  g_object_unref (builder);
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

GtkWidget *
ogmrip_crop_dialog_new (OGMRipTitle *title, guint left, guint top, guint right, guint bottom)
{
  OGMRipCropDialog *dialog;
  gdouble framerate;
  gint32 frame;

  dialog = g_object_new (OGMRIP_TYPE_CROP_DIALOG, NULL);

  g_object_ref (title);
  if (dialog->priv->title)
    g_object_unref (dialog->priv->title);
  dialog->priv->title = title;

  ogmrip_video_stream_get_resolution (ogmrip_title_get_video_stream (title),
      &dialog->priv->raw_width, &dialog->priv->raw_height);
  ogmrip_video_stream_get_framerate (ogmrip_title_get_video_stream (title),
      &dialog->priv->rate_numerator, &dialog->priv->rate_denominator);

  framerate = dialog->priv->rate_numerator / (gdouble) dialog->priv->rate_denominator;
  dialog->priv->length = ogmrip_title_get_length (title, NULL) * framerate;

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (dialog->priv->left_spin), 0.0, (gdouble) dialog->priv->raw_width);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->left_spin), (gdouble) left);

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (dialog->priv->top_spin), 0.0, (gdouble) dialog->priv->raw_height);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->top_spin), (gdouble) top);

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (dialog->priv->right_spin), 0.0, (gdouble) dialog->priv->raw_width);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->right_spin), (gdouble) right);

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (dialog->priv->bottom_spin), 0.0, (gdouble) dialog->priv->raw_height);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->bottom_spin), (gdouble) bottom);

  gtk_range_set_range (GTK_RANGE (dialog->priv->scale), 1.0, dialog->priv->length);
  gtk_range_set_increments (GTK_RANGE (dialog->priv->scale), 1.0, dialog->priv->length / 25);

  frame = g_random_int_range (1, dialog->priv->length);
  gtk_range_set_value (GTK_RANGE (dialog->priv->scale), frame);

  ogmrip_crop_dialog_scale_button_released (dialog, NULL, dialog->priv->scale);

  return GTK_WIDGET (dialog);
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
    gtk_range_set_value (GTK_RANGE (dialog->priv->scale), frame);
  }
}

