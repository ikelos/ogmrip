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

#ifndef __OGMRIP_ENCODING_H__
#define __OGMRIP_ENCODING_H__

#include <ogmrip-base.h>

#include <ogmrip-container.h>
#include <ogmrip-video-codec.h>
#include <ogmrip-audio-codec.h>
#include <ogmrip-subp-codec.h>
#include <ogmrip-chapters.h>
#include <ogmrip-file.h>

G_BEGIN_DECLS

#define OGMRIP_ENCODING_ERROR ogmrip_encoding_error_quark ()

typedef enum
{
  OGMRIP_ENCODING_ERROR_INVALID,
  OGMRIP_ENCODING_ERROR_VIDEO,
  OGMRIP_ENCODING_ERROR_AUDIO,
  OGMRIP_ENCODING_ERROR_SUBP
} OGMRipEncodingError;

GQuark ogmrip_encoding_error_quark (void);

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

typedef enum
{
  OGMRIP_ENCODING_NOT_RUN,
  OGMRIP_ENCODING_FAILURE,
  OGMRIP_ENCODING_CANCELLED,
  OGMRIP_ENCODING_SUCCESS
} OGMRipEncodingStatus;

typedef struct _OGMRipEncoding      OGMRipEncoding;
typedef struct _OGMRipEncodingPriv  OGMRipEncodingPrivate;
typedef struct _OGMRipEncodingClass OGMRipEncodingClass;

struct _OGMRipEncoding
{
  GObject parent_instance;

  OGMRipEncodingPrivate *priv;
};

struct _OGMRipEncodingClass
{
  GObjectClass parent_class;

  void (* run)      (OGMRipEncoding       *encoding,
                     OGMJobTask           *task);
  void (* progress) (OGMRipEncoding       *encoding,
                     OGMJobTask           *task,
                     gdouble              fraction);
  void (* complete) (OGMRipEncoding       *encoding,
                     OGMJobTask           *task,
                     OGMRipEncodingStatus status);
};

GType              ogmrip_encoding_get_type            (void);
OGMRipEncoding *   ogmrip_encoding_new                 (OGMRipTitle          *title);
OGMRipEncoding *   ogmrip_encoding_new_from_file       (GFile                *file,
                                                        GError               **error);
OGMRipEncoding *   ogmrip_encoding_new_from_xml        (OGMRipXML            *xml,
                                                        GError               **error);
gboolean           ogmrip_encoding_export_to_file      (OGMRipEncoding       *encoding,
                                                        GFile                *file,
                                                        GError               **error);
gboolean           ogmrip_encoding_export_to_xml       (OGMRipEncoding       *encoding,
                                                        OGMRipXML            *xml,
                                                        GError               **error);
void               ogmrip_encoding_clear               (OGMRipEncoding       *encoding);
OGMRipCodec *      ogmrip_encoding_get_video_codec     (OGMRipEncoding       *encoding);
gboolean           ogmrip_encoding_set_video_codec     (OGMRipEncoding       *encoding,
                                                        OGMRipVideoCodec     *codec,
                                                        GError               **error);
gboolean           ogmrip_encoding_add_audio_codec     (OGMRipEncoding       *encoding,
                                                        OGMRipAudioCodec     *codec,
                                                        GError               **error);
void               ogmrip_encoding_remove_audio_codec  (OGMRipEncoding       *encoding,
                                                        OGMRipAudioCodec     *codec);
void               ogmrip_encoding_clear_audio_codecs  (OGMRipEncoding       *encoding);
OGMRipCodec *      ogmrip_encoding_get_nth_audio_codec (OGMRipEncoding       *encoding,
                                                        gint                 n);
gint               ogmrip_encoding_get_n_audio_codecs  (OGMRipEncoding       *encoding);
GList *            ogmrip_encoding_get_audio_codecs    (OGMRipEncoding       *encoding);
gboolean           ogmrip_encoding_add_subp_codec      (OGMRipEncoding       *encoding,
                                                        OGMRipSubpCodec      *codec,
                                                        GError               **error);
void               ogmrip_encoding_remove_subp_codec   (OGMRipEncoding       *encoding,
                                                        OGMRipSubpCodec      *codec);
void               ogmrip_encoding_clear_subp_codecs   (OGMRipEncoding       *encoding);
OGMRipCodec *      ogmrip_encoding_get_nth_subp_codec  (OGMRipEncoding       *encoding,
                                                        gint                 n);
