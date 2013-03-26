/* OGMRipModule - A module library for OGMRip
 * Copyright (C) 2012-2013 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-module-object.h"

#include <gmodule.h>
#include <string.h>

typedef void (* OGMRipModuleRegisterFunc) (OGMRipModule *module);

struct _OGMRipModulePriv
{
  GModule *library;

  gchar *name;
  gchar *path;
};

enum
{
  PROP_0,
  PROP_NAME,
  PROP_PATH
};

static gchar *
ogmrip_module_build_path (OGMRipModule *module)
{
  gchar *name, *path;

  name = g_strconcat (module->priv->name, "." G_MODULE_SUFFIX, NULL);
  path = g_build_filename (module->priv->path, name, NULL);
  g_free (name);

  return path;
}

G_DEFINE_TYPE (OGMRipModule, ogmrip_module, G_TYPE_TYPE_MODULE)

static void
ogmrip_module_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  OGMRipModule *module = OGMRIP_MODULE (gobject);

  switch (prop_id)
  {
    case PROP_NAME:
      g_value_set_string (value, module->priv->name);
      break;
    case PROP_PATH:
      g_value_set_string (value, module->priv->path);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_module_set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipModule *module = OGMRIP_MODULE (gobject);
  const gchar *name;

  switch (prop_id)
  {
    case PROP_NAME:
      name = g_value_get_string (value);
      if (g_str_has_suffix (name, "." G_MODULE_SUFFIX))
        module->priv->name = g_strndup (name, strlen (name) - strlen ("." G_MODULE_SUFFIX));
      else
        module->priv->name = g_strdup (name);
      g_type_module_set_name (G_TYPE_MODULE (module), module->priv->name);
      break;
    case PROP_PATH:
      module->priv->path = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_module_finalize (GObject *gobject)
{
  OGMRipModule *module = OGMRIP_MODULE (gobject);

  g_free (module->priv->name);
  g_free (module->priv->path);

  G_OBJECT_CLASS (ogmrip_module_parent_class)->finalize (gobject);
}

static gboolean
ogmrip_module_load_module (GTypeModule *gmodule)
{
  OGMRipModule *module = OGMRIP_MODULE (gmodule);
  OGMRipModuleRegisterFunc register_func;
  gchar *path;

  path = ogmrip_module_build_path (module);
  g_return_val_if_fail (path != NULL, FALSE);

  module->priv->library = g_module_open (path, G_MODULE_BIND_LAZY);
  g_free (path);

  if (!module->priv->library)
  {
    g_warning ("%s: %s", module->priv->name, g_module_error ());
    return FALSE;
  }

  if (!g_module_symbol (module->priv->library,
        "ogmrip_module_load", (gpointer) &register_func))
  {
    g_warning ("%s: %s", module->priv->name, g_module_error ());
    g_module_close (module->priv->library);
    module->priv->library = NULL;

    return FALSE;
  }

  if (!register_func)
  {
    g_warning ("%s: Symbol 'ogmrip_module_load' is not defined", module->priv->name);
    g_module_close (module->priv->library);
    module->priv->library = NULL;

    return FALSE;
  }

  g_module_make_resident (module->priv->library);

  register_func (module);

  g_debug ("Loading module '%s'", module->priv->name);

  return TRUE;
}

static void
ogmrip_module_unload_module (GTypeModule *gmodule)
{
  OGMRipModule *module = OGMRIP_MODULE (gmodule);

  g_debug ("Unloading module '%s'", module->priv->name);

  g_module_close (module->priv->library);
  module->priv->library = NULL;
}

static void
ogmrip_module_init (OGMRipModule *module)
{
  module->priv = G_TYPE_INSTANCE_GET_PRIVATE (module,
      OGMRIP_TYPE_MODULE, OGMRipModulePriv);
}

static void
ogmrip_module_class_init (OGMRipModuleClass *klass)
{
  GObjectClass *gobject_class;
  GTypeModuleClass *module_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_module_get_property;
  gobject_class->set_property = ogmrip_module_set_property;
  gobject_class->finalize = ogmrip_module_finalize;

  module_class = G_TYPE_MODULE_CLASS (klass);
  module_class->load = ogmrip_module_load_module;
  module_class->unload = ogmrip_module_unload_module;

  g_object_class_install_property (gobject_class, PROP_NAME,
      g_param_spec_string ("name", "Module name", "The name of the module",
        NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PATH,
      g_param_spec_string ("path", "Module path", "The path of the module",
        NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipModulePriv));
}

OGMRipModule *
ogmrip_module_new (const gchar *name, const gchar *path)
{
  return g_object_new (OGMRIP_TYPE_MODULE, "name", name, "path", path, NULL);
}

const gchar *
ogmrip_module_get_name (OGMRipModule *module)
{
  g_return_val_if_fail (OGMRIP_IS_MODULE (module), NULL);

  return module->priv->name;
}

const gchar *
ogmrip_module_get_path (OGMRipModule *module)
{
  g_return_val_if_fail (OGMRIP_IS_MODULE (module), NULL);

  return module->priv->path;
}

gboolean
ogmrip_module_get_symbol (OGMRipModule *module, const gchar *name, gpointer *symbol)
{
  g_return_val_if_fail (OGMRIP_IS_MODULE (module), FALSE);

  if (!module->priv->library)
    return FALSE;

  return g_module_symbol (module->priv->library, name, symbol);
}

