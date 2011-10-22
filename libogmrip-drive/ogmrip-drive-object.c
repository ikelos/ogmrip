/* OGMRipDrive - An optical drive library form OGMRip
 * Copyright (C) 2009-2011 Olivier Rolland <billl@users.sourceforge.net>
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

/**
 * SECTION:ogmrip-drive
 * @title: OGMRipDrive
 * @include: ogmrip-drive.h
 * @short_description: An object representing an Optical Drive
 */

#include "ogmrip-drive-object.h"
#include "ogmrip-drive-device.h"

#include <glib.h>
#include <errno.h>

struct _OGMRipDrivePriv
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

#define OGMRIP_DRIVE_GET_PRIV(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_DRIVE, OGMRipDrivePriv))

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

G_DEFINE_TYPE (OGMRipDrive, ogmrip_drive, G_TYPE_OBJECT);

static void
ogmrip_drive_probe_medium (OGMRipDrive *self)
{
  gpointer handle;

  self->priv->medium_type = -1;

  handle = ogmrip_device_open (self->priv->device, FALSE);
  if (handle)
  {
    gint profile;

    profile = ogmrip_device_get_profile (handle);
    switch (profile & 0xFFFF)
    {
      case -1:
        g_assert_not_reached ();
        break;
      case OGMRIP_PROFILE_EMPTY:
        self->priv->medium_type = OGMRIP_MEDIUM_NONE;
        break;
      case OGMRIP_PROFILE_CDROM:
        self->priv->medium_type = OGMRIP_MEDIUM_CD;
        break;
      case OGMRIP_PROFILE_CDR:
        self->priv->medium_type = OGMRIP_MEDIUM_CDR;
        break;
      case OGMRIP_PROFILE_CDRW:
        self->priv->medium_type = OGMRIP_MEDIUM_CDRW;
        break;
      case OGMRIP_PROFILE_DVD_ROM:
        self->priv->medium_type = OGMRIP_MEDIUM_DVD;
        break;
      case OGMRIP_PROFILE_DVD_R:
        self->priv->medium_type = OGMRIP_MEDIUM_DVDR;
        break;
      case OGMRIP_PROFILE_DVD_RAM:
        self->priv->medium_type = OGMRIP_MEDIUM_DVD_RAM;
        break;
      case OGMRIP_PROFILE_DVD_RW_RESTRICTED:
      case OGMRIP_PROFILE_DVD_RW_SEQUENTIAL:
        self->priv->medium_type = OGMRIP_MEDIUM_DVDRW;
        break;
      case OGMRIP_PROFILE_DVD_R_DL_SEQUENTIAL:
      case OGMRIP_PROFILE_DVD_R_DL_JUMP:
        self->priv->medium_type = OGMRIP_MEDIUM_DVDR_DL;
        break;
      case OGMRIP_PROFILE_DVD_RW_PLUS:
        self->priv->medium_type = OGMRIP_MEDIUM_DVD_PLUS_RW;
        break;
      case OGMRIP_PROFILE_DVD_R_PLUS:
        self->priv->medium_type = OGMRIP_MEDIUM_DVD_PLUS_R;
        break;
      case OGMRIP_PROFILE_DVD_RW_PLUS_DL:
        self->priv->medium_type = OGMRIP_MEDIUM_DVD_PLUS_RW_DL;
        break;
      case OGMRIP_PROFILE_DVD_R_PLUS_DL:
        self->priv->medium_type = OGMRIP_MEDIUM_DVD_PLUS_R_DL;
        break;
      case OGMRIP_PROFILE_BD_ROM:
        self->priv->medium_type = OGMRIP_MEDIUM_BD;
        break;
      case OGMRIP_PROFILE_BR_R_SEQUENTIAL:
      case OGMRIP_PROFILE_BR_R_RANDOM:
        self->priv->medium_type = OGMRIP_MEDIUM_BDR;
        break;
      case OGMRIP_PROFILE_BD_RW:
        self->priv->medium_type = OGMRIP_MEDIUM_BDRW;
        break;
      default:
        self->priv->medium_type = OGMRIP_MEDIUM_UNKNOWN;
        break;
    }

    ogmrip_device_close (handle);
  }
}

