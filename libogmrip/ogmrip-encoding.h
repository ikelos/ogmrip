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

#include <ogmdvd.h>
#include <ogmjob.h>

#include <ogmrip-enums.h>
#include <ogmrip-file.h>
#include <ogmrip-options.h>
#include <ogmrip-profile.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_ENCODING          (ogmrip_encoding_get_type ())
#define OGMRIP_ENCODING(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_ENCODING, OGMRipEncoding))
#define OGMRIP_ENCODING_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_ENCODING, OGMRipEncodingClass))
#define OGMRIP_IS_ENCODING(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_ENCODING))
#define OGMRIP_IS_ENCODING_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_ENCODING))
#define OGMRIP_ENCODING_ERROR         (ogmrip_encoding_error_quark ())

/**
 * OGMRIP_ENCODING_IS_BACKUPED:
 * @enc: An #OGMRipEncoding
 *
 * Gets whether @encoding has already been backuped.
 *
 * @Returns: %TRUE if already backuped, %FALSE otherwise
 */
#define OGMRIP_ENCODING_IS_BACKUPED(enc)  ((ogmrip_encoding_get_flags (enc) & OGMRIP_ENCODING_BACKUPED) != 0)

/**
 * OGMRIP_ENCODING_IS_ANALYZED:
 * @enc: An #OGMRipEncoding
 *
 * Gets whether @encoding has already been analyzed.
 *
 * @Returns: %TRUE if already analyzed, %FALSE otherwise
 */
#define OGMRIP_ENCODING_IS_ANALYZED(enc)  ((ogmrip_encoding_get_flags (enc) & OGMRIP_ENCODING_ANALYZED) != 0)

/**
 * OGMRIP_ENCODING_IS_TESTED:
 * @enc: An #OGMRipEncoding
 *
 * Gets whether @encoding has already been tested.
 *
 * @Returns: %TRUE if already tested, %FALSE otherwise
 */
#define OGMRIP_ENCODING_IS_TESTED(enc)    ((ogmrip_encoding_get_flags (enc) & OGMRIP_ENCODING_TESTED) != 0)

/**
 * OGMRIP_ENCODING_IS_EXTRACTED:
 * @enc: An #OGMRipEncoding
 *
 * Gets whether @encoding has already been extracted.
 *
 * @Returns: %TRUE if already extracted, %FALSE otherwise
 */
#define OGMRIP_ENCODING_IS_EXTRACTED(enc) ((ogmrip_encoding_get_flags (enc) & OGMRIP_ENCODING_EXTRACTED) != 0)

/**
 * OGMRIP_ENCODING_IS_BACKUPING:
 * @enc: An #OGMRipEncoding
 *
 * Gets whether @encoding is being backuped.
 *
 * @Returns: %TRUE if backuping, %FALSE otherwise
 */
#define OGMRIP_ENCODING_IS_BACKUPING(enc)  ((ogmrip_encoding_get_flags (enc) & OGMRIP_ENCODING_BACKUPING) != 0)

/**
 * OGMRIP_ENCODING_IS_TESTING:
 * @enc: An #OGMRipEncoding
 *
 * Gets whether @encoding is being tested.
 *
 * @Returns: %TRUE if testing, %FALSE otherwise
 */
#define OGMRIP_ENCODING_IS_TESTING(enc)    ((ogmrip_encoding_get_flags (enc) & OGMRIP_ENCODING_TESTING) != 0)

/**
 * OGMRIP_ENCODING_IS_EXTRACTING:
 * @enc: An #OGMRipEncoding
 *
 * Gets whether @encoding is being extracted.
 *
 * @Returns: %TRUE if extracting, %FALSE otherwise
 */
#define OGMRIP_ENCODING_IS_EXTRACTING(enc) ((ogmrip_encoding_get_flags (enc) & OGMRIP_ENCODING_EXTRACTING) != 0)

/**
 * OGMRIP_ENCODING_IS_RUNNING:
 * @enc: An #OGMRipEncoding
 *
 * Gets whether @encoding is being backuped, tested, or extracted.
 *
 * @Returns: %TRUE if backuping, testing, or extracting, %FALSE otherwise
 */
