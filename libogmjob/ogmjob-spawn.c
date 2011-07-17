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

/**
 * SECTION:ogmjob-spawn
 * @title: OGMJobSpawn
 * @include: ogmjob-spawn.h
 * @short_description: Base class for all spawns
 */

#include "ogmjob-spawn.h"
#include "ogmjob-marshal.h"
#include "ogmjob-log.h"

#define OGMJOB_SPAWN_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMJOB_TYPE_SPAWN, OGMJobSpawnPriv))

struct _OGMJobSpawnPriv
{
  gint result;
  gboolean async;
  GError *error;
  OGMJobSpawn *parent;
};

enum
{
  RUN,
  CANCEL,
  PROGRESS,
  SUSPEND,
  RESUME,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

GQuark
ogmjob_spawn_error_quark (void)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_static_string ("ogmjob-spawn-error-quark");

  return quark;
}

G_DEFINE_ABSTRACT_TYPE (OGMJobSpawn, ogmjob_spawn, G_TYPE_OBJECT)

static void
ogmjob_spawn_class_init (OGMJobSpawnClass *klass)
{
  /**
   * OGMJobSpawn::run
   * @spawn: the spawn that received the signal
   *
   * Emitted each time a spawn is run.
   *
   * Returns: An #OGMJobResultType
   */
  signals[RUN] = g_signal_new ("run", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 
      G_STRUCT_OFFSET (OGMJobSpawnClass, run), NULL, NULL,
      ogmjob_cclosure_marshal_INT__VOID, G_TYPE_INT, 0);

  /**
   * OGMJobSpawn::cancel
   * @spawn: the spawn that received the signal
   *
   * Emitted each time a spawn is canceled.
   */
  signals[CANCEL] = g_signal_new ("cancel", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 
      G_STRUCT_OFFSET (OGMJobSpawnClass, cancel), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  /**
   * OGMJobSpawn::progress
   * @spawn: the spawn that received the signal
   * @fraction: fraction of the spawn that's been completed
   *
   * Emitted each time a spawn progresses.
   */
  signals[PROGRESS] = g_signal_new ("progress", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 
      G_STRUCT_OFFSET (OGMJobSpawnClass, progress), NULL, NULL,
      g_cclosure_marshal_VOID__DOUBLE, G_TYPE_NONE, 1, G_TYPE_DOUBLE);

  /**
   * OGMJobSpawn::suspend
   * @spawn: the spawn that received the signal
   *
   * Emitted each time a spawn is suspended.
   */
  signals[SUSPEND] = g_signal_new ("suspend", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 
      G_STRUCT_OFFSET (OGMJobSpawnClass, suspend), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  /**
   * OGMJobSpawn::resume
   * @spawn: the spawn that received the signal
   *
   * Emitted each time a suspended spawn is resumed.
   */
  signals[RESUME] = g_signal_new ("resume", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 
      G_STRUCT_OFFSET (OGMJobSpawnClass, resume), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  g_type_class_add_private (klass, sizeof (OGMJobSpawnPriv));
}

static void
ogmjob_spawn_init (OGMJobSpawn *spawn)
{
  spawn->priv = OGMJOB_SPAWN_GET_PRIVATE (spawn);

  spawn->priv->result = OGMJOB_RESULT_ERROR;
}

/**
 * ogmjob_spawn_run:
 * @spawn: An #OGMJobSpawn
 * @error: Location for error, or %NULL
 *
 * Runs a spawn.
 *
 * Returns: An #OGMJobResultType
 */
gint
ogmjob_spawn_run (OGMJobSpawn *spawn, GError **error)
{
  g_return_val_if_fail (OGMJOB_IS_SPAWN (spawn), OGMJOB_RESULT_ERROR);
  g_return_val_if_fail (error == NULL || *error == NULL, OGMJOB_RESULT_ERROR);

  spawn->priv->result = OGMJOB_RESULT_ERROR;
  spawn->priv->error = NULL;

  g_signal_emit (spawn, signals[RUN], 0, &spawn->priv->result);

  if (spawn->priv->result == OGMJOB_RESULT_ERROR && spawn->priv->error)
    g_propagate_error (error, spawn->priv->error);

  return spawn->priv->result;
}

/**
 * ogmjob_spawn_cancel:
 * @spawn: An #OGMJobSpawn
 *
 * Cancels a spawn.
 */
void
ogmjob_spawn_cancel (OGMJobSpawn *spawn)
{
  g_return_if_fail (OGMJOB_IS_SPAWN (spawn));

  if (spawn->priv->result != OGMJOB_RESULT_CANCEL)
  {
#ifdef G_ENABLE_DEBUG
    ogmjob_log_printf ("Canceling %s\n", G_OBJECT_TYPE_NAME (spawn));
#endif

    spawn->priv->result = OGMJOB_RESULT_CANCEL;

    g_signal_emit (spawn, signals[CANCEL], 0);
  }
}

/**
 * ogmjob_spawn_suspend:
 * @spawn: An #OGMJobSpawn
 *
 * Suspends a spawn.
 */
void
ogmjob_spawn_suspend (OGMJobSpawn *spawn)
{
  g_return_if_fail (OGMJOB_IS_SPAWN (spawn));

  g_signal_emit (spawn, signals[SUSPEND], 0);
}

/**
 * ogmjob_spawn_resume:
 * @spawn: An #OGMJobSpawn
 *
 * Resumes a suspended spawn.
 */
void
ogmjob_spawn_resume (OGMJobSpawn *spawn)
{
  g_return_if_fail (OGMJOB_IS_SPAWN (spawn));

  g_signal_emit (spawn, signals[RESUME], 0);
}

/**
 * ogmjob_spawn_set_async:
 * @spawn: An #OGMJobSpawn
 * @async: TRUE if asynchronous
 *
 * Sets whether to run the spawn asynchronously.
 */
void
ogmjob_spawn_set_async (OGMJobSpawn *spawn, gboolean async)
{
  g_return_if_fail (OGMJOB_IS_SPAWN (spawn));

  spawn->priv->async = async;
}

/**
 * ogmjob_spawn_get_async:
 * @spawn: An #OGMJobSpawn
 *
 * Gets whether to run the spawn asynchronously.
 *
 * Returns: TRUE if asynchronous
 */
gboolean
ogmjob_spawn_get_async (OGMJobSpawn *spawn)
{
  g_return_val_if_fail (OGMJOB_IS_SPAWN (spawn), FALSE);

  return spawn->priv->async;
}

/**
 * ogmjob_spawn_get_parent:
 * @spawn: An #OGMJobSpawn
 *
 * Returns the parent container of a spawn.
 *
 * Returns: An #OGMJobSpawn, or NULL
 */
OGMJobSpawn *
ogmjob_spawn_get_parent (OGMJobSpawn *spawn)
{
  g_return_val_if_fail (OGMJOB_IS_SPAWN (spawn), NULL);

  return spawn->priv->parent;
}

/**
 * ogmjob_spawn_set_parent:
 * @spawn: An #OGMJobSpawn
 * @parent: The parent container
 *
 * Sets the container as the parent of a widget.
 */
void
ogmjob_spawn_set_parent (OGMJobSpawn *spawn, OGMJobSpawn *parent)
{
  g_return_if_fail (OGMJOB_IS_SPAWN (spawn));
  g_return_if_fail (parent == NULL || OGMJOB_IS_SPAWN (parent));

  spawn->priv->parent = parent;
}

/**
 * ogmjob_spawn_propagate_error:
 * @spawn: An #OGMJobSpawn
 * @error: An #GError
 *
 * Propagates @error in @spawn.
 */
void
ogmjob_spawn_propagate_error (OGMJobSpawn *spawn, GError *error)
{
  g_return_if_fail (OGMJOB_IS_SPAWN (spawn));

  if (error)
    g_propagate_error (&spawn->priv->error, error);
}

