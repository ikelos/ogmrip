/* OGMRip - A library for media ripping and encoding
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

#ifndef __OGMRIP_SUBP_CHOOSER_WIDGET_H__
#define __OGMRIP_SUBP_CHOOSER_WIDGET_H__

#include <ogmrip-file-gtk.h>
#include <ogmrip-list-item.h>
#include <ogmrip-subp-options.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_SUBP_CHOOSER_WIDGET            (ogmrip_subp_chooser_widget_get_type ())
#define OGMRIP_SUBP_CHOOSER_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_SUBP_CHOOSER_WIDGET, OGMRipSubpChooserWidget))
#define OGMRIP_SUBP_CHOOSER_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_SUBP_CHOOSER_WIDGET, OGMRipSubpChooserWidgetClass))
#define OGMRIP_IS_SUBP_CHOOSER_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_SUBP_CHOOSER_WIDGET))
#define OGMRIP_IS_SUBP_CHOOSER_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_SUBP_CHOOSER_WIDGET))

typedef struct _OGMRipSubpChooserWidget      OGMRipSubpChooserWidget;
typedef struct _OGMRipSubpChooserWidgetClass OGMRipSubpChooserWidgetClass;
typedef struct _OGMRipSubpChooserWidgetPriv  OGMRipSubpChooserWidgetPrivate;

struct _OGMRipSubpChooserWidget
{
  OGMRipListItem parent_instance;

  OGMRipSubpChooserWidgetPrivate *priv;
};

struct _OGMRipSubpChooserWidgetClass
{
  OGMRipListItemClass parent_class;
};

GType               ogmrip_subp_chooser_widget_get_type        (void);
GtkWidget *         ogmrip_subp_chooser_widget_new             (void);
OGMRipSubpOptions * ogmrip_subp_chooser_widget_get_options     (OGMRipSubpChooserWidget *chooser);
const gchar *       ogmrip_subp_chooser_widget_get_label       (OGMRipSubpChooserWidget *chooser);
gint                ogmrip_subp_chooser_widget_get_language    (OGMRipSubpChooserWidget *chooser);
gboolean            ogmrip_subp_chooser_widget_get_forced_only (OGMRipSubpChooserWidget *chooser);

G_END_DECLS

#endif /* __OGMRIP_SUBP_CHOOSER_WIDGET_H__ */

