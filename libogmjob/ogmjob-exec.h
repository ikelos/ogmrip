/* OGMJob - A library to spawn processes
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

#ifndef __OGMJOB_EXEC_H__
#define __OGMJOB_EXEC_H__

#include <ogmjob-spawn.h>

G_BEGIN_DECLS

#define OGMJOB_TYPE_EXEC          (ogmjob_exec_get_type ())
#define OGMJOB_EXEC(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMJOB_TYPE_EXEC, OGMJobExec))
#define OGMJOB_EXEC_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMJOB_TYPE_EXEC, OGMJobExecClass))
#define OGMJOB_IS_EXEC(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMJOB_TYPE_EXEC))
#define OGMJOB_IS_EXEC_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMJOB_TYPE_EXEC))

typedef struct _OGMJobExec      OGMJobExec;
typedef struct _OGMJobExecPriv  OGMJobExecPriv;
typedef struct _OGMJobExecClass OGMJobExecClass;

/**
 * OGMJobWatch:
 * @exec: An #OGMJobExec
 * @buffer: The data read
 * @data: The user data
 *
 * Specifies the type of functions passed to ogmjob_exec_add_watch(), and
 * ogmjob_exec_add_watch_full().
 *
 * Returns: The progress made, or -1
 */
typedef gdouble (* OGMJobWatch) (OGMJobExec  *exec,
                                 const gchar *buffer,
                                 gpointer    data);

struct _OGMJobExec
{
  OGMJobSpawn parent_instance;

  OGMJobExecPriv *priv;
};

struct _OGMJobExecClass
{
  OGMJobSpawnClass parent_class;
};

GType         ogmjob_exec_get_type       (void);

OGMJobSpawn * ogmjob_exec_new            (const gchar *command_line);
OGMJobSpawn * ogmjob_exec_newv           (gchar       **argv);

gint          ogmjob_exec_get_status     (OGMJobExec  *exec);

void          ogmjob_exec_add_watch      (OGMJobExec  *exec,
                                          OGMJobWatch watch_func,
                                          gpointer    watch_data);
void          ogmjob_exec_add_watch_full (OGMJobExec  *exec,
                                          OGMJobWatch watch_func,
                                          gpointer    watch_data,
                                          gboolean    watch_out,
                                          gboolean    watch_err,
                                          gboolean    swapped);

G_END_DECLS

#endif /* __OGMJOB_EXEC_H__ */

