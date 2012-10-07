/* OGMRipDirac - A dirac plugin for OGMRip
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

#include <ogmrip-encode.h>
#include <ogmrip-module.h>
#include <ogmrip-mplayer.h>

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

typedef struct _OGMRipDirac OGMRipDirac;
typedef struct _OGMRipDiracClass OGMRipDiracClass;

struct _OGMRipDirac
{
  OGMRipLavc parent_instance;
};

struct _OGMRipDiracClass
{
  OGMRipLavcClass parent_class;
};

enum
{
  PROP_0,
  PROP_PASSES
};

static void ogmrip_dirac_get_property (GObject      *gobject,
                                       guint        property_id,
                                       GValue       *value,
                                       GParamSpec   *pspec);
static void ogmrip_dirac_set_property (GObject      *gobject,
                                       guint        property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);

static gboolean have_dirac = FALSE;
static gboolean have_schro = FALSE;

const gchar *
ogmrip_dirac_get_codec (void)
{
  if (have_dirac)
    return "libdirac";

  return "libschroedinger";
}

static void
ogmrip_dirac_class_init (OGMRipDiracClass *klass)
{
  GObjectClass *gobject_class;
  OGMRipLavcClass *lavc_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_dirac_get_property;
  gobject_class->set_property = ogmrip_dirac_set_property;

  lavc_class = OGMRIP_LAVC_CLASS (klass);
  lavc_class->get_codec = ogmrip_dirac_get_codec;

  g_object_class_install_property (gobject_class, PROP_PASSES,
        g_param_spec_uint ("passes", "Passes property", "Set the number of passes",
           1, 1, 1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_dirac_init (OGMRipDirac *nouveau)
{
}

static void
ogmrip_dirac_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_PASSES:
      g_value_set_uint (value, ogmrip_video_codec_get_passes (OGMRIP_VIDEO_CODEC (gobject)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_dirac_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_PASSES:
      ogmrip_video_codec_set_passes (OGMRIP_VIDEO_CODEC (gobject), g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

G_DEFINE_TYPE (OGMRipDirac, ogmrip_dirac, OGMRIP_TYPE_LAVC)

void
ogmrip_module_load (OGMRipModule *module)
{
  gchar *output, *str;

  if (!ogmrip_check_mencoder ())
  {
    g_warning (_("MEncoder is missing"));
    return;
  }

  if (!g_spawn_command_line_sync ("mplayer -noconfig all -vc help", &output, &str, NULL, NULL))
  {
    g_warning (_("MPlayer is missing"));
    return;
  }

  g_free (str);

  have_dirac = g_regex_match_simple ("^fflibdirac", output, G_REGEX_MULTILINE, 0);
  if (!have_dirac)
    have_schro = g_regex_match_simple ("^fflibschroedinger", output, G_REGEX_MULTILINE, 0);

  g_free (output);

  if (!have_dirac && !have_schro)
  {
    g_warning (_("MPlayer is built without Dirac support"));
    return;
  }

  ogmrip_register_codec (ogmrip_dirac_get_type (),
      "dirac", _("Dirac"), OGMRIP_FORMAT_DIRAC, NULL);
}

