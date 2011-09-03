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

#include "ogmrip-hardsub.h"

#include <glib/gi18n-lib.h>

G_DEFINE_TYPE (OGMRipHardSub, ogmrip_hardsub, OGMRIP_TYPE_SUBP_CODEC)

static void
ogmrip_hardsub_class_init (OGMRipHardSubClass *klass)
{
}

static void
ogmrip_hardsub_init (OGMRipHardSub *hardsub)
{
}

static OGMRipSubpPlugin hardsub_plugin =
{
  NULL,
  G_TYPE_NONE,
  "hardsub",
  N_("Hardcoded subtitle"),
  0,
  FALSE
};

OGMRipSubpPlugin *
ogmrip_hardsub_get_plugin (void)
{
  hardsub_plugin.type = OGMRIP_TYPE_HARDSUB;

  return &hardsub_plugin;
}

