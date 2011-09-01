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

#ifndef __OGMRIP_SUBP_OPTIONS_DIALOG_H__
#define __OGMRIP_SUBP_OPTIONS_DIALOG_H__

#include <ogmrip-subp-options.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_SUBP_OPTIONS_DIALOG            (ogmrip_subp_options_dialog_get_type ())
#define OGMRIP_SUBP_OPTIONS_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_SUBP_OPTIONS_DIALOG, OGMRipSubpOptionsDialog))
#define OGMRIP_SUBP_OPTIONS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_SUBP_OPTIONS_DIALOG, OGMRipSubpOptionsDialogClass))
#define OGMRIP_IS_SUBP_OPTIONS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_SUBP_OPTIONS_DIALOG))
#define OGMRIP_IS_SUBP_OPTIONS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_SUBP_OPTIONS_DIALOG))

typedef struct _OGMRipSubpOptionsDialog      OGMRipSubpOptionsDialog;
typedef struct _OGMRipSubpOptionsDialogClass OGMRipSubpOptionsDialogClass;
typedef struct _OGMRipSubpOptionsDialogPriv  OGMRipSubpOptionsDialogPriv;

struct _OGMRipSubpOptionsDialog
{
  GtkDialog parent_instance;

  OGMRipSubpOptionsDialogPriv *priv;
};

struct _OGMRipSubpOptionsDialogClass
{
  GtkDialogClass parent_class;
};

GType         ogmrip_subp_options_dialog_get_type         (void);
GtkWidget *   ogmrip_subp_options_dialog_new              (void);
const gchar * ogmrip_subp_options_dialog_get_label        (OGMRipSubpOptionsDialog *dialog);
void          ogmrip_subp_options_dialog_set_label        (OGMRipSubpOptionsDialog *dialog,
                                                           const gchar             *label);
gint          ogmrip_subp_options_dialog_get_language     (OGMRipSubpOptionsDialog *dialog);
void          ogmrip_subp_options_dialog_set_language     (OGMRipSubpOptionsDialog *dialog,
                                                           gint                    lang);
gboolean      ogmrip_subp_options_dialog_get_use_defaults (OGMRipSubpOptionsDialog *dialog);
void          ogmrip_subp_options_dialog_set_use_defaults (OGMRipSubpOptionsDialog *dialog,
                                                           gboolean                use_defaults);

G_END_DECLS

#endif /* __OGMRIP_SUBP_OPTIONS_DIALOG_H__ */
