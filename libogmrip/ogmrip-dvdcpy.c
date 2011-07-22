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
 * SECTION:ogmrip-dvdcpy
 * @title: OGMRipDvdcpy
 * @short_description: A codec to copy a DVD title
 * @include: ogmrip-dvdcpy.h
 */

#include "ogmrip-dvdcpy.h"

#include "ogmjob-exec.h"

#include <string.h>
#include <unistd.h>

#include <glib/gstdio.h>

static gint ogmrip_dvdcpy_run (OGMJobSpawn *spawn);

static gdouble
ogmrip_dvdcpy_watch (OGMJobExec *exec, const gchar *buffer, OGMRipVideoCodec *video)
{
  guint bytes, total, percent;

  if (sscanf (buffer, "%u/%u blocks written (%u%%)", &bytes, &total, &percent) == 3)
    return percent / 100.;

  return -1;
}

static gchar **
ogmrip_dvdcpy_command (OGMRipDvdCpy *dvdcpy, const gchar *input, const gchar *output)
{
  OGMDvdStream *stream;
  GPtrArray *argv;

  const gchar *device;
  gint vid;

  g_return_val_if_fail (OGMRIP_IS_DVDCPY (dvdcpy), NULL);

  if (!output)
    output = ogmrip_codec_get_output (OGMRIP_CODEC (dvdcpy));

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (dvdcpy));

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("dvdcpy"));
  g_ptr_array_add (argv, g_strdup ("-s"));
  g_ptr_array_add (argv, g_strdup ("skip"));
  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (output));
  g_ptr_array_add (argv, g_strdup ("-m"));

  vid = ogmdvd_title_get_nr (ogmdvd_stream_get_title (stream));
  g_ptr_array_add (argv, g_strdup ("-t"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", vid + 1));

  device = ogmdvd_disc_get_device (ogmdvd_title_get_disc (ogmdvd_stream_get_title (stream)));
  g_ptr_array_add (argv, g_strdup (device));

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

G_DEFINE_TYPE (OGMRipDvdCpy, ogmrip_dvdcpy, OGMRIP_TYPE_CODEC)

static void
ogmrip_dvdcpy_class_init (OGMRipDvdCpyClass *klass)
{
  OGMJobSpawnClass *spawn_class;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);

  spawn_class->run = ogmrip_dvdcpy_run;
}

static void
ogmrip_dvdcpy_init (OGMRipDvdCpy *dvdcpy)
{
}

static gint
ogmrip_dvdcpy_run (OGMJobSpawn *spawn)
{
  OGMJobSpawn *child;
  gchar **argv;
  gint result;

  argv = ogmrip_dvdcpy_command (OGMRIP_DVDCPY (spawn), NULL, NULL);
  if (!argv)
    return OGMJOB_RESULT_ERROR;

  child = ogmjob_exec_newv (argv);
  ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_dvdcpy_watch, spawn, TRUE, FALSE, FALSE);
  ogmjob_container_add (OGMJOB_CONTAINER (spawn), child);
  g_object_unref (child);

  result = OGMJOB_SPAWN_CLASS (ogmrip_dvdcpy_parent_class)->run (spawn);

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), child);

  return result;
}

/**
 * ogmrip_dvdcpy_new:
 * @title: An #OGMDvdTitle
 * @output: The output file
 *
 * Creates a new #OGMRipDvdcpy.
 *
 * Returns: The new #OGMRipDvdcpy
 */
OGMJobSpawn *
ogmrip_dvdcpy_new (OGMDvdTitle *title, const gchar *output)
{
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (output && *output, NULL);

  return g_object_new (OGMRIP_TYPE_DVDCPY, "input", title, "output", output, NULL);
}