#define OGMRIP_ENCODING_IS_RUNNING(enc)   ((ogmrip_encoding_get_flags (enc) & \
      (OGMRIP_ENCODING_BACKUPING | OGMRIP_ENCODING_TESTING | OGMRIP_ENCODING_EXTRACTING)) != 0)

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
 * OGMRipEncodingFlags:
 * @OGMRIP_ENCODING_BACKUPED: Whether the encoding has been backuped
 * @OGMRIP_ENCODING_ANALYZED: Whether the encoding has been analyzed
 * @OGMRIP_ENCODING_TESTED: Whether the encoding has been tested
 * @OGMRIP_ENCODING_EXTRACTED: Whether the encoding has been extracted
 * @OGMRIP_ENCODING_BACKUPING: If the encoding is being backuped
 * @OGMRIP_ENCODING_TESTING: If the encoding is being tested
 * @OGMRIP_ENCODING_EXTRACTING: If the encoding is being tested
 * @OGMRIP_ENCODING_CANCELING: If the encoding is being cancelled
 *
 * The encoding flags.
 */
typedef enum
{
  OGMRIP_ENCODING_BACKUPED   = 1 << 0,
  OGMRIP_ENCODING_ANALYZED   = 1 << 1,
  OGMRIP_ENCODING_TESTED     = 1 << 2,
  OGMRIP_ENCODING_EXTRACTED  = 1 << 3,
  OGMRIP_ENCODING_BACKUPING  = 1 << 4,
  OGMRIP_ENCODING_TESTING    = 1 << 5,
  OGMRIP_ENCODING_EXTRACTING = 1 << 6,
  OGMRIP_ENCODING_CANCELING  = 1 << 7
} OGMRipEncodingFlags;

/**
 * OGMRipEncodingError:
 * @OGMRIP_ENCODING_ERROR_CONTAINER: Container and codecs are not compatible
 * @OGMRIP_ENCODING_ERROR_STREAMS: A stream is not compatible
 * @OGMRIP_ENCODING_ERROR_SIZE: No enough disk space
 * @OGMRIP_ENCODING_ERROR_TEST: Cannot perform compressibility test
 * @OGMRIP_ENCODING_ERROR_IMPORT: Cannot import encoding file
 * @OGMRIP_ENCODING_ERROR_AUDIO: Cannot contain multiple audio streams
 * @OGMRIP_ENCODING_ERROR_SUBP: Cannot contain multiple subp streams
 * @OGMRIP_ENCODING_ERROR_UNKNOWN: Unknown error
 * @OGMRIP_ENCODING_ERROR_FATAL: Fatal error
 *
 * Error codes returned by ogmdvd_disc_open()
 */
typedef enum
{
  OGMRIP_ENCODING_ERROR_CONTAINER,
  OGMRIP_ENCODING_ERROR_STREAMS,
  OGMRIP_ENCODING_ERROR_SIZE,
  OGMRIP_ENCODING_ERROR_TEST,
  OGMRIP_ENCODING_ERROR_IMPORT,
  OGMRIP_ENCODING_ERROR_AUDIO,
  OGMRIP_ENCODING_ERROR_SUBP,
  OGMRIP_ENCODING_ERROR_UNKNOWN,
  OGMRIP_ENCODING_ERROR_FATAL
} OGMRipEncodingError;

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

/**
 * OGMRipTaskEvent:
 * @OGMRIP_TASK_RUN: When a task is run
 * @OGMRIP_TASK_PROGRESS: When a task has progressed
 * @OGMRIP_TASK_COMPLETE: When a task completes
 * @OGMRIP_TASK_SUSPEND: When a task is suspended
 * @OGMRIP_TASK_RESUME: When a task is resumed
 *
 * The events associated with encoding tasks.
 */
typedef enum
{
  OGMRIP_TASK_RUN,
  OGMRIP_TASK_PROGRESS,
  OGMRIP_TASK_COMPLETE,
  OGMRIP_TASK_SUSPEND,
  OGMRIP_TASK_RESUME
} OGMRipTaskEvent;

