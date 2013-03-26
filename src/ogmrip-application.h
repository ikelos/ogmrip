/* OGMRip - A media encoder for GNOME
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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

#include <ogmrip-encode-gtk.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_APPLICATION            (ogmrip_application_get_type ())
#define OGMRIP_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_APPLICATION, OGMRipApplication))
#define OGMRIP_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_APPLICATION, OGMRipApplicationClass))
#define OGMRIP_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_APPLICATION))
#define OGMRIP_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_APPLICATION))

typedef struct _OGMRipApplication      OGMRipApplication;
typedef struct _OGMRipApplicationClass OGMRipApplicationClass;
typedef struct _OGMRipApplicationPriv  OGMRipApplicationPriv;

struct _OGMRipApplication
{
  GApplication parent_instance;

  OGMRipApplicationPriv *priv;
};

struct _OGMRipApplicationClass
{
  GApplicationClass parent_class;
};

GType          ogmrip_application_get_type     (void);
GApplication * ogmrip_application_new          (const gchar       *app_id);
void           ogmrip_application_prepare      (OGMRipApplication *app);
gboolean       ogmrip_application_is_prepared  (OGMRipApplication *app);

G_END_DECLS

#endif /* __OGMRIP_APPLICATION_H__ */

