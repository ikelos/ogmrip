/* OGMDvd - A wrapper library around libdvdread
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
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

#include <ogmdvd-types.h>

G_BEGIN_DECLS

#define OGMDVD_DISC_ERROR ogmdvd_error_quark ()

/**
 * OGMDvdDiscError:
 * @OGMDVD_DISC_ERROR_EXIST: No such file or directory
 * @OGMDVD_DISC_ERROR_PERM: Permission denied to access device
 * @OGMDVD_DISC_ERROR_TRAY: Tray seems to be open
 * @OGMDVD_DISC_ERROR_DEV: Device does not contain a valid DVD video
 * @OGMDVD_DISC_ERROR_PATH: Path does not contain a valid DVD structure
 * @OGMDVD_DISC_ERROR_ACCESS: No such directory, block device or iso file
 * @OGMDVD_DISC_ERROR_VMG: Cannot open video manager
 * @OGMDVD_DISC_ERROR_VTS: Cannot open video titleset
 * @OGMDVD_DISC_ERROR_ID: The DVD identifiers differ
 * @OGMDVD_DISC_ERROR_UNKNOWN: Unknown error
 *
 * Error codes returned by ogmdvd_disc_open()
 */
typedef enum
{
  OGMDVD_DISC_ERROR_EXIST,  /* No such file or directory */
  OGMDVD_DISC_ERROR_PERM,   /* Permission denied to access device */
  OGMDVD_DISC_ERROR_TRAY,   /* Tray seems to be open */
  OGMDVD_DISC_ERROR_DEV,    /* Device does not contain a valid DVD video */
  OGMDVD_DISC_ERROR_PATH,   /* Path does not contain a valid DVD structure */
  OGMDVD_DISC_ERROR_ACCESS, /* No such directory, block device or iso file */
  OGMDVD_DISC_ERROR_VMG,    /* Cannot open video manager */
  OGMDVD_DISC_ERROR_VTS,    /* Cannot open video titleset */
  OGMDVD_DISC_ERROR_ID,     /* The DVD IDs differ */
  OGMDVD_DISC_ERROR_UNKNOWN /* Unknown error */
} OGMDvdDiscError;

typedef void (* OGMDvdDiscCallback) (OGMDvdDisc *disc,
                                     gdouble    percent,
                                     gpointer   user_data);

GQuark        ogmdvd_error_quark        (void);

OGMDvdDisc *  ogmdvd_disc_new           (const gchar        *device,
                                         GError             **error);

gboolean      ogmdvd_disc_open          (OGMDvdDisc         *disc,
                                         GError             **error);
void          ogmdvd_disc_close         (OGMDvdDisc         *disc);

gboolean      ogmdvd_disc_is_open       (OGMDvdDisc         *disc);

void          ogmdvd_disc_ref           (OGMDvdDisc         *disc);
void          ogmdvd_disc_unref         (OGMDvdDisc         *disc);
const gchar * ogmdvd_disc_get_label     (OGMDvdDisc         *disc);
const gchar * ogmdvd_disc_get_id        (OGMDvdDisc         *disc);
const gchar * ogmdvd_disc_get_device    (OGMDvdDisc         *disc);
gint64        ogmdvd_disc_get_vmg_size  (OGMDvdDisc         *disc);
gint          ogmdvd_disc_get_n_titles  (OGMDvdDisc         *disc);
OGMDvdTitle * ogmdvd_disc_get_nth_title (OGMDvdDisc         *disc,
                                         guint              nr);
GList *       ogmdvd_disc_get_titles    (OGMDvdDisc         *disc);
gboolean      ogmdvd_disc_copy          (OGMDvdDisc         *title,
                                         const gchar        *path,
                                         OGMDvdDiscCallback callback,
                                         gpointer           user_data,
                                         GError             **error);

G_END_DECLS

#endif /* __OGMDVD_DISC_H__ */

