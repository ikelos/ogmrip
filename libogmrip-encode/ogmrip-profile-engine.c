/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-profile-engine.h"
#include "ogmrip-profile-keys.h"
#include "ogmrip-plugin.h"
#include "ogmrip-xml.h"

#include <string.h>

#define OGMRIP_PROFILE_ENGINE_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_PROFILE_ENGINE, OGMRipProfileEnginePriv))

struct _OGMRipProfileEnginePriv
{
  GSettings *settings;
  GHashTable *profiles;
  GList *paths;
};

enum
{
  PROP_0,
  PROP_PROFILES
};

enum
{
  ADD,
  REMOVE,
  LAST_SIGNAL
};

static OGMRipProfileEngine *default_engine = NULL;

static guint signals[LAST_SIGNAL] = { 0 };

static void
ogmrip_profile_add_name (const gchar *key, OGMRipProfile *profile, GArray *array)
{
  gchar *name;

  name = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME);
  g_array_append_val (array, name);
}

static void
ogmrip_profile_add_profile (const gchar *key, OGMRipProfile *profile, GSList **list)
{
  *list = g_slist_prepend (*list, profile);
}

static gchar **
ogmrip_profile_engine_get_profiles (OGMRipProfileEngine *engine)
{
  GArray *array;

  array = g_array_new (TRUE, FALSE, sizeof (gchar *));
  g_hash_table_foreach (engine->priv->profiles, (GHFunc) ogmrip_profile_add_name, array);

  return (gchar **) g_array_free (array, FALSE);
}

static void
ogmrip_profile_engine_set_profiles (OGMRipProfileEngine *engine, const gchar **profiles)
{
  OGMRipProfile *profile;
  guint i;

  for (i = 0; profiles[i]; i ++)
  {
    profile = ogmrip_profile_new (profiles[i]);
    ogmrip_profile_engine_add (engine, profile);
    g_object_unref (profile);
  }
}

static gboolean
compare_by_name (const gchar *key, OGMRipProfile *value, const gchar *name)
{
  return g_str_equal (key, name);
}

static gboolean
ogmrip_profile_engine_check (OGMRipProfileEngine *engine, OGMRipProfile *profile, const gchar *name)
{
  GType container, codec;
  gchar *str;

  if (g_hash_table_find (engine->priv->profiles, (GHRFunc) compare_by_name, (gpointer) name))
    return FALSE;

  ogmrip_profile_get (profile, OGMRIP_PROFILE_GENERAL, OGMRIP_PROFILE_CONTAINER, "s", &str);
  container = ogmrip_plugin_get_container_by_name (str);
  g_free (str);

  if (container == G_TYPE_NONE)
    return FALSE;

  ogmrip_profile_get (profile, OGMRIP_PROFILE_VIDEO, OGMRIP_PROFILE_CODEC, "s", &str);
  codec = ogmrip_plugin_get_video_codec_by_name (str);
  g_free (str);

  if (codec == G_TYPE_NONE || !ogmrip_plugin_can_contain_video (container, codec))
    return FALSE;

  ogmrip_profile_get (profile, OGMRIP_PROFILE_AUDIO, OGMRIP_PROFILE_CODEC, "s", &str);
  codec = ogmrip_plugin_get_audio_codec_by_name (str);
  g_free (str);

  if (codec != G_TYPE_NONE && !ogmrip_plugin_can_contain_audio (container, codec))
    return FALSE;

  ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_CODEC, "s", &str);
  codec = ogmrip_plugin_get_subp_codec_by_name (str);
  g_free (str);

  if (codec != G_TYPE_NONE && !ogmrip_plugin_can_contain_subp (container, codec))
    return FALSE;

  return TRUE;
}

G_DEFINE_TYPE (OGMRipProfileEngine, ogmrip_profile_engine, G_TYPE_OBJECT)

