/* OGMRipXviD - An XviD plugin for OGMRip
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

#ifndef __OGMRIP_XVID_H__
#define __OGMRIP_XVID_H__

#include <ogmrip-encode.h>

G_BEGIN_DECLS

#define OGMRIP_XVID_PROP_BQUANT_OFFSET       "bquant-offset"
#define OGMRIP_XVID_PROP_BQUANT_RATIO        "bquant-ratio"
#define OGMRIP_XVID_PROP_BVHQ                "bvhq"
#define OGMRIP_XVID_PROP_CARTOON             "cartoon"
#define OGMRIP_XVID_PROP_CHROMA_ME           "chroma-me"
#define OGMRIP_XVID_PROP_CHROMA_OPT          "chroma-opt"
#define OGMRIP_XVID_PROP_CLOSED_GOP          "closed-gop"
#define OGMRIP_XVID_PROP_FRAME_DROP_RATIO    "frame-drop-ratio"
#define OGMRIP_XVID_PROP_GMC                 "gmc"
#define OGMRIP_XVID_PROP_GRAYSCALE           "grayscale"
#define OGMRIP_XVID_PROP_INTERLACING         "interlacing"
#define OGMRIP_XVID_PROP_LUMI_MASK           "lumi-mask"
#define OGMRIP_XVID_PROP_MAX_BFRAMES         "max-bframes"
#define OGMRIP_XVID_PROP_MAX_BQUANT          "max-bquant"
#define OGMRIP_XVID_PROP_MAX_IQUANT          "max-iquant"
#define OGMRIP_XVID_PROP_MAX_PQUANT          "max-pquant"
#define OGMRIP_XVID_PROP_ME_QUALITY          "me-quality"
#define OGMRIP_XVID_PROP_MIN_BQUANT          "min-bquant"
#define OGMRIP_XVID_PROP_MIN_IQUANT          "min-iquant"
#define OGMRIP_XVID_PROP_MIN_PQUANT          "min-pquant"
#define OGMRIP_XVID_PROP_MAX_KEYINT          "max-key-interval"
#define OGMRIP_XVID_PROP_PACKED              "packed"
#define OGMRIP_XVID_PROP_PAR_HEIGHT          "par-height"
#define OGMRIP_XVID_PROP_PAR                 "par"
#define OGMRIP_XVID_PROP_PAR_WIDTH           "par-width"
#define OGMRIP_XVID_PROP_PROFILE             "profile"
#define OGMRIP_XVID_PROP_QUANT_TYPE          "quant-type"
#define OGMRIP_XVID_PROP_QPEL                "qpel"
#define OGMRIP_XVID_PROP_TRELLIS             "trellis"
#define OGMRIP_XVID_PROP_VHQ                 "vhq"

#define OGMRIP_XVID_DEFAULT_BQUANT_OFFSET    100
#define OGMRIP_XVID_DEFAULT_BQUANT_RATIO     150
#define OGMRIP_XVID_DEFAULT_BVHQ             1
#define OGMRIP_XVID_DEFAULT_CARTOON          FALSE
#define OGMRIP_XVID_DEFAULT_CHROMA_ME        TRUE
#define OGMRIP_XVID_DEFAULT_CHROMA_OPT       TRUE
#define OGMRIP_XVID_DEFAULT_CLOSED_GOP       TRUE
#define OGMRIP_XVID_DEFAULT_FRAME_DROP_RATIO 0
#define OGMRIP_XVID_DEFAULT_GMC              FALSE
#define OGMRIP_XVID_DEFAULT_GRAYSCALE        FALSE
#define OGMRIP_XVID_DEFAULT_INTERLACING      FALSE
#define OGMRIP_XVID_DEFAULT_LUMI_MASK        TRUE
#define OGMRIP_XVID_DEFAULT_MAX_BFRAMES      2
#define OGMRIP_XVID_DEFAULT_MAX_BQUANT       31
#define OGMRIP_XVID_DEFAULT_MAX_IQUANT       31
#define OGMRIP_XVID_DEFAULT_MAX_PQUANT       31
#define OGMRIP_XVID_DEFAULT_ME_QUALITY       6
#define OGMRIP_XVID_DEFAULT_MIN_BQUANT       2
#define OGMRIP_XVID_DEFAULT_MIN_IQUANT       2
#define OGMRIP_XVID_DEFAULT_MIN_PQUANT       2
#define OGMRIP_XVID_DEFAULT_MAX_KEYINT       250
#define OGMRIP_XVID_DEFAULT_PACKED           FALSE
#define OGMRIP_XVID_DEFAULT_PAR              0
#define OGMRIP_XVID_DEFAULT_PAR_HEIGHT       1
#define OGMRIP_XVID_DEFAULT_PAR_WIDTH        1
#define OGMRIP_XVID_DEFAULT_PROFILE          0
#define OGMRIP_XVID_DEFAULT_QUANT_TYPE       0
#define OGMRIP_XVID_DEFAULT_QPEL             FALSE
#define OGMRIP_XVID_DEFAULT_TRELLIS          TRUE
#define OGMRIP_XVID_DEFAULT_VHQ              1

#define OGMRIP_TYPE_XVID (ogmrip_xvid_get_type ())

GType ogmrip_xvid_get_type (void);

G_END_DECLS

#endif /* __OGMRIP_XVID_H__ */

