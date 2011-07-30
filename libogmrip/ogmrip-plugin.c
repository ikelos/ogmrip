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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/**
 * SECTION:ogmrip-plugin
 * @title: Plugins System
 * @short_description: Functions for manipulating the plugins
 * @include: ogmrip-plugin.h
 */

#include "ogmrip-plugin.h"
#include "ogmrip-container.h"
#include "ogmrip-audio-codec.h"
#include "ogmrip-video-codec.h"
#include "ogmrip-subp-codec.h"

#include "ogmrip-hardsub.h"
#include "ogmrip-novideo.h"

#include "ogmjob-log.h"

#include <string.h>

#include <glib/gi18n-lib.h>

typedef struct _OGMRipPlugin OGMRipPlugin;

struct _OGMRipPlugin
{
  GModule *module;
  GType type;
  gchar *name;
  gchar *description;
};

typedef struct _OGMRipPluginCodec OGMRipPluginCodec;

struct _OGMRipPluginCodec
{
  GModule *module;
  GType type;
  gchar *name;
  gchar *description;
  gint format;
};

typedef OGMRipPlugin * (* OGMRipPluginInit) (GError **error);

static GSList *video_plugins = NULL;
static GSList *audio_plugins = NULL;
static GSList *subp_plugins  = NULL;
static GSList *container_plugins   = NULL;

GQuark
ogmrip_plugin_error_quark (void)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_static_string ("ogmrip-plugin-error-quark");

  return quark;
}

static gint
ogmrip_plugin_compare (OGMRipPlugin *plugin1, OGMRipPlugin *plugin2)
{
  return strcmp (plugin1->name, plugin2->name);
}

static GSList *
ogmrip_plugin_load (GSList *slist, const gchar *dirname, GType type)
{
  GModule *module;
  GPatternSpec *pspec;
  GError *error;
  GDir *dir;

  OGMRipPlugin *plugin;
  OGMRipPluginInit init;
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
      error = NULL;

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

      if (!g_module_symbol (module, "ogmrip_init_plugin", &ptr))
      {
        g_warning ("Cannot find initialization function in module %s", filename);
        g_module_close (module);
        continue;
      }

      init = (OGMRipPluginInit) ptr;

      if (!init)
      {
        g_warning ("Invalid initialization function for module %s", filename);
        g_module_close (module);
        continue;
      }

      plugin = (* init) (&error);
      if (!plugin)
      {
        gchar *msg;

        msg = g_strdup_printf (_("Plugin %s disabled"), filename);
        if (!error)
          g_print ("%s: %s\n", msg, _("some requirements are not available"));
        else
        {
          g_print ("%s: %s\n", msg, error->message);
          g_error_free (error);
        }
        g_free (msg);

        g_module_close (module);
        continue;
      }

      if (!g_type_is_a (plugin->type, type))
      {
        g_warning ("Invalid type for module %s, %s expected, %s found", filename, g_type_name (type), g_type_name (plugin->type));
        g_module_close (module);
        continue;
      }

      plugin->module = module;
      slist = g_slist_insert_sorted (slist, plugin, (GCompareFunc) ogmrip_plugin_compare);
    }
    g_dir_close (dir);
  }

  g_pattern_spec_free (pspec);

  return slist;
}

#define OGMRIP_VIDEO_PLUGINS_DIR \
  OGMRIP_LIB_DIR G_DIR_SEPARATOR_S "ogmrip" G_DIR_SEPARATOR_S "video-plugins"
#define OGMRIP_AUDIO_PLUGINS_DIR \
  OGMRIP_LIB_DIR G_DIR_SEPARATOR_S "ogmrip" G_DIR_SEPARATOR_S "audio-plugins"
#define OGMRIP_SUBP_PLUGINS_DIR \
  OGMRIP_LIB_DIR G_DIR_SEPARATOR_S "ogmrip" G_DIR_SEPARATOR_S "subp-plugins"
#define OGMRIP_CONTAINER_PLUGINS_DIR \
  OGMRIP_LIB_DIR G_DIR_SEPARATOR_S "ogmrip" G_DIR_SEPARATOR_S "container-plugins"

/**
 * ogmrip_plugin_init:
 *
 * Initializes the plugin system.
 */
