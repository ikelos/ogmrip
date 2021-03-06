/* OGMRipLavcMpeg - Mpeg 1, 2 and 4 plugins for OGMRip
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

#include <ogmrip-mplayer.h>
#include <ogmrip-encode.h>
#include <ogmrip-module.h>

#include <glib/gi18n-lib.h>

typedef struct _OGMRipLavcMpeg1      OGMRipLavcMpeg1;
typedef struct _OGMRipLavcMpeg1Class OGMRipLavcMpeg1Class;

struct _OGMRipLavcMpeg1
{
  OGMRipLavc parent_instance;
};

struct _OGMRipLavcMpeg1Class
{
  OGMRipLavcClass parent_class;
};

GType ogmrip_lavc_mpeg1_get_type (void);

G_DEFINE_TYPE (OGMRipLavcMpeg1, ogmrip_lavc_mpeg1, OGMRIP_TYPE_LAVC)

static const gchar *
ogmrip_lavc_mpeg1_get_codec (void)
{
  const gchar *codec = "mpeg1video";

  return codec;
}

static void
ogmrip_lavc_mpeg1_class_init (OGMRipLavcMpeg1Class *klass)
{
  OGMRipLavcClass *lavc_class;

  lavc_class = (OGMRipLavcClass *) klass;
  lavc_class->get_codec = ogmrip_lavc_mpeg1_get_codec;
}

static void
ogmrip_lavc_mpeg1_init (OGMRipLavcMpeg1 *lavc_mpeg1)
{
}

typedef struct _OGMRipLavcMpeg2      OGMRipLavcMpeg2;
typedef struct _OGMRipLavcMpeg2Class OGMRipLavcMpeg2Class;

struct _OGMRipLavcMpeg2
{
  OGMRipLavc parent_instance;
};

struct _OGMRipLavcMpeg2Class
{
  OGMRipLavcClass parent_class;
};

GType ogmrip_lavc_mpeg2_get_type (void);

static gboolean ogmrip_lavc_mpeg2_run (OGMJobTask   *task,
                                       GCancellable *cancellable,
                                       GError       **error);

G_DEFINE_TYPE (OGMRipLavcMpeg2, ogmrip_lavc_mpeg2, OGMRIP_TYPE_LAVC)

static const gchar *
ogmrip_lavc_mpeg2_get_codec (void)
{
  const gchar *codec = "mpeg2video";

  return codec;
}

static void
ogmrip_lavc_mpeg2_class_init (OGMRipLavcMpeg2Class *klass)
{
  OGMJobTaskClass *task_class;
  OGMRipLavcClass *lavc_class;

  task_class = (OGMJobTaskClass *) klass;
  task_class->run = ogmrip_lavc_mpeg2_run;

  lavc_class = (OGMRipLavcClass *) klass;
  lavc_class->get_codec = ogmrip_lavc_mpeg2_get_codec;
}

static void
ogmrip_lavc_mpeg2_init (OGMRipLavcMpeg2 *lavc_mpeg2)
{
}

static gboolean
ogmrip_lavc_mpeg2_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  ogmrip_lavc_set_v4mv (OGMRIP_LAVC (task), FALSE);

  return OGMJOB_TASK_CLASS (ogmrip_lavc_mpeg2_parent_class)->run (task, cancellable, error);
}

typedef struct _OGMRipLavcMpeg4      OGMRipLavcMpeg4;
typedef struct _OGMRipLavcMpeg4Class OGMRipLavcMpeg4Class;

struct _OGMRipLavcMpeg4
{
  OGMRipLavc parent_instance;
};

struct _OGMRipLavcMpeg4Class
{
  OGMRipLavcClass parent_class;
};

GType ogmrip_lavc_mpeg4_get_type (void);

G_DEFINE_TYPE (OGMRipLavcMpeg4, ogmrip_lavc_mpeg4, OGMRIP_TYPE_LAVC)

static void
ogmrip_lavc_mpeg4_class_init (OGMRipLavcMpeg4Class *klass)
{
}

static void
ogmrip_lavc_mpeg4_init (OGMRipLavcMpeg4 *lavc_mpeg4)
{
}

void
ogmrip_module_load (OGMRipModule *module)
{
  gchar *output;
  gboolean match;

  if (!g_spawn_command_line_sync ("mencoder -ovc help", &output, NULL, NULL, NULL))
  {
    g_warning (_("MEncoder is missing"));
    return;
  }

  match = g_regex_match_simple ("^ *lavc *- .*$", output, G_REGEX_MULTILINE, 0);
  g_free (output);

  if (!match)
  {
    g_warning (_("MEncoder is built without LAVC support"));
    return;
  }

  ogmrip_register_codec (ogmrip_lavc_mpeg1_get_type (),
      "lavc-mpeg1", _("Lavc Mpeg-1"), OGMRIP_FORMAT_MPEG1,
      "schema-id", "org.ogmrip.lavc", "schema-name", "lavc", NULL);

  ogmrip_register_codec (ogmrip_lavc_mpeg2_get_type (),
      "lavc-mpeg2", _("Lavc Mpeg-2"), OGMRIP_FORMAT_MPEG2,
      "schema-id", "org.ogmrip.lavc", "schema-name", "lavc", NULL);

  ogmrip_register_codec (ogmrip_lavc_mpeg4_get_type (),
      "lavc-mpeg4", _("Lavc Mpeg-4"), OGMRIP_FORMAT_MPEG4,
      "schema-id", "org.ogmrip.lavc", "schema-name", "lavc", NULL);
}

