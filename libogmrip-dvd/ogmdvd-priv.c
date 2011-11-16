/* OGMDvd - A wrapper library around libdvdread
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmdvd-priv.h"

#include <stdio.h>
#include <string.h>

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

typedef struct
{
  gint val;
  gint ref;
} UInfo;

static gint
g_ulist_min (UInfo *info1, UInfo *info2)
{
  return info2->val - info1->val;
}

static gint
g_ulist_max (UInfo *info1, UInfo *info2)
{
  return info1->val - info2->val;
}

static GSList *
g_ulist_add (GSList *ulist, GCompareFunc func, gint val)
{
  GSList *ulink;
  UInfo *info;

  for (ulink = ulist; ulink; ulink = ulink->next)
  {
    info = ulink->data;

    if (info->val == val)
      break;
  }

  if (ulink)
  {
    info = ulink->data;
    info->ref ++;
  }
  else
  {
    info = g_new0 (UInfo, 1);
    info->val = val;
    info->ref = 1;

    ulist = g_slist_insert_sorted (ulist, info, func);
  }

  return ulist;
}

GSList *
g_ulist_add_min (GSList *ulist, gint val)
{
  return g_ulist_add (ulist, (GCompareFunc) g_ulist_min, val);
}

GSList *
g_ulist_add_max (GSList *ulist, gint val)
{
  return g_ulist_add (ulist, (GCompareFunc) g_ulist_max, val);
}

gint
g_ulist_get_most_frequent (GSList *ulist)
{
  GSList *ulink;
  UInfo *info, *umax;

  if (!ulist)
    return 0;

  umax = ulist->data;

  for (ulink = ulist; ulink; ulink = ulink->next)
  {
    info = ulink->data;

    if (info->ref > umax->ref)
      umax = info;
  }

  return umax->val;
}

void
g_ulist_free (GSList *ulist)
{
  g_slist_foreach (ulist, (GFunc) g_free, NULL);
  g_slist_free (ulist);
}

