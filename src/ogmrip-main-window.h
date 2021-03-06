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

#ifndef __OGMRIP_MAIN_WINDOW_H__
#define __OGMRIP_MAIN_WINDOW_H__

#include <ogmrip-encode-gtk.h>

#include "ogmrip-application.h"

G_BEGIN_DECLS

#define OGMRIP_TYPE_MAIN_WINDOW            (ogmrip_main_window_get_type ())
#define OGMRIP_MAIN_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MAIN_WINDOW, OGMRipMainWindow))
#define OGMRIP_MAIN_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_MAIN_WINDOW, OGMRipMainWindowClass))
#define OGMRIP_IS_MAIN_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_MAIN_WINDOW))
#define OGMRIP_IS_MAIN_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_MAIN_WINDOW))

typedef struct _OGMRipMainWindow      OGMRipMainWindow;
typedef struct _OGMRipMainWindowClass OGMRipMainWindowClass;
typedef struct _OGMRipMainWindowPriv  OGMRipMainWindowPrivate;

struct _OGMRipMainWindow
{
  GtkApplicationWindow parent_instance;

  OGMRipMainWindowPrivate *priv;
};

struct _OGMRipMainWindowClass
{
  GtkApplicationWindowClass parent_class;
};

GType       ogmrip_main_window_get_type  (void);
GtkWidget * ogmrip_main_window_new       (OGMRipApplication     *application,
                                          OGMRipEncodingManager *manager);
gboolean    ogmrip_main_window_load_path (OGMRipMainWindow      *window,
                                          const gchar           *path);
void        ogmrip_main_window_encode    (OGMRipMainWindow      *window,
                                          OGMRipEncoding        *encoding);

G_END_DECLS

#endif /* __OGMRIP_MAIN_WINDOW_H__ */

