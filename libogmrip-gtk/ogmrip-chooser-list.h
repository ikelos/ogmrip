/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_CHOOSER_LIST_H__
#define __OGMRIP_CHOOSER_LIST_H__

#include <ogmdvd-gtk.h>
#include <ogmrip-source-chooser.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_CHOOSER_LIST            (ogmrip_chooser_list_get_type ())
#define OGMRIP_CHOOSER_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_CHOOSER_LIST, OGMRipChooserList))
#define OGMRIP_CHOOSER_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_CHOOSER_LIST, OGMRipChooserListClass))
#define OGMRIP_IS_CHOOSER_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_CHOOSER_LIST))
#define OGMRIP_IS_CHOOSER_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_CHOOSER_LIST))

typedef struct _OGMRipChooserList      OGMRipChooserList;
typedef struct _OGMRipChooserListClass OGMRipChooserListClass;
typedef struct _OGMRipChooserListPriv  OGMRipChooserListPriv;

struct _OGMRipChooserList
{
  GtkVBox widget;
  OGMRipChooserListPriv *priv;
};

struct _OGMRipChooserListClass
{
  GtkVBoxClass parent_class;

  void (* more_clicked) (OGMRipChooserList   *list,
                         OGMRipSourceChooser *chooser);
};

GType       ogmrip_chooser_list_get_type (void);
GtkWidget * ogmrip_chooser_list_new      (GType type);

void        ogmrip_chooser_list_set_max  (OGMRipChooserList *list,
                                          guint             max);
gint        ogmrip_chooser_list_get_max  (OGMRipChooserList *list);

void        ogmrip_chooser_list_clear    (OGMRipChooserList *list);
void        ogmrip_chooser_list_add      (OGMRipChooserList *list,
                                          GtkWidget         *chooser);
void        ogmrip_chooser_list_remove   (OGMRipChooserList *list,
                                          GtkWidget         *chooser);

void        ogmrip_chooser_list_foreach  (OGMRipChooserList *list,
                                          OGMRipSourceType  type,
                                          GFunc             func,
                                          gpointer          data);

GtkWidget * ogmrip_chooser_list_nth      (OGMRipChooserList *list,
                                          guint             n);
gint        ogmrip_chooser_list_length   (OGMRipChooserList *list);

G_END_DECLS

#endif /* __OGMRIP_CHOOSER_LIST_H__ */

