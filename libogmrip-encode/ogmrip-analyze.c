/* OGMRip - A library for media ripping and encoding
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
 * SECTION:ogmrip-analyze
 * @title: OGMRipAnalyze
 * @short_description: A codec to analyze a media
 * @include: ogmrip-analyze.h
 */

#include "ogmrip-analyze.h"

struct _OGMRipAnalyzePriv
{
  OGMRipTitle *title;
  gchar *path;
};

enum
{
  PROP_0,
  PROP_TITLE
};

G_DEFINE_TYPE_WITH_PRIVATE (OGMRipAnalyze, ogmrip_analyze, OGMJOB_TYPE_TASK)

static void
ogmrip_analyze_dispose (GObject *gobject)
{
  OGMRipAnalyze *analyze = OGMRIP_ANALYZE (gobject);

  if (analyze->priv->title)
  {
    g_object_unref (analyze->priv->title);
    analyze->priv->title = NULL;
  }

  G_OBJECT_CLASS (ogmrip_analyze_parent_class)->dispose (gobject);
}

static void
ogmrip_analyze_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipAnalyze *analyze = OGMRIP_ANALYZE (gobject);

  switch (property_id) 
  {
    case PROP_TITLE:
      analyze->priv->title = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_analyze_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipAnalyze *analyze = OGMRIP_ANALYZE (gobject);

  switch (property_id) 
  {
    case PROP_TITLE:
      g_value_set_object (value, analyze->priv->title);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_analyze_progress (OGMRipTitle *title, gdouble percent, gpointer task)
{
  ogmjob_task_set_progress (task, percent);
}

static gboolean
ogmrip_analyze_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMRipAnalyze *analyze = OGMRIP_ANALYZE (task);

  return ogmrip_title_analyze (analyze->priv->title, cancellable, ogmrip_analyze_progress, task, error);
}

static void
ogmrip_analyze_class_init (OGMRipAnalyzeClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobTaskClass *task_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_analyze_dispose;
  gobject_class->get_property = ogmrip_analyze_get_property;
  gobject_class->set_property = ogmrip_analyze_set_property;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_analyze_run;

  g_object_class_install_property (gobject_class, PROP_TITLE,
      g_param_spec_object ("title", "Title property", "Set title",
        OGMRIP_TYPE_TITLE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_analyze_init (OGMRipAnalyze *analyze)
{
  analyze->priv = ogmrip_analyze_get_instance_private (analyze);
}

/**
 * ogmrip_analyze_new:
 * @title: An #OGMRipTitle
 *
 * Creates a new #OGMRipAnalyze.
 *
 * Returns: The new #OGMRipAnalyze
 */
OGMJobTask *
ogmrip_analyze_new (OGMRipTitle *title)
{
  g_return_val_if_fail (OGMRIP_IS_TITLE (title), NULL);

  return g_object_new (OGMRIP_TYPE_ANALYZE, "title", title, NULL);
}

