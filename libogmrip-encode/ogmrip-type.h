/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_TYPE_H__
#define __OGMRIP_TYPE_H__

#include <ogmrip-media.h>
#include <ogmrip-module.h>

G_BEGIN_DECLS

void          ogmrip_register_codec     (GType          gtype,
                                         const gchar    *name,
                                         const gchar    *description,
                                         OGMRipFormat   format);
OGMRipFormat  ogmrip_codec_format       (GType          gtype);
void          ogmrip_register_container (GType          gtype,
                                         const gchar    *name,
                                         const gchar    *description,
                                         OGMRipFormat   *format);
gboolean      ogmrip_container_contains (GType          gtype,
                                         OGMRipFormat   format);

G_END_DECLS

#endif /* __OGMRIP_TYPE_H__ */

