/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.audioforge.net>
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

#ifndef __OGMRIP_AUDIO_CHOOSER_WIDGET_H__
#define __OGMRIP_AUDIO_CHOOSER_WIDGET_H__

#include <ogmrip-file-gtk.h>
#include <ogmrip-list-item.h>
#include <ogmrip-audio-options.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_AUDIO_CHOOSER_WIDGET            (ogmrip_audio_chooser_widget_get_type ())
#define OGMRIP_AUDIO_CHOOSER_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_AUDIO_CHOOSER_WIDGET, OGMRipAudioChooserWidget))
#define OGMRIP_AUDIO_CHOOSER_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_AUDIO_CHOOSER_WIDGET, OGMRipAudioChooserWidgetClass))
#define OGMRIP_IS_AUDIO_CHOOSER_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_AUDIO_CHOOSER_WIDGET))
#define OGMRIP_IS_AUDIO_CHOOSER_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_AUDIO_CHOOSER_WIDGET))

typedef struct _OGMRipAudioChooserWidget      OGMRipAudioChooserWidget;
typedef struct _OGMRipAudioChooserWidgetClass OGMRipAudioChooserWidgetClass;
typedef struct _OGMRipAudioChooserWidgetPriv  OGMRipAudioChooserWidgetPrivate;

struct _OGMRipAudioChooserWidget
{
  OGMRipListItem parent_instance;

  OGMRipAudioChooserWidgetPrivate *priv;
};

struct _OGMRipAudioChooserWidgetClass
{
  OGMRipListItemClass parent_class;
};

GType                ogmrip_audio_chooser_widget_get_type     (void);
GtkWidget *          ogmrip_audio_chooser_widget_new          (void);
OGMRipAudioOptions * ogmrip_audio_chooser_widget_get_options  (OGMRipAudioChooserWidget *chooser);
const gchar *        ogmrip_audio_chooser_widget_get_label    (OGMRipAudioChooserWidget *chooser);
gint                 ogmrip_audio_chooser_widget_get_language (OGMRipAudioChooserWidget *chooser);

G_END_DECLS

#endif /* __OGMRIP_AUDIO_CHOOSER_WIDGET_H__ */

