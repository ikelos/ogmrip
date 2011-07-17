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
 * SECTION:ogmrip-options-plugin
 * @title: Plugins System
 * @short_description: Functions for manipulating the plugins
 * @include: ogmrip-options-plugin.h
 */

#include "ogmrip-options-plugin.h"

#include <string.h>

#define OGMRIP_PLUGIN_DIALOG_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_PLUGIN_DIALOG, OGMRipPluginDialogPriv))

struct _OGMRipPluginDialogPriv
{
  OGMRipProfile *profile;
};

typedef struct
{
  GModule *module;
  GType dialog;
  GType type;
} OGMRipOptionsPlugin;

typedef OGMRipOptionsPlugin * (* OGMRipOptionsPluginInit) (void);

static void ogmrip_plugin_dialog_dispose      (GObject          *gobject);
static void ogmrip_plugin_dialog_set_property (GObject          *gobject,
                                               guint            property_id,
                                               const GValue     *value,
                                               GParamSpec       *pspec);
static void ogmrip_plugin_dialog_get_property (GObject          *gobject,
                                               guint            property_id,
                                               GValue           *value,
                                               GParamSpec       *pspec);
enum
{
  PROP_0,
  PROP_PROFILE
};

static GSList *plugins = NULL;

G_DEFINE_ABSTRACT_TYPE (OGMRipPluginDialog, ogmrip_plugin_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_plugin_dialog_class_init (OGMRipPluginDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) klass;
  gobject_class->dispose = ogmrip_plugin_dialog_dispose;
  gobject_class->set_property = ogmrip_plugin_dialog_set_property;
  gobject_class->get_property = ogmrip_plugin_dialog_get_property;

  g_object_class_install_property (gobject_class, PROP_PROFILE, 
        g_param_spec_object ("profile", "Profile property", "Set the profile property", 
           OGMRIP_TYPE_PROFILE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipPluginDialogPriv));
}

static void
ogmrip_plugin_dialog_init (OGMRipPluginDialog *dialog)
{
  dialog->priv = OGMRIP_PLUGIN_DIALOG_GET_PRIVATE (dialog);
}

static void
ogmrip_plugin_dialog_dispose (GObject *gobject)
{
  OGMRipPluginDialog *dialog;

  dialog = OGMRIP_PLUGIN_DIALOG (gobject);

  if (dialog->priv->profile)
  {
    g_object_unref (dialog->priv->profile);
    dialog->priv->profile = NULL;
  }

  (*G_OBJECT_CLASS (ogmrip_plugin_dialog_parent_class)->dispose) (gobject);
}

