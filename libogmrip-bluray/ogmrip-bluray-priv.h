/* OGMRipBluray - A bluray library for OGMRip
 * Copyright (C) 2012 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_BLURAY_PRIV_H__
#define __OGMRIP_BLURAY_PRIV_H__

#include <ogmrip-media.h>

G_BEGIN_DECLS

struct _OGMBrDiscPriv
{
  gchar *uri;
  gchar *device;
  guint64 handle;
  guint ntitles;
  GList *titles;

  guint type;
  gchar *label;
};

struct _OGMBrTitlePriv
{
  OGMRipMedia *media;

  guint id;
  guint64 handle;
  GList *audio_streams;
  guint naudio_streams;
  GList *subp_streams;
  guint nsubp_streams;
  guint nchapters;
  gdouble length;
  guint64 size;

  guint vid;
  guint64 vhandle;

  OGMRipFormat format;
  guint aspect_num;
  guint aspect_denom;
  guint raw_width;
  guint raw_height;
  guint rate_num;
  guint rate_denom;
};

struct _OGMBrAudioStreamPriv
{
  OGMRipTitle *title;

  guint id;
  guint64 handle;

  OGMRipFormat format;
  guint channels;
  guint samplerate;
  guint content;
  gint language;
};

struct _OGMBrSubpStreamPriv
{
  OGMRipTitle *title;

  guint id;
  guint64 handle;

  OGMRipFormat format;
  guint content;
  gint language;
};

G_END_DECLS

#endif /* __OGMRIP_BLURAY_PRIV_H__ */

