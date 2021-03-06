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

#ifndef __OGMRIP_CODEC_H__
#define __OGMRIP_CODEC_H__

#include <ogmrip-job.h>
#include <ogmrip-file.h>

G_BEGIN_DECLS

#define OGMRIP_CODEC_ERROR (ogmrip_codec_error_quark ())

typedef enum
{
  OGMRIP_CODEC_ERROR_DECODE
} OGMRipCodecError;

GQuark ogmrip_codec_error_quark (void);

#define OGMRIP_TYPE_CODEC           (ogmrip_codec_get_type ())
#define OGMRIP_CODEC(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_CODEC, OGMRipCodec))
#define OGMRIP_CODEC_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_CODEC, OGMRipCodecClass))
#define OGMRIP_IS_CODEC(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_CODEC))
#define OGMRIP_IS_CODEC_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_CODEC))
#define OGMRIP_CODEC_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_CODEC, OGMRipCodecClass))

typedef struct _OGMRipCodec      OGMRipCodec;
typedef struct _OGMRipCodecPriv  OGMRipCodecPrivate;
typedef struct _OGMRipCodecClass OGMRipCodecClass;

struct _OGMRipCodec
{
  OGMJobBin parent_instance;

  OGMRipCodecPrivate *priv;
};

struct _OGMRipCodecClass
{
  OGMJobBinClass parent_class;
};

GType          ogmrip_codec_get_type           (void);
OGMRipStream * ogmrip_codec_get_input          (OGMRipCodec   *codec);
OGMRipFile *   ogmrip_codec_get_output         (OGMRipCodec   *codec);
void           ogmrip_codec_set_output         (OGMRipCodec   *codec,
                                                OGMRipFile    *file);
gdouble        ogmrip_codec_get_length         (OGMRipCodec   *codec,
                                                OGMRipTime    *time_);
void           ogmrip_codec_get_chapters       (OGMRipCodec   *codec,
                                                guint         *start,
                                                guint         *end);
void           ogmrip_codec_set_chapters       (OGMRipCodec   *codec,
                                                guint         start,
                                                gint          end);
gdouble        ogmrip_codec_get_play_length    (OGMRipCodec   *codec);
void           ogmrip_codec_set_play_length    (OGMRipCodec   *codec,
                                                gdouble       length);
gdouble        ogmrip_codec_get_start_position (OGMRipCodec   *codec);
void           ogmrip_codec_set_start_position (OGMRipCodec   *codec,
                                                gdouble       start);
gboolean       ogmrip_codec_get_autoclean      (OGMRipCodec   *codec);
void           ogmrip_codec_set_autoclean      (OGMRipCodec   *codec,
                                                gboolean      autoclean);
void           ogmrip_codec_clean              (OGMRipCodec   *codec);

void           ogmrip_register_codec           (GType         gtype,
                                                const gchar   *name,
                                                const gchar   *description,
                                                OGMRipFormat  format,
                                                const gchar   *property,
                                                ...);
OGMRipFormat   ogmrip_codec_format             (GType         gtype);
gboolean       ogmrip_codec_get_schema         (GType         gtype,
                                                const gchar   **id,
                                                const gchar   **name);

G_END_DECLS

#endif /* __OGMRIP_CODEC_H__ */

