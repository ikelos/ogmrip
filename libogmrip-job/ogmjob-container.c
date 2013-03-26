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
 * SECTION:ogmjob-container
 * @title: OGMJobContainer
 * @include: ogmjob-container.h
 * @short_description: Base class for tasks which contain other tasks
 */

#include "ogmjob-container.h"

#define OGMJOB_CONTAINER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMJOB_TYPE_CONTAINER, OGMJobContainerClass))

enum
{
  ADD,
  REMOVE,
  LAST_SIGNAL
};

static void ogmjob_container_dispose     (GObject         *object);
static void ogmjob_container_suspend     (OGMJobTask      *task);
static void ogmjob_container_resume      (OGMJobTask      *task);
static void ogmjob_container_real_add    (OGMJobContainer *container, 
                                          OGMJobTask      *task);
static void ogmjob_container_real_remove (OGMJobContainer *container,
                                          OGMJobTask      *task);

static gint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_ABSTRACT_TYPE (OGMJobContainer, ogmjob_container, OGMJOB_TYPE_TASK)

static void
ogmjob_container_class_init (OGMJobContainerClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobTaskClass *task_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmjob_container_dispose;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->suspend = ogmjob_container_suspend;
  task_class->resume = ogmjob_container_resume;

  klass->add = ogmjob_container_real_add;
  klass->remove = ogmjob_container_real_remove;

  /**
   * OGMJobContainer::add
   * @container: the container that received the signal
   * @task: the task to be added
   *
   * Emitted each time a task is added to a container.
   */
  signals[ADD] = g_signal_new ("add", G_TYPE_FROM_CLASS (gobject_class),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 
      G_STRUCT_OFFSET (OGMJobContainerClass, add), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, OGMJOB_TYPE_TASK);

  /**
   * OGMJobContainer::remove
   * @container: the container that received the signal
   * @task: the task to be removed
   *
   * Emitted each time a task is removed from a container.
   */
  signals[REMOVE] = g_signal_new ("remove", G_TYPE_FROM_CLASS (gobject_class),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 
      G_STRUCT_OFFSET (OGMJobContainerClass, remove), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, OGMJOB_TYPE_TASK);
}

static void
ogmjob_container_init (OGMJobContainer *container)
{
}

static void
ogmjob_container_dispose (GObject *gobject)
{
  ogmjob_container_foreach (OGMJOB_CONTAINER (gobject), 
      (OGMJobCallback) g_object_unref, NULL);

  G_OBJECT_CLASS (ogmjob_container_parent_class)->dispose (gobject);
}

static void
ogmjob_container_suspend (OGMJobTask *task)
{
  ogmjob_container_foreach (OGMJOB_CONTAINER (task), 
      (OGMJobCallback) ogmjob_task_suspend, NULL);
}

static void
ogmjob_container_resume (OGMJobTask *task)
{
  ogmjob_container_foreach (OGMJOB_CONTAINER (task), 
      (OGMJobCallback) ogmjob_task_resume, NULL);
}

static void
ogmjob_container_real_add (OGMJobContainer *container, OGMJobTask *task)
{
  g_warning ("Not implemented\n");
}

static void
ogmjob_container_real_remove (OGMJobContainer *container, OGMJobTask *task)
{
  g_warning ("Not implemented\n");
}

/**
 * ogmjob_container_add:
 * @container: An #OGMJobContainer
 * @task: An #OGMJobTask
 *
 * Adds @task to @container.
 */
void
ogmjob_container_add (OGMJobContainer *container, OGMJobTask *task)
{
  g_return_if_fail (OGMJOB_IS_CONTAINER (container));
  g_return_if_fail (OGMJOB_IS_TASK (task));

  g_signal_emit (container, signals[ADD], 0, task);
}

/**
 * ogmjob_container_remove:
 * @container: An #OGMJobContainer
 * @task: An #OGMJobTask
 *
 * Removes @task from @container.
 */
void
ogmjob_container_remove (OGMJobContainer *container, OGMJobTask *task)
{
  g_return_if_fail (OGMJOB_IS_CONTAINER (container));
  g_return_if_fail (OGMJOB_IS_TASK (task));

  g_signal_emit (container, signals[REMOVE], 0, task);
}

/**
 * ogmjob_container_foreach:
 * @container: An #OGMJobContainer
 * @callback: A callback
 * @data: Callback user data
 *
 * Invokes @callback on each task of @container.
 */
void
ogmjob_container_foreach (OGMJobContainer *container, OGMJobCallback callback, gpointer data)
{
  OGMJobContainerClass *klass;

  g_return_if_fail (OGMJOB_IS_CONTAINER (container));

  klass = OGMJOB_CONTAINER_GET_CLASS (container);

  if (klass->forall)
    (* klass->forall) (container, callback, data);
}

