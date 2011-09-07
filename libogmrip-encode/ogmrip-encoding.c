/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * SECTION:ogmrip-encoding
 * @title: OGMRipEncoding
 * @short_description: An all-in-one component to encode DVD titles
 * @include: ogmrip-encoding.h
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-encoding.h"
#include "ogmrip-encoding-parser.h"
#include "ogmrip-profile-keys.h"
#include "ogmrip-configurable.h"
#include "ogmrip-hardsub.h"
#include "ogmrip-marshal.h"
#include "ogmrip-plugin.h"
#include "ogmrip-analyze.h"
#include "ogmrip-copy.h"
#include "ogmrip-test.h"
#include "ogmrip-fs.h"

#include <ogmrip-dvd.h>

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>

#define OGMRIP_ENCODING_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_ENCODING, OGMRipEncodingPriv))

struct _OGMRipEncodingPriv
{
  OGMRipTitle *title;

  OGMRipEncodingMethod method;
  OGMRipProfile *profile;
  gchar *log_file;

  OGMRipContainer  *container;
  OGMRipVideoCodec *video_codec;
  GList *audio_codecs;
  GList *subp_codecs;
  GList *chapters;
  GList *files;

  OGMJobSpawn *spawn;

  gboolean log_open;
  gboolean ensure_sync;
  gboolean relative;
  gboolean autocrop;
  gboolean autoscale;
  gboolean copy;
  gboolean test;
};

enum
{
  PROP_0,
  PROP_AUDIO_CODECS,
  PROP_AUTOCROP,
  PROP_AUTOSCALE,
  PROP_CONTAINER,
  PROP_COPY,
  PROP_ENSURE_SYNC,
  PROP_METHOD,
  PROP_LOG_FILE,
  PROP_PROFILE,
  PROP_RELATIVE,
  PROP_SUBP_CODECS,
  PROP_TEST,
  PROP_TITLE,
  PROP_VIDEO_CODEC
};

enum
{
  RUN,
  PROGRESS,
  COMPLETE,
  LAST_SIGNAL
};

static void   ogmrip_encoding_constructed   (GObject        *gobject);
static void   ogmrip_encoding_dispose       (GObject        *gobject);
static void   ogmrip_encoding_finalize      (GObject        *gobject);
static void   ogmrip_encoding_get_property  (GObject        *gobject,
                                             guint          property_id,
                                             GValue         *value,
                                             GParamSpec     *pspec);
static void   ogmrip_encoding_set_property  (GObject        *gobject,
                                             guint          property_id,
                                             const GValue   *value,
                                             GParamSpec     *pspec);
static void   ogmrip_encoding_run           (OGMRipEncoding *encoding,
                                             OGMJobSpawn    *spawn);
static void   ogmrip_encoding_complete      (OGMRipEncoding *encoding,
                                             OGMJobSpawn    *spawn,
                                             guint          result);

guint signals[LAST_SIGNAL] = { 0 };

static void
ogmrip_encoding_open_log (OGMRipEncoding *encoding)
{
  if (!encoding->priv->log_open)
  {
    ogmjob_log_open (encoding->priv->log_file, NULL);
    encoding->priv->log_open = TRUE;
  }
}

static void
ogmrip_encoding_close_log (OGMRipEncoding *encoding)
{
  if (encoding->priv->log_open)
  {
    ogmjob_log_close (NULL);
    encoding->priv->log_open = FALSE;
  }
}

G_DEFINE_TYPE (OGMRipEncoding, ogmrip_encoding, G_TYPE_OBJECT)

static void
ogmrip_encoding_class_init (OGMRipEncodingClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_encoding_constructed;
  gobject_class->dispose = ogmrip_encoding_dispose;
  gobject_class->finalize = ogmrip_encoding_finalize;
  gobject_class->get_property = ogmrip_encoding_get_property;
  gobject_class->set_property = ogmrip_encoding_set_property;

  klass->run = ogmrip_encoding_run;
  klass->complete = ogmrip_encoding_complete;

  signals[RUN] = g_signal_new ("run", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipEncodingClass, run), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, OGMJOB_TYPE_SPAWN);

  signals[PROGRESS] = g_signal_new ("progress", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipEncodingClass, progress), NULL, NULL,
      ogmrip_cclosure_marshal_VOID__OBJECT_DOUBLE, G_TYPE_NONE, 2, OGMJOB_TYPE_SPAWN, G_TYPE_DOUBLE);

  signals[COMPLETE] = g_signal_new ("complete", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipEncodingClass, complete), NULL, NULL,
      ogmrip_cclosure_marshal_VOID__OBJECT_UINT, G_TYPE_NONE, 2, OGMJOB_TYPE_SPAWN, G_TYPE_UINT);
