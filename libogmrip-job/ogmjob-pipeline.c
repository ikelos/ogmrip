/* OGMJob - A library to spawn processes
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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
 * @short_description: A container to run tasks together
 */

#include "ogmjob-pipeline.h"

struct _OGMJobPipelinePriv
{
  GList *children;
  gdouble progress;
};

static gboolean ogmjob_pipeline_run    (OGMJobTask      *task,
                                        GCancellable    *cancellable,
                                        GError          **error);
static void     ogmjob_pipeline_add    (OGMJobContainer *container, 
                                        OGMJobTask      *task);
static void     ogmjob_pipeline_remove (OGMJobContainer *container, 
                                        OGMJobTask      *task);

G_DEFINE_TYPE (OGMJobPipeline, ogmjob_pipeline, OGMJOB_TYPE_CONTAINER)

static void
ogmjob_pipeline_class_init (OGMJobPipelineClass *klass)
{
  OGMJobTaskClass *task_class;
  OGMJobContainerClass *container_class;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmjob_pipeline_run;

  container_class = OGMJOB_CONTAINER_CLASS (klass);
  container_class->add = ogmjob_pipeline_add;
  container_class->remove = ogmjob_pipeline_remove;

  g_type_class_add_private (klass, sizeof (OGMJobPipelinePriv));
}

static void
ogmjob_pipeline_init (OGMJobPipeline *pipeline)
{
  pipeline->priv = G_TYPE_INSTANCE_GET_PRIVATE (pipeline, OGMJOB_TYPE_PIPELINE, OGMJobPipelinePriv);
}

static gboolean
ogmjob_pipeline_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobPipeline *pipeline = OGMJOB_PIPELINE (task);
  GList *child;
  gboolean result;

  if (!pipeline->priv->children)
    return TRUE;

  pipeline->priv->progress = 0.0;

  for (child = pipeline->priv->children; child; child = child->next)
  {
    if (child->next)
      ogmjob_task_run_async (OGMJOB_TASK (child->data), cancellable, NULL, NULL);
    else
      result = ogmjob_task_run (OGMJOB_TASK (child->data), cancellable, error);
  }

  return result;
}

static void
ogmjob_pipeline_child_progress (OGMJobPipeline *pipeline, GParamSpec *pspec, OGMJobTask *task)
{
  gdouble progress;

  progress = ogmjob_task_get_progress (task);
  pipeline->priv->progress = MAX (pipeline->priv->progress, progress);

  ogmjob_task_set_progress (OGMJOB_TASK (pipeline), pipeline->priv->progress);
}

static void
ogmjob_pipeline_add (OGMJobContainer *container, OGMJobTask *task)
{
  OGMJobPipeline *pipeline = OGMJOB_PIPELINE (container);

  pipeline->priv->children = g_list_append (pipeline->priv->children, g_object_ref (task));

  g_signal_connect_swapped (task, "notify::progress", 
      G_CALLBACK (ogmjob_pipeline_child_progress), container);
}

static void
ogmjob_pipeline_remove (OGMJobContainer *container, OGMJobTask *task)
{
  OGMJobPipeline *pipeline = OGMJOB_PIPELINE (container);
  GList *link;

  link = g_list_find (pipeline->priv->children, task);
  if (link)
  {
    g_signal_handlers_disconnect_by_func (task, ogmjob_pipeline_child_progress, container);

    g_object_unref (task);
    pipeline->priv->children = g_list_remove_link (pipeline->priv->children, link);
    g_list_free (link);
  }
}

/**
 * ogmjob_pipeline_new:
 *
 * Creates a new #OGMJobPipeline.
 *
 * Returns: The new #OGMJobPipeline
 */
OGMJobTask *
ogmjob_pipeline_new (void)
{
  return g_object_new (OGMJOB_TYPE_PIPELINE, NULL);
}

