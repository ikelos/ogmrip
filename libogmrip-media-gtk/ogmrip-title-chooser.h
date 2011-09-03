/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_TITLE_CHOOSER_H__
#define __OGMRIP_TITLE_CHOOSER_H__

#include <gtk/gtk.h>

#include <ogmrip-media.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_TITLE_CHOOSER            (ogmrip_title_chooser_get_type ())
#define OGMRIP_TITLE_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_TITLE_CHOOSER, OGMRipTitleChooser))
#define OGMRIP_IS_TITLE_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_TITLE_CHOOSER))
#define OGMRIP_TITLE_CHOOSER_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMRIP_TYPE_TITLE_CHOOSER, OGMRipTitleChooserInterface))

typedef struct _OGMRipTitleChooser          OGMRipTitleChooser;
typedef struct _OGMRipTitleChooserInterface OGMRipTitleChooserInterface;

struct _OGMRipTitleChooserInterface
{
  GTypeInterface base_iface;

  void          (* set_active) (OGMRipTitleChooser *chooser,
                                OGMRipTitle        *title);
  OGMRipTitle * (* get_active) (OGMRipTitleChooser *chooser);
};


GType         ogmrip_title_chooser_get_type   (void) G_GNUC_CONST;

void          ogmrip_title_chooser_set_media  (OGMRipTitleChooser *chooser,
                                               OGMRipMedia        *media);
OGMRipMedia * ogmrip_title_chooser_get_media  (OGMRipTitleChooser *chooser);
void          ogmrip_title_chooser_set_active (OGMRipTitleChooser *chooser,
                                               OGMRipTitle        *title);
OGMRipTitle * ogmrip_title_chooser_get_active (OGMRipTitleChooser *chooser);

G_END_DECLS

#endif /* __OGMRIP_TITLE_CHOOSER_H__ */

