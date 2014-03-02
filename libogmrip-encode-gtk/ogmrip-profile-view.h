/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2010-2013 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_PROFILE_VIEW_H__
#define __OGMRIP_PROFILE_VIEW_H__

#include <ogmrip-profile-store.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_PROFILE_VIEW            (ogmrip_profile_view_get_type ())
#define OGMRIP_PROFILE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_PROFILE_VIEW, OGMRipProfileView))
#define OGMRIP_PROFILE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_PROFILE_VIEW, OGMRipProfileViewClass))
#define OGMRIP_IS_PROFILE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_PROFILE_VIEW))
#define OGMRIP_IS_PROFILE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_PROFILE_VIEW))

typedef struct _OGMRipProfileView      OGMRipProfileView;
typedef struct _OGMRipProfileViewClass OGMRipProfileViewClass;
typedef struct _OGMRipProfileViewPriv  OGMRipProfileViewPrivate;

struct _OGMRipProfileView
{
  GtkTreeView parent_instance;

  OGMRipProfileViewPrivate *priv;
};

struct _OGMRipProfileViewClass
{
  GtkTreeViewClass parent_class;
};

GType       ogmrip_profile_view_get_type (void);
GtkWidget * ogmrip_profile_view_new      (void);

G_END_DECLS

#endif /* __OGMRIP_PROFILE_VIEW_H__ */

