/* OGMRip - A library for DVD ripping and encoding
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
  OGMRipMedia *media;
  gchar *path;
};

enum
{
  PROP_0,
  PROP_MEDIA,
  PROP_PATH
};

static void     ogmrip_copy_constructed  (GObject      *gobject);
static void     ogmrip_copy_dispose      (GObject      *gobject);
static void     ogmrip_copy_finalize     (GObject      *gobject);
static void     ogmrip_copy_set_property (GObject      *gobject,
                                          guint        property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void     ogmrip_copy_get_property (GObject      *gobject,
                                          guint        property_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);
static gboolean ogmrip_copy_run          (OGMJobTask   *task,
                                          GCancellable *cancellable,
                                          GError       **error);

G_DEFINE_TYPE (OGMRipCopy, ogmrip_copy, OGMJOB_TYPE_TASK)

static void
ogmrip_copy_class_init (OGMRipCopyClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobTaskClass *task_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_copy_constructed;
  gobject_class->dispose = ogmrip_copy_dispose;
  gobject_class->finalize = ogmrip_copy_finalize;
  gobject_class->get_property = ogmrip_copy_get_property;
  gobject_class->set_property = ogmrip_copy_set_property;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_copy_run;

  g_object_class_install_property (gobject_class, PROP_MEDIA, 
        g_param_spec_object ("media", "Media property", "Set media", 
           OGMRIP_TYPE_MEDIA, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

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

  if (!copy->priv->media)
    g_error ("No media specified");

  if (!copy->priv->path)
    copy->priv->path = g_strdup ("/dev/null");

  G_OBJECT_CLASS (ogmrip_copy_parent_class)->constructed (gobject);
}

static void
ogmrip_copy_dispose (GObject *gobject)
{
  OGMRipCopy *copy = OGMRIP_COPY (gobject);

  if (copy->priv->media)
  {
    g_object_unref (copy->priv->media);
    copy->priv->media = NULL;
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
    case PROP_MEDIA:
      copy->priv->media = g_value_dup_object (value);
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
    case PROP_MEDIA:
      g_value_set_object (value, copy->priv->media);
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
ogmrip_copy_progress (OGMRipMedia *media, gdouble percent, gpointer task)
{
  ogmjob_task_set_progress (task, percent);
}

static gboolean
ogmrip_copy_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMRipCopy *copy = OGMRIP_COPY (task);

  return ogmrip_media_copy (copy->priv->media, copy->priv->path, cancellable, ogmrip_copy_progress, task, error);
}

/**
 * ogmrip_copy_new:
 * @title: An #OGMRipMedia
 * @path: The output path
 *
 * Creates a new #OGMRipCopy.
 *
 * Returns: The new #OGMRipCopy
 */
OGMJobTask *
ogmrip_copy_new (OGMRipMedia *media, const gchar *path)
{
  g_return_val_if_fail (OGMRIP_IS_MEDIA (media), NULL);
  g_return_val_if_fail (path && *path, NULL);

  return g_object_new (OGMRIP_TYPE_COPY, "media", media, "path", path, NULL);
}

