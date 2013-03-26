/* OGMRipBase - A foundation library for OGMRip
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
 * SECTION:ogmrip-log
 * @title: Logs
 * @include: ogmrip-log.h
 * @short_description: Support for logging the output of spawns
 */

#include "ogmrip-log.h"

#include <string.h>

static GIOChannel *channel     = NULL;
static gboolean   print_stdout = FALSE;
static gboolean   print_stderr = FALSE;

/**
 * ogmrip_log_open:
 * @filename: A filename
 * @error: Location to store the error occuring, or NULL to ignore errors.
 *
 * Opens a new log file. 
 *
 * Returns: %TRUE, when no error
 */
gboolean
ogmrip_log_open (const gchar *filename, GError **error)
{
  GError *tmp_error = NULL;

  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (!ogmrip_log_close (&tmp_error))
  {
    g_propagate_error (error, tmp_error);
    return FALSE;
  }

  channel = g_io_channel_new_file (filename, "w", &tmp_error);
  if (!channel)
  {
    g_propagate_error (error, tmp_error);
    return FALSE;
  }

  g_io_channel_set_close_on_unref (channel, TRUE);

  return TRUE;
}

/**
 * ogmrip_log_close:
 * @error: Location to store the error occuring, or NULL to ignore errors.
 *
 * Closes the log file. 
 *
 * Returns: %TRUE, when no error
 */
gboolean
ogmrip_log_close (GError **error)
{
  GError *tmp_error = NULL;
  GIOStatus status;

  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (!channel)
    return TRUE;

  status = g_io_channel_shutdown (channel, TRUE, &tmp_error);
  if (status == G_IO_STATUS_ERROR)
    g_propagate_error (error, tmp_error);

  g_io_channel_unref (channel);
  channel = NULL;

  return status == G_IO_STATUS_NORMAL;
}

/**
 * ogmrip_log_write:
 * @str: A string to log
 *
 * Logs some information on stdout, stderr, and/or a file.
 */
void
ogmrip_log_write (const gchar *str)
{
  g_return_if_fail (str != NULL);

  if (channel)
  {
    gint len;

    len = strlen (str);
    if (len >= 1 && str[len - 1] == '\r')
    {
      g_io_channel_write_chars (channel, str, len - 1, NULL, NULL);
      g_io_channel_write_chars (channel, "\n", 1, NULL, NULL);
    }
    else
      g_io_channel_write_chars (channel, str, len, NULL, NULL);
  }

  if (print_stdout)
    g_print ("%s", str);

  if (print_stderr)
    g_printerr ("%s", str);
}

/**
 * ogmrip_log_printf:
 * @format: A message format
 * @...: The parameters of the format string
 *
 * Logs some formatted information on stdout, stderr, and/or a file.
 */
void
ogmrip_log_printf (const gchar *format, ...)
{
  va_list args;
  gchar *str;

  g_return_if_fail (format != NULL);

  va_start (args, format);
  str = g_strdup_vprintf (format, args);
  va_end (args);

  ogmrip_log_write (str);

  g_free (str);
}

/**
 * ogmrip_log_set_print_stdout:
 * @log_stdout: %TRUE to log on stdout
 *
 * Sets whether to log information on stdout.
 */
void
ogmrip_log_set_print_stdout (gboolean log_stdout)
{
  print_stdout = log_stdout;
}

/**
 * ogmrip_log_get_print_stdout:
 *
 * Gets whether to log information on stdout.
 *
 * Returns: %TRUE if log on stdout
 */
gboolean 
ogmrip_log_get_print_stdout (void)
{
  return print_stdout;
}

/**
 * ogmrip_log_set_print_stderr:
 * @log_stderr: %TRUE to log on stderr
 *
 * Sets whether to log information on stderr.
 */
void
ogmrip_log_set_print_stderr (gboolean log_stderr)
{
  print_stderr = log_stderr;
}

/**
 * ogmrip_log_get_print_stderr:
 *
 * Gets whether to log information on stderr.
 *
 * Returns: %TRUE if log on stderr
 */
gboolean 
ogmrip_log_get_print_stderr (void)
{
  return print_stderr;
}

