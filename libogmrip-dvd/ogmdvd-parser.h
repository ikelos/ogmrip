/* OGMDvd - A wrapper library around libdvdread
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

#ifndef __OGMDVD_PARSER_H__
#define __OGMDVD_PARSER_H__

#include <ogmdvd-title.h>

G_BEGIN_DECLS

/**
 * OGMDvdParserStatus:
 * @OGMDVD_PARSER_STATUS_NONE: There is no information available yet
 * @OGMDVD_PARSER_STATUS_BITRATES: The bitrates of all the audio tracks are available
 * @OGMDVD_PARSER_STATUS_MAX_FRAMES: The maximum number of frames has been reached
 *
 * Status code returned by ogmdvd_parser_analyze()
 */
typedef enum
{
  OGMDVD_PARSER_STATUS_NONE,
  OGMDVD_PARSER_STATUS_BITRATES,
  OGMDVD_PARSER_STATUS_MAX_FRAMES
} OGMDvdParserStatus;

typedef struct _OGMDvdParser OGMDvdParser;

OGMDvdParser * ogmdvd_parser_new               (OGMDvdTitle  *title);
void           ogmdvd_parser_ref               (OGMDvdParser *parser);
void           ogmdvd_parser_unref             (OGMDvdParser *parser);

void           ogmdvd_parser_set_max_frames    (OGMDvdParser *parser,
                                                gint         max_frames);
gint           ogmdvd_parser_get_max_frames    (OGMDvdParser *parser);

gint           ogmdvd_parser_analyze           (OGMDvdParser *parser,
                                                guchar       *buffer);

gint           ogmdvd_parser_get_audio_bitrate (OGMDvdParser *parser,
                                                guint        nr);

G_END_DECLS

#endif /* __OGMDVD_PARSER_H__ */

