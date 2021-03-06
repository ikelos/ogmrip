/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-encoding-parser.h"
#include "ogmrip-profile-engine.h"
#include "ogmrip-type.h"

static void
ogmrip_encoding_parse_property (OGMRipXML *xml, gpointer gobject, gpointer klass)
{
  gchar *property;

  property = ogmrip_xml_get_string (xml, "name");
  if (property)
  {
    if (g_str_equal (property, "profile") && OGMRIP_IS_ENCODING (gobject))
    {
      OGMRipProfileEngine *engine;
      OGMRipProfile *profile;
      gchar *str;

      engine = ogmrip_profile_engine_get_default ();

      str = ogmrip_xml_get_string (xml, NULL);
      profile = ogmrip_profile_engine_get (engine, str);
      g_free (str);

      if (profile)
      {
        ogmrip_encoding_set_profile (gobject, profile);
        g_object_unref (profile);
      }
    }
    else if (g_str_equal (property, "log-file") && OGMRIP_IS_ENCODING (gobject))
    {
      gchar *utf8, *filename;

      utf8 = ogmrip_xml_get_string (xml, NULL);
      filename = utf8 ? g_filename_from_utf8 (utf8, -1, NULL, NULL, NULL) : NULL;
      g_free (utf8);

      ogmrip_encoding_set_log_file (gobject, filename);
      g_free (filename);
    }
    else if (g_str_equal (property, "output") && OGMRIP_IS_CONTAINER (gobject))
    {
      GFile *file;
      gchar *filename;

      filename = ogmrip_xml_get_string (xml, NULL);
      file = g_file_parse_name (filename);
      g_free (filename);

      ogmrip_container_set_output (gobject, file);
      g_object_unref (file);
    }
    else if (g_str_equal (property, "hardsub") && OGMRIP_IS_VIDEO_CODEC (gobject))
    {
      OGMRipTitle *title;
      OGMRipSubpStream *subp;

      title = ogmrip_stream_get_title (ogmrip_codec_get_input (gobject));

      subp = ogmrip_title_get_subp_stream (title, ogmrip_xml_get_uint (xml, NULL));
      if (subp)
        ogmrip_video_codec_set_hard_subp (gobject, subp, ogmrip_xml_get_boolean (xml, "forced"));
    }
    else
    {
      GParamSpec *pspec;

      pspec = g_object_class_find_property (klass, property);
      if (pspec)
      {
        GValue value = { 0 };

        g_value_init (&value, pspec->value_type);

        ogmrip_xml_get_value (xml, NULL, &value);
        g_object_set_property (gobject, property, &value);

        g_value_unset (&value);
      }
    }

    g_free (property);
  }
}

static void
ogmrip_encoding_parse_properties (OGMRipXML *xml, gpointer gobject, gpointer klass)
{
  if (ogmrip_xml_children (xml))
  {
    do
    {
      if (g_str_equal (ogmrip_xml_get_name (xml), "property"))
        ogmrip_encoding_parse_property (xml, gobject, klass);
    }
    while (ogmrip_xml_next (xml));

    ogmrip_xml_parent (xml);
  }
}

static gboolean
ogmrip_encoding_parse_container (OGMRipEncoding *encoding, OGMRipXML *xml, GError **error)
{
  gboolean retval = FALSE;
  gchar *name;

  name = ogmrip_xml_get_string (xml, "type");
  if (name)
  {
    GType type;

    type = ogmrip_type_from_name (name);
    if (type != G_TYPE_NONE && g_type_is_a (type, OGMRIP_TYPE_CONTAINER))
    {
      OGMRipContainer *container;
      OGMRipContainerClass *klass;

      container = g_object_new (type, NULL);
      retval = ogmrip_encoding_set_container (encoding, container, error);
      g_object_unref (container);

      if (retval)
      {
        klass = OGMRIP_CONTAINER_GET_CLASS (container);
        ogmrip_encoding_parse_properties (xml, container, klass);
      }
    }
    
    g_free (name);
  }

  return retval;
}