void
ogmrip_plugin_init (void)
{
  gchar *dir;

  if (!video_plugins)
  {
    OGMRipVideoPlugin *plugin;

    video_plugins = ogmrip_plugin_load (video_plugins, OGMRIP_VIDEO_PLUGINS_DIR, OGMRIP_TYPE_VIDEO_CODEC);

    dir = g_build_filename (g_get_home_dir (), ".ogmrip", "video-plugins", NULL);
    video_plugins = ogmrip_plugin_load (video_plugins, dir, OGMRIP_TYPE_VIDEO_CODEC);
    g_free (dir);

    plugin = ogmrip_novideo_get_plugin ();
    if (plugin)
      video_plugins = g_slist_insert_sorted (video_plugins, plugin, (GCompareFunc) ogmrip_plugin_compare);
  }

  if (!audio_plugins)
  {
    audio_plugins = ogmrip_plugin_load (audio_plugins, OGMRIP_AUDIO_PLUGINS_DIR, OGMRIP_TYPE_AUDIO_CODEC);

    dir = g_build_filename (g_get_home_dir (), ".ogmrip", "audio-plugins", NULL);
    audio_plugins = ogmrip_plugin_load (audio_plugins, dir, OGMRIP_TYPE_AUDIO_CODEC);
    g_free (dir);
  }

  if (!subp_plugins)
  {
    OGMRipSubpPlugin *plugin;

    subp_plugins = ogmrip_plugin_load (subp_plugins, OGMRIP_SUBP_PLUGINS_DIR, OGMRIP_TYPE_SUBP_CODEC);

    dir = g_build_filename (g_get_home_dir (), ".ogmrip", "subp-plugins", NULL);
    subp_plugins = ogmrip_plugin_load (subp_plugins, dir, OGMRIP_TYPE_SUBP_CODEC);
    g_free (dir);

    plugin = ogmrip_hardsub_get_plugin ();
    if (plugin)
      subp_plugins = g_slist_insert_sorted (subp_plugins, plugin, (GCompareFunc) ogmrip_plugin_compare);
  }

  if (!container_plugins)
  {
    container_plugins = ogmrip_plugin_load (container_plugins, OGMRIP_CONTAINER_PLUGINS_DIR, OGMRIP_TYPE_CONTAINER);

    dir = g_build_filename (g_get_home_dir (), ".ogmrip", "container-plugins", NULL);
    container_plugins = ogmrip_plugin_load (container_plugins, dir, OGMRIP_TYPE_CONTAINER);
    g_free (dir);
  }
}

static void
ogmrip_plugin_close_module (OGMRipPlugin *plugin)
{
  if (plugin->module)
    g_module_close (plugin->module);
}

/**
 * ogmrip_plugin_uninit:
 *
 * Uninitializes the plugin system.
 */
void
ogmrip_plugin_uninit (void)
{
  g_slist_foreach (video_plugins, (GFunc) ogmrip_plugin_close_module, NULL);
  g_slist_foreach (audio_plugins, (GFunc) ogmrip_plugin_close_module, NULL);
  g_slist_foreach (subp_plugins,  (GFunc) ogmrip_plugin_close_module, NULL);
  g_slist_foreach (container_plugins,   (GFunc) ogmrip_plugin_close_module, NULL);
}

static OGMRipContainerPlugin *
ogmrip_plugin_find_container_by_type (GSList *list, GType type)
{
  OGMRipContainerPlugin *plugin;

  while (list)
  {
    plugin = (OGMRipContainerPlugin *) list->data;
    if (plugin->type == type)
      return plugin;
    list = list->next;
  }

  return NULL;
}

static GType
ogmrip_plugin_get_nth_codec (GSList *list, guint n)
{
  OGMRipPlugin *plugin;

  if (!list)
    return G_TYPE_NONE;

  plugin = g_slist_nth_data (list, n);
  if (!plugin)
    plugin = list->data;

  return plugin->type;
}

static GType
ogmrip_plugin_get_type_by_name (GSList *list, const gchar *name)
{
  OGMRipPlugin *plugin;
  GSList *link;

  g_return_val_if_fail (name != NULL, G_TYPE_NONE);

  for (link = list; link; link = link->next)
  {
    plugin = link->data;
    if (g_str_equal (plugin->name, name))
      return plugin->type;
  }

  return G_TYPE_NONE;
}

static gint
ogmrip_plugin_get_codec_index (GSList *list, GType type)
{
  OGMRipPlugin *plugin;
  gint index;

  if (!list)
    return -1;

  for (index = 0; list; index ++, list = list->next)
  {
    plugin = list->data;
    if (plugin->type == type)
      return index;
  }

  return -1;
}

