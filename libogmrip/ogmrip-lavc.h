/* OGMRip - A library for DVD ripping and encoding
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

#ifndef __OGMRIP_LAVC_H__
#define __OGMRIP_LAVC_H__

#include <ogmrip-video-codec.h>

G_BEGIN_DECLS

#define OGMRIP_LAVC_PROP_BUF_SIZE    "buf-size"
#define OGMRIP_LAVC_PROP_CMP         "cmp"
#define OGMRIP_LAVC_PROP_DC          "dc"
#define OGMRIP_LAVC_PROP_DIA         "dia"
#define OGMRIP_LAVC_PROP_GRAYSCALE   "grayscale"
#define OGMRIP_LAVC_PROP_HEADER      "header"
#define OGMRIP_LAVC_PROP_KEYINT      "keyint"
#define OGMRIP_LAVC_PROP_LAST_PRED   "last-pred"
#define OGMRIP_LAVC_PROP_MAX_RATE    "max-rate"
#define OGMRIP_LAVC_PROP_MBD         "mbd"
#define OGMRIP_LAVC_PROP_MIN_RATE    "min-rate"
#define OGMRIP_LAVC_PROP_MV0         "mv0"
#define OGMRIP_LAVC_PROP_PRECMP      "precmp"
#define OGMRIP_LAVC_PROP_PREDIA      "predia"
#define OGMRIP_LAVC_PROP_PREME       "preme"
#define OGMRIP_LAVC_PROP_QNS         "qns"
#define OGMRIP_LAVC_PROP_QPEL        "qpel"
#define OGMRIP_LAVC_PROP_STRICT      "strict"
#define OGMRIP_LAVC_PROP_SUBCMP      "subcmp"
#define OGMRIP_LAVC_PROP_TRELLIS     "trellis"
#define OGMRIP_LAVC_PROP_V4MV        "v4mv"
#define OGMRIP_LAVC_PROP_VB_STRATEGY "vb-strategy"
#define OGMRIP_LAVC_PROP_VQCOMP      "vqcomp"

#define OGMRIP_TYPE_LAVC           (ogmrip_lavc_get_type ())
#define OGMRIP_LAVC(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_LAVC, OGMRipLavc))
#define OGMRIP_LAVC_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_LAVC, OGMRipLavcClass))
#define OGMRIP_IS_LAVC(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_LAVC))
#define OGMRIP_IS_LAVC_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_LAVC))
#define OGMRIP_LAVC_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_LAVC, OGMRipLavcClass))

typedef struct _OGMRipLavc      OGMRipLavc;
typedef struct _OGMRipLavcPriv  OGMRipLavcPriv;
typedef struct _OGMRipLavcClass OGMRipLavcClass;

struct _OGMRipLavc
{
  OGMRipVideoCodec parent_instance;

  OGMRipLavcPriv *priv;
};

struct _OGMRipLavcClass
{
  OGMRipVideoCodecClass parent_class;

  /* vtable */
  const gchar * (* get_codec) (void);
};

/**
 * OGMRipLavcHeaderType:
 * @OGMRIP_LAVC_HEADER_AUTO: Codec decides where to write global headers.
 * @OGMRIP_LAVC_HEADER_EXTRADATA: Write global headers only in extradata.
 * @OGMRIP_LAVC_HEADER_KEYFRAMES: Write global headers only in front of keyframes.
 * @OGMRIP_LAVC_HEADER_COMBINE: Combine @OGMRIP_LAVC_HEADER_EXTRADATA and @OGMRIP_LAVC_HEADER_KEYFRAMES.
 *
 * Controls writing global video headers.
 */
typedef enum
{
  OGMRIP_LAVC_HEADER_AUTO,
  OGMRIP_LAVC_HEADER_EXTRADATA,
  OGMRIP_LAVC_HEADER_KEYFRAMES,
  OGMRIP_LAVC_HEADER_COMBINE
} OGMRipLavcHeaderType;

void          ogmrip_init_lavc_plugin     (void);

GType         ogmrip_lavc_get_type        (void);

void          ogmrip_lavc_set_cmp         (OGMRipLavc           *lavc,
                                           guint                cmp,
                                           guint                precmp,
                                           guint                subcmp);
void          ogmrip_lavc_get_cmp         (OGMRipLavc           *lavc,
                                           guint                *cmp,
                                           guint                *precmp,
                                           guint                *subcmp);
void          ogmrip_lavc_set_dia         (OGMRipLavc           *lavc,
                                           gint                 dia,
                                           gint                 predia);
void          ogmrip_lavc_get_dia         (OGMRipLavc           *lavc,
                                           gint                 *dia,
                                           gint                 *predia);
void          ogmrip_lavc_set_keyint      (OGMRipLavc           *lavc,
                                           guint                keyint);
gint          ogmrip_lavc_get_keyint      (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_buf_size    (OGMRipLavc           *lavc,
                                           guint                buf_size);
gint          ogmrip_lavc_get_buf_size    (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_min_rate    (OGMRipLavc           *lavc,
                                           guint                min_rate);
gint          ogmrip_lavc_get_min_rate    (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_max_rate    (OGMRipLavc           *lavc,
                                           guint                max_rate);
gint          ogmrip_lavc_get_max_rate    (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_strict      (OGMRipLavc           *lavc,
                                           guint                strict);
gint          ogmrip_lavc_get_strict      (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_dc          (OGMRipLavc           *lavc,
                                           guint                dc);
gint          ogmrip_lavc_get_dc          (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_header      (OGMRipLavc           *lavc,
                                           OGMRipLavcHeaderType header);
gint          ogmrip_lavc_get_header      (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_mbd         (OGMRipLavc           *lavc,
                                           guint                mbd);
gint          ogmrip_lavc_get_mbd         (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_qns         (OGMRipLavc           *lavc,
                                           guint                qns);
gint          ogmrip_lavc_get_qns         (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_vb_strategy (OGMRipLavc           *lavc,
                                           guint                vb_strategy);
gint          ogmrip_lavc_get_vb_strategy (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_last_pred   (OGMRipLavc           *lavc,
                                           guint                last_pred);
gint          ogmrip_lavc_get_last_pred   (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_preme       (OGMRipLavc           *lavc,
                                           guint                preme);
gint          ogmrip_lavc_get_preme       (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_vqcomp      (OGMRipLavc           *lavc,
                                           gdouble              vqcomp);
gdouble       ogmrip_lavc_get_vqcomp      (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_mv0         (OGMRipLavc           *lavc,
                                           gboolean             mv0);
gboolean      ogmrip_lavc_get_mv0         (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_grayscale   (OGMRipLavc           *lavc,
                                           gboolean             grayscale);
gboolean      ogmrip_lavc_get_grayscale   (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_qpel        (OGMRipLavc           *lavc,
                                           gboolean             qpel);
gboolean      ogmrip_lavc_get_qpel        (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_trellis     (OGMRipLavc           *lavc,
                                           gboolean             trellis);
gboolean      ogmrip_lavc_get_trellis     (OGMRipLavc           *lavc);
void          ogmrip_lavc_set_v4mv        (OGMRipLavc           *lavc,
                                           gboolean             v4mv);
gboolean      ogmrip_lavc_get_v4mv        (OGMRipLavc           *lavc);

G_END_DECLS

#endif /* __OGMRIP_LAVC_H__ */