/**
 * OGMRipTaskType:
 * @OGMRIP_TASK_BACKUP: Backup task
 * @OGMRIP_TASK_ANALYZE: Analyze task
 * @OGMRIP_TASK_CHAPTERS: Chapters extraction task
 * @OGMRIP_TASK_AUDIO: Audio extraction task
 * @OGMRIP_TASK_SUBP: Subtitles extraction task
 * @OGMRIP_TASK_CROP: Automatic cropping task
 * @OGMRIP_TASK_TEST: Compressibility test task
 * @OGMRIP_TASK_VIDEO: Video extraction task
 * @OGMRIP_TASK_MERGE: Merge task
 *
 * The available tasks.
 */
typedef enum
{
  OGMRIP_TASK_BACKUP,
  OGMRIP_TASK_ANALYZE,
  OGMRIP_TASK_CHAPTERS,
  OGMRIP_TASK_AUDIO,
  OGMRIP_TASK_SUBP,
  OGMRIP_TASK_CROP,
  OGMRIP_TASK_TEST,
  OGMRIP_TASK_VIDEO,
  OGMRIP_TASK_MERGE
} OGMRipTaskType;

/**
 * OGMRipTaskDetail:
 * @fraction: The current fraction of the task that's been completed
 * @result: The result status of a completed task
 *
 * This structure contains either the fraction of the task that's been
 * completed is the task is running, or the result status if the task
 * is finished.
 */
typedef union
{
  gdouble          fraction;
  OGMJobResultType result;
} OGMRipTaskDetail;

/**
 * OGMRipEncodingTask:
 * @spawn: An #OGMJobSpawn
 * @options: An #OGMRipOptions
 * @type: An #OGMRipTaskType
 * @event: An #OGMRipTaskEvent
 * @detail: An #OGMRipTaskDetail
 *
 * This structure describes a running task.
 */
typedef struct
{
  OGMJobSpawn      *spawn;
  OGMRipOptions    *options;
  OGMRipTaskType   type;
  OGMRipTaskEvent  event;
  OGMRipTaskDetail detail;
} OGMRipEncodingTask;

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

  void (* run)      (OGMRipEncoding      *encoding);
  void (* complete) (OGMRipEncoding      *encoding,
                     OGMJobResultType    result);

  void (* task)     (OGMRipEncoding      *encoding,
                     OGMRipEncodingTask  *task);
};

/**
 * OGMRipEncodingAudioFunc:
 * @encoding: An #OGMRipEncoding
 * @stream: An #OGMDvdAudioStream
 * @options: An #OGMRipAudioOptions
 * @data: The user data
 *
 * Specifies the type of functions passed to ogmrip_encoding_foreach_audio_streams().
 */
typedef void (* OGMRipEncodingAudioFunc) (OGMRipEncoding     *encoding,
                                          OGMDvdAudioStream  *stream,
                                          OGMRipAudioOptions *options,
                                          gpointer           data);

/**
 * OGMRipEncodingSubpFunc:
 * @encoding: An #OGMRipEncoding
 * @stream: An #OGMDvdSubpStream
 * @options: An #OGMRipSubpOptions
 * @data: The user data
 *
 * Specifies the type of functions passed to ogmrip_encoding_foreach_subp_streams().
 */
typedef void (* OGMRipEncodingSubpFunc)  (OGMRipEncoding     *encoding,
                                          OGMDvdSubpStream   *stream,
                                          OGMRipSubpOptions  *options,
                                          gpointer           data);

/**
 * OGMRipEncodingFileFunc:
 * @encoding: An #OGMRipEncoding
 * @file: An #OGMRipFile
 * @data: The user data
 *
 * Specifies the type of functions passed to ogmrip_encoding_foreach_audio_files().
 * and ogmrip_encoding_foreach_subp_files().
 */
typedef void (* OGMRipEncodingFileFunc)  (OGMRipEncoding    *encoding,
                                          OGMRipFile        *file,
                                          gpointer          data);

/*
 * Encoding
 */

GQuark              ogmrip_encoding_error_quark            (void);
GType               ogmrip_encoding_get_type               (void);

OGMRipEncoding *    ogmrip_encoding_new                    (OGMDvdTitle             *title,
                                                            const gchar             *filename);
