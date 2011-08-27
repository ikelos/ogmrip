/* OGMRip - A library for DVD ripping and encoding
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
 * SECTION:ogmrip-copy
 * @title: OGMRipCopy
 * @short_description: A codec to copy a DVD
 * @include: ogmrip-copy.h
 */

#include "ogmrip-copy.h"

#define OGMRIP_COPY_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_COPY, OGMRipCopyPriv))

struct _OGMRipCopyPriv
{
  OGMDvdDisc *disc;
  gchar *path;
};

enum
{
  PROP_0,
  PROP_DISC,
  PROP_PATH
};

static void ogmrip_copy_constructed  (GObject      *gobject);
static void ogmrip_copy_dispose      (GObject      *gobject);
static void ogmrip_copy_finalize     (GObject      *gobject);
static void ogmrip_copy_set_property (GObject      *gobject,
                                      guint        property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec);
static void ogmrip_copy_get_property (GObject      *gobject,
                                      guint        property_id,
                                      GValue       *value,
                                      GParamSpec   *pspec);
static gint ogmrip_copy_run          (OGMJobSpawn  *spawn);

G_DEFINE_TYPE (OGMRipCopy, ogmrip_copy, OGMJOB_TYPE_SPAWN)

static void
ogmrip_copy_class_init (OGMRipCopyClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_copy_constructed;
  gobject_class->dispose = ogmrip_copy_dispose;
  gobject_class->finalize = ogmrip_copy_finalize;
  gobject_class->get_property = ogmrip_copy_get_property;
  gobject_class->set_property = ogmrip_copy_set_property;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmrip_copy_run;

  g_object_class_install_property (gobject_class, PROP_DISC, 
        g_param_spec_pointer ("disc", "Disc property", "Set disc", 
           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PATH, 
        g_param_spec_string ("path", "Path property", "Set path", 
           NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipCopyPriv));
}

static void
ogmrip_copy_init (OGMRipCopy *copy)
{
  copy->priv = OGMRIP_COPY_GET_PRIVATE (copy);
}

static void
ogmrip_copy_constructed (GObject *gobject)
{
  OGMRipCopy *copy = OGMRIP_COPY (gobject);

  if (!copy->priv->path)
    copy->priv->path = g_strdup ("/dev/null");

  G_OBJECT_CLASS (ogmrip_copy_parent_class)->constructed (gobject);
}

static void
ogmrip_copy_dispose (GObject *gobject)
{
  OGMRipCopy *copy = OGMRIP_COPY (gobject);

  if (copy->priv->disc)
  {
    ogmdvd_disc_unref (copy->priv->disc);
    copy->priv->disc = NULL;
  }

  G_OBJECT_CLASS (ogmrip_copy_parent_class)->dispose (gobject);
}

static void
ogmrip_copy_finalize (GObject *gobject)
{
  OGMRipCopy *copy = OGMRIP_COPY (gobject);

  g_free (copy->priv->path);

  G_OBJECT_CLASS (ogmrip_copy_parent_class)->finalize (gobject);
}

static void
ogmrip_copy_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipCopy *copy = OGMRIP_COPY (gobject);

  switch (property_id) 
  {
    case PROP_DISC:
      copy->priv->disc = g_value_get_pointer (value);
      ogmdvd_disc_ref (copy->priv->disc);
      break;
    case PROP_PATH:
      g_free (copy->priv->path);
      copy->priv->path = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_copy_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipCopy *copy = OGMRIP_COPY (gobject);

  switch (property_id) 
  {
    case PROP_DISC:
      g_value_set_pointer (value, copy->priv->disc);
      break;
    case PROP_PATH:
      g_value_set_string (value, copy->priv->path);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_copy_progress (OGMDvdDisc *disc, gdouble percent, gpointer spawn)
{
  g_signal_emit_by_name (spawn, "progress", percent);
}

static gint
ogmrip_copy_run (OGMJobSpawn *spawn)
{
  OGMRipCopy *copy = OGMRIP_COPY (spawn);
  GCancellable *cancellable;
  gint retval;

  cancellable = g_cancellable_new ();

  if (ogmdvd_disc_copy (copy->priv->disc, copy->priv->path, cancellable, ogmrip_copy_progress, spawn, NULL))
    retval = OGMJOB_RESULT_SUCCESS;
  else if (g_cancellable_is_cancelled (cancellable))
    retval = OGMJOB_RESULT_CANCEL;
  else
    retval = OGMJOB_RESULT_ERROR;

  g_object_unref (cancellable);

  return retval;
}

/**
 * ogmrip_copy_new:
 * @title: An #OGMDvdTitle
 * @path: The output path
 *
 * Creates a new #OGMRipCopy.
 *
 * Returns: The new #OGMRipCopy
 */
OGMJobSpawn *
ogmrip_copy_new (OGMDvdDisc *disc, const gchar *path)
{
  g_return_val_if_fail (disc != NULL, NULL);
  g_return_val_if_fail (path && *path, NULL);

  return g_object_new (OGMRIP_TYPE_COPY, "disc", disc, "path", path, NULL);
}

