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
#include <ogmrip-bluray-disc.h>

#include <libbluray/bluray.h>
#include <libbluray/meta_data.h>

G_BEGIN_DECLS

struct _OGMBrMakeMKVPriv
{
  GList *drives;
};

struct _OGMBrDiscPriv
{
  gchar *uri;
  gchar *device;
  gchar *label;
  GList *titles;

  BLURAY *bd;
  guint selected;

  OGMBrDisc *copy;
  gboolean require_copy;
};

struct _OGMBrTitlePriv
{
  OGMRipMedia *media;

  guint id;
  guint nopen;

  GList *audio_streams;
  GList *subp_streams;

  guint nchapters;
  guint64 *chapters;

  guint nangles;

  guint64 duration;
  guint64 size;

  bd_stream_type_e type;
  bd_video_aspect_e aspect;
  bd_video_rate_e rate;
  bd_video_format_e format;

  guint crop_x;
  guint crop_y;
  guint crop_width;
  guint crop_height;
/*
  gint bitrate;
*/
  gboolean analyzed;
/*
  gboolean telecine;
*/
};

struct _OGMBrAudioStreamPriv
{
  OGMRipTitle *title;

  guint id;

  bd_stream_type_e type;
  bd_audio_format_e format;
  bd_audio_rate_e rate;
  gint lang;
};

struct _OGMBrSubpStreamPriv
{
  OGMRipTitle *title;

  guint id;
/*
  guint content;
*/
  gint lang;
};

G_END_DECLS

#endif /* __OGMRIP_BLURAY_PRIV_H__ */

