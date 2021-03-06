/* OGMRipDvd - A DVD library for OGMRip
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMDVD_DISC_H__
#define __OGMDVD_DISC_H__

#include <ogmrip-media.h>

G_BEGIN_DECLS

typedef enum
{
  OGMRIP_DVD_ERROR_TRAY,   /* Tray seems to be open */
  OGMRIP_DVD_ERROR_DEV,    /* Device does not contain a valid DVD video */
  OGMRIP_DVD_ERROR_PATH,   /* Path does not contain a valid DVD structure */
  OGMRIP_DVD_ERROR_ACCESS, /* No such directory, block device or iso file */
  OGMRIP_DVD_ERROR_VMG     /* Cannot open DVD manager */
} OGMRipDvdError;

#define OGMDVD_TYPE_DISC             (ogmdvd_disc_get_type ())
#define OGMDVD_DISC(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMDVD_TYPE_DISC, OGMDvdDisc))
#define OGMDVD_IS_DISC(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMDVD_TYPE_DISC))
#define OGMDVD_DISC_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMDVD_TYPE_DISC, OGMDvdDiscClass))
#define OGMDVD_IS_DISC_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMDVD_TYPE_DISC))
#define OGMDVD_DISC_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMDVD_TYPE_DISC, OGMDvdDiscClass))

typedef struct _OGMDvdDisc      OGMDvdDisc;
typedef struct _OGMDvdDiscClass OGMDvdDiscClass;
typedef struct _OGMDvdDiscPriv  OGMDvdDiscPrivate;

struct _OGMDvdDisc
{
  GObject parent_instance;

  OGMDvdDiscPrivate *priv;
};

struct _OGMDvdDiscClass
{
  GObjectClass parent_class;
};

void ogmrip_dvd_register_media (void);

GType         ogmdvd_disc_get_type (void);
OGMRipMedia * ogmdvd_disc_new      (const gchar *device,
                                    GError      **error);

G_END_DECLS

#endif /* __OGMDVD_DISC_H__ */