static gboolean
ogmrip_encoding_parse_video_codec (OGMRipEncoding *encoding, OGMRipXML *xml, GError **error)
{
  gboolean retval = FALSE;
  gchar *name;

  name = ogmrip_xml_get_string (xml, "type");
  if (name)
  {
    GType type;

    type = ogmrip_type_from_name (name);
    if (type != G_TYPE_NONE && g_type_is_a (type, OGMRIP_TYPE_VIDEO_CODEC))
    {
      OGMRipTitle *title;
      OGMRipVideoCodec *codec;
      OGMRipVideoCodecClass *klass;

      title = ogmrip_encoding_get_title (encoding);

      codec = g_object_new (type, "input", ogmrip_title_get_video_stream (title), NULL);
      retval = ogmrip_encoding_set_video_codec (encoding, OGMRIP_VIDEO_CODEC (codec), error);
      g_object_unref (codec);

      if (retval)
      {
        klass = OGMRIP_VIDEO_CODEC_GET_CLASS (codec);
        ogmrip_encoding_parse_properties (xml, codec, klass);
      }
    }
    
    g_free (name);
  }

  return retval;
}

static gboolean
ogmrip_encoding_parse_audio_codec (OGMRipEncoding *encoding, OGMRipXML *xml, GError **error)
{
  gboolean retval = FALSE;
  gchar *name;

  name = ogmrip_xml_get_string (xml, "type");
  if (name)
  {
    GType type;

    type = ogmrip_type_from_name (name);
    if (type != G_TYPE_NONE && g_type_is_a (type, OGMRIP_TYPE_AUDIO_CODEC))
    {
      OGMRipAudioStream *stream;
      guint id;

      id = ogmrip_xml_get_uint (xml, "stream");

      stream = ogmrip_title_get_audio_stream (ogmrip_encoding_get_title (encoding), id);
      if (stream)
      {
        OGMRipAudioCodec *codec;
        OGMRipAudioCodecClass *klass;


        codec = g_object_new (type, "input", stream, NULL);
        retval = ogmrip_encoding_add_audio_codec (encoding, OGMRIP_AUDIO_CODEC (codec), error);
        g_object_unref (codec);

        if (retval)
        {
          klass = OGMRIP_AUDIO_CODEC_GET_CLASS (codec);
          ogmrip_encoding_parse_properties (xml, codec, klass);
        }
      }
    }
    
    g_free (name);
  }

  return retval;
}

static gboolean
ogmrip_encoding_parse_audio_codecs (OGMRipEncoding *encoding, OGMRipXML *xml, GError **error)
{
  if (ogmrip_xml_children (xml))
  {
    do
    {
      if (g_str_equal (ogmrip_xml_get_name (xml), "audio-codec") &&
          !ogmrip_encoding_parse_audio_codec (encoding, xml, error))
        return FALSE;
    }
    while (ogmrip_xml_next (xml));

    ogmrip_xml_parent (xml);
  }

  return TRUE;
}

static gboolean
ogmrip_encoding_parse_subp_codec (OGMRipEncoding *encoding, OGMRipXML *xml, GError **error)
{
  gboolean retval = FALSE;
  gchar *name;

  name = ogmrip_xml_get_string (xml, "type");
  if (name)
  {
    GType type;

    type = ogmrip_type_from_name (name);
    if (type != G_TYPE_NONE && g_type_is_a (type, OGMRIP_TYPE_SUBP_CODEC))
    {
      OGMRipSubpStream *stream;
      guint id;

      id = ogmrip_xml_get_uint (xml, "stream");

      stream = ogmrip_title_get_subp_stream (ogmrip_encoding_get_title (encoding), id);
      if (stream)
      {
        OGMRipSubpCodec *codec;
        OGMRipSubpCodecClass *klass;

        codec = g_object_new (type, "input", stream, NULL);
        retval = ogmrip_encoding_add_subp_codec (encoding, OGMRIP_SUBP_CODEC (codec), error);
        g_object_unref (codec);

        if (retval)
        {
          klass = OGMRIP_SUBP_CODEC_GET_CLASS (codec);
          ogmrip_encoding_parse_properties (xml, codec, klass);
        }
      }
    }
    
    g_free (name);
  }

  return retval;
}

static gboolean
ogmrip_encoding_parse_subp_codecs (OGMRipEncoding *encoding, OGMRipXML *xml, GError **error)
{
  if (ogmrip_xml_children (xml))
  {
    do
    {
      if (g_str_equal (ogmrip_xml_get_name (xml), "subp-codec") &&
          !ogmrip_encoding_parse_subp_codec (encoding, xml, error))
        return FALSE;
    }
    while (ogmrip_xml_next (xml));

    ogmrip_xml_parent (xml);
  }

  return TRUE;
}

