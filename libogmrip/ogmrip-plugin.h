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

#ifndef __OGMRIP_PLUGIN_H__
#define __OGMRIP_PLUGIN_H__

#include <ogmrip-enums.h>

#include <glib-object.h>
#include <gmodule.h>

G_BEGIN_DECLS

#define OGMRIP_PLUGIN_ERROR ogmrip_plugin_error_quark ()

/**
 * OGMRipPluginError:
 * @OGMRIP_PLUGIN_ERROR_REQ: Some requirements are not met
 *
 * Error codes returned by ogmrip_init_plugin().
 */
typedef enum
{
  OGMRIP_PLUGIN_ERROR_REQ
} OGMRipPluginError;

/**
 * OGMRipPluginFunc:
 * @type: The type of the plugin
 * @name: The name of the plugin
 * @description: The description of the plugin
 * @data: The user data
 *
 * Specifies the type of functions passed to ogmrip_plugin_foreach_container(),
 * ogmrip_plugin_foreach_video_codec(), ogmrip_plugin_foreach_audio_codec(),
 * and ogmrip_plugin_foreach_subp_codec().
 */
typedef void (* OGMRipPluginFunc)    (GType         type,
                                      const gchar   *name,
                                      const gchar   *description,
                                      gpointer      data);

/**
 * OGMRipPluginCmpFunc:
 * @type: The type of the plugin
 * @name: The name of the plugin
 * @description: The description of the plugin
 * @data: The user data
 *
 * Specifieѕ the type of functions passed to ogmrip_plugin_find_container(),
 * ogmrip_plugin_find_video_codec(), ogmrip_plugin_find_audio_codec(), and
 * ogmrip_plugin_find_subp_codec().
 *
 * Returns: 0 when the expected plugin is found
 */
typedef gint (* OGMRipPluginCmpFunc) (GType         type,
                                      const gchar   *name,
                                      const gchar   *description,
                                      gconstpointer data);

/**
 * OGMRipVideoPlugin:
 * @module: For internal use only
 * @type: The type of the codec
 * @name: The name of the codec
 * @description: The description of the codec
 * @format: The codec output format
 * @passes: The number of passes supported by the codec
 * @threads: The number of threads supported by the codec
 *
 * A structure describing a video codec plugin
 */

typedef struct _OGMRipVideoPlugin OGMRipVideoPlugin;

struct _OGMRipVideoPlugin
{
  GModule *module;
  /*< public >*/
  GType type;
  gchar *name;
  gchar *description;
  OGMRipFormatType format;
  gint passes;
  gint threads;
};

/**
 * OGMRipAudioPlugin:
 * @module: For internal use only
 * @type: The type of the codec
 * @name: The name of the codec
 * @description: The description of the codec
 * @format: The codec output format
 *
 * A structure describing an audio codec plugin
 */

typedef struct _OGMRipAudioPlugin OGMRipAudioPlugin;

struct _OGMRipAudioPlugin
{
  GModule *module;
  /*< public >*/
  GType type;
  gchar *name;
  gchar *description;
  OGMRipFormatType format;
};

/**
 * OGMRipSubpPlugin:
 * @module: For internal use only
 * @type: The type of the codec
 * @name: The name of the codec
 * @description: The description of the codec
 * @format: The codec output format
 * @text: Whether the codec outputs text subtitles
 *
 * A structure describing a subtitle codec plugin
 */

typedef struct _OGMRipSubpPlugin  OGMRipSubpPlugin;

struct _OGMRipSubpPlugin
{
  GModule *module;
  /*< public >*/
  GType type;
  gchar *name;
  gchar *description;
  OGMRipFormatType format;
  gboolean text;
};

/**
 * OGMRipContainerPlugin:
 * @module: For internal use only
 * @type: The type of the container
 * @name: The name of the container
 * @description: The description of the container
 * @video: Whether the container requires a video stream
 * @bframes: Whether the container supports B-frames
 * @max_audio: The maximum number of audio streams that can be embedded in the container
 * @max_subp: The maximum number of subtitle streams that can be embedded in the container
 * @formats: A NULL terminated array of #OGMRipFormatType supported by the container
 *
 * A structure describing a container plugin
 */

typedef struct _OGMRipContainerPlugin  OGMRipContainerPlugin;

struct _OGMRipContainerPlugin
{
  GModule *module;
  /*< public >*/
  GType type;
  gchar *name;
  gchar *description;
  gboolean video;
  gboolean bframes;
  gint max_audio;
  gint max_subp;
  gint *formats;
};

