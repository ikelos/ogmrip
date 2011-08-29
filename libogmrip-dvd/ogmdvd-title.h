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

#ifndef __OGMDVD_TITLE_H__
#define __OGMDVD_TITLE_H__

#include <ogmrip-media.h>

G_BEGIN_DECLS

#define OGMDVD_TYPE_TITLE             (ogmdvd_title_get_type ())
#define OGMDVD_TITLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMDVD_TYPE_TITLE, OGMDvdTitle))
#define OGMDVD_IS_TITLE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMDVD_TYPE_TITLE))
#define OGMDVD_TITLE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMDVD_TYPE_TITLE, OGMDvdTitleClass))
#define OGMDVD_IS_TITLE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMDVD_TYPE_TITLE))
#define OGMDVD_TITLE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMDVD_TYPE_TITLE, OGMDvdTitleClass))

typedef struct _OGMDvdTitle      OGMDvdTitle;
typedef struct _OGMDvdTitleClass OGMDvdTitleClass;
typedef struct _OGMDvdTitlePriv  OGMDvdTitlePriv;

struct _OGMDvdTitle
{
  GObject parent_instance;

  OGMDvdTitlePriv *priv;
};

struct _OGMDvdTitleClass
{
  GObjectClass parent_class;
};

GType ogmdvd_title_get_type (void);

G_END_DECLS

#endif /* __OGMDVD_TITLE_H__ */