/*
  g_object_class_install_property (gobject_class, PROP_AUDIO_CODECS, 
        g_param_spec_boxed ("audio-codecs", "Audio codecs property", "Set audio codecs", 
           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
*/
  g_object_class_install_property (gobject_class, PROP_AUTOCROP, 
        g_param_spec_boolean ("autocrop", "Autocrop property", "Set autocrop", 
           TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_AUTOSCALE, 
        g_param_spec_boolean ("autoscale", "Autoscale property", "Set autoscale", 
           TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CONTAINER, 
        g_param_spec_object ("container", "Container property", "Set the container", 
           OGMRIP_TYPE_CONTAINER, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_COPY, 
        g_param_spec_boolean ("copy", "Copy property", "Set copy", 
           FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_ENSURE_SYNC, 
        g_param_spec_boolean ("ensure-sync", "Ensure sync property", "Set ensure sync", 
           TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LOG_FILE, 
        g_param_spec_string ("log-file", "Log file property", "Set the log file", 
           NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_METHOD, 
        g_param_spec_uint ("method", "Method property", "Set the method", 
           OGMRIP_ENCODING_SIZE, OGMRIP_ENCODING_QUANTIZER, OGMRIP_ENCODING_SIZE,
           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PROFILE, 
        g_param_spec_object ("profile", "Profile property", "Set the profile", 
           OGMRIP_TYPE_PROFILE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RELATIVE, 
        g_param_spec_boolean ("relative", "Relative property", "Set relative", 
           FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
/*
  g_object_class_install_property (gobject_class, PROP_SUBP_CODECS, 
        g_param_spec_boxed ("subp-codecs", "Subp codecs property", "Set subp codecs", 
           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
*/
  g_object_class_install_property (gobject_class, PROP_TEST, 
        g_param_spec_boolean ("test", "Test property", "Set test", 
           TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TITLE, 
        g_param_spec_object ("title", "Title property", "Set title", 
           OGMRIP_TYPE_TITLE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VIDEO_CODEC, 
        g_param_spec_object ("video-codec", "Video codec property", "Set video codec", 
           OGMRIP_TYPE_VIDEO_CODEC, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipEncodingPriv));
}

static void
ogmrip_encoding_init (OGMRipEncoding *encoding)
{
  encoding->priv = OGMRIP_ENCODING_GET_PRIVATE (encoding);

  encoding->priv->ensure_sync = TRUE;
  encoding->priv->autocrop = TRUE;
  encoding->priv->autoscale = TRUE;
  encoding->priv->test = TRUE;
}

static void
ogmrip_encoding_constructed (GObject *gobject)
{
  OGMRipEncoding *encoding = OGMRIP_ENCODING (gobject);

  if (!encoding->priv->container)
  {
    GType type;

    type = ogmrip_plugin_get_nth_container (0);
    encoding->priv->container = g_object_new (type, NULL);
  }

  (*G_OBJECT_CLASS (ogmrip_encoding_parent_class)->constructed) (gobject);
}

static void
ogmrip_encoding_dispose (GObject *gobject)
{
  OGMRipEncoding *encoding = OGMRIP_ENCODING (gobject);

  if (encoding->priv->container)
  {
    g_object_unref (encoding->priv->container);
    encoding->priv->container = NULL;
  }

  if (encoding->priv->profile)
  {
    g_object_unref (encoding->priv->profile);
    encoding->priv->profile = NULL;
  }

  if (encoding->priv->video_codec)
  {
    g_object_unref (encoding->priv->video_codec);
    encoding->priv->video_codec = NULL;
  }

  if (encoding->priv->audio_codecs)
  {
    g_list_foreach (encoding->priv->audio_codecs, (GFunc) g_object_unref, NULL);
    g_list_free (encoding->priv->audio_codecs);
    encoding->priv->audio_codecs = NULL;
  }

  if (encoding->priv->subp_codecs)
  {
    g_list_foreach (encoding->priv->subp_codecs, (GFunc) g_object_unref, NULL);
    g_list_free (encoding->priv->subp_codecs);
    encoding->priv->subp_codecs = NULL;
  }

  if (encoding->priv->chapters)
  {
    g_list_foreach (encoding->priv->chapters, (GFunc) g_object_unref, NULL);
    g_list_free (encoding->priv->chapters);
    encoding->priv->chapters = NULL;
  }

  if (encoding->priv->title)
  {
    g_object_unref (encoding->priv->title);
    encoding->priv->title = NULL;
  }

  (*G_OBJECT_CLASS (ogmrip_encoding_parent_class)->dispose) (gobject);
}

static void
ogmrip_encoding_finalize (GObject *gobject)
{
  OGMRipEncoding *encoding = OGMRIP_ENCODING (gobject);

  ogmrip_encoding_close_log (encoding);
  g_free (encoding->priv->log_file);

  (*G_OBJECT_CLASS (ogmrip_encoding_parent_class)->finalize) (gobject);
}

static void
ogmrip_encoding_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipEncoding *encoding = OGMRIP_ENCODING (gobject);

  switch (property_id) 
  {
/*
    case PROP_AUDIO_CODECS:
      g_value_set_boxed (value, encoding->priv->audio_codecs);
      break;
*/
    case PROP_AUTOCROP:
      g_value_set_boolean (value, encoding->priv->autocrop);
      break;
    case PROP_AUTOSCALE:
      g_value_set_boolean (value, encoding->priv->autoscale);
      break;
    case PROP_CONTAINER:
      g_value_set_object (value, encoding->priv->container);
      break;
    case PROP_COPY:
      g_value_set_boolean (value, encoding->priv->copy);
      break;
    case PROP_ENSURE_SYNC:
      g_value_set_boolean (value, encoding->priv->ensure_sync);
      break;
    case PROP_LOG_FILE:
      g_value_set_string (value, encoding->priv->log_file);
      break;
    case PROP_METHOD:
      g_value_set_uint (value, encoding->priv->method);
      break;
    case PROP_PROFILE:
      g_value_set_object (value, encoding->priv->profile);
      break;
    case PROP_RELATIVE:
      g_value_set_boolean (value, encoding->priv->relative);
      break;
/*
    case PROP_SUBP_CODECS:
      g_value_set_boxed (value, encoding->priv->subp_codecs);
      break;
*/
    case PROP_TEST:
      g_value_set_boolean (value, encoding->priv->test);
      break;
    case PROP_TITLE:
      g_value_set_object (value, encoding->priv->title);
      break;
    case PROP_VIDEO_CODEC:
      g_value_set_object (value, encoding->priv->video_codec);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_encoding_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipEncoding *encoding = OGMRIP_ENCODING (gobject);

  switch (property_id) 
  {
/*
    case PROP_AUDIO_CODECS:
      encoding->priv->audio_codecs = g_value_get_boxed (value);
      break;
*/
    case PROP_AUTOCROP:
      encoding->priv->autocrop = g_value_get_boolean (value);
      break;
    case PROP_AUTOSCALE:
      encoding->priv->autoscale = g_value_get_boolean (value);
      break;
    case PROP_CONTAINER:
      ogmrip_encoding_set_container (encoding, g_value_get_object (value));
      break;
    case PROP_COPY:
      encoding->priv->copy = g_value_get_boolean (value);
      break;
    case PROP_ENSURE_SYNC:
      encoding->priv->ensure_sync = g_value_get_boolean (value);
      break;
    case PROP_LOG_FILE:
      ogmrip_encoding_set_log_file (encoding, g_value_get_string (value));
      break;
    case PROP_METHOD:
      encoding->priv->method = g_value_get_uint (value);
      break;
    case PROP_PROFILE:
      ogmrip_encoding_set_profile (encoding, g_value_get_object (value));
      break;
    case PROP_RELATIVE:
      encoding->priv->relative = g_value_get_boolean (value);
      break;
/*
    case PROP_SUBP_CODECS:
      encoding->priv->subp_codecs = g_value_get_boxed (value);
      break;
*/
    case PROP_TEST:
      encoding->priv->test = g_value_get_boolean (value);
      break;
    case PROP_TITLE:
      encoding->priv->title = g_value_dup_object (value);
      break;
    case PROP_VIDEO_CODEC:
      ogmrip_encoding_set_video_codec (encoding, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_encoding_run (OGMRipEncoding *encoding, OGMJobSpawn *spawn)
{
  encoding->priv->spawn = spawn;
}

static void
ogmrip_encoding_complete (OGMRipEncoding *encoding, OGMJobSpawn *spawn, guint result)
{
  encoding->priv->spawn = NULL;
}

/**
 * ogmrip_encoding_new:
 *
 * Creates a new #OGMRipEncoding.
 * @title: An #OGMRipTitle
 *
 * Returns: The newly created #OGMRipEncoding, or NULL
 */
OGMRipEncoding *
ogmrip_encoding_new (OGMRipTitle *title)
{
  return g_object_new (OGMRIP_TYPE_ENCODING, "title", title, NULL);
}

static void
weak_notify (gpointer media, GObject *where_the_object_was)
{
  g_object_unref (media);
}

static OGMRipEncoding *
ogmrip_encoding_new_from_xml (OGMRipXML *xml, GError **error)
{
  OGMRipEncoding *encoding = NULL;
  OGMRipMedia *media;
  gchar *str, *utf8;

  if (!g_str_equal (ogmrip_xml_get_name (xml), "encoding"))
    return NULL;

  utf8 = ogmrip_xml_get_string (xml, "uri");
  if (!utf8)
    return NULL;

  str = g_filename_from_utf8 (utf8, -1, NULL, NULL, NULL);
  g_free (utf8);

  if (!str)
    return NULL;

  if (!g_str_has_prefix (str, "dvd://"))
  {
    g_free (str);
    return NULL;
  }

  media = ogmdvd_disc_new (str, error);
  g_free (str);

  if (!media)
    return NULL;

  str = ogmrip_xml_get_string (xml, "id");
  if (str)
  {
    if (!g_str_equal (str, ogmrip_media_get_id (media)))
    {
      g_set_error (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_ID,
          _("Device does not contain the expected DVD"));
      g_object_unref (media);
    }
    else
    {
      OGMRipTitle *title;
      guint nr;

      nr = ogmrip_xml_get_uint (xml, "title");

      title = ogmrip_media_get_nth_title (media, nr);
      if (title)
      {
        encoding = ogmrip_encoding_new (title);
        if (encoding)
        {
          g_object_weak_ref (G_OBJECT (encoding), weak_notify, g_object_ref (media));
          ogmrip_encoding_parse (encoding, xml, NULL);
        }
      }
    }
  }

  g_object_unref (media);

  return encoding;
}

OGMRipEncoding *
ogmrip_encoding_new_from_file (GFile *file, GError **error)
{
  OGMRipXML *xml;
  OGMRipEncoding *encoding;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  xml = ogmrip_xml_new_from_file (file, error);
  if (!xml)
    return NULL;

  encoding = ogmrip_encoding_new_from_xml (xml, error);
  if (encoding == NULL && error != NULL && *error == NULL)
  {
    gchar *filename;

    filename = g_file_get_basename (file);
    g_set_error (error, 0, 0, _("'%s' does not contain a valid encoding"), filename);
    g_free (filename);
  }

  ogmrip_xml_free (xml);

  return encoding;
}

gboolean
ogmrip_encoding_export (OGMRipEncoding *encoding, GFile *file, GError **error)
{
  OGMRipXML *xml;
  gboolean retval;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  xml = ogmrip_xml_new ();

  retval = ogmrip_encoding_dump (encoding, xml, error) &&
    ogmrip_xml_save (xml, file, error);

  ogmrip_xml_free (xml);

  return retval;
}

void
ogmrip_encoding_add_audio_codec (OGMRipEncoding *encoding, OGMRipAudioCodec *codec)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (OGMRIP_IS_AUDIO_CODEC (codec));

  encoding->priv->audio_codecs = g_list_append (encoding->priv->audio_codecs, g_object_ref (codec));

  // g_object_notify (G_OBJECT (encoding), "audio-codecs");
}

OGMRipCodec *
ogmrip_encoding_get_nth_audio_codec (OGMRipEncoding *encoding, gint n)
{
  GList *link;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  if (n < 0)
    link = g_list_last (encoding->priv->audio_codecs);
  else
    link = g_list_nth (encoding->priv->audio_codecs, n);

  if (link)
    return link->data;

  return NULL;
}

GList *
ogmrip_encoding_get_audio_codecs (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return g_list_copy (encoding->priv->audio_codecs);
}

gint
ogmrip_encoding_get_n_audio_codecs (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return g_list_length (encoding->priv->audio_codecs);
}

void
ogmrip_encoding_add_subp_codec (OGMRipEncoding *encoding, OGMRipSubpCodec *codec)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (OGMRIP_IS_SUBP_CODEC (codec));

  encoding->priv->subp_codecs = g_list_append (encoding->priv->subp_codecs, g_object_ref (codec));

  // g_object_notify (G_OBJECT (encoding), "subp-codecs");
}

OGMRipCodec *
ogmrip_encoding_get_nth_subp_codec (OGMRipEncoding *encoding, gint n)
{
  GList *link;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  if (n < 0)
    link = g_list_last (encoding->priv->subp_codecs);
  else
    link = g_list_nth (encoding->priv->subp_codecs, n);

  if (link)
    return link->data;

  return NULL;
}

gint
ogmrip_encoding_get_n_subp_codecs (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return g_list_length (encoding->priv->subp_codecs);
}

GList *
ogmrip_encoding_get_subp_codecs (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return g_list_copy (encoding->priv->subp_codecs);
}

void
ogmrip_encoding_add_chapters (OGMRipEncoding *encoding, OGMRipChapters *chapters)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (OGMRIP_IS_CHAPTERS (chapters));

  encoding->priv->chapters = g_list_append (encoding->priv->chapters, g_object_ref (chapters));
}

GList *
ogmrip_encoding_get_chapters (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return g_list_copy (encoding->priv->chapters);
}

void
ogmrip_encoding_add_file (OGMRipEncoding *encoding, OGMRipFile *file)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (OGMRIP_IS_FILE (file));

  encoding->priv->files = g_list_append (encoding->priv->files, g_object_ref (file));
}

GList *
ogmrip_encoding_get_files (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return g_list_copy (encoding->priv->files);
}

gboolean
ogmrip_encoding_get_autocrop (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->autocrop;
}

void
ogmrip_encoding_set_autocrop (OGMRipEncoding *encoding, gboolean autocrop)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  encoding->priv->autocrop = autocrop;

  g_object_notify (G_OBJECT (encoding), "autocrop");
}

gboolean
ogmrip_encoding_get_autoscale (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->autoscale;
}

void
ogmrip_encoding_set_autoscale (OGMRipEncoding *encoding, gboolean autoscale)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  encoding->priv->autoscale = autoscale;

  g_object_notify (G_OBJECT (encoding), "autoscale");
}

OGMRipContainer *
ogmrip_encoding_get_container (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return encoding->priv->container;
}

void
ogmrip_encoding_set_container (OGMRipEncoding *encoding, OGMRipContainer *container)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));

  g_object_ref (container);
  if (encoding->priv->container)
    g_object_unref (encoding->priv->container);
  encoding->priv->container = container;

  g_object_notify (G_OBJECT (encoding), "container");
}

gboolean
ogmrip_encoding_get_copy (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->copy;
}

void
ogmrip_encoding_set_copy (OGMRipEncoding *encoding, gboolean copy)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  if (copy)
  {
    const gchar *uri;
    struct stat buf;

    copy = FALSE;

    uri = ogmrip_media_get_uri (ogmrip_title_get_media (encoding->priv->title));
    if (!g_str_has_prefix (uri, "dvd://"))
      g_warning ("Unknown scheme for '%s'", uri);
    else
    {
      if (g_stat (uri + 6, &buf) == 0)
        copy = S_ISBLK (buf.st_mode);
    }
  }

  encoding->priv->copy = copy;

  g_object_notify (G_OBJECT (encoding), "copy");
}

gboolean
ogmrip_encoding_get_ensure_sync (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->ensure_sync;
}

void
ogmrip_encoding_set_ensure_sync (OGMRipEncoding *encoding, gboolean ensure_sync)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  encoding->priv->ensure_sync = ensure_sync;

  g_object_notify (G_OBJECT (encoding), "ensure-sync");
}

const gchar *
ogmrip_encoding_get_log_file (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return encoding->priv->log_file;
}

void
ogmrip_encoding_set_log_file (OGMRipEncoding *encoding, const gchar *filename)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  if (encoding->priv->log_file)
  {
    g_free (encoding->priv->log_file);
    encoding->priv->log_file = NULL;
  }
  
  if (filename)
    encoding->priv->log_file = g_strdup (filename);

  g_object_notify (G_OBJECT (encoding), "log-file");
}

gint
ogmrip_encoding_get_method (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return encoding->priv->method;
}

void
ogmrip_encoding_set_method (OGMRipEncoding *encoding, OGMRipEncodingMethod method)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  encoding->priv->method = method;

  g_object_notify (G_OBJECT (encoding), "method");
}

OGMRipProfile *
ogmrip_encoding_get_profile (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return encoding->priv->profile;
}

void
ogmrip_encoding_set_profile (OGMRipEncoding *encoding, OGMRipProfile *profile)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (OGMRIP_IS_PROFILE (profile));

  g_object_ref (profile);
  if (encoding->priv->profile)
    g_object_unref (encoding->priv->profile);
  encoding->priv->profile = profile;

  g_object_notify (G_OBJECT (encoding), "profile");
}

