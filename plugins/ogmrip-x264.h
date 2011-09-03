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

#ifndef __OGMRIP_X264_H__
#define __OGMRIP_X264_H__

#include <ogmrip-encode.h>

G_BEGIN_DECLS

#define OGMRIP_X264_PROP_8X8DCT           "dct8x8"
#define OGMRIP_X264_PROP_AUD              "aud"
#define OGMRIP_X264_PROP_B_ADAPT          "b-adapt"
#define OGMRIP_X264_PROP_B_PYRAMID        "b-pyramid"
#define OGMRIP_X264_PROP_BRDO             "brdo"
#define OGMRIP_X264_PROP_CABAC            "cabac"
#define OGMRIP_X264_PROP_CQM              "cqm"
#define OGMRIP_X264_PROP_DIRECT           "direct"
#define OGMRIP_X264_PROP_FRAMEREF         "frameref"
#define OGMRIP_X264_PROP_GLOBAL_HEADER    "global-header"
#define OGMRIP_X264_PROP_KEYINT           "keyint"
#define OGMRIP_X264_PROP_LEVEL_IDC        "level-idc"
#define OGMRIP_X264_PROP_MAX_BFRAMES      "max-bframes"
#define OGMRIP_X264_PROP_ME               "me"
#define OGMRIP_X264_PROP_MERANGE          "merange"
#define OGMRIP_X264_PROP_MIXED_REFS       "mixed-refs"
#define OGMRIP_X264_PROP_PARTITIONS       "partitionv4mvs"
#define OGMRIP_X264_PROP_PSY_RD           "psy-rd"
#define OGMRIP_X264_PROP_PSY_TRELLIS      "psy-trellis"
#define OGMRIP_X264_PROP_RC_LOOKAHEAD     "rc-lookahead"
#define OGMRIP_X264_PROP_SUBQ             "subq"
#define OGMRIP_X264_PROP_TRELLIS          "trellis"
#define OGMRIP_X264_PROP_VBV_BUFSIZE      "vbv-bufsize"
#define OGMRIP_X264_PROP_VBV_MAXRATE      "vbv-maxrate"
#define OGMRIP_X264_PROP_WEIGHT_B         "weight-b"
#define OGMRIP_X264_PROP_WEIGHT_P         "weight-p"

#define OGMRIP_X264_DEFAULT_8X8DCT        TRUE
#define OGMRIP_X264_DEFAULT_AUD           FALSE
#define OGMRIP_X264_DEFAULT_B_ADAPT       1
#define OGMRIP_X264_DEFAULT_B_PYRAMID     2
#define OGMRIP_X264_DEFAULT_BRDO          FALSE
#define OGMRIP_X264_DEFAULT_CABAC         TRUE
#define OGMRIP_X264_DEFAULT_CQM           0
#define OGMRIP_X264_DEFAULT_DIRECT        3
#define OGMRIP_X264_DEFAULT_FRAMEREF      3
#define OGMRIP_X264_DEFAULT_GLOBAL_HEADER FALSE
#define OGMRIP_X264_DEFAULT_KEYINT        250
#define OGMRIP_X264_DEFAULT_LEVEL_IDC     51
#define OGMRIP_X264_DEFAULT_MAX_BFRAMES   3
#define OGMRIP_X264_DEFAULT_ME            2
#define OGMRIP_X264_DEFAULT_MERANGE       16
#define OGMRIP_X264_DEFAULT_MIXED_REFS    TRUE
#define OGMRIP_X264_DEFAULT_PARTITIONS    "'p8x8,b8x8,i8x8,i4x4"
#define OGMRIP_X264_DEFAULT_PSY_RD        1
#define OGMRIP_X264_DEFAULT_PSY_TRELLIS   0.15
#define OGMRIP_X264_DEFAULT_RC_LOOKAHEAD  40
#define OGMRIP_X264_DEFAULT_SUBQ          7
#define OGMRIP_X264_DEFAULT_TRELLIS       1
#define OGMRIP_X264_DEFAULT_VBV_BUFSIZE   0
#define OGMRIP_X264_DEFAULT_VBV_MAXRATE   0
#define OGMRIP_X264_DEFAULT_WEIGHT_B      TRUE
#define OGMRIP_X264_DEFAULT_WEIGHT_P      2

#define OGMRIP_TYPE_X264 (ogmrip_x264_get_type ())

GType ogmrip_x264_get_type (void);

G_END_DECLS

#endif /* __OGMRIP_X264_H__ */

