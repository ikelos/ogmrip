/* OGMRip - A library for DVD ripping and encoding
 * Analyzeright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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
 * You should have received a analyze of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/**
 * SECTION:ogmrip-analyze
 * @title: OGMRipAnalyze
 * @short_description: A codec to analyze a DVD
 * @include: ogmrip-analyze.h
 */

#include "ogmrip-analyze.h"

#define OGMRIP_ANALYZE_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_ANALYZE, OGMRipAnalyzePriv))

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

static void ogmrip_analyze_dispose      (GObject      *gobject);
static void ogmrip_analyze_set_property (GObject      *gobject,
                                         guint        property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec);
static void ogmrip_analyze_get_property (GObject      *gobject,
                                         guint        property_id,
                                         GValue       *value,
                                         GParamSpec   *pspec);
static gint ogmrip_analyze_run          (OGMJobSpawn  *spawn);

G_DEFINE_TYPE (OGMRipAnalyze, ogmrip_analyze, OGMJOB_TYPE_SPAWN)

static void
ogmrip_analyze_class_init (OGMRipAnalyzeClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_analyze_dispose;
  gobject_class->get_property = ogmrip_analyze_get_property;
  gobject_class->set_property = ogmrip_analyze_set_property;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmrip_analyze_run;

  g_object_class_install_property (gobject_class, PROP_TITLE, 
        g_param_spec_object ("title", "Title property", "Set title", 
           OGMRIP_TYPE_TITLE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipAnalyzePriv));
}

static void
ogmrip_analyze_init (OGMRipAnalyze *analyze)
{
  analyze->priv = OGMRIP_ANALYZE_GET_PRIVATE (analyze);
}

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
ogmrip_analyze_progress (OGMRipTitle *title, gdouble percent, gpointer spawn)
{
  g_signal_emit_by_name (spawn, "progress", percent);
}

static gint
ogmrip_analyze_run (OGMJobSpawn *spawn)
{
  OGMRipAnalyze *analyze = OGMRIP_ANALYZE (spawn);
  GCancellable *cancellable;
  gint retval;

  cancellable = g_cancellable_new ();

  if (ogmrip_title_analyze (analyze->priv->title, cancellable, ogmrip_analyze_progress, spawn, NULL))
    retval = OGMJOB_RESULT_SUCCESS;
  else if (g_cancellable_is_cancelled (cancellable))
    retval = OGMJOB_RESULT_CANCEL;
  else
    retval = OGMJOB_RESULT_ERROR;

  g_object_unref (cancellable);

  return retval;
}

/**
 * ogmrip_analyze_new:
 * @title: An #OGMRipTitle
 * @path: The output path
 *
 * Creates a new #OGMRipAnalyze.
 *
 * Returns: The new #OGMRipAnalyze
 */
OGMJobSpawn *
ogmrip_analyze_new (OGMRipTitle *title)
{
  g_return_val_if_fail (OGMRIP_IS_TITLE (title), NULL);

  return g_object_new (OGMRIP_TYPE_ANALYZE, "title", title, NULL);
}

