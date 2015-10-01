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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmdvd-reader.h"
#include "ogmdvd-priv.h"
#include "ogmdvd-disc.h"

#include <dvdread/nav_read.h>

/**
 * SECTION:ogmdvd-reader
 * @title: Reader
 * @include: ogmdvd-reader.h
 * @short_description: Reads the content of a DVD
 */

/**
 * ogmdvd_reader_new:
 * @title: An #OGMDvdTitle
 * @start_chap: The chapter to start reading at, 0 for the first chapter
 * @end_chap: The chapter to stop reading at, -1 for the last chapter
 * @angle: The angle to read
 *
 * Creates a new #OGMDvdReader.
 *
 * Returns: The new #OGMDvdReader, or NULL
 */
OGMDvdReader *
ogmdvd_reader_new (OGMDvdTitle *title, guint start_chap, gint end_chap, guint angle)
{
  OGMDvdDisc *disc = OGMDVD_DISC (title->priv->disc);
  OGMDvdReader *reader;

  guint8 vts;
  guint16 pgcn, pgn;

  dvd_file_t *file;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (end_chap < 0 || start_chap <= (guint) end_chap, NULL);

  vts = disc->priv->vmg_file ? disc->priv->vmg_file->tt_srpt->title[title->priv->nr].title_set_nr : 1;
  file = DVDOpenFile (disc->priv->reader, vts, DVD_READ_TITLE_VOBS);

  g_return_val_if_fail (file != NULL, NULL);

  reader = g_new0 (OGMDvdReader, 1);
  reader->file = file;
  reader->ref = 1;

  pgcn = title->priv->vts_file->vts_ptt_srpt->title[title->priv->ttn - 1].ptt[start_chap].pgcn;
  pgn  = title->priv->vts_file->vts_ptt_srpt->title[title->priv->ttn - 1].ptt[start_chap].pgn;

  reader->angle = angle;
  reader->pgc = title->priv->vts_file->vts_pgcit->pgci_srp[pgcn - 1].pgc;

  reader->first_cell = reader->pgc->program_map[pgn - 1] - 1;

  reader->last_cell = reader->pgc->nr_of_cells;
  if (end_chap > -1 && end_chap < reader->pgc->nr_of_programs - 1)
  {
    pgn = title->priv->vts_file->vts_ptt_srpt->title[title->priv->ttn - 1].ptt[end_chap + 1].pgn;
    reader->last_cell = reader->pgc->program_map[pgn - 1] - 1;
  }

  if (reader->pgc->cell_playback[reader->first_cell].block_type == BLOCK_TYPE_ANGLE_BLOCK )
    reader->first_cell += angle;

  return reader;
}

/**
 * ogmdvd_reader_new_by_cells:
 * @title: An #OGMDvdTitle
 * @start_cell: The cell to start reading at, 0 for the first cell
 * @end_cell: The cell to stop reading at, -1 for the last cell
 * @angle: The angle to read
 *
 * Creates a new #OGMDvdReader.
 *
 * Returns: The new #OGMDvdReader, or NULL
 */
OGMDvdReader *
ogmdvd_reader_new_by_cells (OGMDvdTitle *title, guint start_cell, gint end_cell, guint angle)
{
  OGMDvdDisc *disc = OGMDVD_DISC (title->priv->disc);
  OGMDvdReader *reader;
  pgc_t *pgc;

  guint8 vts;
  guint16 pgcn;

  dvd_file_t *file;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (end_cell < 0 || start_cell <= (guint) end_cell, NULL);

  pgcn = title->priv->vts_file->vts_ptt_srpt->title[title->priv->ttn - 1].ptt[0].pgcn;
  pgc = title->priv->vts_file->vts_pgcit->pgci_srp[pgcn - 1].pgc;

  g_return_val_if_fail (start_cell < pgc->nr_of_cells && end_cell <= pgc->nr_of_cells, NULL);

  vts = disc->priv->vmg_file ? disc->priv->vmg_file->tt_srpt->title[title->priv->nr].title_set_nr : 1;
  file = DVDOpenFile (disc->priv->reader, vts, DVD_READ_TITLE_VOBS);

  g_return_val_if_fail (file != NULL, NULL);

  reader = g_new0 (OGMDvdReader, 1);
  reader->angle = angle;
  reader->file = file;
  reader->pgc = pgc;
  reader->ref = 1;

  reader->first_cell = start_cell;
  reader->last_cell = end_cell;

  if (reader->pgc->cell_playback[reader->first_cell].block_type == BLOCK_TYPE_ANGLE_BLOCK )
    reader->first_cell += angle;

  return reader;
}

/**
 * ogmdvd_reader_ref:
 * @reader: An #OGMDvdReader
 *
 * Increments the reference count of an #OGMDvdReader.
 */
void
ogmdvd_reader_ref (OGMDvdReader *reader)
{
  g_return_if_fail (reader != NULL);

  reader->ref ++;
}

/**
 * ogmdvd_reader_unref:
 * @reader: An #OGMDvdReader
 *
 * Decrements the reference count of an #OGMDvdReader.
 */
void
ogmdvd_reader_unref (OGMDvdReader *reader)
{
  g_return_if_fail (reader != NULL);

  if (reader->ref > 0)
  {
    reader->ref --;

    if (reader->ref == 0)
    {
      DVDCloseFile (reader->file);

      g_free (reader);
    }
  }
}

