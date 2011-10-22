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

#ifndef __OGMRIP_DRIVE_OBJECT_H__
#define __OGMRIP_DRIVE_OBJECT_H__

#include <glib-object.h>

#include <gio/gio.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_DRIVE             (ogmrip_drive_get_type ())
#define OGMRIP_DRIVE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_DRIVE, OGMRipDrive))
#define OGMRIP_DRIVE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_DRIVE, OGMRipDriveClass))
#define OGMRIP_IS_DRIVE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_DRIVE))
#define OGMRIP_IS_DRIVE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_DRIVE))
#define OGMRIP_DRIVE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_DRIVE, OGMRipDriveClass))

/**
 * OGMRipDriveType:
 * @OGMRIP_DRIVE_NONE: No drive is available
 * @OGMRIP_DRIVE_CDROM: The drive can read CDROM
 * @OGMRIP_DRIVE_CDR: The drive can write CD-R
 * @OGMRIP_DRIVE_CDRW: The drive can write CD-RW
 * @OGMRIP_DRIVE_DVD: The drive can read DVD
 * @OGMRIP_DRIVE_DVDR: The drive can write DVD-R
 * @OGMRIP_DRIVE_DVDRW: The drive can write DVD-RW
 * @OGMRIP_DRIVE_DVDR_PLUS: The drive can write DVD+R
 * @OGMRIP_DRIVE_DVDRW_PLUS: The drive can write DVD+RW
 * @OGMRIP_DRIVE_DVDR_PLUS_DL: The drive can write DVD+R DL
 * @OGMRIP_DRIVE_DVDRW_PLUS_DL: The drive can write DVD+RW DL
 * @OGMRIP_DRIVE_DVDRAM: The drive can read DVD-Ram
 * @OGMRIP_DRIVE_BD: The drive can read BD
 * @OGMRIP_DRIVE_BDR: The drive can write BD-R
 * @OGMRIP_DRIVE_BDRW: The drive can write BD-RW
 *
 * The type of the drive is a combination these constants.
 */
typedef enum
{
  OGMRIP_DRIVE_NONE           = 0,
  OGMRIP_DRIVE_CDROM          = 1 << 0,
  OGMRIP_DRIVE_CDR            = 1 << 1,
  OGMRIP_DRIVE_CDRW           = 1 << 2,
  OGMRIP_DRIVE_DVD            = 1 << 3,
  OGMRIP_DRIVE_DVDR           = 1 << 4,
  OGMRIP_DRIVE_DVDRW          = 1 << 5,
  OGMRIP_DRIVE_DVDR_PLUS      = 1 << 6,
  OGMRIP_DRIVE_DVDRW_PLUS     = 1 << 7,
  OGMRIP_DRIVE_DVDR_PLUS_DL   = 1 << 8,
  OGMRIP_DRIVE_DVDRW_PLUS_DL  = 1 << 9,
  OGMRIP_DRIVE_DVDRAM         = 1 << 10,
  OGMRIP_DRIVE_BD             = 1 << 11,
  OGMRIP_DRIVE_BDR            = 1 << 12,
  OGMRIP_DRIVE_BDRW           = 1 << 13
} OGMRipDriveType;

/**
 * OGMRipMediumType:
 * @OGMRIP_MEDIUM_NONE: No medium is available
 * @OGMRIP_MEDIUM_UNKNOWN: Unknown medium
 * @OGMRIP_MEDIUM_CD: The medium is a CD
 * @OGMRIP_MEDIUM_CDR: The medium is a CD-R
 * @OGMRIP_MEDIUM_CDRW: The medium is a CD-RW
 * @OGMRIP_MEDIUM_DVD: The medium is a DVD
 * @OGMRIP_MEDIUM_DVDR: The medium is a DVD-R
 * @OGMRIP_MEDIUM_DVDRW: The medium is a DVD-RW
 * @OGMRIP_MEDIUM_DVDR_DL: The medium is a DVD-R DL
 * @OGMRIP_MEDIUM_DVD_RAM: The medium is a DVD-Ram
 * @OGMRIP_MEDIUM_DVD_PLUS_R: The medium is a DVD+R
 * @OGMRIP_MEDIUM_DVD_PLUS_RW: The medium is a DVD+RW
 * @OGMRIP_MEDIUM_DVD_PLUS_R_DL: The medium is a DVD+R DL
 * @OGMRIP_MEDIUM_DVD_PLUS_RW_DL: The medium is a DVD+RW DL
 * @OGMRIP_MEDIUM_BD: The medium is a BD
 * @OGMRIP_MEDIUM_BDR: The medium is a BD-R
 * @OGMRIP_MEDIUM_BDRW: The medium is a BD-RW
 *
 * The type of the medium in the drive.
 */
typedef enum
{
  OGMRIP_MEDIUM_NONE,
  OGMRIP_MEDIUM_UNKNOWN,
  OGMRIP_MEDIUM_CD,
  OGMRIP_MEDIUM_CDR,
  OGMRIP_MEDIUM_CDRW,
  OGMRIP_MEDIUM_DVD,
  OGMRIP_MEDIUM_DVDR,
  OGMRIP_MEDIUM_DVDRW,
  OGMRIP_MEDIUM_DVDR_DL,
  OGMRIP_MEDIUM_DVD_RAM,
  OGMRIP_MEDIUM_DVD_PLUS_R,
  OGMRIP_MEDIUM_DVD_PLUS_RW,
  OGMRIP_MEDIUM_DVD_PLUS_R_DL,
  OGMRIP_MEDIUM_DVD_PLUS_RW_DL,
  OGMRIP_MEDIUM_BD,
  OGMRIP_MEDIUM_BDR,
  OGMRIP_MEDIUM_BDRW
} OGMRipMediumType;

typedef struct _OGMRipDrive      OGMRipDrive;
typedef struct _OGMRipDriveClass OGMRipDriveClass;
typedef struct _OGMRipDrivePriv  OGMRipDrivePriv;

struct _OGMRipDrive
{
  GObject parent_instance;

  OGMRipDrivePriv *priv;
};

struct _OGMRipDriveClass
{
  GObjectClass parent_class;

  void (* medium_added)   (OGMRipDrive *drive);
  void (* medium_removed) (OGMRipDrive *drive);
};

GType         ogmrip_drive_get_type        (void) G_GNUC_CONST;

const gchar * ogmrip_drive_get_device      (OGMRipDrive *drive);
GDrive *      ogmrip_drive_get_gdrive      (OGMRipDrive *drive);

gchar *       ogmrip_drive_get_name        (OGMRipDrive *drive);

gint          ogmrip_drive_get_drive_type  (OGMRipDrive *drive);
gint          ogmrip_drive_get_medium_type (OGMRipDrive *drive);

gboolean      ogmrip_drive_can_eject       (OGMRipDrive *drive);
void          ogmrip_drive_eject           (OGMRipDrive *drive);
void          ogmrip_drive_load            (OGMRipDrive *drive);
gboolean      ogmrip_drive_is_door_open    (OGMRipDrive *drive);

gboolean      ogmrip_drive_lock            (OGMRipDrive *drive);
gboolean      ogmrip_drive_unlock          (OGMRipDrive *drive);

G_END_DECLS

#endif /* __OGMRIP_DRIVE_OBJECT_H__ */

