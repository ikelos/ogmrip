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

#ifndef __OGMRIP_CROP_DIALOG_H__
#define __OGMRIP_CROP_DIALOG_H__

#include <ogmrip-encode-gtk.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_CROP_DIALOG            (ogmrip_crop_dialog_get_type ())
#define OGMRIP_CROP_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_CROP_DIALOG, OGMRipCropDialog))
#define OGMRIP_CROP_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_CROP_DIALOG, OGMRipCropDialogClass))
#define OGMRIP_IS_CROP_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_CROP_DIALOG))
#define OGMRIP_IS_CROP_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_CROP_DIALOG))

typedef struct _OGMRipCropDialog      OGMRipCropDialog;
typedef struct _OGMRipCropDialogClass OGMRipCropDialogClass;
typedef struct _OGMRipCropDialogPriv  OGMRipCropDialogPrivate;

struct _OGMRipCropDialog
{
  GtkDialog parent_instance;

  OGMRipCropDialogPrivate *priv;
};

struct _OGMRipCropDialogClass
{
  GtkDialogClass parent_class;
};

GType       ogmrip_crop_dialog_get_type         (void);
GtkWidget * ogmrip_crop_dialog_new              (OGMRipTitle      *title,
                                                 guint            left,
                                                 guint            top,
                                                 guint            right,
                                                 guint            bottom);
void        ogmrip_crop_dialog_get_crop         (OGMRipCropDialog *dialog, 
                                                 guint            *left, 
                                                 guint            *top, 
                                                 guint            *right, 
                                                 guint            *bottom);
void        ogmrip_crop_dialog_set_deinterlacer (OGMRipCropDialog *dialog,
                                                 gboolean         deint);

G_END_DECLS

#endif /* __OGMRIP_CROP_DIALOG_H__ */

