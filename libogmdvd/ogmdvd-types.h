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

#ifndef __OGMDVD_TYPES_H__
#define __OGMDVD_TYPES_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct
{
  guint hour;
  guint min;
  guint sec;
  guint frames;
} OGMDvdTime;

#define OGMDVD_DISC(disc)          ((OGMDvdDisc *) (disc))
#define OGMDVD_TITLE(title)        ((OGMDvdTitle *) (title))
#define OGMDVD_STREAM(stream)      ((OGMDvdStream *) (stream))
#define OGMDVD_AUDIO_STREAM(audio) ((OGMDvdAudioStream *) (audio))
#define OGMDVD_SUBP_STREAM(subp)   ((OGMDvdSubpStream *) (subp))
#define OGMDVD_READER(reader)      ((OGMDvdReader *) (reader))
#define OGMDVD_PARSER(parser)      ((OGMDvdParser *) (parser))

typedef struct _OGMDvdDisc        OGMDvdDisc;
typedef struct _OGMDvdTitle       OGMDvdTitle;
typedef struct _OGMDvdStream      OGMDvdStream;
typedef struct _OGMDvdAudioStream OGMDvdAudioStream;
typedef struct _OGMDvdSubpStream  OGMDvdSubpStream;
typedef struct _OGMDvdReader      OGMDvdReader;
typedef struct _OGMDvdParser      OGMDvdParser;

G_END_DECLS

#endif /* __OGMDVD_TYPES_H__ */