static void
ogmrip_encoding_parse_chapter (OGMRipEncoding *encoding, OGMRipXML *xml)
{
  OGMRipCodec *chapters;
  OGMRipChaptersClass *klass;

  chapters = ogmrip_chapters_new (ogmrip_title_get_video_stream (ogmrip_encoding_get_title (encoding)));
  ogmrip_encoding_add_chapters (encoding, OGMRIP_CHAPTERS (chapters));
  g_object_unref (chapters);

  klass = OGMRIP_CHAPTERS_GET_CLASS (chapters);

  if (ogmrip_xml_children (xml))
  {
    guint number;
    gchar *str;

    do
    {
      if (g_str_equal (ogmrip_xml_get_name (xml), "property"))
        ogmrip_encoding_parse_property (xml, chapters, klass);
      else if (g_str_equal (ogmrip_xml_get_name (xml), "label"))
      {
        str = ogmrip_xml_get_string (xml, NULL);
        if (str)
        {
          number = ogmrip_xml_get_uint (xml, "number");
          ogmrip_chapters_set_label (OGMRIP_CHAPTERS (chapters), number, str);
          g_free (str);
        }
      }
    }
    while (ogmrip_xml_next (xml));

    ogmrip_xml_parent (xml);
  }
}

static void
ogmrip_encoding_parse_chapters (OGMRipEncoding *encoding, OGMRipXML *xml)
{
  if (ogmrip_xml_children (xml))
  {
    do
    {
      if (g_str_equal (ogmrip_xml_get_name (xml), "chapter"))
        ogmrip_encoding_parse_chapter (encoding, xml);
    }
    while (ogmrip_xml_next (xml));

    ogmrip_xml_parent (xml);
  }
}

static gboolean
ogmrip_encoding_parse_files (OGMRipEncoding *encoding, OGMRipXML *xml, GError **error)
{
  if (ogmrip_xml_children (xml))
  {
    gchar *utf8, *type, *uri;

    do
    {
      if (g_str_equal (ogmrip_xml_get_name (xml), "file"))
      {
        utf8 = ogmrip_xml_get_string (xml, NULL);
        uri = utf8 ? g_filename_from_utf8 (utf8, -1, NULL, NULL, NULL) : NULL;
        g_free (utf8);

        if (uri)
        {
          OGMRipMedia *file = NULL;
          gboolean retval = FALSE;

          type = ogmrip_xml_get_string (xml, "type");

          if (type && g_str_equal (type, "audio"))
            file = ogmrip_audio_file_new (uri, error);
          else if (type && g_str_equal (type, "subp"))
            file = ogmrip_subp_file_new (uri, error);

          if (file)
          {
            retval = ogmrip_encoding_add_file (encoding, OGMRIP_FILE (file), error);
            g_object_unref (file);
          }

          g_free (type);
          g_free (uri);

          if (!retval)
            return FALSE;
        }
      }
    }
    while (ogmrip_xml_next (xml));

    ogmrip_xml_parent (xml);
  }

  return TRUE;
}

gboolean
ogmrip_encoding_parse (OGMRipEncoding *encoding, OGMRipXML *xml, GError **error)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);
  g_return_val_if_fail (xml != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (ogmrip_xml_children (xml))
  {
    gboolean retval = TRUE;

    OGMRipEncodingClass *klass;
    const gchar *name;

    klass = OGMRIP_ENCODING_GET_CLASS (encoding);

    do
    {
      name = ogmrip_xml_get_name (xml);

      if (g_str_equal (name, "property"))
        ogmrip_encoding_parse_property (xml, encoding, klass);
      else if (g_str_equal (name, "container"))
        retval = ogmrip_encoding_parse_container (encoding, xml, error);
      else if (g_str_equal (name, "video-codec"))
        retval = ogmrip_encoding_parse_video_codec (encoding, xml, error);
      else if (g_str_equal (name, "audio-codecs"))
        retval = ogmrip_encoding_parse_audio_codecs (encoding, xml, error);
      else if (g_str_equal (name, "subp-codecs"))
        retval = ogmrip_encoding_parse_subp_codecs (encoding, xml, error);
      else if (g_str_equal (name, "chapters"))
        ogmrip_encoding_parse_chapters (encoding, xml);
      else if (g_str_equal (name, "files"))
        retval = ogmrip_encoding_parse_files (encoding, xml, error);

      if (!retval)
        return FALSE;
    }
    while (ogmrip_xml_next (xml));

    ogmrip_xml_parent (xml);
  }

  return TRUE;
}

