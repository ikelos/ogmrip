/* OGMRip - A library for media ripping and encoding
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
 * SECTION:ogmrip-copy
 * @title: OGMRipCopy
 * @short_description: A codec to copy a media
 * @include: ogmrip-copy.h
 */

#include "ogmrip-copy.h"

struct _OGMRipCopyPriv
{
  OGMRipMedia *src;
  OGMRipTitle *title;
  OGMRipMedia *dst;
  gchar *path;
};

enum
{
  PROP_0,
  PROP_MEDIA,
  PROP_TITLE,
  PROP_PATH
};

G_DEFINE_TYPE_WITH_PRIVATE (OGMRipCopy, ogmrip_copy, OGMJOB_TYPE_TASK)

static void
ogmrip_copy_constructed (GObject *gobject)
{
  OGMRipCopy *copy = OGMRIP_COPY (gobject);

  if (!copy->priv->src)
    g_error ("No media specified");

  if (!copy->priv->path)
    copy->priv->path = g_strdup ("/dev/null");

  G_OBJECT_CLASS (ogmrip_copy_parent_class)->constructed (gobject);
}

static void
ogmrip_copy_dispose (GObject *gobject)
{
  OGMRipCopy *copy = OGMRIP_COPY (gobject);

  g_clear_object (&copy->priv->title);
  g_clear_object (&copy->priv->src);
  g_clear_object (&copy->priv->dst);

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
      break;
    case PROP_TITLE:
      copy->priv->title = g_value_dup_object (value);
      copy->priv->src = g_object_ref (ogmrip_title_get_media (copy->priv->title));
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
      g_value_set_object (value, copy->priv->src);
      break;
    case PROP_TITLE:
      g_value_set_object (value, copy->priv->title);
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
ogmrip_copy_progress (GObject *source, gdouble percent, gpointer task)
{
  ogmjob_task_set_progress (task, percent);
}

static gboolean
ogmrip_copy_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMRipCopy *copy = OGMRIP_COPY (task);

  if (copy->priv->dst)
    return TRUE;

  if (copy->priv->title)
    copy->priv->dst = ogmrip_title_copy (copy->priv->title, copy->priv->path,
        cancellable, (OGMRipTitleCallback) ogmrip_copy_progress, task, error);
  else
    copy->priv->dst = ogmrip_media_copy (copy->priv->src, copy->priv->path,
        cancellable, (OGMRipMediaCallback) ogmrip_copy_progress, task, error);

  return copy->priv->dst != NULL;
}

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
           OGMRIP_TYPE_MEDIA, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TITLE, 
        g_param_spec_object ("title", "Title property", "Set title", 
           OGMRIP_TYPE_TITLE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PATH, 
        g_param_spec_string ("path", "Path property", "Set path", 
           NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_copy_init (OGMRipCopy *copy)
{
  copy->priv = ogmrip_copy_get_instance_private (copy);
}

OGMJobTask *
ogmrip_copy_new_from_media (OGMRipMedia *media, const gchar *path)
{
  g_return_val_if_fail (OGMRIP_IS_MEDIA (media), NULL);
  g_return_val_if_fail (path && *path, NULL);

  return g_object_new (OGMRIP_TYPE_COPY, "media", media, "path", path, NULL);
}

OGMJobTask *
ogmrip_copy_new_from_title (OGMRipTitle *title, const gchar *path)
{
  g_return_val_if_fail (OGMRIP_IS_TITLE (title), NULL);
  g_return_val_if_fail (path && *path, NULL);

  return g_object_new (OGMRIP_TYPE_COPY, "title", title, "path", path, NULL);
}

OGMRipMedia *
ogmrip_copy_get_source (OGMRipCopy *copy)
{
  g_return_val_if_fail (OGMRIP_IS_COPY (copy), NULL);

  return copy->priv->src;
}

OGMRipMedia *
ogmrip_copy_get_destination (OGMRipCopy *copy)
{
  g_return_val_if_fail (OGMRIP_IS_COPY (copy), NULL);

  return copy->priv->dst;
}

