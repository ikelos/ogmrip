/* OGMRip - A library for media ripping and encoding
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

#ifndef __OGMRIP_SUBP_CODEC_H__
#define __OGMRIP_SUBP_CODEC_H__

#include <ogmrip-codec.h>
#include <ogmrip-enums.h>
#include <ogmrip-profile.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_SUBP_CODEC           (ogmrip_subp_codec_get_type ())
#define OGMRIP_SUBP_CODEC(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_SUBP_CODEC, OGMRipSubpCodec))
#define OGMRIP_SUBP_CODEC_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_SUBP_CODEC, OGMRipSubpCodecClass))
#define OGMRIP_IS_SUBP_CODEC(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_SUBP_CODEC))
#define OGMRIP_IS_SUBP_CODEC_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_SUBP_CODEC))
#define OGMRIP_SUBP_CODEC_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_SUBP_CODEC, OGMRipSubpCodecClass))

typedef struct _OGMRipSubpCodec      OGMRipSubpCodec;
typedef struct _OGMRipSubpCodecPriv  OGMRipSubpCodecPrivate;
typedef struct _OGMRipSubpCodecClass OGMRipSubpCodecClass;

struct _OGMRipSubpCodec
{
  OGMRipCodec parent_instance;

  OGMRipSubpCodecPrivate *priv;
};

struct _OGMRipSubpCodecClass
{
  OGMRipCodecClass parent_class;
};

GType         ogmrip_subp_codec_get_type         (void);
GType         ogmrip_subp_codec_get_default      (GType            container);
OGMRipCodec * ogmrip_subp_codec_new              (GType            type,
                                                  OGMRipSubpStream *stream);
OGMRipCodec * ogmrip_subp_codec_new_from_profile (OGMRipSubpStream *stream,
                                                  OGMRipProfile    *profile);
gint          ogmrip_subp_codec_get_charset      (OGMRipSubpCodec  *subp);
void          ogmrip_subp_codec_set_charset      (OGMRipSubpCodec  *subp,
                                                  OGMRipCharset    charset);
gboolean      ogmrip_subp_codec_get_forced_only  (OGMRipSubpCodec  *subp);
void          ogmrip_subp_codec_set_forced_only  (OGMRipSubpCodec  *subp,
                                                  gboolean         forced_only);
const gchar * ogmrip_subp_codec_get_label        (OGMRipSubpCodec  *codec);
void          ogmrip_subp_codec_set_label        (OGMRipSubpCodec  *codec,
                                                  const gchar      *label);
gint          ogmrip_subp_codec_get_language     (OGMRipSubpCodec  *subp);
void          ogmrip_subp_codec_set_language     (OGMRipSubpCodec  *subp,
                                                  guint            language);
gint          ogmrip_subp_codec_get_newline      (OGMRipSubpCodec  *subp);
void          ogmrip_subp_codec_set_newline      (OGMRipSubpCodec  *subp,
                                                  OGMRipNewline    newline);

G_END_DECLS

#endif /* __OGMRIP_SUBP_CODEC_H__ */

