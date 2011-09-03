/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_CHAPTERS_H__
#define __OGMRIP_CHAPTERS_H__

#include <ogmrip-codec.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_CHAPTERS           (ogmrip_chapters_get_type ())
#define OGMRIP_CHAPTERS(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_CHAPTERS, OGMRipChapters))
#define OGMRIP_CHAPTERS_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_CHAPTERS, OGMRipChaptersClass))
#define OGMRIP_IS_CHAPTERS(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_CHAPTERS))
#define OGMRIP_IS_CHAPTERS_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_CHAPTERS))
#define OGMRIP_CHAPTERS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_CHAPTERS, OGMRipChaptersClass))

typedef struct _OGMRipChapters      OGMRipChapters;
typedef struct _OGMRipChaptersPriv  OGMRipChaptersPriv;
typedef struct _OGMRipChaptersClass OGMRipChaptersClass;

struct _OGMRipChapters
{
  OGMRipCodec parent_instance;

  OGMRipChaptersPriv *priv;
};

struct _OGMRipChaptersClass
{
  OGMRipCodecClass parent_class;
};

GType         ogmrip_chapters_get_type     (void);
OGMRipCodec * ogmrip_chapters_new          (OGMRipVideoStream *stream);
const gchar * ogmrip_chapters_get_label    (OGMRipChapters    *chapters, 
                                            guint             n);
void          ogmrip_chapters_set_label    (OGMRipChapters    *chapters, 
                                            guint             n, 
                                            const gchar       *label);
void          ogmrip_chapters_set_language (OGMRipChapters    *chapters,
                                            guint             language);
gint          ogmrip_chapters_get_language (OGMRipChapters    *chapters);

G_END_DECLS

#endif /* __OGMRIP_CHAPTERS_H__ */