static void
ogmrip_plugin_dialog_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id) 
  {
    case PROP_PROFILE:
      OGMRIP_PLUGIN_DIALOG (gobject)->priv->profile = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_plugin_dialog_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id) 
  {
    case PROP_PROFILE:
      g_value_set_object (value, OGMRIP_PLUGIN_DIALOG (gobject)->priv->profile);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

/**
 * ogmrip_plugin_dialog_get_profile:
 * @dialog: an #OGMRipPluginDialog
 *
 * Gets the profile.
 *
 * Returns: the profile, or NULL
 */
OGMRipProfile *
ogmrip_plugin_dialog_get_profile (OGMRipPluginDialog *dialog)
{
  g_return_val_if_fail (OGMRIP_IS_PLUGIN_DIALOG (dialog), NULL);

  return dialog->priv->profile;
}

static GSList *
ogmrip_options_plugin_load (GSList *slist, const gchar *dirname)
{
  GModule *module;
  GPatternSpec *pspec;
  GDir *dir;

  OGMRipOptionsPlugin *plugin;
  OGMRipOptionsPluginInit init;
  gpointer ptr;

  const gchar *filename;
  gchar *fullname;
  gint len;

  len = strlen (dirname);

  pspec = g_pattern_spec_new ("*.so");

  dir = g_dir_open (dirname, 0, NULL);
  if (dir)
  {
    while ((filename = g_dir_read_name (dir)))
    {
      init = NULL;

      if (!g_pattern_match_string (pspec, filename))
        continue;

      fullname = g_build_filename (dirname, filename, NULL);
      module = g_module_open (fullname, G_MODULE_BIND_LAZY);
      g_free (fullname);

      if (!module)
      {
        g_warning ("Cannot open module %s", filename);
        continue;
      }

      if (!g_module_symbol (module, "ogmrip_init_options_plugin", &ptr))
      {
        g_warning ("Cannot find initialization function in module %s", filename);
        g_module_close (module);
        continue;
      }

      init = (OGMRipOptionsPluginInit) ptr;

      if (!init)
      {
        g_warning ("Invalid initialization function for module %s", filename);
        g_module_close (module);
        continue;
      }

      plugin = (* init) ();
      if (!plugin)
      {
        g_warning ("Failed to initialize module %s", filename);
        g_module_close (module);
        continue;
      }

      plugin->module = module;
      slist = g_slist_append (slist, plugin);
    }
    g_dir_close (dir);
  }

  g_pattern_spec_free (pspec);

  return slist;
}

#define OGMRIP_OPTIONS_PLUGINS_DIR \
  OGMRIP_LIB_DIR G_DIR_SEPARATOR_S "ogmrip" G_DIR_SEPARATOR_S "options-plugins"

/**
 * ogmrip_options_plugin_init:
 *
 * Initializes the plugin system.
 */
void
ogmrip_options_plugin_init (void)
{
  if (!plugins)
  {
    gchar *dir;

    plugins = ogmrip_options_plugin_load (plugins, OGMRIP_OPTIONS_PLUGINS_DIR);

    dir = g_build_filename (g_get_home_dir (), ".ogmrip", "options-plugins", NULL);
    plugins = ogmrip_options_plugin_load (plugins, dir);
    g_free (dir);
  }
}

static void
ogmrip_options_plugin_close_module (OGMRipOptionsPlugin *plugin)
{
  g_module_close (plugin->module);
}

/**
 * ogmrip_options_plugin_uninit:
 *
 * Uninitializes the plugin system.
 */
void
ogmrip_options_plugin_uninit (void)
{
  g_slist_foreach (plugins, (GFunc) ogmrip_options_plugin_close_module, NULL);
}

static OGMRipOptionsPlugin *
ogmrip_options_plugin_find_by_type (GType type)
{
  OGMRipOptionsPlugin *plugin;
  GSList *link;

  for (link = plugins; link; link = link->next)
  {
    plugin = link->data;

    if (plugin && g_type_is_a (type, plugin->type))
        return plugin;

    if (plugin && plugin->type == type)
      return plugin;
  }

  return NULL;
}

/**
 * ogmrip_options_plugin_exists:
 * @type: The type of a codec or a container
 *
 * Checks wether a plugin exists for the codec or container.
 *
 * Returns: TRUE or FALSE
 */
gboolean
ogmrip_options_plugin_exists (GType type)
{
  g_return_val_if_fail (type == G_TYPE_NONE ||
      g_type_is_a (type, OGMRIP_TYPE_CONTAINER) ||
      g_type_is_a (type, OGMRIP_TYPE_CODEC), FALSE);

  if (ogmrip_options_plugin_find_by_type (type))
    return TRUE;

  return FALSE;
}

static GtkWidget *
ogmrip_options_plugin_dialog_new (GType type, OGMRipProfile *profile)
{
  OGMRipOptionsPlugin *plugin;

  plugin = ogmrip_options_plugin_find_by_type (type);
  if (!plugin)
    return NULL; 

  return g_object_new (plugin->dialog, "profile", profile, NULL);
}

/**
 * ogmrip_container_options_plugin_dialog_new:
 * @type: The type of a container
 * @profile: An #OGMRipProfile
 *
 * Creates a new #GtkDialog to configure the container.
 *
 * Returns: a new #GtkDialog
 */
GtkWidget *
ogmrip_container_options_plugin_dialog_new (GType type, OGMRipProfile *profile)
{
  g_return_val_if_fail (g_type_is_a (type, OGMRIP_TYPE_CONTAINER), NULL);

  return ogmrip_options_plugin_dialog_new (type, profile);
}

/**
 * ogmrip_video_options_plugin_dialog_new:
 * @type: The type of a video codec
 * @profile: An #OGMRipProfile
 *
 * Creates a new #GtkDialog to configure the codec.
 *
 * Returns: a new #GtkDialog
 */
GtkWidget *
ogmrip_video_options_plugin_dialog_new (GType type, OGMRipProfile *profile)
{
  g_return_val_if_fail (g_type_is_a (type, OGMRIP_TYPE_VIDEO_CODEC), NULL);

  return ogmrip_options_plugin_dialog_new (type, profile);
}

/**
 * ogmrip_audio_options_plugin_dialog_new:
 * @type: The type of a audio codec
 * @profile: An #OGMRipProfile
 *
 * Creates a new #GtkDialog to configure the codec.
 *
 * Returns: a new #GtkDialog
 */
GtkWidget *
ogmrip_audio_options_plugin_dialog_new (GType type, OGMRipProfile *profile)
{
  g_return_val_if_fail (g_type_is_a (type, OGMRIP_TYPE_AUDIO_CODEC), NULL);

  return ogmrip_options_plugin_dialog_new (type, profile);
}

/**
 * ogmrip_subp_options_plugin_dialog_new:
 * @type: The type of a subp codec
 * @profile: An #OGMRipProfile
 *
 * Creates a new #GtkDialog to configure the codec.
 *
 * Returns: a new #GtkDialog
 */
GtkWidget *
ogmrip_subp_options_plugin_dialog_new (GType type, OGMRipProfile *profile)
{
  g_return_val_if_fail (g_type_is_a (type, OGMRIP_TYPE_SUBP_CODEC), NULL);

  return ogmrip_options_plugin_dialog_new (type, profile);
}

