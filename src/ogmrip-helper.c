/* OGMRip - A media encoder for GNOME
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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

#include <glib/gi18n-lib.h>
#include <ogmrip-dvd.h>

static void
gtk_dialog_response_accept (GtkDialog *dialog)
{
  gtk_dialog_response (dialog, GTK_RESPONSE_ACCEPT);
}

static GVolume *
g_volume_monitor_get_volume_for_device (GVolumeMonitor *monitor, const gchar *device)
{
  GList *volumes, *volume;
  gboolean found = FALSE;
  gchar *dev;

  volumes = g_volume_monitor_get_volumes (monitor);
  for (volume = volumes; !found && volume; volume = volume->next)
  {
    dev = g_volume_get_identifier (volume->data, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
    found = g_str_equal (dev, device);
    g_free (dev);
  }

  if (found)
    g_object_ref (volume->data);

  g_list_foreach (volumes, (GFunc) g_object_unref, NULL);
  g_list_free (volumes);

  return volume ? volume->data : NULL;
}

gboolean
ogmrip_open_title (GtkWindow *parent, OGMRipTitle *title, GError **error)
{
  GVolumeMonitor *monitor;
  GVolume *volume;

  GtkWidget *dialog;
  const gchar *uri;

  g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), FALSE);
  g_return_val_if_fail (OGMRIP_IS_TITLE (title), FALSE);

  if (ogmrip_title_is_open (title))
    return TRUE;

  if (ogmrip_title_open (title, NULL, NULL, NULL, NULL))
    return TRUE;

  dialog = gtk_message_dialog_new_with_markup (parent,
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_INFO, GTK_BUTTONS_CANCEL, "<b>%s</b>\n\n%s",
      ogmrip_media_get_label (ogmrip_title_get_media (title)),
      _("Please insert the DVD required to encode this title."));
  g_signal_connect (dialog, "delete-event",
      G_CALLBACK (gtk_widget_hide_on_delete), NULL);

  monitor = g_volume_monitor_get ();
  g_signal_connect_swapped (monitor, "volume-added",
      G_CALLBACK (gtk_dialog_response_accept), dialog);

  uri = ogmrip_media_get_uri (ogmrip_title_get_media (title));

  do
  {
    volume = g_volume_monitor_get_volume_for_device (monitor, uri + 6);
    if (volume)
    {
      g_volume_eject_with_operation (volume, G_MOUNT_UNMOUNT_NONE, NULL, NULL, NULL, NULL);
      g_object_unref (volume);
    }

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_CANCEL)
      break;

    if (ogmrip_title_open (title, NULL, NULL, NULL, error))
      break;

    if (*error && (*error)->code != OGMDVD_DISC_ERROR_ID)
      break;

    g_clear_error (error);
  }
  while (1);

  g_object_unref (monitor);

  gtk_widget_destroy (dialog);

  return ogmrip_title_is_open (title);
}

