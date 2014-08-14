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

#ifndef __OGMRIP_MODULE_OBJECT_H__
#define __OGMRIP_MODULE_OBJECT_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_MODULE         (ogmrip_module_get_type ())
#define OGMRIP_MODULE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OGMRIP_TYPE_MODULE, OGMRipModule))
#define OGMRIP_MODULE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), OGMRIP_TYPE_MODULE, OGMRipModule))
#define OGMRIP_IS_MODULE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OGMRIP_TYPE_MODULE))
#define OGMRIP_IS_MODULE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), OGMRIP_TYPE_MODULE))
#define OGMRIP_MODULE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), OGMRIP_TYPE_MODULE, OGMRipModuleClass))

typedef struct _OGMRipModule       OGMRipModule;
typedef struct _OGMRipModuleClass  OGMRipModuleClass;
typedef struct _OGMRipModulePriv   OGMRipModulePrivate;

struct _OGMRipModule
{
  GTypeModule parent_instance;

  OGMRipModulePrivate *priv;
};

struct _OGMRipModuleClass
{
  GTypeModuleClass parent_class;
};

GType          ogmrip_module_get_type   (void) G_GNUC_CONST;
OGMRipModule * ogmrip_module_new        (const gchar  *path,
                                         const gchar  *name);
const gchar *  ogmrip_module_get_name   (OGMRipModule *module);
const gchar *  ogmrip_module_get_path   (OGMRipModule *module);
gboolean       ogmrip_module_get_symbol (OGMRipModule *module,
                                         const gchar  *name,
                                         gpointer     *symbol);

void           ogmrip_module_load       (OGMRipModule *module);

G_END_DECLS

#endif /* __OGMRIP_MODULE_OBJECT_H__ */

