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

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

#include "ogmdvd-parser.h"
#include "ogmdvd-contrib.h"
#include "ogmdvd-title.h"
#include "ogmdvd-priv.h"

#include <string.h>
#include <dvdread/dvd_reader.h>

/**
 * SECTION:ogmdvd-parser
 * @title: Parser
 * @include: ogmdvd-parser.h
 * @short_description: Parses the content of a DVD
 */

#define is_video_id(id) ((id) == 0xE0)

#define is_subp_id(id)  ((id) >= 0x20 && (id) <= 0x3F)

#define is_ac3_id(id)  (((id) >= 0x80 && (id) <= 0x87) || \
                        ((id) >= 0xC0 && (id) <= 0xCF))
#define is_dts_id(id)  (((id) >= 0x88 && (id) <= 0x8F) || \
                        ((id) >= 0x98 && (id) <= 0x9F))
#define is_pcm_id(id)   ((id) >= 0xA0 && (id) <= 0xBF)

#define is_audio_id(id) (is_ac3_id(id) || \
                         is_dts_id(id) || \
                         is_pcm_id(id))

#define subp_idx(id) ((id) - 0x20)
#define ac3_idx(id)  ((id) <= 0x87 ? (id) - 0x80 : (id) - 0xC0)
#define dts_idx(id)  ((id) <= 0x8F ? (id) - 0x88 : (id) - 0x98)
#define pcm_idx(id)  ((id) - 0xA0)

static gint
ogmdvd_parser_demux_block (guchar *buffer, gint *id, gint64 *pts)
{
  int sid, has_pts, len, pos = 0;
  int pes_packet_length, pes_packet_end;
  int pes_header_d_length, pes_header_end;

  g_return_val_if_fail (buffer != NULL, -1);

  if (id)
    *id = -1;

  if (pts)
    *pts = -1;

  /* pack_header */ 
  if (buffer[pos] != 0 || buffer[pos+1] != 0 ||
      buffer[pos+2] != 0x1 || buffer[pos+3] != 0xBA)
  { 
    g_warning ("Incorrect packet header");
    return -1;
  }
  
  pos += 4; /* pack_start_code */
  pos += 9; /* pack_header */
  pos += 1 + (buffer[pos] & 0x7); /* stuffing bytes */
  
  /* system_header */
  if (buffer[pos] == 0 && buffer[pos+1] == 0 &&
      buffer[pos+2] == 0x1 && buffer[pos+3] == 0xBB)
  {
    int header_length;

    pos += 4; /* system_header_start_code */
    header_length  = (buffer[pos] << 8) + buffer[pos+1];
    pos += 2 + header_length;
  }

  while (pos + 6 < DVD_VIDEO_LB_LEN && buffer[pos] == 0 &&
      buffer[pos+1] == 0 && buffer[pos+2] == 0x1)
  {
    pos += 3; /* packet_start_code_prefix */
    sid = buffer[pos];
    pos += 1;

    pes_packet_length = (buffer[pos] << 8) + buffer[pos+1];
    pos += 2; /* pes_packet_length */
    pes_packet_end = pos + pes_packet_length;

    if (sid != 0xE0 && sid != 0xBD && (sid & 0xC0) != 0xC0)
    {
      pos = pes_packet_end;
      continue;
    }

    has_pts = ((buffer[pos+1] >> 6) & 0x2) ? 1 : 0;
    pos += 2; /* Required headers */

    pes_header_d_length = buffer[pos];
    pos += 1;
    pes_header_end = pos + pes_header_d_length;

    if (pts && has_pts)
    {
      *pts = ((((guint64) buffer[pos] >> 1) & 0x7) << 30) +
        (buffer[pos+1] << 22) + ((buffer[pos+2] >> 1) << 15) +
        (buffer[pos+3] << 7)  +  (buffer[pos+4] >> 1);
    }

    pos = pes_header_end;

    if (sid == 0xBD)
    {
      sid |= (buffer[pos] << 8);
      if ((sid & 0xF0FF) == 0x80BD) /* A52 */
        pos += 4;
      else if ((sid & 0xE0FF) == 0x20BD || /* SPU */
               (sid & 0xF0FF) == 0xA0BD)  /* LPCM */
        pos += 1;
    }

    /* Sanity check */
    g_assert (pos < pes_packet_end);

    len = pes_packet_end - pos;
    memmove (buffer, buffer + pos, len);

    pos = pes_packet_end;

    if (id)
    {
      if ((sid & 0x00FF) == 0xBD)
        *id = (sid >> 8);
      else
        *id = sid;
    }

    return len;
  }
  
  return 0;
}

