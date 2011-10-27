/* OGMRip - A library for DVD ripping and encoding
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ogmrip-lavc.h>
#include <ogmrip-lavc-mpeg4.h>
#include <ogmrip-module.h>

#include <glib/gi18n-lib.h>

#define OGMRIP_LAVC_MPEG4(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_LAVC_MPEG4, OGMRipLavcMpeg4))
#define OGMRIP_LAVC_MPEG4_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_LAVC_MPEG4, OGMRipLavcMpeg4Class))
#define OGMRIP_IS_LAVC_MPEG4(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_LAVC_MPEG4))
#define OGMRIP_IS_LAVC_MPEG4_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_LAVC_MPEG4))
#define OGMRIP_LAVC_MPEG4_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_LAVC_MPEG4, OGMRipLavcMpeg4Class))

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

  ogmrip_register_codec (OGMRIP_TYPE_LAVC_MPEG4,
      "lavc-mpeg4", _("Lavc Mpeg-4"), OGMRIP_FORMAT_MPEG4);
}

