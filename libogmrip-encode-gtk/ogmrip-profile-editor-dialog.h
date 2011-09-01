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

#ifndef __OGMRIP_PROFILE_EDITOR_DIALOG_H__
#define __OGMRIP_PROFILE_EDITOR_DIALOG_H__

#include <ogmrip-encode.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_PROFILE_EDITOR_DIALOG            (ogmrip_profile_editor_dialog_get_type ())
#define OGMRIP_PROFILE_EDITOR_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_PROFILE_EDITOR_DIALOG, OGMRipProfileEditorDialog))
#define OGMRIP_PROFILE_EDITOR_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_PROFILE_EDITOR_DIALOG, OGMRipProfileEditorDialogClass))
#define OGMRIP_IS_PROFILE_EDITOR_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_PROFILE_EDITOR_DIALOG))
#define OGMRIP_IS_PROFILE_EDITOR_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_PROFILE_EDITOR_DIALOG))

typedef struct _OGMRipProfileEditorDialog      OGMRipProfileEditorDialog;
typedef struct _OGMRipProfileEditorDialogClass OGMRipProfileEditorDialogClass;
typedef struct _OGMRipProfileEditorDialogPriv  OGMRipProfileEditorDialogPriv;

struct _OGMRipProfileEditorDialog
{
  GtkDialog parent_instance;

  OGMRipProfileEditorDialogPriv *priv;
};

struct _OGMRipProfileEditorDialogClass
{
  GtkDialogClass parent_class;
};

GType       ogmrip_profile_editor_dialog_get_type (void);
GtkWidget * ogmrip_profile_editor_dialog_new      (OGMRipProfile *profile);

G_END_DECLS

#endif /* __OGMRIP_PROFILE_EDITOR_DIALOG_H__ */
