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

#include "ogmrip-encoding-manager.h"

#include <glib/gi18n.h>

#include <string.h>

#define OGMRIP_ENCODING_MANAGER_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_ENCODING_MANAGER, OGMRipEncodingManagerPriv))

struct _OGMRipEncodingManagerPriv
{
  GList *encodings;

  GList *backup_link;
  GList *extract_link;

  gint cleanup_type;
};

static void ogmrip_encoding_manager_dispose (GObject *gobject);

static gint
ogmrip_encoding_encoding_compare_title_set (OGMRipEncoding *encoding1, OGMRipEncoding *encoding2)
{
  OGMDvdTitle *title1, *title2;
  const gchar *id1, *id2;
  gint ts1, ts2;

  if (!encoding1)
    return -1;

  if (!encoding2)
    return 1;

  if (encoding1 == encoding2)
    return 0;

  title1 = ogmrip_encoding_get_title (encoding1);
  title2 = ogmrip_encoding_get_title (encoding2);

  if (title1 == title2)
    return 0;

  ts1 = ogmdvd_title_get_ts_nr (title1);
  ts2 = ogmdvd_title_get_ts_nr (title2);

  if (ts1 != ts2)
    return ts1 - ts2;

  id1 = ogmrip_encoding_get_id (encoding1);
  id2 = ogmrip_encoding_get_id (encoding2);

  return strcmp (id1, id2);
}

G_DEFINE_TYPE (OGMRipEncodingManager, ogmrip_encoding_manager, G_TYPE_OBJECT)