static gboolean
ogmrip_drive_proble_idle (OGMRipDrive *self)
{
  if (self->priv->has_medium && !self->priv->had_medium)
  {
    self->priv->had_medium = TRUE;

    ogmrip_drive_probe_medium (self);

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
ogmrip_drive_probe (OGMRipDrive *self)
{
  gpointer handle;

  handle = ogmrip_device_open (self->priv->device, FALSE);
  if (handle)
  {
    if (ogmrip_device_test_unit (handle) == 0)
      self->priv->has_medium = TRUE;
    else if (errno == ENOMEDIUM)
      self->priv->has_medium = FALSE;

    ogmrip_device_close (handle);

    if (self->priv->probe_id)
      g_source_remove (self->priv->probe_id);

    self->priv->probe_id = g_idle_add ((GSourceFunc) ogmrip_drive_proble_idle, self);
  }
}

static void
ogmrip_drive_gdrive_changed_cb (GDrive *gdrive, OGMRipDrive *self)
{
  ogmrip_drive_probe (self);
}

static void
ogmrip_drive_set_gdrive (OGMRipDrive *self, GDrive *gdrive)
{
  if (self->priv->gdrive)
  {
    g_signal_handlers_disconnect_by_func (self->priv->gdrive,
        ogmrip_drive_gdrive_changed_cb, self);

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
        G_CALLBACK (ogmrip_drive_gdrive_changed_cb), self);

    self->priv->device = g_drive_get_identifier (gdrive, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);

    handle = ogmrip_device_open (self->priv->device, FALSE);
    if (handle)
    {
      if (ogmrip_device_inquiry (handle, &vendor, &model))
      {
        name = g_strdup_printf ("%s %s", g_strstrip (vendor), g_strstrip (model));
        g_free (vendor);
        g_free (model);

        self->priv->name = g_convert_with_fallback (name, -1, "ASCII", "UTF-8", "_", NULL, NULL, NULL);
        g_free (name);
      }

      self->priv->drive_type = ogmrip_device_get_capabilities (handle);

      ogmrip_device_close (handle);
    }
  }

  ogmrip_drive_probe (self);
}

static void
ogmrip_drive_finalize (GObject *object)
{
  OGMRipDrive *self;

  self = OGMRIP_DRIVE (object);

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
        ogmrip_drive_gdrive_changed_cb, self);

    g_object_unref (self->priv->gdrive);
    self->priv->gdrive = NULL;
  }

  G_OBJECT_CLASS (ogmrip_drive_parent_class)->finalize (object);
}

