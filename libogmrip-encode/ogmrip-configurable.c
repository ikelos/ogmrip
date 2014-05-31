/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2010-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-configurable.h"

#include <ogmrip-job.h>

G_DEFINE_INTERFACE (OGMRipConfigurable, ogmrip_configurable, OGMJOB_TYPE_BIN);

static void
ogmrip_configurable_default_init (OGMRipConfigurableInterface *iface)
{
}

void
ogmrip_configurable_configure (OGMRipConfigurable *configurable, OGMRipProfile *profile)
{
  OGMRipConfigurableInterface *iface;

  g_return_if_fail (OGMRIP_IS_CONFIGURABLE (configurable));
  g_return_if_fail (OGMRIP_IS_PROFILE (profile));

  iface = OGMRIP_CONFIGURABLE_GET_IFACE (configurable);

  if (iface->configure)
    return iface->configure (configurable, profile);
}

