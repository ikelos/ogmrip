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

#ifndef __OGMRIP_UPDATE_DIALOG_H__
#define __OGMRIP_UPDATE_DIALOG_H__

#include <ogmrip-encode-gtk.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_UPDATE_DIALOG            (ogmrip_update_dialog_get_type ())
#define OGMRIP_UPDATE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_UPDATE_DIALOG, OGMRipUpdateDialog))
#define OGMRIP_UPDATE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_UPDATE_DIALOG, OGMRipUpdateDialogClass))
#define OGMRIP_IS_UPDATE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_UPDATE_DIALOG))
#define OGMRIP_IS_UPDATE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_UPDATE_DIALOG))

typedef struct _OGMRipUpdateDialog      OGMRipUpdateDialog;
typedef struct _OGMRipUpdateDialogClass OGMRipUpdateDialogClass;
typedef struct _OGMRipUpdateDialogPriv  OGMRipUpdateDialogPriv;

struct _OGMRipUpdateDialog
{
  GtkDialog parent_instance;

  OGMRipUpdateDialogPriv *priv;
};

struct _OGMRipUpdateDialogClass
{
  GtkDialogClass parent_class;
};

GType       ogmrip_update_dialog_get_type     (void);
GtkWidget * ogmrip_update_dialog_new          (void);
void        ogmrip_update_dialog_add_profile  (OGMRipUpdateDialog *dialog,
                                               OGMRipProfile      *profile);
GList *     ogmrip_update_dialog_get_profiles (OGMRipUpdateDialog *dialog);

G_END_DECLS

#endif /* __OGMRIP_UPDATE_DIALOG_H__ */

