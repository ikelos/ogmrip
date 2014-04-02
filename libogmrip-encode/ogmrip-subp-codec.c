/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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
#include "ogmrip-profile-keys.h"
#include "ogmrip-container.h"

#include <ogmrip-base.h>

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

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (OGMRipSubpCodec, ogmrip_subp_codec, OGMRIP_TYPE_CODEC)

static void
ogmrip_subp_codec_constructed (GObject *gobject)
{
  OGMRipStream *stream;

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (gobject));
  if (!OGMRIP_IS_SUBP_STREAM (stream))
    g_error ("No subp stream specified");

  G_OBJECT_CLASS (ogmrip_subp_codec_parent_class)->constructed (gobject);
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

static void
ogmrip_subp_codec_class_init (OGMRipSubpCodecClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = ogmrip_subp_codec_constructed;
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
}

static void
ogmrip_subp_codec_init (OGMRipSubpCodec *subp)
{
  subp->priv = ogmrip_subp_codec_get_instance_private (subp);
}

GType
ogmrip_subp_codec_get_default (GType container)
{
  GType *types;
  guint i;

  g_return_val_if_fail (g_type_is_a (container, OGMRIP_TYPE_CONTAINER), G_TYPE_NONE);

  types = ogmrip_type_children (OGMRIP_TYPE_SUBP_CODEC, NULL);
  for (i = 0; types[i] != G_TYPE_NONE; i ++)
    if (ogmrip_container_contains (container, types[i]))
      return types[i];

  return G_TYPE_NONE;
}

OGMRipCodec *
ogmrip_subp_codec_new (GType type, OGMRipSubpStream *stream)
{
  g_return_val_if_fail (g_type_is_a (type, OGMRIP_TYPE_SUBP_CODEC), NULL);
  g_return_val_if_fail (OGMRIP_IS_SUBP_STREAM (stream), NULL);

  return g_object_new (type, "input", stream, NULL);
}

OGMRipCodec *
ogmrip_subp_codec_new_from_profile (OGMRipSubpStream *stream, OGMRipProfile *profile)
{
  OGMRipCodec *codec;
  GSettings *settings;
  GType type;
  gchar *name;

  g_return_val_if_fail (OGMRIP_IS_PROFILE (profile), NULL);
  g_return_val_if_fail (OGMRIP_IS_SUBP_STREAM (stream), NULL);

  ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_CODEC, "s", &name);
  type = ogmrip_type_from_name (name);
  g_free (name);
  
  if (type == G_TYPE_NONE)
    return NULL;

  codec = ogmrip_subp_codec_new (type, stream);

  settings = ogmrip_profile_get_child (profile, OGMRIP_PROFILE_SUBP);

  ogmrip_subp_codec_set_charset (OGMRIP_SUBP_CODEC (codec),
      g_settings_get_uint (settings, OGMRIP_PROFILE_CHARACTER_SET));
  ogmrip_subp_codec_set_newline (OGMRIP_SUBP_CODEC (codec),
      g_settings_get_uint (settings, OGMRIP_PROFILE_NEWLINE_STYLE));
  ogmrip_subp_codec_set_forced_only (OGMRIP_SUBP_CODEC (codec),
      g_settings_get_boolean (settings, OGMRIP_PROFILE_FORCED_ONLY));

  g_object_unref (settings);

  return codec;
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
const gchar *
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

