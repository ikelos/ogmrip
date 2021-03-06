/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_MEDIA_TIME_H__
#define __OGMRIP_MEDIA_TIME_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct
{
  gulong hour;
  gulong min;
  gulong sec;
  gulong msec;
} OGMRipTime;

gulong  ogmrip_time_to_msec (OGMRipTime *dtime);
void    ogmrip_msec_to_time (gulong     msec,
                             OGMRipTime *dtime);

G_END_DECLS

#endif /* __OGMRIP_MEDIA_TIME_H__ */

