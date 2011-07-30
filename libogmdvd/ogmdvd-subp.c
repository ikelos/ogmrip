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
 * SECTION:ogmdvd-subp
 * @title: OGMDvdSubp
 * @include: ogmdvd-subp.h
 * @short_description: Structure describing a subtitles stream
 */

#include "ogmdvd-subp.h"
#include "ogmdvd-priv.h"

/**
 * ogmdvd_subp_stream_get_content:
 * @subp: An #OGMDvdSubpStream
 *
 * Returns the content of the subtitles stream.
 *
 * Returns: #OGMDvdSubpContent, or -1
 */
gint
ogmdvd_subp_stream_get_content (OGMDvdSubpStream *subp)
{
  g_return_val_if_fail (subp != NULL, -1);

  return subp->lang_extension;
}

/**
 * ogmdvd_subp_stream_get_language:
 * @subp: An #OGMDvdSubpStream
 *
 * Returns the language of the subtitles stream.
 *
 * Returns: The language code, or -1
 */
gint
ogmdvd_subp_stream_get_language (OGMDvdSubpStream *subp)
{
  g_return_val_if_fail (subp != NULL, -1);

  return subp->lang_code;
}

/**
 * ogmdvd_subp_stream_get_name:
 * @subp: An #OGMDvdSubpStream
 *
 * Returns the name of the subp stream.
 *
 * Returns: the name, or NULL
 */
const gchar *
ogmdvd_subp_stream_get_name (OGMDvdSubpStream *subp)
{
  g_return_val_if_fail (subp != NULL, NULL);

  return NULL;
}

