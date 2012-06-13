/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.typeforge.net>
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

#ifndef __OGMRIP_TYPE_CHOOSER_WIDGET_H__
#define __OGMRIP_TYPE_CHOOSER_WIDGET_H__

#include <ogmrip-type-store.h>

G_BEGIN_DECLS

void  ogmrip_type_chooser_widget_construct  (GtkComboBox *chooser,
                                             GType       gtype);
GType ogmrip_type_chooser_widget_get_active (GtkComboBox *chooser);
void  ogmrip_type_chooser_widget_set_active (GtkComboBox *chooser,
                                             GType       gtype);
void  ogmrip_type_chooser_widget_set_filter (GtkComboBox *chooser,
                                             GType       gtype);

G_END_DECLS

#endif /* __OGMRIP_TYPE_CHOOSER_WIDGET_H__ */