OGMRipEncoding *    ogmrip_encoding_new_from_file          (const gchar             *filename,
                                                            GError                  **error);
 
gboolean            ogmrip_encoding_dump                   (OGMRipEncoding          *encoding,
                                                            const gchar             *filename);

gboolean            ogmrip_encoding_equal                  (OGMRipEncoding          *encoding1,
                                                            OGMRipEncoding          *encoding2);

guint32             ogmrip_encoding_get_flags              (OGMRipEncoding          *encoding);

G_CONST_RETURN
gchar *             ogmrip_encoding_get_id                 (OGMRipEncoding          *encoding);
OGMDvdTitle *       ogmrip_encoding_get_title              (OGMRipEncoding          *encoding);

OGMRipProfile *     ogmrip_encoding_get_profile            (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_profile            (OGMRipEncoding          *encoding,
                                                            OGMRipProfile           *profile);

G_CONST_RETURN
gchar *             ogmrip_encoding_get_label              (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_label              (OGMRipEncoding          *encoding,
                                                            const gchar             *label);

void                ogmrip_encoding_get_chapters           (OGMRipEncoding          *encoding,
                                                            guint                   *start_chap,
                                                            gint                    *end_chap);
void                ogmrip_encoding_set_chapters           (OGMRipEncoding          *encoding,
                                                            guint                   start_chap,
                                                            gint                    end_chap);

gint                ogmrip_encoding_get_chapters_language  (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_chapters_language  (OGMRipEncoding          *encoding,
                                                            guint                   language);
G_CONST_RETURN
gchar *             ogmrip_encoding_get_chapter_label      (OGMRipEncoding          *encoding,
                                                            guint                   nr);
void                ogmrip_encoding_set_chapter_label      (OGMRipEncoding          *encoding,
                                                            guint                   nr,
                                                            const gchar             *label);

gboolean            ogmrip_encoding_get_copy_dvd           (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_copy_dvd           (OGMRipEncoding          *encoding,
                                                            gboolean                copy_dvd);

gboolean            ogmrip_encoding_get_ensure_sync        (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_ensure_sync        (OGMRipEncoding          *encoding,
                                                            gboolean                ensure_sync);

gboolean            ogmrip_encoding_get_keep_tmp_files     (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_keep_tmp_files     (OGMRipEncoding          *encoding,
                                                            gboolean                keep_tmp_files);
G_CONST_RETURN
gchar *             ogmrip_encoding_get_filename           (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_filename           (OGMRipEncoding          *encoding,
                                                            const gchar             *filename);
G_CONST_RETURN
gchar *             ogmrip_encoding_get_logfile            (OGMRipEncoding          *encoding);

gint                ogmrip_encoding_get_threads            (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_threads            (OGMRipEncoding          *encoding,
                                                            guint                   threads);

gint                ogmrip_encoding_get_angle              (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_angle              (OGMRipEncoding          *encoding,
                                                            guint                   angle);

GType               ogmrip_encoding_get_container_type     (OGMRipEncoding          *encoding);
gboolean            ogmrip_encoding_set_container_type     (OGMRipEncoding          *encoding,
                                                            GType                   type,
                                                            GError                  **error);
G_CONST_RETURN
gchar *             ogmrip_encoding_get_fourcc             (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_fourcc             (OGMRipEncoding          *encoding,
                                                            const gchar             *fourcc);

gint                ogmrip_encoding_get_method             (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_method             (OGMRipEncoding          *encoding,
                                                            OGMRipEncodingMethod    method);

gint                ogmrip_encoding_get_bitrate            (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_bitrate            (OGMRipEncoding          *encoding,
                                                            guint                   bitrate);

gint                ogmrip_encoding_get_target_number      (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_target_number      (OGMRipEncoding          *encoding,
                                                            guint                   target_number);

gint                ogmrip_encoding_get_target_size        (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_target_size        (OGMRipEncoding          *encoding,
                                                            guint                   target_size);

gdouble             ogmrip_encoding_get_quantizer          (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_quantizer          (OGMRipEncoding          *encoding,
                                                            gdouble                 quantizer);

GType               ogmrip_encoding_get_video_codec_type   (OGMRipEncoding          *encoding);
gboolean            ogmrip_encoding_set_video_codec_type   (OGMRipEncoding          *encoding,
                                                            GType                   type,
                                                            GError                  **error);

gboolean            ogmrip_encoding_get_can_crop           (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_can_crop           (OGMRipEncoding          *encoding,
                                                            gboolean                can_crop);

gboolean            ogmrip_encoding_get_can_scale          (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_can_scale          (OGMRipEncoding          *encoding,
                                                            gboolean                can_scale);

gboolean            ogmrip_encoding_get_deblock            (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_deblock            (OGMRipEncoding          *encoding,
                                                            gboolean                deblock);

gboolean            ogmrip_encoding_get_denoise            (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_denoise            (OGMRipEncoding          *encoding,
                                                            gboolean                denoise);

gboolean            ogmrip_encoding_get_dering             (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_dering             (OGMRipEncoding          *encoding,
                                                            gboolean                dering);

gboolean            ogmrip_encoding_get_turbo              (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_turbo              (OGMRipEncoding          *encoding,
                                                            gboolean                turbo);

void                ogmrip_encoding_get_max_size           (OGMRipEncoding          *encoding,
                                                            guint                   *width,
                                                            guint                   *height,
                                                            gboolean                *expand);
void                ogmrip_encoding_set_max_size           (OGMRipEncoding          *encoding,
                                                            guint                   width,
                                                            guint                   height,
                                                            gboolean                expand);

void                ogmrip_encoding_get_min_size           (OGMRipEncoding          *encoding,
                                                            guint                   *width,
                                                            guint                   *height);
void                ogmrip_encoding_set_min_size           (OGMRipEncoding          *encoding,
                                                            guint                   width,
                                                            guint                   height);

gint                ogmrip_encoding_get_passes             (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_passes             (OGMRipEncoding          *encoding,
                                                            guint                   passes);

gint                ogmrip_encoding_get_quality            (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_quality            (OGMRipEncoding          *encoding,
                                                            OGMRipVideoQuality      quality);

gint                ogmrip_encoding_get_scaler             (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_scaler             (OGMRipEncoding          *encoding,
                                                            OGMRipScalerType        scaler);

gdouble             ogmrip_encoding_get_bits_per_pixel     (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_bits_per_pixel     (OGMRipEncoding          *encoding,
                                                            gdouble                 bpp);

gboolean            ogmrip_encoding_get_relative           (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_relative           (OGMRipEncoding          *encoding,
                                                            gboolean                relative);

gint                ogmrip_encoding_get_deinterlacer       (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_deinterlacer       (OGMRipEncoding          *encoding,
                                                            OGMRipDeintType         deint);

void                ogmrip_encoding_get_aspect_ratio       (OGMRipEncoding          *encoding,
                                                            guint                   *numerator,
                                                            guint                   *denominator);
void                ogmrip_encoding_set_aspect_ratio       (OGMRipEncoding          *encoding,
                                                            guint                   numerator,
                                                            guint                   denominator);

gboolean            ogmrip_encoding_get_test               (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_test               (OGMRipEncoding          *encoding,
                                                            gboolean                test);

gboolean            ogmrip_encoding_get_auto_subp          (OGMRipEncoding          *encoding);
void                ogmrip_encoding_set_auto_subp          (OGMRipEncoding          *encoding,
                                                            gboolean                auto_subp);

gint                ogmrip_encoding_get_crop               (OGMRipEncoding          *encoding,
                                                            guint                   *x,
                                                            guint                   *y,
                                                            guint                   *w,
                                                            guint                   *h);
void                ogmrip_encoding_set_crop               (OGMRipEncoding          *encoding,
                                                            OGMRipOptionsType       type,
                                                            guint                   x,
                                                            guint                   y,
                                                            guint                   w,
                                                            guint                   h);

gint                ogmrip_encoding_get_scale              (OGMRipEncoding          *encoding,
                                                            guint                   *w,
                                                            guint                   *h);
void                ogmrip_encoding_set_scale              (OGMRipEncoding          *encoding,
                                                            OGMRipOptionsType       type,
                                                            guint                   w,
                                                            guint                   h);

gboolean            ogmrip_encoding_add_audio_stream       (OGMRipEncoding          *encoding,
                                                            OGMDvdAudioStream       *stream,
                                                            OGMRipAudioOptions      *options,
                                                            GError                  **error);
gint                ogmrip_encoding_get_n_audio_streams    (OGMRipEncoding          *encoding);
OGMDvdAudioStream * ogmrip_encoding_get_nth_audio_stream   (OGMRipEncoding          *encoding,
                                                            guint                   n);
void                ogmrip_encoding_foreach_audio_streams  (OGMRipEncoding          *encoding,
                                                            OGMRipEncodingAudioFunc func,
                                                            gpointer                data);

gboolean            ogmrip_encoding_get_nth_audio_options  (OGMRipEncoding          *encoding,
                                                            guint                   n,
                                                            OGMRipAudioOptions      *options);
gboolean            ogmrip_encoding_set_nth_audio_options  (OGMRipEncoding          *encoding,
                                                            guint                   n,
                                                            OGMRipAudioOptions      *options,
                                                            GError                  **error);

gboolean            ogmrip_encoding_add_subp_stream        (OGMRipEncoding          *encoding,
                                                            OGMDvdSubpStream        *stream,
                                                            OGMRipSubpOptions       *options,
                                                            GError                  **error);
gint                ogmrip_encoding_get_n_subp_streams     (OGMRipEncoding          *encoding);
OGMDvdSubpStream *  ogmrip_encoding_get_nth_subp_stream    (OGMRipEncoding          *encoding,
                                                            guint                   n);
void                ogmrip_encoding_foreach_subp_streams   (OGMRipEncoding          *encoding,
                                                            OGMRipEncodingSubpFunc  func,
                                                            gpointer                data);

gboolean            ogmrip_encoding_get_nth_subp_options   (OGMRipEncoding          *encoding,
                                                            guint                   n,
                                                            OGMRipSubpOptions       *options);
gboolean            ogmrip_encoding_set_nth_subp_options   (OGMRipEncoding          *encoding,
                                                            guint                   n,
                                                            OGMRipSubpOptions       *options,
                                                            GError                  **error);

gboolean            ogmrip_encoding_add_audio_file         (OGMRipEncoding          *encoding,
                                                            OGMRipFile              *file,
                                                            GError                  **error);
gint                ogmrip_encoding_get_n_audio_files      (OGMRipEncoding          *encoding);
OGMRipFile *        ogmrip_encoding_get_nth_audio_file     (OGMRipEncoding          *encoding,
                                                            guint                   n);
void                ogmrip_encoding_foreach_audio_files    (OGMRipEncoding          *encoding,
                                                            OGMRipEncodingFileFunc  func,
                                                            gpointer                data);

gboolean            ogmrip_encoding_add_subp_file          (OGMRipEncoding          *encoding,
                                                            OGMRipFile              *file,
                                                            GError                  **error);
gint                ogmrip_encoding_get_n_subp_files       (OGMRipEncoding          *encoding);
OGMRipFile *        ogmrip_encoding_get_nth_subp_file      (OGMRipEncoding          *encoding,
                                                            guint                   n);
void                ogmrip_encoding_foreach_subp_files     (OGMRipEncoding          *encoding,
                                                            OGMRipEncodingFileFunc  func,
                                                            gpointer                data);

gboolean            ogmrip_encoding_check_filename         (OGMRipEncoding          *encoding,
                                                            GError                  **error);

gint                ogmrip_encoding_extract                (OGMRipEncoding          *encoding,
                                                            GError                  **error);
gint                ogmrip_encoding_backup                 (OGMRipEncoding          *encoding,
                                                            GError                  **error);
gint                ogmrip_encoding_test                   (OGMRipEncoding          *encoding,
                                                            GError                  **error);

void                ogmrip_encoding_cleanup                (OGMRipEncoding          *encoding);
void                ogmrip_encoding_cancel                 (OGMRipEncoding          *encoding);
void                ogmrip_encoding_suspend                (OGMRipEncoding          *encoding);
void                ogmrip_encoding_resume                 (OGMRipEncoding          *encoding);

G_END_DECLS

#endif /* __OGMRIP_ENCODING_H__ */