static gint
ogmdvd_parser_ac3 (guchar *buffer, gsize len, gint *flags, gint *rate, gint *bitrate)
{
  gint i, ret = 0;
    
  for (i = 0; i < len - 7; i++)
    if ((ret = a52_syncinfo (buffer + i, flags, rate, bitrate)))
      break;

  return ret;
}   
  
static gint
ogmdvd_parser_dts (guchar *buffer, gsize len, gint *flags, gint *rate, gint *bitrate)
{   
  dca_state_t *state;
  gint i, frame_length, ret = 0;

  state = dca_init ();
  for (i = 0; i < len - 7; i++)
    if ((ret = dca_syncinfo (state, buffer + i, flags, rate, bitrate, &frame_length)))
      break;
  dca_free (state);

  return ret;
}

static gint
ogmdvd_parser_spu_assemble (OGMDvdParser *parser, gsize len, const guchar *buffer, gint64 pts)
{
  if (!parser->size_sub || parser->size_sub == parser->size_got)
  {
    gint size_sub, size_rle;

    size_sub = (buffer[0] << 8 ) | buffer[1];
    size_rle = (buffer[2] << 8 ) | buffer[3];

    /* We are looking for the start of a new subtitle */
    if (size_sub && size_rle && size_sub > size_rle && len <= size_sub)
    {
      /* Looks all right so far */
      parser->size_sub = size_sub;
      parser->size_rle = size_rle;

      if (parser->data)
        g_free (parser->data);

      parser->data = g_new0 (guchar, size_sub);

      memcpy (parser->data, buffer, len);
      parser->size_got = len;
      parser->pts      = pts;
    }
  }
  else
  {
    /* We are waiting for the end of the current subtitle */
    if (len <= parser->size_sub - parser->size_got)
    {
      memcpy (parser->data + parser->size_got, buffer, len);
      parser->size_got += len;
      if (pts >= 0)
        parser->pts = pts;
    }
  }

  if (parser->size_sub && parser->size_sub == parser->size_got)
  {
    /* We got a complete subtitle, wait for the next one */
    return parser->size_sub;
  }

  return 0;
}

static void
ogmdvd_parser_spu_analyze (OGMDvdParser *parser)
{
  gint command, next, i;

  parser->pts_forced  = 0;

  i = parser->size_rle;
  while (1)
  {
    i += 2;
    next = (parser->data[i] << 8 ) | parser->data[i+1];
    i += 2;

    while (1)
    {
      command = parser->data[i++];

      if (command == 0xFF)
        break;

      switch (command)
      {
        case 0x00:
          parser->pts_forced = 1;
          break;
        case 0x03:
        case 0x04:
          i += 2;
          break;
        case 0x05:
          i += 6;
          break;
        case 0x06:
          i += 4;
          break;
      }
    }

    if( i > next )
      break;

    i = next;
  }
}

