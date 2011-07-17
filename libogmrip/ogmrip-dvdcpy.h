/* OGMRip - A library for DVD ripping and encoding
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

#ifndef __OGMRIP_DVDCPY_H__
#define __OGMRIP_DVDCPY_H__

#include <ogmrip-video-codec.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_DVDCPY          (ogmrip_dvdcpy_get_type ())
#define OGMRIP_DVDCPY(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_DVDCPY, OGMRipDvdCpy))
#define OGMRIP_DVDCPY_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_DVDCPY, OGMRipDvdCpyClass))
#define OGMRIP_IS_DVDCPY(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_DVDCPY))
#define OGMRIP_IS_DVDCPY_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_DVDCPY))

typedef struct _OGMRipDvdCpy      OGMRipDvdCpy;
typedef struct _OGMRipDvdCpyClass OGMRipDvdCpyClass;

struct _OGMRipDvdCpy
{
  OGMRipCodec parent_instance;
};

struct _OGMRipDvdCpyClass
{
  OGMRipCodecClass parent_class;
};

GType         ogmrip_dvdcpy_get_type  (void);
OGMJobSpawn * ogmrip_dvdcpy_new       (OGMDvdTitle *title,
                                       const gchar *output);

G_END_DECLS

#endif /* __OGMRIP_DVDCPY_H__ */

