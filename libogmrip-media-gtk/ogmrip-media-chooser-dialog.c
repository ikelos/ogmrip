/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#define OGMRIP_UI_RES   "/org/ogmrip/ogmrip-media-chooser-dialog.ui"

enum
{
  EJECT,
  LAST_SIGNAL
};

struct _OGMRipMediaChooserDialogPriv
{
  GtkWidget *media_chooser;
  GtkWidget *eject_button;
  GtkWidget *load_button;
};

static void ogmrip_media_chooser_init (OGMRipMediaChooserInterface *iface);

static guint signals[LAST_SIGNAL] = { 0 };

static OGMRipMedia *
ogmrip_media_chooser_dialog_get_media (OGMRipMediaChooser *chooser)
{
  OGMRipMediaChooserDialog *dialog = OGMRIP_MEDIA_CHOOSER_DIALOG (chooser);

  return ogmrip_media_chooser_get_media (OGMRIP_MEDIA_CHOOSER (dialog->priv->media_chooser));
}

static void
ogmrip_media_chooser_dialog_media_changed (OGMRipMediaChooserDialog *dialog, OGMRipMedia *media, GtkWidget *chooser)
{
  GVolume *volume;

  volume = ogmrip_media_chooser_widget_get_volume (OGMRIP_MEDIA_CHOOSER_WIDGET (dialog->priv->media_chooser));

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

  volume = ogmrip_media_chooser_widget_get_volume (OGMRIP_MEDIA_CHOOSER_WIDGET (dialog->priv->media_chooser));
  if (volume)
    g_volume_eject_with_operation (volume, G_MOUNT_UNMOUNT_NONE, NULL, NULL,
        (GAsyncReadyCallback) ogmrip_media_chooser_dialog_eject_ready, dialog);
}

G_DEFINE_TYPE_WITH_CODE (OGMRipMediaChooserDialog, ogmrip_media_chooser_dialog, GTK_TYPE_DIALOG,
    G_ADD_PRIVATE (OGMRipMediaChooserDialog)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_MEDIA_CHOOSER, ogmrip_media_chooser_init))

static void
ogmrip_media_chooser_dialog_class_init (OGMRipMediaChooserDialogClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), OGMRIP_UI_RES);

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

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipMediaChooserDialog, media_chooser);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipMediaChooserDialog, eject_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipMediaChooserDialog, load_button);
}

static void
ogmrip_media_chooser_init (OGMRipMediaChooserInterface *iface)
{
  iface->get_media = ogmrip_media_chooser_dialog_get_media;
}

static void
ogmrip_media_chooser_dialog_init (OGMRipMediaChooserDialog *dialog)
{
  g_type_ensure (OGMRIP_TYPE_MEDIA_CHOOSER_WIDGET);

  gtk_widget_init_template (GTK_WIDGET (dialog));

  dialog->priv = ogmrip_media_chooser_dialog_get_instance_private (dialog);

  g_signal_connect_swapped (dialog->priv->media_chooser, "media-changed", 
      G_CALLBACK (ogmrip_media_chooser_dialog_media_changed), dialog);

  g_signal_connect_swapped (dialog->priv->eject_button, "clicked",
      G_CALLBACK (ogmrip_media_chooser_dialog_eject_clicked), dialog);

  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->media_chooser), 0);
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

