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

#ifndef __OGMRIP_PROGRESS_DIALOG_H__
#define __OGMRIP_PROGRESS_DIALOG_H__

#include <ogmrip-gtk.h>

G_BEGIN_DECLS

enum
{
  OGMRIP_RESPONSE_CANCEL_ALL,
  OGMRIP_RESPONSE_SUSPEND,
  OGMRIP_RESPONSE_RESUME
};

#define OGMRIP_TYPE_PROGRESS_DIALOG            (ogmrip_progress_dialog_get_type ())
#define OGMRIP_PROGRESS_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_PROGRESS_DIALOG, OGMRipProgressDialog))
#define OGMRIP_PROGRESS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_PROGRESS_DIALOG, OGMRipProgressDialogClass))
#define OGMRIP_IS_PROGRESS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_PROGRESS_DIALOG))
#define OGMRIP_IS_PROGRESS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_PROGRESS_DIALOG))

typedef struct _OGMRipProgressDialog      OGMRipProgressDialog;
typedef struct _OGMRipProgressDialogClass OGMRipProgressDialogClass;
typedef struct _OGMRipProgressDialogPriv  OGMRipProgressDialogPriv;

struct _OGMRipProgressDialog
{
  GtkDialog parent_instance;

  OGMRipProgressDialogPriv *priv;
};

struct _OGMRipProgressDialogClass
{
  GtkDialogClass parent_class;
};

GType            ogmrip_progress_dialog_get_type       (void);

GtkWidget *      ogmrip_progress_dialog_new            (void);

void             ogmrip_progress_dialog_set_encoding   (OGMRipProgressDialog *dialog,
                                                        OGMRipEncoding       *encoding);
OGMRipEncoding * ogmrip_progress_dialog_get_encoding   (OGMRipProgressDialog *dialog);

void             ogmrip_progress_dialog_can_quit       (OGMRipProgressDialog *dialog,
                                                        gboolean             can_quit);
gboolean         ogmrip_progress_dialog_get_quit       (OGMRipProgressDialog *dialog);

G_END_DECLS

#endif /* __OGMRIP_PROGRESS_DIALOG_H__ */

