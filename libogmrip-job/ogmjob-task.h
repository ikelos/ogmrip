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

#ifndef __OGMJOB_TASK_H__
#define __OGMJOB_TASK_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define OGMJOB_TYPE_TASK          (ogmjob_task_get_type ())
#define OGMJOB_TASK(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMJOB_TYPE_TASK, OGMJobTask))
#define OGMJOB_TASK_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMJOB_TYPE_TASK, OGMJobTaskClass))
#define OGMJOB_IS_TASK(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMJOB_TYPE_TASK))
#define OGMJOB_IS_TASK_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMJOB_TYPE_TASK))

typedef enum
{
  OGMJOB_STATE_IDLE,
  OGMJOB_STATE_RUNNING,
  OGMJOB_STATE_SUSPENDED
} OGMJobState;

typedef struct _OGMJobTask      OGMJobTask;
typedef struct _OGMJobTaskPriv  OGMJobTaskPriv;
typedef struct _OGMJobTaskClass OGMJobTaskClass;

struct _OGMJobTask
{
  GObject parent_instance;

  OGMJobTaskPriv *priv;
};

struct _OGMJobTaskClass
{
  GObjectClass parent_class;

  /* vtable */
  void     (* run_async)  (OGMJobTask          *task,
                           GCancellable        *cancellable,
                           GAsyncReadyCallback callback,
                           gpointer            user_data);
  gboolean (* run_finish) (OGMJobTask          *task,
                           GAsyncResult        *res,
                           GError              **error);
  gboolean (* run)        (OGMJobTask          *task,
                           GCancellable        *cancellable,
                           GError              **error);
  void     (* cancel)     (OGMJobTask          *task);
  void     (* suspend)    (OGMJobTask          *task);
  void     (* resume)     (OGMJobTask          *task);
};

GType    ogmjob_task_get_type     (void);
gint     ogmjob_task_get_state    (OGMJobTask          *task);
gdouble  ogmjob_task_get_progress (OGMJobTask          *task);
void     ogmjob_task_set_progress (OGMJobTask          *task,
                                   gdouble             progress);
void     ogmjob_task_run_async    (OGMJobTask          *task,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback callback,
                                   gpointer            user_data);
gboolean ogmjob_task_run_finish   (OGMJobTask          *task,
                                   GAsyncResult        *res,
                                   GError              **error);
gboolean ogmjob_task_run          (OGMJobTask          *task,
                                   GCancellable        *cancellable,
                                   GError              **error);
void     ogmjob_task_suspend      (OGMJobTask          *task);
void     ogmjob_task_resume       (OGMJobTask          *task);

G_END_DECLS

#endif /* __OGMJOB_TASK_H__ */