static void
ogmrip_plugin_foreach_codec (GSList *list, OGMRipPluginFunc func, gpointer data)
{
  OGMRipPlugin *plugin;

  while (list)
  {
    plugin = list->data;
    func (plugin->type, (const gchar *) plugin->name, (const gchar *) plugin->description, data);
    list = list->next;
  }
}

static OGMRipPlugin *
ogmrip_plugin_find_codec_by_type (GSList *list, GType type)
{
  OGMRipPlugin *plugin;

  while (list)
  {
    plugin = list->data;
    if (plugin->type == type)
      return plugin;
    list = list->next;
  }

  return NULL;
}

static const gchar *
ogmrip_plugin_get_codec_name (GSList *list, GType type)
{
  OGMRipPlugin *plugin;

  plugin = ogmrip_plugin_find_codec_by_type (list, type);
  if (plugin)
    return (gchar *) plugin->name;

  return NULL;
}

static gint
ogmrip_plugin_get_codec_format (GSList *list, GType type)
{
  OGMRipPluginCodec *plugin;

  plugin = (OGMRipPluginCodec *) ogmrip_plugin_find_codec_by_type (list, type);
  if (plugin)
    return plugin->format;

  return -1;
}

static GType
ogmrip_plugin_find_codec (GSList *list, OGMRipPluginCmpFunc func, gconstpointer data)
{
  OGMRipPlugin *plugin;

  while (list)
  {
    plugin = list->data;
    if (func (plugin->type, (gchar *) plugin->name, (gchar *) plugin->description, data) == 0)
      return plugin->type;
    list = list->next;
  }

  return G_TYPE_NONE;
}

/**
 * ogmrip_plugin_get_n_containers:
 *
 * Gets the number of container plugins.
 *
 * Returns: the number of container plugins
 */
gint
ogmrip_plugin_get_n_containers (void)
{
  return g_slist_length (container_plugins);
}

/**
 * ogmrip_plugin_foreach_container:
 * @func: The function to call with each container plugin's data
 * @data: User data to pass to the function
 *
 * Calls a function for each container plugin.
 */
void
ogmrip_plugin_foreach_container (OGMRipPluginFunc func, gpointer data)
{
  OGMRipContainerPlugin *plugin;
  GSList *link;

  g_return_if_fail (func != NULL);

  for (link = container_plugins; link; link = link->next)
  {
    plugin = link->data;
    func (plugin->type, (const gchar *) plugin->name, (const gchar *) plugin->description, data);
  }
}

/**
 * ogmrip_plugin_find_container:
 * @func: The function to call for each container plugin. It should return 0
 * when the desired container is found
 * @data: User data passed to the function
 *
 * Finds a container using the supplied function.
 *
 * Returns: The type of the container, or %G_TYPE_NONE
 */
GType
ogmrip_plugin_find_container (OGMRipPluginCmpFunc func, gconstpointer data)
{
  OGMRipContainerPlugin *plugin;
  GSList *link;

  g_return_val_if_fail (func != NULL, G_TYPE_NONE);

  for (link = container_plugins; link; link = link->next)
  {
    plugin = link->data;
    if (func (plugin->type, (gchar *) plugin->name, (gchar *) plugin->description, data) == 0)
      return plugin->type;
  }

  return G_TYPE_NONE;
}

/**
 * ogmrip_plugin_get_nth_container:
 * @n: The index of the container
 *
 * Gets the container at the given position.
 *
 * Returns: The type of the container, or %G_TYPE_NONE
 */
GType
ogmrip_plugin_get_nth_container (guint n)
{
  OGMRipContainerPlugin *plugin;

  if (!container_plugins)
    return G_TYPE_NONE;

  plugin = g_slist_nth_data (container_plugins, n);
  if (!plugin)
    plugin = container_plugins->data;

  return plugin->type;
}

/**
 * ogmrip_plugin_get_container_by_name:
 * @name: The name of the container
 *
 * Gets the container with the given name.
 *
 * Returns: The type of the container, or %G_TYPE_NONE
 */
GType
ogmrip_plugin_get_container_by_name (const gchar *name)
{
  return ogmrip_plugin_get_type_by_name (container_plugins, name);
}

/**
 * ogmrip_plugin_get_container_index:
 * @container: A container type
 *
 * Gets the position of the given container.
 *
 * Returns: The index of the container, or -1
 */
gint
ogmrip_plugin_get_container_index (GType container)
{
  GSList *link;
  OGMRipContainerPlugin *plugin;
  gint index;

  g_return_val_if_fail (g_type_is_a (container, OGMRIP_TYPE_CONTAINER), 0);

  for (index = 0, link = container_plugins; link; index ++, link = link->next)
  {
    plugin = link->data;
    if (plugin->type == container)
      return index;
  }

  return -1;
}

