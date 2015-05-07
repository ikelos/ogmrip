/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_PROFILE_ENGINE_H__
#define __OGMRIP_PROFILE_ENGINE_H__

#include <ogmrip-profile.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_PROFILE_ENGINE         (ogmrip_profile_engine_get_type ())
#define OGMRIP_PROFILE_ENGINE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OGMRIP_TYPE_PROFILE_ENGINE, OGMRipProfileEngine))
#define OGMRIP_PROFILE_ENGINE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), OGMRIP_TYPE_PROFILE_ENGINE, OGMRipProfileEngine))
#define OGMRIP_IS_PROFILE_ENGINE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OGMRIP_TYPE_PROFILE_ENGINE))
#define OGMRIP_IS_PROFILE_ENGINE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), OGMRIP_TYPE_PROFILE_ENGINE))
#define OGMRIP_PROFILE_ENGINE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), OGMRIP_TYPE_PROFILE_ENGINE, OGMRipProfileEngineClass))

typedef struct _OGMRipProfileEngine       OGMRipProfileEngine;
typedef struct _OGMRipProfileEngineClass  OGMRipProfileEngineClass;
typedef struct _OGMRipProfileEnginePriv   OGMRipProfileEnginePrivate;

struct _OGMRipProfileEngine
{
  GObject parent_instance;

  OGMRipProfileEnginePrivate *priv;
};

struct _OGMRipProfileEngineClass
{
  GObjectClass parent_class;

  void (* add)    (OGMRipProfileEngine *engine,
                   OGMRipProfile       *profile);
  void (* remove) (OGMRipProfileEngine *engine,
                   OGMRipProfile       *profile);
  void (* update) (OGMRipProfileEngine *engine,
                   OGMRipProfile       *profile);
};

GType                 ogmrip_profile_engine_get_type    (void) G_GNUC_CONST;
OGMRipProfileEngine * ogmrip_profile_engine_get_default (void);
void                  ogmrip_profile_engine_add_path    (OGMRipProfileEngine *engine,
                                                         const gchar         *path);
OGMRipProfile *       ogmrip_profile_engine_get         (OGMRipProfileEngine *engine,
                                                         const gchar         *name);
void                  ogmrip_profile_engine_add         (OGMRipProfileEngine *engine,
                                                         OGMRipProfile       *profile);
void                  ogmrip_profile_engine_remove      (OGMRipProfileEngine *engine,
                                                         OGMRipProfile       *profile);
void                  ogmrip_profile_engine_update      (OGMRipProfileEngine *engine,
                                                         OGMRipProfile       *profile);
GSList *              ogmrip_profile_engine_get_list    (OGMRipProfileEngine *engine);

G_END_DECLS

#endif /* __OGMRIP_PROFILE_ENGINE_H__ */

