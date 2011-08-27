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
 * SECTION:ogmdvd-stream
 * @title: OGMDvdStream
 * @include: ogmdvd-stream.h
 * @short_description: Structure describing a stream
 */

#include "ogmdvd-disc.h"
#include "ogmdvd-stream.h"
#include "ogmdvd-title.h"
#include "ogmdvd-priv.h"

/**
 * ogmdvd_stream_ref:
 * @stream: An #OGMDvdStream
 *
 * Increments the reference count of an #OGMDvdStream.
 */
void
ogmdvd_stream_ref (OGMDvdStream *stream)
{
  g_return_if_fail (stream != NULL);

  ogmdvd_disc_ref (stream->title->disc);
}

/**
 * ogmdvd_stream_unref:
 * @stream: An #OGMDvdStream
 *
 * Decrements the reference count of an #OGMDvdStream.
 */
void
ogmdvd_stream_unref (OGMDvdStream *stream)
{
  g_return_if_fail (stream != NULL);

  ogmdvd_disc_unref (stream->title->disc);
}

/**
 * ogmdvd_stream_get_title:
 * @stream: An #OGMDvdStream
 *
 * Returns the title the #OGMDvdStream belongs to
 *
 * Returns: An #OGMDvdTitle, or NULL
 */
OGMDvdTitle *
ogmdvd_stream_get_title (OGMDvdStream *stream)
{
  g_return_val_if_fail (stream != NULL, NULL);

  return stream->title;
}

/**
 * ogmdvd_stream_get_nr:
 * @stream: An #OGMDvdStream
 *
 * Returns the stream number.
 *
 * Returns: The stream number, or -1
 */
gint
ogmdvd_stream_get_nr (OGMDvdStream *stream)
{
  g_return_val_if_fail (stream != NULL, -1);

  return stream->nr;
}

/**
 * ogmdvd_stream_get_id:
 * @stream: An #OGMDvdStream
 *
 * Returns the stream identifier.
 *
 * Returns: The stream identifier, or -1
 */
gint
ogmdvd_stream_get_id (OGMDvdStream *stream)
{
  g_return_val_if_fail (stream != NULL, -1);

  return stream->id;
}

