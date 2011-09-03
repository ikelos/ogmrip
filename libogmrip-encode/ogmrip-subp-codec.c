/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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
  gboolean forced_only;

  gchar *label;
  guint language;

  OGMRipCharset charset;
  OGMRipNewline newline;
};

enum 
{
  PROP_0,
  PROP_FORCED_ONLY,
  PROP_CHARSET,
  PROP_NEWLINE,
  PROP_LABEL,
  PROP_LANGUAGE
};

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

  gobject_class->finalize = ogmrip_subp_codec_finalize;
  gobject_class->set_property = ogmrip_subp_codec_set_property;
  gobject_class->get_property = ogmrip_subp_codec_get_property;

  g_object_class_install_property (gobject_class, PROP_FORCED_ONLY, 
        g_param_spec_boolean ("forced-only", "Forced only property", "Set forced only", 
           FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CHARSET,
      g_param_spec_int ("charset", "Charset property", "Set charset",
        OGMRIP_CHARSET_UNDEFINED, OGMRIP_CHARSET_ASCII, OGMRIP_CHARSET_UNDEFINED,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NEWLINE,
      g_param_spec_int ("newline", "Newline property", "Set newline",
        OGMRIP_NEWLINE_UNDEFINED, OGMRIP_NEWLINE_CR_LF, OGMRIP_NEWLINE_UNDEFINED,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LABEL, 
        g_param_spec_string ("label", "Label property", "Set label", 
           NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LANGUAGE, 
        g_param_spec_uint ("language", "Language property", "Set language", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipSubpCodecPriv));
}

static void
ogmrip_subp_codec_init (OGMRipSubpCodec *subp)
{
  subp->priv = OGMRIP_SUBP_GET_PRIVATE (subp);
}

static void
ogmrip_subp_codec_finalize (GObject *gobject)
{
  OGMRipSubpCodec *subp = OGMRIP_SUBP_CODEC (gobject);

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
  OGMRipSubpCodec *subp = OGMRIP_SUBP_CODEC (gobject);

  switch (property_id) 
  {
    case PROP_FORCED_ONLY: 
      subp->priv->forced_only = g_value_get_boolean (value);
      break;
    case PROP_CHARSET:
      subp->priv->charset = g_value_get_int (value);
      break;
    case PROP_NEWLINE: 
      subp->priv->newline = g_value_get_int (value);
      break;
    case PROP_LABEL: 
      g_free (subp->priv->label);
      subp->priv->label = g_value_dup_string (value);
      break;
    case PROP_LANGUAGE: 
      subp->priv->language = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_subp_codec_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipSubpCodec *subp = OGMRIP_SUBP_CODEC (gobject);

  switch (property_id) 
  {
    case PROP_FORCED_ONLY: 
      g_value_set_boolean (value, subp->priv->forced_only);
      break;
    case PROP_CHARSET:
      g_value_set_uint (value, subp->priv->charset);
      break;
    case PROP_NEWLINE:
      g_value_set_uint (value, subp->priv->newline);
      break;
    case PROP_LABEL:
      g_value_set_string (value, subp->priv->label);
      break;
    case PROP_LANGUAGE:
      g_value_set_uint (value, subp->priv->language);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
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

  g_object_notify (G_OBJECT (subp), "forced-only");
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

  g_object_notify (G_OBJECT (subp), "charset");
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

  g_object_notify (G_OBJECT (subp), "newline");
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

  g_object_notify (G_OBJECT (subp), "label");
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

/**
 * ogmrip_subp_codec_set_language:
 * @subp: an #OGMRipSubpCodec
 * @language: the language
 *
 * Sets the language of the track.
 */
void
ogmrip_subp_codec_set_language (OGMRipSubpCodec *subp, guint language)
{
  g_return_if_fail (OGMRIP_IS_SUBP_CODEC (subp));

  subp->priv->language = language;

  g_object_notify (G_OBJECT (subp), "language");
}

/**
 * ogmrip_subp_codec_get_language:
 * @subp: an #OGMRipSubpCodec
 *
 * Gets the language of the track.
 *
 * Returns: the language
 */
gint
ogmrip_subp_codec_get_language (OGMRipSubpCodec *subp)
{
  g_return_val_if_fail (OGMRIP_IS_SUBP_CODEC (subp), -1);

  return subp->priv->language;
}

