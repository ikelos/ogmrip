/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_HELPER_H__
#define __OGMRIP_HELPER_H__

#include <ogmrip-media.h>
#include <ogmrip-encode.h>

#include <gtk/gtk.h>
#include <sys/types.h>

G_BEGIN_DECLS

#define gtk_builder_get_widget(builder, name) \
    (GtkWidget *) gtk_builder_get_object ((builder), (name))

gboolean ogmrip_open_title (GtkWindow   *parent,
                            OGMRipTitle *title);

G_END_DECLS

#endif /* __OGMRIP_HELPER_H__ */

