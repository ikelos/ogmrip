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

#ifndef __OGMDVD_DRIVE_CHOOSER_WIDGET_H__
#define __OGMDVD_DRIVE_CHOOSER_WIDGET_H__

#include <ogmrip-media-chooser.h>

G_BEGIN_DECLS

#define OGMDVD_TYPE_DRIVE_CHOOSER_WIDGET            (ogmdvd_drive_chooser_widget_get_type ())
#define OGMDVD_DRIVE_CHOOSER_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMDVD_TYPE_DRIVE_CHOOSER_WIDGET, OGMDvdDriveChooserWidget))
#define OGMDVD_DRIVE_CHOOSER_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMDVD_TYPE_DRIVE_CHOOSER_WIDGET, OGMDvdDriveChooserWidgetClass))
#define OGMDVD_IS_DRIVE_CHOOSER_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMDVD_TYPE_DRIVE_CHOOSER_WIDGET))
#define OGMDVD_IS_DRIVE_CHOOSER_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMDVD_TYPE_DRIVE_CHOOSER_WIDGET))

typedef struct _OGMDvdDriveChooserWidget      OGMDvdDriveChooserWidget;
typedef struct _OGMDvdDriveChooserWidgetClass OGMDvdDriveChooserWidgetClass;
typedef struct _OGMDvdDriveChooserWidgetPriv  OGMDvdDriveChooserWidgetPriv;

struct _OGMDvdDriveChooserWidget
{
  GtkComboBox parent_instance;
  OGMDvdDriveChooserWidgetPriv *priv;
};

struct _OGMDvdDriveChooserWidgetClass
{
  GtkComboBoxClass parent_class;
};

GType       ogmdvd_drive_chooser_widget_get_type (void);
GtkWidget * ogmdvd_drive_chooser_widget_new      (void);

G_END_DECLS

#endif /* __OGMDVD_DRIVE_CHOOSER_WIDGET_H__ */

