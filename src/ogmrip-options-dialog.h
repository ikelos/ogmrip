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

#ifndef __OGMRIP_OPTIONS_DIALOG_H__
#define __OGMRIP_OPTIONS_DIALOG_H__

#include <ogmrip-gtk.h>

G_BEGIN_DECLS

enum
{
  OGMRIP_RESPONSE_EXTRACT,
  OGMRIP_RESPONSE_ENQUEUE,
  OGMRIP_RESPONSE_TEST
};

#define OGMRIP_TYPE_OPTIONS_DIALOG            (ogmrip_options_dialog_get_type ())
#define OGMRIP_OPTIONS_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_OPTIONS_DIALOG, OGMRipOptionsDialog))
#define OGMRIP_OPTIONS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_OPTIONS_DIALOG, OGMRipOptionsDialogClass))
#define OGMRIP_IS_OPTIONS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_OPTIONS_DIALOG))
#define OGMRIP_IS_OPTIONS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_OPTIONS_DIALOG))

typedef struct _OGMRipOptionsDialog      OGMRipOptionsDialog;
typedef struct _OGMRipOptionsDialogClass OGMRipOptionsDialogClass;
typedef struct _OGMRipOptionsDialogPriv  OGMRipOptionsDialogPriv;

struct _OGMRipOptionsDialog
{
  GtkDialog parent_instance;

  OGMRipOptionsDialogPriv *priv;
};

struct _OGMRipOptionsDialogClass
{
  GtkDialogClass parent_class;
};

GType       ogmrip_options_dialog_get_type         (void);

GtkWidget * ogmrip_options_dialog_new              (OGMRipEncoding      *encoding);
void        ogmrip_options_dialog_get_scale_size   (OGMRipOptionsDialog *dialog,
                                                    guint               *width,
                                                    guint               *height);
void        ogmrip_options_dialog_get_crop_size    (OGMRipOptionsDialog *dialog,
                                                    guint               *x,
                                                    guint               *y,
                                                    guint               *width,
                                                    guint               *height);
gint        ogmrip_options_dialog_get_deinterlacer (OGMRipOptionsDialog *dialog);

G_END_DECLS

#endif /* __OGMRIP_OPTIONS_DIALOG_H__ */