static gint
ogmdvd_parser_mpeg2 (OGMDvdParser *parser, guchar *buffer, gsize len)
{
/*
  gint flags = 0;

  g_return_val_if_fail (parser != NULL, -1);
  g_return_val_if_fail (buffer != NULL, -1);
  g_return_val_if_fail (len > 0, -1);

  if (buffer)
  {
    mpeg2_state_t state = STATE_END;

    mpeg2_buffer (parser->mpeg2, buffer, buffer + len);

    while (state != STATE_BUFFER)
    {
      state = mpeg2_parse (parser->mpeg2);
      if ((state == STATE_SLICE || state == STATE_END) && parser->info->display_fbuf)
        flags = parser->info->display_picture->flags;
      else if (state == STATE_INVALID)
        mpeg2_reset (parser->mpeg2, 0);
    }
  }

  return flags;
*/
  return 0;
}

/**
 * ogmdvd_parser_new:
 * @title: An #OGMDvdTitle
 *
 * Creates a new #OGMDvdParser.
 *
 * Returns: The new #OGMDvdParser, or NULL
 */
OGMDvdParser *
ogmdvd_parser_new (OGMDvdTitle *title)
{
  OGMDvdParser *parser;

  parser = g_new0 (OGMDvdParser, 1);
  parser->ref = 1;
  parser->max_frames = 100;
/*
  parser->mpeg2 = mpeg2_init ();
  parser->info = mpeg2_info (parser->mpeg2);
*/
  ogmrip_video_stream_get_resolution (OGMRIP_VIDEO_STREAM (title), &parser->width, &parser->height);

  parser->naudio_streams = ogmrip_title_get_n_audio_streams (OGMRIP_TITLE (title));
  parser->bitrates = g_new0 (int, parser->naudio_streams);

  parser->nsubp_streams = ogmrip_title_get_n_subp_streams (OGMRIP_TITLE (title));
  parser->forced_subs = g_new0 (int, parser->nsubp_streams);

  return parser;
}

/**
 * ogmdvd_parser_ref:
 * @parser: An #OGMDvdParser
 *
 * Increments the reference count of an #OGMDvdParser.
 */
void
ogmdvd_parser_ref (OGMDvdParser *parser)
{
  g_return_if_fail (parser != NULL);

  parser->ref ++;
}

/**
 * ogmdvd_parser_unref:
 * @parser: An #OGMDvdParser
 *
 * Decrements the reference count of an #OGMDvdParser.
 */
void
ogmdvd_parser_unref (OGMDvdParser *parser)
{
  g_return_if_fail (parser != NULL);

  if (parser->ref > 0)
  {
    parser->ref --;

    if (parser->ref == 0)
    {
/*
      mpeg2_close (parser->mpeg2);
      parser->mpeg2 = NULL;
      parser->info = NULL;
*/
      g_free (parser->bitrates);
      parser->bitrates = NULL;

      g_free (parser->forced_subs);
      parser->forced_subs = NULL;

      if (parser->data)
        g_free (parser->data);
      parser->data = NULL;

      g_free (parser);
    }
  }
}

/**
 * ogmdvd_parser_set_max_frames:
 * @parser: An #OGMDvdParser
 * @max_frames: A number of frames
 *
 * Sets the maximum number of frames to analyse. If @max_frames is negative,
 * all the frames of the title will be analyzed.
 */
void
ogmdvd_parser_set_max_frames (OGMDvdParser *parser, gint max_frames)
{
  g_return_if_fail (parser != NULL);
  g_return_if_fail (max_frames != 0);

  if (max_frames < 0)
    parser->max_frames = -1;

  parser->max_frames = max_frames;
}

/**
 * ogmdvd_parser_get_max_frames:
 * @parser: An #OGMDvdParser
 *
 * Returns the maximum number of frames to analyze.
 *
 * Returns: The number of frames, -1 if the whole title is to be analyzed, 0 on error
 */
gint
ogmdvd_parser_get_max_frames (OGMDvdParser *parser)
{
  g_return_val_if_fail (parser != NULL, 0);

  return parser->max_frames;
}

/**
 * ogmdvd_parser_analyze:
 * @parser: An #OGMDvdParser
 * @buffer: A buffer containing a block to analyze
 *
 * Analyzes the block contained in @buffer.
 *
 * Returns: the status of the analysis
 */
