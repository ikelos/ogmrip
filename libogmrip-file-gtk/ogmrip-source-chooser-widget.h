/* OGMRipFile - A file library for OGMRip
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#include <ogmrip-media-gtk.h>
#include <ogmrip-file-chooser-dialog.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_SOURCE_CHOOSER_WIDGET            (ogmrip_source_chooser_widget_get_type ())
#define OGMRIP_SOURCE_CHOOSER_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_SOURCE_CHOOSER_WIDGET, OGMRipSourceChooserWidget))
#define OGMRIP_SOURCE_CHOOSER_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_SOURCE_CHOOSER_WIDGET, OGMRipSourceChooserWidgetClass))
#define OGMRIP_IS_SOURCE_CHOOSER_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_SOURCE_CHOOSER_WIDGET))
#define OGMRIP_IS_SOURCE_CHOOSER_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_SOURCE_CHOOSER_WIDGET))

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

GType       ogmrip_source_chooser_widget_get_type         (void);
GtkWidget * ogmrip_source_chooser_widget_new              (void);
GtkWidget * ogmrip_source_chooser_widget_new_with_dialog  (OGMRipFileChooserDialog   *dialog);
void        ogmrip_source_chooser_widget_add_audio_stream (OGMRipSourceChooserWidget *chooser,
                                                           OGMRipAudioStream         *stream);
void        ogmrip_source_chooser_widget_add_subp_stream  (OGMRipSourceChooserWidget *chooser,
                                                           OGMRipSubpStream          *stream);

G_END_DECLS

#endif /* __OGMRIP_SOURCE_CHOOSER_WIDGET_H__ */

