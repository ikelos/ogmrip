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

#include "ogmrip-media-file.h"
#include "ogmrip-media-info.h"
#include "ogmrip-file-priv.h"

#include <ogmrip-base.h>

#include <stdio.h>
#include <stdlib.h>

#define OGMRIP_TYPE_MEDIA_FILE_STREAM (ogmrip_media_file_stream_get_type ())
#define OGMRIP_MEDIA_FILE_STREAM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MEDIA_FILE_STREAM, OGMRipMediaFileStream))

typedef struct _OGMRipMediaFileStream      OGMRipMediaFileStream;
typedef struct _OGMRipMediaFileStreamClass OGMRipMediaFileStreamClass;

struct _OGMRipMediaFileStream
{
  GObject parent_instance;

  OGMRipTitle *title;
  gint format;
  gint id;
};

struct _OGMRipMediaFileStreamClass
{
  GObjectClass parent_class;
};

#define OGMRIP_TYPE_MEDIA_FILE_AUDIO (ogmrip_media_file_audio_get_type ())
#define OGMRIP_MEDIA_FILE_AUDIO(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MEDIA_FILE_AUDIO, OGMRipMediaFileAudio))

typedef struct _OGMRipMediaFileAudio      OGMRipMediaFileAudio;
typedef struct _OGMRipMediaFileAudioClass OGMRipMediaFileAudioClass;

struct _OGMRipMediaFileAudio
{
  OGMRipMediaFileStream parent_instance;

  OGMRipAudioFilePriv *priv;
};

struct _OGMRipMediaFileAudioClass
{
  OGMRipMediaFileStreamClass parent_class;
};

#define OGMRIP_TYPE_MEDIA_FILE_SUBP (ogmrip_media_file_subp_get_type ())
#define OGMRIP_MEDIA_FILE_SUBP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MEDIA_FILE_SUBP, OGMRipMediaFileSubp))

typedef struct _OGMRipMediaFileSubp      OGMRipMediaFileSubp;
typedef struct _OGMRipMediaFileSubpClass OGMRipMediaFileSubpClass;

struct _OGMRipMediaFileSubp
{
  OGMRipMediaFileStream parent_instance;

  OGMRipSubpFilePriv *priv;
};

struct _OGMRipMediaFileSubpClass
{
  OGMRipMediaFileStreamClass parent_class;
};

static void      ogmrip_title_iface_init        (OGMRipTitleInterface       *iface);
static void      ogmrip_stream_iface_init       (OGMRipStreamInterface      *iface);
static void      ogmrip_audio_stream_iface_init (OGMRipAudioStreamInterface *iface);
static void      ogmrip_subp_stream_iface_init  (OGMRipSubpStreamInterface  *iface);
static GObject * ogmrip_media_file_constructor  (GType type,
                                                 guint n_properties,
                                                 GObjectConstructParam *properties);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (OGMRipMediaFileStream, ogmrip_media_file_stream, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_STREAM, ogmrip_stream_iface_init));

static void
ogmrip_media_file_stream_init (OGMRipMediaFileStream *stream)
{
}

static void
ogmrip_media_file_stream_class_init (OGMRipMediaFileStreamClass *klass)
{
}

static gint
ogmrip_media_file_get_id (OGMRipStream *stream)
{
  return OGMRIP_MEDIA_FILE_STREAM (stream)->id;
}

static gint
ogmrip_media_file_get_format (OGMRipStream *stream)
{
  return OGMRIP_MEDIA_FILE_STREAM (stream)->format;
}

OGMRipTitle *
ogmrip_media_file_get_title (OGMRipStream *stream)
{
  return OGMRIP_MEDIA_FILE_STREAM (stream)->title;
}

static void
ogmrip_stream_iface_init (OGMRipStreamInterface *iface)
{
  iface->get_id     = ogmrip_media_file_get_id;
  iface->get_format = ogmrip_media_file_get_format;
  iface->get_title  = ogmrip_media_file_get_title;
}

G_DEFINE_TYPE_WITH_CODE (OGMRipMediaFileAudio, ogmrip_media_file_audio, OGMRIP_TYPE_MEDIA_FILE_STREAM,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_AUDIO_STREAM, ogmrip_audio_stream_iface_init));

static void
ogmrip_media_file_audio_init (OGMRipMediaFileAudio *audio)
{
  audio->priv = G_TYPE_INSTANCE_GET_PRIVATE (audio, OGMRIP_TYPE_MEDIA_FILE_AUDIO, OGMRipAudioFilePriv);
}

