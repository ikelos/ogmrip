/* OGMRipFile - A file library for OGMRip
 * Copyright (C) 2010-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_FILE_PRIV_H__
#define __OGMRIP_FILE_PRIV_H__

struct _OGMRipFilePriv
{
  gchar *uri;
  gchar *path;
  gchar *label;
  gdouble length;
  gint64 size;
  gint format;
};

struct _OGMRipVideoFilePriv
{
  gint aspect;
  gint bitrate;
  gint standard;
  guint width;
  guint height;
  guint framerate_num;
  guint framerate_denom;
  guint aspect_num;
  guint aspect_denom;
};

struct _OGMRipAudioFilePriv
{
  gint bitrate;
  gint channels;
  gint language;
  gint samplerate;
};

struct _OGMRipSubpFilePriv
{
  gint language;
};

#endif /* __OGMRIP_FILE_PRIV_H__ */

