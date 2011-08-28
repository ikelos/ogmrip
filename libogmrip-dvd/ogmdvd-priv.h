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

#ifndef __OGMDVD_PRIV_H__
#define __OGMDVD_PRIV_H__

#include "ogmdvd-types.h"

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

#include <gio/gio.h>

#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>

G_BEGIN_DECLS

/**
 * OGMDvdDisc:
 *
 * An opaque structure representing a DVD disc
 */
struct _OGMDvdDisc
{
  guint ref;
  gchar *device;
  gchar *orig_device;
  gchar *label;
  gchar *id;

  guint ntitles;
  GList *titles;

  guint64 vmg_size;

  dvd_reader_t *reader;
  ifo_handle_t *vmg_file;

  GFileMonitor *monitor;
};

/**
 * OGMDvdTitle:
 *
 * An opaque structure representing a DVD title
 */
struct _OGMDvdTitle
{
  guint nr;

  guint8 nr_of_angles;

  OGMDvdVideoStream *video_stream;

  guint8 nr_of_audio_streams;
  GList *audio_streams;

  guint8 nr_of_subp_streams;
  GList *subp_streams;

  gulong *length_of_chapters;
  guint8 nr_of_chapters;

  guint64 vts_size;

  uint32_t palette[16];

  dvd_time_t playback_time;

  OGMDvdDisc *disc;
  gboolean close_disc;

  guint8 ttn;

  guint8 title_set_nr;
  ifo_handle_t *vts_file;

  gboolean analyzed;
  gboolean interlaced;
  gboolean progressive;
  gboolean telecine;

  gint *bitrates;
/*
  OGMDvdReader *reader;
  OGMDvdParser *parser;

  gint block_len;
  guchar *buffer;
  guchar *ptr;
*/
};

/**
 * OGMDvdStream:
 *
 * An opaque structure representing a DVD stream
 */
struct _OGMDvdStream
{
  OGMDvdTitle *title;
  guint16 id;
  guint nr;
};

/**
 * OGMDvdVideoStream:
 *
 * An opaque structure representing a DVD video stream
 */
struct _OGMDvdVideoStream
{
  OGMDvdStream stream;
  guint video_format : 2;
  guint picture_size : 2;
  guint display_aspect_ratio : 2;
  guint permitted_df : 2;

  guint crop_x;
  guint crop_y;
  guint crop_w;
  guint crop_h;
};

/**
 * OGMDvdAudioStream:
 *
 * An opaque structure representing a DVD audio stream
 */
struct _OGMDvdAudioStream
{
  OGMDvdStream stream;
  guint   format       : 3;
  guint   channels     : 3;
  guint   quantization : 2;
  guint8  code_extension;
  guint16 lang_code;
};

/**
 * OGMDvdSubpStream:
 *
 * An opaque structure representing a DVD subtitle stream
 */
struct _OGMDvdSubpStream
{
  OGMDvdStream stream;
  guint8 lang_extension;
  guint16 lang_code;
};

/**
 * OGMDvdReader:
 *
 * An opaque structure to read the content of a DVD
 */
struct _OGMDvdReader
{
  gint ref;

  dvd_file_t *file;
  pgc_t *pgc;

  guint8 angle;

  guint8 first_cell;
  guint8 last_cell;

  /* private */

  guint8 cur_cell;

  guint32 cell_first_pack;
  guint32 cell_cur_pack;
  guint32 packs_left;

  guint32 pack_next_vobu;
};

/**
 * OGMDvdParser:
 *
 * An opaque structure to parse the content of a DVD
 */
struct _OGMDvdParser
{
  gint ref;
  gint max_frames;

  glong progressive_frames;
  glong video_frames;
  
  gint *bitrates;
  gint nbitrates;
  gint naudio_streams;
  
  gboolean *forced_subs;
  gint nforced_subs;
  gint nsubp_streams;
  
  /* SPU specific */
  int size_sub;
  int size_rle;
  int size_got;
  int pts_forced;
  gint64 pts;
  guchar *data;
  
  /* mpeg2 specific */
  /* mpeg2dec_t *mpeg2; */
  /* const mpeg2_info_t *info; */
  guint width, height;
};

gulong ogmdvd_time_to_msec (dvd_time_t *dtime);

GSList * g_ulist_add_min           (GSList *ulist,
                                    gint   val);
GSList * g_ulist_add_max           (GSList *ulist,
                                    gint   val);
gint     g_ulist_get_most_frequent (GSList *ulist);
void     g_ulist_free              (GSList *ulist);

G_END_DECLS

#endif /* __OGMDVD_PRIV_H__ */

