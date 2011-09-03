/* OGMDvd - A wrapper library around libdvdread
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

/**
 * SECTION:ogmdvd-drive-chooser
 * @title: OGMDvdDriveChooser
 * @include: ogmdvd-drive-chooser.h
 * @short_description: DVD drive chooser interface used by OGMDvdDriveChooserDialog
 *                     and OGMDvdDriveChooserWidget
 */

#include "ogmdvd-drive-chooser.h"
#include "ogmdvd-marshal.h"

G_DEFINE_INTERFACE (OGMDvdDriveChooser, ogmdvd_drive_chooser, GTK_TYPE_WIDGET);

static void
ogmdvd_drive_chooser_default_init (OGMDvdDriveChooserInterface *iface)
{
  GType iface_type = G_TYPE_FROM_INTERFACE (iface);

  /**
   * OGMDvdDriveChooser::device-changed:
   * @chooser: the widget that received the signal
   * @device: the DVD device
   *
   * Emitted each time a device is selected.
   */
  g_signal_new ("device-changed", iface_type,
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMDvdDriveChooserInterface, device_changed), NULL, NULL,
      ogmdvd_cclosure_marshal_VOID__STRING_UINT, G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_UINT);
}

/**
 * ogmdvd_drive_chooser_get_device:
 * @chooser: An #OGMDvdDriveChooser
 * @type: Location to store the type of the device, or NULL
 *
 * Returns the selected device.
 *
 * Returns: A device or NULL
 */
gchar *
ogmdvd_drive_chooser_get_device (OGMDvdDriveChooser *chooser, OGMDvdDeviceType *type)
{
  g_return_val_if_fail (OGMDVD_IS_DRIVE_CHOOSER (chooser), NULL);

  return OGMDVD_DRIVE_CHOOSER_GET_IFACE (chooser)->get_device (chooser, type);
}

