/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_COPY_H__
#define __OGMRIP_COPY_H__

#include <ogmrip-job.h>
#include <ogmrip-media.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_COPY          (ogmrip_copy_get_type ())
#define OGMRIP_COPY(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_COPY, OGMRipCopy))
#define OGMRIP_COPY_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_COPY, OGMRipCopyClass))
#define OGMRIP_IS_COPY(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_COPY))
#define OGMRIP_IS_COPY_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_COPY))

typedef struct _OGMRipCopy      OGMRipCopy;
typedef struct _OGMRipCopyClass OGMRipCopyClass;
typedef struct _OGMRipCopyPriv  OGMRipCopyPriv;

struct _OGMRipCopy
{
  OGMJobSpawn parent_instance;

  OGMRipCopyPriv *priv;
};

struct _OGMRipCopyClass
{
  OGMJobSpawnClass parent_class;
};

GType        ogmrip_copy_get_type (void);
OGMJobTask * ogmrip_copy_new      (OGMRipMedia *media,
                                   const gchar *path);

G_END_DECLS

#endif /* __OGMRIP_COPY_H__ */

