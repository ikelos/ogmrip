/* OGMRipDvd - A DVD library for OGMRip
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmdvd-contrib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * The following code is part of libdca, a free DTS Coherent Acoustics
 * stream decoder.
 * See http://www.videolan.org/developers/libdca.html for updates.
 *
 * Copyright (C) 2004 Gildas Bazin <gbazin@videolan.org>
 *
 * libdca is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdca is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef WORDS_BIGENDIAN
#define swab32(x) (x)
#else
#define swab32(x)\
  ((((uint8_t*)&x)[0] << 24) | (((uint8_t*)&x)[1] << 16) |  \
   (((uint8_t*)&x)[2] << 8)  | (((uint8_t*)&x)[3]))
#endif

#ifdef WORDS_BIGENDIAN
#define swable32(x)\
  ((((uint8_t*)&x)[0] << 16) | (((uint8_t*)&x)[1] << 24) |  \
   (((uint8_t*)&x)[2])  | (((uint8_t*)&x)[3] << 8))
#else
#define swable32(x)\
  ((((uint32_t)x) >> 16) | (((uint32_t)x) << 16))
#endif

struct dca_state_s
{
  uint32_t *buffer_start;
  uint32_t bits_left;
  uint32_t current_word;
  int      word_mode;         /* 16/14 bits word format (1 -> 16, 0 -> 14) */
  int      bigendian_mode;    /* endianness (1 -> be, 0 -> le) */
};

static const int dca_sample_rates[] =
{
  0, 8000, 16000, 32000, 0, 0, 11025, 22050,
  44100, 0, 0, 12000, 24000, 48000, 96000, 192000
};

static const int dca_bit_rates[] =
{
  32000, 56000, 64000, 96000, 112000, 128000,
  192000, 224000, 256000, 320000, 384000,
  448000, 512000, 576000, 640000, 768000,
  896000, 1024000, 1152000, 1280000, 1344000,
  1408000, 1411200, 1472000, 1536000, 1920000,
  2048000, 3072000, 3840000, 1/*open*/, 2/*variable*/, 3/*lossless*/
};

static inline void
bitstream_fill_current (dca_state_t * state)
{
  uint32_t tmp;

  tmp = *(state->buffer_start++);

  if (state->bigendian_mode)
    state->current_word = swab32 (tmp);
  else
    state->current_word = swable32 (tmp);

  if (!state->word_mode)
    state->current_word = (state->current_word & 0x00003FFF) |
      ((state->current_word & 0x3FFF0000 ) >> 2);
}

static uint32_t
dca_bitstream_get_bh (dca_state_t * state, uint32_t num_bits)
{
  uint32_t result;

  num_bits -= state->bits_left;

  result = ((state->current_word << (32 - state->bits_left)) >>
      (32 - state->bits_left));

  if ( !state->word_mode && num_bits > 28 )
  {
    bitstream_fill_current (state);
    result = (result << 28) | state->current_word;
    num_bits -= 28;
  }

  bitstream_fill_current (state);

  if ( state->word_mode )
  {
    if (num_bits != 0)
      result = (result << num_bits) |
        (state->current_word >> (32 - num_bits));

    state->bits_left = 32 - num_bits;
  }
  else
  {
    if (num_bits != 0)
      result = (result << num_bits) |
        (state->current_word >> (28 - num_bits));

    state->bits_left = 28 - num_bits;
  }

  return result;
}

static inline uint32_t
bitstream_get (dca_state_t * state, uint32_t num_bits)
{
  uint32_t result;

  if (num_bits < state->bits_left)
  {
    result = (state->current_word << (32 - state->bits_left)) >> (32 - num_bits);

    state->bits_left -= num_bits;
    return result;
  }

  return dca_bitstream_get_bh (state, num_bits);
}

void
dca_bitstream_init (dca_state_t * state, uint8_t * buf, int word_mode, int bigendian_mode)
{
  intptr_t align;

  align = (uintptr_t)buf & 3;
  state->buffer_start = (uint32_t *) ((uintptr_t)buf - align);
  state->bits_left = 0; 
  state->current_word = 0;
  state->word_mode = word_mode;
  state->bigendian_mode = bigendian_mode;
  bitstream_get (state, align * 8);
}