/**
 * ogmrip_plugin_get_container_name:
 * @container: A container type
 *
 * Gets the name of the given container.
 *
 * Returns: The name of the container, or NULL 
 */
const gchar *
ogmrip_plugin_get_container_name (GType container)
{
  OGMRipContainerPlugin *plugin;

  g_return_val_if_fail (g_type_is_a (container, OGMRIP_TYPE_CONTAINER), NULL);

  plugin = ogmrip_plugin_find_container_by_type (container_plugins, container);
  if (plugin)
    return (gchar *) plugin->name;

  return NULL;
}

/**
 * ogmrip_plugin_get_container_bframes:
 * @container: A container type
 *
 * Gets whether the given container supports B-frames
 *
 * Returns: %TRUE if the container supports B-frames
 */
gboolean
ogmrip_plugin_get_container_bframes (GType container)
{
  OGMRipContainerPlugin *plugin;

  g_return_val_if_fail (g_type_is_a (container, OGMRIP_TYPE_CONTAINER), FALSE);

  plugin = ogmrip_plugin_find_container_by_type (container_plugins, container);
  if (plugin)
    return plugin->bframes;

  return FALSE;
}

/**
 * ogmrip_plugin_get_container_max_audio:
 * @container: A container type
 *
 * Returns the number of audio streams the given container can contain.
 *
 * Returns: the number of audio streams, or -1
 */
gint
ogmrip_plugin_get_container_max_audio (GType container)
{
  OGMRipContainerPlugin *plugin;

  g_return_val_if_fail (g_type_is_a (container, OGMRIP_TYPE_CONTAINER), -1);

  plugin = ogmrip_plugin_find_container_by_type (container_plugins, container);
  if (!plugin)
    return -1;

  return plugin->max_audio;
}

/**
 * ogmrip_plugin_get_container_max_subp:
 * @container: A container type
 *
 * Returns the number of subtitle streams the given container can contain.
 *
 * Returns: the number of subtitle streams, or -1
 */
gint
ogmrip_plugin_get_container_max_subp (GType container)
{
  OGMRipContainerPlugin *plugin;

  g_return_val_if_fail (g_type_is_a (container, OGMRIP_TYPE_CONTAINER), -1);

  plugin = ogmrip_plugin_find_container_by_type (container_plugins, container);
  if (!plugin)
    return -1;

  return plugin->max_subp;
}

/**
 * ogmrip_plugin_get_container_module:
 * @container: A container type
 *
 * Gets the #GModule associated with @container.
 *
 * Returns: A #GModule, or NULL
 */
GModule *
ogmrip_plugin_get_container_module (GType container)
{
  OGMRipContainerPlugin *plugin;

  g_return_val_if_fail (g_type_is_a (container, OGMRIP_TYPE_CONTAINER), NULL);

  plugin = ogmrip_plugin_find_container_by_type (container_plugins, container);
  if (!plugin)
    return NULL;

  return plugin->module;
}

/**
 * ogmrip_plugin_get_n_video_codecs:
 *
 * Gets the number of video codec plugins.
 *
 * Returns: the number of video codec plugins
 */
gint
ogmrip_plugin_get_n_video_codecs (void)
{
  return g_slist_length (video_plugins);
}

/**
 * ogmrip_plugin_foreach_video_codec:
 * @func: The function to call with each video codec plugin's data
 * @data: User data to pass to the function
 *
 * Calls a function for each video codec plugin.
 */
void
ogmrip_plugin_foreach_video_codec (OGMRipPluginFunc func, gpointer data)
{
  g_return_if_fail (func != NULL);

  ogmrip_plugin_foreach_codec (video_plugins, func, data);
}

/**
 * ogmrip_plugin_find_video_codec:
 * @func: The function to call for each video codec plugin. It should return 0
 * when the desired video codec is found
 * @data: User data passed to the function
 *
 * Finds a video codec using the supplied function.
 *
 * Returns: The type of the video codec, or %G_TYPE_NONE
 */
GType
ogmrip_plugin_find_video_codec (OGMRipPluginCmpFunc func, gconstpointer data)
{
  g_return_val_if_fail (func != NULL, G_TYPE_NONE);

  return ogmrip_plugin_find_codec (video_plugins, func, data);
}

