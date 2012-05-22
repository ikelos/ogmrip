/* OGMRipBase - A foundation library for OGMRip
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/**
 * SECTION:ogmrip-fs
 * @title: Filesystem
 * @short_description: Functions for handling files, directories, links and fifos
 * @include: ogmrip-fs.h
 */

#include "ogmrip-fs.h"
#include "ogmrip-log.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif

static gchar *ogmrip_tmp_dir = NULL;

/**
 * ogmrip_fs_get_tmp_dir:
 *
 * Returns OGMRip's temporary directory.
 * 
 * Returns: The temporaty directory
 */
const gchar *
ogmrip_fs_get_tmp_dir (void)
{
  if (!ogmrip_tmp_dir)
    ogmrip_fs_set_tmp_dir (NULL);

  return ogmrip_tmp_dir;
}

/**
 * ogmrip_fs_set_tmp_dir:
 * @dir: The new temporary directory
 *
 * Sets OGMRip's temporary directory. If @dir is NULL, OGMRip's temporary
 * directory will be the system's temporary directory.
 */
void
ogmrip_fs_set_tmp_dir (const gchar *dir)
{
  if (ogmrip_tmp_dir)
    g_free (ogmrip_tmp_dir);

  if (!dir)
    dir = g_get_tmp_dir ();

  g_return_if_fail (g_file_test (dir, G_FILE_TEST_IS_DIR));

  ogmrip_tmp_dir = g_strdup (dir);
}

/**
 * ogmrip_fs_rmdir:
 * @path: A path to a directory
 * @recursive: %TRUE to remove the directory and its content recursively
 * @error: A location to return an error of type #G_FILE_ERROR
 *
 * If @recusive is %FALSE, removes the directory of @path if it is empty. If
 * @recusive is %TRUE, also removes its content recursively.
 * 
 * Returns: %TRUE on success, %FALSE if an error was set
 */
gboolean
ogmrip_fs_rmdir (const gchar *path, gboolean recursive, GError **error)
{
  g_return_val_if_fail (path && *path, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (recursive)
  {
    GDir *dir;
    GError *tmp_error = NULL;

    gchar *filename;
    const gchar *name;

    dir = g_dir_open (path, 0, &tmp_error);
    if (!dir)
    {
      g_propagate_error (error, tmp_error);
      return FALSE;
    }

    while ((name = g_dir_read_name (dir)))
    {
      filename = g_build_filename (path, name, NULL);
      if (g_file_test (filename, G_FILE_TEST_IS_DIR))
      {
        if (!ogmrip_fs_rmdir (filename, TRUE, &tmp_error))
        {
          if (tmp_error)
            g_propagate_error (error, tmp_error);
          g_free (filename);
          return FALSE;
        }
      }
      else
      {
        if (g_unlink (filename) != 0)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
              _("Failed to unlink file '%s': %s"), filename, g_strerror (errno));
          g_free (filename);
          return FALSE;
        }
      }
      g_free (filename);
    }

    g_dir_close (dir);
  }

  if (g_rmdir (path) != 0)
  {
    g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
        _("Failed to remove directory '%s': %s"), path, g_strerror (errno));
    return FALSE;
  }

  return TRUE;
}

/**
 * ogmrip_fs_mktemp:
 * @tmpl: Template for file name, as in g_mkstemp(), basename only
 * @error: A location to return an error of type #G_FILE_ERROR
 *
 * Creates a file in OGMRip's temporary directory (as returned by
 * ogmrip_fs_get_tmp_dir()).
 *
 * Returns: The actual name used, or NULL
 */
gchar *
ogmrip_fs_mktemp (const gchar *tmpl, GError **error)
{
  gchar *filename;
  int fd;

  g_return_val_if_fail (tmpl && *tmpl, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  filename = g_build_filename (ogmrip_fs_get_tmp_dir (), tmpl, NULL);
  fd = g_mkstemp (filename);
  if (fd < 0)
  {
    g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno), 
        _("Failed to create file '%s': %s"), filename, g_strerror (errno));
    g_free (filename);
    return NULL;
  }

  close (fd);

  return filename;
}

/**
 * ogmrip_fs_mkftemp:
 * @tmpl: Template for fifo name, basename only
 * @error: A location to return an error of type #G_FILE_ERROR
 *
 * Creates a fifo in OGMRip's temporary directory (as returned by
 * ogmrip_fs_get_tmp_dir()).
 *
 * Returns: The actual name used, or NULL
 */
