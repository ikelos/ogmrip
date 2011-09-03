/* OGMDvd - A wrapper library around libdvdread
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmdvd-device.h"
#include "ogmdvd-transport.h"
#include "ogmdvd-drive.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_STRUCT_CAM_DEVICE
#include <camlib.h>
#endif

gpointer
ogmdvd_device_open (const gchar *device, gboolean exclusive)
{
  g_return_val_if_fail (device != NULL, NULL);

#if defined(HAVE_STRUCT_CAM_DEVICE)
  struct cam_device *cam;

  cam = cam_open_device (device, O_RDWR);
  if (!cam)
  {
    g_message ("%s", cam_errbuf);
    return NULL;
  }

  return cam;

#elif defined(__linux__)
  int flags, fd;

  flags = O_NONBLOCK;
  if (exclusive)
    flags |= O_EXCL;

  if ((fd = open (device, flags | O_RDWR, 0)) < 0 &&
      (fd = open (device, flags | O_RDONLY, 0)) < 0)
    return NULL;

  return GINT_TO_POINTER (fd);

#elif defined(__OpenBSD__) || defined(__NetBSD__)
  int fd, flags;
  gchar *rdevnode;

  flags = O_RDWR | O_NONBLOCK;
  if (exclusive)
    flags |= O_EXCL;

  rdevnode = g_strdup_printf ("/dev/r%s", device + strlen ("/dev/"));
  fd = open (rdevnode, flags);
  g_free (rdevnode);
  if (fd < 0)
    return NULL;

  return GINT_TO_POINTER (fd);

#elif defined(__sun) || defined(sun)
  int fd;

  if (g_str_has_prefix (device, "/dev/dsk/"))
  {
    gchar *rawdisk;
    rawdisk = g_strdup_printf ("/dev/rdsk/%s", device + 9);
    fd = open (rawdisk, O_RDONLY | O_NONBLOCK);
    g_free (rawdisk);
  }
  else
    fd = open (device, O_RDONLY | O_NONBLOCK);

  if (fd < 0)
    return NULL;

  return GINT_TO_POINTER (fd);

#else
  return NULL;

#endif
}

void
ogmdvd_device_close (gpointer handle)
{
  g_return_if_fail (handle != NULL);

#if defined(HAVE_STRUCT_CAM_DEVICE)
  cam_close_device ((struct cam_device *) handle);

#elif defined(__linux__) || defined(__sun) || defined(sun) || defined(__OpenBSD__) || defined(__NetBSD__)
  close (GPOINTER_TO_INT (handle));

#endif
}

gint
ogmdvd_device_get_fd (gpointer handle)
{
  g_return_val_if_fail (handle != NULL, -1);

#ifdef HAVE_STRUCT_CAM_DEVICE
  return ((struct cam_device *) handle)->fd;
#endif

#if defined(__linux__) || defined(__sun) || defined(sun) || defined(__OpenBSD__) || defined(__NetBSD__)
  return GPOINTER_TO_INT (handle);
#endif

  return -1;
}

gboolean
ogmdvd_device_inquiry (gpointer handle, gchar **vendor, gchar **name)
{
  Scsi_Command *cmd;
  guchar buf[36];
  gint fd, res;

  g_return_val_if_fail (handle != NULL, FALSE);

  fd = ogmdvd_device_get_fd (handle);
  if (fd < 0)
    return FALSE;

  cmd = scsi_command_new_from_fd (fd);

  memset (&buf, 0, sizeof (buf));

  scsi_command_set (cmd, 0, 0x12);
  scsi_command_set (cmd, 4, sizeof (buf));
  scsi_command_set (cmd, 5, 0);

  res = scsi_command_transport (cmd, READ, buf, sizeof (buf));

  scsi_command_free (cmd);

  res = (res == 0 && (buf[0] & 0x1F) == 0x05);

  if (vendor)
    *vendor = res ? g_strndup ((gchar *) (buf + 8), 8) : NULL;

  if (name)
    *name = res ? g_strndup ((gchar *) (buf + 16), 16) : NULL;

  return res;
}

gint
ogmdvd_device_test_unit (gpointer handle)
{
  Scsi_Command *cmd;
  gint fd, res;

  g_return_val_if_fail (handle != NULL, -1);

  fd = ogmdvd_device_get_fd (handle);
  if (fd < 0)
    return -1;

  cmd = scsi_command_new_from_fd (fd);

  scsi_command_set (cmd, 0, 0);
  scsi_command_set (cmd, 5, 0);

  res = scsi_command_transport (cmd, NONE, NULL, 0);

  scsi_command_free (cmd);

  return res;
}

gboolean
ogmdvd_device_start_stop_unit (gpointer handle)
{
  Scsi_Command *cmd;
  gint fd, res;

  g_return_val_if_fail (handle != NULL, FALSE);

  fd = ogmdvd_device_get_fd (handle);
  if (fd < 0)
    return FALSE;

  cmd = scsi_command_new_from_fd (fd);

  scsi_command_set (cmd, 0, 0x1B);
  scsi_command_set (cmd, 4, 0x3); /* load */
  /* scsi_command_set (cmd, 4, 0x2); */ /* eject */
  scsi_command_set (cmd, 5, 0);

  res = scsi_command_transport (cmd, NONE, NULL, 0);

  scsi_command_free (cmd);

  return res != 0;
}

