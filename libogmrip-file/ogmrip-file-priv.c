/* OGMRipFile - A file library for OGMRip
 * Copyright (C) 2010-2013 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-file-priv.h"

#include <ogmrip-media.h>

#include <stdio.h>
#include <stdlib.h>

void
ogmrip_media_info_get_file_info (OGMRipMediaInfo *info, OGMRipFilePriv *file)
{
  const gchar *str;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_GENERAL, 0, "Duration");
  file->length = str ? atoi (str) : -1.0;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_GENERAL, 0, "FileSize");
  file->media_size = str ? atoll (str) : -1;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_GENERAL, 0, "Title");
  file->label = str ? g_strdup (str) : NULL;
}

gint
ogmrip_media_info_get_video_format (OGMRipMediaInfo *info, guint track)
{
  const gchar *str;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, track, "Format");
  if (!str)
    return OGMRIP_FORMAT_UNDEFINED;

  if (g_str_equal (str, "MPEG Video"))
  {
    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, track, "Format_Version");

    if (str && g_str_equal (str, "Version 1"))
      return OGMRIP_FORMAT_MPEG1;

    if (str && g_str_equal (str, "Version 2"))
      return OGMRIP_FORMAT_MPEG2;

    return OGMRIP_FORMAT_UNDEFINED;
  }

  if (g_str_equal (str, "JPEG"))
  {
    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, track, "CodecID");

    if (str && g_str_equal (str, "MJPG"))
      return OGMRIP_FORMAT_MJPEG;

    return OGMRIP_FORMAT_UNDEFINED;
  }

  if (g_str_equal (str, "MPEG-4 Visual"))
    return OGMRIP_FORMAT_MPEG4;

  if (g_str_equal (str, "AVC"))
    return OGMRIP_FORMAT_H264;

  if (g_str_equal (str, "Dirac"))
    return OGMRIP_FORMAT_DIRAC;

  if (g_str_equal (str, "Theora"))
    return OGMRIP_FORMAT_THEORA;

  if (g_str_equal (str, "VP8"))
    return OGMRIP_FORMAT_VP8;

  return OGMRIP_FORMAT_UNDEFINED;
}

void
ogmrip_media_info_get_video_info (OGMRipMediaInfo *info, guint track, OGMRipVideoFilePriv *video)
{
  const gchar *str;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, track, "BitRate");
  video->bitrate = str ? atoi (str) : -1;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, track, "Width");
  video->width = str ? atoi (str) : 0;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, track, "Height");
  video->height = str ? atoi (str) : 0;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, track, "Standard");
  if (str)
  {
    if (g_str_equal (str, "PAL"))
      video->standard = OGMRIP_STANDARD_PAL;
    else if (g_str_equal (str, "NTSC"))
      video->standard = OGMRIP_STANDARD_NTSC;
  }

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, track, "FrameRate");
  if (str)
  {
    if (g_str_equal (str, "25.000"))
      video->framerate_num = 25;
    else if (g_str_equal (str, "23.976"))
    {
      video->framerate_num = 24000;
      video->framerate_num = 1001;
    }
    else if (g_str_equal (str, "29.970"))
    {
      video->framerate_num = 30000;
      video->framerate_num = 1001;
    }
    else
    {
      gdouble framerate;
      
      framerate = strtod (str, NULL);
      video->framerate_num = framerate * 1000;
      video->framerate_num = 1000;
    }
  }

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, track, "DisplayAspectRatio/String");
  if (str)
  {
    gdouble aspect;
    gchar *endptr;

    aspect = g_ascii_strtod (str, &endptr);
    g_assert (endptr != str);

    video->aspect_denom = 1;

    if (*endptr == ':')
    {
      str = endptr + 1;

      video->aspect_denom = strtoul (str, &endptr, 10);
      g_assert (endptr != str);

      if (video->aspect_denom > 1)
        video->aspect_num = aspect;
    }

    if (video->aspect_denom > 1)
      video->aspect_num = aspect;
    else
    {
      video->aspect_num = aspect * 1000;
      video->aspect_denom = 1000;
    }
  }

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, track, "StreamSize");
  video->size = str ? atoll (str) : -1;
}

gint
ogmrip_media_info_get_audio_format (OGMRipMediaInfo *info, guint track)
{
  const gchar *str;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, track, "Format");
  if (!str)
    return OGMRIP_FORMAT_UNDEFINED;

  if (g_str_equal (str, "MPEG Audio"))
  {
    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, track, "Format_Profile");

    if (str && g_str_equal (str, "Layer 3"))
      return OGMRIP_FORMAT_MP3;

    if (str && g_str_equal (str, "Layer 2"))
      return OGMRIP_FORMAT_MP2;

    return OGMRIP_FORMAT_UNDEFINED;
  }

  if (g_str_equal (str, "AAC"))
    return OGMRIP_FORMAT_AAC;

  if (g_str_equal (str, "FLAC"))
    return OGMRIP_FORMAT_FLAC;

  if (g_str_equal (str, "DTS"))
    return OGMRIP_FORMAT_DTS;

  if (g_str_equal (str, "AC-3"))
    return OGMRIP_FORMAT_AC3;

  if (g_str_equal (str, "Vorbis"))
    return OGMRIP_FORMAT_VORBIS;

  if (g_str_equal (str, "PCM"))
    return OGMRIP_FORMAT_PCM;

  return OGMRIP_FORMAT_UNDEFINED;
}

void
ogmrip_media_info_get_audio_info (OGMRipMediaInfo *info, guint track, OGMRipAudioFilePriv *audio)
{
  const gchar *str;
  gint channels;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, track, "BitRate");
  audio->bitrate = str ? atoi (str) : -1;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, track, "Channel(s)");
  channels = str ? atoi (str) : -1;
  switch (channels)
  {
    case 1:
      audio->channels = OGMRIP_CHANNELS_MONO;
      break;
    case 2:
      audio->channels = OGMRIP_CHANNELS_STEREO;
      break;
    case 4:
      audio->channels = OGMRIP_CHANNELS_SURROUND;
      break;
    case 6:
      audio->channels = OGMRIP_CHANNELS_5_1;
      break;
    case 7:
      audio->channels = OGMRIP_CHANNELS_6_1;
      break;
    case 8:
      audio->channels = OGMRIP_CHANNELS_7_1;
      break;
    default:
      audio->channels = OGMRIP_CHANNELS_UNDEFINED;
      break;
  }

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, track, "SamplingRate");
  audio->samplerate = str ? atoi (str) : -1;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, track, "Language/String2");
  audio->language = str ? (str[0] << 8) | str[1] : 0;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, track, "StreamSize");
  audio->size = str ? atoll (str) : -1;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, track, "Title");
  audio->label = str ? g_strdup (str) : NULL;
}

gint
ogmrip_media_info_get_subp_format (OGMRipMediaInfo *info, guint track)
{
  const gchar *str;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_TEXT, track, "Format");
  if (!str)
    return OGMRIP_FORMAT_UNDEFINED;

  if (g_str_equal (str, "SubRip"))
    return OGMRIP_FORMAT_SRT;

  if (g_str_equal (str, "VobSub"))
    return OGMRIP_FORMAT_VOBSUB;

  if (g_str_equal (str, "PGS"))
    return OGMRIP_FORMAT_PGS;

  return OGMRIP_FORMAT_UNDEFINED;
}

void
ogmrip_media_info_get_subp_info (OGMRipMediaInfo *info, guint track, OGMRipSubpFilePriv *subp)
{
  const gchar *str;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_TEXT, track, "Language/String2");
  subp->language = str ? (str[0] << 8) | str[1] : 0;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_TEXT, track, "StreamSize");
  subp->size = str ? atoll (str) : -1;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_TEXT, track, "Title");
  subp->label = str ? g_strdup (str) : NULL;
}

gchar *
g_file_get_id (GFile *file)
{
  GFileInfo *info;
  const gchar *str;
  gchar *id = NULL;

  info = g_file_query_info (file, G_FILE_ATTRIBUTE_ID_FILE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
  if (!info)
    return NULL;

  str = g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_ID_FILE);
  if (str)
    id = g_strdup (str);
  g_object_unref (info);

  return id;
}

