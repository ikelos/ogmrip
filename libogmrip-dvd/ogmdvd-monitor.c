/* OGMDvd - A wrapper library around libdvdread
 * Copyright (C) 2009-2010 Olivier Rolland <billl@users.sourceforge.net>
 *
 * Code from libbrasero-media
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Libbrasero-media is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Libbrasero-media is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * SECTION:ogmdvd-monitor
 * @title: OGMDvdMonitor
 * @include: ogmdvd-monitor.h
 * @short_description: An object monitoring optical drives
 */

#include "ogmdvd-monitor.h"
#include "ogmdvd-device.h"

#include <glib.h>
#include <gio/gio.h>

struct _OGMDvdMonitorPriv
{
  GSList *drives;

  GSList *waiting_removal;
  guint waiting_removal_id;

  GVolumeMonitor *gmonitor;
};

#define OGMDVD_MONITOR_GET_PRIV(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMDVD_TYPE_MONITOR, OGMDvdMonitorPriv))

enum
{
  DRIVE_ADDED,
  DRIVE_REMOVED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (OGMDvdMonitor, ogmdvd_monitor, G_TYPE_OBJECT);

static gboolean
ogmdvd_monitor_is_drive (OGMDvdMonitor *monitor, const gchar *device)
{
  gpointer handle;
  gboolean res;

  handle = ogmdvd_device_open (device, FALSE);
  if (!handle)
    return FALSE;

  res = ogmdvd_device_inquiry (handle, NULL, NULL);

  /*
   * vérifier si le lecteur sait lire les dvd ?
   */

  ogmdvd_device_close (handle);

  return res;
}

static void
ogmdvd_monitor_add_drive (OGMDvdMonitor *self, const gchar *device, GDrive *gdrive)
{
  OGMDvdDrive *drive;

  drive = ogmdvd_monitor_get_drive (self, device);
  if (drive)
  {
    self->priv->waiting_removal = g_slist_remove (self->priv->waiting_removal, drive);
    g_object_set (drive, "gdrive", gdrive, NULL);
    g_object_unref (drive);
  }
  else if (ogmdvd_monitor_is_drive (self, device))
  {
    drive = g_object_new (OGMDVD_TYPE_DRIVE, "gdrive", gdrive, NULL);
    self->priv->drives = g_slist_prepend (self->priv->drives, drive);

    g_signal_emit (self, signals [DRIVE_ADDED], 0, drive);
  }
}

static gboolean
ogmdvd_monitor_disconnected (OGMDvdMonitor *self)
{
  OGMDvdDrive *drive;

  if (!self->priv->waiting_removal)
  {
    self->priv->waiting_removal_id = 0;
    return FALSE;
  }

  drive = self->priv->waiting_removal->data;

  self->priv->waiting_removal = g_slist_remove (self->priv->waiting_removal, drive);
  self->priv->drives = g_slist_remove (self->priv->drives, drive);

  g_signal_emit (self, signals [DRIVE_REMOVED], 0, drive);
  g_object_unref (drive);

  return TRUE;
}

static void
ogmdvd_monitor_remove_drive (OGMDvdMonitor *self, const gchar *device, GDrive *gdrive)
{
  OGMDvdDrive *drive;

  drive = ogmdvd_monitor_get_drive (self, device);
  if (drive)
  {
    if (g_slist_find (self->priv->waiting_removal, drive) != NULL)
      g_object_unref (drive);
    else
    {
      GDrive *associated_gdrive;

      associated_gdrive = ogmdvd_drive_get_gdrive (drive);
      if (associated_gdrive == gdrive)
      {
        self->priv->waiting_removal = g_slist_append (self->priv->waiting_removal, drive);

        if (!self->priv->waiting_removal_id)
          self->priv->waiting_removal_id = g_timeout_add_seconds (2, (GSourceFunc) ogmdvd_monitor_disconnected, self);
      }
    }
    g_object_unref (drive);
  }
}

static void
ogmdvd_monitor_connected_cb (GVolumeMonitor *monitor, GDrive *gdrive, OGMDvdMonitor *self)
{
  gchar *device;

  device = g_drive_get_identifier (gdrive, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
  ogmdvd_monitor_add_drive (self, device, gdrive);
  g_free (device);
}

static void
ogmdvd_monitor_disconnected_cb (GVolumeMonitor *monitor, GDrive *gdrive, OGMDvdMonitor *self)
{
  gchar *device;

  device = g_drive_get_identifier (gdrive, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
  ogmdvd_monitor_remove_drive (self, device, gdrive);
  g_free (device);
}

static void
ogmdvd_monitor_init (OGMDvdMonitor *self)
{
  GList *iter, *drives;
  GDrive *gdrive;

  OGMDvdDrive *drive;
  gchar *device;

  self->priv = OGMDVD_MONITOR_GET_PRIV (self);

  self->priv->gmonitor = g_volume_monitor_get ();

  drives = g_volume_monitor_get_connected_drives (self->priv->gmonitor);
  for (iter = drives; iter; iter = iter->next)
  {
    gdrive = iter->data;

    device = g_drive_get_identifier (gdrive, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
    if (ogmdvd_monitor_is_drive (self, device))
    {
      drive = g_object_new (OGMDVD_TYPE_DRIVE, "gdrive", gdrive, NULL);
      self->priv->drives = g_slist_prepend (self->priv->drives, drive);
    }
    g_free (device);
  }
  g_list_foreach (drives, (GFunc) g_object_unref, NULL);
  g_list_free (drives);

  g_signal_connect (self->priv->gmonitor, "drive-connected",
      G_CALLBACK (ogmdvd_monitor_connected_cb), self);
  g_signal_connect (self->priv->gmonitor, "drive-disconnected",
      G_CALLBACK (ogmdvd_monitor_disconnected_cb), self);
}

static void
ogmdvd_monitor_finalize (GObject *object)
{
  OGMDvdMonitor *self;

  self = OGMDVD_MONITOR (object);

  if (self->priv->waiting_removal_id)
  {
    g_source_remove (self->priv->waiting_removal_id);
    self->priv->waiting_removal_id = 0;
  }

  if (self->priv->waiting_removal)
  {
    g_slist_free (self->priv->waiting_removal);
    self->priv->waiting_removal = NULL;
  }

  if (self->priv->drives)
  {
    g_slist_foreach (self->priv->drives, (GFunc) g_object_unref, NULL);
    g_slist_free (self->priv->drives);
    self->priv->drives = NULL;
  }

  if (self->priv->gmonitor)
  {
    g_signal_handlers_disconnect_by_func (self->priv->gmonitor,
                                          ogmdvd_monitor_connected_cb,
                                          self);
    g_signal_handlers_disconnect_by_func (self->priv->gmonitor,
                                          ogmdvd_monitor_disconnected_cb,
                                          self);
    g_object_unref (self->priv->gmonitor);
  }

  G_OBJECT_CLASS (ogmdvd_monitor_parent_class)->finalize (object);
}

static void
ogmdvd_monitor_class_init (OGMDvdMonitorClass *klass)
{
  GObjectClass* object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = ogmdvd_monitor_finalize;

  signals[DRIVE_ADDED] = g_signal_new ("drive_added",
                                       G_OBJECT_CLASS_TYPE (klass),
                                       G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                                       G_STRUCT_OFFSET (OGMDvdMonitorClass, drive_added),
                                       NULL, NULL,
                                       g_cclosure_marshal_VOID__OBJECT,
                                       G_TYPE_NONE, 1,
                                       OGMDVD_TYPE_DRIVE);

  signals[DRIVE_REMOVED] = g_signal_new ("drive_removed",
                                         G_OBJECT_CLASS_TYPE (klass),
                                         G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                                         G_STRUCT_OFFSET (OGMDvdMonitorClass, drive_removed),
                                         NULL, NULL,
                                         g_cclosure_marshal_VOID__OBJECT,
                                         G_TYPE_NONE, 1,
                                         OGMDVD_TYPE_DRIVE);

  g_type_class_add_private (klass, sizeof (OGMDvdMonitorPriv));
}

static OGMDvdMonitor *singleton = NULL;

/**
 * ogmdvd_monitor_get_default:
 *
 * Gets the default monitor.
 *
 * Returns: an #OGMDvdMonitor
 **/
OGMDvdMonitor *
ogmdvd_monitor_get_default (void)
{
  if (!singleton)
    singleton = g_object_new (OGMDVD_TYPE_MONITOR, NULL);

  g_object_ref (singleton);

  return singleton;
}

/**
 * ogmdvd_monitor_get_drive:
 * @monitor: #OGMDvdMonitor
 * @device: a device path
 *
 * Gets the drive with the given device in any.
 *
 * Returns: an #OGMDvdDrive or NULL
 **/
OGMDvdDrive *
ogmdvd_monitor_get_drive (OGMDvdMonitor *monitor, const gchar *device)
{
  GSList *iter;
  OGMDvdDrive *drive;
  const gchar *dev;

  g_return_val_if_fail (OGMDVD_IS_MONITOR (monitor), NULL);
  g_return_val_if_fail (device != NULL, NULL);

  for (iter = monitor->priv->drives; iter; iter = iter->next)
  {
    drive = iter->data;

    dev = ogmdvd_drive_get_device (drive);
    if (dev && g_str_equal (dev, device))
    {
      g_object_ref (drive);
      return drive;
    }
  }

  return NULL;
}

/**
 * ogmdvd_monitor_get_drives:
 * @monitor: #OGMDvdMonitor
 *
 * Get the list of available #OGMDvdDrive.
 *
 * Returns: a list of optical drives
 **/
GSList *
ogmdvd_monitor_get_drives (OGMDvdMonitor *monitor)
{
  GSList *drives;

  g_return_val_if_fail (OGMDVD_IS_MONITOR (monitor), NULL);

  drives = g_slist_copy (monitor->priv->drives);
  g_slist_foreach (drives, (GFunc) g_object_ref, NULL);

  return drives;
}