/**
 * ogmrip_plugin_get_nth_video_codec:
 * @n: The index of the video codec
 *
 * Gets the video codec at the given position.
 *
 * Returns: The type of the video codec, or %G_TYPE_NONE
 */
GType
ogmrip_plugin_get_nth_video_codec (guint n)
{
  return ogmrip_plugin_get_nth_codec (video_plugins, n);
}

/**
 * ogmrip_plugin_get_video_codec_by_name:
 * @name: The name of the video codec
 *
 * Gets the video codec with the given name.
 *
 * Returns: The type of the video codec, or %G_TYPE_NONE
 */
GType
ogmrip_plugin_get_video_codec_by_name (const gchar *name)
{
  return ogmrip_plugin_get_type_by_name (video_plugins, name);
}

/**
 * ogmrip_plugin_get_video_codec_index:
 * @codec: A video codec type
 *
 * Gets the position of the given video codec.
 *
 * Returns: The index of the video codec, or -1
 */
gint
ogmrip_plugin_get_video_codec_index (GType codec)
{
  return ogmrip_plugin_get_codec_index (video_plugins, codec);
}

/**
 * ogmrip_plugin_get_video_codec_name:
 * @codec: A video codec type
 *
 * Gets the name of the given video codec.
 *
 * Returns: The name of the video codec, or NULL 
 */
const gchar *
ogmrip_plugin_get_video_codec_name (GType codec)
{
  g_return_val_if_fail (g_type_is_a (codec, OGMRIP_TYPE_VIDEO_CODEC), NULL);

  return ogmrip_plugin_get_codec_name (video_plugins, codec);
}

/**
 * ogmrip_plugin_get_video_codec_format:
 * @codec: A video codec type
 *
 * Gets the format of the given video codec.
 *
 * Returns: The format of the video codec, or NULL 
 */
gint
ogmrip_plugin_get_video_codec_format (GType codec)
{
  g_return_val_if_fail (g_type_is_a (codec, OGMRIP_TYPE_VIDEO_CODEC), -1);

  return ogmrip_plugin_get_codec_format (video_plugins, codec);
}

/**
 * ogmrip_plugin_get_video_codec_passes:
 * @codec: A video codec type
 *
 * Gets the maximum number of passes the given video codec supports.
 *
 * Returns: The maximum number of passes, or -1 
 */
gint
ogmrip_plugin_get_video_codec_passes (GType codec)
{
  OGMRipPlugin *plugin;

  g_return_val_if_fail (g_type_is_a (codec, OGMRIP_TYPE_VIDEO_CODEC), -1);

  plugin = ogmrip_plugin_find_codec_by_type (video_plugins, codec);
  if (!plugin)
    return -1;

  return ((OGMRipVideoPlugin *) plugin)->passes;
}

/**
 * ogmrip_plugin_get_video_codec_threads:
 * @codec: A video codec type
 *
 * Gets the maximum number of threads the given video codec supports.
 *
 * Returns: The maximum number of threads, or -1
 */
gint
ogmrip_plugin_get_video_codec_threads (GType codec)
{
  OGMRipPlugin *plugin;

  g_return_val_if_fail (g_type_is_a (codec, OGMRIP_TYPE_VIDEO_CODEC), -1);

  plugin = ogmrip_plugin_find_codec_by_type (video_plugins, codec);
  if (!plugin)
    return -1;

  return ((OGMRipVideoPlugin *) plugin)->threads;
}

/**
 * ogmrip_plugin_get_video_codec_module:
 * @codec: A video codec type
 *
 * Gets the #GModule associated with @codec.
 *
 * Returns: A #GModule, or NULL
 */
GModule *
ogmrip_plugin_get_video_codec_module (GType codec)
{
  OGMRipPlugin *plugin;

  g_return_val_if_fail (g_type_is_a (codec, OGMRIP_TYPE_VIDEO_CODEC), NULL);

  plugin = ogmrip_plugin_find_codec_by_type (video_plugins, codec);
  if (!plugin)
    return NULL;

  return plugin->module;
}

/**
 * ogmrip_plugin_get_n_audio_codecs:
 *
 * Gets the number of audio codec plugins.
 *
 * Returns: the number of audio codec plugins
 */
gint
ogmrip_plugin_get_n_audio_codecs (void)
{
  return g_slist_length (audio_plugins);
}

/**
 * ogmrip_plugin_foreach_audio_codec:
 * @func: The function to call with each plugin's data
 * @data: User data to pass to the function
 *
 * Calls a function for each plugin.
 */
