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

#ifndef __OGMRIP_PROFILE_PARSER_H__
#define __OGMRIP_PROFILE_PARSER_H__

#include <ogmrip-profile.h>
#include <ogmrip-xml.h>

G_BEGIN_DECLS

gboolean ogmrip_profile_parse (OGMRipProfile *profile,
                               OGMRipXML     *xml,
                               GError        **error);
gboolean ogmrip_profile_dump  (OGMRipProfile *profile,
                               OGMRipXML     *xml,
                               GError        **error);

G_END_DECLS

#endif /* __OGMRIP_PROFILE_PARSER_H__ */

