/* OGMRip - A media encoder for GNOME
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-application.h"

enum
{
  PREPARE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_INTERFACE (OGMRipApplication, ogmrip_application, G_TYPE_APPLICATION);

static void
ogmrip_application_default_init (OGMRipApplicationInterface *iface)
{
  signals[PREPARE] = g_signal_new ("prepare", G_TYPE_FROM_INTERFACE (iface),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipApplicationInterface, prepare), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

void
ogmrip_application_prepare (OGMRipApplication *app)
{
  g_return_if_fail (OGMRIP_IS_APPLICATION (app));

  g_signal_emit (app, signals[PREPARE], 0);
}

gboolean
ogmrip_application_get_is_prepared (OGMRipApplication *app)
{
  OGMRipApplicationInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_APPLICATION (app), FALSE);

  iface = OGMRIP_APPLICATION_GET_IFACE (app);

  if (!iface->get_is_prepared)
    return FALSE;

  return iface->get_is_prepared (app);
}