void
ogmrip_plugin_foreach_audio_codec (OGMRipPluginFunc func, gpointer data)
{
  g_return_if_fail (func != NULL);

  ogmrip_plugin_foreach_codec (audio_plugins, func, data);
}

/**
 * ogmrip_plugin_find_audio_codec:
 * @func: The function to call for each audio codec plugin. It should return 0
 * when the desired audio codec is found
 * @data: User data passed to the function
 *
 * Finds a audio codec using the supplied function.
 *
 * Returns: The type of the audio codec, or %G_TYPE_NONE
 */
GType
ogmrip_plugin_find_audio_codec (OGMRipPluginCmpFunc func, gconstpointer data)
{
  g_return_val_if_fail (func != NULL, G_TYPE_NONE);

  return ogmrip_plugin_find_codec (audio_plugins, func, data);
}

/**
 * ogmrip_plugin_get_nth_audio_codec:
 * @n: The index of the audio codec
 *
 * Gets the audio codec at the given position.
 *
 * Returns: The type of the audio codec, or %G_TYPE_NONE
 */
GType
ogmrip_plugin_get_nth_audio_codec (guint n)
{
  return ogmrip_plugin_get_nth_codec (audio_plugins, n);
}

/**
 * ogmrip_plugin_get_audio_codec_by_name:
 * @name: The name of the audio codec
 *
 * Gets the audio codec with the given name.
 *
 * Returns: The type of the audio codec, or %G_TYPE_NONE
 */
GType
ogmrip_plugin_get_audio_codec_by_name (const gchar *name)
{
  return ogmrip_plugin_get_type_by_name (audio_plugins, name);
}

/**
 * ogmrip_plugin_get_audio_codec_index:
 * @codec: An audio codec type
 *
 * Gets the position of the given audio codec.
 *
 * Returns: The index of the audio codec, or -1
 */
gint
ogmrip_plugin_get_audio_codec_index (GType codec)
{
  return ogmrip_plugin_get_codec_index (audio_plugins, codec);
}

/**
 * ogmrip_plugin_get_audio_codec_name:
 * @codec: An audio codec type
 *
 * Gets the name of the given audio codec.
 *
 * Returns: The name of the audio codec, or NULL 
 */
const gchar *
ogmrip_plugin_get_audio_codec_name (GType codec)
{
  g_return_val_if_fail (g_type_is_a (codec, OGMRIP_TYPE_AUDIO_CODEC), NULL);

  return ogmrip_plugin_get_codec_name (audio_plugins, codec);
}

/**
 * ogmrip_plugin_get_audio_codec_format:
 * @codec: A audio codec type
 *
 * Gets the format of the given audio codec.
 *
 * Returns: The format of the audio codec, or NULL 
 */
gint
ogmrip_plugin_get_audio_codec_format (GType codec)
{
  g_return_val_if_fail (g_type_is_a (codec, OGMRIP_TYPE_AUDIO_CODEC), -1);

  return ogmrip_plugin_get_codec_format (audio_plugins, codec);
}

/**
 * ogmrip_plugin_get_audio_codec_module:
 * @codec: A audio codec type
 *
 * Gets the #GModule associated with @codec.
 *
 * Returns: A #GModule, or NULL
 */
GModule *
ogmrip_plugin_get_audio_codec_module (GType codec)
{
  OGMRipPlugin *plugin;

  g_return_val_if_fail (g_type_is_a (codec, OGMRIP_TYPE_AUDIO_CODEC), NULL);

  plugin = ogmrip_plugin_find_codec_by_type (audio_plugins, codec);
  if (!plugin)
    return NULL;

  return plugin->module;
}

/**
 * ogmrip_plugin_get_n_subp_codecs:
 *
 * Gets the number of subtitle codec plugins.
 *
 * Returns: the number of subtitle codec plugins
 */
gint
ogmrip_plugin_get_n_subp_codecs (void)
{
  return g_slist_length (subp_plugins);
}

/**
 * ogmrip_plugin_foreach_subp_codec:
 * @func: The function to call with each plugin's data
 * @data: User data to pass to the function
 *
 * Calls a function for each plugin.
 */
void
ogmrip_plugin_foreach_subp_codec (OGMRipPluginFunc func, gpointer data)
{
  g_return_if_fail (func != NULL);

  ogmrip_plugin_foreach_codec (subp_plugins, func, data);
}

