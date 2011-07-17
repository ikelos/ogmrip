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

#include <string.h>

#define OGMRIP_PROFILE_ENGINE_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_PROFILE_ENGINE, OGMRipProfileEnginePriv))

struct _OGMRipProfileEnginePriv
{
  GHashTable *profiles;
};

enum
{
  PROP_0,
  PROP_PROFILES
};

static OGMRipProfileEngine *default_engine = NULL;

static void
ogmrip_profile_add_name (const gchar *key, OGMRipProfile *profile, GArray *array)
{
  gchar *name;

  g_object_get (profile, "name", &name, NULL);
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
ogmrip_profile_engine_finalize (GObject *gobject)
{
  OGMRipProfileEngine *engine = OGMRIP_PROFILE_ENGINE (gobject);

  g_hash_table_destroy (engine->priv->profiles);

  G_OBJECT_CLASS (ogmrip_profile_engine_parent_class)->finalize (gobject);
}

static void
ogmrip_profile_engine_init (OGMRipProfileEngine *engine)
{
  engine->priv = OGMRIP_PROFILE_ENGINE_GET_PRIVATE (engine);

  engine->priv->profiles = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
}

static void
ogmrip_profile_engine_class_init (OGMRipProfileEngineClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_profile_engine_get_property;
  gobject_class->set_property = ogmrip_profile_engine_set_property;
  gobject_class->finalize = ogmrip_profile_engine_finalize;

  g_object_class_install_property (gobject_class, PROP_PROFILES,
      g_param_spec_boxed ("profiles", "profiles", "profiles",
        G_TYPE_STRV, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipProfileEnginePriv));
}

OGMRipProfileEngine *
ogmrip_profile_engine_get_default (void)
{
  if (!default_engine)
    default_engine = g_object_new (OGMRIP_TYPE_PROFILE_ENGINE, NULL);

  return default_engine;
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
  gchar *name;

  g_return_if_fail (OGMRIP_IS_PROFILE_ENGINE (engine));
  g_return_if_fail (OGMRIP_IS_PROFILE (profile));

  g_object_get (profile, "name", &name, NULL);
  g_hash_table_insert (engine->priv->profiles, name, g_object_ref (profile));
}

void
ogmrip_profile_engine_remove (OGMRipProfileEngine *engine, OGMRipProfile *profile)
{
  gchar *name;

  g_return_if_fail (OGMRIP_IS_PROFILE_ENGINE (engine));
  g_return_if_fail (OGMRIP_IS_PROFILE (profile));

  g_object_get (profile, "name", &name, NULL);
  g_hash_table_remove (engine->priv->profiles, name);
  g_free (name);
}

GSList *
ogmrip_profile_engine_get_list (OGMRipProfileEngine *engine)
{
  GSList *list = NULL;

  g_return_val_if_fail (OGMRIP_IS_PROFILE_ENGINE (engine), NULL);

  g_hash_table_foreach (engine->priv->profiles, (GHFunc) ogmrip_profile_add_profile, &list);

  return list;
}

