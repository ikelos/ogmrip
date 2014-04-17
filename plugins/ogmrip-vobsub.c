/* OGMRipVobsub - A VobSub plugin for OGMRip
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ogmrip-encode.h>
#include <ogmrip-mplayer.h>
#include <ogmrip-module.h>

#include <errno.h>
#include <fcntl.h>

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

static void     ogmrip_vobsub_finalize (GObject      *gobject);
static gboolean ogmrip_vobsub_run      (OGMJobTask   *task,
                                        GCancellable *cancellable,
                                        GError       **error);

G_DEFINE_TYPE (OGMRipVobSub, ogmrip_vobsub, OGMRIP_TYPE_SUBP_CODEC)

static void
ogmrip_vobsub_class_init (OGMRipVobSubClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobTaskClass *task_class;

  gobject_class = G_OBJECT_CLASS (klass);
  task_class = OGMJOB_TASK_CLASS (klass);

  gobject_class->finalize = ogmrip_vobsub_finalize;
  task_class->run = ogmrip_vobsub_run;
}

static void
ogmrip_vobsub_init (OGMRipVobSub *vobsub)
{
}

static void
ogmrip_vobsub_finalize (GObject *gobject)
{
  const gchar *output;

  output = ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (gobject)));
  if (output)
  {
/*
    if (ogmrip_codec_get_unlink_on_unref (OGMRIP_CODEC (gobject)))
    {
      gchar *filename;

      filename = g_strconcat (output, ".idx", NULL);
      g_unlink (filename);
      g_free (filename);

      filename = g_strconcat (output, ".sub", NULL);
      g_unlink (filename);
      g_free (filename);
    }
*/
  }

  G_OBJECT_CLASS (ogmrip_vobsub_parent_class)->finalize (gobject);
}

static gboolean
ogmrip_vobsub_set_foo (OGMJobTask *task, const gchar *filename, GError **error)
{
  gssize w;
  gint fd;

  fd = g_open (filename, O_WRONLY);
  if (fd < 0)
  {
    g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
        "Cannot open file '%s': %s", filename, g_strerror (errno));
    return FALSE;
  }

  w = write (fd, "foo", 3);
  close (fd);

  if (w != 3)
  {
    g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
        "Cannot write to file '%s': %s", filename, g_strerror (errno));
    return FALSE;
  }

  return TRUE;
}

#define FORCED_SUBS_LINE "forced subs: ON"

static gboolean
ogmrip_vobsub_set_forced (OGMJobTask *task, const gchar *filename, GError **error)
{
  gchar *content, **vline;

  if (!g_file_get_contents (filename, &content, NULL, error))
    return FALSE;

  vline = g_strsplit_set (content, "\r\n", -1);
  g_free (content);

  if (vline)
  {
    gint fd, i, w, len;

    fd = g_open (filename, O_WRONLY);
    if (fd < 0)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
          "Cannot open file '%s': %s", filename, g_strerror (errno));
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

        g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
            "Cannot write to file '%s': %s", filename, g_strerror (errno));
        return FALSE;
      }
    }

    close (fd);

    g_strfreev (vline);
  }

  return TRUE;
}

static gboolean
ogmrip_vobsub_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobTask *child;
  gboolean result;

  child = ogmrip_mencoder_vobsub_command (OGMRIP_SUBP_CODEC (task),
      ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (task))));
  ogmjob_container_add (OGMJOB_CONTAINER (task), child);
  g_object_unref (child);

  result = OGMJOB_TASK_CLASS (ogmrip_vobsub_parent_class)->run (task, cancellable, error);
  if (result)
  {
    struct stat buf; 
    const gchar *basename;
    gchar *idxname, *subname;

    basename = ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (task)));
    idxname = g_strconcat (basename, ".idx", NULL);
    subname = g_strconcat (basename, ".sub", NULL);

    if ((g_stat (idxname, &buf) == 0 && buf.st_size > 0) &&
        (g_stat (subname, &buf) == 0 && buf.st_size > 0))
    {
      if (!ogmrip_vobsub_set_foo (task, basename, error))
        return FALSE;

      if (ogmrip_subp_codec_get_forced_only (OGMRIP_SUBP_CODEC (task)) &&
          !ogmrip_vobsub_set_forced (task, idxname, error))
        return FALSE;
    }

    g_free (idxname);
    g_free (subname);
  }

  ogmjob_container_remove (OGMJOB_CONTAINER (task), child);

  return result;
}

void
ogmrip_module_load (OGMRipModule *module)
{
  if (!ogmrip_check_mencoder ())
  {
    g_warning (_("MEncoder is missing"));
    return;
  }

  ogmrip_register_codec (OGMRIP_TYPE_VOBSUB,
      "vobsub", _("VobSub"), OGMRIP_FORMAT_VOBSUB, NULL);
}

