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

#ifndef __OGMRIP_MEDIA_CHAPTERS_H__
#define __OGMRIP_MEDIA_CHAPTERS_H__

#include <ogmrip-media-types.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_CHAPTERS_STREAM            (ogmrip_chapters_stream_get_type ())
#define OGMRIP_CHAPTERS_STREAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_CHAPTERS_STREAM, OGMRipChaptersStream))
#define OGMRIP_IS_CHAPTERS_STREAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_CHAPTERS_STREAM))
#define OGMRIP_CHAPTERS_STREAM_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMRIP_TYPE_CHAPTERS_STREAM, OGMRipChaptersStreamInterface))

typedef struct _OGMRipChaptersStreamInterface OGMRipChaptersStreamInterface;

struct _OGMRipChaptersStreamInterface
{
  GTypeInterface base_iface;

  const gchar * (* get_label)    (OGMRipChaptersStream *chapters,
                                  guint                nr);
  gint          (* get_language) (OGMRipChaptersStream *chapters);
};

GType         ogmrip_chapters_stream_get_type     (void) G_GNUC_CONST;
const gchar * ogmrip_chapters_stream_get_label    (OGMRipChaptersStream *chapters,
                                                   guint                nr);
gint          ogmrip_chapters_stream_get_language (OGMRipChaptersStream *chapters);

G_END_DECLS

#endif /* __OGMRIP_MEDIA_CHAPTERS_H__ */

