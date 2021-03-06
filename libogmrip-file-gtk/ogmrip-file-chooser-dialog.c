/* OGMRipFile - A file library for OGMRip
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-file-chooser-dialog.h"
#include "ogmrip-language-chooser-widget.h"

#include <glib/gi18n-lib.h>

struct _OGMRipFileChooserDialogPriv
{
  GtkWidget *lang_chooser;
};

static void ogmrip_file_chooser_dialog_constructed (GObject *gobject);

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (OGMRipFileChooserDialog, ogmrip_file_chooser_dialog, GTK_TYPE_FILE_CHOOSER_DIALOG);

static void
ogmrip_file_chooser_dialog_class_init (OGMRipFileChooserDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_file_chooser_dialog_constructed;
}

static void
ogmrip_file_chooser_dialog_init (OGMRipFileChooserDialog *dialog)
{
  dialog->priv = ogmrip_file_chooser_dialog_get_instance_private (dialog);

}

static void
ogmrip_file_chooser_dialog_constructed (GObject *gobject)
{
  OGMRipFileChooserDialog *dialog = OGMRIP_FILE_CHOOSER_DIALOG (gobject);
  GtkWidget *hbox, *label;

  GtkTreeModel *model;
  GtkTreeIter iter;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Language:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  dialog->priv->lang_chooser = ogmrip_language_chooser_widget_new ();
  gtk_box_pack_start (GTK_BOX (hbox), dialog->priv->lang_chooser, TRUE, TRUE, 0);
  gtk_widget_show (dialog->priv->lang_chooser);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (dialog->priv->lang_chooser));
  gtk_list_store_prepend (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      OGMRIP_LANGUAGE_STORE_NAME_COLUMN, _("None"),
      OGMRIP_LANGUAGE_STORE_CODE_COLUMN, 0, -1);
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->lang_chooser), 0);

  G_OBJECT_CLASS (ogmrip_file_chooser_dialog_parent_class)->constructed (gobject);
}

OGMRipFile *
ogmrip_file_chooser_dialog_get_file (OGMRipFileChooserDialog *chooser, GError **error)
{
  OGMRipFileChooserDialogClass *klass;
  OGMRipFile *file;
  gint lang;

  g_return_val_if_fail (OGMRIP_IS_FILE_CHOOSER_DIALOG (chooser), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  klass = OGMRIP_FILE_CHOOSER_DIALOG_GET_CLASS (chooser);

  if (!klass->get_file)
    return NULL;

  file = (* klass->get_file) (chooser, error);
  if (!file)
    return NULL;

  lang = ogmrip_language_chooser_widget_get_active (OGMRIP_LANGUAGE_CHOOSER_WIDGET (chooser->priv->lang_chooser));

  if (OGMRIP_IS_AUDIO_FILE (file))
    ogmrip_audio_file_set_language (OGMRIP_AUDIO_FILE (file), lang);
  else
    ogmrip_subp_file_set_language (OGMRIP_SUBP_FILE (file), lang);

  return file;
}

