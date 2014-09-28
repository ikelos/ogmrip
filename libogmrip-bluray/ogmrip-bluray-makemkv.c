/* OGMRipBluray - A bluray library for OGMRip
 * Copyright (C) 2012 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-bluray-makemkv.h"
#include "ogmrip-bluray-priv.h"

#include <stdio.h>

typedef struct
{
  guint index;
  gchar *device;
} DriveData;
static OGMBrMakeMKV *default_mmkv;

static void
free_drive_data (DriveData *data)
{
  g_free (data->device);
  g_free (data);
}

G_DEFINE_TYPE (OGMBrMakeMKV, ogmbr_makemkv, OGMJOB_TYPE_BIN);

static void
ogmbr_makemkv_finalize (GObject *gobject)
{
  OGMBrMakeMKV *mmkv = OGMBR_MAKEMKV (gobject);

  g_list_foreach (mmkv->priv->drives, (GFunc) free_drive_data, NULL);
  g_list_free (mmkv->priv->drives);

  G_OBJECT_CLASS (ogmbr_makemkv_parent_class)->finalize (gobject);
}

static void
ogmbr_makemkv_init (OGMBrMakeMKV *makemkv)
{
  makemkv->priv = G_TYPE_INSTANCE_GET_PRIVATE (makemkv, OGMBR_TYPE_MAKEMKV, OGMBrMakeMKVPriv);
}

static void
ogmbr_makemkv_class_init (OGMBrMakeMKVClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = ogmbr_makemkv_finalize;

  g_type_class_add_private (klass, sizeof (OGMBrMakeMKVPriv));
}

static gboolean
ogmbr_makemkv_parse_prgv (OGMJobTask *task, const gchar *buffer)
{
  guint current, total, max;

  if (sscanf (buffer, "PRGV:%u,%u,%u", &current, &total, &max) != 3)
    return FALSE;

  ogmjob_task_set_progress (task, total / (gdouble) max);

  return TRUE;
}

static gboolean
ogmbr_makemkv_parse_drv (OGMBrMakeMKV *mmkv, const gchar *buffer)
{
  DriveData *drive;
  guint index, visible, enabled, flags;
  gchar name[1024], label[1024], device[1024];

  if (sscanf (buffer, "DRV:%u,%u,%u,%u,\"%s\",\"%s\",\"%s\"",
        &index, &visible, &enabled, &flags, name, label, device) != 6)
    return FALSE;

  if (visible > 0 && enabled > 0)
  {
    drive = g_new0 (DriveData, 1);
    drive->device = g_strdup (device);

    mmkv->priv->drives = g_list_append (mmkv->priv->drives, drive);
  }

  return TRUE;
}

OGMBrMakeMKV *
ogmbr_makemkv_get_default (void)
{
  if (!default_mmkv)
    default_mmkv = g_object_new (OGMBR_TYPE_MAKEMKV, NULL);
  else
    g_object_ref (default_mmkv);

  return default_mmkv;
}

static gboolean
ogmbr_makemkv_update_drives_watch (OGMJobTask *task, const gchar *buffer, OGMBrMakeMKV *mmkv, GError **error)
{
  if (ogmbr_makemkv_parse_prgv (task, buffer))
    return TRUE;

  if (ogmbr_makemkv_parse_drv (mmkv, buffer))
    return TRUE;

  return TRUE;
}

static gboolean
ogmbr_makemkv_update_drives (OGMBrMakeMKV *mmkv, GCancellable *cancellable, GError **error)
{
  GPtrArray *argv;
  OGMJobTask *spawn;
  gboolean retval;

  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("makemkvcon"));
  g_ptr_array_add (argv, g_strdup ("--robot"));
  g_ptr_array_add (argv, g_strdup ("--noscan"));
  g_ptr_array_add (argv, g_strdup ("--cache=1"));
  g_ptr_array_add (argv, g_strdup ("--progress=-same"));
  g_ptr_array_add (argv, g_strdup ("info"));
  g_ptr_array_add (argv, g_strdup ("disc:9999"));
  g_ptr_array_add (argv, NULL);

  spawn = ogmjob_spawn_newv ((gchar **) g_ptr_array_free (argv, FALSE));
  ogmjob_container_add (OGMJOB_CONTAINER (mmkv), spawn);
  g_object_unref (spawn);

  ogmjob_spawn_set_watch (OGMJOB_SPAWN (spawn), OGMJOB_STREAM_OUTPUT,
      (OGMJobWatch) ogmbr_makemkv_update_drives_watch, mmkv, NULL);

  retval = ogmjob_task_run (OGMJOB_TASK (mmkv), cancellable, error);
  ogmjob_task_set_progress (spawn, 1.0);

  ogmjob_container_remove (OGMJOB_CONTAINER (mmkv), spawn);

  return retval;
}

static gboolean
ogmbr_makemkv_backup_disc_watch (OGMJobTask *task, const gchar *buffer, OGMBrDisc *disc, GError **error)
{
  if (ogmbr_makemkv_parse_prgv (task, buffer))
    return TRUE;

  return TRUE;
}

gboolean
ogmbr_makemkv_backup_disc (OGMBrMakeMKV *mmkv, OGMBrDisc *disc, const gchar *output, GCancellable *cancellable, GError **error)
{
  GPtrArray *argv;
  OGMJobTask *spawn;
  gboolean retval;

  GList *link;
  gint nr = -1;

  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (OGMBR_IS_DISC (disc), FALSE);
  g_return_val_if_fail (output != NULL, FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (!mmkv->priv->drives && !ogmbr_makemkv_update_drives (mmkv, cancellable, error))
    return FALSE;

  for (link = mmkv->priv->drives; link && nr < 0; link = link->next)
  {
    DriveData *data = link->data;

    if (g_str_equal (data->device, disc->priv->device))
      nr = data->index;
  }

  if (nr < 0)
    return FALSE;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("makemkvcon"));
  g_ptr_array_add (argv, g_strdup ("--robot"));
  g_ptr_array_add (argv, g_strdup ("--noscan"));
  g_ptr_array_add (argv, g_strdup ("--cache=16"));
  g_ptr_array_add (argv, g_strdup ("--decrypt"));
  g_ptr_array_add (argv, g_strdup ("--progress=-same"));
  g_ptr_array_add (argv, g_strdup ("backup"));
  g_ptr_array_add (argv, g_strdup_printf ("disc:%d", nr));
  g_ptr_array_add (argv, g_strdup_printf ("%s", output));
  g_ptr_array_add (argv, NULL);

  spawn = ogmjob_spawn_newv ((gchar **) g_ptr_array_free (argv, FALSE));
  ogmjob_container_add (OGMJOB_CONTAINER (mmkv), spawn);
  g_object_unref (spawn);

  ogmjob_spawn_set_watch (OGMJOB_SPAWN (spawn), OGMJOB_STREAM_OUTPUT,
      (OGMJobWatch) ogmbr_makemkv_backup_disc_watch, disc, NULL);

  retval = ogmjob_task_run (OGMJOB_TASK (mmkv), cancellable, error);
  ogmjob_task_set_progress (spawn, 1.0);

  ogmjob_container_remove (OGMJOB_CONTAINER (mmkv), spawn);

  return retval;
}