static void
ogmrip_media_file_audio_class_init (OGMRipMediaFileAudioClass *klass)
{
  g_type_class_add_private (klass, sizeof (OGMRipAudioFilePriv));
}

static gint
ogmrip_media_file_audio_get_nr (OGMRipAudioStream *audio)
{
  return OGMRIP_MEDIA_FILE_STREAM (audio)->id;
}

static gint
ogmrip_media_file_audio_get_bitrate (OGMRipAudioStream *audio)
{
  return OGMRIP_MEDIA_FILE_AUDIO (audio)->priv->bitrate;
}

static gint
ogmrip_media_file_audio_get_channels (OGMRipAudioStream *audio)
{
  return OGMRIP_MEDIA_FILE_AUDIO (audio)->priv->channels;
}

static const gchar *
ogmrip_media_file_audio_get_label (OGMRipAudioStream *audio)
{
  return OGMRIP_MEDIA_FILE_AUDIO (audio)->priv->label;
}

static gint
ogmrip_media_file_audio_get_language (OGMRipAudioStream *audio)
{
  return OGMRIP_MEDIA_FILE_AUDIO (audio)->priv->language;;
}

static gint
ogmrip_media_file_audio_get_sample_rate (OGMRipAudioStream *audio)
{
  return OGMRIP_MEDIA_FILE_AUDIO (audio)->priv->samplerate;;
}

static void
ogmrip_audio_stream_iface_init (OGMRipAudioStreamInterface *iface)
{
  iface->get_nr           = ogmrip_media_file_audio_get_nr;
  iface->get_bitrate      = ogmrip_media_file_audio_get_bitrate;
  iface->get_channels     = ogmrip_media_file_audio_get_channels;
  iface->get_label        = ogmrip_media_file_audio_get_label;
  iface->get_language     = ogmrip_media_file_audio_get_language;
  iface->get_sample_rate  = ogmrip_media_file_audio_get_sample_rate;
}

G_DEFINE_TYPE_WITH_CODE (OGMRipMediaFileSubp, ogmrip_media_file_subp, OGMRIP_TYPE_MEDIA_FILE_STREAM,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_SUBP_STREAM, ogmrip_subp_stream_iface_init));

static void
ogmrip_media_file_subp_init (OGMRipMediaFileSubp *subp)
{
  subp->priv = G_TYPE_INSTANCE_GET_PRIVATE (subp, OGMRIP_TYPE_MEDIA_FILE_SUBP, OGMRipSubpFilePriv);
}

static void
ogmrip_media_file_subp_class_init (OGMRipMediaFileSubpClass *klass)
{
  g_type_class_add_private (klass, sizeof (OGMRipSubpFilePriv));
}

static gint
ogmrip_media_file_subp_get_nr (OGMRipSubpStream *subp)
{
  return OGMRIP_MEDIA_FILE_STREAM (subp)->id;
}

static const gchar *
ogmrip_media_file_subp_get_label (OGMRipSubpStream *subp)
{
  return OGMRIP_MEDIA_FILE_SUBP (subp)->priv->label;;
}

static gint
ogmrip_media_file_subp_get_language (OGMRipSubpStream *subp)
{
  return OGMRIP_MEDIA_FILE_SUBP (subp)->priv->language;
}

static void
ogmrip_subp_stream_iface_init (OGMRipSubpStreamInterface *iface)
{
  iface->get_nr       = ogmrip_media_file_subp_get_nr;
  iface->get_label    = ogmrip_media_file_subp_get_label;
  iface->get_language = ogmrip_media_file_subp_get_language;
}

G_DEFINE_TYPE_WITH_CODE (OGMRipMediaFile, ogmrip_media_file, OGMRIP_TYPE_VIDEO_FILE,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_TITLE, ogmrip_title_iface_init));

static void
ogmrip_media_file_init (OGMRipMediaFile *media)
{
  media->priv = G_TYPE_INSTANCE_GET_PRIVATE (media, OGMRIP_TYPE_MEDIA_FILE, OGMRipMediaFilePriv);
}

static void
ogmrip_media_file_class_init (OGMRipMediaFileClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructor = ogmrip_media_file_constructor;

  g_type_class_add_private (klass, sizeof (OGMRipMediaFilePriv));
}

