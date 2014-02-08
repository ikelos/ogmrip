/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_PROFILE_CHOOSER_WIDGET_H__
#define __OGMRIP_PROFILE_CHOOSER_WIDGET_H__

#include <ogmrip-profile-store.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_PROFILE_CHOOSER_WIDGET            (ogmrip_profile_chooser_widget_get_type ())
#define OGMRIP_PROFILE_CHOOSER_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_PROFILE_CHOOSER_WIDGET, OGMRipProfileChooserWidget))
#define OGMRIP_PROFILE_CHOOSER_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_PROFILE_CHOOSER_WIDGET, OGMRipProfileChooserWidgetClass))
#define OGMRIP_IS_PROFILE_CHOOSER_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_PROFILE_CHOOSER_WIDGET))
#define OGMRIP_IS_PROFILE_CHOOSER_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_PROFILE_CHOOSER_WIDGET))

typedef struct _OGMRipProfileChooserWidget      OGMRipProfileChooserWidget;
typedef struct _OGMRipProfileChooserWidgetClass OGMRipProfileChooserWidgetClass;
typedef struct _OGMRipProfileChooserWidgetPriv  OGMRipProfileChooserWidgetPrivate;

struct _OGMRipProfileChooserWidget
{
  GtkComboBox parent_instance;

  OGMRipProfileChooserWidgetPrivate *priv;
};

struct _OGMRipProfileChooserWidgetClass
{
  GtkComboBoxClass parent_class;
};

GType           ogmrip_profile_chooser_widget_get_type   (void);
GtkWidget *     ogmrip_profile_chooser_widget_new        (void);
OGMRipProfile * ogmrip_profile_chooser_widget_get_active (OGMRipProfileChooserWidget *chooser);
void            ogmrip_profile_chooser_widget_set_active (OGMRipProfileChooserWidget *chooser,
                                                          OGMRipProfile              *profile);

G_END_DECLS

#endif /* __OGMRIP_PROFILE_CHOOSER_WIDGET_H__ */

