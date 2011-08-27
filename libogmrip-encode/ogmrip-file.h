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

#ifndef __OGMRIP_FILE_H__
#define __OGMRIP_FILE_H__

#include <glib.h>

G_BEGIN_DECLS

#define OGMRIP_FILE_ERROR ogmrip_file_error_quark ()

/**
 * OGMRipFileError:
 * @OGMRIP_FILE_ERROR_UNKNOWN: Unknown error
 * @OGMRIP_FILE_ERROR_RANGE: Range error
 * @OGMRIP_FILE_ERROR_BITRATE: Impossible to get bitrate
 * @OGMRIP_FILE_ERROR_RATE: Impossible to get rate
 * @OGMRIP_FILE_ERROR_LENGTH: Impossible to get length
 * @OGMRIP_FILE_ERROR_FORMAT: Impossible to get format
 * @OGMRIP_FILE_ERROR_WIDTH: Impossible to get width
 * @OGMRIP_FILE_ERROR_HEIGHT: Impossible to get height
 * @OGMRIP_FILE_ERROR_ASPECT: Impossible to get aspect
 * @OGMRIP_FILE_ERROR_FPS: Impossible to get fps
 *
 * Error codes returned by #OGMRipFile functions.
 */
typedef enum
{
  OGMRIP_FILE_ERROR_UNKNOWN,
  OGMRIP_FILE_ERROR_RANGE,
  OGMRIP_FILE_ERROR_BITRATE,
  OGMRIP_FILE_ERROR_RATE,
  OGMRIP_FILE_ERROR_LENGTH,
  OGMRIP_FILE_ERROR_FORMAT,
  OGMRIP_FILE_ERROR_WIDTH,
  OGMRIP_FILE_ERROR_HEIGHT,
  OGMRIP_FILE_ERROR_ASPECT,
  OGMRIP_FILE_ERROR_FPS
} OGMRipFileError;

/**
 * OGMRipFileType:
 * @OGMRIP_FILE_TYPE_VIDEO: The file contains a video stream
 * @OGMRIP_FILE_TYPE_AUDIO: The file contains an audio stream
 * @OGMRIP_FILE_TYPE_SUBP: The file contains a subtitle stream
 *
 * The stream type of the file.
 */
typedef enum
{
  OGMRIP_FILE_TYPE_VIDEO,
  OGMRIP_FILE_TYPE_AUDIO,
  OGMRIP_FILE_TYPE_SUBP
} OGMRipFileType;

#define OGMRIP_FILE(file)       ((OGMRipFile *) (file))
#define OGMRIP_VIDEO_FILE(file) ((OGMRipVideoFile *) (file))
#define OGMRIP_AUDIO_FILE(file) ((OGMRipAudioFile *) (file))
#define OGMRIP_SUBP_FILE(file)  ((OGMRipSubpFile *) (file))

typedef struct _OGMRipFile      OGMRipFile;
typedef struct _OGMRipVideoFile OGMRipVideoFile;
typedef struct _OGMRipAudioFile OGMRipAudioFile;
typedef struct _OGMRipSubpFile  OGMRipSubpFile;

GQuark        ogmrip_file_error_quark                 (void);

OGMRipFile *  ogmrip_file_ref                         (OGMRipFile      *file);
void          ogmrip_file_unref                       (OGMRipFile      *file);
void          ogmrip_file_set_unlink_on_unref         (OGMRipFile      *file,
                                                       gboolean        do_unlink);
gboolean      ogmrip_file_get_unlink_on_unref         (OGMRipFile      *file);
gint          ogmrip_file_get_type                    (OGMRipFile      *file);
gint          ogmrip_file_get_format                  (OGMRipFile      *file);
gint64        ogmrip_file_get_size                    (OGMRipFile      *file);
const gchar * ogmrip_file_get_filename                (OGMRipFile      *file);
void          ogmrip_file_set_language                (OGMRipFile      *file,
                                                       gint            lang);
gint          ogmrip_file_get_language                (OGMRipFile      *file);

OGMRipFile *  ogmrip_video_file_new                   (const gchar     *filename,
                                                       GError          **error);
gint          ogmrip_video_file_get_bitrate           (OGMRipVideoFile *video);
gdouble       ogmrip_video_file_get_length            (OGMRipVideoFile *video);
void          ogmrip_video_file_get_size              (OGMRipVideoFile *video,
                                                       guint           *width,
                                                       guint           *height);
gdouble       ogmrip_video_file_get_framerate         (OGMRipVideoFile *video);
gdouble       ogmrip_video_file_get_aspect_ratio      (OGMRipVideoFile *video);

OGMRipFile *  ogmrip_audio_file_new                   (const gchar     *filename,
                                                       GError          **error);
gint          ogmrip_audio_file_get_bitrate           (OGMRipAudioFile *audio);
gdouble       ogmrip_audio_file_get_length            (OGMRipAudioFile *audio);
gint          ogmrip_audio_file_get_sample_rate       (OGMRipAudioFile *audio);
gint          ogmrip_audio_file_get_samples_per_frame (OGMRipAudioFile *audio);
gint          ogmrip_audio_file_get_channels          (OGMRipAudioFile *audio);

OGMRipFile *  ogmrip_subp_file_new                    (const gchar     *filename,
                                                       GError          **error);
gint          ogmrip_subp_file_get_charset            (OGMRipSubpFile  *subp);

G_END_DECLS

#endif /* __OGMRIP_FILE_H__ */

