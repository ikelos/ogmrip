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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-audio-file-chooser-dialog.h"

#include <ogmrip-file.h>

#include <glib/gi18n-lib.h>

static void ogmrip_audio_file_chooser_dialog_constructed (GObject *gobject);

static OGMRipFile *
ogmrip_audio_file_chooser_dialog_get_file (OGMRipFileChooserDialog *dialog, GError **error)
{
  OGMRipMedia *file;
  gchar *uri;

  uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
  file = ogmrip_audio_file_new (uri);
  g_free (uri);

  if (!file)
  {
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, _("Could not open '%s'"), uri);
    return NULL;
  }

  return OGMRIP_FILE (file);
}

G_DEFINE_TYPE (OGMRipAudioFileChooserDialog, ogmrip_audio_file_chooser_dialog, OGMRIP_TYPE_FILE_CHOOSER_DIALOG);

static void
ogmrip_audio_file_chooser_dialog_class_init (OGMRipAudioFileChooserDialogClass *klass)
{
  GObjectClass *gobject_class;
  OGMRipFileChooserDialogClass *dialog_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_audio_file_chooser_dialog_constructed;

  dialog_class = OGMRIP_FILE_CHOOSER_DIALOG_CLASS (klass);
  dialog_class->get_file = ogmrip_audio_file_chooser_dialog_get_file;
}

static void
ogmrip_audio_file_chooser_dialog_init (OGMRipAudioFileChooserDialog *dialog)
{
}

static void
ogmrip_audio_file_chooser_dialog_constructed (GObject *gobject)
{
  OGMRipAudioFileChooserDialog *dialog = OGMRIP_AUDIO_FILE_CHOOSER_DIALOG (gobject);
  GtkFileFilter *filter;

  gtk_file_chooser_set_action (GTK_FILE_CHOOSER (dialog), GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
      GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
      GTK_RESPONSE_ACCEPT, GTK_RESPONSE_REJECT, -1);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Select an audio file"));

  filter = gtk_file_filter_new ();
  gtk_file_filter_add_mime_type (filter, "audio/?*");
  gtk_file_filter_add_mime_type (filter, "application/ogg");
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

  G_OBJECT_CLASS (ogmrip_audio_file_chooser_dialog_parent_class)->constructed (gobject);
}

GtkWidget *
ogmrip_audio_file_chooser_dialog_new (void)
{
  return g_object_new (OGMRIP_TYPE_AUDIO_FILE_CHOOSER_DIALOG, NULL);
}