static void
ogmrip_drive_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  g_return_if_fail (OGMRIP_IS_DRIVE (object));

  switch (prop_id)
  {
    case PROP_GDRIVE:
      ogmrip_drive_set_gdrive (OGMRIP_DRIVE (object), g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
ogmrip_drive_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  g_return_if_fail (OGMRIP_IS_DRIVE (object));

  switch (prop_id)
  {
    case PROP_GDRIVE:
      g_value_set_object (value, OGMRIP_DRIVE (object)->priv->gdrive);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
ogmrip_drive_init (OGMRipDrive *self)
{
  self->priv = OGMRIP_DRIVE_GET_PRIV (self);
}

static void
ogmrip_drive_class_init (OGMRipDriveClass *klass)
{
  GObjectClass* object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = ogmrip_drive_finalize;
  object_class->set_property = ogmrip_drive_set_property;
  object_class->get_property = ogmrip_drive_get_property;

  signals[MEDIUM_ADDED] = g_signal_new ("medium_added", G_OBJECT_CLASS_TYPE (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE, G_STRUCT_OFFSET (OGMRipDriveClass, medium_added),
      NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  signals[MEDIUM_REMOVED] = g_signal_new ("medium_removed", G_OBJECT_CLASS_TYPE (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE, G_STRUCT_OFFSET (OGMRipDriveClass, medium_removed),
      NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  g_object_class_install_property (object_class, PROP_GDRIVE,
      g_param_spec_object ("gdrive", "GDrive", "A GDrive object for the drive",
        G_TYPE_DRIVE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipDrivePriv));
}

/**
 * ogmrip_drive_can_eject:
 * @drive: #OGMRipDrive
 *
 * Checks if a drive can be ejected.
 *
 * Returns: %TRUE if the drive can be ejected, FALSE otherwise.
 **/
gboolean
ogmrip_drive_can_eject (OGMRipDrive *drive)
{
  g_return_val_if_fail (OGMRIP_IS_DRIVE (drive), FALSE);

  if (!drive->priv->gdrive)
    return FALSE;

  return g_drive_can_eject (drive->priv->gdrive);
}

/**
 * ogmrip_drive_eject:
 * @drive: #OGMRipDrive
 *
 * Ejects a drive.
 **/
void
ogmrip_drive_eject (OGMRipDrive *drive)
{
  g_return_if_fail (OGMRIP_IS_DRIVE (drive));

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
 * ogmrip_drive_load:
 * @drive: #OGMRipDrive
 *
 * Loads a drive.
 **/
void
ogmrip_drive_load (OGMRipDrive *drive)
{
  const gchar *device;

  g_return_if_fail (OGMRIP_IS_DRIVE (drive));

  device = ogmrip_drive_get_device (drive);
  if (device)
  {
    gpointer handle;

    handle = ogmrip_device_open (device, FALSE);
    if (handle)
    {
      ogmrip_device_start_stop_unit (handle);
      ogmrip_device_close (handle);
    }
  }
}

/**
 * ogmrip_drive_is_door_open:
 * @drive: #OGMRipDrive
 *
 * Reports whether the drive door or tray is open.
 *
 * Returns: %TRUE if the drive door is open, %FALSE otherwise.
 **/
gboolean
ogmrip_drive_is_door_open (OGMRipDrive *drive)
{
  g_message ("Not yet implemented");

  return FALSE;
}

/**
 * ogmrip_drive_lock:
 * @drive: #OGMRipDrive
 *
 * Locks a drive.
 *
 * Returns: %TRUE if the drive has been locked, %FALSE otherwise
 **/
gboolean
ogmrip_drive_lock (OGMRipDrive *drive)
{
  const gchar *device;
  gpointer handle;
  gboolean res;

  g_return_val_if_fail (OGMRIP_IS_DRIVE (drive), FALSE);

  device = ogmrip_drive_get_device (drive);
  if (!device)
    return FALSE;

  handle = ogmrip_device_open (device, FALSE);
  if (!handle)
    return FALSE;

  res = ogmrip_device_set_lock (handle, TRUE);

  ogmrip_device_close (handle);

  return res == 0;
}

/**
 * ogmrip_drive_unlock:
 * @drive: #OGMRipDrive
 *
 * Unlocks a drive.
 *
 * Returns: %TRUE if the drive has been unlocked, %FALSE otherwise
 **/
gboolean
ogmrip_drive_unlock (OGMRipDrive *drive)
{
  const gchar *device;
  gpointer handle;
  gboolean res;

  g_return_val_if_fail (OGMRIP_IS_DRIVE (drive), FALSE);

  device = ogmrip_drive_get_device (drive);
  if (!device)
    return FALSE;

  handle = ogmrip_device_open (device, FALSE);
  if (!handle)
    return FALSE;

  res = ogmrip_device_set_lock (handle, TRUE);

  ogmrip_device_close (handle);

  return res == 0;
}

/**
 * ogmrip_drive_get_name:
 * @drive: #OGMRipDrive
 *
 * Gets the name of the drive for use in a user interface
 *
 * Returns: the name of the drive (must be freed with g_free())
 **/
gchar *
ogmrip_drive_get_name (OGMRipDrive *drive)
{
  g_return_val_if_fail (OGMRIP_IS_DRIVE (drive), NULL);
/*
  if (drive->priv->gdrive)
    return g_drive_get_name (drive->priv->gdrive);
*/
  if (!drive->priv->name)
    return NULL;

  return g_strdup (drive->priv->name);
}

/**
 * ogmrip_drive_get_device:
 * @drive: #OGMRipDrive
 *
 * Gets the name of the device associated with the drive.
 *
 * Returns: the name of the device (must be not be freed).
 **/
const gchar *
ogmrip_drive_get_device (OGMRipDrive *drive)
{
  g_return_val_if_fail (OGMRIP_IS_DRIVE (drive), NULL);

  return drive->priv->device;
}

/**
 * ogmrip_drive_get_gdrive:
 * @drive: #OGMRipDrive
 *
 * Gets the #GDrive associated to @drive.
 *
 * Returns: a #GDrive, or NULL
 **/
GDrive *
ogmrip_drive_get_gdrive (OGMRipDrive *drive)
{
  g_return_val_if_fail (OGMRIP_IS_DRIVE (drive), NULL);

  return drive->priv->gdrive;
}

/**
 * ogmrip_drive_get_drive_type:
 * @drive: #OGMRipDrive
 *
 * Gets the type of the drive.
 *
 * Returns: the type of drive, or -1
 */
gint
ogmrip_drive_get_drive_type (OGMRipDrive *drive)
{
  g_return_val_if_fail (OGMRIP_IS_DRIVE (drive), -1);

  return drive->priv->drive_type;
}

/**
 * ogmrip_drive_get_medium_type:
 * @drive: #OGMRipDrive
 *
 * Gets the type of the medium in @drive.
 *
 * Return value: The #OGMRipMediumType of the media in the drive or the
 * following special values:
 *
 *    %OGMRIP_MEDIA_TYPE_UNKNOWN if the type can not be determined
 *    %OGMRIP_MEDIA_TYPE_NONE    if no medium is present
 */
gint
ogmrip_drive_get_medium_type (OGMRipDrive *drive)
{
  g_return_val_if_fail (OGMRIP_IS_DRIVE (drive), -1);

  if (!drive->priv->has_medium)
    return OGMRIP_MEDIUM_NONE;

  return drive->priv->medium_type;
}

