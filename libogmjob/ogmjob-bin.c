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
 * SECTION:ogmjob-bin
 * @title: OGMJobBin
 * @include: ogmjob-bin.h
 * @short_description: Base class for spawns which contain one spawn only
 */

#include "ogmjob-bin.h"

#define OGMJOB_BIN_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMJOB_TYPE_BIN, OGMJobBinPriv))

struct _OGMJobBinPriv
{
  OGMJobSpawn *child;
};

static gint ogmjob_bin_run      (OGMJobSpawn     *spawn);
static void ogmjob_bin_add      (OGMJobContainer *container,
                                OGMJobSpawn     *child);
static void ogmjob_bin_remove   (OGMJobContainer *container,
                                OGMJobSpawn     *child);
static void ogmjob_bin_forall   (OGMJobContainer *container,
                                OGMJobCallback  callback,
                                gpointer       data);

G_DEFINE_ABSTRACT_TYPE (OGMJobBin, ogmjob_bin, OGMJOB_TYPE_CONTAINER)

static void
ogmjob_bin_class_init (OGMJobBinClass *klass)
{
  OGMJobSpawnClass *spawn_class;
  OGMJobContainerClass *container_class;


  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmjob_bin_run;

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

static gint
ogmjob_bin_run (OGMJobSpawn *spawn)
{
  OGMJobBin *bin;

  bin = OGMJOB_BIN (spawn);
  if (bin->priv->child)
  {
    GError *tmp_error = NULL;
    gint result;

    result = ogmjob_spawn_run (bin->priv->child, &tmp_error);
    if (result == OGMJOB_RESULT_ERROR && tmp_error)
      ogmjob_spawn_propagate_error (spawn, tmp_error);

    return result;
  }

  return OGMJOB_RESULT_SUCCESS;
}

static void
ogmjob_bin_child_progress (OGMJobContainer *container, gdouble fraction, OGMJobSpawn *child)
{
  g_signal_emit_by_name (container, "progress", fraction);
}

static void
ogmjob_bin_add (OGMJobContainer *container, OGMJobSpawn *child)
{
  OGMJobBin *bin;

  bin = OGMJOB_BIN (container);
  if (bin->priv->child == NULL)
  {
    guint handler;

    g_object_ref (child);
    bin->priv->child = child;

    handler = g_signal_connect_swapped (child, "progress", 
        G_CALLBACK (ogmjob_bin_child_progress), container);
    g_object_set_data (G_OBJECT (child), "__child_progress_handler__", 
        GUINT_TO_POINTER (handler));
  }
}

static void
ogmjob_bin_remove (OGMJobContainer *container, OGMJobSpawn *child)
{
  OGMJobBin *bin;

  bin = OGMJOB_BIN (container);
  if (bin->priv->child == child)
  {
    gpointer handler;

    handler = g_object_get_data (G_OBJECT (child), "__child_progress_handler__");
    if (handler)
      g_signal_handler_disconnect (child, GPOINTER_TO_UINT (handler));

    g_object_unref (child);
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

/**
 * ogmjob_bin_get_child:
 * @bin: An #OGMJobBin
 *
 * Gets the child of the #OGMJobBin, or NULL if the bin contains no child spawn.
 *
 * Returns: An #OGMJobSpawn
 */
OGMJobSpawn *
ogmjob_bin_get_child (OGMJobBin *bin)
{
  g_return_val_if_fail (OGMJOB_IS_BIN (bin), NULL);

  return bin->priv->child;
}

