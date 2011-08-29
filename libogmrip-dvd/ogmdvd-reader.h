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

#ifndef __OGMDVD_READER_H__
#define __OGMDVD_READER_H__

#include <ogmdvd-title.h>

G_BEGIN_DECLS

#ifndef DVD_VIDEO_LB_LEN
#define DVD_VIDEO_LB_LEN 2048
#endif

typedef struct _OGMDvdReader OGMDvdReader;

OGMDvdReader * ogmdvd_reader_new          (OGMDvdTitle  *title,
                                           guint        start_chap,
                                           gint         end_chap,
                                           guint        angle);
OGMDvdReader * ogmdvd_reader_new_by_cells (OGMDvdTitle  *title,
                                           guint        start_cell,
                                           gint         end_cell,
                                           guint        angle);
void           ogmdvd_reader_ref          (OGMDvdReader *reader);
void           ogmdvd_reader_unref        (OGMDvdReader *reader);
gint           ogmdvd_reader_get_block    (OGMDvdReader *reader,
                                           gsize        len,
                                           guchar       *buffer);

G_END_DECLS

#endif /* __OGMDVD_READER_H__ */

