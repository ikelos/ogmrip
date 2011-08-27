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

#ifndef __OGMDVD_DRIVE_H__
#define __OGMDVD_DRIVE_H__

#include <glib-object.h>

#include <gio/gio.h>

G_BEGIN_DECLS

#define OGMDVD_TYPE_DRIVE             (ogmdvd_drive_get_type ())
#define OGMDVD_DRIVE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMDVD_TYPE_DRIVE, OGMDvdDrive))
#define OGMDVD_DRIVE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMDVD_TYPE_DRIVE, OGMDvdDriveClass))
#define OGMDVD_IS_DRIVE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMDVD_TYPE_DRIVE))
#define OGMDVD_IS_DRIVE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMDVD_TYPE_DRIVE))
#define OGMDVD_DRIVE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMDVD_TYPE_DRIVE, OGMDvdDriveClass))

/**
 * OGMDvdDriveType:
 * @OGMDVD_DRIVE_NONE: No drive is available
 * @OGMDVD_DRIVE_CDROM: The drive can read CDROM
 * @OGMDVD_DRIVE_CDR: The drive can write CD-R
 * @OGMDVD_DRIVE_CDRW: The drive can write CD-RW
 * @OGMDVD_DRIVE_DVD: The drive can read DVD
 * @OGMDVD_DRIVE_DVDR: The drive can write DVD-R
 * @OGMDVD_DRIVE_DVDRW: The drive can write DVD-RW
 * @OGMDVD_DRIVE_DVDR_PLUS: The drive can write DVD+R
 * @OGMDVD_DRIVE_DVDRW_PLUS: The drive can write DVD+RW
 * @OGMDVD_DRIVE_DVDR_PLUS_DL: The drive can write DVD+R DL
 * @OGMDVD_DRIVE_DVDRW_PLUS_DL: The drive can write DVD+RW DL
 * @OGMDVD_DRIVE_DVDRAM: The drive can read DVD-Ram
 * @OGMDVD_DRIVE_BD: The drive can read BD
 * @OGMDVD_DRIVE_BDR: The drive can write BD-R
 * @OGMDVD_DRIVE_BDRW: The drive can write BD-RW
 *
 * The type of the drive is a combination these constants.
 */
typedef enum
{
  OGMDVD_DRIVE_NONE           = 0,
  OGMDVD_DRIVE_CDROM          = 1 << 0,
  OGMDVD_DRIVE_CDR            = 1 << 1,
  OGMDVD_DRIVE_CDRW           = 1 << 2,
  OGMDVD_DRIVE_DVD            = 1 << 3,
  OGMDVD_DRIVE_DVDR           = 1 << 4,
  OGMDVD_DRIVE_DVDRW          = 1 << 5,
  OGMDVD_DRIVE_DVDR_PLUS      = 1 << 6,
  OGMDVD_DRIVE_DVDRW_PLUS     = 1 << 7,
  OGMDVD_DRIVE_DVDR_PLUS_DL   = 1 << 8,
  OGMDVD_DRIVE_DVDRW_PLUS_DL  = 1 << 9,
  OGMDVD_DRIVE_DVDRAM         = 1 << 10,
  OGMDVD_DRIVE_BD             = 1 << 11,
  OGMDVD_DRIVE_BDR            = 1 << 12,
  OGMDVD_DRIVE_BDRW           = 1 << 13
} OGMDvdDriveType;

/**
 * OGMDvdMediumType:
 * @OGMDVD_MEDIUM_NONE: No medium is available
 * @OGMDVD_MEDIUM_UNKNOWN: Unknown medium
 * @OGMDVD_MEDIUM_CD: The medium is a CD
 * @OGMDVD_MEDIUM_CDR: The medium is a CD-R
 * @OGMDVD_MEDIUM_CDRW: The medium is a CD-RW
 * @OGMDVD_MEDIUM_DVD: The medium is a DVD
 * @OGMDVD_MEDIUM_DVDR: The medium is a DVD-R
 * @OGMDVD_MEDIUM_DVDRW: The medium is a DVD-RW
 * @OGMDVD_MEDIUM_DVDR_DL: The medium is a DVD-R DL
 * @OGMDVD_MEDIUM_DVD_RAM: The medium is a DVD-Ram
 * @OGMDVD_MEDIUM_DVD_PLUS_R: The medium is a DVD+R
 * @OGMDVD_MEDIUM_DVD_PLUS_RW: The medium is a DVD+RW
 * @OGMDVD_MEDIUM_DVD_PLUS_R_DL: The medium is a DVD+R DL
 * @OGMDVD_MEDIUM_DVD_PLUS_RW_DL: The medium is a DVD+RW DL
 * @OGMDVD_MEDIUM_BD: The medium is a BD
 * @OGMDVD_MEDIUM_BDR: The medium is a BD-R
 * @OGMDVD_MEDIUM_BDRW: The medium is a BD-RW
 *
 * The type of the medium in the drive.
 */
typedef enum
{
  OGMDVD_MEDIUM_NONE,
  OGMDVD_MEDIUM_UNKNOWN,
  OGMDVD_MEDIUM_CD,
  OGMDVD_MEDIUM_CDR,
  OGMDVD_MEDIUM_CDRW,
  OGMDVD_MEDIUM_DVD,
  OGMDVD_MEDIUM_DVDR,
  OGMDVD_MEDIUM_DVDRW,
  OGMDVD_MEDIUM_DVDR_DL,
  OGMDVD_MEDIUM_DVD_RAM,
  OGMDVD_MEDIUM_DVD_PLUS_R,
  OGMDVD_MEDIUM_DVD_PLUS_RW,
  OGMDVD_MEDIUM_DVD_PLUS_R_DL,
  OGMDVD_MEDIUM_DVD_PLUS_RW_DL,
  OGMDVD_MEDIUM_BD,
  OGMDVD_MEDIUM_BDR,
  OGMDVD_MEDIUM_BDRW
} OGMDvdMediumType;

typedef struct _OGMDvdDrive      OGMDvdDrive;
typedef struct _OGMDvdDriveClass OGMDvdDriveClass;
typedef struct _OGMDvdDrivePriv  OGMDvdDrivePriv;

struct _OGMDvdDrive
{
  GObject parent_instance;

  OGMDvdDrivePriv *priv;
};

struct _OGMDvdDriveClass
{
  GObjectClass parent_class;

  void (* medium_added)   (OGMDvdDrive *drive);
  void (* medium_removed) (OGMDvdDrive *drive);
};

GType         ogmdvd_drive_get_type        (void) G_GNUC_CONST;

const gchar * ogmdvd_drive_get_device      (OGMDvdDrive *drive);
GDrive *      ogmdvd_drive_get_gdrive      (OGMDvdDrive *drive);

gchar *       ogmdvd_drive_get_name        (OGMDvdDrive *drive);

gint          ogmdvd_drive_get_drive_type  (OGMDvdDrive *drive);
gint          ogmdvd_drive_get_medium_type (OGMDvdDrive *drive);

gboolean      ogmdvd_drive_can_eject       (OGMDvdDrive *drive);
void          ogmdvd_drive_eject           (OGMDvdDrive *drive);
void          ogmdvd_drive_load            (OGMDvdDrive *drive);
gboolean      ogmdvd_drive_is_door_open    (OGMDvdDrive *drive);

gboolean      ogmdvd_drive_lock            (OGMDvdDrive *drive);
gboolean      ogmdvd_drive_unlock          (OGMDvdDrive *drive);

G_END_DECLS

#endif /* __OGMDVD_DRIVE_H__ */
