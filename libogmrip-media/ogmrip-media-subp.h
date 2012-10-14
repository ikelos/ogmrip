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

#ifndef __OGMRIP_MEDIA_SUBP_H__
#define __OGMRIP_MEDIA_SUBP_H__

#include <ogmrip-media-types.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_SUBP_STREAM            (ogmrip_subp_stream_get_type ())
#define OGMRIP_SUBP_STREAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_SUBP_STREAM, OGMRipSubpStream))
#define OGMRIP_IS_SUBP_STREAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_SUBP_STREAM))
#define OGMRIP_SUBP_STREAM_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMRIP_TYPE_SUBP_STREAM, OGMRipSubpStreamInterface))

typedef struct _OGMRipSubpStreamInterface OGMRipSubpStreamInterface;

struct _OGMRipSubpStreamInterface
{
  GTypeInterface base_iface;

  gint          (* get_charset)  (OGMRipSubpStream *subp);
  gint          (* get_content)  (OGMRipSubpStream *subp);
  const gchar * (* get_label)    (OGMRipSubpStream *subp);
  gint          (* get_language) (OGMRipSubpStream *subp);
  gint          (* get_newline)  (OGMRipSubpStream *subp);
  gint          (* get_nr)       (OGMRipSubpStream *subp);
};

GType         ogmrip_subp_stream_get_type     (void) G_GNUC_CONST;
gint          ogmrip_subp_stream_get_charset  (OGMRipSubpStream *subp);
gint          ogmrip_subp_stream_get_content  (OGMRipSubpStream *subp);
const gchar * ogmrip_subp_stream_get_label    (OGMRipSubpStream *subp);
gint          ogmrip_subp_stream_get_language (OGMRipSubpStream *subp);
gint          ogmrip_subp_stream_get_newline  (OGMRipSubpStream *subp);
gint          ogmrip_subp_stream_get_nr       (OGMRipSubpStream *subp);
gboolean      ogmrip_subp_stream_equal        (OGMRipSubpStream *subp1,
                                               OGMRipSubpStream *subp2);

G_END_DECLS

#endif /* __OGMRIP_MEDIA_SUBP_H__ */

