/* OGMRip - A wrapper library around libdvdread
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

#ifndef __OGMRIP_MEDIA_CHOOSER_DIALOG_H__
#define __OGMRIP_MEDIA_CHOOSER_DIALOG_H__

#include <ogmrip-media-chooser.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_MEDIA_CHOOSER_DIALOG            (ogmrip_media_chooser_dialog_get_type ())
#define OGMRIP_MEDIA_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MEDIA_CHOOSER_DIALOG, OGMRipMediaChooserDialog))
#define OGMRIP_MEDIA_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_MEDIA_CHOOSER_DIALOG, OGMRipMediaChooserDialogClass))
#define OGMRIP_IS_MEDIA_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_MEDIA_CHOOSER_DIALOG))
#define OGMRIP_IS_MEDIA_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_MEDIA_CHOOSER_DIALOG))

typedef struct _OGMRipMediaChooserDialog      OGMRipMediaChooserDialog;
typedef struct _OGMRipMediaChooserDialogClass OGMRipMediaChooserDialogClass;
typedef struct _OGMRipMediaChooserDialogPriv  OGMRipMediaChooserDialogPriv;

struct _OGMRipMediaChooserDialog
{
  GtkDialog parent_instance;
  OGMRipMediaChooserDialogPriv *priv;
};

struct _OGMRipMediaChooserDialogClass
{
  GtkDialogClass parent_class;

  void (* eject) (OGMRipMediaChooserDialog *dialog);
};

GType       ogmrip_media_chooser_dialog_get_type (void);
GtkWidget * ogmrip_media_chooser_dialog_new      (void);

G_END_DECLS

#endif /* __OGMRIP_MEDIA_CHOOSER_DIALOG_H__ */