static void
ogmrip_profile_engine_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipProfileEngine *engine = OGMRIP_PROFILE_ENGINE (gobject);

  switch (property_id) 
  {
    case PROP_PROFILES:
      g_value_take_boxed (value, (gconstpointer) ogmrip_profile_engine_get_profiles (engine));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_profile_engine_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipProfileEngine *engine = OGMRIP_PROFILE_ENGINE (gobject);

  switch (property_id) 
  {
    case PROP_PROFILES:
      ogmrip_profile_engine_set_profiles (engine, (const gchar **) g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_profile_engine_dispose (GObject *gobject)
{
  OGMRipProfileEngine *engine = OGMRIP_PROFILE_ENGINE (gobject);

  if (engine->priv->settings)
  {
    g_object_unref (engine->priv->settings);
    engine->priv->settings= NULL;
  }

  if (engine->priv->profiles)
  {
    g_hash_table_destroy (engine->priv->profiles);
    engine->priv->profiles = NULL;
  }

  G_OBJECT_CLASS (ogmrip_profile_engine_parent_class)->dispose (gobject);
}

static void
ogmrip_profile_engine_finalize (GObject *gobject)
{
  OGMRipProfileEngine *engine = OGMRIP_PROFILE_ENGINE (gobject);

  if (engine->priv->paths)
  {
    g_list_foreach (engine->priv->paths, (GFunc) g_free, NULL);
    g_list_free (engine->priv->paths);
    engine->priv->paths = NULL;
  }

  G_OBJECT_CLASS (ogmrip_profile_engine_parent_class)->finalize (gobject);
}

static void
ogmrip_profile_engine_add_internal (OGMRipProfileEngine *engine, OGMRipProfile *profile)
{
  gchar *name;

  g_object_get (profile, "name", &name, NULL);
  if (ogmrip_profile_engine_check (engine, profile, name))
  {
    g_hash_table_insert (engine->priv->profiles, g_strdup (name), g_object_ref (profile));
    g_object_notify (G_OBJECT (engine), "profiles");
  }
  g_free (name);
}

static void
ogmrip_profile_engine_remove_internal (OGMRipProfileEngine *engine, OGMRipProfile *profile)
{
  gchar *name;

  name = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME);
  if (g_hash_table_remove (engine->priv->profiles, name))
    g_object_notify (G_OBJECT (engine), "profiles");
  g_free (name);
}

static void
ogmrip_profile_engine_init (OGMRipProfileEngine *engine)
{
  engine->priv = OGMRIP_PROFILE_ENGINE_GET_PRIVATE (engine);

  engine->priv->settings = g_settings_new_with_path ("org.ogmrip.profiles", "/apps/ogmrip/preferences/");
  engine->priv->profiles = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
}

static void
ogmrip_profile_engine_class_init (OGMRipProfileEngineClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_profile_engine_get_property;
  gobject_class->set_property = ogmrip_profile_engine_set_property;
  gobject_class->dispose = ogmrip_profile_engine_dispose;
  gobject_class->finalize = ogmrip_profile_engine_finalize;

  klass->add = ogmrip_profile_engine_add_internal;
  klass->remove = ogmrip_profile_engine_remove_internal;

  g_object_class_install_property (gobject_class, PROP_PROFILES,
      g_param_spec_boxed ("profiles", "profiles", "profiles",
        G_TYPE_STRV, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  signals[ADD] = g_signal_new ("add", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipProfileEngineClass, add), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, OGMRIP_TYPE_PROFILE);

  signals[REMOVE] = g_signal_new ("remove", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipProfileEngineClass, remove), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, OGMRIP_TYPE_PROFILE);

  g_type_class_add_private (klass, sizeof (OGMRipProfileEnginePriv));
}

OGMRipProfileEngine *
ogmrip_profile_engine_get_default (void)
{
  if (!default_engine)
  {
    default_engine = g_object_new (OGMRIP_TYPE_PROFILE_ENGINE, NULL);
    g_settings_bind (default_engine->priv->settings, "profiles",
        default_engine, "profiles", G_SETTINGS_BIND_DEFAULT);
  }

  return default_engine;
}

static void
ogmrip_profile_engine_load_file (OGMRipProfileEngine *engine, const gchar *filename)
{
  GFile *file;
  OGMRipXML *xml;

  file = g_file_new_for_path (filename);

  xml = ogmrip_xml_new_from_file (file, NULL);
  if (xml)
  {
    gchar *name;

    name = ogmrip_xml_get_string (xml, "name");
    if (name)
    {
      OGMRipProfile *profile;

      profile = ogmrip_profile_engine_get (engine, name);
      if (profile)
      {
/*
        GVariant *variant;
        guint major1, minor1, major2, minor2;

        g_settings_get (G_SETTINGS (profile), "version", "(uu)", &major1, &minor1);

        variant = ogmrip_xml_get_variant (xml, "version", "(uu)");
        g_variant_get (variant, "(uu)", &major2, &minor2);
        g_variant_unref (variant);

        if (major2 > major1 || (major2 == major1 && minor2 > minor1))
        {
        }
*/
      }
      else
      {
        profile = ogmrip_profile_new_from_file (file, NULL);
        ogmrip_profile_engine_add (engine, profile);
        g_object_unref (profile);
      }
      g_free (name);
    }

    ogmrip_xml_free (xml);
  }

  g_object_unref (file);
}

static void
ogmrip_profile_engine_load_dir (OGMRipProfileEngine *engine, const gchar *path)
{
  GDir *dir;

  dir = g_dir_open (path, 0, NULL);
  if (dir)
  {
    const gchar *basename;
    gchar *filename;

    while ((basename = g_dir_read_name (dir)))
    {
      filename = g_build_filename (path, basename, NULL);
      ogmrip_profile_engine_load_file (engine, filename);
      g_free (filename);
    }

    g_dir_close (dir);
  }
}

void
ogmrip_profile_engine_rescan (OGMRipProfileEngine *engine)
{
  GList *link;

  g_return_if_fail (OGMRIP_IS_PROFILE_ENGINE (engine));

  for (link = engine->priv->paths; link; link = link->next)
    ogmrip_profile_engine_load_dir (engine, link->data);
}

void
ogmrip_profile_engine_add_path (OGMRipProfileEngine *engine, const gchar *path)
{
  g_return_if_fail (OGMRIP_IS_PROFILE_ENGINE (engine));
  g_return_if_fail (path != NULL); 

  engine->priv->paths = g_list_append (engine->priv->paths, g_strdup  (path));

  ogmrip_profile_engine_load_dir (engine, path);
}

OGMRipProfile *
ogmrip_profile_engine_get (OGMRipProfileEngine *engine, const gchar *name)
{
  g_return_val_if_fail (OGMRIP_IS_PROFILE_ENGINE (engine), NULL);

  return g_hash_table_lookup (engine->priv->profiles, name);
}

void
ogmrip_profile_engine_add (OGMRipProfileEngine *engine, OGMRipProfile *profile)
{
  g_return_if_fail (OGMRIP_IS_PROFILE_ENGINE (engine));
  g_return_if_fail (OGMRIP_IS_PROFILE (profile));

  g_signal_emit (engine, signals[ADD], 0, profile);
}

void
ogmrip_profile_engine_remove (OGMRipProfileEngine *engine, OGMRipProfile *profile)
{
  g_return_if_fail (OGMRIP_IS_PROFILE_ENGINE (engine));
  g_return_if_fail (OGMRIP_IS_PROFILE (profile));

  g_signal_emit (engine, signals[REMOVE], 0, profile);
}

GSList *
ogmrip_profile_engine_get_list (OGMRipProfileEngine *engine)
{
  GSList *list = NULL;

  g_return_val_if_fail (OGMRIP_IS_PROFILE_ENGINE (engine), NULL);

  g_hash_table_foreach (engine->priv->profiles, (GHFunc) ogmrip_profile_add_profile, &list);

  return list;
}

