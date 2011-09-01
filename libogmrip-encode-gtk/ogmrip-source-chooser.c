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
      g_param_spec_object ("title", "Title property", "The DVD title",
        OGMRIP_TYPE_TITLE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

/**
 * ogmrip_source_chooser_set_title:
 * @chooser: An #OGMRipSourceChooser
 * @title: An #OGMRipTitle
 *
 * Sets the #OGMRipTitle from which to select the source.
 */
void
ogmrip_source_chooser_set_title (OGMRipSourceChooser *chooser, OGMRipTitle *title)
{
  g_return_if_fail (OGMRIP_IS_SOURCE_CHOOSER (chooser));

  g_object_set (chooser, "title", title, NULL);
}

/**
 * ogmrip_source_chooser_get_title:
 * @chooser: An #OGMRipSourceChooser
 *
 * Returns the OGMRipTitle which was passed to ogmrip_source_chooser_set_title().
 *
 * Returns: The current #OGMRipTitle
 */
OGMRipTitle *
ogmrip_source_chooser_get_title (OGMRipSourceChooser *chooser)
{
  OGMRipTitle *title;

  g_return_val_if_fail (OGMRIP_IS_SOURCE_CHOOSER (chooser), NULL);

  g_object_get (chooser, "title", &title, NULL);

  if (title)
    g_object_unref (title);

  return title;
}

/**
 * ogmrip_source_chooser_get_active:
 * @chooser: An #OGMRipSourceChooser
 *
 * Returns the active stream.
 *
 * Returns: The active #OGMRipStream
 */
OGMRipStream *
ogmrip_source_chooser_get_active (OGMRipSourceChooser *chooser)
{
  g_return_val_if_fail (OGMRIP_IS_SOURCE_CHOOSER (chooser), NULL);

  if (OGMRIP_SOURCE_CHOOSER_GET_IFACE (chooser)->get_active)
    return OGMRIP_SOURCE_CHOOSER_GET_IFACE (chooser)->get_active (chooser);

  return NULL;
}

/**
 * ogmrip_source_chooser_set_active:
 * @chooser: An #OGMRipSourceChooser
 * @stream: An #OGMRipStream
 *
 * Set the current active stream.
 */
void
ogmrip_source_chooser_set_active (OGMRipSourceChooser *chooser, OGMRipStream *stream)
{
  g_return_if_fail (OGMRIP_IS_SOURCE_CHOOSER (chooser));

  if (OGMRIP_SOURCE_CHOOSER_GET_IFACE (chooser)->set_active)
    OGMRIP_SOURCE_CHOOSER_GET_IFACE (chooser)->set_active (chooser, stream);
}

/**
 * ogmrip_source_chooser_select_language:
 * @chooser: An #OGMRipSourceChooser
 * @language: The language to select
 *
 * Select the first stream entry of the chosen language.
 */
void
ogmrip_source_chooser_select_language (OGMRipSourceChooser *chooser, gint language)
{
  g_return_if_fail (OGMRIP_IS_SOURCE_CHOOSER (chooser));

  if (OGMRIP_SOURCE_CHOOSER_GET_IFACE (chooser)->select_language)
    OGMRIP_SOURCE_CHOOSER_GET_IFACE (chooser)->select_language (chooser, language);
}

