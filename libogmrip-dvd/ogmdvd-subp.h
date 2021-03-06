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

#ifndef __OGMDVD_SUBP_H__
#define __OGMDVD_SUBP_H__

#include <ogmrip-media.h>

G_BEGIN_DECLS

#define OGMDVD_TYPE_SUBP_STREAM             (ogmdvd_subp_stream_get_type ())
#define OGMDVD_SUBP_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMDVD_TYPE_SUBP_STREAM, OGMDvdSubpStream))
#define OGMDVD_IS_SUBP_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMDVD_TYPE_SUBP_STREAM))
#define OGMDVD_SUBP_STREAM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMDVD_TYPE_SUBP_STREAM, OGMDvdSubpStreamClass))
#define OGMDVD_IS_SUBP_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMDVD_TYPE_SUBP_STREAM))
#define OGMDVD_SUBP_STREAM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMDVD_TYPE_SUBP_STREAM, OGMDvdSubpStreamClass))

typedef struct _OGMDvdSubpStream      OGMDvdSubpStream;
typedef struct _OGMDvdSubpStreamClass OGMDvdSubpStreamClass;
typedef struct _OGMDvdSubpStreamPriv  OGMDvdSubpStreamPrivate;

struct _OGMDvdSubpStream
{
  GObject parent_instance;

  OGMDvdSubpStreamPrivate *priv;
};

struct _OGMDvdSubpStreamClass
{
  GObjectClass parent_class;
};

GType ogmdvd_subp_stream_get_type (void);

G_END_DECLS

#endif /* __OGMDVD_SUBP_H__ */

