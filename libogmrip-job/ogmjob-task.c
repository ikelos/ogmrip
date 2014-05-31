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

/**
 * SECTION:ogmjob-task
 * @title: OGMJobTask
 * @include: ogmjob-task.h
 * @short_description: Base class for all tasks
 */

#include "ogmjob-task.h"

#define OGMJOB_TASK_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMJOB_TYPE_TASK, OGMJobTaskClass))

struct _OGMJobTaskPriv
{
  OGMJobState state;
  gdouble progress;
  gboolean async;
};

enum
{
  PROP_0,
  PROP_STATE,
  PROP_PROGRESS
};

static void
ogmjob_task_set_state (OGMJobTask *task, OGMJobState state)
{
  task->priv->state = state;

  g_object_notify (G_OBJECT (task), "state");
}

G_DEFINE_ABSTRACT_TYPE (OGMJobTask, ogmjob_task, G_TYPE_OBJECT)

static void
ogmjob_task_finalize (GObject *gobject)
{
#ifdef G_ENABLE_DEBUG
  g_debug ("Finalizing %s", G_OBJECT_TYPE_NAME (gobject));
#endif

  G_OBJECT_CLASS (ogmjob_task_parent_class)->finalize (gobject);
}

static void
ogmjob_task_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
  {
    case PROP_STATE:
      g_value_set_uint (value, OGMJOB_TASK (gobject)->priv->state);
      break;
    case PROP_PROGRESS:
      g_value_set_double (value, OGMJOB_TASK (gobject)->priv->progress);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmjob_task_set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
  {
    case PROP_PROGRESS:
      OGMJOB_TASK (gobject)->priv->progress = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmjob_task_run_thread (GTask *gtask, OGMJobTask *task, gpointer data, GCancellable *cancellable)
{
  GError *error = NULL;
  gboolean retval;

  retval = ogmjob_task_run (task, cancellable, &error);

  if (!retval)
    g_task_return_error (gtask, error);
  else
    g_task_return_boolean (gtask, TRUE);
}

static void
ogmjob_task_real_run_async (OGMJobTask *task, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  GTask *gtask;

  gtask = g_task_new (task, cancellable, callback, user_data);
  g_task_run_in_thread (gtask, (GTaskThreadFunc) ogmjob_task_run_thread);
  g_object_unref (gtask);
}

static gboolean
ogmjob_task_real_run_finish (OGMJobTask *task, GAsyncResult *res, GError **error)
{
  g_return_val_if_fail (g_task_is_valid (res, task), FALSE);

  return g_task_propagate_boolean (G_TASK (res), error);
}

static void
ogmjob_task_class_init (OGMJobTaskClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmjob_task_get_property;
  gobject_class->set_property = ogmjob_task_set_property;
  gobject_class->finalize = ogmjob_task_finalize;

  klass->run_async = ogmjob_task_real_run_async;
  klass->run_finish = ogmjob_task_real_run_finish;

  g_object_class_install_property (gobject_class, PROP_STATE,
      g_param_spec_uint ("state", "State", "The state of the task",
        OGMJOB_STATE_IDLE, OGMJOB_STATE_SUSPENDED, OGMJOB_STATE_RUNNING,
        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PROGRESS,
      g_param_spec_double ("progress", "Progress", "The fraction of total work that has been completed",
        0.0, 1.0, 0.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMJobTaskPriv));
}

static void
ogmjob_task_init (OGMJobTask *task)
{
  task->priv = G_TYPE_INSTANCE_GET_PRIVATE (task, OGMJOB_TYPE_TASK, OGMJobTaskPriv);
}

void
ogmjob_task_run_async (OGMJobTask *task, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  g_return_if_fail (OGMJOB_IS_TASK (task));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  if (task->priv->state == OGMJOB_STATE_IDLE)
  {
    OGMJOB_TASK_GET_CLASS (task)->run_async (task, cancellable, callback, user_data);
    ogmjob_task_set_state (task, OGMJOB_STATE_RUNNING);
  }
}

gboolean
ogmjob_task_run_finish (OGMJobTask *task, GAsyncResult *res, GError **error)
{
  gboolean retval;

  g_return_val_if_fail (OGMJOB_IS_TASK (task), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (task->priv->state != OGMJOB_STATE_RUNNING)
    return TRUE;

  retval = OGMJOB_TASK_GET_CLASS (task)->run_finish (task, res, error);
  ogmjob_task_set_state (task, OGMJOB_STATE_IDLE);

  return retval;
}

gboolean
ogmjob_task_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobTaskClass *klass;
  gboolean result;

  g_return_val_if_fail (OGMJOB_IS_TASK (task), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (task->priv->state != OGMJOB_STATE_IDLE)
    return FALSE;

  klass = OGMJOB_TASK_GET_CLASS (task);
  if (!klass->run)
    return TRUE;

  ogmjob_task_set_state (task, OGMJOB_STATE_RUNNING);
  result = klass->run (task, cancellable, error);
  ogmjob_task_set_state (task, OGMJOB_STATE_IDLE);

  return result;
}

void
ogmjob_task_suspend (OGMJobTask *task)
{
  g_return_if_fail (OGMJOB_IS_TASK (task));

  if (task->priv->state == OGMJOB_STATE_RUNNING)
  {
    OGMJobTaskClass *klass;

    klass = OGMJOB_TASK_GET_CLASS (task);
    if (klass->suspend)
    {
      klass->suspend (task);
      ogmjob_task_set_state (task, OGMJOB_STATE_SUSPENDED);
    }
  }
}

void
ogmjob_task_resume (OGMJobTask *task)
{
  g_return_if_fail (OGMJOB_IS_TASK (task));

  if (task->priv->state == OGMJOB_STATE_SUSPENDED)
  {
    OGMJobTaskClass *klass;

    klass = OGMJOB_TASK_GET_CLASS (task);
    if (klass->resume)
    {
      klass->resume (task);
      ogmjob_task_set_state (task, OGMJOB_STATE_RUNNING);
    }
  }
}

gint
ogmjob_task_get_state (OGMJobTask *task)
{
  g_return_val_if_fail (OGMJOB_IS_TASK (task), -1);

  return task->priv->state;
}

gdouble
ogmjob_task_get_progress (OGMJobTask *task)
{
  g_return_val_if_fail (OGMJOB_IS_TASK (task), -1.0);

  return task->priv->progress;
}

void
ogmjob_task_set_progress (OGMJobTask *task, gdouble progress)
{
  g_return_if_fail (OGMJOB_IS_TASK (task));

  task->priv->progress = progress;

  g_object_notify (G_OBJECT (task), "progress");
}