gboolean
ogmrip_encoding_get_test (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->test;
}

void
ogmrip_encoding_set_test (OGMRipEncoding *encoding, gboolean test)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  encoding->priv->test = test;

  g_object_notify (G_OBJECT (encoding), "test");
}

OGMRipTitle *
ogmrip_encoding_get_title (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return encoding->priv->title;
}

OGMRipCodec *
ogmrip_encoding_get_video_codec (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return OGMRIP_CODEC (encoding->priv->video_codec);
}

void
ogmrip_encoding_set_video_codec (OGMRipEncoding *encoding, OGMRipVideoCodec *codec)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (codec == NULL || OGMRIP_IS_VIDEO_CODEC (codec));

  if (codec)
    g_object_ref (codec);
  if (encoding->priv->video_codec)
    g_object_unref (encoding->priv->video_codec);
  encoding->priv->video_codec = codec;

  g_object_notify (G_OBJECT (encoding), "video-codec");
}

gboolean
ogmrip_encoding_get_relative (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->relative;
}

void
ogmrip_encoding_set_relative (OGMRipEncoding *encoding, gboolean relative)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  encoding->priv->relative = relative;

  g_object_notify (G_OBJECT (encoding), "relative");
}

static gint64
ogmrip_encoding_get_rip_size (OGMRipEncoding *encoding)
{
  guint number, size;
  gint bitrate;

  ogmrip_container_get_split (encoding->priv->container, &number, &size);

  if (size > 0)
  {
    gdouble factor = 1.0;

    if (encoding->priv->relative)
    {
      OGMRipStream *stream;
      gdouble full_len;

      stream = ogmrip_codec_get_input (OGMRIP_CODEC (encoding->priv->video_codec));

      full_len = ogmrip_title_get_length (ogmrip_stream_get_title (stream), NULL);
      if (full_len < 0)
        return -1;

      factor = ogmrip_codec_get_length (OGMRIP_CODEC (encoding->priv->video_codec), NULL) / full_len;
    }

    return ceil (factor * size * number * 1024 * 1024);
  }

  bitrate = ogmrip_video_codec_get_bitrate (encoding->priv->video_codec);
  if (bitrate > 0)
    return ceil (bitrate * ogmrip_codec_get_length (OGMRIP_CODEC (encoding->priv->video_codec), NULL) / 8.0);

  return 0;
}

