/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * SECTION:ogmrip-encoding-manager
 * @title: OGMRipEncodingManager
 * @short_description: An encoding manager
 * @include: ogmrip-encoding-manager.h
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-encoding-manager.h"
#include "ogmrip-marshal.h"

#include <glib/gi18n-lib.h>

#include <string.h>

#define OGMRIP_ENCODING_MANAGER_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_ENCODING_MANAGER, OGMRipEncodingManagerPriv))

struct _OGMRipEncodingManagerPriv
{
  GList *encodings;
};

enum
{
  ADD,
  REMOVE,
  MOVE,
  LAST_SIGNAL
};

static void ogmrip_encoding_manager_dispose         (GObject               *gobject);
static void ogmrip_encoding_manager_add_internal    (OGMRipEncodingManager *manager,
                                                     OGMRipEncoding        *encoding);
static void ogmrip_encoding_manager_remove_internal (OGMRipEncodingManager *manager,
                                                     OGMRipEncoding        *encoding);
static void ogmrip_encoding_manager_move_internal   (OGMRipEncodingManager *manager,
                                                     OGMRipEncoding        *encoding,
                                                     OGMRipDirection       direction);

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (OGMRipEncodingManager, ogmrip_encoding_manager, G_TYPE_OBJECT)

static void
ogmrip_encoding_manager_class_init (OGMRipEncodingManagerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_encoding_manager_dispose;

  klass->add = ogmrip_encoding_manager_add_internal;
  klass->remove = ogmrip_encoding_manager_remove_internal;
  klass->move = ogmrip_encoding_manager_move_internal;

  signals[ADD] = g_signal_new ("add", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipEncodingManagerClass, add), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, OGMRIP_TYPE_ENCODING);

  signals[REMOVE] = g_signal_new ("remove", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipEncodingManagerClass, remove), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, OGMRIP_TYPE_ENCODING);

  signals[MOVE] = g_signal_new ("move", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipEncodingManagerClass, move), NULL, NULL,
      ogmrip_cclosure_marshal_VOID__OBJECT_UINT, G_TYPE_NONE, 2, OGMRIP_TYPE_ENCODING, G_TYPE_UINT);

  g_type_class_add_private (klass, sizeof (OGMRipEncodingManagerPriv));
}

static void
ogmrip_encoding_manager_init (OGMRipEncodingManager *manager)
{
  manager->priv = OGMRIP_ENCODING_MANAGER_GET_PRIVATE (manager);
}

static void
ogmrip_encoding_manager_dispose (GObject *gobject)
{
  OGMRipEncodingManager *manager;

  manager = OGMRIP_ENCODING_MANAGER (gobject);

  if (manager->priv->encodings)
  {
    g_list_foreach (manager->priv->encodings, (GFunc) g_object_unref, NULL);
    g_list_free (manager->priv->encodings);
    manager->priv->encodings = NULL;
  }

  (*G_OBJECT_CLASS (ogmrip_encoding_manager_parent_class)->dispose) (gobject);
}

static void
ogmrip_encoding_manager_add_internal (OGMRipEncodingManager *manager, OGMRipEncoding *encoding)
{
  manager->priv->encodings = g_list_append (manager->priv->encodings, g_object_ref (encoding));
}

static void
ogmrip_encoding_manager_remove_internal (OGMRipEncodingManager *manager, OGMRipEncoding *encoding)
{
  GList *link;

  link = g_list_find (manager->priv->encodings, encoding);
  if (link)
  {
    manager->priv->encodings = g_list_delete_link (manager->priv->encodings, link);
    g_object_unref (encoding);
  }
}

static void
ogmrip_encoding_manager_move_internal (OGMRipEncodingManager *manager, OGMRipEncoding *encoding, OGMRipDirection direction)
{
  GList *link;
  gpointer data;

  switch (direction)
  {
    case OGMRIP_DIRECTION_UP:
      link = g_list_find (manager->priv->encodings, encoding);
      if (link && link->prev)
      {
        data = link->prev->data;
        link->prev->data = link->data;
        link->data = data;
      }
      break;
    case OGMRIP_DIRECTION_DOWN:
      link = g_list_find (manager->priv->encodings, encoding);
      if (link && link->next)
      {
        data = link->next->data;
        link->next->data = link->data;
        link->data = data;
      }
      break;
    case OGMRIP_DIRECTION_TOP:
      link = g_list_find (manager->priv->encodings, encoding);
      if (link)
      {
        manager->priv->encodings = g_list_delete_link (manager->priv->encodings, link);
        manager->priv->encodings = g_list_prepend (manager->priv->encodings, encoding);
      }
      break;
    case OGMRIP_DIRECTION_BOTTOM:
      link = g_list_find (manager->priv->encodings, encoding);
      if (link)
      {
        manager->priv->encodings = g_list_delete_link (manager->priv->encodings, link);
        manager->priv->encodings = g_list_append (manager->priv->encodings, encoding);
      }
      break;
  }
}

/**
 * ogmrip_encoding_manager_new:
 *
 * Creates a new #OGMRipEncodingManager.
 *
 * Returns: The new #OGMRipEncodingManager
 */
OGMRipEncodingManager *
ogmrip_encoding_manager_new (void)
{
  return g_object_new (OGMRIP_TYPE_ENCODING_MANAGER, NULL);
}

/**
 * ogmrip_encoding_manager_add:
 * @manager: An #OGMRipEncodingManager
 * @encoding: An #OGMRipEncoding
 *
 * Adds @encoding to @manager.
 */
void
ogmrip_encoding_manager_add (OGMRipEncodingManager *manager, OGMRipEncoding *encoding)
{
  g_return_if_fail (OGMRIP_IS_ENCODING_MANAGER (manager));
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  g_signal_emit (manager, signals[ADD], 0, encoding);
}

/**
 * ogmrip_encoding_manager_remove:
 * @manager: An #OGMRipEncodingManager
 * @encoding: An #OGMRipEncoding
 *
 * Removes @encoding from @manager.
 */
void
ogmrip_encoding_manager_remove (OGMRipEncodingManager *manager, OGMRipEncoding *encoding)
{
  g_return_if_fail (OGMRIP_IS_ENCODING_MANAGER (manager));
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  g_signal_emit (manager, signals[REMOVE], 0, encoding);
}

void
ogmrip_encoding_manager_move (OGMRipEncodingManager *manager, OGMRipEncoding *encoding, OGMRipDirection direction)
{
  g_return_if_fail (OGMRIP_IS_ENCODING_MANAGER (manager));
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  g_signal_emit (manager, signals[MOVE], 0, encoding, direction);
}

GList *
ogmrip_encoding_manager_get_list (OGMRipEncodingManager *manager)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING_MANAGER (manager), NULL);

  return g_list_copy (manager->priv->encodings);
}