gint
ogmdvd_parser_analyze (OGMDvdParser *parser, guchar *buffer)
{
  gint id, status = OGMDVD_PARSER_STATUS_NONE;
  gint64 pts;
  gsize len;

  g_return_val_if_fail (parser != NULL, -1);
  g_return_val_if_fail (buffer != NULL, -1);

  if ((len = ogmdvd_parser_demux_block (buffer, &id, &pts)))
  {
    if (is_video_id (id))
    {
      if (parser->max_frames < 0 || parser->video_frames < parser->max_frames)
      {
        if (parser->progressive_frames < 6)
        {
          gint flags;

          flags = ogmdvd_parser_mpeg2 (parser, buffer, len);
          /*
          if (parser->height == 480 && (flags & PIC_FLAG_PROGRESSIVE_FRAME))
            parser->progressive_frames ++;
          */
        }
        parser->video_frames ++;
      }
    }
    else if (is_audio_id (id))
    {
      gint flags = 0, sample_rate = 0, bit_rate = 0;

      if (is_ac3_id (id) && !parser->bitrates[ac3_idx(id)] &&
          ogmdvd_parser_ac3 (buffer, len, &flags, &sample_rate, &bit_rate))
      {
        parser->bitrates[ac3_idx(id)] = bit_rate;
        parser->nbitrates ++;
      }
      else if (is_dts_id (id) && !parser->bitrates[dts_idx(id)] &&
          ogmdvd_parser_dts (buffer, len, &flags, &sample_rate, &bit_rate))
      {
        parser->bitrates[ac3_idx(id)] = bit_rate;
        parser->nbitrates ++;
      }
      else if (is_pcm_id (id) && !parser->bitrates[pcm_idx(id)])
      {
        parser->bitrates[pcm_idx(id)] = -1;
        parser->nbitrates ++;
      }
    }
    else if (is_subp_id (id))
    {
      if (!parser->forced_subs[subp_idx(id)] &&
          ogmdvd_parser_spu_assemble (parser, len, buffer, pts))
      {
        ogmdvd_parser_spu_analyze (parser);
        if (parser->pts_forced)
        {
          parser->forced_subs[subp_idx(id)] = parser->pts_forced;
          parser->nforced_subs ++;
        }
      }
    }
    else
      g_warning ("unknown id: %u", id);
  }

  if (parser->naudio_streams == parser->nbitrates)
    status |= OGMDVD_PARSER_STATUS_BITRATES;

  if (parser->video_frames > parser->max_frames)
    status |= OGMDVD_PARSER_STATUS_MAX_FRAMES;

  return status;
}

/**
 * ogmdvd_parser_get_video_frames:
 * @parser: An #OGMDvdParser
 *
 * Returns the number of video frames already parsed.
 *
 * Returns: The number of frames, or -1
 */
glong
ogmdvd_parser_get_video_frames (OGMDvdParser *parser)
{
  g_return_val_if_fail (parser != NULL, -1);

  return parser->video_frames;
}
/*
gboolean
ogmdvd_parser_get_progressive (OGMDvdParser *parser)
{
  g_return_val_if_fail (parser != NULL, FALSE);

  return parser->progressive_frames >= 6;
}
*/

/**
 * ogmdvd_parser_get_audio_bitrate:
 * @parser: An #OGMDvdParser
 * @nr: The audio stream number
 *
 * Returns the bitrate of the audio stream at position nr, starting at 0.
 *
 * Returns: The bitrate, or -1
 */
gint
ogmdvd_parser_get_audio_bitrate (OGMDvdParser *parser, guint nr)
{
  g_return_val_if_fail (parser != NULL, -1);
  g_return_val_if_fail (parser->naudio_streams == parser->nbitrates, -1);
  g_return_val_if_fail (nr < parser->naudio_streams, -1);

  return parser->bitrates[nr];
}

