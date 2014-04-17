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

#ifndef __OGMRIP_APPLICATION_H__
#define __OGMRIP_APPLICATION_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_APPLICATION            (ogmrip_application_get_type ())
#define OGMRIP_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_APPLICATION, OGMRipApplication))
#define OGMRIP_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_APPLICATION))
#define OGMRIP_APPLICATION_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMRIP_TYPE_APPLICATION, OGMRipApplicationInterface))

typedef struct _OGMRipApplication          OGMRipApplication;
typedef struct _OGMRipApplicationInterface OGMRipApplicationInterface;

struct _OGMRipApplicationInterface
{
  GTypeInterface base_iface;

  /* signals */
  void (* prepare) (OGMRipApplication *app);

  /* funcs */
  gboolean (* get_is_prepared) (OGMRipApplication *app);
};

GType    ogmrip_application_get_type        (void);
void     ogmrip_application_prepare         (OGMRipApplication *app);
gboolean ogmrip_application_get_is_prepared (OGMRipApplication *app);

G_END_DECLS

#endif /* __OGMRIP_APPLICATION_H__ */

