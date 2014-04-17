/* OGMRip - A media encoder for GNOME
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_SPELL_DIALOG_H__
#define __OGMRIP_SPELL_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_SPELL_DIALOG            (ogmrip_spell_dialog_get_type ())
#define OGMRIP_SPELL_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_SPELL_DIALOG, OGMRipSpellDialog))
#define OGMRIP_SPELL_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_SPELL_DIALOG, OGMRipSpellDialogClass))
#define OGMRIP_IS_SPELL_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_SPELL_DIALOG))
#define OGMRIP_IS_SPELL_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_SPELL_DIALOG))

typedef struct _OGMRipSpellDialog      OGMRipSpellDialog;
typedef struct _OGMRipSpellDialogClass OGMRipSpellDialogClass;
typedef struct _OGMRipSpellDialogPriv  OGMRipSpellDialogPrivate;

struct _OGMRipSpellDialog
{
  GtkDialog parent_instance;

  OGMRipSpellDialogPrivate *priv;
};

struct _OGMRipSpellDialogClass
{
  GtkDialogClass parent_class;
};

GType         ogmrip_spell_dialog_get_type    (void);

GtkWidget *   ogmrip_spell_dialog_new         (const gchar       *language);

gboolean      ogmrip_spell_dialog_check_word  (OGMRipSpellDialog *dialog, 
                                               const gchar       *word,
                                               gint              offset,
                                               gchar             **corrected);
gboolean      ogmrip_spell_dialog_check_text  (OGMRipSpellDialog *dialog, 
                                               const gchar       *text,
                                               gchar             **corrected);

G_END_DECLS

#endif /* __OGMRIP_SPELL_DIALOG_H__ */

