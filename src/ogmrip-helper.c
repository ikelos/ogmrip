/* OGMRip - A library for DVD ripping and encoding
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

/**
 * SECTION:ogmrip-helper
 * @title: Helper
 * @include: ogmrip-source-chooser.h
 * @short_description: A list of helper functions
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-helper.h"
#include "ogmrip-profile-store.h"

#include <glib/gi18n-lib.h>

#include <stdlib.h>
#include <locale.h>

#include <ogmrip-dvd.h>

extern const gchar *ogmdvd_languages[][3];
extern const guint  ogmdvd_nlanguages;

static void
gtk_dialog_response_accept (GtkDialog *dialog)
{
  gtk_dialog_response (dialog, GTK_RESPONSE_ACCEPT);
}

static gboolean
ogmrip_drive_eject_idle (OGMDvdDrive *drive)
{
  ogmdvd_drive_eject (drive);

  return FALSE;
}

gboolean
ogmrip_open_title (GtkWindow *parent, OGMRipTitle *title)
{
  GError *error = NULL;
  OGMDvdMonitor *monitor;
  OGMDvdDrive *drive;
  GtkWidget *dialog;
  const gchar *uri;

  g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), FALSE);
  g_return_val_if_fail (OGMRIP_IS_TITLE (title), FALSE);

  if (ogmrip_title_is_open (title))
    return TRUE;

  if (ogmrip_title_open (title, &error))
    return TRUE;

  uri = ogmrip_media_get_uri (ogmrip_title_get_media (title));

  monitor = ogmdvd_monitor_get_default ();
  drive = ogmdvd_monitor_get_drive (monitor, uri + 6);
  g_object_unref (monitor);

  if (!drive)
    return FALSE;

  dialog = gtk_message_dialog_new_with_markup (parent,
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_INFO, GTK_BUTTONS_CANCEL, "<b>%s</b>\n\n%s",
      ogmrip_media_get_label (ogmrip_title_get_media (title)),
      _("Please insert the DVD required to encode this title."));

  g_signal_connect (dialog, "delete-event",
      G_CALLBACK (gtk_widget_hide_on_delete), NULL);
  g_signal_connect_swapped (drive, "medium-added",
      G_CALLBACK (gtk_dialog_response_accept), dialog);

  do
  {
    g_clear_error (&error);
    if (ogmrip_title_open (title, &error))
      break;

    if (error && error->code != OGMDVD_DISC_ERROR_ID)
      break;

    g_idle_add ((GSourceFunc) ogmrip_drive_eject_idle, drive);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_CANCEL)
      break;
  }
  while (1);

  g_object_unref (drive);
  gtk_widget_destroy (dialog);
  g_clear_error (&error);

  return ogmrip_title_is_open (title);
}

