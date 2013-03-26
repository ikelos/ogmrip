/* OGMRipMedia - A media library for OGMRip
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

#ifndef __OGMRIP_MEDIA_BACKEND_H__
#define __OGMRIP_MEDIA_BACKEND_H__

#include <ogmrip-media-types.h>

G_BEGIN_DECLS

gboolean ogmrip_title_benchmark   (OGMRipTitle         *title,
                                   gboolean            *progressive,
                                   gboolean            *telecine,
                                   gboolean            *interlaced,
                                   GCancellable        *cancellable,
                                   OGMRipTitleCallback callback,
                                   gpointer            user_data,
                                   GError              **error);
gboolean ogmrip_title_crop_detect (OGMRipTitle         *title,
                                   guint               *crop_x,
                                   guint               *crop_y,
                                   guint               *crop_w,
                                   guint               *crop_h,
                                   GCancellable        *cancellable,
                                   OGMRipTitleCallback callback,
                                   gpointer            user_data,
                                   GError              **error);
GFile *  ogmrip_title_grab_frame  (OGMRipTitle         *title,
                                   gulong              frame,
                                   GCancellable        *cancellable,
                                   GError              **error);

G_END_DECLS

#endif /* __OGMRIP_MEDIA_BACKEND_H__ */

