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
 * SECTION:ogmjob-pipeline
 * @title: OGMJobPipeline
 * @include: ogmjob-pipeline.h
 * @short_description: A container to run spawns together
 */

#include "ogmjob-pipeline.h"

#define OGMJOB_PIPELINE_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMJOB_TYPE_PIPELINE, OGMJobPipelinePriv))

struct _OGMJobPipelinePriv
{
  gdouble fraction;
};

static gint ogmjob_pipeline_run    (OGMJobSpawn     *spawn);
static void ogmjob_pipeline_add    (OGMJobContainer *container, 
                                    OGMJobSpawn     *child);
static void ogmjob_pipeline_remove (OGMJobContainer *container, 
                                    OGMJobSpawn     *child);

G_DEFINE_TYPE (OGMJobPipeline, ogmjob_pipeline, OGMJOB_TYPE_LIST)

static void
ogmjob_pipeline_class_init (OGMJobPipelineClass *klass)
{
  OGMJobSpawnClass *spawn_class;
  OGMJobContainerClass *container_class;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmjob_pipeline_run;

  container_class = OGMJOB_CONTAINER_CLASS (klass);
  container_class->add = ogmjob_pipeline_add;
  container_class->remove = ogmjob_pipeline_remove;

  g_type_class_add_private (klass, sizeof (OGMJobPipelinePriv));
}

static void
ogmjob_pipeline_init (OGMJobPipeline *pipeline)
{
  pipeline->priv = OGMJOB_PIPELINE_GET_PRIVATE (pipeline);
}

static gint
ogmjob_pipeline_run (OGMJobSpawn *spawn)
{
  OGMJobPipeline *pipeline;
  GList *children, *child;

  GError *tmp_error = NULL;
  gint result = OGMJOB_RESULT_SUCCESS;

  pipeline = OGMJOB_PIPELINE (spawn);

  children = ogmjob_list_get_children (OGMJOB_LIST (spawn));
  pipeline->priv->fraction = 0.0;

  for (child = children; child; child = child->next)
  {
    if (result != OGMJOB_RESULT_SUCCESS)
      break;

    ogmjob_spawn_set_async (OGMJOB_SPAWN (child->data), child->next != NULL);
    result = ogmjob_spawn_run (OGMJOB_SPAWN (child->data), &tmp_error);

    if (result == OGMJOB_RESULT_ERROR && tmp_error)
      ogmjob_spawn_propagate_error (spawn, tmp_error);
  }
  g_list_free (children);

  return result;
}

static void
ogmjob_pipeline_child_progress (OGMJobPipeline *pipeline, gdouble fraction, OGMJobSpawn *child)
{
  if (fraction < 0.0)
    g_signal_emit_by_name (pipeline, "progress", fraction);
  else if (fraction > pipeline->priv->fraction)
  {
    pipeline->priv->fraction = fraction;
    g_signal_emit_by_name (pipeline, "progress", fraction);
  }
}

static void
ogmjob_pipeline_add (OGMJobContainer *container, OGMJobSpawn *child)
{
  guint handler;

  handler = g_signal_connect_swapped (child, "progress", 
      G_CALLBACK (ogmjob_pipeline_child_progress), container);
  g_object_set_data (G_OBJECT (child), "__child_progress_handler__", 
      GUINT_TO_POINTER (handler));

  OGMJOB_CONTAINER_CLASS (ogmjob_pipeline_parent_class)->add (container, child);
}

static void
ogmjob_pipeline_remove (OGMJobContainer *container, OGMJobSpawn *child)
{
  gpointer handler;

  handler = g_object_get_data (G_OBJECT (child), "__child_progress_handler__");
  if (handler)
    g_signal_handler_disconnect (child, GPOINTER_TO_UINT (handler));

  OGMJOB_CONTAINER_CLASS (ogmjob_pipeline_parent_class)->remove (container, child);
}

/**
 * ogmjob_pipeline_new:
 *
 * Creates a new #OGMJobPipeline.
 *
 * Returns: The new #OGMJobPipeline
 */
OGMJobSpawn *
ogmjob_pipeline_new (void)
{
  return g_object_new (OGMJOB_TYPE_PIPELINE, NULL);
}

