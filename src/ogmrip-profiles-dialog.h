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

#ifndef __OGMRIP_PROFILES_DIALOG_H__
#define __OGMRIP_PROFILES_DIALOG_H__

#include <ogmrip-gtk.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_PROFILES_DIALOG            (ogmrip_profiles_dialog_get_type ())
#define OGMRIP_PROFILES_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_PROFILES_DIALOG, OGMRipProfilesDialog))
#define OGMRIP_PROFILES_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_PROFILES_DIALOG, OGMRipProfilesDialogClass))
#define OGMRIP_IS_PROFILES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_PROFILES_DIALOG))
#define OGMRIP_IS_PROFILES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_PROFILES_DIALOG))

typedef struct _OGMRipProfilesDialog      OGMRipProfilesDialog;
typedef struct _OGMRipProfilesDialogClass OGMRipProfilesDialogClass;
typedef struct _OGMRipProfilesDialogPriv  OGMRipProfilesDialogPriv;

struct _OGMRipProfilesDialog
{
  GtkDialog parent_instance;

  OGMRipProfilesDialogPriv *priv;
};

struct _OGMRipProfilesDialogClass
{
  GtkDialogClass parent_class;
};

GType       ogmrip_profiles_dialog_get_type (void);
GtkWidget * ogmrip_profiles_dialog_new      (void);

G_END_DECLS

#endif /* __OGMRIP_PROFILES_DIALOG_H__ */

