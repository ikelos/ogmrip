/* OGMRip - A library for media ripping and encoding
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

#ifndef __OGMRIP_HARDSUB_H__
#define __OGMRIP_HARDSUB_H__

#include <ogmrip-subp-codec.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_HARDSUB          (ogmrip_hardsub_get_type ())
#define OGMRIP_HARDSUB(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_HARDSUB, OGMRipHardSub))
#define OGMRIP_HARDSUB_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_HARDSUB, OGMRipHardSubClass))
#define OGMRIP_IS_HARDSUB(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_HARDSUB))
#define OGMRIP_IS_HARDSUB_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_HARDSUB))

typedef struct _OGMRipHardSub      OGMRipHardSub;
typedef struct _OGMRipHardSubClass OGMRipHardSubClass;

struct _OGMRipHardSub
{
  OGMRipSubpCodec parent_instance;
};

struct _OGMRipHardSubClass
{
  OGMRipSubpCodecClass parent_class;
};

GType ogmrip_hardsub_get_type       (void);
void  ogmrip_hardsub_register_codec (void);

G_END_DECLS

#endif /* __OGMRIP_HARDSUB_H__ */