static void
ogmrip_encoding_manager_class_init (OGMRipEncodingManagerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_encoding_manager_dispose;

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

static gboolean
ogmrip_encoding_manager_check_cleanup (OGMRipEncodingManager *manager, OGMRipEncoding *encoding)
{
  OGMRipEncoding *next_encoding;

  if (manager->priv->cleanup_type == OGMRIP_CLEANUP_KEEP_ALL)
    return FALSE;

  if (!manager->priv->extract_link->next)
    return manager->priv->cleanup_type != OGMRIP_CLEANUP_KEEP_LAST;

  next_encoding = manager->priv->extract_link->next->data;

  if (ogmrip_encoding_encoding_compare_title_set (encoding, next_encoding) != 0)
    return TRUE;

  return FALSE;
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
 * ogmrip_encoding_manager_run:
 * @manager: An #OGMRipEncodingManager
 * @error: A location to return an error of type #OGMRIP_ENCODING_ERROR
 *
 * Performs all the encodings contained in @manager.
 *
 * Returns: An #OGMJobResultType
 */
gint
ogmrip_encoding_manager_run (OGMRipEncodingManager *manager, GError **error)
{
  OGMRipEncoding *encoding = NULL;
  GList *link;

  gint result = OGMJOB_RESULT_ERROR;

  g_return_val_if_fail (manager != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  for (link = manager->priv->encodings; link; link = link->next)
  {
    encoding = link->data;

    if (!OGMRIP_ENCODING_IS_BACKUPED (encoding) || !OGMRIP_ENCODING_IS_EXTRACTED (encoding))
      break;
  }

  manager->priv->extract_link = manager->priv->backup_link = link;

  while (manager->priv->extract_link)
  {
    /* backup as many titles as possible */
    while (manager->priv->backup_link)
    {
      encoding = manager->priv->backup_link->data;

      if (ogmrip_encoding_get_copy_dvd (encoding))
      {
        result = ogmrip_encoding_backup (encoding, error);

        /* stop backuping when there is not enough disk space */
        if (result == OGMJOB_RESULT_ERROR && g_error_matches (*error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_SIZE))
        {
          /* at least one title has been backuped */
          if (manager->priv->backup_link != manager->priv->extract_link)
          {
            g_clear_error (error);
            break;
          }
        }

        if (result != OGMJOB_RESULT_SUCCESS)
          goto cleanup;
      }

      manager->priv->backup_link = manager->priv->backup_link->next;
    }

    encoding = manager->priv->extract_link->data;

    /* extract the title */
    result = ogmrip_encoding_extract (encoding, error);
    if (result != OGMJOB_RESULT_SUCCESS)
      goto cleanup;

    /* cleanup the copy if it's not needed anymore */
    if (ogmrip_encoding_manager_check_cleanup (manager, encoding))
      ogmrip_encoding_cleanup (encoding);

    manager->priv->extract_link = manager->priv->extract_link->next;
  }

cleanup:
  if (encoding && result != OGMJOB_RESULT_SUCCESS)
    ogmrip_encoding_cleanup (encoding);
 
  return result;
}

/**
 * ogmrip_encoding_manager_cancel:
 * @manager: An #OGMRipEncodingManager
 *
 * Cancels all encodings.
 */
void
ogmrip_encoding_manager_cancel (OGMRipEncodingManager *manager)
{
  g_return_if_fail (OGMRIP_IS_ENCODING_MANAGER (manager));

  g_message ("Not yet implemented");
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
  GList *item;

  g_return_if_fail (OGMRIP_IS_ENCODING_MANAGER (manager));
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  item = g_list_alloc ();
  item->data = g_object_ref (encoding);
  item->next = NULL;
  item->prev = NULL;

  if (!manager->priv->encodings)
    manager->priv->encodings = item;
  else
  {
    GList *list, *link;

    list = manager->priv->backup_link ? manager->priv->backup_link : manager->priv->extract_link ? manager->priv->extract_link : manager->priv->encodings;

    for (link = list; link->next; link = link->next)
    {
      if (ogmrip_encoding_encoding_compare_title_set (encoding, link->data) == 0)
      {
        while (link->next && ogmrip_encoding_encoding_compare_title_set (encoding, link->data) == 0)
          link = link->next;
        break;
      }
    }

    item->next = link->next;
    item->prev = link;

    if (link->next)
      link->next->prev = item;
    link->next = item;
  }
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
  GList *link;
  gboolean extract_found = FALSE, backup_found = FALSE;

  g_return_if_fail (OGMRIP_IS_ENCODING_MANAGER (manager));
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  for (link = manager->priv->encodings; link; link = link->next)
  {
    if (link->data == encoding)
      break;

    if (link == manager->priv->extract_link)
      extract_found = TRUE;

    if (link == manager->priv->backup_link)
      backup_found = TRUE;
  }

  if (link)
  {
    if (link != manager->priv->extract_link && link != manager->priv->backup_link)
    {
      if (backup_found && !extract_found)
        ogmrip_encoding_cleanup (OGMRIP_ENCODING (link->data));

      if (link == manager->priv->encodings)
        manager->priv->encodings = link->next;

      if (link->next)
        link->next->prev = link->prev;

      if (link->prev)
        link->prev->next = link->next;

      link->next = NULL;
      link->prev = NULL;

      g_object_unref (link->data);
      g_list_free (link);
    }
  }
}

/**
 * ogmrip_encoding_manager_foreach:
 * @manager: An #OGMRipEncodingManager
 * @func: A callback
 * @data: Callback user data
 *
 * Invokes @func on each encoding of @manager.
 *
 * Returns: %TRUE if @func returned TRUE for all encodings, %FALSE otherwise
 */
gboolean
ogmrip_encoding_manager_foreach (OGMRipEncodingManager *manager, OGMRipEncodingFunc func, gpointer data)
{
  GList *link, *next;

  g_return_val_if_fail (OGMRIP_IS_ENCODING_MANAGER (manager), FALSE);
  g_return_val_if_fail (func != NULL, FALSE);

  link = manager->priv->encodings;
  while (link)
  {
    next = link->next;

    if (!(* func) (link->data, data))
      return FALSE;

    link = next;
  }

  return TRUE;
}

/**
 * ogmrip_encoding_manager_find:
 * @manager: An #OGMRipEncodingManager
 * @func: A function to call for each encoding. It should return %TRUE when the desired encoding is found
 * @data: User data passed to the function
 *
 * Finds the encoding of @manager Finds the element in a GList which contains the given data.
 *
 * Returns: The found encoding, or NULL.
 */
OGMRipEncoding *
ogmrip_encoding_manager_find (OGMRipEncodingManager *manager, OGMRipEncodingFunc func, gpointer data)
{
  GList *link, *next;

  g_return_val_if_fail (OGMRIP_IS_ENCODING_MANAGER (manager), FALSE);
  g_return_val_if_fail (func != NULL, FALSE);

  link = manager->priv->encodings;
  while (link)
  {
    next = link->next;

    if ((* func) (link->data, data))
      return link->data;

    link = next;
  }

  return NULL;
}

/**
 * ogmrip_encoding_manager_nth:
 * @manager: An #OGMRipEncodingManager
 * @n: The position of the encoding, counting from 0
 *
 * Gets the encoding at the given position.
 *
 * Returns: The encoding, or NULL.
 */
OGMRipEncoding *
ogmrip_encoding_manager_nth (OGMRipEncodingManager *manager, gint n)
{
  GList *link = NULL;

  g_return_val_if_fail (OGMRIP_IS_ENCODING_MANAGER (manager), NULL);

  if (!manager->priv->encodings)
    return NULL;

  if (n >= 0)
    link = g_list_nth (manager->priv->encodings, n);

  if (!link)
    link = g_list_last (manager->priv->encodings);

  return link->data;
}

/**
 * ogmrip_encoding_manager_set_cleanup:
 * @manager: An #OGMRipEncodingManager
 * @type: The cleanup type
 *
 * Sets the cleanup method.
 */
void
ogmrip_encoding_manager_set_cleanup (OGMRipEncodingManager *manager, OGMRipCleanupType type)
{
  g_return_if_fail (OGMRIP_IS_ENCODING_MANAGER (manager));

  manager->priv->cleanup_type = type;
}

/**
 * ogmrip_encoding_manager_get_cleanup:
 * @manager: An #OGMRipEncodingManager
 *
 * Gets the cleanup method.
 *
 * Returns: The cleanup type
 */
gint
ogmrip_encoding_manager_get_cleanup (OGMRipEncodingManager *manager)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING_MANAGER (manager), -1);

  return manager->priv->cleanup_type;
}

