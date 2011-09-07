/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2010-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_CONFIGURABLE_H__
#define __OGMRIP_CONFIGURABLE_H__

#include <ogmrip-profile.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_CONFIGURABLE            (ogmrip_configurable_get_type ())
#define OGMRIP_CONFIGURABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_CONFIGURABLE, OGMRipConfigurable))
#define OGMRIP_IS_CONFIGURABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_CONFIGURABLE))
#define OGMRIP_CONFIGURABLE_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMRIP_TYPE_CONFIGURABLE, OGMRipConfigurableInterface))

typedef struct _OGMRipConfigurable          OGMRipConfigurable;
typedef struct _OGMRipConfigurableInterface OGMRipConfigurableInterface;

struct _OGMRipConfigurableInterface
{
  GTypeInterface base_iface;

  void (* configure) (OGMRipConfigurable *configurable,
                      OGMRipProfile      *profile);
};


GType ogmrip_configurable_get_type  (void) G_GNUC_CONST;
void  ogmrip_configurable_configure (OGMRipConfigurable *configurable,
                                     OGMRipProfile      *profile);

G_END_DECLS

#endif /* __OGMRIP_CONFIGURABLE_H__ */

