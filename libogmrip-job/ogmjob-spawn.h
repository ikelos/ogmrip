/* OGMJob - A library to spawn processes
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMJOB_SPAWN_H__
#define __OGMJOB_SPAWN_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define OGMJOB_TYPE_SPAWN          (ogmjob_spawn_get_type ())
#define OGMJOB_SPAWN(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMJOB_TYPE_SPAWN, OGMJobSpawn))
#define OGMJOB_SPAWN_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMJOB_TYPE_SPAWN, OGMJobSpawnClass))
#define OGMJOB_IS_SPAWN(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMJOB_TYPE_SPAWN))
#define OGMJOB_IS_SPAWN_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMJOB_TYPE_SPAWN))
#define OGMJOB_SPAWN_ERROR         (ogmjob_spawn_error_quark ())

/**
 * OGMJobResultType:
 * @OGMJOB_RESULT_ERROR: An error occured
 * @OGMJOB_RESULT_CANCEL: The spawn has been canceled
 * @OGMJOB_RESULT_COMPLETED:  The spawn has completed successfully
 *
 * Result codes returned by ogmjob_spawn_run()
 */
typedef enum
{
  OGMJOB_RESULT_ERROR   = -1,
  OGMJOB_RESULT_CANCEL  =  0,
  OGMJOB_RESULT_SUCCESS =  1
} OGMJobResultType;

typedef struct _OGMJobSpawn      OGMJobSpawn;
typedef struct _OGMJobSpawnPriv  OGMJobSpawnPriv;
typedef struct _OGMJobSpawnClass OGMJobSpawnClass;

struct _OGMJobSpawn
{
  GObject parent_instance;

  OGMJobSpawnPriv *priv;
};

struct _OGMJobSpawnClass
{
  GObjectClass parent_class;

  gint (* run)      (OGMJobSpawn *spawn);
  void (* cancel)   (OGMJobSpawn *spawn);
  void (* progress) (OGMJobSpawn *spawn,
                     gdouble     fraction);
  void (* suspend)  (OGMJobSpawn *spawn);
  void (* resume)   (OGMJobSpawn *spawn);
};

GType         ogmjob_spawn_get_type        (void);
GQuark        ogmjob_spawn_error_quark     (void);

gint          ogmjob_spawn_run             (OGMJobSpawn  *spawn,
                                            GError       **error);
void          ogmjob_spawn_cancel          (OGMJobSpawn  *spawn);

void          ogmjob_spawn_suspend         (OGMJobSpawn  *spawn);
void          ogmjob_spawn_resume          (OGMJobSpawn  *spawn);

void          ogmjob_spawn_set_async       (OGMJobSpawn  *spawn,
                                            gboolean     async);
gboolean      ogmjob_spawn_get_async       (OGMJobSpawn  *spawn);

OGMJobSpawn * ogmjob_spawn_get_parent      (OGMJobSpawn *spawn);
void          ogmjob_spawn_set_parent      (OGMJobSpawn *spawn,
                                           OGMJobSpawn  *parent);
void          ogmjob_spawn_propagate_error (OGMJobSpawn  *spawn,
                                            GError       *error);

G_END_DECLS

#endif /* __OGMJOB_SPAWN_H__ */