static gint
is_nav_pack (guint8 *ptr)
{
  guint32 start_code;

  start_code = (guint32) (ptr[0]) << 24;
  start_code |= (guint32) (ptr[1]) << 16;
  start_code |= (guint32) (ptr[2]) << 8;
  start_code |= (guint32) (ptr[3]);

  if (start_code != 0x000001ba)
    return 0;

  if ((ptr[4] & 0xc0) != 0x40)
    return 0;

  ptr += 14;

  start_code = (guint32) (ptr[0]) << 24;
  start_code |= (guint32) (ptr[1]) << 16;
  start_code |= (guint32) (ptr[2]) << 8;
  start_code |= (guint32) (ptr[3]);

  if (start_code != 0x000001bb)
    return 0;

  ptr += 24;

  start_code = (guint32) (ptr[0]) << 24;
  start_code |= (guint32) (ptr[1]) << 16;
  start_code |= (guint32) (ptr[2]) << 8;
  start_code |= (guint32) (ptr[3]);

  if (start_code != 0x000001bf)
    return 0;

  ptr += 986;

  start_code = (guint32) (ptr[0]) << 24;
  start_code |= (guint32) (ptr[1]) << 16;
  start_code |= (guint32) (ptr[2]) << 8;
  start_code |= (guint32) (ptr[3]);

  if (start_code != 0x000001bf)
    return 0;

  return 1;
}

static gint
ogmdvd_reader_get_next_cell (OGMDvdReader *reader)
{
  guint8 next_cell = reader->cur_cell;

  if (reader->pgc->cell_playback[next_cell].block_type == BLOCK_TYPE_ANGLE_BLOCK )
  {
    while (next_cell < reader->last_cell)
    {
      if (reader->pgc->cell_playback[next_cell].block_mode == BLOCK_MODE_LAST_CELL)
        break;
      next_cell ++;
    }
  }

  next_cell ++;
  if (next_cell >= reader->last_cell)
    return 0; /* EOF */

  if (reader->pgc->cell_playback[next_cell].block_type == BLOCK_TYPE_ANGLE_BLOCK )
  {
    next_cell += reader->angle;
    if (next_cell >= reader->last_cell)
      return 0; /* EOF */
  }

  reader->cur_cell = next_cell;

  reader->cell_first_pack = reader->pgc->cell_playback[next_cell].first_sector;

  reader->cell_cur_pack = 0;
  reader->pack_next_vobu = 0;

  return next_cell;
}

/**
 * ogmdvd_reader_get_block:
 * @reader: An #OGMDvdReader
 * @len: The number of blocks to read
 * @buffer: The buffer to store the blocks read
 *
 * Reads up to @len blocks from the DVD into the buffer starting at @buffer.
 *
 * Returns: The number of blocks read, or -1
 */
int
ogmdvd_reader_get_block (OGMDvdReader *reader, gsize len, guchar *buffer)
{
  guchar buf[DVD_VIDEO_LB_LEN];
  dsi_t dsi_pack;

  g_return_val_if_fail (reader != NULL, -1);
  g_return_val_if_fail (buffer != NULL, -1);
  g_return_val_if_fail (len > 0, -1);
  
  if (reader->packs_left == 0)
  {
    /* Enf of vobu */
    
    if (reader->pack_next_vobu == SRI_END_OF_CELL)
    {
      /* End of cell */
      if (!ogmdvd_reader_get_next_cell (reader))
        return 0;
      reader->pack_next_vobu = 0;
    }
    
    if (!reader->pack_next_vobu)
    {
      /* First cell or end of cell */
      if (!reader->cur_cell)
      {
        /* First cell */
        reader->cur_cell = reader->first_cell;
        reader->cell_first_pack = reader->pgc->cell_playback[reader->first_cell].first_sector;
      }
    }
    else
    {
      /* Go to next vobu */
      reader->cell_first_pack += reader->pack_next_vobu & 0x7fffffff;
    }

    reader->cell_cur_pack = reader->cell_first_pack;

    if (DVDReadBlocks (reader->file, reader->cell_cur_pack, 1, buf) != 1)
    {
      g_warning ("Error while readint NAVI block");
      return -1;
    }

    if (!is_nav_pack (buf))
    {
      g_warning ("NAVI block expected");
      return -1;
    }

    navRead_DSI (&dsi_pack, buf + DSI_START_BYTE);

    if (reader->cell_cur_pack != dsi_pack.dsi_gi.nv_pck_lbn)
    {
      g_warning ("Bad current pack");
      return -1;
    }

    reader->cell_cur_pack ++;

    reader->packs_left = dsi_pack.dsi_gi.vobu_ea;
    reader->pack_next_vobu = dsi_pack.vobu_sri.next_vobu;

    if (reader->packs_left >= 1024)
    {
      g_warning ("Number of packets >= 1024");
      return -1;
    }
  }
  if (len > reader->packs_left)
    len = reader->packs_left;

  if (DVDReadBlocks (reader->file, reader->cell_cur_pack, len, buffer) != (gint) len)
    return -1;

  reader->cell_cur_pack += len;
  reader->packs_left -= len;

  return len;
}

