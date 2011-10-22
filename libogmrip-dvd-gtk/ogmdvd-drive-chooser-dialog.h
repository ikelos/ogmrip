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

#ifndef __OGMDVD_DRIVE_CHOOSER_DIALOG_H__
#define __OGMDVD_DRIVE_CHOOSER_DIALOG_H__

#include <ogmrip-media-chooser.h>

G_BEGIN_DECLS

#define OGMDVD_TYPE_DRIVE_CHOOSER_DIALOG            (ogmdvd_drive_chooser_dialog_get_type ())
#define OGMDVD_DRIVE_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMDVD_TYPE_DRIVE_CHOOSER_DIALOG, OGMDvdDriveChooserDialog))
#define OGMDVD_DRIVE_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMDVD_TYPE_DRIVE_CHOOSER_DIALOG, OGMDvdDriveChooserDialogClass))
#define OGMDVD_IS_DRIVE_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMDVD_TYPE_DRIVE_CHOOSER_DIALOG))
#define OGMDVD_IS_DRIVE_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMDVD_TYPE_DRIVE_CHOOSER_DIALOG))

typedef struct _OGMDvdDriveChooserDialog      OGMDvdDriveChooserDialog;
typedef struct _OGMDvdDriveChooserDialogClass OGMDvdDriveChooserDialogClass;
typedef struct _OGMDvdDriveChooserDialogPriv  OGMDvdDriveChooserDialogPriv;

struct _OGMDvdDriveChooserDialog
{
  GtkDialog parent_instance;
  OGMDvdDriveChooserDialogPriv *priv;
};

struct _OGMDvdDriveChooserDialogClass
{
  GtkDialogClass parent_class;

  void (* eject) (OGMDvdDriveChooserDialog *dialog);
};

GType       ogmdvd_drive_chooser_dialog_get_type (void);
GtkWidget * ogmdvd_drive_chooser_dialog_new      (void);

G_END_DECLS

#endif /* __OGMDVD_DRIVE_CHOOSER_DIALOG_H__ */