static gint64
ogmrip_encoding_get_file_size (OGMRipCodec *codec)
{
  struct stat buf;
  const gchar *filename;
  guint64 size = 0;

  filename = ogmrip_file_get_path (ogmrip_codec_get_output (codec));
  if (filename && g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    if (g_stat (filename, &buf) == 0)
      size = (guint64) buf.st_size;

  return size;
}

static gint64
ogmrip_encoding_get_nonvideo_size (OGMRipEncoding *encoding)
{
  GList *link;
  gint64 nonvideo = 0;

  for (link = encoding->priv->audio_codecs; link; link = link->next)
    nonvideo += ogmrip_encoding_get_file_size (link->data);

  for (link = encoding->priv->subp_codecs; link; link = link->next)
    nonvideo += ogmrip_encoding_get_file_size (link->data);

  for (link = encoding->priv->chapters; link; link = link->next)
    nonvideo += ogmrip_encoding_get_file_size (link->data);

  for (link = encoding->priv->files; link; link = link->next)
    nonvideo += ogmrip_title_get_size (link->data);

  return nonvideo;
}

static gint64
ogmrip_encoding_get_video_overhead (OGMRipEncoding *encoding)
{
  OGMRipStream *stream;
  gdouble framerate, length, frames;
  guint num, denom;
  gint overhead;

  if (!encoding->priv->video_codec)
    return 0;

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (encoding->priv->video_codec));
  ogmrip_video_stream_get_framerate (OGMRIP_VIDEO_STREAM (stream), &num, &denom);
  framerate = num / (gdouble) denom;

  length = ogmrip_codec_get_length (OGMRIP_CODEC (encoding->priv->video_codec), NULL);
  frames = length * framerate;

  overhead = ogmrip_container_get_overhead (encoding->priv->container);

  return (gint64) (frames * overhead);
}

