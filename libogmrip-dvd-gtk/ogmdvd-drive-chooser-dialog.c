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
 * SECTION:ogmdvd-drive-chooser-dialog
 * @title: OGMDvdDriveChooserDialog
 * @include: ogmdvd-drive-chooser-dialog.h
 * @short_description: A DVD drive chooser dialog
 */

#include "ogmdvd-drive-chooser-dialog.h"
#include "ogmdvd-drive-chooser-widget.h"

#include <ogmrip-dvd.h>

#include <glib/gi18n-lib.h>

#define OGMDVD_DRIVE_CHOOSER_DIALOG_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMDVD_TYPE_DRIVE_CHOOSER_DIALOG, OGMDvdDriveChooserDialogPriv))

enum
{
  EJECT,
  LAST_SIGNAL
};

struct _OGMDvdDriveChooserDialogPriv
{
  GtkWidget *chooser;

  GtkWidget *eject_button;
  GtkWidget *load_button;
};

/*
 * GObject vfuncs
 */

static void ogmdvd_drive_chooser_init (OGMDvdDriveChooserInterface *iface);

static gchar * ogmdvd_drive_chooser_dialog_get_device     (OGMDvdDriveChooser       *chooser,
                                                           OGMDvdDeviceType         *type);
static void    ogmdvd_drive_chooser_dialog_eject_clicked  (GtkDialog                *dialog);
static void    ogmdvd_drive_chooser_dialog_device_changed (OGMDvdDriveChooserDialog *dialog,
                                                           const gchar              *device,
                                                           OGMDvdDeviceType         type,
                                                           GtkWidget                *chooser);

static int signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (OGMDvdDriveChooserDialog, ogmdvd_drive_chooser_dialog, GTK_TYPE_DIALOG,
    G_IMPLEMENT_INTERFACE (OGMDVD_TYPE_DRIVE_CHOOSER, ogmdvd_drive_chooser_init))

static void
ogmdvd_drive_chooser_dialog_class_init (OGMDvdDriveChooserDialogClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  /**
   * OGMDvdDriveChooserDialog::eject
   * @dialog: the widget that received the signal
   *
   * Emitted each time the eject button is clicked.
   */
  signals[EJECT] = g_signal_new ("eject", G_TYPE_FROM_CLASS (object_class), 
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMDvdDriveChooserDialogClass, eject), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  g_type_class_add_private (klass, sizeof (OGMDvdDriveChooserDialogPriv));
}

static void
ogmdvd_drive_chooser_init (OGMDvdDriveChooserInterface *iface)
{
  iface->get_device = ogmdvd_drive_chooser_dialog_get_device;
}

static void
ogmdvd_drive_chooser_dialog_init (OGMDvdDriveChooserDialog *dialog)
{
  GtkWidget *area, *image, *label, *vbox;

  OGMDvdDeviceType type;
  gchar *device;

  dialog->priv = OGMDVD_DRIVE_CHOOSER_DIALOG_GET_PRIVATE (dialog);

  gtk_window_set_title (GTK_WINDOW (dialog), _("Open DVD Disk"));
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

  area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));

  dialog->priv->eject_button = gtk_button_new_with_mnemonic (_("_Eject"));
  gtk_container_add (GTK_CONTAINER (area), dialog->priv->eject_button);
  gtk_widget_show (dialog->priv->eject_button);

  g_signal_connect_swapped (dialog->priv->eject_button, "clicked", G_CALLBACK (ogmdvd_drive_chooser_dialog_eject_clicked), dialog);

  image = gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (dialog->priv->eject_button), image);

  dialog->priv->load_button = gtk_button_new_with_mnemonic (_("_Load"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), dialog->priv->load_button, GTK_RESPONSE_OK);
  gtk_widget_show (dialog->priv->load_button);

  image = gtk_image_new_from_stock (GTK_STOCK_CDROM, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (dialog->priv->load_button), image);

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (area), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("<b>Select _DVD Drive:</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  dialog->priv->chooser = ogmdvd_drive_chooser_widget_new ();
  gtk_box_pack_start (GTK_BOX (vbox), dialog->priv->chooser, TRUE, TRUE, 0);
  gtk_widget_show (dialog->priv->chooser);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->priv->chooser);

  g_signal_connect_swapped (dialog->priv->chooser, "device-changed", 
      G_CALLBACK (ogmdvd_drive_chooser_dialog_device_changed), dialog);

  device = ogmdvd_drive_chooser_get_device (OGMDVD_DRIVE_CHOOSER (dialog->priv->chooser), &type);

  gtk_widget_set_sensitive (dialog->priv->eject_button, device != NULL && type == OGMDVD_DEVICE_BLOCK);
  gtk_widget_set_sensitive (dialog->priv->load_button, device != NULL && type != OGMDVD_DEVICE_NONE);

  g_free (device);
}

/*
 * OGMDvdDriveChooser vfuncs
 */

static gchar *
ogmdvd_drive_chooser_dialog_get_device (OGMDvdDriveChooser *chooser, OGMDvdDeviceType *type)
{
  OGMDvdDriveChooserDialog *dialog;

  g_return_val_if_fail (OGMDVD_IS_DRIVE_CHOOSER_DIALOG (chooser), NULL);

  dialog = OGMDVD_DRIVE_CHOOSER_DIALOG (chooser);

  return ogmdvd_drive_chooser_get_device (OGMDVD_DRIVE_CHOOSER (dialog->priv->chooser), type);
}

/*
 * Internal functions
 */

static void
ogmdvd_drive_chooser_dialog_eject_clicked (GtkDialog *dialog)
{
  OGMDvdDeviceType type;
  gchar *device;

  device = ogmdvd_drive_chooser_get_device (OGMDVD_DRIVE_CHOOSER (OGMDVD_DRIVE_CHOOSER_DIALOG (dialog)->priv->chooser), &type);
  if (device != NULL && type == OGMDVD_DEVICE_BLOCK)
  {
    OGMDvdMonitor *monitor;
    OGMDvdDrive *drive;

    monitor = ogmdvd_monitor_get_default ();
    drive = ogmdvd_monitor_get_drive (monitor, device);
    g_object_unref (monitor);

    if (drive)
    {
      g_signal_emit (dialog, signals[EJECT], 0, NULL);
      ogmdvd_drive_eject (OGMDVD_DRIVE (drive));
      g_object_unref (drive);
    }
  }

  if (device)
    g_free (device);
}

static void
ogmdvd_drive_chooser_dialog_device_changed (OGMDvdDriveChooserDialog *dialog, const gchar *device, OGMDvdDeviceType type, GtkWidget *chooser)
{
  gtk_widget_set_sensitive (dialog->priv->load_button, device != NULL);
  gtk_widget_set_sensitive (dialog->priv->eject_button, device != NULL && type == OGMDVD_DEVICE_BLOCK);
}

/**
 * ogmdvd_drive_chooser_dialog_new:
 *
 * Creates a new #OGMDvdDriveChooserDialog.
 *
 * Returns: The new #OGMDvdDriveChooserDialog
 */
GtkWidget *
ogmdvd_drive_chooser_dialog_new (void)
{
  GtkWidget *widget;

  widget = g_object_new (OGMDVD_TYPE_DRIVE_CHOOSER_DIALOG, NULL);

  return widget;
}
