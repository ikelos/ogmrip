/* OGMRipDvd - A DVD library for OGMRip
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#include <ogmrip-media.h>
#include <ogmdvd-disc.h>
#include <ogmdvd-title.h>

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
struct _OGMDvdDiscPriv
{
  gchar *uri;
  gchar *device;
  gchar *label;
  gchar *id;
  gchar *real_id;

  guint ntitles;
  GList *titles;

  guint64 vmg_size;

  dvd_reader_t *reader;
  ifo_handle_t *vmg_file;
  guint nopen;

  OGMDvdDisc *copy;
};

/**
 * OGMDvdTitle:
 *
 * An opaque structure representing a DVD title
 */
struct _OGMDvdTitlePriv
{
  guint nr;

  guint8 nr_of_angles;

  guint8 nr_of_audio_streams;
  GList *audio_streams;

  guint8 nr_of_subp_streams;
  GList *subp_streams;

  gulong *length_of_chapters;
  guint8 nr_of_chapters;

  guint64 vts_size;
  guint nopen;

  uint32_t palette[16];

  guint video_format;
  guint picture_size;
  guint display_aspect_ratio;
  guint permitted_df;

  gboolean analyzed;
  gboolean interlaced;
  gboolean progressive;
  gboolean telecine;

  guint crop_x;
  guint crop_y;
  guint crop_w;
  guint crop_h;

  dvd_time_t playback_time;

  OGMRipMedia *disc;

  guint8 ttn;

  guint8 title_set_nr;
  ifo_handle_t *vts_file;

  gint *bitrates;
};

/**
 * OGMDvdAudioStream:
 *
 * An opaque structure representing a DVD audio stream
 */
struct _OGMDvdAudioStreamPriv
{
  OGMRipTitle *title;
  guint id;
  guint format;
  guint channels;
  guint quantization;
  guint code_extension;
  guint lang_code;
};

/**
 * OGMDvdSubpStream:
 *
 * An opaque structure representing a DVD subtitle stream
 */
struct _OGMDvdSubpStreamPriv
{
  OGMRipTitle *title;
  guint id;
  guint code_extension;
  guint lang_code;
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
  gsize size_sub;
  gsize size_rle;
  gsize size_got;
  int pts_forced;
  gint64 pts;
  guchar *data;
  
  /* mpeg2 specific */
  /* mpeg2dec_t *mpeg2; */
  /* const mpeg2_info_t *info; */
  guint width, height;
};

gulong ogmdvd_time_to_msec (dvd_time_t *dtime);

G_END_DECLS

#endif /* __OGMDVD_PRIV_H__ */