static gint64
ogmrip_encoding_get_audio_overhead (OGMRipEncoding *encoding, OGMRipAudioCodec *codec)
{
  OGMRipFile *output;
  gdouble length, audio_frames;
  gint samples_per_frame, sample_rate, channels, overhead;

  length = ogmrip_codec_get_length (OGMRIP_CODEC (codec), NULL);

  output = ogmrip_codec_get_output (OGMRIP_CODEC (codec));
  samples_per_frame = ogmrip_audio_stream_get_samples_per_frame (OGMRIP_AUDIO_STREAM (output));

  sample_rate = 48000;
  channels = 1;

  if (ogmrip_plugin_get_audio_codec_format (G_OBJECT_TYPE (codec)) == OGMRIP_FORMAT_COPY)
  {
    OGMRipStream *stream;

    stream = ogmrip_codec_get_input (OGMRIP_CODEC (codec));
    sample_rate = ogmrip_audio_stream_get_sample_rate (OGMRIP_AUDIO_STREAM (stream));
    channels = ogmrip_audio_stream_get_channels (OGMRIP_AUDIO_STREAM (stream));
  }
  else
  {
    sample_rate = ogmrip_audio_codec_get_sample_rate (codec);
    channels = ogmrip_audio_codec_get_channels (codec);
  }

  audio_frames = length * sample_rate * (channels + 1) / samples_per_frame;

  overhead = ogmrip_container_get_overhead (encoding->priv->container);

  return (gint64) (audio_frames * overhead);
}

static gint64
ogmrip_encoding_get_subp_overhead (OGMRipEncoding *encoding, OGMRipSubpCodec *codec)
{
  return 0;
}

static gint64
ogmrip_encoding_get_file_overhead (OGMRipEncoding *encoding, OGMRipFile *file)
{
  glong length, audio_frames;
  gint samples_per_frame, sample_rate, channels, overhead;

  if (!OGMRIP_IS_AUDIO_STREAM (file))
    return 0;

  length = ogmrip_title_get_length (OGMRIP_TITLE (file), NULL);
  sample_rate = ogmrip_audio_stream_get_sample_rate (OGMRIP_AUDIO_STREAM (file));
  samples_per_frame = ogmrip_audio_stream_get_samples_per_frame (OGMRIP_AUDIO_STREAM (file));

  channels = 1;
  if (ogmrip_stream_get_format (OGMRIP_STREAM (file)) != OGMRIP_FORMAT_COPY)
    channels = ogmrip_audio_stream_get_channels (OGMRIP_AUDIO_STREAM (file));

  audio_frames = length * sample_rate * (channels + 1) / samples_per_frame;

  overhead = ogmrip_container_get_overhead (encoding->priv->container);

  return (gint64) (audio_frames * overhead);
}

