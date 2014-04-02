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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-profile.h"
#include "ogmrip-profile-engine.h"
#include "ogmrip-profile-keys.h"
#include "ogmrip-profile-parser.h"
#include "ogmrip-marshal.h"
#include "ogmrip-video-codec.h"
#include "ogmrip-audio-codec.h"
#include "ogmrip-subp-codec.h"
#include "ogmrip-container.h"
#include "ogmrip-hardsub.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>

struct _OGMRipProfilePriv
{
  gboolean is_valid;

  GSettings *general_settings;
  GSettings *video_settings;
  GSettings *audio_settings;
  GSettings *subp_settings;

  gchar *name, *path;
  GFile *file;
};

enum
{
  PROP_0,
  PROP_NAME,
  PROP_FILE,
  PROP_VALID
};

GQuark
ogmrip_profile_error_quark (void)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_static_string ("ogmrip-profile-error-quark");

  return quark;
}

static gboolean
ogmrip_profile_has_schema (const gchar *schema)
{
  const gchar * const *schemas;
  guint i;

#if GLIB_CHECK_VERSION(2, 28, 0)
  schemas = g_settings_list_relocatable_schemas ();
#else
  schemas = g_settings_list_schemas ();
#endif

  for (i = 0; schemas[i]; i ++)
    if (g_str_equal (schemas[i], schema))
      return TRUE;

  return FALSE;
}

static GType
ogmrip_profile_get_container (OGMRipProfile *profile)
{
  GType gtype = G_TYPE_NONE;
  gchar *name;

  name = g_settings_get_string (profile->priv->general_settings, OGMRIP_PROFILE_CONTAINER);
  if (name && *name)
    gtype = ogmrip_type_from_name (name);
  g_free (name);

  return gtype;
}

static GType
ogmrip_profile_get_video_codec (OGMRipProfile *profile)
{
  GType gtype = G_TYPE_NONE;
  gchar *name;

  name = g_settings_get_string (profile->priv->video_settings, OGMRIP_PROFILE_CODEC);
  if (name && *name)
    gtype = ogmrip_type_from_name (name);
  g_free (name);

  return gtype;
}

static GType
ogmrip_profile_get_audio_codec (OGMRipProfile *profile)
{
  GType gtype = G_TYPE_NONE;
  gchar *name;

  name = g_settings_get_string (profile->priv->audio_settings, OGMRIP_PROFILE_CODEC);
  if (name && *name)
    gtype = ogmrip_type_from_name (name);
  g_free (name);

  return gtype;
}

static GType
ogmrip_profile_get_subp_codec (OGMRipProfile *profile)
{
  GType gtype = G_TYPE_NONE;
  gchar *name;

  name = g_settings_get_string (profile->priv->subp_settings, OGMRIP_PROFILE_CODEC);
  if (name && *name)
    gtype = ogmrip_type_from_name (name);
  g_free (name);

  return gtype;
}

static void
ogmrip_profile_check (OGMRipProfile *profile)
{
  GType container, codec;
  gboolean is_valid;

  container = ogmrip_profile_get_container (profile);
  is_valid = container != G_TYPE_NONE;

  if (is_valid)
  {
    codec = ogmrip_profile_get_video_codec (profile);
    is_valid = codec != G_TYPE_NONE && ogmrip_container_contains (container, ogmrip_codec_format (codec));
  }

  if (is_valid)
  {
    codec = ogmrip_profile_get_audio_codec (profile);
    is_valid = codec != G_TYPE_NONE && ogmrip_container_contains (container, ogmrip_codec_format (codec));
  }

  if (is_valid)
  {
    codec = ogmrip_profile_get_subp_codec (profile);
    is_valid = codec != G_TYPE_NONE && (codec == OGMRIP_TYPE_HARDSUB || ogmrip_container_contains(container, ogmrip_codec_format (codec)));
  }

  if (profile->priv->is_valid != is_valid)
  {
    profile->priv->is_valid = is_valid;
    g_object_notify (G_OBJECT (profile), "validity");
  }
}