/**
 * ogmrip_plugin_find_subp_codec:
 * @func: The function to call for each subtitle codec plugin. It should return 0
 * when the desired subtitle codec is found
 * @data: User data passed to the function
 *
 * Finds a subtitle codec using the supplied function.
 *
 * Returns: The type of the subtitle codec, or %G_TYPE_NONE
 */
GType
ogmrip_plugin_find_subp_codec (OGMRipPluginCmpFunc func, gconstpointer data)
{
  g_return_val_if_fail (func != NULL, G_TYPE_NONE);

  return ogmrip_plugin_find_codec (subp_plugins, func, data);
}

/**
 * ogmrip_plugin_get_nth_subp_codec:
 * @n: The index of the subtitle codec
 *
 * Gets the subtitle codec at the given position.
 *
 * Returns: The type of the subtitle codec, or %G_TYPE_NONE
 */
GType
ogmrip_plugin_get_nth_subp_codec (guint n)
{
  return ogmrip_plugin_get_nth_codec (subp_plugins, n);
}

/**
 * ogmrip_plugin_get_subp_codec_by_name:
 * @name: The name of the subp codec
 *
 * Gets the subp codec with the given name.
 *
 * Returns: The type of the subp codec, or %G_TYPE_NONE
 */
GType
ogmrip_plugin_get_subp_codec_by_name (const gchar *name)
{
  return ogmrip_plugin_get_type_by_name (subp_plugins, name);
}

/**
 * ogmrip_plugin_get_subp_codec_index:
 * @codec: A subtitle codec type
 *
 * Gets the position of the given subtitle codec.
 *
 * Returns: The index of the subtitle codec, or -1
 */
gint
ogmrip_plugin_get_subp_codec_index (GType codec)
{
  g_return_val_if_fail (g_type_is_a (codec, OGMRIP_TYPE_SUBP_CODEC), -1);

  return ogmrip_plugin_get_codec_index (subp_plugins, codec);
}

/**
 * ogmrip_plugin_get_subp_codec_name:
 * @codec: A subtitle codec type
 *
 * Gets the name of the given subtitle codec.
 *
 * Returns: The name of the subtitle codec, or NULL 
 */
const gchar *
ogmrip_plugin_get_subp_codec_name (GType codec)
{
  g_return_val_if_fail (g_type_is_a (codec, OGMRIP_TYPE_SUBP_CODEC), NULL);

  return ogmrip_plugin_get_codec_name (subp_plugins, codec);
}

/**
 * ogmrip_plugin_get_subp_codec_format:
 * @codec: A subtitle codec type
 *
 * Gets the format of the given subtitle codec.
 *
 * Returns: The format of the subtitle codec, or NULL 
 */
gint
ogmrip_plugin_get_subp_codec_format (GType codec)
{
  g_return_val_if_fail (g_type_is_a (codec, OGMRIP_TYPE_SUBP_CODEC), -1);

  return ogmrip_plugin_get_codec_format (subp_plugins, codec);
}

/**
 * ogmrip_plugin_get_subp_codec_text:
 * @codec: A subtitle codec type
 *
 * Gets whether the given codec outputs text subtitles.
 *
 * Returns: %TRUE if the codec output text subtitles
 */
gboolean
ogmrip_plugin_get_subp_codec_text (GType codec)
{
  OGMRipPlugin *plugin;

  g_return_val_if_fail (g_type_is_a (codec, OGMRIP_TYPE_SUBP_CODEC), FALSE);

  plugin = ogmrip_plugin_find_codec_by_type (subp_plugins, codec);
  if (!plugin)
    return FALSE;

  return ((OGMRipSubpPlugin *) plugin)->text;
}

/**
 * ogmrip_plugin_get_subp_codec_module:
 * @codec: A subp codec type
 *
 * Gets the #GModule associated with @codec.
 *
 * Returns: A #GModule, or NULL
 */
GModule *
ogmrip_plugin_get_subp_codec_module (GType codec)
{
  OGMRipPlugin *plugin;

  g_return_val_if_fail (g_type_is_a (codec, OGMRIP_TYPE_SUBP_CODEC), NULL);

  plugin = ogmrip_plugin_find_codec_by_type (subp_plugins, codec);
  if (!plugin)
    return NULL;

  return plugin->module;
}

/**
 * ogmrip_plugin_can_contain_format:
 * @container: A container type
 * @format: An #OGMRipFormatType
 *
 * Returns whether @container supports the given format.
 *
 * Returns: %TRUE if @container supports @format
 */