static gint64
ogmrip_encoding_get_overhead_size (OGMRipEncoding *encoding)
{
  GList *link;
  gint64 overhead = 0;

  overhead = ogmrip_encoding_get_video_overhead (encoding);

  for (link = encoding->priv->audio_codecs; link; link = link->next)
    overhead += ogmrip_encoding_get_audio_overhead (encoding, link->data);

  for (link = encoding->priv->subp_codecs; link; link = link->next)
    overhead += ogmrip_encoding_get_subp_overhead (encoding, link->data);

  for (link = encoding->priv->files; link; link = link->next)
    overhead += ogmrip_encoding_get_file_overhead (encoding, link->data);

  return overhead;
}

gint
ogmrip_encoding_autobitrate (OGMRipEncoding *encoding)
{
  gdouble video_size, length;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);
  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (encoding->priv->video_codec), -1);

  video_size = ogmrip_encoding_get_rip_size (encoding) -
    ogmrip_encoding_get_nonvideo_size (encoding) -
    ogmrip_encoding_get_overhead_size (encoding);

  length = ogmrip_codec_get_length (OGMRIP_CODEC (encoding->priv->video_codec), NULL);

  return (video_size * 8.) / length;
}

#define ROUND(x) ((gint) ((x) + 0.5) != (gint) (x) ? ((gint) ((x) + 0.5)) : ((gint) (x)))

void
ogmrip_encoding_autoscale (OGMRipEncoding *encoding, gdouble bpp, guint *width, guint *height)
{
  OGMRipStream *stream;
  guint an, ad, rn, rd, cw, ch, rw, rh;
  gdouble ratio, rbpp;
  gint br;

  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (OGMRIP_IS_VIDEO_CODEC (encoding->priv->video_codec));
  g_return_if_fail (width != NULL && height != NULL);

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (encoding->priv->video_codec));

  ogmrip_video_stream_get_resolution (OGMRIP_VIDEO_STREAM (stream), &rw, &rh);
  ogmrip_video_stream_get_framerate (OGMRIP_VIDEO_STREAM (stream), &rn, &rd);

  ogmrip_video_codec_get_crop_size (encoding->priv->video_codec, NULL, NULL, &cw, &ch);
  ogmrip_video_codec_get_aspect_ratio (encoding->priv->video_codec, &an, &ad);

  ratio = cw / (gdouble) ch * rh / rw * an / ad;

  br = ogmrip_video_codec_get_bitrate (encoding->priv->video_codec);
  if (br > 0)
  {
    *height = rh;
    for (*width = rw - 25 * 16; *width <= rw; *width += 16)
    {
      *height = ROUND (*width / ratio);

      rbpp = (br * rd) / (gdouble) (*width * *height * rn);

      if (rbpp < bpp)
        break;
    }
  }
  else
  {
    *width = cw;
    *height = ROUND (*width / ratio);
  }

  *width = MIN (*width, rw);
}

static gboolean
get_space_left (const gchar *path, guint64 *space, gchar **id, GError **error)
{
  GFile *file;
  GFileInfo *info;

  file = g_file_new_for_path (path);
  info = g_file_query_info (file,
      G_FILE_ATTRIBUTE_FILESYSTEM_FREE "," G_FILE_ATTRIBUTE_ID_FILESYSTEM,
      G_FILE_QUERY_INFO_NONE, NULL, error);
  g_object_unref (file);

  *space = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
  *id = g_strdup (g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_ID_FILESYSTEM));

  g_object_unref (info);

  return TRUE;
}

static gboolean
ogmrip_encoding_check_space (OGMRipEncoding *encoding, guint64 output_size, guint64 tmp_size, GError **error)
{
  OGMRipContainer *container;
  guint64 output_space, tmp_space;
  gchar *output_id, *tmp_id, *str;
  gboolean retval = FALSE;

  if (output_size + tmp_size == 0)
    return TRUE;

  container = ogmrip_encoding_get_container (encoding);

  if (!get_space_left (ogmrip_container_get_output (container), &output_space, &output_id, error))
    return FALSE;

  if (!get_space_left (ogmrip_fs_get_tmp_dir (), &tmp_space, &tmp_id, error))
  {
    g_free (tmp_id);
    return FALSE;
  }

  if (g_str_equal (output_id, tmp_id))
  {
    retval = output_size + tmp_size < output_space;
    if (!retval)
    {
      str = g_format_size_for_display (output_size + tmp_size);
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NO_SPACE,
          _("Not enough space to store output and temporary files (%s needed)."), str);
      g_free (str);
    }
  }
  else
  {
    retval = output_size < output_space;
    if (!retval)
    {
      str = g_format_size_for_display (output_size);
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NO_SPACE,
          _("Not enough space to store the output file (%s needed)."), str);
      g_free (str);
    }
    else
    {
      retval = tmp_size < tmp_space;
      if (!retval)
      {
        str = g_format_size_for_display (output_size);
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_NO_SPACE,
            _("Not enough space to store the temporary files (%s needed)."), str);
        g_free (str);
      }
    }
  }

  g_free (output_id);
  g_free (tmp_id);

  return retval;
}

static void
ogmrip_encoding_task_progressed (OGMRipEncoding *encoding, gdouble fraction, OGMJobSpawn *spawn)
{
  g_signal_emit (encoding, signals[PROGRESS], 0, spawn, fraction);
}

static gint
ogmrip_encoding_copy (OGMRipEncoding *encoding, GError **error)
{
  OGMRipMedia *media;
  OGMJobSpawn *spawn;
  gboolean result;
  gchar *output;

  media = ogmrip_title_get_media (encoding->priv->title);

  ogmjob_log_printf ("Copying %s\n\n", ogmrip_media_get_label (media));

  output = g_build_filename (ogmrip_fs_get_tmp_dir (), ogmrip_media_get_id (media), NULL);
  spawn = ogmrip_copy_new (media, output);
  g_free (output);

  g_signal_connect_swapped (spawn, "progress",
      G_CALLBACK (ogmrip_encoding_task_progressed), encoding);

  g_signal_emit (encoding, signals[RUN], 0, spawn);
  result = ogmjob_spawn_run (spawn, error);
  g_signal_emit (encoding, signals[COMPLETE], 0, spawn, result);

  g_object_unref (spawn);

  return result;
}

