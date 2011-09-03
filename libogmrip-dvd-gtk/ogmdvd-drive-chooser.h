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

#ifndef __OGMDVD_DRIVE_CHOOSER_H__
#define __OGMDVD_DRIVE_CHOOSER_H__

#include <gtk/gtk.h>

#include <ogmdvd-drive.h>

G_BEGIN_DECLS

typedef enum
{
  OGMDVD_DEVICE_NONE,
  OGMDVD_DEVICE_BLOCK,
  OGMDVD_DEVICE_FILE,
  OGMDVD_DEVICE_DIR
} OGMDvdDeviceType;

#define OGMDVD_TYPE_DRIVE_CHOOSER            (ogmdvd_drive_chooser_get_type ())
#define OGMDVD_DRIVE_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMDVD_TYPE_DRIVE_CHOOSER, OGMDvdDriveChooser))
#define OGMDVD_IS_DRIVE_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMDVD_TYPE_DRIVE_CHOOSER))
#define OGMDVD_DRIVE_CHOOSER_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMDVD_TYPE_DRIVE_CHOOSER, OGMDvdDriveChooserInterface))

typedef struct _OGMDvdDriveChooser          OGMDvdDriveChooser;
typedef struct _OGMDvdDriveChooserInterface OGMDvdDriveChooserInterface;

struct _OGMDvdDriveChooserInterface
{
  GTypeInterface base_iface;

  /*
   * Methods
   */
  gchar * (*get_device) (OGMDvdDriveChooser *chooser,
                         OGMDvdDeviceType   *type);

  /*
   * Signals
   */
  void (* device_changed) (OGMDvdDriveChooser *chooser, 
                           const char         *device_path,
                           OGMDvdDeviceType   type);
};


GType   ogmdvd_drive_chooser_get_type   (void) G_GNUC_CONST;

gchar * ogmdvd_drive_chooser_get_device (OGMDvdDriveChooser *chooser,
                                         OGMDvdDeviceType   *type);

G_END_DECLS

#endif /* __OGMDVD_DRIVE_CHOOSER_H__ */
