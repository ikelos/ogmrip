/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __OGMRIP_ENCODING_H__
#define __OGMRIP_ENCODING_H__

#include <ogmrip-container.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_ENCODING           (ogmrip_encoding_get_type ())
#define OGMRIP_ENCODING(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_ENCODING, OGMRipEncoding))
#define OGMRIP_ENCODING_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_ENCODING, OGMRipEncodingClass))
#define OGMRIP_IS_ENCODING(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_ENCODING))
#define OGMRIP_IS_ENCODING_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_ENCODING))
#define OGMRIP_ENCODING_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_ENCODING, OGMRipEncodingClass))

/**
 * OGMRipOptionsType:
 * @OGMRIP_OPTIONS_NONE: The option is disabled
 * @OGMRIP_OPTIONS_AUTOMATIC: The option will be automatically determined
 * @OGMRIP_OPTIONS_MANUAL: The option has been manually set
 *
 * How options are set.
 */
typedef enum
{
  OGMRIP_OPTIONS_NONE,
  OGMRIP_OPTIONS_AUTOMATIC,
  OGMRIP_OPTIONS_MANUAL
} OGMRipOptionsType;

/**
 * OGMRipEncodingMethod:
 * @OGMRIP_ENCODING_SIZE: Encoding with output size
 * @OGMRIP_ENCODING_BITRATE: Encoding with constant bitrate
 * @OGMRIP_ENCODING_QUANTIZER: Encoding with constant quantizer
 *
 * The encoding methods.
 */
typedef enum
{
  OGMRIP_ENCODING_SIZE,
  OGMRIP_ENCODING_BITRATE,
  OGMRIP_ENCODING_QUANTIZER
} OGMRipEncodingMethod;

typedef struct _OGMRipEncoding      OGMRipEncoding;
typedef struct _OGMRipEncodingPriv  OGMRipEncodingPriv;
typedef struct _OGMRipEncodingClass OGMRipEncodingClass;

struct _OGMRipEncoding
{
  GObject parent_instance;

  OGMRipEncodingPriv *priv;
};

struct _OGMRipEncodingClass
{
  GObjectClass parent_class;

  void (* run)      (OGMRipEncoding *encoding,
                     OGMJobSpawn    *spawn);
  void (* progress) (OGMRipEncoding *encoding,
                     OGMJobSpawn    *spawn,
                     gdouble        fraction);
  void (* complete) (OGMRipEncoding *encoding,
                     OGMJobSpawn    *spawn,
                     guint          result);
};

GType              ogmrip_encoding_get_type            (void);
OGMRipEncoding *   ogmrip_encoding_new                 (OGMDvdTitle      *title);
OGMRipEncoding *   ogmrip_encoding_new_from_file       (GFile            *file,
                                                        GError           **error);
gboolean           ogmrip_encoding_export              (OGMRipEncoding   *encoding,
                                                        GFile            *file,
                                                        GError           **error);
OGMRipCodec *      ogmrip_encoding_get_video_codec     (OGMRipEncoding   *encoding);
void               ogmrip_encoding_set_video_codec     (OGMRipEncoding   *encoding,
                                                        OGMRipVideoCodec *codec);
void               ogmrip_encoding_add_audio_codec     (OGMRipEncoding   *encoding,
                                                        OGMRipAudioCodec *codec);
OGMRipCodec *      ogmrip_encoding_get_nth_audio_codec (OGMRipEncoding   *encoding,
                                                        gint             n);
GList *            ogmrip_encoding_get_audio_codecs    (OGMRipEncoding   *encoding);
void               ogmrip_encoding_add_subp_codec      (OGMRipEncoding   *encoding,
                                                        OGMRipSubpCodec  *codec);
OGMRipCodec *      ogmrip_encoding_get_nth_subp_codec  (OGMRipEncoding   *encoding,
                                                        gint             n);
GList *            ogmrip_encoding_get_subp_codecs     (OGMRipEncoding   *encoding);
void               ogmrip_encoding_add_chapters        (OGMRipEncoding   *encoding,
                                                        OGMRipChapters   *chapters);
GList *            ogmrip_encoding_get_chapters        (OGMRipEncoding   *encoding);
void               ogmrip_encoding_add_file            (OGMRipEncoding   *encoding,
                                                        OGMRipFile       *file);
GList *            ogmrip_encoding_get_files           (OGMRipEncoding   *encoding);
gboolean           ogmrip_encoding_get_autocrop        (OGMRipEncoding   *encoding);
void               ogmrip_encoding_set_autocrop        (OGMRipEncoding   *encoding,
                                                        gboolean         autocrop);
gboolean           ogmrip_encoding_get_autoscale       (OGMRipEncoding   *encoding);
void               ogmrip_encoding_set_autoscale       (OGMRipEncoding   *encoding,
                                                        gboolean         autoscale);
OGMRipContainer *  ogmrip_encoding_get_container       (OGMRipEncoding   *encoding);
void               ogmrip_encoding_set_container       (OGMRipEncoding   *encoding,
                                                        OGMRipContainer  *container);
const gchar *      ogmrip_encoding_get_log_file        (OGMRipEncoding   *encoding);
void               ogmrip_encoding_set_log_file        (OGMRipEncoding   *encoding,
                                                        const gchar      *filename);
OGMRipProfile *    ogmrip_encoding_get_profile         (OGMRipEncoding   *encoding);
void               ogmrip_encoding_set_profile         (OGMRipEncoding   *encoding,
                                                        OGMRipProfile    *profile);
gboolean           ogmrip_encoding_get_test            (OGMRipEncoding   *encoding);
void               ogmrip_encoding_set_test            (OGMRipEncoding   *encoding,
                                                        gboolean         test);
OGMDvdTitle *      ogmrip_encoding_get_title           (OGMRipEncoding   *encoding);
gboolean           ogmrip_encoding_get_relative        (OGMRipEncoding   *encoding);
void               ogmrip_encoding_set_relative        (OGMRipEncoding   *encoding,
                                                        gboolean         relative);
gint               ogmrip_encoding_autobitrate         (OGMRipEncoding   *encoding);
void               ogmrip_encoding_autoscale           (OGMRipEncoding   *encoding,
                                                        gdouble          bpp,
                                                        guint            *width,
                                                        guint            *height);
gint               ogmrip_encoding_encode              (OGMRipEncoding   *encoding,
                                                        GError           **error);

G_END_DECLS

#endif /* __OGMRIP_ENCODING_H__ */