static gint
ogmrip_encoding_analyze (OGMRipEncoding *encoding, GError **error)
{
  OGMJobSpawn *spawn;
  gboolean result;

  ogmjob_log_printf ("\nAnalyzing video title %d\n", ogmrip_title_get_nr (encoding->priv->title) + 1);
  ogmjob_log_printf ("-----------------------\n\n");

  spawn = ogmrip_analyze_new (encoding->priv->title);
  g_signal_connect_swapped (spawn, "progress",
      G_CALLBACK (ogmrip_encoding_task_progressed), encoding);

  g_signal_emit (encoding, signals[RUN], 0, spawn);
  result = ogmjob_spawn_run (spawn, error);
  g_signal_emit (encoding, signals[COMPLETE], 0, spawn, result);

  g_object_unref (spawn);

  if (result == OGMJOB_RESULT_SUCCESS)
  {
    ogmjob_log_printf ("\nTelecine: %s\n",
        ogmrip_title_get_telecine (encoding->priv->title) ? "true" : "false");
    ogmjob_log_printf ("Progressive: %s\n\n",
        ogmrip_title_get_progressive (encoding->priv->title) ? "true" : "false");
    ogmjob_log_printf ("Interlaced: %s\n\n",
        ogmrip_title_get_interlaced (encoding->priv->title) ? "true" : "false");
  }

  return result;
}

static gint
ogmrip_encoding_test (OGMRipEncoding *encoding, GError **error)
{
  OGMJobSpawn *spawn;
  gboolean result;

  spawn = ogmrip_test_new (encoding);
  g_signal_connect_swapped (spawn, "progress",
      G_CALLBACK (ogmrip_encoding_task_progressed), encoding);

  g_signal_emit (encoding, signals[RUN], 0, spawn);
  result = ogmjob_spawn_run (spawn, error);
  g_signal_emit (encoding, signals[COMPLETE], 0, spawn, result);

  g_object_unref (spawn);

  return result;
}

static gint
ogmrip_encoding_run_codec (OGMRipEncoding *encoding, OGMRipCodec *codec, GError **error)
{
  OGMRipStream *stream;
  gint result;

  stream = ogmrip_codec_get_input (codec);
  if (OGMRIP_IS_SUBP_STREAM (stream))
  {
    ogmjob_log_printf ("\nEncoding subp stream %02d\n",
        ogmrip_subp_stream_get_nr (OGMRIP_SUBP_STREAM (stream)) + 1);
    ogmjob_log_printf ("-----------------------\n\n");
  }
  else if (OGMRIP_IS_AUDIO_STREAM (stream))
  {
    ogmjob_log_printf ("\nEncoding audio stream %02d\n",
        ogmrip_audio_stream_get_nr (OGMRIP_AUDIO_STREAM (stream)) + 1);
    ogmjob_log_printf ("------------------------\n\n");
  }
  else if (OGMRIP_IS_VIDEO_STREAM (stream))
  {
    ogmjob_log_printf ("\nEncoding video stream\n");
    ogmjob_log_printf ("---------------------\n\n");
  }

  if (OGMRIP_IS_CONFIGURABLE (codec) &&
      ogmrip_video_codec_get_quality (OGMRIP_VIDEO_CODEC (codec)) == OGMRIP_QUALITY_USER)
    ogmrip_configurable_configure (OGMRIP_CONFIGURABLE (codec), encoding->priv->profile);

  g_signal_connect_swapped (codec, "progress",
      G_CALLBACK (ogmrip_encoding_task_progressed), encoding);

  g_signal_emit (encoding, signals[RUN], 0, codec);
  result = ogmjob_spawn_run (OGMJOB_SPAWN (codec), error);
  g_signal_emit (encoding, signals[COMPLETE], 0, codec, result);

  g_signal_handlers_disconnect_by_func (codec,
      ogmrip_encoding_task_progressed, encoding);

  if (result != OGMJOB_RESULT_SUCCESS)
    return result;

  ogmrip_container_add_file (encoding->priv->container, ogmrip_codec_get_output (codec));

  return OGMJOB_RESULT_SUCCESS;
}

static gint
ogmrip_encoding_merge (OGMRipEncoding *encoding, GError **error)
{
  gint result;

  ogmjob_log_printf ("\nMerging\n");
  ogmjob_log_printf ("-------\n\n");

  g_signal_connect_swapped (encoding->priv->container, "progress",
      G_CALLBACK (ogmrip_encoding_task_progressed), encoding);

  g_signal_emit (encoding, signals[RUN], 0, encoding->priv->container);
  result = ogmjob_spawn_run (OGMJOB_SPAWN (encoding->priv->container), error);
  g_signal_emit (encoding, signals[COMPLETE], 0, encoding->priv->container, result);

  g_signal_handlers_disconnect_by_func (encoding->priv->container,
      ogmrip_encoding_task_progressed, encoding);

  return result;
}

gint
ogmrip_encoding_encode (OGMRipEncoding *encoding, GError **error)
{
  gint result = OGMJOB_RESULT_ERROR;
  GList *list, *link;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), OGMJOB_RESULT_ERROR);
  g_return_val_if_fail (error == NULL || *error == NULL, OGMJOB_RESULT_ERROR);
  g_return_val_if_fail (ogmrip_title_is_open (encoding->priv->title), OGMJOB_RESULT_ERROR);

  ogmrip_encoding_open_log (encoding);
