/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/**
 * SECTION:ogmrip-media-chooser
 * @title: OGMRipMediaChooser
 * @include: ogmrip-media-chooser.h
 * @short_description: Media chooser interface used by OGMRipMediaChooserDialog
 *                     and OGMRipMediaChooserWidget
 */

#include "ogmrip-media-chooser.h"

G_DEFINE_INTERFACE (OGMRipMediaChooser, ogmrip_media_chooser, GTK_TYPE_WIDGET);

static void
ogmrip_media_chooser_default_init (OGMRipMediaChooserInterface *iface)
{
  GType iface_type = G_TYPE_FROM_INTERFACE (iface);

  /**
   * OGMRipMediaChooser::changed:
   * @chooser: the widget that received the signal
   * @media: the media
   *
   * Emitted each time a media is selected.
   */
  g_signal_new ("media-changed", iface_type,
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipMediaChooserInterface, media_changed), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, OGMRIP_TYPE_MEDIA);
}

/**
 * ogmrip_media_chooser_get_media:
 * @chooser: An #OGMRipMediaChooser
 *
 * Returns the selected media.
 *
 * Returns: An #OGMRipMedia or NULL
 */
OGMRipMedia *
ogmrip_media_chooser_get_media (OGMRipMediaChooser *chooser)
{
  g_return_val_if_fail (OGMRIP_IS_MEDIA_CHOOSER (chooser), NULL);

  return OGMRIP_MEDIA_CHOOSER_GET_IFACE (chooser)->get_media (chooser);
}