static void
ogmrip_encoding_dump_property (OGMRipXML *xml, gpointer gobject, gpointer klass, const gchar *property)
{
  GParamSpec *pspec;

  pspec = g_object_class_find_property (klass, property);
  if (pspec)
  {
    GValue value = { 0 };

    ogmrip_xml_append (xml, "property");

    g_value_init (&value, pspec->value_type);
    g_object_get_property (gobject, property, &value);

    ogmrip_xml_set_string (xml, "name", property);
    ogmrip_xml_set_value (xml, NULL, &value);

    g_value_unset (&value);

    ogmrip_xml_parent (xml);
  }
}

static void
ogmrip_encoding_dump_container (OGMRipXML *xml, OGMRipContainer *container)
{
  OGMRipContainerClass *klass;
  GFile *file;

  if (!container)
    return;

  ogmrip_xml_append (xml, "container");

  ogmrip_xml_set_string (xml, "type",
      ogmrip_type_name (G_OBJECT_TYPE (container)));

  file = ogmrip_container_get_output (container);
  if (file)
  {
    gchar *filename;

    ogmrip_xml_append (xml, "property");
    ogmrip_xml_set_string (xml, "name", "output");

    filename = g_file_get_parse_name (file);
    ogmrip_xml_set_string (xml, NULL, filename);
    g_free (filename);

    ogmrip_xml_parent (xml);
  }

  klass = OGMRIP_CONTAINER_GET_CLASS (container);

  ogmrip_encoding_dump_property (xml, container, klass, "fourcc");
  ogmrip_encoding_dump_property (xml, container, klass, "label");
  ogmrip_encoding_dump_property (xml, container, klass, "start-delay");
  ogmrip_encoding_dump_property (xml, container, klass, "target-number");
  ogmrip_encoding_dump_property (xml, container, klass, "target-size");

  ogmrip_xml_parent (xml);
}

static void
ogmrip_encoding_dump_video_codec (OGMRipXML *xml, OGMRipCodec *codec)
{
  OGMRipVideoCodecClass *klass;
  OGMRipSubpStream *subp;
  gboolean forced;

  if (!codec)
    return;

  ogmrip_xml_append (xml, "video-codec");

  ogmrip_xml_set_string (xml, "type",
      ogmrip_type_name (G_OBJECT_TYPE (codec)));

  subp = ogmrip_video_codec_get_hard_subp (OGMRIP_VIDEO_CODEC (codec), &forced);
  if (subp)
  {
    ogmrip_xml_append (xml, "property");

    ogmrip_xml_set_string (xml, "name", "hardsub");
    ogmrip_xml_set_boolean (xml, "forced", forced);

    ogmrip_xml_set_uint (xml, NULL,
        ogmrip_stream_get_id (OGMRIP_STREAM (subp)));

    ogmrip_xml_parent (xml);
  }

  klass = OGMRIP_VIDEO_CODEC_GET_CLASS (codec);

  ogmrip_encoding_dump_property (xml, codec, klass, "start-chapter");
  ogmrip_encoding_dump_property (xml, codec, klass, "end-chapter");
  ogmrip_encoding_dump_property (xml, codec, klass, "angle");
  ogmrip_encoding_dump_property (xml, codec, klass, "bitrate");
  ogmrip_encoding_dump_property (xml, codec, klass, "bpp");
  ogmrip_encoding_dump_property (xml, codec, klass, "can-crop");
  ogmrip_encoding_dump_property (xml, codec, klass, "deblock");
  ogmrip_encoding_dump_property (xml, codec, klass, "deinterlacer");
  ogmrip_encoding_dump_property (xml, codec, klass, "denoise");
  ogmrip_encoding_dump_property (xml, codec, klass, "dering");
  ogmrip_encoding_dump_property (xml, codec, klass, "passes");
  ogmrip_encoding_dump_property (xml, codec, klass, "quality");
  ogmrip_encoding_dump_property (xml, codec, klass, "quantizer");
  ogmrip_encoding_dump_property (xml, codec, klass, "scaler");
  ogmrip_encoding_dump_property (xml, codec, klass, "threads");
  ogmrip_encoding_dump_property (xml, codec, klass, "turbo");
  ogmrip_encoding_dump_property (xml, codec, klass, "min-width");
  ogmrip_encoding_dump_property (xml, codec, klass, "min-height");
  ogmrip_encoding_dump_property (xml, codec, klass, "max-width");
  ogmrip_encoding_dump_property (xml, codec, klass, "max-height");
  ogmrip_encoding_dump_property (xml, codec, klass, "expand");
  ogmrip_encoding_dump_property (xml, codec, klass, "crop-x");
  ogmrip_encoding_dump_property (xml, codec, klass, "crop-y");
  ogmrip_encoding_dump_property (xml, codec, klass, "crop-width");
  ogmrip_encoding_dump_property (xml, codec, klass, "crop-height");
  ogmrip_encoding_dump_property (xml, codec, klass, "scale-width");
  ogmrip_encoding_dump_property (xml, codec, klass, "scale-height");

  ogmrip_xml_parent (xml);
}

