/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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
 * SECTION:ogmrip-media-chooser-dialog
 * @title: OGMRipMediaChooserDialog
 * @include: ogmrip-media-chooser-dialog.h
 * @short_description: A media chooser dialog
 */

#include "ogmrip-media-chooser-dialog.h"
#include "ogmrip-media-chooser-widget.h"

#include <ogmrip-media.h>

#include <glib/gi18n-lib.h>

#define OGMRIP_MEDIA_CHOOSER_DIALOG_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_MEDIA_CHOOSER_DIALOG, OGMRipMediaChooserDialogPriv))

enum
{
  EJECT,
  LAST_SIGNAL
};

struct _OGMRipMediaChooserDialogPriv
{
  GtkWidget *chooser;

  GtkWidget *eject_button;
  GtkWidget *load_button;
};

static void ogmrip_media_chooser_init (OGMRipMediaChooserInterface *iface);

static guint signals[LAST_SIGNAL] = { 0 };

static OGMRipMedia *
ogmrip_media_chooser_dialog_get_media (OGMRipMediaChooser *chooser)
{
  OGMRipMediaChooserDialog *dialog = OGMRIP_MEDIA_CHOOSER_DIALOG (chooser);

  return ogmrip_media_chooser_get_media (OGMRIP_MEDIA_CHOOSER (dialog->priv->chooser));
}

static void
ogmrip_media_chooser_dialog_media_changed (OGMRipMediaChooserDialog *dialog, OGMRipMedia *media, GtkWidget *chooser)
{
  GVolume *volume;

  volume = ogmrip_media_chooser_widget_get_volume (OGMRIP_MEDIA_CHOOSER_WIDGET (dialog->priv->chooser));

  gtk_widget_set_sensitive (dialog->priv->eject_button, media != NULL && volume != NULL && g_volume_can_eject (volume));
  gtk_widget_set_sensitive (dialog->priv->load_button, media != NULL);
}

static void
ogmrip_media_chooser_dialog_eject_ready (GVolume *volume, GAsyncResult *res, OGMRipMediaChooserDialog *dialog)
{
  if (g_volume_eject_with_operation_finish (volume, res, NULL))
    g_signal_emit (dialog, signals[EJECT], 0);
}

static void
ogmrip_media_chooser_dialog_eject_clicked (OGMRipMediaChooserDialog *dialog)
{
  GVolume *volume;

  volume = ogmrip_media_chooser_widget_get_volume (OGMRIP_MEDIA_CHOOSER_WIDGET (dialog->priv->chooser));
  if (volume)
    g_volume_eject_with_operation (volume, G_MOUNT_UNMOUNT_NONE, NULL, NULL,
        (GAsyncReadyCallback) ogmrip_media_chooser_dialog_eject_ready, dialog);
}

G_DEFINE_TYPE_WITH_CODE (OGMRipMediaChooserDialog, ogmrip_media_chooser_dialog, GTK_TYPE_DIALOG,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_MEDIA_CHOOSER, ogmrip_media_chooser_init))

static void
ogmrip_media_chooser_dialog_class_init (OGMRipMediaChooserDialogClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  /**
   * OGMRipMediaChooserDialog::eject
   * @dialog: the widget that received the signal
   *
   * Emitted each time the eject button is clicked.
   */
  signals[EJECT] = g_signal_new ("eject", G_TYPE_FROM_CLASS (object_class), 
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipMediaChooserDialogClass, eject), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  g_type_class_add_private (klass, sizeof (OGMRipMediaChooserDialogPriv));
}

static void
ogmrip_media_chooser_init (OGMRipMediaChooserInterface *iface)
{
  iface->get_media = ogmrip_media_chooser_dialog_get_media;
}

static void
ogmrip_media_chooser_dialog_init (OGMRipMediaChooserDialog *dialog)
{
  GtkWidget *area, *image, *label, *vbox;

  dialog->priv = OGMRIP_MEDIA_CHOOSER_DIALOG_GET_PRIVATE (dialog);

  gtk_window_set_title (GTK_WINDOW (dialog), _("Select media"));
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

  area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));

  dialog->priv->eject_button = gtk_button_new_with_mnemonic (_("_Eject"));
  gtk_container_add (GTK_CONTAINER (area), dialog->priv->eject_button);
  gtk_widget_show (dialog->priv->eject_button);

  g_signal_connect_swapped (dialog->priv->eject_button, "clicked",
      G_CALLBACK (ogmrip_media_chooser_dialog_eject_clicked), dialog);

  image = gtk_image_new_from_icon_name ("view-refresh", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (dialog->priv->eject_button), image);

  dialog->priv->load_button = gtk_button_new_with_mnemonic (_("_Load"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), dialog->priv->load_button, GTK_RESPONSE_OK);
  gtk_widget_show (dialog->priv->load_button);

  image = gtk_image_new_from_icon_name ("media-optical", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (dialog->priv->load_button), image);

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  vbox = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (vbox), 6);
  gtk_grid_set_column_spacing (GTK_GRID (vbox), 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (area), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("<b>Select _media:</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (vbox), label, 0, 0, 1, 1);
  gtk_widget_show (label);

  dialog->priv->chooser = ogmrip_media_chooser_widget_new ();
  gtk_grid_attach (GTK_GRID (vbox), dialog->priv->chooser, 0, 1, 1, 1);
  gtk_widget_set_hexpand (dialog->priv->chooser, TRUE);
  gtk_widget_show (dialog->priv->chooser);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->priv->chooser);

  g_signal_connect_swapped (dialog->priv->chooser, "media-changed", 
      G_CALLBACK (ogmrip_media_chooser_dialog_media_changed), dialog);

  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->chooser), 0);
}

/**
 * ogmrip_media_chooser_dialog_new:
 *
 * Creates a new #OGMRipMediaChooserDialog.
 *
 * Returns: The new #OGMRipMediaChooserDialog
 */
GtkWidget *
ogmrip_media_chooser_dialog_new (void)
{
  GtkWidget *widget;

  widget = g_object_new (OGMRIP_TYPE_MEDIA_CHOOSER_DIALOG, NULL);

  return widget;
}

