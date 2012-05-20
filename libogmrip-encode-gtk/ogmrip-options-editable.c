/* OGMRip - A library for DVD ripping and encoding
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

#include "ogmrip-options-editable.h"

#include <gtk/gtk.h>

G_DEFINE_INTERFACE (OGMRipOptionsEditable, ogmrip_options_editable, GTK_TYPE_DIALOG);

static void
ogmrip_options_editable_default_init (OGMRipOptionsEditableInterface *iface)
{
  g_object_interface_install_property (iface,
      g_param_spec_object ("profile", "Profile property", "Set profile",
        OGMRIP_TYPE_PROFILE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
}

OGMRipProfile *
ogmrip_options_editable_get_profile (OGMRipOptionsEditable *editable)
{
  OGMRipProfile *profile;

  g_return_val_if_fail (OGMRIP_IS_OPTIONS_EDITABLE (editable), NULL);

  g_object_get (editable, "profile", &profile, NULL);
  if (profile)
    g_object_unref (profile);

  return profile;
}

void
ogmrip_options_editable_set_profile (OGMRipOptionsEditable *editable, OGMRipProfile *profile)
{
  g_return_if_fail (OGMRIP_IS_OPTIONS_EDITABLE (editable));
  g_return_if_fail (OGMRIP_IS_PROFILE (profile));

  g_object_set (editable, "profile", profile, NULL);
}

