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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ogmrip-job.h>
#include <ogmrip-encode.h>
#include <ogmrip-mplayer.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#define OGMRIP_TYPE_VOBSUB          (ogmrip_vobsub_get_type ())
#define OGMRIP_VOBSUB(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_VOBSUB, OGMRipVobSub))
#define OGMRIP_VOBSUB_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_VOBSUB, OGMRipVobSubClass))
#define OGMRIP_IS_VOBSUB(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_VOBSUB))
#define OGMRIP_IS_VOBSUB_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_VOBSUB))

typedef struct _OGMRipVobSub      OGMRipVobSub;
typedef struct _OGMRipVobSubClass OGMRipVobSubClass;

struct _OGMRipVobSub
{
  OGMRipSubpCodec parent_instance;
};

struct _OGMRipVobSubClass
{
  OGMRipSubpCodecClass parent_class;
};

static gint ogmrip_vobsub_run      (OGMJobSpawn *spawn);
static void ogmrip_vobsub_finalize (GObject     *gobject);

G_DEFINE_TYPE (OGMRipVobSub, ogmrip_vobsub, OGMRIP_TYPE_SUBP_CODEC)

static gchar **
ogmrip_vobsub_command (OGMRipSubpCodec *subp, const gchar *input, const gchar *output)
{
  GPtrArray *argv;

  argv = ogmrip_mencoder_vobsub_command (subp, output);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static void
ogmrip_vobsub_class_init (OGMRipVobSubClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  spawn_class = OGMJOB_SPAWN_CLASS (klass);

  gobject_class->finalize = ogmrip_vobsub_finalize;
  spawn_class->run = ogmrip_vobsub_run;
}

static void
ogmrip_vobsub_init (OGMRipVobSub *vobsub)
{
}

static void
ogmrip_vobsub_finalize (GObject *gobject)
{
  const gchar *output;

  output = ogmrip_codec_get_output (OGMRIP_CODEC (gobject));
  if (output)
  {
/*
    if (ogmrip_codec_get_unlink_on_unref (OGMRIP_CODEC (gobject)))
    {
      gchar *filename;

      filename = g_strconcat (output, ".idx", NULL);
      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        g_unlink (filename);
      g_free (filename);

      filename = g_strconcat (output, ".sub", NULL);
      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        g_unlink (filename);
      g_free (filename);
    }
*/
  }

  G_OBJECT_CLASS (ogmrip_vobsub_parent_class)->finalize (gobject);
}

static gboolean
ogmrip_vobsub_set_foo (OGMJobSpawn *spawn, const gchar *filename)
{
  GError *error;
  gssize w;
  gint fd;

  fd = g_open (filename, O_WRONLY);
  if (fd < 0)
  {
    error = g_error_new (G_FILE_ERROR, g_file_error_from_errno (errno),
        "Cannot open file '%s': %s", filename, g_strerror (errno));
    ogmjob_spawn_propagate_error (spawn, error);

    return FALSE;
  }

  w = write (fd, "foo", 3);
  close (fd);

  if (w != 3)
  {
    error = g_error_new (G_FILE_ERROR, g_file_error_from_errno (errno),
        "Cannot write to file '%s': %s", filename, g_strerror (errno));
    ogmjob_spawn_propagate_error (spawn, error);

    return FALSE;
  }

  return TRUE;
}

#define FORCED_SUBS_LINE "forced subs: ON"

static gboolean
ogmrip_vobsub_set_forced (OGMJobSpawn *spawn, const gchar *filename)
{
  GError *error = NULL;
  gchar *content, **vline;

  if (!g_file_get_contents (filename, &content, NULL, &error))
  {
    ogmjob_spawn_propagate_error (spawn, error);

    return FALSE;
  }

  vline = g_strsplit_set (content, "\r\n", -1);
  g_free (content);

  if (vline)
  {
    gint fd, i, w, len;

    fd = g_open (filename, O_WRONLY);
    if (fd < 0)
    {
      error = g_error_new (G_FILE_ERROR, g_file_error_from_errno (errno),
          "Cannot open file '%s': %s", filename, g_strerror (errno));
      ogmjob_spawn_propagate_error (spawn, error);

      return FALSE;
    }

    for (i = 0; vline[i]; i++)
    {
      if (g_ascii_strncasecmp (vline[i], "forced subs:", 12) == 0)
      {
        len = strlen (FORCED_SUBS_LINE);
        w = write (fd, FORCED_SUBS_LINE, len);
      }
      else
      {
        len = strlen (vline[i]);
        w = write (fd, vline[i], len);
      }

      if (w != len || write (fd, "\n", 1) != 1)
      {
        close (fd);
        g_strfreev (vline);

        error = g_error_new (G_FILE_ERROR, g_file_error_from_errno (errno),
            "Cannot write to file '%s': %s", filename, g_strerror (errno));
        ogmjob_spawn_propagate_error (spawn, error);

        return FALSE;
      }
    }

    close (fd);

    g_strfreev (vline);
  }

  return TRUE;
}

static gint
ogmrip_vobsub_run (OGMJobSpawn *spawn)
{
  OGMJobSpawn *child;
  gchar **argv;
  gint result;

  argv = ogmrip_vobsub_command (OGMRIP_SUBP_CODEC (spawn), NULL, NULL);
  if (!argv)
    return OGMJOB_RESULT_ERROR;

  child = ogmjob_exec_newv (argv);
  ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_mencoder_vobsub_watch, spawn, TRUE, FALSE, FALSE);
  ogmjob_container_add (OGMJOB_CONTAINER (spawn), child);
  g_object_unref (child);

  result = OGMJOB_SPAWN_CLASS (ogmrip_vobsub_parent_class)->run (spawn);
  if (result == OGMJOB_RESULT_SUCCESS)
  {
    struct stat buf; 
    const gchar *basename;
    gchar *idxname, *subname;

    basename = ogmrip_codec_get_output (OGMRIP_CODEC (spawn));
    idxname = g_strconcat (basename, ".idx", NULL);
    subname = g_strconcat (basename, ".sub", NULL);

    if ((g_file_test (idxname, G_FILE_TEST_IS_REGULAR) &&
         g_stat (idxname, &buf) == 0 && buf.st_size > 0) &&
        (g_file_test (subname, G_FILE_TEST_IS_REGULAR) &&
         g_stat (subname, &buf) == 0 && buf.st_size > 0))
    {
      if (!ogmrip_vobsub_set_foo (spawn, basename))
        return OGMJOB_RESULT_ERROR;

      if (ogmrip_subp_codec_get_forced_only (OGMRIP_SUBP_CODEC (spawn)) &&
          !ogmrip_vobsub_set_forced (spawn, idxname))
        return OGMJOB_RESULT_ERROR;
    }

    g_free (idxname);
    g_free (subname);
  }

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), child);

  return result;
}

static OGMRipSubpPlugin vobsub_plugin =
{
  NULL,
  G_TYPE_NONE,
  "vobsub",
  N_("VobSub"),
  OGMRIP_FORMAT_VOBSUB,
  FALSE
};

OGMRipSubpPlugin *
ogmrip_init_plugin (GError **error)
{
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (!ogmrip_check_mencoder ())
  {
    g_set_error (error, OGMRIP_PLUGIN_ERROR, OGMRIP_PLUGIN_ERROR_REQ, _("MEncoder is missing"));
    return NULL;
  }

  vobsub_plugin.type = OGMRIP_TYPE_VOBSUB;

  return &vobsub_plugin;
}

