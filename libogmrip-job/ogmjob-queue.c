/* OGMJob - A library to spawn processes
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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

/**
 * SECTION:ogmjob-queue
 * @title: OGMJobQueue
 * @include: ogmjob-queue.h
 * @short_description: A container to run tasks in order
 */

#include "ogmjob-queue.h"

#include <sys/types.h>
#include <sys/wait.h>

struct _OGMJobQueuePriv
{
  GList *children;
  guint nchildren;
  guint completed;
  gdouble progress;
};

static gboolean
ogmjob_queue_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobQueue *queue = OGMJOB_QUEUE (task);
  GList *child;
  gboolean result;

  if (!queue->priv->children)
    return TRUE;

  queue->priv->nchildren = g_list_length (queue->priv->children);
  queue->priv->completed = 0;

  for (child = queue->priv->children; child; child = child->next)
  {
    result = ogmjob_task_run (child->data, cancellable, error);
    if (!result)
      break;

    queue->priv->completed ++;
  }

  return result;
}

static void
ogmjob_queue_child_progress (OGMJobQueue *queue, GParamSpec *pspec, OGMJobTask *task)
{
  ogmjob_task_set_progress (OGMJOB_TASK (queue),
      (queue->priv->completed + ogmjob_task_get_progress (task)) / (gdouble) queue->priv->nchildren);
}

static void
ogmjob_queue_add (OGMJobContainer *container, OGMJobTask *task)
{
  OGMJobQueue *queue = OGMJOB_QUEUE (container);

  queue->priv->children = g_list_append (queue->priv->children, g_object_ref (task));

  g_signal_connect_swapped (task, "notify::progress", 
      G_CALLBACK (ogmjob_queue_child_progress), container);
}

static void
ogmjob_queue_remove (OGMJobContainer *container, OGMJobTask *task)
{
  OGMJobQueue *queue = OGMJOB_QUEUE (container);
  GList *link;

  link = g_list_find (queue->priv->children, task);
  if (link)
  {
    g_signal_handlers_disconnect_by_func (task, ogmjob_queue_child_progress, container);

    g_object_unref (task);
    queue->priv->children = g_list_remove_link (queue->priv->children, link);
    g_list_free (link);
  }
}

static void
ogmjob_queue_forall (OGMJobContainer *container, OGMJobCallback callback, gpointer data)
{
  GList *children = OGMJOB_QUEUE (container)->priv->children;
  OGMJobTask *child;

  while (children)
  {
    child = children->data;
    children = children->next;

    (* callback) (child, data);
  }
}

G_DEFINE_TYPE (OGMJobQueue, ogmjob_queue, OGMJOB_TYPE_CONTAINER)

static void
ogmjob_queue_class_init (OGMJobQueueClass *klass)
{
  OGMJobTaskClass *task_class;
  OGMJobContainerClass *container_class;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmjob_queue_run;

  container_class = OGMJOB_CONTAINER_CLASS (klass);
  container_class->add = ogmjob_queue_add;
  container_class->remove = ogmjob_queue_remove;
  container_class->forall = ogmjob_queue_forall;

  g_type_class_add_private (klass, sizeof (OGMJobQueuePriv));
}

static void
ogmjob_queue_init (OGMJobQueue *queue)
{
  queue->priv = G_TYPE_INSTANCE_GET_PRIVATE (queue, OGMJOB_TYPE_QUEUE, OGMJobQueuePriv);
}

/**
 * ogmjob_queue_new:
 *
 * Creates a new #OGMJobQueue.
 *
 * Returns: The new #OGMJobQueue
 */
OGMJobTask *
ogmjob_queue_new (void)
{
  return g_object_new (OGMJOB_TYPE_QUEUE, NULL);
}

