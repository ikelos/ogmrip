/* OGMRipMedia - A media library for OGMRip
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

#ifndef __OGMRIP_MEDIA_CHOOSER_WIDGET_H__
#define __OGMRIP_MEDIA_CHOOSER_WIDGET_H__

#include <ogmrip-media-chooser.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_MEDIA_CHOOSER_WIDGET            (ogmrip_media_chooser_widget_get_type ())
#define OGMRIP_MEDIA_CHOOSER_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MEDIA_CHOOSER_WIDGET, OGMRipMediaChooserWidget))
#define OGMRIP_MEDIA_CHOOSER_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_MEDIA_CHOOSER_WIDGET, OGMRipMediaChooserWidgetClass))
#define OGMRIP_IS_MEDIA_CHOOSER_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_MEDIA_CHOOSER_WIDGET))
#define OGMRIP_IS_MEDIA_CHOOSER_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_MEDIA_CHOOSER_WIDGET))

typedef struct _OGMRipMediaChooserWidget      OGMRipMediaChooserWidget;
typedef struct _OGMRipMediaChooserWidgetClass OGMRipMediaChooserWidgetClass;
typedef struct _OGMRipMediaChooserWidgetPriv  OGMRipMediaChooserWidgetPriv;

struct _OGMRipMediaChooserWidget
{
  GtkComboBox parent_instance;
  OGMRipMediaChooserWidgetPriv *priv;
};

struct _OGMRipMediaChooserWidgetClass
{
  GtkComboBoxClass parent_class;
};

GType       ogmrip_media_chooser_widget_get_type   (void);
GtkWidget * ogmrip_media_chooser_widget_new        (void);
GVolume *   ogmrip_media_chooser_widget_get_volume (OGMRipMediaChooserWidget *chooser);

G_END_DECLS

#endif /* __OGMRIP_MEDIA_CHOOSER_WIDGET_H__ */