static int
syncinfo (dca_state_t * state, int * flags, int * sample_rate, int * bit_rate, int * frame_length)
{
  int frame_size;

  /* Sync code */
  bitstream_get (state, 32);
  /* Frame type */
  bitstream_get (state, 1);
  /* Samples deficit */
  bitstream_get (state, 5);
  /* CRC present */
  bitstream_get (state, 1);

  *frame_length = (bitstream_get (state, 7) + 1) * 32;
  frame_size = bitstream_get (state, 14) + 1;
  if (!state->word_mode)
    frame_size = frame_size * 8 / 14 * 2;

  /* Audio channel arrangement */
  *flags = bitstream_get (state, 6);
  if (*flags > 63)
    return 0;

  *sample_rate = bitstream_get (state, 4);
  if ((size_t)*sample_rate >= sizeof (dca_sample_rates) / sizeof (int))
    return 0;
  *sample_rate = dca_sample_rates[ *sample_rate ];
  if (!*sample_rate)
    return 0;

  *bit_rate = bitstream_get (state, 5);
  if ((size_t)*bit_rate >= sizeof (dca_bit_rates) / sizeof (int))
    return 0;
  *bit_rate = dca_bit_rates[ *bit_rate ];
  if (!*bit_rate)
    return 0;

  /* LFE */
  bitstream_get (state, 10);
  if (bitstream_get (state, 2))
    *flags |= DCA_LFE;

  return frame_size;
}

dca_state_t *
dca_init (void)
{
  dca_state_t * state;

  state = (dca_state_t *) malloc (sizeof (dca_state_t));
  if (state == NULL)
    return NULL;

  memset (state, 0, sizeof(dca_state_t));

  return state;
}

void
dca_free (dca_state_t * state)
{
  if (state)
    free (state);
}

int
dca_syncinfo (dca_state_t * state, uint8_t * buf, int * flags,
    int * sample_rate, int * bit_rate, int * frame_length)
{
  int frame_size = 0;

  /*
   * Look for sync code
   */

  /* 14 bits and little endian bitstream */
  if (buf[0] == 0xff && buf[1] == 0x1f &&
      buf[2] == 0x00 && buf[3] == 0xe8 &&
      (buf[4] & 0xf0) == 0xf0 && buf[5] == 0x07)
  {
    dca_bitstream_init (state, buf, 0, 0);
    frame_size = syncinfo (state, flags, sample_rate, bit_rate, frame_length);
  }

  /* 14 bits and big endian bitstream */
  if (buf[0] == 0x1f && buf[1] == 0xff &&
      buf[2] == 0xe8 && buf[3] == 0x00 &&
      buf[4] == 0x07 && (buf[5] & 0xf0) == 0xf0)
  {
    dca_bitstream_init (state, buf, 0, 1);
    frame_size = syncinfo (state, flags, sample_rate, bit_rate, frame_length);
  }

  /* 16 bits and little endian bitstream */
  if (buf[0] == 0xfe && buf[1] == 0x7f &&
      buf[2] == 0x01 && buf[3] == 0x80)
  {
    dca_bitstream_init (state, buf, 1, 0);
    frame_size = syncinfo (state, flags, sample_rate, bit_rate, frame_length);
  }

  /* 16 bits and big endian bitstream */
  if (buf[0] == 0x7f && buf[1] == 0xfe &&
      buf[2] == 0x80 && buf[3] == 0x01)
  {
    dca_bitstream_init (state, buf, 1, 1);
    frame_size = syncinfo (state, flags, sample_rate, bit_rate, frame_length);
  }

  return frame_size;
}

/*
 * The following code is part of a52dec, a free ATSC A-52 stream decoder.
 * See http://liba52.sourceforge.net/ for updates.
 *
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * a52dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * a52dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define A52_DOLBY 10
#define A52_LFE   16

int
a52_syncinfo (uint8_t * buf, int * flags, int * sample_rate, int * bit_rate)
{   
  static const int rate[] =
  { 32,  40,  48,  56,  64,  80,  96, 112, 128,
    160, 192, 224, 256, 320, 384, 448, 512, 576, 640 };
  static const uint8_t lfeon[8] =
  { 0x10, 0x10, 0x04, 0x04, 0x04, 0x01, 0x04, 0x01 };
  static const uint8_t halfrate[12] =
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3 };

  int frmsizecod;
  int bitrate;
  int half;
  int acmod;

  if ((buf[0] != 0x0b) || (buf[1] != 0x77)) /* syncword */
    return 0;
    
  if (buf[5] >= 0x60)   /* bsid >= 12 */
    return 0; 
  half = halfrate[buf[5] >> 3];
  
  /* acmod, dsurmod and lfeon */
  acmod = buf[6] >> 5;
  *flags = ((((buf[6] & 0xf8) == 0x50) ? A52_DOLBY :
        acmod) | ((buf[6] & lfeon[acmod]) ? A52_LFE : 0));

  frmsizecod = buf[4] & 63;
  if (frmsizecod >= 38)
    return 0;
  bitrate = rate [frmsizecod >> 1];
  *bit_rate = (bitrate * 1000) >> half;

  switch (buf[4] & 0xc0)
  {
    case 0:
      *sample_rate = 48000 >> half;
      return 4 * bitrate;
    case 0x40:
      *sample_rate = 44100 >> half;
      return 2 * (320 * bitrate / 147 + (frmsizecod & 1));
    case 0x80:
      *sample_rate = 32000 >> half;
      return 6 * bitrate;
    default:
      return 0;
  }
}

