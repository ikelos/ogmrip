/* OGMDvd - A wrapper library around libdvdread
 * Copyright (C) 2009-2011 Olivier Rolland <billl@users.sourceforge.net>
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
 * SECTION:ogmdvd-drive
 * @title: OGMDvdDrive
 * @include: ogmdvd-drive.h
 * @short_description: An object representing an Optical Drive
 */

#include "ogmdvd-drive.h"
#include "ogmdvd-device.h"

#include <glib.h>
#include <errno.h>

struct _OGMDvdDrivePriv
{
  GDrive *gdrive;

  gchar *name;
  gchar *device;
  gint drive_type;

  gboolean has_medium;
  gboolean had_medium;
  gint  medium_type;

  gint probe_id;
};

#define OGMDVD_DRIVE_GET_PRIV(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMDVD_TYPE_DRIVE, OGMDvdDrivePriv))

enum
{
  MEDIUM_ADDED,
  MEDIUM_REMOVED,
  LAST_SIGNAL
};

enum {
  PROP_NONE  = 0,
  PROP_GDRIVE
};

static gulong signals [LAST_SIGNAL] = { 0 };

void g_drive_eject (GDrive               *drive,
                    GMountUnmountFlags   flags,
                    GCancellable         *cancellable,
                    GAsyncReadyCallback  callback,
                    gpointer             user_data);

G_DEFINE_TYPE (OGMDvdDrive, ogmdvd_drive, G_TYPE_OBJECT);

static void
ogmdvd_drive_probe_medium (OGMDvdDrive *self)
{
  gpointer handle;

  self->priv->medium_type = -1;

  handle = ogmdvd_device_open (self->priv->device, FALSE);
  if (handle)
  {
    gint profile;

    profile = ogmdvd_device_get_profile (handle);
    switch (profile & 0xFFFF)
    {
      case -1:
        g_assert_not_reached ();
        break;
      case OGMDVD_PROFILE_EMPTY:
        self->priv->medium_type = OGMDVD_MEDIUM_NONE;
        break;
      case OGMDVD_PROFILE_CDROM:
        self->priv->medium_type = OGMDVD_MEDIUM_CD;
        break;
      case OGMDVD_PROFILE_CDR:
        self->priv->medium_type = OGMDVD_MEDIUM_CDR;
        break;
      case OGMDVD_PROFILE_CDRW:
        self->priv->medium_type = OGMDVD_MEDIUM_CDRW;
        break;
      case OGMDVD_PROFILE_DVD_ROM:
        self->priv->medium_type = OGMDVD_MEDIUM_DVD;
        break;
      case OGMDVD_PROFILE_DVD_R:
        self->priv->medium_type = OGMDVD_MEDIUM_DVDR;
        break;
      case OGMDVD_PROFILE_DVD_RAM:
        self->priv->medium_type = OGMDVD_MEDIUM_DVD_RAM;
        break;
      case OGMDVD_PROFILE_DVD_RW_RESTRICTED:
      case OGMDVD_PROFILE_DVD_RW_SEQUENTIAL:
        self->priv->medium_type = OGMDVD_MEDIUM_DVDRW;
        break;
      case OGMDVD_PROFILE_DVD_R_DL_SEQUENTIAL:
      case OGMDVD_PROFILE_DVD_R_DL_JUMP:
        self->priv->medium_type = OGMDVD_MEDIUM_DVDR_DL;
        break;
      case OGMDVD_PROFILE_DVD_RW_PLUS:
        self->priv->medium_type = OGMDVD_MEDIUM_DVD_PLUS_RW;
        break;
      case OGMDVD_PROFILE_DVD_R_PLUS:
        self->priv->medium_type = OGMDVD_MEDIUM_DVD_PLUS_R;
        break;
      case OGMDVD_PROFILE_DVD_RW_PLUS_DL:
        self->priv->medium_type = OGMDVD_MEDIUM_DVD_PLUS_RW_DL;
        break;
      case OGMDVD_PROFILE_DVD_R_PLUS_DL:
        self->priv->medium_type = OGMDVD_MEDIUM_DVD_PLUS_R_DL;
        break;
      case OGMDVD_PROFILE_BD_ROM:
        self->priv->medium_type = OGMDVD_MEDIUM_BD;
        break;
      case OGMDVD_PROFILE_BR_R_SEQUENTIAL:
      case OGMDVD_PROFILE_BR_R_RANDOM:
        self->priv->medium_type = OGMDVD_MEDIUM_BDR;
        break;
      case OGMDVD_PROFILE_BD_RW:
        self->priv->medium_type = OGMDVD_MEDIUM_BDRW;
        break;
      default:
        self->priv->medium_type = OGMDVD_MEDIUM_UNKNOWN;
        break;
    }

    ogmdvd_device_close (handle);
  }
}

