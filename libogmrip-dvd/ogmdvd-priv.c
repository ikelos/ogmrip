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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmdvd-priv.h"

gulong
ogmdvd_time_to_msec (dvd_time_t *dtime)
{
  guint hour, min, sec, frames;
  gfloat fps;

  hour   = ((dtime->hour    & 0xf0) >> 4) * 10 + (dtime->hour    & 0x0f);
  min    = ((dtime->minute  & 0xf0) >> 4) * 10 + (dtime->minute  & 0x0f);
  sec    = ((dtime->second  & 0xf0) >> 4) * 10 + (dtime->second  & 0x0f);
  frames = ((dtime->frame_u & 0x30) >> 4) * 10 + (dtime->frame_u & 0x0f);

  if (((dtime->frame_u & 0xc0) >> 6) == 1)
    fps = 25.0;
  else
    fps = 30000 / 1001.0;

  return hour * 60 * 60 * 1000 + min * 60 * 1000 + sec * 1000 + (gfloat) frames * 1000.0 / fps;
}

