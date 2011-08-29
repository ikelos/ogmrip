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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/**
 * SECTION:ogmdvd-title-chooser
 * @title: OGMDvdTitleChooser
 * @include: ogmdvd-title-chooser.h
 * @short_description: DVD title chooser interface used by OGMDvdTitleChooserWidget
 */

#include "ogmdvd-title-chooser.h"

G_DEFINE_INTERFACE (OGMDvdTitleChooser, ogmdvd_title_chooser, GTK_TYPE_WIDGET);

static void
ogmdvd_title_chooser_default_init (OGMDvdTitleChooserInterface *iface)
{
  g_object_interface_install_property (iface,
      g_param_spec_object ("media", "Media property", "The media",
        OGMRIP_TYPE_MEDIA, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

/**
 * ogmdvd_title_chooser_set_disc:
 * @chooser: An #OGMDvdTitleChooser
 * @media: An #OGMRipMedia
 *
 * Sets the #OGMRipMedia to select the title from.
 */
void
ogmdvd_title_chooser_set_disc (OGMDvdTitleChooser *chooser, OGMRipMedia *media)
{
  g_return_if_fail (OGMDVD_IS_TITLE_CHOOSER (chooser));

  g_object_set (chooser, "media", media, NULL);
}

/**
 * ogmdvd_title_chooser_get_disc:
 * @chooser: An #OGMDvdTitleChooser
 *
 * Returns the #OGMRipMedia which was passed to ogmdvd_title_chooser_set_disc().
 *
 * Returns: The current #OGMRipMedia
 */
OGMRipMedia *
ogmdvd_title_chooser_get_disc (OGMDvdTitleChooser *chooser)
{
  OGMRipMedia *media;

  g_return_val_if_fail (OGMDVD_IS_TITLE_CHOOSER (chooser), NULL);

  g_object_get (chooser, "media", &media, NULL);

  if (media)
    g_object_unref (media);

  return media;
}

/**
 * ogmdvd_title_chooser_set_active:
 * @chooser: An #OGMDvdTitleChooser
 * @title: An #OGMRipTitle
 *
 * Sets the active #OGMRipTitle.
 */
void
ogmdvd_title_chooser_set_active (OGMDvdTitleChooser *chooser, OGMRipTitle *title)
{
  g_return_if_fail (OGMDVD_IS_TITLE_CHOOSER (chooser));

  OGMDVD_TITLE_CHOOSER_GET_IFACE (chooser)->set_active (chooser, title);
}

/**
 * ogmdvd_title_chooser_get_active:
 * @chooser: An #OGMDvdTitleChooser
 *
 * Returns the active #OGMRipTitle.
 *
 * Returns: The active #OGMRipTitle
 */
OGMRipTitle *
ogmdvd_title_chooser_get_active (OGMDvdTitleChooser *chooser)
{
  g_return_val_if_fail (OGMDVD_IS_TITLE_CHOOSER (chooser), NULL);

  return OGMDVD_TITLE_CHOOSER_GET_IFACE (chooser)->get_active (chooser);
}

