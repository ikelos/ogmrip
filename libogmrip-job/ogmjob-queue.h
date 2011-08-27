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

#ifndef __OGMJOB_QUEUE_H__
#define __OGMJOB_QUEUE_H__

#include <ogmjob-list.h>

G_BEGIN_DECLS

#define OGMJOB_TYPE_QUEUE           (ogmjob_queue_get_type ())
#define OGMJOB_QUEUE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMJOB_TYPE_QUEUE, OGMJobQueue))
#define OGMJOB_QUEUE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), OGMJOB_TYPE_QUEUE, OGMJobQueueClass))
#define OGMJOB_IS_QUEUE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMJOB_TYPE_QUEUE))
#define OGMJOB_IS_QUEUE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMJOB_TYPE_QUEUE))
#define OGMJOB_QUEUE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMJOB_TYPE_QUEUE, OGMJobQueueClass))

typedef struct _OGMJobQueue      OGMJobQueue;
typedef struct _OGMJobQueuePriv  OGMJobQueuePriv;
typedef struct _OGMJobQueueClass OGMJobQueueClass;

struct _OGMJobQueue
{
  OGMJobList parent_instance;

  OGMJobQueuePriv *priv;
};

struct _OGMJobQueueClass
{
  OGMJobListClass parent_class;
};

GType         ogmjob_queue_get_type (void);
OGMJobSpawn * ogmjob_queue_new      (void);

G_END_DECLS

#endif /* __OGMJOB_QUEUE_H__ */

