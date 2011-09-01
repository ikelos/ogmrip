/* OGMRip - A wrapper library around libdvdread
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-file-object.h"
#include "ogmrip-file-priv.h"

enum
{
  PROP_0,
  PROP_FORMAT,
  PROP_LABEL,
  PROP_LENGTH,
  PROP_SIZE,
  PROP_URI
};

static void ogmrip_media_iface_init  (OGMRipMediaInterface  *iface);
static void ogmrip_title_iface_init  (OGMRipTitleInterface  *iface);
static void ogmrip_stream_iface_init (OGMRipStreamInterface *iface);
static void ogmrip_file_finalize     (GObject               *gobject);
static void ogmrip_file_get_property (GObject               *gobject,
                                      guint                 property_id,
                                      GValue                *value,
                                      GParamSpec            *pspec);
static void ogmrip_file_set_property (GObject               *gobject,
                                      guint                 property_id,
                                      const GValue          *value,
                                      GParamSpec            *pspec);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (OGMRipFile, ogmrip_file, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_MEDIA, ogmrip_media_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_TITLE, ogmrip_title_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_STREAM, ogmrip_stream_iface_init));

static void
ogmrip_file_init (OGMRipFile *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream, OGMRIP_TYPE_FILE, OGMRipFilePriv);
}

static void
ogmrip_file_class_init (OGMRipFileClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = ogmrip_file_finalize;
  gobject_class->get_property = ogmrip_file_get_property;
  gobject_class->set_property = ogmrip_file_set_property;

  g_object_class_override_property (gobject_class, PROP_URI, "uri");

  g_object_class_install_property (gobject_class, PROP_FORMAT,
      g_param_spec_int ("format", "Format property", "Set format",
        OGMRIP_FORMAT_UNDEFINED, OGMRIP_FORMAT_LAST - 1, OGMRIP_FORMAT_UNDEFINED,
        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LABEL,
      g_param_spec_string ("label", "Label property", "Set label",
        NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LENGTH,
      g_param_spec_double ("length", "Length property", "Set length",
        -1.0, G_MAXDOUBLE, -1.0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SIZE,
      g_param_spec_int64 ("size", "Size property", "Set size",
        -1, G_MAXINT64, -1, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipFilePriv));
}

static void
ogmrip_file_finalize (GObject *gobject)
{
  OGMRipFile *file = OGMRIP_FILE (gobject);

  g_free (file->priv->uri);
  g_free (file->priv->path);
  g_free (file->priv->label);

  G_OBJECT_CLASS (ogmrip_file_parent_class)->finalize (gobject);
}

static void
ogmrip_file_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipFile *file = OGMRIP_FILE (gobject);

  switch (property_id)
  {
    case PROP_FORMAT:
      g_value_set_int (value, file->priv->format);
      break;
    case PROP_LABEL:
      g_value_set_static_string (value, file->priv->label);
      break;
    case PROP_LENGTH:
      g_value_set_double (value, file->priv->length);
      break;
    case PROP_SIZE:
      g_value_set_int64 (value, file->priv->size);
      break;
    case PROP_URI:
      g_value_set_string (value, file->priv->uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_file_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipFile *file = OGMRIP_FILE (gobject);

  switch (property_id)
  {
    case PROP_URI:
      file->priv->uri = g_value_dup_string (value);
      file->priv->path = g_filename_from_uri (file->priv->uri, NULL, NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static const gchar *
ogmrip_file_get_media_label (OGMRipMedia *media)
{
  gchar *label;

  g_object_get (media, "label", &label, NULL);

  return label;
}

static const gchar *
ogmrip_file_get_uri (OGMRipMedia *media)
{
  return OGMRIP_FILE (media)->priv->uri;
}

static gint64
ogmrip_file_get_media_size (OGMRipMedia *media)
{
  gint64 size;

  g_object_get (media, "size", &size, NULL);

  return size;
}

static gint
ogmrip_file_get_n_titles (OGMRipMedia *media)
{
  return 1;
}

static OGMRipTitle *
ogmrip_file_get_nth_title (OGMRipMedia *media, guint nr)
{
  return OGMRIP_TITLE (media);
}

static void
ogmrip_media_iface_init (OGMRipMediaInterface *iface)
{
  iface->get_label = ogmrip_file_get_media_label;
  iface->get_uri = ogmrip_file_get_uri;
  iface->get_size = ogmrip_file_get_media_size;
  iface->get_n_titles = ogmrip_file_get_n_titles;
  iface->get_nth_title = ogmrip_file_get_nth_title;
/*
  iface->get_n_audio_streams = ogmrip_audio_file_get_n_audio_streams;
  iface->get_nth_audio_stream = ogmrip_audio_file_get_nth_audio_stream;
  iface->get_n_subp_streams = ogmrip_subp_file_get_n_subp_streams;
  iface->get_nth_subp_stream = ogmrip_subp_file_get_nth_subp_stream;
  iface->get_video_stream = ogmrip_video_file_get_video_stream;
*/
}

static OGMRipMedia *
ogmrip_file_get_media (OGMRipTitle *title)
{
  return OGMRIP_MEDIA (title);
}

static gint64
ogmrip_file_get_title_size (OGMRipTitle *title)
{
  gint64 size;

  g_object_get (title, "size", &size, NULL);

  return size;
}

static gdouble
ogmrip_file_get_length (OGMRipTitle *title, OGMRipTime *length)
{
  gdouble duration;

  g_object_get (title, "length", &duration, NULL);

  if (length)
    ogmrip_msec_to_time (duration, length);

  return duration;
}

static void
ogmrip_title_iface_init (OGMRipTitleInterface *iface)
{
  iface->get_media = ogmrip_file_get_media;
  iface->get_size = ogmrip_file_get_title_size;
  iface->get_length = ogmrip_file_get_length;
}

static gint
ogmrip_file_get_format (OGMRipStream *stream)
{
  return OGMRIP_FILE (stream)->priv->format;
}

OGMRipTitle *
ogmrip_file_get_title (OGMRipStream *stream)
{
  return OGMRIP_TITLE (stream);
}

static void
ogmrip_stream_iface_init (OGMRipStreamInterface *iface)
{
  iface->get_format = ogmrip_file_get_format;
  iface->get_title  = ogmrip_file_get_title;
}

const gchar *
ogmrip_file_get_path (OGMRipFile *file)
{
  g_return_val_if_fail (OGMRIP_IS_FILE (file), NULL);

  return file->priv->path;
}