static GObject *
ogmrip_media_file_constructor (GType type, guint n_properties, GObjectConstructParam *properties)
{
  GFile *file;
  GObject *gobject;
  OGMRipMediaInfo *info;
  OGMRipMediaFile *media;
  OGMRipMediaFileStream *stream;
  const gchar *str;
  gint i, n;

  gobject = G_OBJECT_CLASS (ogmrip_media_file_parent_class)->constructor (type, n_properties, properties);
  if (!gobject)
    return NULL;

  info = ogmrip_media_info_get_default ();
  if (!info || !OGMRIP_FILE (gobject)->priv->path)
  {
    g_object_unref (gobject);
    return NULL;
  }

  file = g_file_new_for_path (OGMRIP_FILE (gobject)->priv->path);
  OGMRIP_FILE (gobject)->priv->id = g_file_get_id (file);
  g_object_unref (file);

  if (!OGMRIP_FILE (gobject)->priv->id)
  {
    g_object_unref (gobject);
    return NULL;
  }

  if (!ogmrip_media_info_open (info, OGMRIP_FILE (gobject)->priv->path))
  {
    g_object_unref (gobject);
    return NULL;
  }

  media = OGMRIP_MEDIA_FILE (gobject);

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_GENERAL, 0, "AudioCount");

  n = str ? atoi (str) : 0;
  for (i = 0; i < n; i ++)
  {
    stream = g_object_new (OGMRIP_TYPE_MEDIA_FILE_AUDIO, 0);
    stream->title = OGMRIP_TITLE (gobject);
    stream->format = ogmrip_media_info_get_audio_format (info, i);
    stream->id = i;

    ogmrip_media_info_get_audio_info (info, i, OGMRIP_MEDIA_FILE_AUDIO (stream)->priv);
    OGMRIP_FILE (gobject)->priv->title_size += OGMRIP_MEDIA_FILE_AUDIO (stream)->priv->size;

    media->priv->audio_streams = g_list_append (media->priv->audio_streams, stream);
  }

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_GENERAL, 0, "TextCount");

  n = str ? atoi (str) : 0;
  for (i = 0; i < n; i ++)
  {
    stream = g_object_new (OGMRIP_TYPE_MEDIA_FILE_SUBP, 0);
    stream->title = OGMRIP_TITLE (gobject);
    stream->format = ogmrip_media_info_get_subp_format (info, i);
    stream->id = i;

    ogmrip_media_info_get_subp_info (info, i, OGMRIP_MEDIA_FILE_SUBP (stream)->priv);
    OGMRIP_FILE (gobject)->priv->title_size += OGMRIP_MEDIA_FILE_SUBP (stream)->priv->size;

    media->priv->subp_streams = g_list_append (media->priv->subp_streams, stream);
  }

  ogmrip_media_info_close (info);

  return gobject;
}

static gint
ogmrip_media_file_get_n_audio_streams (OGMRipTitle *title)
{
  return g_list_length (OGMRIP_MEDIA_FILE (title)->priv->audio_streams);
}

static OGMRipAudioStream *
ogmrip_media_file_get_nth_audio_stream (OGMRipTitle *title, guint nr)
{
  return g_list_nth_data (OGMRIP_MEDIA_FILE (title)->priv->audio_streams, nr);
}

static gint
ogmrip_media_file_get_n_subp_streams (OGMRipTitle *title)
{
  return g_list_length (OGMRIP_MEDIA_FILE (title)->priv->subp_streams);
}

static OGMRipSubpStream *
ogmrip_media_file_get_nth_subp_stream (OGMRipTitle *title, guint nr)
{
  return g_list_nth_data (OGMRIP_MEDIA_FILE (title)->priv->subp_streams, nr);
}

static void
ogmrip_title_iface_init (OGMRipTitleInterface *iface)
{
  iface->get_n_audio_streams = ogmrip_media_file_get_n_audio_streams;
  iface->get_nth_audio_stream = ogmrip_media_file_get_nth_audio_stream;
  iface->get_n_subp_streams = ogmrip_media_file_get_n_subp_streams;
  iface->get_nth_subp_stream = ogmrip_media_file_get_nth_subp_stream;
}

OGMRipMedia *
ogmrip_media_file_new (const gchar *uri)
{
  g_return_val_if_fail (uri != NULL, NULL);

  return g_object_new (OGMRIP_TYPE_MEDIA_FILE, "uri", uri, NULL);
}

void
ogmrip_file_register_media (void)
{
  OGMRipTypeInfo *info;

  info = g_object_new (OGMRIP_TYPE_TYPE_INFO, "name", "File", "description", "File", NULL);
  ogmrip_type_register (OGMRIP_TYPE_MEDIA_FILE, info);
}

