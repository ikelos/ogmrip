/* OGMRipFile - A file library for OGMRip
 * Copyright (C) 2010-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#include <ogmrip-media-info.h>
#include <ogmrip-video-file.h>
#include <ogmrip-audio-file.h>
#include <ogmrip-subp-file.h>

G_BEGIN_DECLS

struct _OGMRipFilePriv
{
  gchar *uri;
  gchar *path;
  gchar *label;
  gdouble length;
  gint64 media_size;
  gint64 title_size;
  gint format;
};

struct _OGMRipVideoFilePriv
{
  gint bitrate;
  gint standard;
  guint width;
  guint height;
  guint framerate_num;
  guint framerate_denom;
  guint aspect_num;
  guint aspect_denom;
  gint64 size;
};

struct _OGMRipAudioFilePriv
{
  gint bitrate;
  gint channels;
  gint language;
  gint samplerate;
  gint64 size;
  gchar *label;
};

struct _OGMRipSubpFilePriv
{
  gint language;
  gint64 size;
  gchar *label;
};

struct _OGMRipMediaFilePriv
{
  GList *audio_streams;
  GList *subp_streams;
};

void ogmrip_media_info_get_file_info    (OGMRipMediaInfo     *info,
                                         OGMRipFilePriv      *file);
gint ogmrip_media_info_get_video_format (OGMRipMediaInfo     *info,
                                         guint               track);
void ogmrip_media_info_get_video_info   (OGMRipMediaInfo     *info,
                                         guint               track,
                                         OGMRipVideoFilePriv *video);
gint ogmrip_media_info_get_audio_format (OGMRipMediaInfo     *info,
                                         guint               track);
void ogmrip_media_info_get_audio_info   (OGMRipMediaInfo     *info,
                                         guint               track,
                                         OGMRipAudioFilePriv *audio);
gint ogmrip_media_info_get_subp_format  (OGMRipMediaInfo     *info,
                                         guint               track);
void ogmrip_media_info_get_subp_info    (OGMRipMediaInfo     *info,
                                         guint               track,
                                         OGMRipSubpFilePriv  *subp);

G_END_DECLS

#endif /* __OGMRIP_FILE_PRIV_H__ */

