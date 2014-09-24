/* OGMRipModule - A module library for OGMRip
 * Copyright (C) 2012-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-module-engine.h"

#include <string.h>
#include <gmodule.h>

struct _OGMRipModuleEnginePriv
{
  GSList *modules;
};

enum
{
  PROP_0,
  PROP_MODULES
};

static OGMRipModuleEngine *default_engine = NULL;

static gchar **
ogmrip_module_engine_get_modules (OGMRipModuleEngine *engine)
{
  GSList *link;
  GArray *array;
  gchar *name;

  array = g_array_new (TRUE, FALSE, sizeof (gchar *));
  for (link = engine->priv->modules; link; link = link->next)
  {
    name = g_strdup (ogmrip_module_get_name (link->data));
    g_array_append_val (array, name);
  }

  return (gchar **) g_array_free (array, FALSE);
}

G_DEFINE_TYPE_WITH_PRIVATE (OGMRipModuleEngine, ogmrip_module_engine, G_TYPE_OBJECT)

static GObject *
ogmrip_module_engine_constructor (GType type, guint n_params, GObjectConstructParam *params)
{
  GObject *gobject;

  if (!g_module_supported ())
    g_error ("ogmrip is not able to create the "
        "modules engine as modules are not supported.");

  gobject = G_OBJECT_CLASS (ogmrip_module_engine_parent_class)->constructor (type, n_params, params);

  if (!default_engine)
  {
    default_engine = OGMRIP_MODULE_ENGINE (gobject);
    g_object_add_weak_pointer (gobject, (gpointer *) &default_engine);
  }

  return gobject;
}

static void
ogmrip_module_engine_dispose (GObject *gobject)
{
  OGMRipModuleEngine *engine = OGMRIP_MODULE_ENGINE (gobject);

  if (engine->priv->modules)
  {
    g_slist_free_full (engine->priv->modules, (GDestroyNotify) g_type_module_unuse);
    engine->priv->modules = NULL;
  }

  G_OBJECT_CLASS (ogmrip_module_engine_parent_class)->dispose (gobject);
}

static void
ogmrip_module_engine_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipModuleEngine *engine = OGMRIP_MODULE_ENGINE (gobject);

  switch (property_id)
  {
    case PROP_MODULES:
      g_value_take_boxed (value, (gconstpointer) ogmrip_module_engine_get_modules (engine));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_module_engine_init (OGMRipModuleEngine *engine)
{
  engine->priv = ogmrip_module_engine_get_instance_private (engine);
}

static void
ogmrip_module_engine_class_init (OGMRipModuleEngineClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructor = ogmrip_module_engine_constructor;
  gobject_class->dispose = ogmrip_module_engine_dispose;
  gobject_class->get_property = ogmrip_module_engine_get_property;

  g_object_class_install_property (gobject_class, PROP_MODULES,
      g_param_spec_boxed ("modules", "modules", "modules",
        G_TYPE_STRV, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

OGMRipModuleEngine *
ogmrip_module_engine_get_default (void)
{
  if (!default_engine)
    return g_object_new (OGMRIP_TYPE_MODULE_ENGINE, NULL);

  return default_engine;
}

gboolean
ogmrip_module_engine_add_path (OGMRipModuleEngine *engine, const gchar *path, GError **error)
{
  GDir *dir;
  OGMRipModule *module;
  const gchar *name;

  g_return_val_if_fail (OGMRIP_IS_MODULE_ENGINE (engine), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  dir = g_dir_open (path, 0, error);
  if (!dir)
    return FALSE;

  while ((name = g_dir_read_name (dir)))
  {
    if (!g_str_has_prefix (name, "lib") &&
        g_str_has_suffix (name, "." G_MODULE_SUFFIX))
    {
      module = ogmrip_module_new (name, path);
      if (g_type_module_use (G_TYPE_MODULE (module)))
        engine->priv->modules = g_slist_prepend (engine->priv->modules, module);
      else
        g_object_unref (module);
    }
  }

  g_dir_close (dir);

  g_object_notify (G_OBJECT (engine), "modules");

  return TRUE;
}

static gint
ogmrip_module_compare_name (OGMRipModule *module, const gchar *name)
{
  return strcmp (ogmrip_module_get_name (module), name);
}

OGMRipModule *
ogmrip_module_engine_get (OGMRipModuleEngine *engine, const gchar *name)
{
  GSList *link;

  g_return_val_if_fail (OGMRIP_IS_MODULE_ENGINE (engine), FALSE);
  g_return_val_if_fail (name != NULL, NULL);

  link = g_slist_find_custom (engine->priv->modules,
      name, (GCompareFunc) ogmrip_module_compare_name);

  if (!link)
    return NULL;

  return link->data;
}

GSList *  
ogmrip_module_engine_get_list (OGMRipModuleEngine *engine)
{
  g_return_val_if_fail (OGMRIP_IS_MODULE_ENGINE (engine), NULL);

  return g_slist_copy (engine->priv->modules);
}

