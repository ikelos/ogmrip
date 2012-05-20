/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2012 Olivier Rolland <billl@users.sourceforge.net>
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
 * SECTION:ogmrip-title-chooser
 * @title: OGMRipTitleChooser
 * @include: ogmrip-title-chooser.h
 * @short_description: DVD title chooser interface used by OGMRipTitleChooserWidget
 */

#include "ogmrip-title-chooser.h"

G_DEFINE_INTERFACE (OGMRipTitleChooser, ogmrip_title_chooser, GTK_TYPE_WIDGET);

static void
ogmrip_title_chooser_default_init (OGMRipTitleChooserInterface *iface)
{
  g_object_interface_install_property (iface,
      g_param_spec_object ("media", "Media property", "The media",
        OGMRIP_TYPE_MEDIA, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

/**
 * ogmrip_title_chooser_set_media:
 * @chooser: An #OGMRipTitleChooser
 * @media: An #OGMRipMedia
 *
 * Sets the #OGMRipMedia to select the title from.
 */
void
ogmrip_title_chooser_set_media (OGMRipTitleChooser *chooser, OGMRipMedia *media)
{
  g_return_if_fail (OGMRIP_IS_TITLE_CHOOSER (chooser));

  g_object_set (chooser, "media", media, NULL);
}

/**
 * ogmrip_title_chooser_get_disc:
 * @chooser: An #OGMRipTitleChooser
 *
 * Returns the #OGMRipMedia which was passed to ogmrip_title_chooser_set_media().
 *
 * Returns: The current #OGMRipMedia
 */
OGMRipMedia *
ogmrip_title_chooser_get_disc (OGMRipTitleChooser *chooser)
{
  OGMRipMedia *media;

  g_return_val_if_fail (OGMRIP_IS_TITLE_CHOOSER (chooser), NULL);

  g_object_get (chooser, "media", &media, NULL);

  if (media)
    g_object_unref (media);

  return media;
}

/**
 * ogmrip_title_chooser_set_active:
 * @chooser: An #OGMRipTitleChooser
 * @title: An #OGMRipTitle
 *
 * Sets the active #OGMRipTitle.
 */
void
ogmrip_title_chooser_set_active (OGMRipTitleChooser *chooser, OGMRipTitle *title)
{
  g_return_if_fail (OGMRIP_IS_TITLE_CHOOSER (chooser));
  g_return_if_fail (OGMRIP_IS_TITLE (title));

  OGMRIP_TITLE_CHOOSER_GET_IFACE (chooser)->set_active (chooser, title);
}

/**
 * ogmrip_title_chooser_get_active:
 * @chooser: An #OGMRipTitleChooser
 *
 * Returns the active #OGMRipTitle.
 *
 * Returns: The active #OGMRipTitle
 */
OGMRipTitle *
ogmrip_title_chooser_get_active (OGMRipTitleChooser *chooser)
{
  g_return_val_if_fail (OGMRIP_IS_TITLE_CHOOSER (chooser), NULL);

  return OGMRIP_TITLE_CHOOSER_GET_IFACE (chooser)->get_active (chooser);
}

