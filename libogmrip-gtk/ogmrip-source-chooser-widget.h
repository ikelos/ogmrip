/* OGMRip - A wrapper library around libdvdread
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_SOURCE_CHOOSER_WIDGET_H__
#define __OGMRIP_SOURCE_CHOOSER_WIDGET_H__

#include <ogmrip-source-chooser.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_AUDIO_CHOOSER_WIDGET            (ogmrip_audio_chooser_widget_get_type ())
#define OGMRIP_AUDIO_CHOOSER_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_AUDIO_CHOOSER_WIDGET, OGMRipSourceChooserWidget))
#define OGMRIP_AUDIO_CHOOSER_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_AUDIO_CHOOSER_WIDGET, OGMRipSourceChooserWidgetClass))
#define OGMRIP_IS_AUDIO_CHOOSER_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_AUDIO_CHOOSER_WIDGET))
#define OGMRIP_IS_AUDIO_CHOOSER_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_AUDIO_CHOOSER_WIDGET))

#define OGMRIP_TYPE_SUBP_CHOOSER_WIDGET             (ogmrip_subp_chooser_widget_get_type ())
#define OGMRIP_SUBP_CHOOSER_WIDGET(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_SUBP_CHOOSER_WIDGET, OGMRipSourceChooserWidget))
#define OGMRIP_SUBP_CHOOSER_WIDGET_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_SUBP_CHOOSER_WIDGET, OGMRipSourceChooserWidgetClass))
#define OGMRIP_IS_SUBP_CHOOSER_WIDGET(obj)          (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_SUBP_CHOOSER_WIDGET))
#define OGMRIP_IS_SUBP_CHOOSER_WIDGET_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_SUBP_CHOOSER_WIDGET))

typedef struct _OGMRipSourceChooserWidget      OGMRipSourceChooserWidget;
typedef struct _OGMRipSourceChooserWidgetClass OGMRipSourceChooserWidgetClass;
typedef struct _OGMRipSourceChooserWidgetPriv  OGMRipSourceChooserWidgetPriv;

struct _OGMRipSourceChooserWidget
{
  GtkComboBox parent_instance;
  OGMRipSourceChooserWidgetPriv *priv;
};

struct _OGMRipSourceChooserWidgetClass
{
  GtkComboBoxClass parent_class;
};

GType       ogmrip_audio_chooser_widget_get_type (void);
GtkWidget * ogmrip_audio_chooser_widget_new      (void);

GType       ogmrip_subp_chooser_widget_get_type  (void);
GtkWidget * ogmrip_subp_chooser_widget_new       (void);

G_END_DECLS

#endif /* __OGMRIP_SOURCE_CHOOSER_WIDGET_H__ */