static void
ogmrip_encoding_dump_audio_codec (OGMRipXML *xml, OGMRipCodec *codec)
{
  OGMRipAudioCodecClass *klass;
  OGMRipStream *stream;

  if (!codec)
    return;

  ogmrip_xml_append (xml, "audio-codec");

  ogmrip_xml_set_string (xml, "type",
      ogmrip_type_name (G_OBJECT_TYPE (codec)));

  stream = ogmrip_codec_get_input (codec);
  ogmrip_xml_set_uint (xml, "stream",
      ogmrip_stream_get_id (stream));

  klass = OGMRIP_AUDIO_CODEC_GET_CLASS (codec);

  ogmrip_encoding_dump_property (xml, codec, klass, "start-chapter");
  ogmrip_encoding_dump_property (xml, codec, klass, "end-chapter");
  ogmrip_encoding_dump_property (xml, codec, klass, "channels");
  ogmrip_encoding_dump_property (xml, codec, klass, "fast");
  ogmrip_encoding_dump_property (xml, codec, klass, "label");
  ogmrip_encoding_dump_property (xml, codec, klass, "language");
  ogmrip_encoding_dump_property (xml, codec, klass, "normalize");
  ogmrip_encoding_dump_property (xml, codec, klass, "quality");
  ogmrip_encoding_dump_property (xml, codec, klass, "sample-rate");

  ogmrip_xml_parent (xml);
}

static void
ogmrip_encoding_dump_subp_codec (OGMRipXML *xml, OGMRipCodec *codec)
{
  OGMRipSubpCodecClass *klass;
  OGMRipStream *stream;

  if (!codec)
    return;

  ogmrip_xml_append (xml, "subp-codec");

  ogmrip_xml_set_string (xml, "type",
      ogmrip_type_name (G_OBJECT_TYPE (codec)));

  stream = ogmrip_codec_get_input (codec);
  ogmrip_xml_set_uint (xml, "stream",
      ogmrip_stream_get_id (stream));

  klass = OGMRIP_SUBP_CODEC_GET_CLASS (codec);

  ogmrip_encoding_dump_property (xml, codec, klass, "start-chapter");
  ogmrip_encoding_dump_property (xml, codec, klass, "end-chapter");
  ogmrip_encoding_dump_property (xml, codec, klass, "charset");
  ogmrip_encoding_dump_property (xml, codec, klass, "forced-only");
  ogmrip_encoding_dump_property (xml, codec, klass, "label");
  ogmrip_encoding_dump_property (xml, codec, klass, "language");
  ogmrip_encoding_dump_property (xml, codec, klass, "newline");

  ogmrip_xml_parent (xml);
}

static void
ogmrip_encoding_dump_chapters (OGMRipXML *xml, OGMRipChapters *chapters)
{
  OGMRipChaptersClass *klass;
  guint start, end, i;
  gchar *escaped;

  if (!chapters)
    return;

  ogmrip_xml_append (xml, "chapter");

  klass = OGMRIP_CHAPTERS_GET_CLASS (chapters);

  ogmrip_encoding_dump_property (xml, chapters, klass, "start-chapter");
  ogmrip_encoding_dump_property (xml, chapters, klass, "end-chapter");
  ogmrip_encoding_dump_property (xml, chapters, klass, "language");

  ogmrip_codec_get_chapters (OGMRIP_CODEC (chapters), &start, &end);
  for (i = start; i <= end; i ++)
  {
    ogmrip_xml_append (xml, "label");

    ogmrip_xml_set_uint (xml, "number", i);

    escaped = g_markup_escape_text (ogmrip_chapters_get_label (chapters, i), -1);
    ogmrip_xml_set_string (xml, NULL, escaped);
    g_free (escaped);

    ogmrip_xml_parent (xml);
  }

  ogmrip_xml_parent (xml);
}

