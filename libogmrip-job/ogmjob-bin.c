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
 * SECTION:ogmjob-bin
 * @title: OGMJobBin
 * @include: ogmjob-bin.h
 * @short_description: Base class for tasks which contain one task only
 */

#include "ogmjob-bin.h"

#define OGMJOB_BIN_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMJOB_TYPE_BIN, OGMJobBinPriv))

struct _OGMJobBinPriv
{
  OGMJobTask *child;
};

G_DEFINE_ABSTRACT_TYPE (OGMJobBin, ogmjob_bin, OGMJOB_TYPE_CONTAINER)

static void
ogmjob_bin_run_async (OGMJobTask *task, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  OGMJobBin *bin = OGMJOB_BIN (task);

  if (bin->priv->child)
    ogmjob_task_run_async (bin->priv->child, cancellable, callback, user_data);
}

static gboolean
ogmjob_bin_run_finish (OGMJobTask *task, GAsyncResult *res, GError **error)
{
  OGMJobBin *bin = OGMJOB_BIN (task);

  if (!bin->priv->child)
    return FALSE;

  return ogmjob_task_run_finish (bin->priv->child, res, error);
}

static gboolean
ogmjob_bin_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobBin *bin = OGMJOB_BIN (task);

  if (!bin->priv->child)
    return TRUE;

  return ogmjob_task_run (bin->priv->child, cancellable, error);
}

static void
ogmjob_bin_child_progress (OGMJobContainer *container, GParamSpec *pspec, OGMJobTask *child)
{
  ogmjob_task_set_progress (OGMJOB_TASK (container), ogmjob_task_get_progress (child));
}

static void
ogmjob_bin_add (OGMJobContainer *container, OGMJobTask *task)
{
  OGMJobBin *bin;

  bin = OGMJOB_BIN (container);
  if (!bin->priv->child)
  {
    bin->priv->child = g_object_ref (task);

    g_signal_connect_swapped (task, "notify::progress", 
        G_CALLBACK (ogmjob_bin_child_progress), container);
  }
}

static void
ogmjob_bin_remove (OGMJobContainer *container, OGMJobTask *task)
{
  OGMJobBin *bin;

  bin = OGMJOB_BIN (container);
  if (bin->priv->child == task)
  {
    g_signal_handlers_disconnect_by_func (task, ogmjob_bin_child_progress, container);

    g_object_unref (task);
    bin->priv->child = NULL;
  }
}

static void
ogmjob_bin_forall (OGMJobContainer *container, OGMJobCallback callback, gpointer data)
{
  OGMJobBin *bin;

  bin = OGMJOB_BIN (container);
  if (callback && bin->priv->child)
    (* callback) (bin->priv->child, data);
}

static void
ogmjob_bin_class_init (OGMJobBinClass *klass)
{
  OGMJobTaskClass *task_class;
  OGMJobContainerClass *container_class;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run_async = ogmjob_bin_run_async;
  task_class->run_finish = ogmjob_bin_run_finish;
  task_class->run = ogmjob_bin_run;

  container_class = OGMJOB_CONTAINER_CLASS (klass);
  container_class->add = ogmjob_bin_add;
  container_class->remove = ogmjob_bin_remove;
  container_class->forall = ogmjob_bin_forall;

  g_type_class_add_private (klass, sizeof (OGMJobBinPriv));
}

static void
ogmjob_bin_init (OGMJobBin *bin)
{
  bin->priv = OGMJOB_BIN_GET_PRIVATE (bin);
  bin->priv->child = NULL;
}

/**
 * ogmjob_bin_get_child:
 * @bin: An #OGMJobBin
 *
 * Gets the child of the #OGMJobBin, or NULL if the bin contains no child task.
 *
 * Returns: An #OGMJobTask
 */
OGMJobTask *
ogmjob_bin_get_child (OGMJobBin *bin)
{
  g_return_val_if_fail (OGMJOB_IS_BIN (bin), NULL);

  return bin->priv->child;
}

