/* OGMDvd - A wrapper library around libdvdread
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

#ifndef __OGMDVD_TITLE_CHOOSER_H__
#define __OGMDVD_TITLE_CHOOSER_H__

#include <gtk/gtk.h>

#include <ogmdvd.h>

G_BEGIN_DECLS

#define OGMDVD_TYPE_TITLE_CHOOSER            (ogmdvd_title_chooser_get_type ())
#define OGMDVD_TITLE_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMDVD_TYPE_TITLE_CHOOSER, OGMDvdTitleChooser))
#define OGMDVD_IS_TITLE_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMDVD_TYPE_TITLE_CHOOSER))
#define OGMDVD_TITLE_CHOOSER_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMDVD_TYPE_TITLE_CHOOSER, OGMDvdTitleChooserIface))

typedef struct _OGMDvdTitleChooser      OGMDvdTitleChooser;
typedef struct _OGMDvdTitleChooserIface OGMDvdTitleChooserIface;

struct _OGMDvdTitleChooserIface
{
  GTypeInterface base_iface;

  /*
   * Methods
   */

  void          (* set_disc)   (OGMDvdTitleChooser *chooser,
                                OGMDvdDisc         *disc);
  OGMDvdDisc *  (* get_disc)   (OGMDvdTitleChooser *chooser);
  OGMDvdTitle * (* get_active) (OGMDvdTitleChooser *chooser);
};


GType         ogmdvd_title_chooser_get_type   (void) G_GNUC_CONST;

void          ogmdvd_title_chooser_set_disc   (OGMDvdTitleChooser *chooser,
                                               OGMDvdDisc         *disc);
OGMDvdDisc *  ogmdvd_title_chooser_get_disc   (OGMDvdTitleChooser *chooser);
OGMDvdTitle * ogmdvd_title_chooser_get_active (OGMDvdTitleChooser *chooser);

G_END_DECLS

#endif /* __OGMDVD_TITLE_CHOOSER_H__ */