gint
ogmdvd_device_set_lock (gpointer handle, gboolean lock)
{
  Scsi_Command *cmd;
  gint fd, res;

  g_return_val_if_fail (handle != NULL, -1);

  fd = ogmdvd_device_get_fd (handle);
  if (fd < 0)
    return -1;

  cmd = scsi_command_new_from_fd (fd);

  scsi_command_set (cmd, 0, 0x1e);
  scsi_command_set (cmd, 4, lock ? 1 : 0);
  scsi_command_set (cmd, 5, 0);

  res = scsi_command_transport (cmd, NONE, NULL, 0);

  scsi_command_free (cmd);

  return res;
}

gint
ogmdvd_device_get_profile (gpointer handle)
{
  Scsi_Command *cmd;
  guchar buf[8], info[32], formats[260];

  gint fd, len, profile, retval = -1;

  g_return_val_if_fail (handle != NULL, -1);

  fd = ogmdvd_device_get_fd (handle);
  if (fd < 0)
    return -1;

  cmd = scsi_command_new_from_fd (fd);

  memset (&buf, 0, sizeof (buf));
  
  scsi_command_set (cmd, 0, 0x46); /* get configuration */
  scsi_command_set (cmd, 8, sizeof (buf));
  scsi_command_set (cmd, 9, 0);

  if (scsi_command_transport (cmd, READ, buf, sizeof (buf)) != 0)
    goto cleanup;

  profile = (buf[6] << 8 | buf[7]);

  memset (&info, 0, sizeof (info));

  scsi_command_set (cmd, 0, 0x51); /* read disc information */
  scsi_command_set (cmd, 8, sizeof (info));
  scsi_command_set (cmd, 9, 0);

  if (scsi_command_transport (cmd, READ, info, sizeof (info)) != 0)
    goto cleanup;

  /* blank ? */
  if ((info[2] & 3) == 0)
    goto cleanup;

  if (profile != 0x1A && profile != 0x13 && profile != 0x12)
  {
    retval = profile;
    goto cleanup;
  }

  memset (&formats, 0, sizeof (info));

  scsi_command_set (cmd, 0, 0x23); /* read format capacities */
  scsi_command_set (cmd, 8, 12);
  scsi_command_set (cmd, 9, 0);

  if (scsi_command_transport (cmd, READ, formats, 12) != 0)
    goto cleanup;

  len = formats[3];
  if (len & 7 || len < 16)
    goto cleanup;

  scsi_command_set (cmd, 0, 0x23); /* read format capacities */
  scsi_command_set (cmd, 7, (len + 4) >> 8);
  scsi_command_set (cmd, 8, (len + 4) & 0xFF);
  scsi_command_set (cmd, 9, 0);

  if (scsi_command_transport (cmd, READ, formats, len + 4) != 0)
    goto cleanup;

  if (len != formats[3])
    goto cleanup;

  /* blank ? */
  if ((formats[8] & 3) != 2)
    goto cleanup;

  retval = profile;

cleanup:
  scsi_command_free (cmd);

  return retval;
}

