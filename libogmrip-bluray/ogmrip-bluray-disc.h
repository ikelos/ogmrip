/* OGMRipBluray - A Bluray library for OGMBr
 * Copyright (C) 2012 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMBR_DISC_H__
#define __OGMBR_DISC_H__

#include <ogmrip-media.h>

G_BEGIN_DECLS

typedef enum
{
  OGMBR_DIR_UNKNOWN  = 0,
  OGMBR_DISC_DVD     = 6206,
  OGMBR_DISC_BLURAY  = 6209,
  OGMBR_DISC_HDDVD   = 6212
} OGMBrDiscType;

#define OGMBR_TYPE_DISC             (ogmbr_disc_get_type ())
#define OGMBR_DISC(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMBR_TYPE_DISC, OGMBrDisc))
#define OGMBR_IS_DISC(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMBR_TYPE_DISC))
#define OGMBR_DISC_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMBR_TYPE_DISC, OGMBrDiscClass))
#define OGMBR_IS_DISC_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMBR_TYPE_DISC))
#define OGMBR_DISC_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMBR_TYPE_DISC, OGMBrDiscClass))

typedef struct _OGMBrDisc      OGMBrDisc;
typedef struct _OGMBrDiscClass OGMBrDiscClass;
typedef struct _OGMBrDiscPriv  OGMBrDiscPriv;

struct _OGMBrDisc
{
  GObject parent_instance;

  OGMBrDiscPriv *priv;
};

struct _OGMBrDiscClass
{
  GObjectClass parent_class;
};

void ogmrip_bluray_register_media  (void);

GType         ogmbr_disc_get_type  (void);
OGMRipMedia * ogmbr_disc_new       (const gchar *device,
                                    GError      **error);

G_END_DECLS

#endif /* __OGMBR_DISC_H__ */

