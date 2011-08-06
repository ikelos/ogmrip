/* OGMRip - A wrapper library around libdvdread
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

/**
 * SECTION:ogmrip-source-chooser
 * @title: OGMRipSourceChooser
 * @include: ogmrip-source-chooser.h
 * @short_description: Source chooser interface used by OGMRipSourceChooserWidget
 */

#include "ogmrip-source-chooser.h"

G_DEFINE_INTERFACE (OGMRipSourceChooser, ogmrip_source_chooser, GTK_TYPE_WIDGET);

static void
ogmrip_source_chooser_default_init (OGMRipSourceChooserInterface *iface)
{
  g_object_interface_install_property (iface,
      g_param_spec_pointer ("title", "Title property", "The DVD title",
        G_PARAM_READWRITE));
}

/**
 * ogmrip_source_chooser_set_title:
 * @chooser: An #OGMRipSourceChooser
 * @title: An #OGMDvdTitle
 *
 * Sets the #OGMDvdTitle from which to select the source.
 */
void
ogmrip_source_chooser_set_title (OGMRipSourceChooser *chooser, OGMDvdTitle *title)
{
  g_return_if_fail (OGMRIP_IS_SOURCE_CHOOSER (chooser));

  g_object_set (chooser, "title", title, NULL);
}

/**
 * ogmrip_source_chooser_get_title:
 * @chooser: An #OGMRipSourceChooser
 *
 * Returns the OGMDvdTitle which was passed to ogmrip_source_chooser_set_title().
 *
 * Returns: The current #OGMDvdTitle
 */
OGMDvdTitle *
ogmrip_source_chooser_get_title (OGMRipSourceChooser *chooser)
{
  OGMDvdTitle *title;

  g_return_val_if_fail (OGMRIP_IS_SOURCE_CHOOSER (chooser), NULL);

  g_object_get (chooser, "title", &title, NULL);

  return title;
}

/**
 * ogmrip_source_chooser_get_active:
 * @chooser: An #OGMRipSourceChooser
 * @type: A pointer to store the type of the chooser
 *
 * Returns the active source and its type.
 *
 * Returns: The active #OGMRipSource
 */
OGMRipSource *
ogmrip_source_chooser_get_active (OGMRipSourceChooser *chooser, OGMRipSourceType *type)
{
  g_return_val_if_fail (OGMRIP_IS_SOURCE_CHOOSER (chooser), NULL);

  if (OGMRIP_SOURCE_CHOOSER_GET_IFACE (chooser)->get_active)
    return OGMRIP_SOURCE_CHOOSER_GET_IFACE (chooser)->get_active (chooser, type);

  return NULL;
}

/**
 * ogmrip_source_chooser_select_language:
 * @chooser: An #OGMRipSourceChooser
 * @language: The language to select
 *
 * Select the first source entry of the chosen language.
 */
void
ogmrip_source_chooser_select_language (OGMRipSourceChooser *chooser, gint language)
{
  g_return_if_fail (OGMRIP_IS_SOURCE_CHOOSER (chooser));

  if (OGMRIP_SOURCE_CHOOSER_GET_IFACE (chooser)->select_language)
    OGMRIP_SOURCE_CHOOSER_GET_IFACE (chooser)->select_language (chooser, language);
}

