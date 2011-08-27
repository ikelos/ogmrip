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

#include <gtk/gtk.h>

G_BEGIN_DECLS

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

enum
{
  OGMRIP_RESPONSE_SUSPEND,
  OGMRIP_RESPONSE_RESUME
};

GType       ogmrip_progress_dialog_get_type     (void);
GtkWidget * ogmrip_progress_dialog_new          (GtkWindow            *parent,
                                                 GtkDialogFlags       flags,
                                                 gboolean             can_suspend);
void        ogmrip_progress_dialog_set_title    (OGMRipProgressDialog *dialog,
                                                 const gchar          *title);
void        ogmrip_progress_dialog_set_message  (OGMRipProgressDialog *dialog,
                                                 const gchar          *message);
void        ogmrip_progress_dialog_set_fraction (OGMRipProgressDialog *dialog,
                                                 gdouble              fraction);

G_END_DECLS

#endif /* __OGMRIP_PROGRESS_DIALOG_H__ */

