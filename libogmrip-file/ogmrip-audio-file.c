/* OGMRipFile - A file library for OGMRip
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

#include "ogmrip-audio-file.h"
#include "ogmrip-media-info.h"
#include "ogmrip-file-priv.h"

#include <stdlib.h>

static void g_initable_iface_init          (GInitableIface             *iface);
static void ogmrip_audio_stream_iface_init (OGMRipAudioStreamInterface *iface);

G_DEFINE_TYPE_WITH_CODE (OGMRipAudioFile, ogmrip_audio_file, OGMRIP_TYPE_FILE,
    G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, g_initable_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_AUDIO_STREAM, ogmrip_audio_stream_iface_init));

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
  g_type_class_add_private (klass, sizeof (OGMRipAudioFilePriv));
}

static gboolean
ogmrip_audio_file_initable_init (GInitable *initable, GCancellable *cancellable, GError **error)
{
  GFile *file;
  OGMRipMediaInfo *info;
  const gchar *str;

  info = ogmrip_media_info_get_default ();
  if  (!info || !OGMRIP_FILE (initable)->priv->path)
    return FALSE;

  file = g_file_new_for_path (OGMRIP_FILE (initable)->priv->path);
  OGMRIP_FILE (initable)->priv->id = g_file_get_id (file);
  g_object_unref (file);

  if (!OGMRIP_FILE (initable)->priv->id)
    return FALSE;

  if (!ogmrip_media_info_open (info, OGMRIP_FILE (initable)->priv->path))
    return FALSE;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_GENERAL, 0, "AudioCount");
  if (!str || !g_str_equal (str, "1"))
  {
    g_object_unref (info);
    return FALSE;
  }

  OGMRIP_FILE (initable)->priv->format = ogmrip_media_info_get_audio_format (info, 0);
  if (OGMRIP_FILE (initable)->priv->format < 0)
  {
    g_object_unref (info);
    return FALSE;
  }

  ogmrip_media_info_get_file_info (info, OGMRIP_FILE (initable)->priv);
  ogmrip_media_info_get_audio_info (info, 0, OGMRIP_AUDIO_FILE (initable)->priv);

  OGMRIP_FILE (initable)->priv->title_size = OGMRIP_AUDIO_FILE (initable)->priv->size;

  ogmrip_media_info_close (info);

  return TRUE;
}

static void
g_initable_iface_init (GInitableIface *iface)
{
  iface->init = ogmrip_audio_file_initable_init;
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
  return OGMRIP_AUDIO_FILE (audio)->priv->label;
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
ogmrip_audio_stream_iface_init (OGMRipAudioStreamInterface *iface)
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

  return g_initable_new (OGMRIP_TYPE_AUDIO_FILE, NULL, NULL, "uri", uri, NULL);
}

void
ogmrip_audio_file_set_language (OGMRipAudioFile *file, guint language)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_FILE (file));

  file->priv->language = language;
}

