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

#ifndef __OGMRIP_DRIVE_DEVICE_H__
#define __OGMRIP_DRIVE_DEVICE_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum
{
  OGMRIP_PROFILE_EMPTY               = 0x0000,
  OGMRIP_PROFILE_NON_REMOVABLE       = 0x0001,
  OGMRIP_PROFILE_REMOVABLE           = 0x0002,
  OGMRIP_PROFILE_MO_ERASABLE         = 0x0003,
  OGMRIP_PROFILE_MO_WRITE_ONCE       = 0x0004,
  OGMRIP_PROFILE_MO_ADVANCED_STORAGE = 0x0005,
  /* reserved */
  OGMRIP_PROFILE_CDROM               = 0x0008,
  OGMRIP_PROFILE_CDR                 = 0x0009,
  OGMRIP_PROFILE_CDRW                = 0x000A,
  /* reserved */
  OGMRIP_PROFILE_DVD_ROM             = 0x0010,
  OGMRIP_PROFILE_DVD_R               = 0x0011,
  OGMRIP_PROFILE_DVD_RAM             = 0x0012,
  OGMRIP_PROFILE_DVD_RW_RESTRICTED   = 0x0013,
  OGMRIP_PROFILE_DVD_RW_SEQUENTIAL   = 0x0014,
  OGMRIP_PROFILE_DVD_R_DL_SEQUENTIAL = 0x0015,
  OGMRIP_PROFILE_DVD_R_DL_JUMP       = 0x0016,
  /* reserved */
  OGMRIP_PROFILE_DVD_RW_PLUS         = 0x001A,
  OGMRIP_PROFILE_DVD_R_PLUS          = 0x001B,
  /* reserved */
  OGMRIP_PROFILE_DDCD_ROM            = 0x0020,
  OGMRIP_PROFILE_DDCD_R              = 0x0021,
  OGMRIP_PROFILE_DDCD_RW             = 0x0022,
  /* reserved */
  OGMRIP_PROFILE_DVD_RW_PLUS_DL      = 0x002A,
  OGMRIP_PROFILE_DVD_R_PLUS_DL       = 0x002B,
  /* reserved */
  OGMRIP_PROFILE_BD_ROM              = 0x0040,
  OGMRIP_PROFILE_BR_R_SEQUENTIAL     = 0x0041,
  OGMRIP_PROFILE_BR_R_RANDOM         = 0x0042,
  OGMRIP_PROFILE_BD_RW               = 0x0043,
  OGMRIP_PROFILE_HD_DVD_ROM          = 0x0050,
  OGMRIP_PROFILE_HD_DVD_R            = 0x0051,
  OGMRIP_PROFILE_HD_DVD_RAM          = 0x0052,
  /* reserved */
} OGMRipProfileType;

gpointer ogmrip_device_open              (const gchar *device,
                                          gboolean    exclusive);
void     ogmrip_device_close             (gpointer    handle);

gint     ogmrip_device_get_fd            (gpointer    handle);

gboolean ogmrip_device_inquiry           (gpointer    handle,
                                          gchar       **vendor,
                                          gchar       **name);
gint     ogmrip_device_set_lock          (gpointer    handle,
                                          gboolean    lock);
gint     ogmrip_device_test_unit         (gpointer    handle);
gint     ogmrip_device_get_profile       (gpointer    handle);
gint     ogmrip_device_get_capabilities  (gpointer    handle);
gboolean ogmrip_device_start_stop_unit   (gpointer    handle);

G_END_DECLS

#endif /* __OGMRIP_DRIVE_DEVICE_H__ */