gint               ogmrip_encoding_get_n_subp_codecs   (OGMRipEncoding       *encoding);
GList *            ogmrip_encoding_get_subp_codecs     (OGMRipEncoding       *encoding);
void               ogmrip_encoding_add_chapters        (OGMRipEncoding       *encoding,
                                                        OGMRipChapters       *chapters);
void               ogmrip_encoding_remove_chapters     (OGMRipEncoding       *encoding,
                                                        OGMRipChapters       *chapters);
void               ogmrip_encoding_clear_chapters      (OGMRipEncoding       *encoding);
GList *            ogmrip_encoding_get_chapters        (OGMRipEncoding       *encoding);
gboolean           ogmrip_encoding_add_file            (OGMRipEncoding       *encoding,
                                                        OGMRipFile           *file,
                                                        GError               **error);
void               ogmrip_encoding_remove_file         (OGMRipEncoding       *encoding,
                                                        OGMRipFile           *file);
void               ogmrip_encoding_clear_files         (OGMRipEncoding       *encoding);
GList *            ogmrip_encoding_get_files           (OGMRipEncoding       *encoding);
gboolean           ogmrip_encoding_get_autocrop        (OGMRipEncoding       *encoding);
void               ogmrip_encoding_set_autocrop        (OGMRipEncoding       *encoding,
                                                        gboolean             autocrop);
gboolean           ogmrip_encoding_get_autoscale       (OGMRipEncoding       *encoding);
void               ogmrip_encoding_set_autoscale       (OGMRipEncoding       *encoding,
                                                        gboolean             autoscale);
OGMRipContainer *  ogmrip_encoding_get_container       (OGMRipEncoding       *encoding);
gboolean           ogmrip_encoding_set_container       (OGMRipEncoding       *encoding,
                                                        OGMRipContainer      *container,
                                                        GError               **error);
gboolean           ogmrip_encoding_get_copy            (OGMRipEncoding       *encoding);
void               ogmrip_encoding_set_copy            (OGMRipEncoding       *encoding,
                                                        gboolean             copy);
gboolean           ogmrip_encoding_get_ensure_sync     (OGMRipEncoding       *encoding);
void               ogmrip_encoding_set_ensure_sync     (OGMRipEncoding       *encoding,
                                                        gboolean             ensure_sync);
const gchar *      ogmrip_encoding_get_log_file        (OGMRipEncoding       *encoding);
void               ogmrip_encoding_set_log_file        (OGMRipEncoding       *encoding,
                                                        const gchar          *filename);
gint               ogmrip_encoding_get_method          (OGMRipEncoding       *encoding);
void               ogmrip_encoding_set_method          (OGMRipEncoding       *encoding,
                                                        OGMRipEncodingMethod method);
OGMRipProfile *    ogmrip_encoding_get_profile         (OGMRipEncoding       *encoding);
void               ogmrip_encoding_set_profile         (OGMRipEncoding       *encoding,
                                                        OGMRipProfile        *profile);
gboolean           ogmrip_encoding_get_test            (OGMRipEncoding       *encoding);
void               ogmrip_encoding_set_test            (OGMRipEncoding       *encoding,
                                                        gboolean             test);
OGMRipTitle *      ogmrip_encoding_get_title           (OGMRipEncoding       *encoding);
gboolean           ogmrip_encoding_get_relative        (OGMRipEncoding       *encoding);
void               ogmrip_encoding_set_relative        (OGMRipEncoding       *encoding,
                                                        gboolean             relative);
gint               ogmrip_encoding_autobitrate         (OGMRipEncoding       *encoding);
void               ogmrip_encoding_autoscale           (OGMRipEncoding       *encoding,
                                                        gdouble              bpp,
                                                        guint                *width,
                                                        guint                *height);
gboolean           ogmrip_encoding_test                (OGMRipEncoding       *encoding,
                                                        GCancellable         *cancellable,
                                                        GError               **error);
gboolean           ogmrip_encoding_encode              (OGMRipEncoding       *encoding,
                                                        GCancellable         *cancellable,
                                                        GError               **error);
void               ogmrip_encoding_suspend             (OGMRipEncoding       *encoding);
void               ogmrip_encoding_resume              (OGMRipEncoding       *encoding);
void               ogmrip_encoding_clean               (OGMRipEncoding       *encoding,
                                                        gboolean             temporary,
                                                        gboolean             copy,
                                                        gboolean             log);

G_END_DECLS

#endif /* __OGMRIP_ENCODING_H__ */

