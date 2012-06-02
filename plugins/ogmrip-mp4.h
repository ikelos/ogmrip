/* OGMRip - A library for DVD ripping and encoding
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

#ifndef __OGMRIP_MP4_H__
#define __OGMRIP_MP4_H__

#include <ogmrip-encode.h>

G_BEGIN_DECLS

#define OGMRIP_MP4_PROP_FORMAT "format"
#define OGMRIP_MP4_PROP_HINT   "hint"

#define OGMRIP_MP4_DEFAULT_FORMAT 0
#define OGMRIP_MP4_DEFAULT_HINT   FALSE

#define OGMRIP_TYPE_MP4 (ogmrip_mp4_get_type ())

GType ogmrip_mp4_get_type (void);

G_END_DECLS

#endif /* __OGMRIP_MP4_H__ */