static void
ogmrip_encoding_dump_file (OGMRipXML *xml, OGMRipFile *file)
{
  if (!file)
    return;

  if (OGMRIP_IS_AUDIO_FILE (file) || OGMRIP_IS_SUBP_FILE (file))
  {
    gchar *utf8;

    ogmrip_xml_append (xml, "file");

    if (OGMRIP_IS_AUDIO_FILE (file))
      ogmrip_xml_set_string (xml, "type", "audio");
    else
      ogmrip_xml_set_string (xml, "type", "subp");

    utf8 = g_filename_to_utf8 (ogmrip_file_get_path (file), -1, NULL, NULL, NULL);
    ogmrip_xml_set_string (xml, NULL, utf8);
    g_free (utf8);

    ogmrip_xml_parent (xml);
  }
}

gboolean
ogmrip_encoding_dump (OGMRipEncoding *encoding, OGMRipXML *xml, GError **error)
{
  OGMRipEncodingClass *klass;
  OGMRipProfile *profile;
  OGMRipTitle *title;
  GList *list, *link;
  const gchar *log;
  gchar *utf8;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);
  g_return_val_if_fail (xml != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  ogmrip_xml_append (xml, "encoding");

  title = ogmrip_encoding_get_title (encoding);

  utf8 = g_filename_to_utf8 (ogmrip_media_get_uri (ogmrip_title_get_media (title)), -1, NULL, NULL, NULL);
  ogmrip_xml_set_string (xml, "uri", utf8);
  g_free (utf8);

  ogmrip_xml_set_string (xml, "id",
      ogmrip_media_get_id (ogmrip_title_get_media (title)));
  ogmrip_xml_set_int (xml, "title",
      ogmrip_title_get_id (title));

  profile = ogmrip_encoding_get_profile (encoding);
  if (profile)
  {
    gchar *str;

    ogmrip_xml_append (xml, "property");
    ogmrip_xml_set_string (xml, "name", "profile");

    g_object_get (profile, "name", &str, NULL);
    ogmrip_xml_set_string (xml, NULL, str);
    g_free (str);

    ogmrip_xml_parent (xml);
  }

  klass = OGMRIP_ENCODING_GET_CLASS (encoding);

  log = ogmrip_encoding_get_log_file (encoding);
  if (log)
  {
    ogmrip_xml_append (xml, "property");
    ogmrip_xml_set_string (xml, "name", "log-file");

    utf8 = g_filename_to_utf8 (log, -1, NULL, NULL, NULL);
    ogmrip_xml_set_string (xml, NULL, utf8);
    g_free (utf8);

    ogmrip_xml_parent (xml);
  }

  ogmrip_encoding_dump_property (xml, encoding, klass, "autocrop");
  ogmrip_encoding_dump_property (xml, encoding, klass, "autoscale");
  ogmrip_encoding_dump_property (xml, encoding, klass, "copy");
  ogmrip_encoding_dump_property (xml, encoding, klass, "ensure-sync");
  ogmrip_encoding_dump_property (xml, encoding, klass, "method");
  ogmrip_encoding_dump_property (xml, encoding, klass, "relative");
  ogmrip_encoding_dump_property (xml, encoding, klass, "test");

  ogmrip_encoding_dump_container (xml, ogmrip_encoding_get_container (encoding));
  ogmrip_encoding_dump_video_codec (xml, ogmrip_encoding_get_video_codec (encoding));

  list = ogmrip_encoding_get_audio_codecs (encoding);
  if (list)
  {
    ogmrip_xml_append (xml, "audio-codecs");
    for (link = list; link; link = link->next)
      ogmrip_encoding_dump_audio_codec (xml, link->data);
    ogmrip_xml_parent (xml);
  }
  g_list_free (list);

  list = ogmrip_encoding_get_subp_codecs (encoding);
  if (list)
  {
    ogmrip_xml_append (xml, "subp-codecs");
    for (link = list; link; link = link->next)
      ogmrip_encoding_dump_subp_codec (xml, link->data);
    ogmrip_xml_parent (xml);
  }
  g_list_free (list);

  list = ogmrip_encoding_get_chapters (encoding);
  if (list)
  {
    ogmrip_xml_append (xml, "chapters");
    for (link = list; link; link = link->next)
      ogmrip_encoding_dump_chapters (xml, link->data);
    ogmrip_xml_parent (xml);
  }
  g_list_free (list);

  list = ogmrip_encoding_get_files (encoding);
  if (list)
  {
    ogmrip_xml_append (xml, "files");
    for (link = list; link; link = link->next)
      ogmrip_encoding_dump_file (xml, link->data);
    ogmrip_xml_parent (xml);
  }
  g_list_free (list);

  return TRUE;
}

