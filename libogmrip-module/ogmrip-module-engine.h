/* ogmrip - Create .tag files
 * Copyright (C) 2010 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_MODULE_ENGINE_H__
#define __OGMRIP_MODULE_ENGINE_H__

#include <ogmrip-module-object.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_MODULE_ENGINE         (ogmrip_module_engine_get_type ())
#define OGMRIP_MODULE_ENGINE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OGMRIP_TYPE_MODULE_ENGINE, OGMRipModuleEngine))
#define OGMRIP_MODULE_ENGINE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), OGMRIP_TYPE_MODULE_ENGINE, OGMRipModuleEngine))
#define OGMRIP_IS_MODULE_ENGINE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OGMRIP_TYPE_MODULE_ENGINE))
#define OGMRIP_IS_MODULE_ENGINE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), OGMRIP_TYPE_MODULE_ENGINE))
#define OGMRIP_MODULE_ENGINE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), OGMRIP_TYPE_MODULE_ENGINE, OGMRipModuleEngineClass))

typedef struct _OGMRipModuleEngine       OGMRipModuleEngine;
typedef struct _OGMRipModuleEngineClass  OGMRipModuleEngineClass;
typedef struct _OGMRipModuleEnginePriv   OGMRipModuleEnginePriv;

struct _OGMRipModuleEngine
{
  GObject parent_instance;

  OGMRipModuleEnginePriv *priv;
};

struct _OGMRipModuleEngineClass
{
  GObjectClass parent_class;
};

GType                ogmrip_module_engine_get_type    (void) G_GNUC_CONST;
OGMRipModuleEngine * ogmrip_module_engine_get_default (void);
gboolean             ogmrip_module_engine_add_path    (OGMRipModuleEngine  *engine,
                                                       const gchar         *path,
                                                       GError              **error);
OGMRipModule *       ogmrip_module_engine_get         (OGMRipModuleEngine  *engine,
                                                       const gchar         *name);
GSList *             ogmrip_module_engine_get_list    (OGMRipModuleEngine  *engine);

G_END_DECLS

#endif /* __OGMRIP_MODULE_ENGINE_H__ */

