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

/**
 * SECTION:ogmjob-container
 * @title: OGMJobContainer
 * @include: ogmjob-container.h
 * @short_description: Base class for spawns which contain other spawns
 */

#include "ogmjob-container.h"

#define OGMJOB_CONTAINER_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMJOB_TYPE_CONTAINER, OGMJobContainerPriv))

enum
{
  ADD,
  REMOVE,
  LAST_SIGNAL
};

static void ogmjob_container_dispose     (GObject         *object);
static void ogmjob_container_cancel      (OGMJobSpawn     *spawn);
static void ogmjob_container_suspend     (OGMJobSpawn     *spawn);
static void ogmjob_container_resume      (OGMJobSpawn     *spawn);
static void ogmjob_container_real_add    (OGMJobContainer *container, 
                                          OGMJobSpawn     *child);
static void ogmjob_container_real_remove (OGMJobContainer *container,
                                          OGMJobSpawn     *spawn);

static gint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_ABSTRACT_TYPE (OGMJobContainer, ogmjob_container, OGMJOB_TYPE_SPAWN)

static void
ogmjob_container_class_init (OGMJobContainerClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  spawn_class = OGMJOB_SPAWN_CLASS (klass);

  gobject_class->dispose = ogmjob_container_dispose;

  spawn_class->cancel = ogmjob_container_cancel;
  spawn_class->suspend = ogmjob_container_suspend;
  spawn_class->resume = ogmjob_container_resume;

  klass->add = ogmjob_container_real_add;
  klass->remove = ogmjob_container_real_remove;

  /**
   * OGMJobContainer::add
   * @container: the container that received the signal
   * @child: the child to be added
   *
   * Emitted each time a child is added to a container.
   */
  signals[ADD] = g_signal_new ("add", G_TYPE_FROM_CLASS (gobject_class),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 
      G_STRUCT_OFFSET (OGMJobContainerClass, add), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, OGMJOB_TYPE_SPAWN);

  /**
   * OGMJobContainer::remove
   * @container: the container that received the signal
   * @child: the child to be removed
   *
   * Emitted each time a child is removed from a container.
   */
  signals[REMOVE] = g_signal_new ("remove", G_TYPE_FROM_CLASS (gobject_class),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 
      G_STRUCT_OFFSET (OGMJobContainerClass, remove), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, OGMJOB_TYPE_SPAWN);
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
ogmjob_container_cancel (OGMJobSpawn *spawn)
{
  ogmjob_container_foreach (OGMJOB_CONTAINER (spawn), 
      (OGMJobCallback) ogmjob_spawn_cancel, NULL);
}

static void
ogmjob_container_suspend (OGMJobSpawn *spawn)
{
  ogmjob_container_foreach (OGMJOB_CONTAINER (spawn), 
      (OGMJobCallback) ogmjob_spawn_suspend, NULL);
}

static void
ogmjob_container_resume (OGMJobSpawn *spawn)
{
  ogmjob_container_foreach (OGMJOB_CONTAINER (spawn), 
      (OGMJobCallback) ogmjob_spawn_resume, NULL);
}

static void
ogmjob_container_real_add (OGMJobContainer *container, OGMJobSpawn *spawn)
{
  g_warning ("Not implemented\n");
}

static void
ogmjob_container_real_remove (OGMJobContainer *container, OGMJobSpawn *spawn)
{
  g_warning ("Not implemented\n");
}

/**
 * ogmjob_container_add:
 * @container: An #OGMJobContainer
 * @spawn: An #OGMJobSpawn
 *
 * Adds @spawn to @container.
 */
void
ogmjob_container_add (OGMJobContainer *container, OGMJobSpawn *spawn)
{
  g_return_if_fail (OGMJOB_IS_CONTAINER (container));
  g_return_if_fail (OGMJOB_IS_SPAWN (spawn));

  if (ogmjob_spawn_get_parent (spawn))
    g_warning ("Can't add a spawn which is already in a container.\n");
  else
  {
    g_signal_emit (container, signals[ADD], 0, spawn);
    ogmjob_spawn_set_parent (spawn, OGMJOB_SPAWN (container));
  }
}

/**
 * ogmjob_container_remove:
 * @container: An #OGMJobContainer
 * @spawn: An #OGMJobSpawn
 *
 * Removes @spawn from @container.
 */
void
ogmjob_container_remove (OGMJobContainer *container, OGMJobSpawn *spawn)
{
  g_return_if_fail (OGMJOB_IS_CONTAINER (container));
  g_return_if_fail (OGMJOB_IS_SPAWN (spawn));

  ogmjob_spawn_set_parent (spawn, NULL);

  g_signal_emit (container, signals[REMOVE], 0, spawn);
}

/**
 * ogmjob_container_foreach:
 * @container: An #OGMJobContainer
 * @callback: A callback
 * @data: Callback user data
 *
 * Invokes @callback on each child of @container.
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

