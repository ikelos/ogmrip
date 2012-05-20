/* OGMRipFile - A file library for OGMRip
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_FILE_CHOOSER_DIALOG_H__
#define __OGMRIP_FILE_CHOOSER_DIALOG_H__

#include <gtk/gtk.h>

#include <ogmrip-file.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_FILE_CHOOSER_DIALOG            (ogmrip_file_chooser_dialog_get_type ())
#define OGMRIP_FILE_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_FILE_CHOOSER_DIALOG, OGMRipFileChooserDialog))
#define OGMRIP_FILE_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_FILE_CHOOSER_DIALOG, OGMRipFileChooserDialogClass))
#define OGMRIP_IS_FILE_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_FILE_CHOOSER_DIALOG))
#define OGMRIP_IS_FILE_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_FILE_CHOOSER_DIALOG))
#define OGMRIP_FILE_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_FILE_CHOOSER_DIALOG, OGMRipFileChooserDialogClass))

typedef struct _OGMRipFileChooserDialog      OGMRipFileChooserDialog;
typedef struct _OGMRipFileChooserDialogClass OGMRipFileChooserDialogClass;
typedef struct _OGMRipFileChooserDialogPriv  OGMRipFileChooserDialogPriv;

struct _OGMRipFileChooserDialog
{
  GtkFileChooserDialog parent_instance;

  OGMRipFileChooserDialogPriv *priv;
};

struct _OGMRipFileChooserDialogClass
{
  GtkFileChooserDialogClass parent_class;

  OGMRipFile * (* get_file) (OGMRipFileChooserDialog *dialog,
                             GError                  **error);
};

GType        ogmrip_file_chooser_dialog_get_type (void);
OGMRipFile * ogmrip_file_chooser_dialog_get_file (OGMRipFileChooserDialog *chooser,
                                                  GError                  **error);

G_END_DECLS

#endif /* __OGMRIP_FILE_CHOOSER_DIALOG_H__ */