static gboolean
ogmdvd_drive_proble_idle (OGMDvdDrive *self)
{
  if (self->priv->has_medium && !self->priv->had_medium)
  {
    self->priv->had_medium = TRUE;

    ogmdvd_drive_probe_medium (self);

    g_signal_emit (self, signals[MEDIUM_ADDED], 0);
  }
  else if (!self->priv->has_medium && self->priv->had_medium)
  {
    self->priv->had_medium = FALSE;

    g_signal_emit (self, signals[MEDIUM_REMOVED], 0);
  }

  self->priv->probe_id = 0;

  return FALSE;
}

#ifndef ENOMEDIUM
#define ENOMEDIUM ENODEV
#endif

static void
ogmdvd_drive_probe (OGMDvdDrive *self)
{
  gpointer handle;

  handle = ogmdvd_device_open (self->priv->device, FALSE);
  if (handle)
  {
    if (ogmdvd_device_test_unit (handle) == 0)
      self->priv->has_medium = TRUE;
    else if (errno == ENOMEDIUM)
      self->priv->has_medium = FALSE;

    ogmdvd_device_close (handle);

    if (self->priv->probe_id)
      g_source_remove (self->priv->probe_id);

    self->priv->probe_id = g_idle_add ((GSourceFunc) ogmdvd_drive_proble_idle, self);
  }
}

static void
ogmdvd_drive_gdrive_changed_cb (GDrive *gdrive, OGMDvdDrive *self)
{
  ogmdvd_drive_probe (self);
}

static void
ogmdvd_drive_set_gdrive (OGMDvdDrive *self, GDrive *gdrive)
{
  if (self->priv->gdrive)
  {
    g_signal_handlers_disconnect_by_func (self->priv->gdrive,
        ogmdvd_drive_gdrive_changed_cb, self);

    g_object_unref (self->priv->gdrive);
    self->priv->gdrive = NULL;
  }

  if (self->priv->device)
  {
    g_free (self->priv->device);
    self->priv->device = NULL;
  }

  if (self->priv->name)
  {
    g_free (self->priv->name);
    self->priv->name = NULL;
  }

  if (gdrive)
  {
    gpointer handle;
    gchar *vendor, *model, *name;

    self->priv->gdrive = g_object_ref (gdrive);

    g_signal_connect (self->priv->gdrive, "changed",
        G_CALLBACK (ogmdvd_drive_gdrive_changed_cb), self);

    self->priv->device = g_drive_get_identifier (gdrive, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);

    handle = ogmdvd_device_open (self->priv->device, FALSE);
    if (handle)
    {
      if (ogmdvd_device_inquiry (handle, &vendor, &model))
      {
        name = g_strdup_printf ("%s %s", g_strstrip (vendor), g_strstrip (model));
        g_free (vendor);
        g_free (model);

        self->priv->name = g_convert_with_fallback (name, -1, "ASCII", "UTF-8", "_", NULL, NULL, NULL);
        g_free (name);
      }

      self->priv->drive_type = ogmdvd_device_get_capabilities (handle);

      ogmdvd_device_close (handle);
    }
  }

  ogmdvd_drive_probe (self);
}

static void
ogmdvd_drive_finalize (GObject *object)
{
  OGMDvdDrive *self;

  self = OGMDVD_DRIVE (object);

  if (self->priv->probe_id)
  {
    g_source_remove (self->priv->probe_id);
    self->priv->probe_id = 0;
  }

  if (self->priv->has_medium)
  {
    g_signal_emit (self, signals [MEDIUM_REMOVED], 0);
    self->priv->has_medium = FALSE;
  }

  if (self->priv->name)
  {
    g_free (self->priv->name);
    self->priv->name = NULL;
  }

  if (self->priv->device)
  {
    g_free (self->priv->device);
    self->priv->device = NULL;
  }

  if (self->priv->gdrive)
  {
    g_signal_handlers_disconnect_by_func (self->priv->gdrive,
        ogmdvd_drive_gdrive_changed_cb, self);

    g_object_unref (self->priv->gdrive);
    self->priv->gdrive = NULL;
  }

  G_OBJECT_CLASS (ogmdvd_drive_parent_class)->finalize (object);
}

