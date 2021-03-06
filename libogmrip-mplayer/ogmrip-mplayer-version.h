/* OGMRipMplayer - A library around mplayer/mencoder for OGMRip
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

#ifndef __OGMRIP_MPLAYER_VERSION_H__
#define __OGMRIP_MPLAYER_VERSION_H__

#include <glib.h>

G_BEGIN_DECLS

/**
 * MPLAYER_CHECK_VERSION:
 * @major: A major version number
 * @minor: A minor version number
 * @rc: An rc version number
 * @pre: A pre version number
 *
 * Check if version is equal or greather than major.minor, major.minor-rc or
 * major.minor-pre, in that order
 */
#define MPLAYER_CHECK_VERSION(major,minor,rc,pre) \
  (ogmrip_check_mplayer_version (major, minor, rc, pre))

gboolean ogmrip_check_mplayer         (void);
gboolean ogmrip_check_mencoder        (void);

gboolean ogmrip_check_mplayer_version (gint major,
                                       gint minor,
                                       gint rc,
                                       gint pre);
gboolean ogmrip_check_mplayer_dts     (void);

G_END_DECLS

#endif /* __OGMRIP_MPLAYER_VERSION_H__ */

