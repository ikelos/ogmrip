/* OGMJob - A library to spawn processes
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

#ifndef __OGMJOB_SPAWN_H__
#define __OGMJOB_SPAWN_H__

#include <ogmjob-task.h>

#include <sys/types.h>
#include <sys/wait.h>

G_BEGIN_DECLS

#define OGMJOB_TYPE_SPAWN          (ogmjob_spawn_get_type ())
#define OGMJOB_SPAWN(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMJOB_TYPE_SPAWN, OGMJobSpawn))
#define OGMJOB_SPAWN_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMJOB_TYPE_SPAWN, OGMJobSpawnClass))
#define OGMJOB_IS_SPAWN(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMJOB_TYPE_SPAWN))
#define OGMJOB_IS_SPAWN_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMJOB_TYPE_SPAWN))

typedef struct _OGMJobSpawn      OGMJobSpawn;
typedef struct _OGMJobSpawnPriv  OGMJobSpawnPrivate;
typedef struct _OGMJobSpawnClass OGMJobSpawnClass;

/**
 * OGMJobWatch:
 * @spawn: An #OGMJobSpawn
 * @buffer: The data read
 * @data: The user data
 *
 * Specifies the type of functions passed to ogmjob_spawn_add_watch(), and
 * ogmjob_spawn_add_watch_full().
 *
 * Returns: %TRUE to stop watching
 */
typedef gboolean (* OGMJobWatch) (OGMJobSpawn *spawn,
                                  const gchar *buffer,
                                  gpointer    data,
                                  GError      **error);

struct _OGMJobSpawn
{
  OGMJobTask parent_instance;

  OGMJobSpawnPrivate *priv;
};

struct _OGMJobSpawnClass
{
  OGMJobTaskClass parent_class;
};

GType        ogmjob_spawn_get_type         (void);
OGMJobTask * ogmjob_spawn_new              (const gchar *command_line);
OGMJobTask * ogmjob_spawn_newv             (gchar       **argv);
gint         ogmjob_spawn_get_status       (OGMJobSpawn *spawn);
void         ogmjob_spawn_set_watch_stdout (OGMJobSpawn *spawn,
                                            OGMJobWatch watch_func,
                                            gpointer    watch_data);
void         ogmjob_spawn_set_watch_stderr (OGMJobSpawn *spawn,
                                            OGMJobWatch watch_func,
                                            gpointer    watch_data);

G_END_DECLS

#endif /* __OGMJOB_SPAWN_H__ */

