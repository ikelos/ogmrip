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

#include "ogmrip-audio-file.h"
#include "ogmrip-media-info.h"
#include "ogmrip-file-priv.h"

#include <stdlib.h>

static void ogmrip_audio_iface_init (OGMRipAudioStreamInterface *iface);
static GObject * ogmrip_audio_file_constructor  (GType type,
                                                 guint n_properties,
                                                 GObjectConstructParam *properties);

static gint
ogmrip_media_info_get_audio_format (OGMRipMediaInfo *info)
{
  const gchar *str;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, 0, "Format");
  if (!str)
    return OGMRIP_FORMAT_UNDEFINED;

  if (g_str_equal (str, "MPEG Audio"))
  {
    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, 0, "Format_Profile");

    if (str && g_str_equal (str, "Layer 3"))
      return OGMRIP_FORMAT_MP3;

    if (str && g_str_equal (str, "Layer 2"))
      return OGMRIP_FORMAT_MP2;

    return OGMRIP_FORMAT_UNDEFINED;
  }

  if (g_str_equal (str, "FLAC"))
    return OGMRIP_FORMAT_FLAC;

  return OGMRIP_FORMAT_UNDEFINED;
}

G_DEFINE_TYPE_WITH_CODE (OGMRipAudioFile, ogmrip_audio_file, OGMRIP_TYPE_FILE,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_AUDIO_STREAM, ogmrip_audio_iface_init));

static void
ogmrip_audio_file_init (OGMRipAudioFile *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream, OGMRIP_TYPE_AUDIO_FILE, OGMRipAudioFilePriv);

  stream->priv->bitrate = -1;
  stream->priv->channels = OGMRIP_CHANNELS_UNDEFINED;
}

static void
ogmrip_audio_file_class_init (OGMRipAudioFileClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructor = ogmrip_audio_file_constructor;

  g_type_class_add_private (klass, sizeof (OGMRipAudioFilePriv));
}

static GObject *
ogmrip_audio_file_constructor (GType type, guint n_properties, GObjectConstructParam *properties)
{
  GObject *gobject;
  OGMRipMediaInfo *info;

  gobject = G_OBJECT_CLASS (ogmrip_audio_file_parent_class)->constructor (type, n_properties, properties);

  info = ogmrip_media_info_get_default ();
  if  (!info || !OGMRIP_FILE (gobject)->priv->path)
  {
    g_object_unref (gobject);
    return NULL;
  }

  if (g_file_test (OGMRIP_FILE (gobject)->priv->path, G_FILE_TEST_EXISTS) &&
      ogmrip_media_info_open (info, OGMRIP_FILE (gobject)->priv->path))
  {
    const gchar *str;

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_GENERAL, 0, "AudioCount");
    if (!str || !g_str_equal (str, "1"))
    {
      g_object_unref (info);
      g_object_unref (gobject);
      return NULL;
    }

    OGMRIP_FILE (gobject)->priv->format = ogmrip_media_info_get_audio_format (info);
    if (OGMRIP_FILE (gobject)->priv->format < 0)
    {
      g_object_unref (info);
      g_object_unref (gobject);
      return NULL;
    }

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, 0, "Duration");
    OGMRIP_FILE (gobject)->priv->length = str ? atoi (str) / 1000. : -1.0;

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, 0, "StreamSize");
    OGMRIP_FILE (gobject)->priv->size = str ? atoll (str) : -1;

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_GENERAL, 0, "Track");
    OGMRIP_FILE (gobject)->priv->label = str ? g_strdup (str) : NULL;

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, 0, "BitRate");
    OGMRIP_AUDIO_FILE (gobject)->priv->bitrate = str ? atoi (str) : -1;

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, 0, "Channel(s)");
    OGMRIP_AUDIO_FILE (gobject)->priv->channels = str ? atoi (str) : -1;

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, 0, "SamplingRate");
    OGMRIP_AUDIO_FILE (gobject)->priv->samplerate = str ? atoi (str) : -1;

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_AUDIO, 0, "Language/String2");
    OGMRIP_AUDIO_FILE (gobject)->priv->language = str ? (str[0] << 8) | str[1] : 0;

    ogmrip_media_info_close (info);
  }

  return gobject;
}

static gint
ogmrip_audio_file_get_bitrate (OGMRipAudioStream *audio)
{
  return OGMRIP_AUDIO_FILE (audio)->priv->bitrate;
}

static gint
ogmrip_audio_file_get_channels (OGMRipAudioStream *audio)
{
  return OGMRIP_AUDIO_FILE (audio)->priv->channels;
}

static const gchar *
ogmrip_audio_file_get_label (OGMRipAudioStream *audio)
{
  return OGMRIP_FILE (audio)->priv->label;
}

static gint
ogmrip_audio_file_get_language (OGMRipAudioStream *audio)
{
  return OGMRIP_AUDIO_FILE (audio)->priv->language;;
}

static gint
ogmrip_audio_file_get_sample_rate (OGMRipAudioStream *audio)
{
  return OGMRIP_AUDIO_FILE (audio)->priv->samplerate;;
}

static void
ogmrip_audio_iface_init (OGMRipAudioStreamInterface *iface)
{
  iface->get_bitrate      = ogmrip_audio_file_get_bitrate;
  iface->get_channels     = ogmrip_audio_file_get_channels;
  iface->get_label        = ogmrip_audio_file_get_label;
  iface->get_language     = ogmrip_audio_file_get_language;
  iface->get_sample_rate  = ogmrip_audio_file_get_sample_rate;
}

OGMRipMedia *
ogmrip_audio_file_new (const gchar *uri)
{
  g_return_val_if_fail (uri != NULL, NULL);

  return g_object_new (OGMRIP_TYPE_AUDIO_FILE, "uri", uri, NULL);
}

void
ogmrip_audio_file_set_language (OGMRipAudioFile *file, guint language)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_FILE (file));

  file->priv->language = language;
}

