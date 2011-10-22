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

#ifndef __OGMRIP_MEDIA_CHOOSER_H__
#define __OGMRIP_MEDIA_CHOOSER_H__

#include <gtk/gtk.h>

#include <ogmrip-media.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_MEDIA_CHOOSER            (ogmrip_media_chooser_get_type ())
#define OGMRIP_MEDIA_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MEDIA_CHOOSER, OGMRipMediaChooser))
#define OGMRIP_IS_MEDIA_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_MEDIA_CHOOSER))
#define OGMRIP_MEDIA_CHOOSER_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMRIP_TYPE_MEDIA_CHOOSER, OGMRipMediaChooserInterface))

typedef struct _OGMRipMediaChooser          OGMRipMediaChooser;
typedef struct _OGMRipMediaChooserInterface OGMRipMediaChooserInterface;

struct _OGMRipMediaChooserInterface
{
  GTypeInterface base_iface;

  /*
   * Methods
   */
  OGMRipMedia * (*get_media) (OGMRipMediaChooser *chooser);

  /*
   * Signals
   */
  void (* media_changed) (OGMRipMediaChooser *chooser, 
                          OGMRipMedia        *media);
};


GType         ogmrip_media_chooser_get_type  (void) G_GNUC_CONST;
OGMRipMedia * ogmrip_media_chooser_get_media (OGMRipMediaChooser *chooser);

G_END_DECLS

#endif /* __OGMRIP_MEDIA_CHOOSER_H__ */