G_DEFINE_TYPE_WITH_PRIVATE (OGMRipProfile, ogmrip_profile, G_TYPE_SETTINGS)

static void
ogmrip_profile_constructed (GObject *gobject)
{
  OGMRipProfile *profile = OGMRIP_PROFILE (gobject);
  GType container, codec;
  gchar *str;

  G_OBJECT_CLASS (ogmrip_profile_parent_class)->constructed (gobject);

  profile->priv->general_settings = g_settings_get_child (G_SETTINGS (profile), OGMRIP_PROFILE_GENERAL);

  container = ogmrip_profile_get_container (profile);
  if (container == G_TYPE_NONE)
  {
    container = ogmrip_container_get_default ();
    if (container != G_TYPE_NONE)
      g_settings_set_string (profile->priv->general_settings, OGMRIP_PROFILE_CONTAINER, ogmrip_type_name (container));
  }

  g_signal_connect_swapped (profile->priv->general_settings, "changed::" OGMRIP_PROFILE_CONTAINER,
      G_CALLBACK (ogmrip_profile_check), profile);

  profile->priv->video_settings = g_settings_get_child (G_SETTINGS (profile), OGMRIP_PROFILE_VIDEO);

  codec = ogmrip_profile_get_video_codec (profile);
  if (codec == G_TYPE_NONE && container != G_TYPE_NONE)
  {
    codec = ogmrip_video_codec_get_default (container);
    if (codec != G_TYPE_NONE)
      g_settings_set_string (profile->priv->video_settings, OGMRIP_PROFILE_CODEC, ogmrip_type_name (codec));
  }

  g_signal_connect_swapped (profile->priv->video_settings, "changed::" OGMRIP_PROFILE_CODEC,
      G_CALLBACK (ogmrip_profile_check), profile);

  profile->priv->audio_settings = g_settings_get_child (G_SETTINGS (profile), OGMRIP_PROFILE_AUDIO);

  codec = ogmrip_profile_get_audio_codec (profile);
  if (codec == G_TYPE_NONE && container != G_TYPE_NONE)
  {
    codec = ogmrip_audio_codec_get_default (container);
    if (codec != G_TYPE_NONE)
      g_settings_set_string (profile->priv->audio_settings, OGMRIP_PROFILE_CODEC, ogmrip_type_name (codec));
  }

  g_signal_connect_swapped (profile->priv->audio_settings, "changed::" OGMRIP_PROFILE_CODEC,
      G_CALLBACK (ogmrip_profile_check), profile);

  profile->priv->subp_settings = g_settings_get_child (G_SETTINGS (profile), OGMRIP_PROFILE_SUBP);

  codec = ogmrip_profile_get_subp_codec (profile);
  if (codec == G_TYPE_NONE && container != G_TYPE_NONE)
  {
    codec = ogmrip_subp_codec_get_default (container);
    if (codec != G_TYPE_NONE)
      g_settings_set_string (profile->priv->subp_settings, OGMRIP_PROFILE_CODEC, ogmrip_type_name (codec));
  }

  g_signal_connect_swapped (profile->priv->subp_settings, "changed::" OGMRIP_PROFILE_CODEC,
      G_CALLBACK (ogmrip_profile_check), profile);

  g_object_get (profile, "path", &profile->priv->path, NULL);

  str = g_path_get_dirname (profile->priv->path);
  profile->priv->name = g_path_get_basename (str);
  g_free (str);

  ogmrip_profile_check (profile);
}