gboolean
ogmrip_plugin_can_contain_format (GType container, OGMRipFormatType format)
{
  OGMRipContainerPlugin *plugin;
  gint i;

  g_return_val_if_fail (g_type_is_a (container, OGMRIP_TYPE_CONTAINER), FALSE);

  plugin = ogmrip_plugin_find_container_by_type (container_plugins, container);
  if (!plugin || !plugin->formats)
    return FALSE;

  for (i = 0; plugin->formats[i] != -1; i ++)
    if (plugin->formats[i] == format)
      return TRUE;

  return FALSE;
}

static gboolean
ogmrip_plugin_can_contain_codec (GType container, GSList *codecs, GType codec)
{
  return ogmrip_plugin_can_contain_format (container,
      ogmrip_plugin_get_codec_format (codecs, codec));
}

/**
 * ogmrip_plugin_can_contain_video:
 * @container: A container type
 * @codec: A video codec type
 *
 * Returns whether @container supports the given video codec.
 *
 * Returns: %TRUE if @container supports @type
 */
gboolean
ogmrip_plugin_can_contain_video (GType container, GType codec)
{
  g_return_val_if_fail (g_type_is_a (container, OGMRIP_TYPE_CONTAINER), FALSE);
  g_return_val_if_fail (codec == G_TYPE_NONE || g_type_is_a (codec, OGMRIP_TYPE_VIDEO_CODEC), FALSE);

  if (codec == G_TYPE_NONE)
  {
    OGMRipContainerPlugin *plugin;

    plugin = ogmrip_plugin_find_container_by_type (container_plugins, container);
    if (!plugin)
      return FALSE;

    return plugin->video ? FALSE : TRUE;
  }

  return ogmrip_plugin_can_contain_codec (container, video_plugins, codec);
}

/**
 * ogmrip_plugin_can_contain_audio:
 * @container: A container type
 * @codec: An audio codec type
 *
 * Returns whether @container supports the given audio codec.
 *
 * Returns: %TRUE if @container supports @type
 */
gboolean
ogmrip_plugin_can_contain_audio (GType container, GType codec)
{
  g_return_val_if_fail (g_type_is_a (container, OGMRIP_TYPE_CONTAINER), FALSE);
  g_return_val_if_fail (g_type_is_a (codec, OGMRIP_TYPE_AUDIO_CODEC), FALSE);

  return ogmrip_plugin_can_contain_codec (container, audio_plugins, codec);
}

/**
 * ogmrip_plugin_can_contain_subp:
 * @container: A container type
 * @codec: A subtitle codec type
 *
 * Returns whether @container supports the given subtitle codec.
 *
 * Returns: %TRUE if @container supports @type
 */
gboolean
ogmrip_plugin_can_contain_subp (GType container, GType codec)
{
  g_return_val_if_fail (g_type_is_a (container, OGMRIP_TYPE_CONTAINER), FALSE);
  g_return_val_if_fail (g_type_is_a (codec, OGMRIP_TYPE_SUBP_CODEC), FALSE);

  if (codec == OGMRIP_TYPE_HARDSUB)
    return TRUE;

  return ogmrip_plugin_can_contain_codec (container, subp_plugins, codec);
}

/**
 * ogmrip_plugin_can_contain_n_audio:
 * @container: A container type
 * @ncodec: The number of audio codecs
 *
 * Returns whether @container can contain @ncodec audio streams.
 *
 * Returns: %TRUE if @container can contain @ncodec audio streams
 */
gboolean
ogmrip_plugin_can_contain_n_audio (GType container, guint ncodec)
{
  OGMRipContainerPlugin *plugin;

  g_return_val_if_fail (g_type_is_a (container, OGMRIP_TYPE_CONTAINER), FALSE);

  plugin = ogmrip_plugin_find_container_by_type (container_plugins, container);
  if (!plugin)
    return FALSE;

  return ncodec <= plugin->max_audio;
}

/**
 * ogmrip_plugin_can_contain_n_subp:
 * @container: A container type
 * @ncodec: The number of subtitle codecs
 *
 * Returns whether @container can contain @ncodec subtitle streams.
 *
 * Returns: %TRUE if @container can contain @ncodec subtitle streams
 */
gboolean
ogmrip_plugin_can_contain_n_subp (GType container, guint ncodec)
{
  OGMRipContainerPlugin *plugin;

  g_return_val_if_fail (g_type_is_a (container, OGMRIP_TYPE_CONTAINER), FALSE);

  plugin = ogmrip_plugin_find_container_by_type (container_plugins, container);
  if (!plugin)
    return FALSE;

  return ncodec <= plugin->max_subp;
}

