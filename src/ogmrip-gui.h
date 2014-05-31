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

#ifndef __OGMRIP_GUI_H__
#define __OGMRIP_GUI_H__

#include <ogmrip-encode-gtk.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_GUI            (ogmrip_gui_get_type ())
#define OGMRIP_GUI(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_GUI, OGMRipGui))
#define OGMRIP_GUI_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_GUI, OGMRipGuiClass))
#define OGMRIP_IS_GUI(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_GUI))
#define OGMRIP_IS_GUI_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_GUI))

typedef struct _OGMRipGui      OGMRipGui;
typedef struct _OGMRipGuiClass OGMRipGuiClass;
typedef struct _OGMRipGuiPriv  OGMRipGuiPrivate;

struct _OGMRipGui
{
  GtkApplication parent_instance;

  OGMRipGuiPrivate *priv;
};

struct _OGMRipGuiClass
{
  GtkApplicationClass parent_class;

  void (* prepare) (OGMRipGui *gui);
};

GType          ogmrip_gui_get_type (void);
GApplication * ogmrip_gui_new      (const gchar *app_id);

G_END_DECLS

#endif /* __OGMRIP_GUI_H__ */

