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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-profile.h"
#include "ogmrip-profile-engine.h"
#include "ogmrip-profile-keys.h"
#include "ogmrip-profile-parser.h"
#include "ogmrip-marshal.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>

#define OGMRIP_PROFILE_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_PROFILE, OGMRipProfilePriv))

struct _OGMRipProfilePriv
{
  GSettings *general_settings;
  gchar *container;

  GSettings *video_settings;
  gchar *video_codec;

  GSettings *audio_settings;
  gchar *audio_codec;

  GSettings *subp_settings;
  gchar *subp_codec;

  gchar *name, *path;
};

enum
{
  PROP_0,
  PROP_NAME
};

static void ogmrip_profile_constructed  (GObject      *gobject);
static void ogmrip_profile_dispose      (GObject      *gobject);
static void ogmrip_profile_finalize     (GObject      *gobject);
static void ogmrip_profile_get_property (GObject      *gobject,
                                         guint        property_id,
                                         GValue       *value,
                                         GParamSpec   *pspec);

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

static void
ogmrip_profile_codec_changed (GSettings *settings, const gchar *key, gchar **name)
{
  g_free (*name);
  *name = g_settings_get_string (settings, key);
}

G_DEFINE_TYPE (OGMRipProfile, ogmrip_profile, G_TYPE_SETTINGS)

static void
ogmrip_profile_class_init (OGMRipProfileClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_profile_constructed;
  gobject_class->dispose = ogmrip_profile_dispose;
  gobject_class->finalize = ogmrip_profile_finalize;
  gobject_class->get_property = ogmrip_profile_get_property;

  g_object_class_install_property (gobject_class, PROP_NAME,
      g_param_spec_string ("name", "Name property", "Get name",
        NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipProfilePriv));
}

static void
ogmrip_profile_init (OGMRipProfile *profile)
{
  profile->priv = OGMRIP_PROFILE_GET_PRIVATE (profile);
}

static void
ogmrip_profile_constructed (GObject *gobject)
{
  OGMRipProfile *profile = OGMRIP_PROFILE (gobject);
  gchar *str;

  G_OBJECT_CLASS (ogmrip_profile_parent_class)->constructed (gobject);

  profile->priv->general_settings = g_settings_get_child (G_SETTINGS (profile), OGMRIP_PROFILE_GENERAL);
  profile->priv->container = g_settings_get_string (profile->priv->general_settings, OGMRIP_PROFILE_CONTAINER);

  g_signal_connect (profile->priv->general_settings, "changed::" OGMRIP_PROFILE_CONTAINER,
      G_CALLBACK (ogmrip_profile_codec_changed), &profile->priv->container);

  profile->priv->video_settings = g_settings_get_child (G_SETTINGS (profile), OGMRIP_PROFILE_VIDEO);
  profile->priv->video_codec = g_settings_get_string (profile->priv->video_settings, OGMRIP_PROFILE_CODEC);

  g_signal_connect (profile->priv->video_settings, "changed::" OGMRIP_PROFILE_CODEC,
      G_CALLBACK (ogmrip_profile_codec_changed), &profile->priv->video_codec);

  profile->priv->audio_settings = g_settings_get_child (G_SETTINGS (profile), OGMRIP_PROFILE_AUDIO);
  profile->priv->audio_codec = g_settings_get_string (profile->priv->audio_settings, OGMRIP_PROFILE_CODEC);

  g_signal_connect (profile->priv->audio_settings, "changed::" OGMRIP_PROFILE_CODEC,
      G_CALLBACK (ogmrip_profile_codec_changed), &profile->priv->audio_codec);

  profile->priv->subp_settings = g_settings_get_child (G_SETTINGS (profile), OGMRIP_PROFILE_SUBP);
  profile->priv->subp_codec = g_settings_get_string (profile->priv->subp_settings, OGMRIP_PROFILE_CODEC);

  g_signal_connect (profile->priv->subp_settings, "changed::" OGMRIP_PROFILE_CODEC,
      G_CALLBACK (ogmrip_profile_codec_changed), &profile->priv->subp_codec);

  g_object_get (profile, "path", &profile->priv->path, NULL);

  str = g_path_get_dirname (profile->priv->path);
  profile->priv->name = g_path_get_basename (str);
  g_free (str);
}