gchar *
ogmrip_fs_mkftemp (const gchar *tmpl, GError **error)
{
  GError *tmp_error = NULL;
  gchar *name;
  gint fd;

  g_return_val_if_fail (tmpl && *tmpl, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  fd = g_file_open_tmp (tmpl, &name, &tmp_error);

  if (fd < 0)
  {
    g_propagate_error (error, tmp_error);
    return NULL;
  }

  close (fd);
  g_unlink (name);

  if (mkfifo (name, 0666) < 0)
  {
    g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno), 
        _("Failed to create fifo '%s': %s"), name, g_strerror (errno));
    g_free (name);
    return NULL;
  }

  return name;
}

/**
 * ogmrip_fs_open_tmp:
 * @tmpl: Template for file name, as in g_mkstemp(), basename only
 * @name_used: Location to store actual name used
 * @error: A location to return an error of type #G_FILE_ERROR
 *
 * Opens a file for writing in OGMRip's temporary directory (as returned by
 * g_get_tmp_dir()).
 *
 * Returns: A file handle (as from open()) to the file opened for reading and
 * writing. The file is opened in binary mode on platforms where there is a
 * difference. The file handle should be closed with close(). In case of errors,
 * -1 is returned and @error will be set.
 */
gint
ogmrip_fs_open_tmp (const gchar *tmpl, gchar **name_used, GError **error)
{
  const gchar *tmpdir;
  gchar *fulltmpl;
  gint retval;

  g_return_val_if_fail (error == NULL || *error == NULL, -1);

  if (!tmpl)
    tmpl = ".XXXXXX";

  if (!g_str_has_suffix (tmpl, "XXXXXX"))
  {
    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, 
        _("Template '%s' doesn't end with XXXXXX"), tmpl);
    return -1;
  }

  if ((strchr (tmpl, G_DIR_SEPARATOR)) != NULL
#ifdef G_OS_WIN32
      || (strchr (tmpl, '/') != NULL)
#endif
      )
  {
    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, 
        _("Template '%s' invalid, should not contain a '/'"), tmpl);
    return -1;
  }

  tmpdir = ogmrip_fs_get_tmp_dir ();
  fulltmpl = g_build_filename (tmpdir, tmpl, NULL);

  retval = g_mkstemp (fulltmpl);
  if (retval < 0)
  {
    g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno), 
        _("Failed to create file '%s': %s"), tmpl, g_strerror (errno));
    g_free (fulltmpl);
    return -1;
  }

  if (name_used)
    *name_used = fulltmpl;
  else
    g_free (fulltmpl);

  return retval;
}

/**
 * ogmrip_fs_get_extension:
 * @filename: The path to an existing filename
 *
 * Returns the extension of @filename.
 * 
 * Returns: The extension, or NULL
 */
const gchar *
ogmrip_fs_get_extension (const gchar *filename)
{
  gchar *dot;

  g_return_val_if_fail (filename != NULL, NULL);

  dot = strrchr (filename, '.');
  if (dot && ++dot)
    return dot;

  return NULL;
}

/**
 * ogmrip_fs_set_extension:
 * @filename: The path to an existing filename
 * @extension: The new extension
 *
 * If @filename already has an extension, replaces it with @extension. If not,
 * appends @extension to @filename.
 * 
 * Returns: The new name of the file, or NULL
 */
gchar *
ogmrip_fs_set_extension (const gchar *filename, const gchar *extension)
{
  gchar *dot;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (extension != NULL, NULL);

  dot = strrchr (filename, '.');
  if (!dot)
  {
    if (*extension == '.')
      return g_strconcat (filename, extension, NULL);

    return g_strconcat (filename, ".", extension, NULL);
  }

  if (dot[1] == '\0')
  {
    if (*extension == '.')
      return g_strconcat (filename, extension + 1, NULL);

    return g_strconcat (filename, ".", extension, NULL);
  }

  if (strcmp (dot + 1, extension) == 0)
    return g_strdup (filename);
  else
  {
    gchar *name;

    name = g_new0 (gchar, dot - filename + 5);
    strncpy (name, filename, dot - filename + 1);

    if (*extension == '.')
      strcat (name, extension + 1);
    else
      strcat (name, extension);

    return name;
  }
}