void     ogmrip_plugin_init        (void);
void     ogmrip_plugin_uninit      (void);
GQuark   ogmrip_plugin_error_quark (void);

/*
 * Container functions
 */

gint      ogmrip_plugin_get_n_containers        (void);
void      ogmrip_plugin_foreach_container       (OGMRipPluginFunc    func,
                                                 gpointer             data);
GType     ogmrip_plugin_find_container          (OGMRipPluginCmpFunc func,
                                                 gconstpointer        data);

GType     ogmrip_plugin_get_nth_container       (guint n);
GType     ogmrip_plugin_get_container_by_name   (const gchar *name);
gint      ogmrip_plugin_get_container_index     (GType container);
G_CONST_RETURN
gchar *   ogmrip_plugin_get_container_name      (GType container);
gboolean  ogmrip_plugin_get_container_bframes   (GType container);
gint      ogmrip_plugin_get_container_max_audio (GType container);
gint      ogmrip_plugin_get_container_max_subp  (GType container);
GModule * ogmrip_plugin_get_container_module    (GType container);

/*
 * Video codec functions
 */

gint      ogmrip_plugin_get_n_video_codecs      (void);
void      ogmrip_plugin_foreach_video_codec     (OGMRipPluginFunc    func,
                                                 gpointer             data);
GType     ogmrip_plugin_find_video_codec        (OGMRipPluginCmpFunc func,
                                                 gconstpointer        data);

GType     ogmrip_plugin_get_nth_video_codec     (guint n);
GType     ogmrip_plugin_get_video_codec_by_name (const gchar *name);
gint      ogmrip_plugin_get_video_codec_index   (GType codec);
G_CONST_RETURN
gchar *   ogmrip_plugin_get_video_codec_name    (GType codec);
gint      ogmrip_plugin_get_video_codec_format  (GType codec);
gint      ogmrip_plugin_get_video_codec_passes  (GType codec);
gint      ogmrip_plugin_get_video_codec_threads (GType codec);
GModule * ogmrip_plugin_get_video_codec_module  (GType codec);

/*
 * Audio codec functions
 */

gint     ogmrip_plugin_get_n_audio_codecs      (void);
void     ogmrip_plugin_foreach_audio_codec     (OGMRipPluginFunc    func,
                                                gpointer             data);
GType    ogmrip_plugin_find_audio_codec        (OGMRipPluginCmpFunc func,
                                                gconstpointer        data);

GType     ogmrip_plugin_get_nth_audio_codec     (guint n);
GType     ogmrip_plugin_get_audio_codec_by_name (const gchar *name);
gint      ogmrip_plugin_get_audio_codec_index   (GType codec);
G_CONST_RETURN
gchar *   ogmrip_plugin_get_audio_codec_name    (GType codec);
gint      ogmrip_plugin_get_audio_codec_format  (GType codec);
GModule * ogmrip_plugin_get_audio_codec_module  (GType codec);

/*
 * Subp codec functions
 */

gint     ogmrip_plugin_get_n_subp_codecs      (void);
void     ogmrip_plugin_foreach_subp_codec     (OGMRipPluginFunc    func,
                                               gpointer             data);
GType    ogmrip_plugin_find_subp_codec        (OGMRipPluginCmpFunc func,
                                               gconstpointer        data);

GType     ogmrip_plugin_get_nth_subp_codec     (guint n);
GType     ogmrip_plugin_get_subp_codec_by_name (const gchar *name);
gint      ogmrip_plugin_get_subp_codec_index   (GType codec);
G_CONST_RETURN
gchar *   ogmrip_plugin_get_subp_codec_name    (GType codec);
gint      ogmrip_plugin_get_subp_codec_format  (GType codec);
gboolean  ogmrip_plugin_get_subp_codec_text    (GType codec);
GModule * ogmrip_plugin_get_subp_codec_module  (GType codec);

/*
 * Compatibility functions
 */

gboolean ogmrip_plugin_can_contain_format  (GType            container,
                                            OGMRipFormatType format);
gboolean ogmrip_plugin_can_contain_video   (GType            container,
                                            GType            codec);
gboolean ogmrip_plugin_can_contain_audio   (GType            container,
                                            GType            codec);
gboolean ogmrip_plugin_can_contain_subp    (GType            container,
                                            GType            codec);

gboolean ogmrip_plugin_can_contain_n_audio (GType            container,
                                            guint            ncodec);
gboolean ogmrip_plugin_can_contain_n_subp  (GType            container,
                                            guint            ncodec);

G_END_DECLS

#endif /* __OGMRIP_PLUGIN_H__ */

