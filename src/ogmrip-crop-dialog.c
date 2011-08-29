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

#include "ogmrip-crop-dialog.h"

#include <ogmrip-job.h>

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

static gchar **
ogmrip_crop_dialog_grab_command (OGMRipCropDialog *dialog, gulong frame)
{
  GPtrArray *argv;
  const gchar *uri;
  gint vid, time_;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("mplayer"));
  g_ptr_array_add (argv, g_strdup ("-nolirc"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-nosound"));
  g_ptr_array_add (argv, g_strdup ("-nozoom"));

  if (ogmrip_check_mplayer_nosub ())
    g_ptr_array_add (argv, g_strdup ("-nosub"));

  g_ptr_array_add (argv, g_strdup ("-vo"));

  if (MPLAYER_CHECK_VERSION (1,0,0,6))
    g_ptr_array_add (argv, g_strdup_printf ("jpeg:outdir=%s", ogmrip_fs_get_tmp_dir ()));
  else
  {
    g_ptr_array_add (argv, g_strdup ("jpeg"));
    g_ptr_array_add (argv, g_strdup ("-jpeg"));
    g_ptr_array_add (argv, g_strdup_printf ("outdir=%s", ogmrip_fs_get_tmp_dir ()));
  }

  if (MPLAYER_CHECK_VERSION (1,0,0,8))
  {
    g_ptr_array_add (argv, g_strdup ("-vc"));
    g_ptr_array_add (argv, g_strdup ("ffmpeg12"));
    g_ptr_array_add (argv, g_strdup ("-frames"));
    g_ptr_array_add (argv, g_strdup ("1"));
  }
  else
  {
    g_ptr_array_add (argv, g_strdup ("-frames"));
    g_ptr_array_add (argv, g_strdup ("3"));
    g_ptr_array_add (argv, g_strdup ("-sstep"));
    g_ptr_array_add (argv, g_strdup ("1"));
  }

  g_ptr_array_add (argv, g_strdup ("-speed"));
  g_ptr_array_add (argv, g_strdup ("100"));

  if (dialog->priv->deint)
  {
    g_ptr_array_add (argv, g_strdup ("-vf"));
    g_ptr_array_add (argv, g_strdup ("pp=lb"));
  }

  time_ = (gint) (frame * dialog->priv->rate_denominator / (gdouble) dialog->priv->rate_numerator);
  g_ptr_array_add (argv, g_strdup ("-ss"));
  g_ptr_array_add (argv, g_strdup_printf ("%u", time_));

  uri = ogmrip_media_get_uri (ogmrip_title_get_media (dialog->priv->title));
  if (!g_str_has_prefix (uri, "dvd://"))
    g_warning ("Unknown scheme for '%s'", uri);
  else
  {
    g_ptr_array_add (argv, g_strdup ("-dvd-device"));
    g_ptr_array_add (argv, g_strdup (uri + 6));
  }

  vid = ogmrip_title_get_nr (dialog->priv->title);

  if (MPLAYER_CHECK_VERSION (1,0,0,1))
    g_ptr_array_add (argv, g_strdup_printf ("dvd://%d", vid + 1));
  else
  {
    g_ptr_array_add (argv, g_strdup ("-dvd"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", vid + 1));
  }

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static void
ogmrip_crop_dialog_grab_frame (OGMRipCropDialog *dialog, gulong frame)
{
  OGMJobSpawn *spawn;
  gchar **cmd;

  cmd = ogmrip_crop_dialog_grab_command (dialog, frame);

  spawn = ogmjob_exec_newv (cmd);
  if (ogmjob_spawn_run (spawn, NULL) == OGMJOB_RESULT_SUCCESS)
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
  g_object_unref (spawn);
}

static void
ogmrip_crop_dialog_spin_value_changed (OGMRipCropDialog *dialog)
{
  ogmrip_crop_dialog_crop_frame (dialog);
}

static void
ogmrip_crop_dialog_scale_button_released (OGMRipCropDialog *dialog, GdkEventButton *event, GtkWidget *scale)
{
  if (event->button == 1)
  {
    gulong frame;
    gchar *text;

    frame = (guint) gtk_range_get_value (GTK_RANGE (scale));
    text = g_strdup_printf (_("Frame %lu of %lu"), frame, dialog->priv->length);
    gtk_label_set_text (GTK_LABEL (dialog->priv->label), text);
    g_free (text);

    ogmrip_crop_dialog_grab_frame (dialog, frame);
  }
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
  {
    g_critical ("Couldn't load builder file: %s", error->message);
    return;
  }

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