static void
ogmdvd_drive_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  g_return_if_fail (OGMDVD_IS_DRIVE (object));

  switch (prop_id)
  {
    case PROP_GDRIVE:
      ogmdvd_drive_set_gdrive (OGMDVD_DRIVE (object), g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
ogmdvd_drive_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  g_return_if_fail (OGMDVD_IS_DRIVE (object));

  switch (prop_id)
  {
    case PROP_GDRIVE:
      g_value_set_object (value, OGMDVD_DRIVE (object)->priv->gdrive);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
ogmdvd_drive_init (OGMDvdDrive *self)
{
  self->priv = OGMDVD_DRIVE_GET_PRIV (self);
}

static void
ogmdvd_drive_class_init (OGMDvdDriveClass *klass)
{
  GObjectClass* object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = ogmdvd_drive_finalize;
  object_class->set_property = ogmdvd_drive_set_property;
  object_class->get_property = ogmdvd_drive_get_property;

  signals[MEDIUM_ADDED] = g_signal_new ("medium_added", G_OBJECT_CLASS_TYPE (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE, G_STRUCT_OFFSET (OGMDvdDriveClass, medium_added),
      NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  signals[MEDIUM_REMOVED] = g_signal_new ("medium_removed", G_OBJECT_CLASS_TYPE (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE, G_STRUCT_OFFSET (OGMDvdDriveClass, medium_removed),
      NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  g_object_class_install_property (object_class, PROP_GDRIVE,
      g_param_spec_object ("gdrive", "GDrive", "A GDrive object for the drive",
        G_TYPE_DRIVE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMDvdDrivePriv));
}

/**
 * ogmdvd_drive_can_eject:
 * @drive: #OGMDvdDrive
 *
 * Checks if a drive can be ejected.
 *
 * Returns: %TRUE if the drive can be ejected, FALSE otherwise.
 **/
gboolean
ogmdvd_drive_can_eject (OGMDvdDrive *drive)
{
  g_return_val_if_fail (OGMDVD_IS_DRIVE (drive), FALSE);

  if (!drive->priv->gdrive)
    return FALSE;

  return g_drive_can_eject (drive->priv->gdrive);
}

/**
 * ogmdvd_drive_eject:
 * @drive: #OGMDvdDrive
 *
 * Ejects a drive.
 **/
void
ogmdvd_drive_eject (OGMDvdDrive *drive)
{
  g_return_if_fail (OGMDVD_IS_DRIVE (drive));

  if (drive->priv->gdrive && g_drive_can_eject (drive->priv->gdrive))
  {
#if GLIB_CHECK_VERSION(2,22,0)
    g_drive_eject_with_operation (drive->priv->gdrive, G_MOUNT_UNMOUNT_NONE, NULL, NULL, NULL, NULL);
#else
    g_drive_eject (drive->priv->gdrive, G_MOUNT_UNMOUNT_NONE, NULL, NULL, NULL);
#endif
    g_drive_eject (drive->priv->gdrive, 0, NULL, NULL, NULL);
  }
}

/**
 * ogmdvd_drive_load:
 * @drive: #OGMDvdDrive
 *
 * Loads a drive.
 **/
void
ogmdvd_drive_load (OGMDvdDrive *drive)
{
  const gchar *device;

  g_return_if_fail (OGMDVD_IS_DRIVE (drive));

  device = ogmdvd_drive_get_device (drive);
  if (device)
  {
    gpointer handle;

    handle = ogmdvd_device_open (device, FALSE);
    if (handle)
    {
      ogmdvd_device_start_stop_unit (handle);
      ogmdvd_device_close (handle);
    }
  }
}

/**
 * ogmdvd_drive_is_door_open:
 * @drive: #OGMDvdDrive
 *
 * Reports whether the drive door or tray is open.
 *
 * Returns: %TRUE if the drive door is open, %FALSE otherwise.
 **/
gboolean
ogmdvd_drive_is_door_open (OGMDvdDrive *drive)
{
  g_message ("Not yet implemented");

  return FALSE;
}

/**
 * ogmdvd_drive_lock:
 * @drive: #OGMDvdDrive
 *
 * Locks a drive.
 *
 * Returns: %TRUE if the drive has been locked, %FALSE otherwise
 **/
gboolean
ogmdvd_drive_lock (OGMDvdDrive *drive)
{
  const gchar *device;
  gpointer handle;
  gboolean res;

  g_return_val_if_fail (OGMDVD_IS_DRIVE (drive), FALSE);

  device = ogmdvd_drive_get_device (drive);
  if (!device)
    return FALSE;

  handle = ogmdvd_device_open (device, FALSE);
  if (!handle)
    return FALSE;

  res = ogmdvd_device_set_lock (handle, TRUE);

  ogmdvd_device_close (handle);

  return res == 0;
}

/**
 * ogmdvd_drive_unlock:
 * @drive: #OGMDvdDrive
 *
 * Unlocks a drive.
 *
 * Returns: %TRUE if the drive has been unlocked, %FALSE otherwise
 **/
gboolean
ogmdvd_drive_unlock (OGMDvdDrive *drive)
{
  const gchar *device;
  gpointer handle;
  gboolean res;

  g_return_val_if_fail (OGMDVD_IS_DRIVE (drive), FALSE);

  device = ogmdvd_drive_get_device (drive);
  if (!device)
    return FALSE;

  handle = ogmdvd_device_open (device, FALSE);
  if (!handle)
    return FALSE;

  res = ogmdvd_device_set_lock (handle, TRUE);

  ogmdvd_device_close (handle);

  return res == 0;
}

/**
 * ogmdvd_drive_get_name:
 * @drive: #OGMDvdDrive
 *
 * Gets the name of the drive for use in a user interface
 *
 * Returns: the name of the drive (must be freed with g_free())
 **/
gchar *
ogmdvd_drive_get_name (OGMDvdDrive *drive)
{
  g_return_val_if_fail (OGMDVD_IS_DRIVE (drive), NULL);
/*
  if (drive->priv->gdrive)
    return g_drive_get_name (drive->priv->gdrive);
*/
  if (!drive->priv->name)
    return NULL;

  return g_strdup (drive->priv->name);
}

/**
 * ogmdvd_drive_get_device:
 * @drive: #OGMDvdDrive
 *
 * Gets the name of the device associated with the drive.
 *
 * Returns: the name of the device (must be not be freed).
 **/
const gchar *
ogmdvd_drive_get_device (OGMDvdDrive *drive)
{
  g_return_val_if_fail (OGMDVD_IS_DRIVE (drive), NULL);

  return drive->priv->device;
}

/**
 * ogmdvd_drive_get_gdrive:
 * @drive: #OGMDvdDrive
 *
 * Gets the #GDrive associated to @drive.
 *
 * Returns: a #GDrive, or NULL
 **/
GDrive *
ogmdvd_drive_get_gdrive (OGMDvdDrive *drive)
{
  g_return_val_if_fail (OGMDVD_IS_DRIVE (drive), NULL);

  return drive->priv->gdrive;
}

/**
 * ogmdvd_drive_get_drive_type:
 * @drive: #OGMDvdDrive
 *
 * Gets the type of the drive.
 *
 * Returns: the type of drive, or -1
 */
gint
ogmdvd_drive_get_drive_type (OGMDvdDrive *drive)
{
  g_return_val_if_fail (OGMDVD_IS_DRIVE (drive), -1);

  return drive->priv->drive_type;
}

/**
 * ogmdvd_drive_get_medium_type:
 * @drive: #OGMDvdDrive
 *
 * Gets the type of the medium in @drive.
 *
 * Return value: The #OGMDvdMediumType of the media in the drive or the
 * following special values:
 *
 *    %OGMDVD_MEDIA_TYPE_UNKNOWN if the type can not be determined
 *    %OGMDVD_MEDIA_TYPE_NONE    if no medium is present
 */
gint
ogmdvd_drive_get_medium_type (OGMDvdDrive *drive)
{
  g_return_val_if_fail (OGMDVD_IS_DRIVE (drive), -1);

  if (!drive->priv->has_medium)
    return OGMDVD_MEDIUM_NONE;

  return drive->priv->medium_type;
}