/*
  ogmjob_log_printf ("ENCODING: %s\n\n", ogmrip_encoding_get_label (encoding));
*/
  g_signal_emit (encoding, signals[RUN], 0, NULL);

  if (!ogmrip_encoding_check_space (encoding, 0, 0, error))
    return FALSE;

  /*
   * Copy the media
   */
  if (encoding->priv->copy)
  {
    result = ogmrip_encoding_copy (encoding, error);
    if (result != OGMJOB_RESULT_SUCCESS)
      goto encode_cleanup;
  }

  /*
   * Analyze the title
   */
  if (encoding->priv->title)
  {
    result = ogmrip_encoding_analyze (encoding, error);
    if (result != OGMJOB_RESULT_SUCCESS)
      goto encode_cleanup;
  }

  /*
   * Clear the container
   */
  list = ogmrip_container_get_files (encoding->priv->container);
  for (link = list; link; link = link->next)
  {
    ogmrip_container_remove_file (encoding->priv->container, link->data);
    g_free (link->data);
  }
  g_list_free (list);

  /*
   * Extract all chapters
   */
  for (link = encoding->priv->chapters; link; link = link->next)
  {
    result = ogmrip_encoding_run_codec (encoding, link->data, error);
    if (result != OGMJOB_RESULT_SUCCESS)
      goto encode_cleanup;
  }

  /*
   * Extract all subtitles except harsubed ones
   */
  for (link = encoding->priv->subp_codecs; link; link = link->next)
  {
    if (G_OBJECT_TYPE (link->data) != OGMRIP_TYPE_HARDSUB)
    {
      result = ogmrip_encoding_run_codec (encoding, link->data, error);
      if (result != OGMJOB_RESULT_SUCCESS)
        goto encode_cleanup;
    }
  }

  /*
   * Encode all audio tracks
   */
  for (link = encoding->priv->audio_codecs; link; link = link->next)
  {
    result = ogmrip_encoding_run_codec (encoding, link->data, error);
    if (result != OGMJOB_RESULT_SUCCESS)
      goto encode_cleanup;
  }

  if (encoding->priv->video_codec)
  {
    ogmjob_log_printf ("\nSetting video parameters");
    ogmjob_log_printf ("\n------------------------\n");

    /*
     * Compute bitrate if encoding by size
     */
    switch (encoding->priv->method)
    {
      case OGMRIP_ENCODING_SIZE:
        ogmrip_video_codec_set_bitrate (encoding->priv->video_codec,
            ogmrip_encoding_autobitrate (encoding));
        ogmjob_log_printf ("Automatic bitrate: %d bps\n\n",
            ogmrip_video_codec_get_bitrate (encoding->priv->video_codec));
        break;
      case OGMRIP_ENCODING_BITRATE:
        ogmjob_log_printf ("Constant bitrate: %d bps\n\n",
            ogmrip_video_codec_get_bitrate (encoding->priv->video_codec));
        break;
      case OGMRIP_ENCODING_QUANTIZER:
        ogmjob_log_printf ("Constant quantizer: %lf\n\n",
            ogmrip_video_codec_get_quantizer (encoding->priv->video_codec));
        break;
    }

    /*
     * Get cropping parameters
     */
    if (encoding->priv->autocrop &&
        ogmrip_video_codec_get_can_crop (encoding->priv->video_codec))
    {
      OGMRipStream *stream;
      guint x, y, w, h;

      stream = ogmrip_codec_get_input (OGMRIP_CODEC (encoding->priv->video_codec));
      ogmrip_video_stream_get_crop_size (OGMRIP_VIDEO_STREAM (stream), &x, &y, &w, &h);
      ogmrip_video_codec_set_crop_size (encoding->priv->video_codec, x, y, w, h);
    }

    /*
     * Compute scaling parameters
     */
    if (encoding->priv->autoscale &&
        ogmrip_video_codec_get_scaler (encoding->priv->video_codec) != OGMRIP_SCALER_NONE)
    {
      gdouble bpp;
      guint w, h;

      bpp = ogmrip_video_codec_get_bits_per_pixel (encoding->priv->video_codec);
      ogmrip_encoding_autoscale (encoding, bpp, &w, &h);
      ogmrip_video_codec_set_scale_size (encoding->priv->video_codec, w, h);
    }

    /*
     * Set ensure sync
     */
    if (encoding->priv->audio_codecs && encoding->priv->ensure_sync)
    {
      OGMRipStream *stream;

      stream = ogmrip_codec_get_input (OGMRIP_CODEC (encoding->priv->audio_codecs->data));
      ogmrip_video_codec_set_ensure_sync (encoding->priv->video_codec, OGMRIP_AUDIO_STREAM (stream));
    }

    /*
     * Run the compressibility test
     */
    if (encoding->priv->test)
    {
      result = ogmrip_encoding_test (encoding, error);
      if (result != OGMJOB_RESULT_SUCCESS)
        goto encode_cleanup;
    }

    /*
     * Encode video stream
     */
    result = ogmrip_encoding_run_codec (encoding, OGMRIP_CODEC (encoding->priv->video_codec), error);
    if (result != OGMJOB_RESULT_SUCCESS)
      goto encode_cleanup;
  }

  result = ogmrip_encoding_merge (encoding, error);

encode_cleanup:
  g_signal_emit (encoding, signals[COMPLETE], 0, NULL, result);

  ogmrip_encoding_close_log (encoding);

  return result;
}

void
ogmrip_encoding_cancel (OGMRipEncoding *encoding)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  if (encoding->priv->spawn)
    ogmjob_spawn_cancel (encoding->priv->spawn);
}

void
ogmrip_encoding_suspend (OGMRipEncoding *encoding)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  if (encoding->priv->spawn)
    ogmjob_spawn_suspend (encoding->priv->spawn);
}

void
ogmrip_encoding_resume (OGMRipEncoding *encoding)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  if (encoding->priv->spawn)
    ogmjob_spawn_resume (encoding->priv->spawn);
}