static void
ogmrip_profile_dispose (GObject *gobject)
{
  OGMRipProfile *profile = OGMRIP_PROFILE (gobject);

  if (profile->priv->general_settings)
  {
    g_signal_handlers_disconnect_by_func (profile->priv->general_settings,
        ogmrip_profile_check, profile);
    g_object_unref (profile->priv->general_settings);
    profile->priv->general_settings = NULL;
  }

  if (profile->priv->video_settings)
  {
    g_signal_handlers_disconnect_by_func (profile->priv->video_settings,
        ogmrip_profile_check, profile);
    g_object_unref (profile->priv->video_settings);
    profile->priv->video_settings = NULL;
  }

  if (profile->priv->audio_settings)
  {
    g_signal_handlers_disconnect_by_func (profile->priv->audio_settings,
        ogmrip_profile_check, profile);
    g_object_unref (profile->priv->audio_settings);
    profile->priv->audio_settings = NULL;
  }

  if (profile->priv->subp_settings)
  {
    g_signal_handlers_disconnect_by_func (profile->priv->subp_settings,
        ogmrip_profile_check, profile);
    g_object_unref (profile->priv->subp_settings);
    profile->priv->subp_settings = NULL;
  }

  if (profile->priv->file)
  {
    g_object_unref (profile->priv->file);
    profile->priv->file = NULL;
  }

  G_OBJECT_CLASS (ogmrip_profile_parent_class)->dispose (gobject);
}

static void
ogmrip_profile_finalize (GObject *gobject)
{
#ifdef G_ENABLE_DEBUG
  g_debug ("Finalizing profile '%s'", OGMRIP_PROFILE (gobject)->priv->name);
#endif

  g_free (OGMRIP_PROFILE (gobject)->priv->name);
  g_free (OGMRIP_PROFILE (gobject)->priv->path);

  G_OBJECT_CLASS (ogmrip_profile_parent_class)->finalize (gobject);
}

