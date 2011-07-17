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

#ifndef __OGMRIP_OPTIONS_PLUGIN_H__
#define __OGMRIP_OPTIONS_PLUGIN_H__

#include <ogmrip.h>

#include <gmodule.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_PLUGIN_DIALOG            (ogmrip_plugin_dialog_get_type ())
#define OGMRIP_PLUGIN_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_PLUGIN_DIALOG, OGMRipPluginDialog))
#define OGMRIP_PLUGIN_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_PLUGIN_DIALOG, OGMRipPluginDialogClass))
#define OGMRIP_IS_PLUGIN_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_PLUGIN_DIALOG))
#define OGMRIP_IS_PLUGIN_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_PLUGIN_DIALOG))

typedef struct _OGMRipPluginDialog      OGMRipPluginDialog;
typedef struct _OGMRipPluginDialogClass OGMRipPluginDialogClass;
typedef struct _OGMRipPluginDialogPriv  OGMRipPluginDialogPriv;

struct _OGMRipPluginDialog
{
  GtkDialog parent_instance;

  OGMRipPluginDialogPriv *priv;
};

struct _OGMRipPluginDialogClass
{
  GtkDialogClass parent_class;
};

/**
 * OGMRipContainerOptionsPlugin:
 * @module: For internal use only
 * @dialog: The type of the dialog
 * @type: The type of the associated container
 *
 * A structure describing an options plugin for a container
 */

typedef struct _OGMRipContainerOptionsPlugin OGMRipContainerOptionsPlugin;

struct _OGMRipContainerOptionsPlugin
{
  GModule *module;
  /*< public >*/
  GType dialog;
  GType type;
};

/**
 * OGMRipVideoOptionsPlugin:
 * @module: For internal use only
 * @dialog: The type of the dialog
 * @type: The type of the associated video codec
 *
 * A structure describing an options plugin for a video codec
 */

typedef struct _OGMRipVideoOptionsPlugin OGMRipVideoOptionsPlugin;

struct _OGMRipVideoOptionsPlugin
{
  GModule *module;
  /*< public >*/
  GType dialog;
  GType type;
};

/**
 * OGMRipAudioOptionsPlugin:
 * @module: For internal use only
 * @dialog: The type of the dialog
 * @type: The type of the associated video codec
 *
 * A structure describing an options plugin for a video codec
 */

typedef struct _OGMRipAudioOptionsPlugin OGMRipAudioOptionsPlugin;

struct _OGMRipAudioOptionsPlugin
{
  GModule *module;
  /*< public >*/
  GType dialog;
  GType type;
};

/**
 * OGMRipSubpOptionsPlugin:
 * @module: For internal use only
 * @dialog: The type of the dialog
 * @type: The type of the associated video codec
 *
 * A structure describing an options plugin for a video codec
 */

typedef struct _OGMRipSubpOptionsPlugin OGMRipSubpOptionsPlugin;

struct _OGMRipSubpOptionsPlugin
{
  GModule *module;
  /*< public >*/
  GType dialog;
  GType type;
};

GType           ogmrip_plugin_dialog_get_type                 (void);
OGMRipProfile * ogmrip_plugin_dialog_get_profile              (OGMRipPluginDialog *dialog);

void            ogmrip_options_plugin_init                    (void);
void            ogmrip_options_plugin_uninit                  (void);

gboolean        ogmrip_options_plugin_exists                  (GType              type);

GtkWidget *     ogmrip_container_options_plugin_dialog_new    (GType              type,
                                                               OGMRipProfile      *profile);
GtkWidget *     ogmrip_video_options_plugin_dialog_new        (GType              type,
                                                               OGMRipProfile      *profile);
GtkWidget *     ogmrip_audio_options_plugin_dialog_new        (GType              type, 
                                                               OGMRipProfile      *profile);
GtkWidget *     ogmrip_subp_options_plugin_dialog_new         (GType              type,
                                                               OGMRipProfile      *profile);

G_END_DECLS

#endif /* __OGMRIP_OPTIONS_PLUGIN_H__ */