static void
ogmrip_profile_dispose (GObject *gobject)
{
  OGMRipProfile *profile = OGMRIP_PROFILE (gobject);

  if (profile->priv->general_settings)
  {
    g_object_unref (profile->priv->general_settings);
    profile->priv->general_settings = NULL;
  }

  if (profile->priv->video_settings)
  {
    g_object_unref (profile->priv->video_settings);
    profile->priv->video_settings = NULL;
  }

  if (profile->priv->audio_settings)
  {
    g_object_unref (profile->priv->audio_settings);
    profile->priv->audio_settings = NULL;
  }

  if (profile->priv->subp_settings)
  {
    g_object_unref (profile->priv->subp_settings);
    profile->priv->subp_settings = NULL;
  }

  G_OBJECT_CLASS (ogmrip_profile_parent_class)->dispose (gobject);
}

static void
ogmrip_profile_finalize (GObject *gobject)
{
  g_free (OGMRIP_PROFILE (gobject)->priv->name);
  g_free (OGMRIP_PROFILE (gobject)->priv->path);

  G_OBJECT_CLASS (ogmrip_profile_parent_class)->finalize (gobject);
}

static void
ogmrip_profile_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_NAME:
      g_value_set_string (value, OGMRIP_PROFILE (gobject)->priv->name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
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
    return;

  keys = g_settings_list_keys (src);
  for (i = 0; keys[i]; i ++)
  {
    value = g_settings_get_value (src, keys[i]);
    g_settings_set_value (dst, keys[i], value);
    g_variant_unref (value);
    g_free (keys[i]);
  }
  g_free (keys);
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
  if (profile == NULL && error != NULL && *error == NULL)
  {
    gchar *filename;

    filename = g_file_get_basename (file);
    g_set_error (error, 0, 0, _("'%s' does not contain a valid encoding"), filename);
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
ogmrip_profile_get (OGMRipProfile *profile, const gchar *section, const gchar *key, const gchar *format, ...)
{
  GSettings *settings;
  GVariant *value;
  va_list ap;

  g_return_if_fail (OGMRIP_IS_PROFILE (profile));

  settings = ogmrip_profile_get_child (profile, section);
  g_assert (settings != NULL);

  value = g_settings_get_value (settings, key);

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
  va_list ap;

  g_return_val_if_fail (OGMRIP_IS_PROFILE (profile), FALSE);

  va_start (ap, format);
  value = g_variant_new_va (format, NULL, &ap);
  va_end (ap);

  settings = ogmrip_profile_get_child (profile, section);
  g_assert (settings != NULL);

  return g_settings_set_value (settings, key, value);
}

static GSettings *
ogmrip_profile_get_child_for_codec (OGMRipProfile *profile, const gchar *name)
{
  GSettings *settings = NULL;
  gchar *schema;

  schema = g_strconcat ("org.ogmrip.", name, NULL);
  if (ogmrip_profile_has_schema (schema))
  {
    gchar *path;

    path = g_strconcat (profile->priv->path, name, "/", NULL);
    settings = g_settings_new_with_path (schema, path);
    g_free (path);
  }
  g_free (schema);

  return settings;
}

GSettings *
ogmrip_profile_get_child (OGMRipProfile *profile, const gchar *name)
{
  if (g_str_equal (name, OGMRIP_PROFILE_GENERAL))
    return profile->priv->general_settings;

  if (g_str_equal (name, OGMRIP_PROFILE_VIDEO))
    return profile->priv->video_settings;

  if (g_str_equal (name, OGMRIP_PROFILE_AUDIO))
    return profile->priv->audio_settings;

  if (g_str_equal (name, OGMRIP_PROFILE_SUBP))
    return profile->priv->subp_settings;

  if (g_str_equal (name, profile->priv->container))
    return ogmrip_profile_get_child_for_codec (profile, name);

  if (g_str_equal (name, profile->priv->video_codec))
    return ogmrip_profile_get_child_for_codec (profile, name);

  if (g_str_equal (name, profile->priv->audio_codec))
    return ogmrip_profile_get_child_for_codec (profile, name);

  if (g_str_equal (name, profile->priv->subp_codec))
    return ogmrip_profile_get_child_for_codec (profile, name);

  return NULL;
}

