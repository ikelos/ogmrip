/* OGMRipFile - A file library for OGMRip
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_SUBP_FILE_CHOOSER_DIALOG_H__
#define __OGMRIP_SUBP_FILE_CHOOSER_DIALOG_H__

#include <ogmrip-file-chooser-dialog.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_SUBP_FILE_CHOOSER_DIALOG            (ogmrip_subp_file_chooser_dialog_get_type ())
#define OGMRIP_SUBP_FILE_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_SUBP_FILE_CHOOSER_DIALOG, OGMRipSubpFileChooserDialog))
#define OGMRIP_SUBP_FILE_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_SUBP_FILE_CHOOSER_DIALOG, OGMRipSubpFileChooserDialogClass))
#define OGMRIP_IS_SUBP_FILE_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_SUBP_FILE_CHOOSER_DIALOG))
#define OGMRIP_IS_SUBP_FILE_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_SUBP_FILE_CHOOSER_DIALOG))

typedef struct _OGMRipSubpFileChooserDialog      OGMRipSubpFileChooserDialog;
typedef struct _OGMRipSubpFileChooserDialogClass OGMRipSubpFileChooserDialogClass;

struct _OGMRipSubpFileChooserDialog
{
  OGMRipFileChooserDialog parent_instance;
};

struct _OGMRipSubpFileChooserDialogClass
{
  OGMRipFileChooserDialogClass parent_class;
};

GType       ogmrip_subp_file_chooser_dialog_get_type (void);
GtkWidget * ogmrip_subp_file_chooser_dialog_new      (void);

G_END_DECLS

#endif /* __OGMRIP_SUBP_FILE_CHOOSER_DIALOG_H__ */

