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
 * SECTION:ogmjob-list
 * @title: OGMJobList
 * @include: ogmjob-list.h
 * @short_description: Base class for spawns which contain multiple spawns
 */

#include "ogmjob-list.h"

#define OGMJOB_LIST_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMJOB_TYPE_LIST, OGMJobListPriv))

struct _OGMJobListPriv
{
  GList *children;
};

static void ogmjob_list_finalize (GObject         *object);
static void ogmjob_list_add      (OGMJobContainer *container,
                                  OGMJobSpawn     *spawn);
static void ogmjob_list_remove   (OGMJobContainer *container,
                                  OGMJobSpawn     *spawn);
static void ogmjob_list_forall   (OGMJobContainer *container,
                                  OGMJobCallback  callback,
                                  gpointer        data);

G_DEFINE_ABSTRACT_TYPE (OGMJobList, ogmjob_list, OGMJOB_TYPE_CONTAINER)

static void
ogmjob_list_class_init (OGMJobListClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobContainerClass *container_class;

  gobject_class = G_OBJECT_CLASS (klass);
  container_class = OGMJOB_CONTAINER_CLASS (klass);

  gobject_class->finalize = ogmjob_list_finalize;
  
  container_class->add = ogmjob_list_add;
  container_class->remove = ogmjob_list_remove;
  container_class->forall = ogmjob_list_forall;

  g_type_class_add_private (klass, sizeof (OGMJobListPriv));
}

static void
ogmjob_list_init (OGMJobList *list)
{
  list->priv = OGMJOB_LIST_GET_PRIVATE (list);
  list->priv->children = NULL;
}

static void
ogmjob_list_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (ogmjob_list_parent_class)->finalize (gobject);

  g_list_free (OGMJOB_LIST (gobject)->priv->children);
}

static void
ogmjob_list_add (OGMJobContainer *container, OGMJobSpawn *spawn)
{
  OGMJobList *list;

  g_object_ref (spawn);

  list = OGMJOB_LIST (container);
  list->priv->children = g_list_append (list->priv->children, spawn);
}

static void
ogmjob_list_remove (OGMJobContainer *container, OGMJobSpawn *spawn)
{
  OGMJobList *list;
  GList *link;

  list = OGMJOB_LIST (container);
  link = g_list_find (list->priv->children, spawn);
  if (link)
  {
    g_object_unref (spawn);
    list->priv->children = g_list_remove_link (list->priv->children, link);
    g_list_free (link);
  }
}

static void
ogmjob_list_forall (OGMJobContainer *container, OGMJobCallback callback, gpointer user_data)
{
  if (callback)
    g_list_foreach (OGMJOB_LIST (container)->priv->children, (GFunc) callback, user_data);
}

/**
 * ogmjob_list_get_children:
 * @list: An #OGMJobList
 *
 * Returns the list's children.
 *
 * Returns: A #GList, or NULL
 */
GList *
ogmjob_list_get_children (OGMJobList *list)
{
  g_return_val_if_fail (OGMJOB_IS_LIST (list), NULL);

  return g_list_copy (list->priv->children);
}