gint
ogmdvd_device_get_capabilities (gpointer handle)
{
  Scsi_Command *cmd;
  guchar buf[264];

  gint fd, len, i, caps = 0, retval = -1;

  g_return_val_if_fail (handle != NULL, -1);

  fd = ogmdvd_device_get_fd (handle);
  if (fd < 0)
    return -1;

  cmd = scsi_command_new_from_fd (fd);

  memset (&buf, 0, sizeof (buf));
  
  scsi_command_set (cmd, 0, 0x46); /* get configuration */
  scsi_command_set (cmd, 1, 2);
  scsi_command_set (cmd, 8, 8);
  scsi_command_set (cmd, 9, 0);

  if (scsi_command_transport (cmd, READ, buf, 8) != 0)
    goto cleanup;

  len = 4 + (buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3]);

  scsi_command_set (cmd, 0, 0x46);
  scsi_command_set (cmd, 1, 2);
  scsi_command_set (cmd, 7, len >> 8);
  scsi_command_set (cmd, 8, len);
  scsi_command_set (cmd, 9, 0);

  if (scsi_command_transport (cmd, READ, buf, len) != 0)
    goto cleanup;

  if (len <= 12 || buf[11] > len)
    goto cleanup;

  for (i = 12; i < 12 + buf[11]; i += 4)
  {
    switch (buf[i] << 8 | buf[i + 1])
    {
      case OGMDVD_PROFILE_CDROM:
        caps |= OGMDVD_DRIVE_CDROM;
        break;
      case OGMDVD_PROFILE_CDR:
        caps |= OGMDVD_DRIVE_CDR;
        break;
      case OGMDVD_PROFILE_CDRW:
        caps |= OGMDVD_DRIVE_CDRW;
        break;
      case OGMDVD_PROFILE_DVD_ROM:
        caps |= OGMDVD_DRIVE_DVD;
        break;
      case OGMDVD_PROFILE_DVD_R:
        caps |= OGMDVD_DRIVE_DVDR;
        break;
      case OGMDVD_PROFILE_DVD_RAM:
        caps |= OGMDVD_DRIVE_DVDRAM;
        break;
      case OGMDVD_PROFILE_DVD_RW_RESTRICTED:
      case OGMDVD_PROFILE_DVD_RW_SEQUENTIAL:
        caps |= OGMDVD_DRIVE_DVDRW;
        break;
      case OGMDVD_PROFILE_DVD_RW_PLUS:
        caps |= OGMDVD_DRIVE_DVDRW_PLUS;
        break;
      case OGMDVD_PROFILE_DVD_R_PLUS:
        caps |= OGMDVD_DRIVE_DVDR_PLUS;
        break;
      case OGMDVD_PROFILE_DVD_RW_PLUS_DL:
        caps |= OGMDVD_DRIVE_DVDRW_PLUS_DL;
        break;
      case OGMDVD_PROFILE_DVD_R_PLUS_DL:
        caps |= OGMDVD_DRIVE_DVDR_PLUS_DL;
        break;
      case OGMDVD_PROFILE_BD_ROM:
        caps |= OGMDVD_DRIVE_BD;
        break;
      case OGMDVD_PROFILE_BR_R_SEQUENTIAL:
      case OGMDVD_PROFILE_BR_R_RANDOM:
        caps |= OGMDVD_DRIVE_BDR;
        break;
      case OGMDVD_PROFILE_BD_RW:
        caps |= OGMDVD_DRIVE_BDRW;
        break;
      default:
        break;
    }
  }

  retval = caps;

cleanup:
  scsi_command_free (cmd);

  return retval;
}
