/* OGMRip - A library for DVD ripping and encoding
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

/**
 * SECTION:ogmrip-subp-codec
 * @title: OGMRipSubpCodec
 * @short_description: Base class for subtitles codecs
 * @include: ogmrip-subp-codec.h
 */

#include "ogmrip-subp-codec.h"

#define OGMRIP_SUBP_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_SUBP_CODEC, OGMRipSubpCodecPriv))

struct _OGMRipSubpCodecPriv
{
  OGMDvdSubpStream *stream;
  gboolean forced_only;

  gchar *label;

  OGMRipCharset charset;
  OGMRipNewline newline;
};

enum 
{
  PROP_0,
  PROP_STREAM,
  PROP_FORCED_ONLY,
  PROP_CHARSET,
  PROP_NEWLINE
};

static void ogmrip_subp_codec_dispose      (GObject      *gobject);
static void ogmrip_subp_codec_finalize     (GObject      *gobject);
static void ogmrip_subp_codec_set_property (GObject      *gobject,
                                            guint        property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void ogmrip_subp_codec_get_property (GObject      *gobject,
                                            guint        property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);

G_DEFINE_ABSTRACT_TYPE (OGMRipSubpCodec, ogmrip_subp_codec, OGMRIP_TYPE_CODEC)

static void
ogmrip_subp_codec_class_init (OGMRipSubpCodecClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = ogmrip_subp_codec_dispose;
  gobject_class->finalize = ogmrip_subp_codec_finalize;
  gobject_class->set_property = ogmrip_subp_codec_set_property;
  gobject_class->get_property = ogmrip_subp_codec_get_property;

  g_object_class_install_property (gobject_class, PROP_STREAM, 
        g_param_spec_pointer ("stream", "Sub stream property", "Set subp stream", 
           G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FORCED_ONLY, 
        g_param_spec_boolean ("forced-only", "Forced only property", "Set forced only", 
           FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHARSET,
      g_param_spec_uint ("charset", "Charset property", "Set charset",
        0, OGMRIP_CHARSET_ASCII, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_NEWLINE,
      g_param_spec_uint ("newline", "Newline property", "Set newline",
        0, OGMRIP_NEWLINE_CR_LF, 0, G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (OGMRipSubpCodecPriv));
}

static void
ogmrip_subp_codec_init (OGMRipSubpCodec *subp)
{
  subp->priv = OGMRIP_SUBP_GET_PRIVATE (subp);
}

static void
ogmrip_subp_codec_dispose (GObject *gobject)
{
  OGMRipSubpCodec *subp;

  subp = OGMRIP_SUBP_CODEC (gobject);
  if (subp->priv->stream)
  {
    ogmdvd_stream_unref (OGMDVD_STREAM (subp->priv->stream));
    subp->priv->stream = NULL;
  }

  G_OBJECT_CLASS (ogmrip_subp_codec_parent_class)->dispose (gobject);
}

static void
ogmrip_subp_codec_finalize (GObject *gobject)
{
  OGMRipSubpCodec *subp;

  subp = OGMRIP_SUBP_CODEC (gobject);
  if (subp->priv->label)
  {
    g_free (subp->priv->label);
    subp->priv->label = NULL;
  }

  G_OBJECT_CLASS (ogmrip_subp_codec_parent_class)->finalize (gobject);
}

static void
ogmrip_subp_codec_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipSubpCodec *subp;

  subp = OGMRIP_SUBP_CODEC (gobject);

  switch (property_id) 
  {
    case PROP_STREAM:
      ogmrip_subp_codec_set_dvd_subp_stream (subp, g_value_get_pointer (value));
      break;
    case PROP_FORCED_ONLY: 
      subp->priv->forced_only = g_value_get_boolean (value);
      break;
    case PROP_CHARSET:
      subp->priv->charset = g_value_get_uint (value);
      break;
    case PROP_NEWLINE: 
      subp->priv->newline = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_subp_codec_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipSubpCodec *subp;

  subp = OGMRIP_SUBP_CODEC (gobject);

  switch (property_id) 
  {
    case PROP_STREAM:
      g_value_set_pointer (value, subp->priv->stream);
      break;
    case PROP_FORCED_ONLY: 
      g_value_set_boolean (value, subp->priv->forced_only);
      break;
    case PROP_CHARSET:
      g_value_set_uint (value, subp->priv->charset);
      break;
    case PROP_NEWLINE:
      g_value_set_uint (value, subp->priv->newline);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

/**
 * ogmrip_subp_codec_set_dvd_subp_stream:
 * @subp: an #OGMRipSubpCodec
 * @stream: an #OGMDvdSubpStream
 *
 * Sets the subtitle stream to encode.
 */
void
ogmrip_subp_codec_set_dvd_subp_stream (OGMRipSubpCodec *subp, OGMDvdSubpStream *stream)
{
  g_return_if_fail (OGMRIP_IS_SUBP_CODEC (subp));
  g_return_if_fail (stream != NULL);

  if (subp->priv->stream != stream)
  {
    ogmdvd_stream_ref (OGMDVD_STREAM (stream));

    if (subp->priv->stream)
      ogmdvd_stream_unref (OGMDVD_STREAM (subp->priv->stream));
    subp->priv->stream = stream;

    ogmrip_codec_set_input (OGMRIP_CODEC (subp), 
        ogmdvd_stream_get_title (OGMDVD_STREAM (stream)));
  }
}

/**
 * ogmrip_subp_codec_get_dvd_subp_stream:
 * @subp: an #OGMRipSubpCodec
 *
 * Gets the subtitle stream to encode.
 *
 * Returns: an #OGMDvdSubpStream, or NULL
 */
OGMDvdSubpStream *
ogmrip_subp_codec_get_dvd_subp_stream (OGMRipSubpCodec *subp)
{
  g_return_val_if_fail (OGMRIP_IS_SUBP_CODEC (subp), NULL);

  return subp->priv->stream;
}

/**
 * ogmrip_subp_codec_set_forced_only:
 * @subp: an #OGMRipSubpCodec
 * @forced_only: %TRUE to extract forced subtitles only
 *
 * Sets whether to extract forced subtitles only.
 */
void
ogmrip_subp_codec_set_forced_only (OGMRipSubpCodec *subp, gboolean forced_only)
{
  g_return_if_fail (OGMRIP_IS_SUBP_CODEC (subp));

  subp->priv->forced_only = forced_only;
}

/**
 * ogmrip_subp_codec_get_forced_only:
 * @subp: an #OGMRipSubpCodec
 *
 * Gets whether to extract forced subtitles only.
 *
 * Returns: %TRUE to extract forced subtitles only
 */
gboolean
ogmrip_subp_codec_get_forced_only (OGMRipSubpCodec *subp)
{
  g_return_val_if_fail (OGMRIP_IS_SUBP_CODEC (subp), FALSE);

  return subp->priv->forced_only;
}

/**
 * ogmrip_subp_codec_set_charset:
 * @subp: an #OGMRipSubpCodec
 * @charset: the #OGMRipCharset
 *
 * Sets the character set of text subtitles
 */
void
ogmrip_subp_codec_set_charset (OGMRipSubpCodec *subp, OGMRipCharset charset)
{
  g_return_if_fail (OGMRIP_IS_SUBP_CODEC (subp));

  subp->priv->charset = charset;
}

/**
 * ogmrip_subp_codec_get_charset:
 * @subp: an #OGMRipSubpCodec
 *
 * Gets the character set of text subtitles
 *
 * Returns: an #OGMRipCharset, or -1
 */
gint
ogmrip_subp_codec_get_charset (OGMRipSubpCodec *subp)
{ 
  g_return_val_if_fail (OGMRIP_IS_SUBP_CODEC (subp), -1);
      
  return subp->priv->charset;
} 
    
/**
 * ogmrip_subp_codec_set_newline:
 * @subp: an #OGMRipSubpCodec
 * @newline: the #OGMRipNewline
 *
 * Sets the end-of-line characters of text subtitles
 */
void
ogmrip_subp_codec_set_newline (OGMRipSubpCodec *subp, OGMRipNewline newline)
{ 
  g_return_if_fail (OGMRIP_IS_SUBP_CODEC (subp));

  subp->priv->newline = newline;
}

/**
 * ogmrip_subp_codec_get_newline:
 * @subp: an #OGMRipSubpCodec
 *
 * Gets the end-of-line characters of text subtitles
 *
 * Returns: the #OGMRipNewline, or -1
 */
gint
ogmrip_subp_codec_get_newline (OGMRipSubpCodec *subp) 
{
  g_return_val_if_fail (OGMRIP_IS_SUBP_CODEC (subp), -1);

  return subp->priv->newline;
}

/**
 * ogmrip_subp_codec_set_label:
 * @subp: an #OGMRipSubpCodec
 * @label: the track name
 *
 * Sets the name of the track.
 */
void
ogmrip_subp_codec_set_label (OGMRipSubpCodec *subp, const gchar *label)
{
  g_return_if_fail (OGMRIP_IS_SUBP_CODEC (subp));

  if (subp->priv->label)
  {
    g_free (subp->priv->label);
    subp->priv->label = NULL;
  }

  if (label)
    subp->priv->label = g_strdup (label);
}

/**
 * ogmrip_subp_codec_get_label:
 * @subp: an #OGMRipSubpCodec
 *
 * Gets the name of the track.
 *
 * Returns: the track name
 */
G_CONST_RETURN gchar *
ogmrip_subp_codec_get_label (OGMRipSubpCodec *subp)
{
  g_return_val_if_fail (OGMRIP_IS_SUBP_CODEC (subp), NULL);

  return subp->priv->label;
}

