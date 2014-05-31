/* OGMRip - A library for media ripping and encoding
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

#ifndef __OGMRIP_OPTIONS_EDITABLE_H__
#define __OGMRIP_OPTIONS_EDITABLE_H__

#include <ogmrip-encode.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_OPTIONS_EDITABLE            (ogmrip_options_editable_get_type ())
#define OGMRIP_OPTIONS_EDITABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_OPTIONS_EDITABLE, OGMRipOptionsEditable))
#define OGMRIP_IS_OPTIONS_EDITABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_OPTIONS_EDITABLE))
#define OGMRIP_OPTIONS_EDITABLE_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMRIP_TYPE_OPTIONS_EDITABLE, OGMRipOptionsEditableInterface))

typedef struct _OGMRipOptionsEditable          OGMRipOptionsEditable;
typedef struct _OGMRipOptionsEditableInterface OGMRipOptionsEditableInterface;

struct _OGMRipOptionsEditableInterface
{
  GTypeInterface base_iface;
};

GType           ogmrip_options_editable_get_type    (void);
OGMRipProfile * ogmrip_options_editable_get_profile (OGMRipOptionsEditable *editable);
void            ogmrip_options_editable_set_profile (OGMRipOptionsEditable *editable,
                                                     OGMRipProfile         *profile);

G_END_DECLS

#endif /* __OGMRIP_OPTIONS_EDITABLE_H__ */