static void
ogmrip_profile_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipProfile *profile = OGMRIP_PROFILE (gobject);

  switch (property_id)
  {
    case PROP_NAME:
      g_value_set_string (value, profile->priv->name);
      break;
    case PROP_FILE:
      g_value_set_object (value, profile->priv->file);
      break;
    case PROP_VALID:
      g_value_set_boolean (value, profile->priv->is_valid);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_profile_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipProfile *profile = OGMRIP_PROFILE (gobject);

  switch (property_id)
  {
    case PROP_FILE:
      if (!profile->priv->file)
        profile->priv->file = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_profile_class_init (OGMRipProfileClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_profile_constructed;
  gobject_class->dispose = ogmrip_profile_dispose;
  gobject_class->finalize = ogmrip_profile_finalize;
  gobject_class->get_property = ogmrip_profile_get_property;
  gobject_class->set_property = ogmrip_profile_set_property;

  g_object_class_install_property (gobject_class, PROP_NAME,
      g_param_spec_string ("name", "Name property", "Get name",
        NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FILE,
      g_param_spec_object ("file", "File property", "Get file",
        G_TYPE_FILE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VALID,
      g_param_spec_boolean ("validity", "Validity property", "Get validity",
        FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_profile_init (OGMRipProfile *profile)
{
  profile->priv = ogmrip_profile_get_instance_private (profile);
}

static gchar *
ogmrip_profile_create_name (void)
{
  OGMRipProfileEngine *engine;
  gchar **strv;
  guint i, n, m = 0;

  engine = ogmrip_profile_engine_get_default ();

  g_object_get (engine, "profiles", &strv, NULL);

  for (i = 0; strv[i]; i ++)
    if (sscanf (strv[i], "profile-%u", &n))
      m = MAX (m, n) + 1;

  g_strfreev (strv);

  return g_strdup_printf ("profile-%u", m);
}

OGMRipProfile *
ogmrip_profile_new (const gchar *name)
{
  OGMRipProfile *profile;
  gchar *path;

  if (name)
    path = g_strconcat (OGMRIP_PROFILE_PATH, name, "/", NULL);
  else
  {
    gchar *str;

    str = ogmrip_profile_create_name ();
    path = g_strconcat (OGMRIP_PROFILE_PATH, str, "/", NULL);
    g_free (str);
  }

  profile = g_object_new (OGMRIP_TYPE_PROFILE, "schema", "org.ogmrip.profile", "path", path, NULL);
  g_free (path);

  return profile;
}

static void
ogmrip_profile_copy_section (OGMRipProfile *profile1, OGMRipProfile *profile2, const gchar *name)
{
  GSettings *src, *dst;
  GVariant *value;
  gchar **keys;
  guint i;

  src = ogmrip_profile_get_child (profile1, name);
  if (!src)
    return;

  dst = ogmrip_profile_get_child (profile2, name);
  if (!dst)
  {
    g_object_unref (src);
    return;
  }

  keys = g_settings_list_keys (src);
  for (i = 0; keys[i]; i ++)
  {
    value = g_settings_get_value (src, keys[i]);
    g_settings_set_value (dst, keys[i], value);
    g_variant_unref (value);
    g_free (keys[i]);
  }
  g_free (keys);

  g_object_unref (src);
  g_object_unref (dst);
}

OGMRipProfile *
ogmrip_profile_copy (OGMRipProfile *profile, gchar *name)
{
  OGMRipProfile *new_profile;
  gchar *str;

  new_profile = ogmrip_profile_new (name);
  if (!new_profile)
    return NULL;

  ogmrip_profile_copy_section (profile, new_profile, OGMRIP_PROFILE_GENERAL);

  str = g_settings_get_string (profile->priv->general_settings, OGMRIP_PROFILE_CONTAINER);
  ogmrip_profile_copy_section (profile, new_profile, str);
  g_free (str);

  ogmrip_profile_copy_section (profile, new_profile, OGMRIP_PROFILE_VIDEO);

  str = g_settings_get_string (profile->priv->video_settings, OGMRIP_PROFILE_CODEC);
  ogmrip_profile_copy_section (profile, new_profile, str);
  g_free (str);

  ogmrip_profile_copy_section (profile, new_profile, OGMRIP_PROFILE_AUDIO);

  str = g_settings_get_string (profile->priv->audio_settings, OGMRIP_PROFILE_CODEC);
  ogmrip_profile_copy_section (profile, new_profile, str);
  g_free (str);

  ogmrip_profile_copy_section (profile, new_profile, OGMRIP_PROFILE_SUBP);

  str = g_settings_get_string (profile->priv->subp_settings, OGMRIP_PROFILE_CODEC);
  ogmrip_profile_copy_section (profile, new_profile, str);
  g_free (str);

  return new_profile;
}

static OGMRipProfile *
ogmrip_profile_new_from_xml (OGMRipXML *xml, GError **error)
{
  OGMRipProfile *profile;
  gchar *str;

  if (!g_str_equal (ogmrip_xml_get_name (xml), "profile"))
    return NULL;

  str = ogmrip_xml_get_string (xml, "name");
  if (!str)
    return NULL;

  profile = ogmrip_profile_new (str);
  if (profile)
    ogmrip_profile_parse (profile, xml, error);

  g_free (str);

  return profile;
}


OGMRipProfile *
ogmrip_profile_new_from_file (GFile *file, GError **error)
{
  OGMRipXML *xml;
  OGMRipProfile *profile;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  xml = ogmrip_xml_new_from_file (file, error);
  if (!xml)
    return NULL;

  profile = ogmrip_profile_new_from_xml (xml, error);
  if (profile)
    g_object_set (profile, "file", file, NULL);
  else if (error != NULL && *error == NULL)
  {
    gchar *filename;

    filename = g_file_get_basename (file);
    g_set_error (error, OGMRIP_PROFILE_ERROR, OGMRIP_PROFILE_ERROR_INVALID,
        _("'%s' does not contain a valid profile"), filename);
    g_free (filename);
  }

  ogmrip_xml_free (xml);

  return profile;
}

gboolean
ogmrip_profile_export (OGMRipProfile *profile, GFile *file, GError **error)
{
  OGMRipXML *xml;
  gboolean retval;

  g_return_val_if_fail (OGMRIP_IS_PROFILE (profile), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  xml = ogmrip_xml_new ();

  retval = ogmrip_profile_dump (profile, xml, error) &&
    ogmrip_xml_save (xml, file, error);

  ogmrip_xml_free (xml);

  return retval;
}

void
ogmrip_profile_reset (OGMRipProfile *profile)
{
  g_return_if_fail (OGMRIP_IS_PROFILE (profile));

  if (profile->priv->file)
  {
    OGMRipXML *xml;

    xml = ogmrip_xml_new_from_file (profile->priv->file, NULL);
    if (xml)
    {
      ogmrip_profile_parse (profile, xml, NULL);
      ogmrip_xml_free (xml);
    }
  }
}

void
ogmrip_profile_get (OGMRipProfile *profile, const gchar *section, const gchar *key, const gchar *format, ...)
{
  GSettings *settings;
  GVariant *value;
  va_list ap;

  g_return_if_fail (OGMRIP_IS_PROFILE (profile));

  settings = ogmrip_profile_get_child (profile, section);
  g_assert (settings != NULL);

  value = g_settings_get_value (settings, key);
  g_object_unref (settings);

  va_start (ap, format);
  g_variant_get_va (value, format, NULL, &ap);
  va_end (ap);

  g_variant_unref (value);
}

gboolean
ogmrip_profile_set (OGMRipProfile *profile, const gchar *section, const gchar *key, const gchar *format, ...)
{
  GSettings *settings;
  GVariant *value;
  gboolean retval;
  va_list ap;

  g_return_val_if_fail (OGMRIP_IS_PROFILE (profile), FALSE);

  va_start (ap, format);
  value = g_variant_new_va (format, NULL, &ap);
  va_end (ap);

  settings = ogmrip_profile_get_child (profile, section);
  g_assert (settings != NULL);

  retval = g_settings_set_value (settings, key, value);
  g_object_unref (settings);

  return retval;
}

static GSettings *
ogmrip_profile_get_child_internal (OGMRipProfile *profile, const gchar *id, const gchar *name)
{
  GSettings *settings;
  gchar *path;

  if (!ogmrip_profile_has_schema (id))
    return NULL;

  path = g_strconcat (profile->priv->path, name, "/", NULL);
  settings = g_settings_new_with_path (id, path);
  g_free (path);

  return settings;
}

static GSettings *
ogmrip_profile_get_child_for_codec (OGMRipProfile *profile, const gchar *codec)
{
  const gchar *id, *name;

  if (!ogmrip_codec_get_schema (ogmrip_type_from_name (codec), &id, &name))
    return NULL;

  return ogmrip_profile_get_child_internal (profile, id, name);
}

static GSettings *
ogmrip_profile_get_child_for_container (OGMRipProfile *profile, const gchar *container)
{
  const gchar *id, *name;

  if (!ogmrip_container_get_schema (ogmrip_type_from_name (container), &id, &name))
    return NULL;

  return ogmrip_profile_get_child_internal (profile, id, name);
}

#define STRING_IS_DEFINED(str) ((str) != NULL && (*str) != '\0')

GSettings *
ogmrip_profile_get_child (OGMRipProfile *profile, const gchar *name)
{
  GType gtype;

  if (g_str_equal (name, OGMRIP_PROFILE_GENERAL))
    return g_object_ref (profile->priv->general_settings);

  if (g_str_equal (name, OGMRIP_PROFILE_VIDEO))
    return g_object_ref (profile->priv->video_settings);

  if (g_str_equal (name, OGMRIP_PROFILE_AUDIO))
    return g_object_ref (profile->priv->audio_settings);

  if (g_str_equal (name, OGMRIP_PROFILE_SUBP))
    return g_object_ref (profile->priv->subp_settings);

  gtype = ogmrip_type_from_name (name);
  if (gtype == G_TYPE_NONE)
    return NULL;

  if (ogmrip_profile_get_container (profile) == gtype)
    return ogmrip_profile_get_child_for_container (profile, name);

  if (ogmrip_profile_get_video_codec (profile) == gtype)
    return ogmrip_profile_get_child_for_codec (profile, name);

  if (ogmrip_profile_get_audio_codec (profile) == gtype)
    return ogmrip_profile_get_child_for_codec (profile, name);

  if (ogmrip_profile_get_subp_codec (profile) == gtype)
    return ogmrip_profile_get_child_for_codec (profile, name);

  return NULL;
}

gboolean
ogmrip_profile_is_valid (OGMRipProfile *profile)
{
  g_return_val_if_fail (OGMRIP_IS_PROFILE (profile), FALSE);

  return profile->priv->is_valid;
}

