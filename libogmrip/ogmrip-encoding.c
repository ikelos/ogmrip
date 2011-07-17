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

/**
 * SECTION:ogmrip-encoding
 * @title: OGMRipEncoding
 * @short_description: An all-in-one component to encode DVD titles
 * @include: ogmrip-encoding.h
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-container.h"
#include "ogmrip-dvdcpy.h"
#include "ogmrip-encoding.h"
#include "ogmrip-fs.h"
#include "ogmrip-hardsub.h"
#include "ogmrip-version.h"

#include <libxml/tree.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <string.h>
#include <unistd.h>
#include <math.h>

#define OGMRIP_ENCODING_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_ENCODING, OGMRipEncodingPriv))

#define OGMRIP_ENCODING_SET_FLAGS(enc,flag)    G_STMT_START{ (enc->priv->flags |=  (flag)); }G_STMT_END
#define OGMRIP_ENCODING_UNSET_FLAGS(enc,flag)  G_STMT_START{ (enc->priv->flags &= ~(flag)); }G_STMT_END

#undef OGMRIP_ENCODING_IS_RUNNING
#define OGMRIP_ENCODING_IS_RUNNING(enc) (((enc)->priv->flags & (OGMRIP_ENCODING_BACKUPING | OGMRIP_ENCODING_TESTING | OGMRIP_ENCODING_EXTRACTING)) != 0)

#define ROUND(x) ((gint) ((x) + 0.5) != (gint) (x) ? ((gint) ((x) + 0.5)) : ((gint) (x)))

#define SAMPLE_LENGTH  10.0
#define SAMPLE_PERCENT 0.05

struct _OGMRipEncodingPriv
{
  gint ntitle;
  gchar *device;
  gchar *id;

  /*< Parameters >*/

  gchar *label;
  gboolean auto_subp;
  gboolean relative;
  gboolean test;
  gint angle;

  OGMRipDeintType deint;

  guint crop_type;
  guint crop_x, crop_y;
  guint crop_w, crop_h;

  guint scale_type;
  guint scale_w, scale_h;

  GSList *audio_files;
  GSList *audio_streams;

  GSList *subp_files;
  GSList *subp_streams;

  GSList *chapters;

  gdouble start_time;
  gdouble play_length;

  /*< Options >*/

  gboolean keep_temp;
  gboolean ensure_sync;
  gboolean copy_dvd;
  guint threads;

  /*< Container options >*/

  GType container_type;
  gchar *fourcc;
  guint method;
  guint bitrate;
  guint target_number;
  guint target_size;
  gdouble quantizer;

  /*< Video options >*/

  GType video_codec_type;
  gboolean can_crop;
  gboolean can_scale;
  gboolean deblock;
  gboolean denoise;
  gboolean dering;
  gboolean expand;
  guint max_height;
  guint max_width;
  guint min_height;
  guint min_width;
  guint passes;
  guint quality;
  guint scaler;
  gboolean turbo;
  gdouble bpp;

  /*< Preferences >*/

  OGMDvdTitle *title;
  OGMDvdTitle *orig_title;

  OGMRipContainer *container;

  gint chap_lang;
  guint start_chap;
  gint  end_chap;

  /*< Computed >*/

  OGMRipEncodingTask task;

  OGMRipProfile *profile;

  guint32 flags;

  guint aspect_num;
  guint aspect_denom;

  gboolean progressive;
  gboolean telecine;
  gboolean log_open;

  gdouble rip_length;
  gint64 rip_size;
  gint64 dvd_size;
  gint64 sync_size;

  gchar *filename;
  gchar *logfile;

  /*< progress >*/

  gdouble pulse_step;
  gdouble fraction;
};

typedef struct
{
  gint nr;
  OGMDvdAudioStream *stream;
  OGMRipAudioOptions options;
} OGMRipAudioData;

typedef struct
{
  gint nr;
  OGMDvdSubpStream *stream;
  OGMRipSubpOptions options;
} OGMRipSubpData;

typedef struct
{
  gint nr;
  gchar *label;
} OGMRipChapterData;

enum
{
  RUN,
  COMPLETE,
  TASK,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_PROFILE,
  PROP_LABEL,
  PROP_START_CHAPTER,
  PROP_END_CHAPTER,
  PROP_CHAPTERS_LANG,
  PROP_COPY_DVD,
  PROP_ENSURE_SYNC,
  PROP_KEEP_TMP_FILES,
  PROP_FILENAME,
  PROP_THREADS,
  PROP_CONTAINER_TYPE,
  PROP_FOURCC,
  PROP_METHOD,
  PROP_BITRATE,
  PROP_TARGET_NUMBER,
  PROP_TARGET_SIZE,
  PROP_QUANTIZER,
  PROP_VIDEO_CODEC_TYPE,
  PROP_CAN_CROP,
  PROP_CAN_SCALE,
  PROP_DEBLOCK,
  PROP_DENOISE,
  PROP_DERING,
  PROP_TURBO,
  PROP_MAX_WIDTH,
  PROP_MAX_HEIGHT,
  PROP_EXPAND,
  PROP_MIN_WIDTH,
  PROP_MIN_HEIGHT,
  PROP_PASSES,
  PROP_PRESET,
  PROP_SCALER,
  PROP_BPP,
  PROP_RELATIVE,
  PROP_DEINTERLACER,
  PROP_ASPECT_NUM,
  PROP_ASPECT_DENOM,
  PROP_TEST,
  PROP_CROP_TYPE,
  PROP_CROP_X,
  PROP_CROP_Y,
  PROP_CROP_W,
  PROP_CROP_H,
  PROP_SCALE_TYPE,
  PROP_SCALE_W,
  PROP_SCALE_H,
  PROP_AUTO_SUBP
};

static void   ogmrip_encoding_dispose       (GObject             *gobject);
static void   ogmrip_encoding_finalize      (GObject             *gobject);
static void   ogmrip_encoding_set_property  (GObject             *gobject,
                                             guint               property_id,
                                             const GValue        *value,
                                             GParamSpec          *pspec);
static void   ogmrip_encoding_get_property  (GObject             *gobject,
                                             guint               property_id,
                                             GValue              *value,
                                             GParamSpec          *pspec);

static void   ogmrip_encoding_task          (OGMRipEncoding      *encoding,
                                             OGMRipEncodingTask  *task);
static void   ogmrip_encoding_complete      (OGMRipEncoding      *encoding,
                                             OGMJobResultType    result);

static gint64 ogmrip_encoding_get_sync_size (OGMRipEncoding      *encoding);

static guint signals[LAST_SIGNAL] = { 0 };

GQuark
ogmrip_encoding_error_quark (void)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_static_string ("ogmrip-encoding-error-quark");

  return quark;
}

/*
 * Audio data
 */

static void
ogmrip_encoding_free_audio_data (OGMRipAudioData *data)
{
  if (data->stream)
    ogmdvd_stream_unref (OGMDVD_STREAM (data->stream));

  ogmrip_audio_options_reset (&data->options);

  g_free (data);
}

static OGMRipAudioData *
ogmrip_encoding_get_nth_audio_data (OGMRipEncoding *encoding, guint n)
{
  return g_slist_nth_data (encoding->priv->audio_streams, n);
}

/*
 * SubpData
 */

static void
ogmrip_encoding_free_subp_data (OGMRipSubpData *data)
{
  if (data->stream)
    ogmdvd_stream_unref (OGMDVD_STREAM (data->stream));

  ogmrip_subp_options_reset (&data->options);

  g_free (data);
}

static OGMRipSubpData *
ogmrip_encoding_get_nth_subp_data (OGMRipEncoding *encoding, guint n)
{
  return g_slist_nth_data (encoding->priv->subp_streams, n);
}

/*
 * ChapterData
 */

static void
ogmrip_encoding_free_chapter_data (OGMRipChapterData *data)
{
  g_free (data->label);
  g_free (data);
}

static OGMRipChapterData *
ogmrip_encoding_get_chapter_data (OGMRipEncoding *encoding, guint nr)
{
  GSList *link;
  OGMRipChapterData *data;

  for (link = encoding->priv->chapters; link; link = link->next)
  {
    data = link->data;
    if (data->nr == nr)
      return data;
  }

  return NULL;
}

static gint
ogmrip_compare_chapter_data (OGMRipChapterData *data1, OGMRipChapterData *data2)
{
  return data1->nr - data2->nr;
}

static G_CONST_RETURN gchar *
ogmrip_encoding_get_device (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  if (encoding->priv->title)
    return ogmdvd_disc_get_device (ogmdvd_title_get_disc (encoding->priv->title));

  return encoding->priv->device;
}

static void
ogmrip_encoding_open_log (OGMRipEncoding *encoding)
{
  if (!encoding->priv->log_open)
  {
    ogmjob_log_open (encoding->priv->logfile, NULL);
    encoding->priv->log_open = TRUE;
  }
}

static void
ogmrip_encoding_close_log (OGMRipEncoding *encoding)
{
  if (encoding->priv->log_open)
  {
    ogmjob_log_close (NULL);
    encoding->priv->log_open = FALSE;
  }
}

/*
 * Object
 */

G_DEFINE_TYPE (OGMRipEncoding, ogmrip_encoding, G_TYPE_OBJECT)

static void
ogmrip_encoding_class_init (OGMRipEncodingClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_encoding_dispose;
  gobject_class->finalize = ogmrip_encoding_finalize;
  gobject_class->get_property = ogmrip_encoding_get_property;
  gobject_class->set_property = ogmrip_encoding_set_property;

  klass->task = ogmrip_encoding_task;
  klass->complete = ogmrip_encoding_complete;

  /**
   * OGMRipEncoding::run
   * @encoding: An #OGMRipEncoding
   *
   * Emitted each time an encoding starts.
   */
  signals[RUN] = g_signal_new ("run", G_TYPE_FROM_CLASS (klass), 
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipEncodingClass, run), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  /**
   * OGMRipEncoding::complete
   * @encoding: An #OGMRipEncoding
   * @result: An #OGMJobResultType
   *
   * Emitted each time an encoding completes.
   */
  signals[COMPLETE] = g_signal_new ("complete", G_TYPE_FROM_CLASS (klass), 
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipEncodingClass, complete), NULL, NULL,
      g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

  /**
   * OGMRipEncoding::task
   * @encoding: An #OGMRipEncoding
   * @task: An #OGMRipEncodingTask
   *
   * Emitted each time a task of @encoding starts, completeѕ, progresses, is
   * suspended, or is resumed.
   */
  signals[TASK] = g_signal_new ("task", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS | G_SIGNAL_DETAILED,
      G_STRUCT_OFFSET (OGMRipEncodingClass, task), NULL, NULL,
      g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

  g_object_class_install_property (gobject_class, PROP_CONTAINER_TYPE, 
      g_param_spec_gtype ("container-type", "Container type property", "Set the type of the container",
        OGMRIP_TYPE_CONTAINER, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_VIDEO_CODEC_TYPE, 
      g_param_spec_gtype ("video-codec-type", "Video codec type property", "Set the type of the video codec",
        OGMRIP_TYPE_VIDEO_CODEC, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PROFILE,
      g_param_spec_object ("profile", "Profile property", "Set the profile",
        G_TYPE_SETTINGS, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LABEL, 
      g_param_spec_string ("label", "Label property", "Set the label", 
        NULL, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FILENAME, 
      g_param_spec_string ("filename", "Filename property", "Set the filename", 
        NULL, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FOURCC, 
      g_param_spec_string ("fourcc", "FourCC property", "Set the fourCC", 
        NULL, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_COPY_DVD, 
      g_param_spec_boolean ("copy-dvd", "Copy DVD property", "Whether to copy the DVD on the hard drive", 
        TRUE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ENSURE_SYNC, 
      g_param_spec_boolean ("ensure-sync", "Ensure sync property", "Whether to ensure A/V sync", 
        TRUE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_KEEP_TMP_FILES, 
      g_param_spec_boolean ("keep-tmp-files", "Keep temporary files property", "Whether to keep temporary files", 
        FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CAN_CROP, 
      g_param_spec_boolean ("can-crop", "Can crop property", "Whether to crop the input", 
        TRUE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CAN_SCALE, 
      g_param_spec_boolean ("can-scale", "Can scale property", "Whether to scale the input", 
        TRUE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DEBLOCK, 
      g_param_spec_boolean ("deblock", "Deblock property", "Whether to deblock the input", 
        FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DENOISE, 
      g_param_spec_boolean ("denoise", "Denoise property", "Whether to denoise the input", 
        TRUE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DERING, 
      g_param_spec_boolean ("dering", "Dering property", "Whether to dering the input", 
        FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TURBO, 
      g_param_spec_boolean ("turbo", "Turbo property", "Whether to use turbo on first pass", 
        TRUE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RELATIVE, 
      g_param_spec_boolean ("relative", "Relative property", "Whether to compute the bitrate relatively to the length of the title", 
        FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TEST, 
      g_param_spec_boolean ("test", "Test property", "Whether to perform a compressibility test", 
        TRUE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_EXPAND, 
      g_param_spec_boolean ("expand", "Expand property", "Whether to expand to max size", 
        TRUE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_AUTO_SUBP, 
      g_param_spec_boolean ("auto-subp", "Auto subp property", "Whether to automatically hardcode subtitles", 
        TRUE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BITRATE,
      g_param_spec_uint ("bitrate", "Bitrate property", "Set bitrate",
        4000, 24000000, 800000, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_QUANTIZER, 
      g_param_spec_double ("quantizer", "Quantizer property", "Set quantizer", 
        -1.0, 31.0, -1.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BPP, 
      g_param_spec_double ("bpp", "Bits per pixel property", "Set bits per pixel", 
        0.0, 1.0, 0.25, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PASSES, 
      g_param_spec_uint ("passes", "Passes property", "Set the number of passes", 
        1, G_MAXUINT, 1, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_THREADS, 
      g_param_spec_uint ("threads", "Threads property", "Set the number of threads", 
        0, G_MAXUINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TARGET_SIZE, 
      g_param_spec_uint ("target-size", "Target size property", "Set target size", 
        0, G_MAXUINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TARGET_NUMBER, 
      g_param_spec_uint ("target-number", "Target number property", "Set target number", 
        0, G_MAXUINT, 1, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_START_CHAPTER, 
      g_param_spec_int ("start-chapter", "Start chapter property", "Set start chapter", 
        0, G_MAXINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_END_CHAPTER, 
      g_param_spec_int ("end-chapter", "End chapter property", "Set end chapter", 
        -1, G_MAXINT, -1, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_METHOD, 
      g_param_spec_uint ("method", "Method property", "Set method", 
        OGMRIP_ENCODING_SIZE, OGMRIP_ENCODING_QUANTIZER, OGMRIP_ENCODING_SIZE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MAX_WIDTH, 
      g_param_spec_uint ("max-width", "Max width property", "Set max width", 
        0, G_MAXINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MAX_HEIGHT, 
      g_param_spec_uint ("max-height", "Max height property", "Set max height", 
        0, G_MAXINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MIN_WIDTH, 
      g_param_spec_uint ("min-width", "Min width property", "Set min width", 
        0, G_MAXINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MIN_HEIGHT, 
      g_param_spec_uint ("min-height", "Min height property", "Set min height", 
        0, G_MAXINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ASPECT_NUM, 
      g_param_spec_uint ("aspect-num", "Aspect numerator property", "Set the aspect numerator", 
        0, G_MAXINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ASPECT_DENOM, 
      g_param_spec_uint ("aspect-denom", "Aspect denominator property", "Set the aspect denominator", 
        0, G_MAXINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CROP_X, 
      g_param_spec_uint ("crop-x", "Crop X property", "Set the crop horizontal position", 
        0, G_MAXINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CROP_Y, 
      g_param_spec_uint ("crop-y", "Crop Y property", "Set the crop vertical position", 
        0, G_MAXINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CROP_W, 
      g_param_spec_uint ("crop-width", "Crop width property", "Set the crop width", 
        0, G_MAXINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CROP_H, 
      g_param_spec_uint ("crop-height", "Crop height property", "Set the crop height", 
        0, G_MAXINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SCALE_W, 
      g_param_spec_uint ("scale-width", "Scale width property", "Set the scale width", 
        0, G_MAXINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SCALE_H, 
      g_param_spec_uint ("scale-height", "Scale height property", "Set the scale height", 
        0, G_MAXINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PRESET, 
      g_param_spec_uint ("quality", "Quality property", "Set the quality", 
        OGMRIP_VIDEO_QUALITY_EXTREME, OGMRIP_VIDEO_QUALITY_USER, OGMRIP_VIDEO_QUALITY_EXTREME, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SCALER, 
      g_param_spec_uint ("scaler", "Scaler property", "Set the software scaler", 
        OGMRIP_SCALER_FAST_BILINEAR, OGMRIP_SCALER_BICUBIC_SPLINE, OGMRIP_SCALER_GAUSS, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DEINTERLACER, 
      g_param_spec_uint ("deinterlacer", "Deinterlacer property", "Set the deinterlacer", 
        OGMRIP_DEINT_NONE, OGMRIP_DEINT_YADIF, OGMRIP_DEINT_NONE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CROP_TYPE, 
      g_param_spec_uint ("crop-type", "Crop type property", "Set the crop type", 
        OGMRIP_OPTIONS_NONE, OGMRIP_OPTIONS_MANUAL, OGMRIP_OPTIONS_AUTOMATIC, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SCALE_TYPE, 
      g_param_spec_uint ("scale-type", "Scale type property", "Set the scale type", 
        OGMRIP_OPTIONS_NONE, OGMRIP_OPTIONS_MANUAL, OGMRIP_OPTIONS_AUTOMATIC, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHAPTERS_LANG, 
      g_param_spec_uint ("chapters-lang", "Chapters language property", "Set the language of the chapter's label", 
        0, G_MAXINT, 0, G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (OGMRipEncodingPriv));
}

static void
ogmrip_encoding_init (OGMRipEncoding *encoding)
{
  encoding->priv = OGMRIP_ENCODING_GET_PRIVATE (encoding);

  encoding->priv->angle = 1;

  encoding->priv->start_time = -1;
  encoding->priv->play_length = -1;
  encoding->priv->start_chap = 0;
  encoding->priv->end_chap = -1;

  encoding->priv->ensure_sync = TRUE;
  encoding->priv->copy_dvd = TRUE;

#ifdef HAVE_SYSCONF_NPROC
  encoding->priv->threads = sysconf (_SC_NPROCESSORS_ONLN);
#else
  encoding->priv->threads = 1;
#endif

  encoding->priv->container_type = G_TYPE_NONE;
  encoding->priv->method = OGMRIP_ENCODING_SIZE;
  encoding->priv->bitrate = 800000;
  encoding->priv->target_number = 1;
  encoding->priv->target_size = 700;
  encoding->priv->quantizer = 2;

  encoding->priv->video_codec_type = G_TYPE_NONE;
  encoding->priv->can_crop = TRUE;
  encoding->priv->can_scale = TRUE;
  encoding->priv->denoise = TRUE;
  encoding->priv->turbo = TRUE;
  encoding->priv->passes = 2;
  encoding->priv->scaler = OGMRIP_SCALER_GAUSS;
  encoding->priv->bpp = 0.25;

  encoding->priv->crop_type = OGMRIP_OPTIONS_AUTOMATIC;
  encoding->priv->scale_type = OGMRIP_OPTIONS_AUTOMATIC;
  encoding->priv->auto_subp = TRUE;
  encoding->priv->test = TRUE;
}

static void
ogmrip_encoding_dispose (GObject *gobject)
{
  OGMRipEncoding *encoding;

  encoding = OGMRIP_ENCODING (gobject);

  if (encoding->priv->container)
  {
    g_object_unref (encoding->priv->container);
    encoding->priv->container = NULL;
  }

  if (encoding->priv->title)
  {
    ogmdvd_title_unref (encoding->priv->title);
    encoding->priv->title = NULL;
  }

  if (encoding->priv->orig_title)
  {
    ogmdvd_title_unref (encoding->priv->orig_title);
    encoding->priv->orig_title = NULL;
  }

  ogmrip_encoding_close_log (encoding);

  (*G_OBJECT_CLASS (ogmrip_encoding_parent_class)->dispose) (gobject);
}

static void
ogmrip_encoding_finalize (GObject *gobject)
{
  OGMRipEncoding *encoding;

  encoding = OGMRIP_ENCODING (gobject);

  if (encoding->priv->filename)
  {
    g_free (encoding->priv->filename);
    encoding->priv->filename = NULL;
  }

  if (encoding->priv->logfile)
  {
    g_free (encoding->priv->logfile);
    encoding->priv->logfile = NULL;
  }

  if (encoding->priv->device)
  {
    g_free (encoding->priv->device);
    encoding->priv->device = NULL;
  }

  if (encoding->priv->id)
  {
    g_free (encoding->priv->id);
    encoding->priv->id = NULL;
  }

  if (encoding->priv->profile)
  {
    g_object_unref (encoding->priv->profile);
    encoding->priv->profile= NULL;
  }

  if (encoding->priv->label)
  {
    g_free (encoding->priv->label);
    encoding->priv->label = NULL;
  }

  if (encoding->priv->fourcc)
  {
    g_free (encoding->priv->fourcc);
    encoding->priv->fourcc = NULL;
  }

  if (encoding->priv->audio_streams)
  {
    g_slist_foreach (encoding->priv->audio_streams,
        (GFunc) ogmrip_encoding_free_audio_data, NULL);
    g_slist_free (encoding->priv->audio_streams);
    encoding->priv->audio_streams = NULL;
  }

  if (encoding->priv->subp_streams)
  {
    g_slist_foreach (encoding->priv->subp_streams,
        (GFunc) ogmrip_encoding_free_subp_data, NULL);
    g_slist_free (encoding->priv->subp_streams);
    encoding->priv->subp_streams = NULL;
  }

  if (encoding->priv->audio_files)
  {
    g_slist_foreach (encoding->priv->audio_files,
        (GFunc) ogmrip_file_unref, NULL);
    g_slist_free (encoding->priv->audio_files);
    encoding->priv->audio_files = NULL;
  }

  if (encoding->priv->subp_files)
  {
    g_slist_foreach (encoding->priv->subp_files,
        (GFunc) ogmrip_file_unref, NULL);
    g_slist_free (encoding->priv->subp_files);
    encoding->priv->subp_files = NULL;
  }

  if (encoding->priv->chapters)
  {
    g_slist_foreach (encoding->priv->chapters,
        (GFunc) ogmrip_encoding_free_chapter_data, NULL);
    g_slist_free (encoding->priv->chapters);
    encoding->priv->chapters = NULL;
  }

  (*G_OBJECT_CLASS (ogmrip_encoding_parent_class)->finalize) (gobject);
}

static void
ogmrip_encoding_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipEncoding *encoding;

  encoding = OGMRIP_ENCODING (gobject);

  switch (property_id) 
  {
    case PROP_PROFILE:
      ogmrip_encoding_set_profile (encoding, g_value_get_object (value));
      break;
    case PROP_LABEL:
      ogmrip_encoding_set_label (encoding, g_value_get_string (value));
      break;
    case PROP_START_CHAPTER:
      ogmrip_encoding_set_chapters (encoding, g_value_get_int (value), encoding->priv->end_chap);
      break;
    case PROP_END_CHAPTER:
      ogmrip_encoding_set_chapters (encoding, encoding->priv->start_chap, g_value_get_int (value));
      break;
    case PROP_CHAPTERS_LANG:
      ogmrip_encoding_set_chapters_language (encoding, g_value_get_uint (value));
      break;
    case PROP_COPY_DVD:
      ogmrip_encoding_set_copy_dvd (encoding, g_value_get_boolean (value));
      break;
    case PROP_ENSURE_SYNC:
      ogmrip_encoding_set_ensure_sync (encoding, g_value_get_boolean (value));
      break;
    case PROP_KEEP_TMP_FILES:
      ogmrip_encoding_set_keep_tmp_files (encoding, g_value_get_boolean (value));
      break;
    case PROP_FILENAME:
      ogmrip_encoding_set_filename (encoding, g_value_get_string (value));
      break;
    case PROP_THREADS:
      ogmrip_encoding_set_threads (encoding, g_value_get_uint (value));
      break;
    case PROP_CONTAINER_TYPE:
      ogmrip_encoding_set_container_type (encoding, g_value_get_gtype (value), NULL);
      break;
    case PROP_FOURCC:
      ogmrip_encoding_set_fourcc (encoding, g_value_get_string (value));
      break;
    case PROP_METHOD:
      ogmrip_encoding_set_method (encoding, g_value_get_uint (value));
      break;
    case PROP_BITRATE:
      ogmrip_encoding_set_bitrate (encoding, g_value_get_uint (value));
      break;
    case PROP_TARGET_NUMBER:
      ogmrip_encoding_set_target_number (encoding, g_value_get_uint (value));
      break;
    case PROP_TARGET_SIZE:
      ogmrip_encoding_set_target_size (encoding, g_value_get_uint (value));
      break;
    case PROP_QUANTIZER:
      ogmrip_encoding_set_quantizer (encoding, g_value_get_double (value));
      break;
    case PROP_VIDEO_CODEC_TYPE:
      ogmrip_encoding_set_video_codec_type (encoding, g_value_get_gtype (value), NULL);
      break;
    case PROP_CAN_CROP:
      ogmrip_encoding_set_can_crop (encoding, g_value_get_boolean (value));
      break;
    case PROP_CAN_SCALE:
      ogmrip_encoding_set_can_scale (encoding, g_value_get_boolean (value));
      break;
    case PROP_DEBLOCK:
      ogmrip_encoding_set_deblock (encoding, g_value_get_boolean (value));
      break;
    case PROP_DENOISE:
      ogmrip_encoding_set_denoise (encoding, g_value_get_boolean (value));
      break;
    case PROP_DERING:
      ogmrip_encoding_set_dering (encoding, g_value_get_boolean (value));
      break;
    case PROP_TURBO:
      ogmrip_encoding_set_turbo (encoding, g_value_get_boolean (value));
      break;
    case PROP_MAX_WIDTH:
      ogmrip_encoding_set_max_size (encoding, g_value_get_uint (value), encoding->priv->max_height, encoding->priv->expand);
      break;
    case PROP_MAX_HEIGHT:
      ogmrip_encoding_set_max_size (encoding, encoding->priv->max_width, g_value_get_uint (value), encoding->priv->expand);
      break;
    case PROP_EXPAND:
      ogmrip_encoding_set_max_size (encoding, encoding->priv->max_width, encoding->priv->max_height, g_value_get_boolean (value));
      break;
    case PROP_MIN_WIDTH:
      ogmrip_encoding_set_min_size (encoding, g_value_get_uint (value), encoding->priv->min_height);
      break;
    case PROP_MIN_HEIGHT:
      ogmrip_encoding_set_min_size (encoding, encoding->priv->min_width, g_value_get_uint (value));
      break;
    case PROP_PASSES:
      ogmrip_encoding_set_passes (encoding, g_value_get_uint (value));
      break;
    case PROP_PRESET:
      ogmrip_encoding_set_quality (encoding, g_value_get_uint (value));
      break;
    case PROP_SCALER:
      ogmrip_encoding_set_scaler (encoding, g_value_get_uint (value));
      break;
    case PROP_BPP:
      ogmrip_encoding_set_bits_per_pixel (encoding, g_value_get_double (value));
      break;
    case PROP_RELATIVE:
      ogmrip_encoding_set_relative (encoding, g_value_get_boolean (value));
      break;
    case PROP_DEINTERLACER:
      ogmrip_encoding_set_deinterlacer (encoding, g_value_get_uint (value));
      break;
    case PROP_ASPECT_NUM:
      ogmrip_encoding_set_aspect_ratio (encoding, g_value_get_uint (value), encoding->priv->aspect_denom);
      break;
    case PROP_ASPECT_DENOM:
      ogmrip_encoding_set_aspect_ratio (encoding, encoding->priv->aspect_num, g_value_get_uint (value));
      break;
    case PROP_TEST:
      ogmrip_encoding_set_test (encoding, g_value_get_boolean (value));
      break;
    case PROP_CROP_TYPE:
      ogmrip_encoding_set_crop (encoding, g_value_get_uint (value), encoding->priv->crop_x,
          encoding->priv->crop_y, encoding->priv->crop_w, encoding->priv->crop_h);
      break;
    case PROP_CROP_X:
      ogmrip_encoding_set_crop (encoding, encoding->priv->crop_type, g_value_get_uint (value),
          encoding->priv->crop_y, encoding->priv->crop_w, encoding->priv->crop_h);
      break;
    case PROP_CROP_Y:
      ogmrip_encoding_set_crop (encoding, encoding->priv->crop_type, encoding->priv->crop_x,
          g_value_get_uint (value), encoding->priv->crop_w, encoding->priv->crop_h);
      break;
    case PROP_CROP_W:
      ogmrip_encoding_set_crop (encoding, encoding->priv->crop_type, encoding->priv->crop_x,
          encoding->priv->crop_y, g_value_get_uint (value), encoding->priv->crop_h);
      break;
    case PROP_CROP_H:
      ogmrip_encoding_set_crop (encoding, encoding->priv->crop_type, encoding->priv->crop_x,
          encoding->priv->crop_y, encoding->priv->crop_w, g_value_get_uint (value));
      break;
    case PROP_SCALE_TYPE:
      ogmrip_encoding_set_scale (encoding, g_value_get_uint (value),
          encoding->priv->scale_w, encoding->priv->scale_h);
      break;
    case PROP_SCALE_W:
      ogmrip_encoding_set_scale (encoding, encoding->priv->scale_type,
          g_value_get_uint (value), encoding->priv->scale_h);
      break;
    case PROP_SCALE_H:
      ogmrip_encoding_set_scale (encoding, encoding->priv->scale_type,
          encoding->priv->scale_w, g_value_get_uint (value));
      break;
    case PROP_AUTO_SUBP:
      ogmrip_encoding_set_auto_subp (encoding, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_encoding_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipEncoding *encoding;

  encoding = OGMRIP_ENCODING (gobject);

  switch (property_id) 
  {
    case PROP_PROFILE:
      g_value_set_object (value, encoding->priv->profile);
      break;
    case PROP_LABEL:
      g_value_set_string (value, encoding->priv->label);
      break;
    case PROP_START_CHAPTER:
      g_value_set_int (value, encoding->priv->start_chap);
      break;
    case PROP_END_CHAPTER:
      g_value_set_int (value, encoding->priv->end_chap);
      break;
    case PROP_CHAPTERS_LANG:
      g_value_set_uint (value, encoding->priv->chap_lang);
      break;
    case PROP_COPY_DVD:
      g_value_set_boolean (value, encoding->priv->copy_dvd);
      break;
    case PROP_ENSURE_SYNC:
      g_value_set_boolean (value, encoding->priv->ensure_sync);
      break;
    case PROP_KEEP_TMP_FILES:
      g_value_set_boolean (value, encoding->priv->keep_temp);
      break;
    case PROP_FILENAME:
      g_value_set_string (value, encoding->priv->filename);
      break;
    case PROP_THREADS:
      g_value_set_uint (value, encoding->priv->threads);
      break;
    case PROP_CONTAINER_TYPE:
      g_value_set_gtype (value, encoding->priv->container_type);
      break;
    case PROP_FOURCC:
      g_value_set_string (value, encoding->priv->fourcc);
      break;
    case PROP_METHOD:
      g_value_set_uint (value, encoding->priv->method);
      break;
    case PROP_BITRATE:
      g_value_set_uint (value, encoding->priv->bitrate);
      break;
    case PROP_TARGET_NUMBER:
      g_value_set_uint (value, encoding->priv->target_number);
      break;
    case PROP_TARGET_SIZE:
      g_value_set_uint (value, encoding->priv->target_size);
      break;
    case PROP_QUANTIZER:
      g_value_set_double (value, encoding->priv->quantizer);
      break;
    case PROP_VIDEO_CODEC_TYPE:
      g_value_set_gtype (value, encoding->priv->video_codec_type);
      break;
    case PROP_CAN_CROP:
      g_value_set_boolean (value, encoding->priv->can_crop);
      break;
    case PROP_CAN_SCALE:
      g_value_set_boolean (value, encoding->priv->can_scale);
      break;
    case PROP_DEBLOCK:
      g_value_set_boolean (value, encoding->priv->deblock);
      break;
    case PROP_DENOISE:
      g_value_set_boolean (value, encoding->priv->denoise);
      break;
    case PROP_DERING:
      g_value_set_boolean (value, encoding->priv->dering);
      break;
    case PROP_TURBO:
      g_value_set_boolean (value, encoding->priv->turbo);
      break;
    case PROP_MAX_WIDTH:
      g_value_set_uint (value, encoding->priv->max_width);
      break;
    case PROP_MAX_HEIGHT:
      g_value_set_uint (value, encoding->priv->max_height);
      break;
    case PROP_EXPAND:
      g_value_set_boolean (value, encoding->priv->expand);
      break;
    case PROP_MIN_WIDTH:
      g_value_set_uint (value, encoding->priv->min_width);
      break;
    case PROP_MIN_HEIGHT:
      g_value_set_uint (value, encoding->priv->min_height);
      break;
    case PROP_PASSES:
      g_value_set_uint (value, encoding->priv->passes);
      break;
    case PROP_PRESET:
      g_value_set_uint (value, encoding->priv->quality);
      break;
    case PROP_SCALER:
      g_value_set_uint (value, encoding->priv->scaler);
      break;
    case PROP_BPP:
      g_value_set_double (value, encoding->priv->bpp);
      break;
    case PROP_RELATIVE:
      g_value_set_boolean (value, encoding->priv->relative);
      break;
    case PROP_DEINTERLACER:
      g_value_set_uint (value, encoding->priv->deint);
      break;
    case PROP_ASPECT_NUM:
      g_value_set_uint (value, encoding->priv->aspect_num);
      break;
    case PROP_ASPECT_DENOM:
      g_value_set_uint (value, encoding->priv->aspect_denom);
      break;
    case PROP_TEST:
      g_value_set_boolean (value, encoding->priv->test);
      break;
    case PROP_CROP_TYPE:
      g_value_set_uint (value, encoding->priv->crop_type);
      break;
    case PROP_CROP_X:
      g_value_set_uint (value, encoding->priv->crop_x);
      break;
    case PROP_CROP_Y:
      g_value_set_uint (value, encoding->priv->crop_y);
      break;
    case PROP_CROP_W:
      g_value_set_uint (value, encoding->priv->crop_w);
      break;
    case PROP_CROP_H:
      g_value_set_uint (value, encoding->priv->crop_h);
      break;
    case PROP_SCALE_TYPE:
      g_value_set_uint (value, encoding->priv->scale_type);
      break;
    case PROP_SCALE_W:
      g_value_set_uint (value, encoding->priv->scale_w);
      break;
    case PROP_SCALE_H:
      g_value_set_uint (value, encoding->priv->scale_h);
      break;
    case PROP_AUTO_SUBP:
      g_value_set_boolean (value, encoding->priv->auto_subp);
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_encoding_emit_task (OGMRipEncoding *encoding, OGMJobSpawn *spawn,
    OGMRipOptions*options, OGMRipTaskEvent event, OGMRipTaskType type, OGMRipTaskDetail detail)
{
  static const gchar *str[] =
  {
    "run",
    "progress",
    "complete",
    "suspend",
    "resume"
  };

  if (spawn)
    encoding->priv->task.spawn = spawn;

  if (options)
    encoding->priv->task.options = options;

  encoding->priv->task.type = type;
  encoding->priv->task.event = event;
  encoding->priv->task.detail = detail;

  g_signal_emit (encoding, signals[TASK], g_quark_from_string (str[event]), &encoding->priv->task);
}

static void
ogmrip_encoding_spawn_progressed (OGMRipEncoding *encoding, gdouble fraction, OGMJobSpawn *spawn)
{
  ogmrip_encoding_emit_task (encoding,
      encoding->priv->task.spawn,
      encoding->priv->task.options,
      OGMRIP_TASK_PROGRESS,
      encoding->priv->task.type,
      (OGMRipTaskDetail) fraction);
}

static void
ogmrip_encoding_spawn_suspended (OGMRipEncoding *encoding, OGMJobSpawn *spawn)
{
  ogmrip_encoding_emit_task (encoding,
      encoding->priv->task.spawn,
      encoding->priv->task.options,
      OGMRIP_TASK_SUSPEND,
      encoding->priv->task.type,
      (OGMRipTaskDetail) 0);
}

static void
ogmrip_encoding_spawn_resumed (OGMRipEncoding *encoding, OGMJobSpawn *spawn)
{
  ogmrip_encoding_emit_task (encoding,
      encoding->priv->task.spawn,
      encoding->priv->task.options,
      OGMRIP_TASK_RESUME,
      encoding->priv->task.type,
      (OGMRipTaskDetail) 0);
}

static void
ogmrip_encoding_task (OGMRipEncoding *encoding, OGMRipEncodingTask *task)
{
  if (task->spawn)
  {
    if (task->event == OGMRIP_TASK_RUN)
    {
      if (task->type != OGMRIP_TASK_TEST)
        g_signal_connect_swapped (task->spawn, "progress",
            G_CALLBACK (ogmrip_encoding_spawn_progressed), encoding);

      g_signal_connect_swapped (task->spawn, "suspend",
          G_CALLBACK (ogmrip_encoding_spawn_suspended), encoding);
      g_signal_connect_swapped (task->spawn, "resume",
          G_CALLBACK (ogmrip_encoding_spawn_resumed), encoding);
    }
    else if (task->event == OGMRIP_TASK_COMPLETE)
    {
      g_signal_handlers_disconnect_by_func (task->spawn,
          ogmrip_encoding_spawn_progressed, encoding);
      g_signal_handlers_disconnect_by_func (task->spawn,
          ogmrip_encoding_spawn_suspended, encoding);
      g_signal_handlers_disconnect_by_func (task->spawn,
          ogmrip_encoding_spawn_resumed, encoding);

      OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_CANCELING);
    }
  }
}

static void
ogmrip_encoding_complete (OGMRipEncoding *encoding, OGMJobResultType result)
{
  if (encoding->priv->log_open)
    ogmrip_encoding_close_log (encoding);

  if (encoding->priv->container)
  {
    g_object_unref (encoding->priv->container);
    encoding->priv->container = NULL;
  }
}

/*
 * Internal functions
 */

static gint
ogmrip_encoding_get_title_nr (OGMRipEncoding *encoding)
{
  if (encoding->priv->title)
    return ogmdvd_title_get_nr (encoding->priv->title);

  return encoding->priv->ntitle;
}

/*
 * Adds external files to the container
 */
static void
ogmrip_encoding_container_add_files (OGMRipEncoding *encoding, OGMRipContainer *container)
{
  GSList *link;

  for (link = encoding->priv->audio_files; link; link = link->next)
    ogmrip_container_add_file (container, OGMRIP_FILE (link->data));

  for (link = encoding->priv->subp_files; link; link = link->next)
    ogmrip_container_add_file (container, OGMRIP_FILE (link->data));
}

static gchar *
ogmrip_encoding_get_backup_dir (OGMRipEncoding *encoding)
{
  gchar *dir, *path;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  if (!encoding->priv->id)
    return NULL;

  dir = g_strdup_printf ("dvd-%d-%s", ogmdvd_title_get_ts_nr (encoding->priv->title), encoding->priv->id);
  path = g_build_filename (ogmrip_fs_get_tmp_dir (), dir, NULL);
  g_free (dir);

  return path;
}

/*
 * Creates the container
 */
static OGMRipContainer *
ogmrip_encoding_get_container (OGMRipEncoding *encoding)
{
  OGMJobSpawn *spawn;

  if (encoding->priv->container)
    return encoding->priv->container;

  spawn = g_object_new (encoding->priv->container_type, "output", ogmrip_encoding_get_filename (encoding), NULL);

  ogmrip_container_set_split (OGMRIP_CONTAINER (spawn), encoding->priv->target_number, encoding->priv->target_size);
  ogmrip_container_set_label (OGMRIP_CONTAINER (spawn), encoding->priv->label);
  ogmrip_container_set_fourcc (OGMRIP_CONTAINER (spawn), encoding->priv->fourcc);

  ogmrip_encoding_container_add_files (encoding, OGMRIP_CONTAINER (spawn));

  encoding->priv->container = OGMRIP_CONTAINER (spawn);

  return encoding->priv->container;
}

typedef struct
{
  guint files;
  gdouble length;
} OGMRipLengthData;

static void
ogmrip_encoding_get_video_length (OGMRipCodec *codec, OGMRipLengthData *data)
{
  OGMRipFile *file;

  file = ogmrip_video_file_new (ogmrip_codec_get_output (codec), NULL);
  if (file)
  {
    data->files ++;
    data->length += ogmrip_video_file_get_length (OGMRIP_VIDEO_FILE (file));
    ogmrip_file_unref (file);
  }
}

static void
ogmrip_encoding_get_audio_length (OGMRipContainer *container, OGMRipCodec *codec, guint demuxer, gint language, OGMRipLengthData *data)
{
  OGMRipFile *file;

  file = ogmrip_audio_file_new (ogmrip_codec_get_output (codec), NULL);
  if (file)
  {
    data->files ++;
    data->length += ogmrip_audio_file_get_length (OGMRIP_AUDIO_FILE (file));
    ogmrip_file_unref (file);
  }
}

static gdouble
ogmrip_encoding_get_rip_length_from_files (OGMRipEncoding *encoding)
{
  OGMRipContainer *container;
  OGMRipLengthData data;

  data.files = 0;
  data.length = 0.0;

  container = ogmrip_encoding_get_container (encoding);
  ogmrip_container_foreach_audio (container, (OGMRipContainerCodecFunc) ogmrip_encoding_get_audio_length, &data);

  if (!data.files)
  {
    OGMRipVideoCodec *codec;

    codec = ogmrip_container_get_video (container);
    if (codec)
      ogmrip_encoding_get_video_length (OGMRIP_CODEC (codec), &data);
  }

  if (!data.files)
    return 0.0;

  return data.length / data.files;
}

/*
 * Returns the rip length
 */
static gdouble
ogmrip_encoding_get_rip_length (OGMRipEncoding *encoding)
{
  if (encoding->priv->rip_length <= 0.0)
  {
    if (encoding->priv->play_length > 0.0)
    {
      encoding->priv->rip_length = ogmrip_encoding_get_rip_length_from_files (encoding);
      if (encoding->priv->rip_length <= 0.0)
        return encoding->priv->play_length;
    }
    else if (encoding->priv->start_chap == 0 && encoding->priv->end_chap == -1)
      encoding->priv->rip_length = ogmdvd_title_get_length (encoding->priv->title, NULL);
    else
      encoding->priv->rip_length = ogmdvd_title_get_chapters_length (encoding->priv->title,
          encoding->priv->start_chap, encoding->priv->end_chap, NULL);
  }

  return encoding->priv->rip_length;
}

/*
 * Returns the estimed audio size used to enforce sync
 */
static gint64
ogmrip_encoding_get_sync_size (OGMRipEncoding *encoding)
{
  if (!encoding->priv->sync_size && encoding->priv->ensure_sync)
  {
    gdouble chap_len;

    chap_len = ogmdvd_title_get_chapters_length (encoding->priv->title, encoding->priv->start_chap, encoding->priv->end_chap, NULL);
    if (chap_len < 0.0)
      return -1;

    encoding->priv->sync_size = (gint64) (chap_len * 16000);
  }

  return encoding->priv->sync_size;
}

/*
 * Returns the estimated rip size
 */
static gint64
ogmrip_encoding_get_rip_size (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  if (!encoding->priv->rip_size)
  {
    g_return_val_if_fail (encoding->priv->title != NULL, -1);

    if (encoding->priv->target_size > 0)
    {
      gdouble factor = 1.0;

      if (encoding->priv->relative)
      {
        gdouble full_len;

        full_len = ogmdvd_title_get_length (encoding->priv->title, NULL);
        if (full_len < 0)
          return -1;

        factor = ogmrip_encoding_get_rip_length (encoding) / full_len;
      }

      encoding->priv->rip_size = (gint64) ceil (factor * encoding->priv->target_size * encoding->priv->target_number * 1024 * 1024);
    }
    else if (encoding->priv->bitrate > 0)
      encoding->priv->rip_size = (gint64) ceil (encoding->priv->bitrate * ogmrip_encoding_get_rip_length (encoding) / 8.0);
  }
  
  return encoding->priv->rip_size;
}

/*
 * Returns the VMG + VTS size
 */
static gint64
ogmrip_encoding_get_dvd_size (OGMRipEncoding *encoding)
{
  gchar *path;
  gboolean is_dir;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  if (!encoding->priv->copy_dvd)
    return 0;

  path = ogmrip_encoding_get_backup_dir (encoding);
  is_dir = g_file_test (path, G_FILE_TEST_IS_DIR);
  g_free (path);

  if (is_dir)
  {
    OGMDvdDisc *disc;

    disc = ogmdvd_disc_new (path, NULL);
    if (disc)
    {
      ogmdvd_disc_unref (disc);
      return 0;
    }
  }

  if (!encoding->priv->dvd_size)
  {
    OGMDvdDisc *disc;
    gint64 vts_size;

    g_return_val_if_fail (encoding->priv->title != NULL, -1);

    vts_size = ogmdvd_title_get_vts_size (encoding->priv->title);
    if (vts_size < 0)
      return -1;

    disc = ogmdvd_title_get_disc (encoding->priv->title);

    encoding->priv->dvd_size = vts_size + ogmdvd_disc_get_vmg_size (disc);
  }

  return encoding->priv->dvd_size;
}

static gint64
ogmrip_encoding_get_tmp_size (OGMRipEncoding *encoding)
{
  gint64 sync_size, rip_size;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  rip_size  = ogmrip_encoding_get_rip_size  (encoding);
  sync_size = ogmrip_encoding_get_sync_size (encoding);

  return rip_size + sync_size;
}

static void
ogmrip_encoding_set_title (OGMRipEncoding *encoding, OGMDvdTitle *title)
{
  if (title != encoding->priv->title)
  {
    OGMDvdDisc *disc;
    OGMRipAudioData *audio_data;
    OGMRipSubpData *subp_data;
    GSList *link;

    struct stat buf;

    if (title == encoding->priv->orig_title)
      encoding->priv->orig_title = NULL;
    else
    {
      ogmdvd_title_ref (title);

      if (encoding->priv->title)
        encoding->priv->orig_title = encoding->priv->title;
    }

    encoding->priv->title = title;

    disc = ogmdvd_title_get_disc (title);

    if (encoding->priv->device)
      g_free (encoding->priv->device);
    encoding->priv->device = g_strdup (ogmdvd_disc_get_device (disc));

    if (!encoding->priv->id)
      encoding->priv->id = g_strdup (ogmdvd_disc_get_id (disc));

    encoding->priv->ntitle = ogmdvd_title_get_nr (title);

    for (link = encoding->priv->audio_streams; link; link = link->next)
    {
      audio_data = link->data;

      if (audio_data->stream)
        ogmdvd_stream_unref (OGMDVD_STREAM (audio_data->stream));

      audio_data->stream = ogmdvd_title_get_nth_audio_stream (encoding->priv->title, audio_data->nr);
    }

    for (link = encoding->priv->subp_streams; link; link = link->next)
    {
      subp_data = link->data;

      if (subp_data->stream)
        ogmdvd_stream_unref (OGMDVD_STREAM (subp_data->stream));

      subp_data->stream = ogmdvd_title_get_nth_subp_stream (encoding->priv->title, subp_data->nr);
    }

    ogmrip_encoding_set_chapters (encoding, 0, -1);

    encoding->priv->copy_dvd &= encoding->priv->device && g_stat (encoding->priv->device, &buf) == 0 && S_ISBLK (buf.st_mode);
  }
}

static void
ogmrip_encoding_set_play_length (OGMRipEncoding *encoding, gdouble play_length)
{
  encoding->priv->play_length = play_length;

  encoding->priv->sync_size = 0;
  encoding->priv->rip_length = 0;
  encoding->priv->rip_size = 0;
}

static void
ogmrip_encoding_set_start_time (OGMRipEncoding *encoding, gdouble start_time)
{
  encoding->priv->start_time = start_time;
}

static void
ogmrip_encoding_set_relative_internal (OGMRipEncoding *encoding, gboolean relative)
{
  encoding->priv->relative = relative;
  encoding->priv->rip_size = 0;
}

static void
ogmrip_encoding_set_scale_internal (OGMRipEncoding *encoding, OGMRipOptionsType type, guint w, guint h)
{
  encoding->priv->scale_w = w;
  encoding->priv->scale_h = h;

  if (!w && !h)
    type = OGMRIP_OPTIONS_NONE;

  encoding->priv->scale_type = type;
}

/*
 * Closes the DVD title
 */
static void
ogmrip_encoding_close_title (OGMRipEncoding *encoding)
{
  if (ogmdvd_title_is_open (encoding->priv->title))
    ogmdvd_title_close (encoding->priv->title);
}

/*
 * Opens the DVD title
 */
static gboolean
ogmrip_encoding_open_title (OGMRipEncoding *encoding, GError **error)
{
  if (encoding->priv->copy_dvd)
  {
    gchar *path;

    path = ogmrip_encoding_get_backup_dir (encoding);
    if (g_file_test (path, G_FILE_TEST_IS_DIR))
    {
      OGMDvdDisc *disc;

      disc = ogmdvd_disc_new (path, NULL);
      if (disc)
      {
        OGMDvdTitle *title;

        title = ogmdvd_disc_get_nth_title (disc, ogmdvd_title_get_nr (encoding->priv->title));
        if (title)
        {
          if (encoding->priv->title)
            ogmrip_encoding_close_title (encoding);

          ogmrip_encoding_set_title (encoding, title);

          ogmdvd_title_unref (title);
        }
        ogmdvd_disc_unref (disc);
      }
    }
    g_free (path);
  }

  if (ogmdvd_title_is_open (encoding->priv->title))
    return TRUE;

  if (ogmdvd_title_open (encoding->priv->title, error))
    return TRUE;

  if (!encoding->priv->orig_title)
    return FALSE;

  ogmrip_encoding_set_title (encoding, encoding->priv->orig_title);
  encoding->priv->copy_dvd = TRUE;

  return ogmrip_encoding_open_title (encoding, error);
}

/*
 * Analyzes the video stream
 */
static gint
ogmrip_encoding_analyze_video_stream (OGMRipEncoding *encoding, GError **error)
{
  OGMJobSpawn *spawn;
  gint result;

  if ((encoding->priv->flags & OGMRIP_ENCODING_CANCELING) != 0)
    return OGMJOB_RESULT_CANCEL;

  if ((encoding->priv->flags & OGMRIP_ENCODING_ANALYZED) != 0)
    return OGMJOB_RESULT_SUCCESS;

  ogmjob_log_printf ("\nAnalyzing video title %d\n", ogmrip_encoding_get_title_nr (encoding) + 1);
  ogmjob_log_printf ("-----------------------\n\n");

  spawn = g_object_new (encoding->priv->video_codec_type, "input", encoding->priv->title, NULL);
  if (!spawn)
    return OGMJOB_RESULT_ERROR;

  ogmrip_encoding_emit_task (encoding, spawn,
      NULL, OGMRIP_TASK_RUN, OGMRIP_TASK_ANALYZE, (OGMRipTaskDetail) 0);
  result = ogmrip_video_codec_analyze (OGMRIP_VIDEO_CODEC (spawn), 0) ? OGMJOB_RESULT_SUCCESS : OGMJOB_RESULT_CANCEL;
  ogmrip_encoding_emit_task (encoding, spawn,
      NULL, OGMRIP_TASK_COMPLETE, OGMRIP_TASK_ANALYZE, (OGMRipTaskDetail) result);

  if (result == OGMJOB_RESULT_SUCCESS)
  {
    encoding->priv->telecine = ogmrip_codec_get_telecine (OGMRIP_CODEC (spawn));
    ogmjob_log_printf ("\nTelecine: %s\n", encoding->priv->telecine ? "true" : "false");

    encoding->priv->progressive = ogmrip_codec_get_progressive (OGMRIP_CODEC (spawn));
    ogmjob_log_printf ("Progressive: %s\n\n", encoding->priv->progressive ? "true" : "false");

    OGMRIP_ENCODING_SET_FLAGS (encoding, OGMRIP_ENCODING_ANALYZED);
  }

  g_object_unref (spawn);

  if (result != OGMJOB_RESULT_SUCCESS)
    return OGMJOB_RESULT_CANCEL;

  return OGMJOB_RESULT_SUCCESS;
}

/*
 * Creates chapters extractor
 */
static OGMJobSpawn *
ogmrip_encoding_get_chapters_extractor (OGMRipEncoding *encoding, GError **error)
{
  OGMJobSpawn *spawn;
  const gchar *label;
  gchar *output;
  gint chapter;

  output = ogmrip_fs_mktemp ("chapters.XXXXXX", error);
  if (!output)
    return NULL;

  spawn = ogmrip_chapters_new (encoding->priv->title, output);
  g_free (output);

  for (chapter = encoding->priv->start_chap; chapter <= encoding->priv->end_chap; chapter ++)
    if ((label = ogmrip_encoding_get_chapter_label (encoding, chapter)))
      ogmrip_chapters_set_label (OGMRIP_CHAPTERS (spawn), chapter, label);

  ogmrip_codec_set_unlink_on_unref (OGMRIP_CODEC (spawn), !encoding->priv->keep_temp);
  ogmrip_codec_set_chapters (OGMRIP_CODEC (spawn), encoding->priv->start_chap, encoding->priv->end_chap);

  ogmrip_container_add_chapters (ogmrip_encoding_get_container (encoding), OGMRIP_CHAPTERS (spawn), encoding->priv->chap_lang);
  g_object_unref (spawn);

  return spawn;
}

/*
 * Extracts chapter information
 */
static gint
ogmrip_encoding_extract_chapters (OGMRipEncoding *encoding, GError **error)
{
  OGMJobSpawn *spawn;
  gint result;

  if ((encoding->priv->flags & OGMRIP_ENCODING_CANCELING) != 0)
    return OGMJOB_RESULT_CANCEL;

  spawn = ogmrip_encoding_get_chapters_extractor (encoding, error);
  if (!spawn)
    return OGMJOB_RESULT_ERROR;

  ogmrip_encoding_emit_task (encoding, spawn, NULL, OGMRIP_TASK_RUN, OGMRIP_TASK_CHAPTERS, (OGMRipTaskDetail) 0);
  result = ogmjob_spawn_run (spawn, error);
  ogmrip_encoding_emit_task (encoding, spawn, NULL, OGMRIP_TASK_COMPLETE, OGMRIP_TASK_CHAPTERS, (OGMRipTaskDetail) result);

  if (result != OGMJOB_RESULT_SUCCESS)
  {
    if (result == OGMJOB_RESULT_ERROR && error && !(*error))
      g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_UNKNOWN,
          _("Unknown error while extracting chapters"));

    return result;
  }

  return OGMJOB_RESULT_SUCCESS;
}

/*
 * Creates the subp encoder
 */
static OGMJobSpawn *
ogmrip_encoding_get_subp_encoder (OGMRipEncoding *encoding, OGMRipSubpData *data, GError **error)
{
  OGMRipSubpDemuxer demuxer;
  OGMJobSpawn *spawn;
  gchar *output;

  if (data->options.codec == G_TYPE_NONE || data->options.codec == OGMRIP_TYPE_HARDSUB)
    return NULL;

  output = ogmrip_fs_mktemp ("subp.XXXXXX", error);
  if (!output)
    return NULL;

  spawn = g_object_new (data->options.codec, "stream", data->stream, "output", output, NULL);
  g_free (output);

  ogmrip_codec_set_unlink_on_unref (OGMRIP_CODEC (spawn), !encoding->priv->keep_temp);
  ogmrip_codec_set_chapters (OGMRIP_CODEC (spawn), encoding->priv->start_chap, encoding->priv->end_chap);
  ogmrip_codec_set_progressive (OGMRIP_CODEC (spawn), encoding->priv->progressive);

  ogmrip_subp_codec_set_charset (OGMRIP_SUBP_CODEC (spawn), data->options.charset);
  ogmrip_subp_codec_set_newline (OGMRIP_SUBP_CODEC (spawn), data->options.newline);
  ogmrip_subp_codec_set_forced_only (OGMRIP_SUBP_CODEC (spawn), data->options.forced_subs);
  ogmrip_subp_codec_set_label (OGMRIP_SUBP_CODEC (spawn), data->options.label);

  demuxer = OGMRIP_SUBP_DEMUXER_AUTO;
  if (ogmrip_plugin_get_subp_codec_format (data->options.codec) == OGMRIP_FORMAT_VOBSUB)
    demuxer = OGMRIP_SUBP_DEMUXER_VOBSUB;

  ogmrip_container_add_subp (ogmrip_encoding_get_container (encoding), OGMRIP_SUBP_CODEC (spawn), demuxer, data->options.language);
  g_object_unref (spawn);

  return spawn;
}

/*
 * Encodes the subp streams
 */
static gint
ogmrip_encoding_encode_subp_streams (OGMRipEncoding *encoding, gboolean do_test, GError **error)
{
  OGMRipSubpData *data;
  OGMJobSpawn *spawn;
  gint i, n, result;

  n = ogmrip_encoding_get_n_subp_streams (encoding);
  for (i = 0; i < n; i ++)
  {
    if ((encoding->priv->flags & OGMRIP_ENCODING_CANCELING) != 0)
      return OGMJOB_RESULT_CANCEL;

    data = ogmrip_encoding_get_nth_subp_data (encoding, i);
    if (data->options.codec != OGMRIP_TYPE_HARDSUB)
    {
      ogmjob_log_printf ("\nEncoding subp stream %02d\n", i + 1);
      ogmjob_log_printf ("-----------------------\n\n");

      spawn = ogmrip_encoding_get_subp_encoder (encoding, data, error);
      if (!spawn)
        return OGMJOB_RESULT_ERROR;

      if (encoding->priv->start_time > 0.0)
        ogmrip_codec_set_start  (OGMRIP_CODEC (spawn), encoding->priv->start_time);

      if (encoding->priv->play_length > 0.0)
        ogmrip_codec_set_play_length (OGMRIP_CODEC (spawn), encoding->priv->play_length);

      if (do_test)
        ogmrip_codec_set_unlink_on_unref (OGMRIP_CODEC (spawn), do_test);

      if (encoding->priv->profile)
        ogmrip_codec_set_profile (OGMRIP_CODEC (spawn), encoding->priv->profile);

      ogmrip_encoding_emit_task (encoding, spawn, (OGMRipOptions *) &data->options,
          OGMRIP_TASK_RUN, OGMRIP_TASK_SUBP, (OGMRipTaskDetail) 0);
      result = ogmjob_spawn_run (spawn, error);
      ogmrip_encoding_emit_task (encoding, spawn, (OGMRipOptions *) &data->options,
          OGMRIP_TASK_COMPLETE, OGMRIP_TASK_SUBP, (OGMRipTaskDetail) result);

      if (result != OGMJOB_RESULT_SUCCESS)
      {
        if (result == OGMJOB_RESULT_ERROR && error && !(*error))
          g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_UNKNOWN,
              _("Unknown error while extracting subtitle stream"));

        return result;
      }
    }
  }

  return OGMJOB_RESULT_SUCCESS;
}

/*
 * Creates the audio encoder
 */
static OGMJobSpawn *
ogmrip_encoding_get_audio_encoder (OGMRipEncoding *encoding, OGMRipAudioData *data, GError **error)
{
  OGMRipAudioDemuxer demuxer;
  OGMJobSpawn *spawn;
  gchar *output;

  static const gint sample_rate[] =
  { 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000 };
  static const gint channels[] = 
  { OGMDVD_AUDIO_CHANNELS_MONO, OGMDVD_AUDIO_CHANNELS_STEREO, OGMDVD_AUDIO_CHANNELS_SURROUND, OGMDVD_AUDIO_CHANNELS_5_1 };

  if (data->options.codec == G_TYPE_NONE)
    return NULL;

  output = ogmrip_fs_mktemp ("audio.XXXXXX", error);
  if (!output)
    return NULL;

  spawn = g_object_new (data->options.codec, "stream", data->stream, "output", output, NULL);
  g_free (output);

  ogmrip_codec_set_unlink_on_unref (OGMRIP_CODEC (spawn), !encoding->priv->keep_temp);
  ogmrip_codec_set_chapters (OGMRIP_CODEC (spawn), encoding->priv->start_chap, encoding->priv->end_chap);
  ogmrip_codec_set_progressive (OGMRIP_CODEC (spawn), encoding->priv->progressive);

  ogmrip_audio_codec_set_quality (OGMRIP_AUDIO_CODEC (spawn), data->options.quality);
  ogmrip_audio_codec_set_normalize (OGMRIP_AUDIO_CODEC (spawn), data->options.normalize);
  ogmrip_audio_codec_set_sample_rate (OGMRIP_AUDIO_CODEC (spawn), sample_rate[data->options.srate]);
  ogmrip_audio_codec_set_channels (OGMRIP_AUDIO_CODEC (spawn), channels[data->options.channels]);
  ogmrip_audio_codec_set_fast (OGMRIP_AUDIO_CODEC (spawn), !encoding->priv->ensure_sync);
  ogmrip_audio_codec_set_label (OGMRIP_AUDIO_CODEC (spawn), data->options.label);

  demuxer = OGMRIP_AUDIO_DEMUXER_AUTO;
  if (ogmrip_plugin_get_audio_codec_format (data->options.codec) == OGMRIP_FORMAT_COPY)
  {
    switch (ogmdvd_audio_stream_get_format (data->stream))
    {
      case OGMDVD_AUDIO_FORMAT_AC3:
        demuxer = OGMRIP_AUDIO_DEMUXER_AC3;
        break;
      case OGMDVD_AUDIO_FORMAT_DTS:
        demuxer = OGMRIP_AUDIO_DEMUXER_DTS;
        break;
      default:
        break;
    }
  }

  ogmrip_container_add_audio (ogmrip_encoding_get_container (encoding), OGMRIP_AUDIO_CODEC (spawn), demuxer, data->options.language);
  g_object_unref (spawn);

  return spawn;
}

/*
 * Encodes the audio streams
 */
static gint
ogmrip_encoding_encode_audio_streams (OGMRipEncoding *encoding, gboolean do_test, GError **error)
{
  OGMRipAudioData *data;
  OGMJobSpawn *spawn;

  gint i, n, result;

  n = ogmrip_encoding_get_n_audio_streams (encoding);
  for (i = 0; i < n; i ++)
  {
    if ((encoding->priv->flags & OGMRIP_ENCODING_CANCELING) != 0)
      return OGMJOB_RESULT_CANCEL;

    ogmjob_log_printf ("\nEncoding audio stream %02d\n", i + 1);
    ogmjob_log_printf ("------------------------\n\n");

    data = ogmrip_encoding_get_nth_audio_data (encoding, i);

    spawn = ogmrip_encoding_get_audio_encoder (encoding, data, error);
    if (!spawn)
      return OGMJOB_RESULT_ERROR;

    if (encoding->priv->start_time > 0.0)
      ogmrip_codec_set_start  (OGMRIP_CODEC (spawn), encoding->priv->start_time);

    if (encoding->priv->play_length > 0.0)
      ogmrip_codec_set_play_length (OGMRIP_CODEC (spawn), encoding->priv->play_length);

    if (do_test)
      ogmrip_codec_set_unlink_on_unref (OGMRIP_CODEC (spawn), do_test);

    if (encoding->priv->profile)
      ogmrip_codec_set_profile (OGMRIP_CODEC (spawn), encoding->priv->profile);

    ogmrip_encoding_emit_task (encoding, spawn, (OGMRipOptions *) &data->options,
        OGMRIP_TASK_RUN, OGMRIP_TASK_AUDIO, (OGMRipTaskDetail) 0);
    result = ogmjob_spawn_run (spawn, error);
    ogmrip_encoding_emit_task (encoding, spawn, (OGMRipOptions *) &data->options,
        OGMRIP_TASK_COMPLETE, OGMRIP_TASK_AUDIO, (OGMRipTaskDetail) result);

    if (result != OGMJOB_RESULT_SUCCESS)
    {
      if (result == OGMJOB_RESULT_ERROR && error && !(*error))
        g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_UNKNOWN,
            _("Unknown error while extracting audio stream"));
      return result;
    }
  }

  return OGMJOB_RESULT_SUCCESS;
}

static gint
ogmdvd_subp_stream_compare_lang (OGMDvdSubpStream *stream, gint lang)
{
  return ogmdvd_subp_stream_get_language (stream) - lang;
}

static void
ogmrip_encoding_bitrate (OGMRipEncoding *encoding, OGMRipVideoCodec *codec, GError **error)
{
  switch (encoding->priv->method)
  {
    case OGMRIP_ENCODING_BITRATE:
      ogmrip_video_codec_set_bitrate (codec, encoding->priv->bitrate);
      ogmjob_log_printf ("Constant bitrate: %d bps\n\n", encoding->priv->bitrate);
      break;
    case OGMRIP_ENCODING_QUANTIZER:
      ogmrip_video_codec_set_quantizer (codec, encoding->priv->quantizer);
      ogmrip_video_codec_set_passes (codec, 1);
      ogmjob_log_printf ("Constant quantizer: %lf\n\n", encoding->priv->quantizer);
      break;
    default:
      {
        gint64 nvsize, ohsize, rpsize;

        nvsize = ogmrip_container_get_nonvideo_size (encoding->priv->container);
        ohsize = ogmrip_container_get_overhead_size (encoding->priv->container);
        rpsize = ogmrip_encoding_get_rip_size (encoding);

        ogmrip_video_codec_autobitrate (codec, nvsize, ohsize, rpsize);

        encoding->priv->method = OGMRIP_ENCODING_BITRATE;
        encoding->priv->bitrate = ogmrip_video_codec_get_bitrate (codec);

        ogmjob_log_printf ("Automatic bitrate: %d (non-video: %" G_GINT64_FORMAT
            ", overhead: %" G_GINT64_FORMAT ", total: %" G_GINT64_FORMAT ", length: %lf)\n\n",
            encoding->priv->bitrate, nvsize, ohsize, rpsize,
            ogmrip_codec_get_length (OGMRIP_CODEC (codec), NULL));
      }
      break;
  }
}

static gint
ogmrip_encoding_crop (OGMRipEncoding *encoding, OGMRipVideoCodec *codec, GError **error)
{
  gint result;

  if ((encoding->priv->flags & OGMRIP_ENCODING_CANCELING) != 0)
    return OGMJOB_RESULT_CANCEL;

  switch (encoding->priv->crop_type)
  {
    case OGMRIP_OPTIONS_AUTOMATIC:
      ogmrip_encoding_emit_task (encoding, OGMJOB_SPAWN (codec),
          NULL, OGMRIP_TASK_RUN, OGMRIP_TASK_CROP, (OGMRipTaskDetail) 0);
      result = ogmrip_video_codec_autocrop (codec, 0) ? OGMJOB_RESULT_SUCCESS : OGMJOB_RESULT_CANCEL;
      ogmrip_encoding_emit_task (encoding, OGMJOB_SPAWN (codec),
          NULL, OGMRIP_TASK_COMPLETE, OGMRIP_TASK_CROP, (OGMRipTaskDetail) result);

      if (result != OGMJOB_RESULT_SUCCESS)
        return OGMJOB_RESULT_CANCEL;

      encoding->priv->crop_type = OGMRIP_OPTIONS_MANUAL;
      ogmrip_video_codec_get_crop_size (codec,
          &encoding->priv->crop_x, &encoding->priv->crop_y, &encoding->priv->crop_w, &encoding->priv->crop_h);
      ogmjob_log_printf ("\nAutomatic cropping: %d,%d %dx%d\n\n",
          encoding->priv->crop_x, encoding->priv->crop_y, encoding->priv->crop_w, encoding->priv->crop_h);
      break;
    case OGMRIP_OPTIONS_MANUAL:
      ogmrip_video_codec_set_crop_size (codec,
          encoding->priv->crop_x, encoding->priv->crop_y, encoding->priv->crop_w, encoding->priv->crop_h);
      ogmjob_log_printf ("Manual cropping: %d,%d %dx%d\n\n",
          encoding->priv->crop_x, encoding->priv->crop_y, encoding->priv->crop_w, encoding->priv->crop_h);
      break;
    default:
      ogmjob_log_printf ("No cropping\n\n");
      break;
  }

  return OGMJOB_RESULT_SUCCESS;
}

static void
ogmrip_encoding_scale (OGMRipEncoding *encoding, OGMRipVideoCodec *codec, GError **error)
{
  switch (encoding->priv->scale_type)
  {
    case OGMRIP_OPTIONS_AUTOMATIC:
      ogmrip_video_codec_set_bits_per_pixel (codec, encoding->priv->bpp);
      ogmrip_video_codec_autoscale (codec);

      encoding->priv->scale_type = OGMRIP_OPTIONS_MANUAL;
      ogmrip_video_codec_get_scale_size (codec,
          &encoding->priv->scale_w, &encoding->priv->scale_h);
      ogmjob_log_printf ("Automatic scaling: %dx%d\n\n",
          encoding->priv->scale_w, encoding->priv->scale_h);
      break;
    case OGMRIP_OPTIONS_MANUAL:
      ogmrip_video_codec_set_scale_size (codec,
          encoding->priv->scale_w, encoding->priv->scale_h);
      ogmjob_log_printf ("Manual scaling: %dx%d\n\n",
          encoding->priv->scale_w, encoding->priv->scale_h);
      break;
    default:
      ogmjob_log_printf ("No scaling\n\n");
      break;
  }
}

/*
 * Creates the video encoder
 */
static OGMRipVideoCodec *
ogmrip_encoding_get_video_encoder (OGMRipEncoding *encoding, GError **error)
{
  OGMJobSpawn *spawn;
  OGMRipContainer *container;

  gchar *output;
  guint start_delay, num, denom;

  output = ogmrip_fs_mktemp ("video.XXXXXX", error);
  if (!output)
    return NULL;

  container = ogmrip_encoding_get_container (encoding);

  spawn = g_object_new (encoding->priv->video_codec_type, "input", encoding->priv->title, "output", output, NULL);
  g_free (output);

  ogmrip_codec_set_unlink_on_unref (OGMRIP_CODEC (spawn), !encoding->priv->keep_temp);
  ogmrip_codec_set_chapters (OGMRIP_CODEC (spawn), encoding->priv->start_chap, encoding->priv->end_chap);
  ogmrip_codec_set_progressive (OGMRIP_CODEC (spawn), encoding->priv->progressive);
  ogmrip_codec_set_telecine (OGMRIP_CODEC (spawn), encoding->priv->telecine);

  ogmrip_video_codec_set_threads (OGMRIP_VIDEO_CODEC (spawn), encoding->priv->threads);
  ogmrip_video_codec_set_passes  (OGMRIP_VIDEO_CODEC (spawn), encoding->priv->passes);
  ogmrip_video_codec_set_quality (OGMRIP_VIDEO_CODEC (spawn), encoding->priv->quality);
  ogmrip_video_codec_set_denoise (OGMRIP_VIDEO_CODEC (spawn), encoding->priv->denoise);
  ogmrip_video_codec_set_deblock (OGMRIP_VIDEO_CODEC (spawn), encoding->priv->deblock);
  ogmrip_video_codec_set_dering  (OGMRIP_VIDEO_CODEC (spawn), encoding->priv->dering);
  ogmrip_video_codec_set_scaler  (OGMRIP_VIDEO_CODEC (spawn), encoding->priv->scaler);
  ogmrip_video_codec_set_turbo   (OGMRIP_VIDEO_CODEC (spawn), encoding->priv->turbo);

  if (encoding->priv->max_width > 0 && encoding->priv->max_height > 0)
    ogmrip_video_codec_set_max_size (OGMRIP_VIDEO_CODEC (spawn),
        encoding->priv->max_width, encoding->priv->max_height, encoding->priv->expand);

  if (encoding->priv->min_width > 0 && encoding->priv->min_height > 0)
    ogmrip_video_codec_set_min_size (OGMRIP_VIDEO_CODEC (spawn),
        encoding->priv->min_width, encoding->priv->min_height);

  ogmrip_encoding_get_aspect_ratio (encoding, &num, &denom);
  ogmrip_video_codec_set_aspect_ratio (OGMRIP_VIDEO_CODEC (spawn), num, denom); 

  ogmrip_video_codec_set_deinterlacer (OGMRIP_VIDEO_CODEC (spawn), encoding->priv->deint);
  ogmrip_video_codec_set_angle (OGMRIP_VIDEO_CODEC (spawn), encoding->priv->angle);

  if (encoding->priv->ensure_sync && encoding->priv->audio_streams)
  {
    OGMRipAudioData *audio_data = encoding->priv->audio_streams->data;
    ogmrip_video_codec_set_ensure_sync (OGMRIP_VIDEO_CODEC (spawn), audio_data->stream);
  }

  if (encoding->priv->subp_streams)
  {
    GSList *link;
    OGMRipSubpData *subp_data;

    for (link = encoding->priv->subp_streams; link; link = link->next)
    {
      subp_data = link->data;
      if (subp_data->options.codec == OGMRIP_TYPE_HARDSUB)
      {
        ogmrip_video_codec_set_hard_subp (OGMRIP_VIDEO_CODEC (spawn), subp_data->stream, subp_data->options.forced_subs);
        break;
      }
    }
  }
  else if (encoding->priv->audio_streams && encoding->priv->auto_subp)
  {
    OGMDvdSubpStream *subp_stream;
    gint lang;

    lang = ((OGMRipAudioData *) encoding->priv->audio_streams->data)->options.language;
    if (lang >= 0)
    {
      subp_stream = ogmdvd_title_find_subp_stream (encoding->priv->title,
          (GCompareFunc) ogmdvd_subp_stream_compare_lang, GINT_TO_POINTER (lang));

      if (subp_stream)
        ogmrip_video_codec_set_hard_subp (OGMRIP_VIDEO_CODEC (spawn), subp_stream, TRUE);
    }
  }

  start_delay = ogmrip_video_codec_get_start_delay (OGMRIP_VIDEO_CODEC (spawn));
  if (start_delay > 0)
    ogmrip_container_set_start_delay (container, start_delay);

  return OGMRIP_VIDEO_CODEC (spawn);
}

/*
 * Encodes the video stream
 */
static gint
ogmrip_encoding_encode_video_stream (OGMRipEncoding *encoding, OGMRipVideoCodec *codec, GError **error)
{
  gint result;

  if ((encoding->priv->flags & OGMRIP_ENCODING_CANCELING) != 0)
    return OGMJOB_RESULT_CANCEL;

  ogmjob_log_printf ("\nEncoding video title %d\n", ogmrip_encoding_get_title_nr (encoding) + 1);
  ogmjob_log_printf ("----------------------\n\n");

  if (encoding->priv->profile && encoding->priv->quality == OGMRIP_VIDEO_QUALITY_USER)
      ogmrip_codec_set_profile (OGMRIP_CODEC (codec), encoding->priv->profile);

  ogmrip_encoding_emit_task (encoding, OGMJOB_SPAWN (codec), NULL, OGMRIP_TASK_RUN, OGMRIP_TASK_VIDEO, (OGMRipTaskDetail) 0);
  result = ogmjob_spawn_run (OGMJOB_SPAWN (codec), error);
  ogmrip_encoding_emit_task (encoding, OGMJOB_SPAWN (codec), NULL, OGMRIP_TASK_COMPLETE, OGMRIP_TASK_VIDEO, (OGMRipTaskDetail) result);

  if (result == OGMJOB_RESULT_ERROR && error && !(*error))
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_UNKNOWN,
        _("Unknown error while extracting video stream"));

  return result;
}

/*
 * Merges the streams together
 */
static gint
ogmrip_encoding_merge (OGMRipEncoding *encoding, OGMRipContainer *container, GError **error)
{
  gchar *cwd = NULL;
  gint result;

  if ((encoding->priv->flags & OGMRIP_ENCODING_CANCELING) != 0)
    return OGMJOB_RESULT_CANCEL;

  if (!MPLAYER_CHECK_VERSION(1,0,0,8))
  {
    cwd = g_get_current_dir ();
    g_chdir (ogmrip_fs_get_tmp_dir ());
  }

  ogmjob_log_printf ("\nMerging\n");
  ogmjob_log_printf ("-------\n\n");

  ogmrip_encoding_emit_task (encoding, OGMJOB_SPAWN (container), NULL, OGMRIP_TASK_RUN, OGMRIP_TASK_MERGE, (OGMRipTaskDetail) 0);
  result = ogmjob_spawn_run (OGMJOB_SPAWN (container), error);
  ogmrip_encoding_emit_task (encoding, OGMJOB_SPAWN (container), NULL, OGMRIP_TASK_COMPLETE, OGMRIP_TASK_MERGE, (OGMRipTaskDetail) result);

  if (cwd)
  {
    g_chdir (cwd);
    g_free (cwd);
  }

  if (result == OGMJOB_RESULT_ERROR && error && !(*error))
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_UNKNOWN,
        _("Unknown error while merging"));

  return result;
}

/*
 * Tests the video stream
 */
static gint
ogmrip_encoding_test_video_stream (OGMRipEncoding *encoding, guint *bitrate, GError **error)
{
  OGMRipVideoCodec *codec;
  const gchar *output;
  gint result;

  if ((encoding->priv->flags & OGMRIP_ENCODING_CANCELING) != 0)
    return OGMJOB_RESULT_CANCEL;

  codec = ogmrip_encoding_get_video_encoder (encoding, error);
  if (!codec)
    return -1;

  ogmrip_codec_set_start  (OGMRIP_CODEC (codec), encoding->priv->start_time);
  ogmrip_codec_set_play_length (OGMRIP_CODEC (codec), encoding->priv->play_length);
  ogmrip_codec_set_unlink_on_unref (OGMRIP_CODEC (codec), TRUE);

  ogmrip_video_codec_set_scale_size (OGMRIP_VIDEO_CODEC (codec),
      encoding->priv->scale_w, encoding->priv->scale_h);
  ogmrip_video_codec_set_crop_size (OGMRIP_VIDEO_CODEC (codec),
      encoding->priv->crop_x, encoding->priv->crop_y, encoding->priv->crop_w, encoding->priv->crop_h);

  ogmrip_video_codec_set_quantizer (OGMRIP_VIDEO_CODEC (codec), 2);
  ogmrip_video_codec_set_passes (OGMRIP_VIDEO_CODEC (codec), 1);

  output = ogmrip_codec_get_output (OGMRIP_CODEC (codec));

  result = ogmjob_spawn_run (OGMJOB_SPAWN (codec), error);

  encoding->priv->fraction += encoding->priv->pulse_step;
  ogmrip_encoding_emit_task (encoding, OGMJOB_SPAWN (codec), NULL, OGMRIP_TASK_PROGRESS,
      OGMRIP_TASK_TEST, (OGMRipTaskDetail) encoding->priv->fraction);

  if (result != OGMJOB_RESULT_SUCCESS)
    *bitrate = 0;
  else
  {
    OGMRipFile *file;

    file = ogmrip_video_file_new (output, error);
    if (!file)
      return OGMJOB_RESULT_ERROR;

    ogmrip_file_set_unlink_on_unref (file, TRUE);
    *bitrate = ogmrip_video_file_get_bitrate (OGMRIP_VIDEO_FILE (file));

    ogmrip_file_unref (file);
  }

  return result;
}

static gint
ogmrip_encoding_test_internal (OGMRipEncoding *encoding, GError **error)
{
  OGMRipContainer *container;
  guint bitrate, user_bitrate, optimal_bitrate;
  gdouble length, start;
  gint result, files;
  gboolean relative;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), OGMJOB_RESULT_ERROR);
  g_return_val_if_fail (error == NULL || *error == NULL, OGMJOB_RESULT_ERROR);

  if ((encoding->priv->flags & OGMRIP_ENCODING_TESTED) != 0)
    return OGMJOB_RESULT_SUCCESS;

  user_bitrate = encoding->priv->bitrate;
  optimal_bitrate = 0;

  result = ogmrip_encoding_analyze_video_stream (encoding, error);
  if (result != OGMJOB_RESULT_SUCCESS)
    return result;

  container = ogmrip_encoding_get_container (encoding);

  ogmrip_encoding_open_log (encoding);

  ogmjob_log_printf ("TESTING: %s\n\n", ogmrip_encoding_get_label (encoding));

  length = ogmrip_encoding_get_rip_length (encoding);

  encoding->priv->pulse_step = SAMPLE_LENGTH / length;
  if (encoding->priv->method == OGMRIP_ENCODING_SIZE)
  {
    gdouble n = 1.0;

    n += ogmrip_encoding_get_n_audio_streams (encoding);
    n += ogmrip_encoding_get_n_subp_streams (encoding);

    encoding->priv->pulse_step /= n;
  }
  encoding->priv->fraction = 0.0;

  relative = ogmrip_encoding_get_relative (encoding);
  ogmrip_encoding_set_relative_internal (encoding, TRUE);

  ogmrip_encoding_emit_task (encoding, NULL, NULL, OGMRIP_TASK_RUN, OGMRIP_TASK_TEST, (OGMRipTaskDetail) 0);

  ogmrip_encoding_set_play_length (encoding, SAMPLE_LENGTH * SAMPLE_PERCENT);

  for (start = 0, files = 0; start + SAMPLE_LENGTH * SAMPLE_PERCENT < length; start += SAMPLE_LENGTH)
  {
    ogmrip_encoding_set_start_time (encoding, start);

    if (encoding->priv->bitrate == 0)
    {
      result = ogmrip_encoding_encode_subp_streams (encoding, TRUE, error);
      if (result != OGMJOB_RESULT_SUCCESS)
        break;

      result = ogmrip_encoding_encode_audio_streams (encoding, TRUE, error);
      if (result != OGMJOB_RESULT_SUCCESS)
        break;
    }

    result = ogmrip_encoding_test_video_stream (encoding, &bitrate, error);
    if (result != OGMJOB_RESULT_SUCCESS)
      break;

    if (bitrate > 0)
    {
      ogmjob_log_printf ("Optimal bitrate: %d\n\n", bitrate);

      optimal_bitrate += bitrate;

      if (encoding->priv->bitrate == 0)
      {
        gint64 nvsize, ohsize, rpsize;

        nvsize = ogmrip_container_get_nonvideo_size (container);
        ohsize = ogmrip_container_get_overhead_size (container);
        rpsize = ogmrip_encoding_get_rip_size (encoding);

        bitrate = (rpsize - nvsize - ohsize) * 8. / ogmrip_encoding_get_rip_length (encoding);
        user_bitrate += bitrate;

        ogmjob_log_printf ("User bitrate: %d (%" G_GINT64_FORMAT ", %" G_GINT64_FORMAT
            ", %" G_GINT64_FORMAT ")\n", bitrate, nvsize, ohsize, rpsize);

        ogmrip_container_foreach_audio (container,
            (OGMRipContainerCodecFunc) ogmrip_container_remove_audio, NULL);
        ogmrip_container_foreach_subp (container,
            (OGMRipContainerCodecFunc) ogmrip_container_remove_subp, NULL);
      }

      files ++;
    }
  }

  ogmrip_encoding_emit_task (encoding, NULL, NULL, OGMRIP_TASK_COMPLETE, OGMRIP_TASK_TEST, (OGMRipTaskDetail) 0);

  if (result == OGMJOB_RESULT_SUCCESS && files > 0)
  {
    if (encoding->priv->bitrate == 0)
      user_bitrate /= files;

    optimal_bitrate /= files;
  }

  ogmrip_encoding_set_relative_internal (encoding, relative);
  ogmrip_encoding_set_play_length (encoding, -1.0);
  ogmrip_encoding_set_start_time (encoding, -1.0);

  if (result == OGMJOB_RESULT_SUCCESS && user_bitrate > 0 && optimal_bitrate > 0)
  {
    gdouble ratio, user_quality, optimal_quality, cfactor;
    guint crop_x, crop_y, crop_w, crop_h, scale_w, scale_h;
    guint raw_w, raw_h, fnum, fdenom, anum, adenom;

    ogmjob_log_printf ("Number of files: %d\n\n", files);

    ogmdvd_title_get_size (encoding->priv->title, &raw_w, &raw_h);
    ogmjob_log_printf ("Raw resolution: %ux%u\n", raw_w, raw_h);

    ogmrip_encoding_get_crop (encoding, &crop_x, &crop_y, &crop_w, &crop_h);
    ogmjob_log_printf ("Crop size: %u,%u %ux%u\n\n", crop_x, crop_y, crop_w, crop_h);

    ogmrip_encoding_get_scale (encoding, &scale_w, &scale_h);

    ogmrip_encoding_get_aspect_ratio (encoding, &anum, &adenom);
    ratio = crop_w / (gdouble) crop_h * raw_h / raw_w * anum / adenom;

    ogmdvd_title_get_framerate (encoding->priv->title, &fnum, &fdenom);

    optimal_quality = optimal_bitrate / (gdouble) scale_w / scale_h / fnum * fdenom;
    user_quality = user_bitrate / (gdouble) scale_w / scale_h / fnum * fdenom;
    cfactor = user_quality / optimal_quality * 100;

    ogmjob_log_printf ("User bitrate: %u\n", user_bitrate);
    ogmjob_log_printf ("Optimal bitrate: %u\n", optimal_bitrate);
    ogmjob_log_printf ("Optimal quality: %.02lf\n\n", optimal_quality);

    ogmjob_log_printf ("Scale size: %ux%u\n", scale_w, scale_h);
    ogmjob_log_printf ("User quality: %.02lf\n\n", user_quality);
    ogmjob_log_printf ("Compressibility factor: %.0lf%%\n\n", cfactor);

    while (cfactor > 65.0)
    {
      scale_w += 16;
      scale_h = 16 * ROUND (scale_w / ratio / 16);

      user_quality = user_bitrate / (gdouble) scale_w / scale_h / fnum * fdenom;
      cfactor = user_quality / optimal_quality * 100;

      ogmjob_log_printf ("Scale size: %ux%u\n", scale_w, scale_h);
      ogmjob_log_printf ("User quality: %.02lf\n\n", user_quality);
      ogmjob_log_printf ("Compressibility factor: %.0lf%%\n\n", cfactor);
    }

    while (cfactor < 35.0)
    {
      scale_w -= 16;
      scale_h = 16 * ROUND (scale_w / ratio / 16);

      user_quality = user_bitrate / (gdouble) scale_w / scale_h / fnum * fdenom;
      cfactor = user_quality / optimal_quality * 100;

      ogmjob_log_printf ("Scale size: %ux%u\n", scale_w, scale_h);
      ogmjob_log_printf ("User quality: %.02lf\n\n", user_quality);
      ogmjob_log_printf ("Compressibility factor: %.0lf%%\n\n", cfactor);
    }

    ogmrip_encoding_set_scale_internal (encoding, OGMRIP_OPTIONS_MANUAL, scale_w, scale_h);

    OGMRIP_ENCODING_SET_FLAGS (encoding, OGMRIP_ENCODING_TESTED);
  }

  return result;
}

static gboolean
xmlNodeGetBoolContent (xmlNode *node)
{
  xmlChar *str;
  gboolean retval = FALSE;

  str = xmlNodeGetContent (node);
  if (str)
    retval = xmlStrEqual (str, (xmlChar *) "true") ? TRUE : FALSE;
  xmlFree (str);

  return retval;
}

static gint
xmlNodeGetIntContent (xmlNode *node)
{
  xmlChar *str;
  gint retval = -1;

  str = xmlNodeGetContent (node);
  if (str)
    retval = atoi ((char *) str);
  xmlFree (str);

  return retval;
}

static gint
xmlGetIntProp (xmlNode *node, const xmlChar *prop)
{
  xmlChar *str;
  gint retval = -1;

  str = xmlGetProp (node, prop);
  if (str)
    retval = atoi ((char *) str);
  xmlFree (str);

  return retval;
}

static void
ogmrip_encoding_import_title (OGMRipEncoding *encoding, xmlNode *node)
{
  xmlChar *device = NULL, *id = NULL;
  gint nr = -1, angle = -1;

  for (node = node->children; node; node = node->next)
  {
    if (xmlStrEqual (node->name, (xmlChar *) "device"))
      device = xmlNodeGetContent (node);
    else if (xmlStrEqual (node->name, (xmlChar *) "id"))
      id = xmlNodeGetContent (node);
    else if (xmlStrEqual (node->name, (xmlChar *) "nr"))
      nr = xmlNodeGetIntContent (node);
    else if (xmlStrEqual (node->name, (xmlChar *) "angle"))
      angle = xmlNodeGetIntContent (node);
  }

  if (device && id && nr >= 0)
  {
    encoding->priv->ntitle = nr;
    encoding->priv->device = (gchar *) device;
    encoding->priv->id = (gchar *) id;

    if (angle >= 0)
      encoding->priv->angle = MAX (1, angle);
  }
  else
  {
    if (device)
      xmlFree (device);

    if (id)
      xmlFree (id);
  }
}

static void
ogmrip_encoding_import_crop (OGMRipEncoding *encoding, xmlNode *node)
{
  gint type = -1;

  type = xmlGetIntProp (node, (xmlChar *) "type");
  if (type == OGMRIP_OPTIONS_NONE)
    ogmrip_encoding_set_crop (encoding, OGMRIP_OPTIONS_NONE, 0, 0, 0, 0);
  else if (type == OGMRIP_OPTIONS_MANUAL)
  {
    gint x = -1, y = -1, w = -1, h = -1;

    for (node = node->children; node; node = node->next)
    {
      if (xmlStrEqual (node->name, (xmlChar *) "x"))
        x = xmlNodeGetIntContent (node);
      else if (xmlStrEqual (node->name, (xmlChar *) "y"))
        y = xmlNodeGetIntContent (node);
      else if (xmlStrEqual (node->name, (xmlChar *) "w"))
        w = xmlNodeGetIntContent (node);
      else if (xmlStrEqual (node->name, (xmlChar *) "h"))
        h = xmlNodeGetIntContent (node);
    }

    if (x >= 0 && y >= 0 && w >= 0 && h >= 0)
      ogmrip_encoding_set_crop (encoding, type, x, y, w, h);
  }
}

static void
ogmrip_encoding_import_scale (OGMRipEncoding *encoding, xmlNode *node)
{
  gint type = -1;

  type = xmlGetIntProp (node, (xmlChar *) "type");
  if (type == OGMRIP_OPTIONS_NONE)
    ogmrip_encoding_set_scale (encoding, OGMRIP_OPTIONS_NONE, 0, 0);
  else if (type == OGMRIP_OPTIONS_MANUAL)
  {
    gint w = -1, h = -1;

    for (node = node->children; node; node = node->next)
    {
      if (xmlStrEqual (node->name, (xmlChar *) "w"))
        w = xmlNodeGetIntContent (node);
      else if (xmlStrEqual (node->name, (xmlChar *) "h"))
        h = xmlNodeGetIntContent (node);
    }

    if (w >= 0 && h >= 0)
      ogmrip_encoding_set_scale (encoding, type, w, h);
  }
}

static void
ogmrip_encoding_import_audio_files (OGMRipEncoding *encoding, xmlNode *node)
{
  xmlChar *filename;
  OGMRipFile *file;

  for (node = node->children; node; node = node->next)
  {
    if (xmlStrEqual (node->name, (xmlChar *) "filename"))
    {
      filename = xmlNodeGetContent (node);
      if (filename)
      {
        file = ogmrip_audio_file_new ((gchar *) filename, NULL);
        if (file)
        {
          ogmrip_encoding_add_audio_file (encoding, file, NULL);
          ogmrip_file_unref (file);
        }
        xmlFree (filename);
      }
    }
  }
}

static void
ogmrip_encoding_import_subp_files (OGMRipEncoding *encoding, xmlNode *node)
{
  xmlChar *filename;
  OGMRipFile *file;

  for (node = node->children; node; node = node->next)
  {
    if (xmlStrEqual (node->name, (xmlChar *) "filename"))
    {
      filename = xmlNodeGetContent (node);
      if (filename)
      {
        file = ogmrip_subp_file_new ((gchar *) filename, NULL);
        if (file)
        {
          ogmrip_encoding_add_subp_file (encoding, file, NULL);
          ogmrip_file_unref (file);
        }
        xmlFree (filename);
      }
    }
  }
}

static void
ogmrip_encoding_import_audio_streams (OGMRipEncoding *encoding, xmlNode *node)
{
  xmlNode *child;
  xmlChar *codec, *label;

  OGMRipAudioData *data;
  gint nr, lang, quality, srate, channels;
  gboolean normalize, defaults;

  for (node = node->children; node; node = node->next)
  {
    if (xmlStrEqual (node->name, (xmlChar *) "audio-stream"))
    {
      nr = xmlGetIntProp (node, (xmlChar *) "nr");
      if (nr >= 0)
      {
        codec = label = NULL;
        lang = quality = srate = channels = -1;
        normalize = defaults = TRUE;

        for (child = node->children; child; child = child->next)
        {
          if (xmlStrEqual (child->name, (xmlChar *) "codec"))
            codec = xmlNodeGetContent (child);
          else if (xmlStrEqual (child->name, (xmlChar *) "label"))
            label = xmlNodeGetContent (child);
          else if (xmlStrEqual (child->name, (xmlChar *) "quality"))
            quality = xmlNodeGetIntContent (child);
          else if (xmlStrEqual (child->name, (xmlChar *) "sample-rate"))
            srate = xmlNodeGetIntContent (child);
          else if (xmlStrEqual (child->name, (xmlChar *) "channels"))
            channels = xmlNodeGetIntContent (child);
          else if (xmlStrEqual (child->name, (xmlChar *) "language"))
            lang = xmlNodeGetIntContent (child);
          else if (xmlStrEqual (child->name, (xmlChar *) "normalize"))
            normalize = xmlNodeGetBoolContent (child);
          else if (xmlStrEqual (child->name, (xmlChar *) "defaults"))
            defaults = xmlNodeGetBoolContent (child);
        }

        if (codec)
        {
          data = g_new0 (OGMRipAudioData, 1);

          data->nr = nr;
          ogmrip_audio_options_init (&data->options);

          data->options.codec = ogmrip_plugin_get_audio_codec_by_name ((gchar *) codec);
          xmlFree (codec);

          if (label)
            data->options.label = g_strdup ((gchar *) label);

          if (quality >= 0)
            data->options.quality = quality;

          if (srate >= 0)
            data->options.srate = srate;

          if (channels >= 0)
            data->options.channels = channels;

          if (lang > 0)
            data->options.language = lang;

          data->options.normalize = normalize;
          data->options.defaults = defaults;

          encoding->priv->audio_streams = g_slist_append (encoding->priv->audio_streams, data);
        }

        if (label)
          xmlFree (label);
      }
    }
  }
}

static void
ogmrip_encoding_import_subp_streams (OGMRipEncoding *encoding, xmlNode *node)
{
  xmlNode *child;
  xmlChar *codec, *label;

  OGMRipSubpData *data;
  gint nr, lang, charset, newline;
  gboolean spell, forced, defaults;

  for (node = node->children; node; node = node->next)
  {
    if (xmlStrEqual (node->name, (xmlChar *) "subp-stream"))
    {
      nr = xmlGetIntProp (node, (xmlChar *) "nr");
      if (nr >= 0)
      {
        codec = label = NULL;
        lang = charset = newline = -1;
        spell = forced = defaults = FALSE;

        for (child = node->children; child; child = child->next)
        {
          if (xmlStrEqual (child->name, (xmlChar *) "codec"))
            codec = xmlNodeGetContent (child);
          if (xmlStrEqual (child->name, (xmlChar *) "label"))
            label = xmlNodeGetContent (child);
          else if (xmlStrEqual (child->name, (xmlChar *) "charset"))
            charset = xmlNodeGetIntContent (node);
          else if (xmlStrEqual (child->name, (xmlChar *) "newline"))
            newline = xmlNodeGetIntContent (child);
          else if (xmlStrEqual (child->name, (xmlChar *) "language"))
            lang = xmlNodeGetIntContent (child);
          else if (xmlStrEqual (child->name, (xmlChar *) "spell-check"))
            spell = xmlNodeGetBoolContent (child);
          else if (xmlStrEqual (child->name, (xmlChar *) "forced-subs"))
            forced = xmlNodeGetBoolContent (child);
          else if (xmlStrEqual (child->name, (xmlChar *) "defaults"))
            defaults = xmlNodeGetBoolContent (child);
        }

        if (codec)
        {
          data = g_new0 (OGMRipSubpData, 1);

          data->nr = nr;
          ogmrip_subp_options_init (&data->options);

          data->options.codec = ogmrip_plugin_get_subp_codec_by_name ((gchar *) codec);
          xmlFree (codec);

          if (label)
            data->options.label = g_strdup ((gchar *) label);

          if (charset >= 0)
            data->options.charset = charset;

          if (newline >= 0)
            data->options.newline = newline;

          if (lang > 0)
            data->options.language = lang;

          data->options.spell = spell;
          data->options.forced_subs = forced;
          data->options.defaults = defaults;

          encoding->priv->subp_streams = g_slist_append (encoding->priv->subp_streams, data);
        }

        if (label)
          xmlFree (label);
      }
    }
  }
}

static void
ogmrip_encoding_import_chapters (OGMRipEncoding *encoding, xmlNode *node)
{
  OGMRipChapterData *data;
  xmlChar *name;
  gint nr;

  for (node = node->children; node; node = node->next)
  {
    if (xmlStrEqual (node->name, (xmlChar *) "chapter"))
    {
      nr = xmlGetIntProp (node, (xmlChar *) "nr");
      if (nr >= 0)
      {
        name = xmlNodeGetContent (node);
        if (name)
        {
          ogmrip_encoding_set_chapter_label (encoding, nr, (gchar *) name);
          xmlFree (name);
        }
      }
    }
  }

  if (encoding->priv->chapters)
  {
    data = encoding->priv->chapters->data;
    if (data)
      encoding->priv->start_chap = data->nr;

    data = g_slist_last (encoding->priv->chapters)->data;
    if (data)
      encoding->priv->end_chap = data->nr;
  }
}

static gboolean
ogmrip_encoding_import (OGMRipEncoding *encoding, xmlNode *node)
{
  OGMRipProfile *profile;
  xmlChar *str;
  gint i;

  if (!g_str_equal (node->name, "encoding"))
    return FALSE;

  str = xmlGetProp (node, (xmlChar *) "profile");
  if (!str)
    return FALSE;

  /*
   * TODO wrong !
   */
  profile = ogmrip_profile_new ((gchar *) str);
  xmlFree (str);

  ogmrip_encoding_set_profile (encoding, profile);
  g_object_unref (profile);

  str = xmlGetProp (node, (xmlChar *) "name");
  if (str)
  {
    ogmrip_encoding_set_label (encoding, (gchar *) str);
    xmlFree (str);
  }

  str = xmlGetProp (node, (xmlChar *) "output");
  if (str)
  {
    ogmrip_encoding_set_filename (encoding, (gchar *) str);
    xmlFree (str);
  }

  for (node = node->children; node; node = node->next)
  {
    if (xmlStrEqual (node->name, (xmlChar *) "title"))
      ogmrip_encoding_import_title (encoding, node);
    else if (xmlStrEqual (node->name, (xmlChar *) "relative"))
      ogmrip_encoding_set_relative (encoding, xmlNodeGetBoolContent (node));
    else if (xmlStrEqual (node->name, (xmlChar *) "test"))
      ogmrip_encoding_set_test (encoding, xmlNodeGetBoolContent (node));
    else if (xmlStrEqual (node->name, (xmlChar *) "deinterlacer"))
    {
      if ((i = xmlNodeGetIntContent (node)) >= 0)
        ogmrip_encoding_set_deinterlacer (encoding, i);
    }
    else if (xmlStrEqual (node->name, (xmlChar *) "crop"))
      ogmrip_encoding_import_crop (encoding, node);
    else if (xmlStrEqual (node->name, (xmlChar *) "scale"))
      ogmrip_encoding_import_scale (encoding, node);
    else if (xmlStrEqual (node->name, (xmlChar *) "audio-files"))
      ogmrip_encoding_import_audio_files (encoding, node);
    else if (xmlStrEqual (node->name, (xmlChar *) "subp-files"))
      ogmrip_encoding_import_subp_files (encoding, node);
    else if (xmlStrEqual (node->name, (xmlChar *) "audio-streams"))
      ogmrip_encoding_import_audio_streams (encoding, node);
    else if (xmlStrEqual (node->name, (xmlChar *) "subp-streams"))
      ogmrip_encoding_import_subp_streams (encoding, node);
    else if (xmlStrEqual (node->name, (xmlChar *) "chapters"))
      ogmrip_encoding_import_chapters (encoding, node);
  }

  return TRUE;
}

static void
ogmrip_encoding_dump_file (OGMRipFile *file, FILE *f)
{
  gchar *filename;

  filename = ogmrip_file_get_filename (file);
  fprintf (f, "<filename>%s</filename>", filename);
  g_free (filename);
}

static void
ogmrip_encoding_dump_audio_stream (OGMRipAudioData *data, FILE *f)
{
  fprintf (f, "<audio-stream nr=\"%d\">", data->nr);
  fprintf (f, "<codec>%s</codec>", ogmrip_plugin_get_audio_codec_name (data->options.codec));
  fprintf (f, "<label>%s</label>", data->options.label);
  fprintf (f, "<quality>%d</quality>", data->options.quality);
  fprintf (f, "<sample-rate>%d</sample-rate>", data->options.srate);
  fprintf (f, "<channels>%d</channels>", data->options.channels);
  fprintf (f, "<language>%d</language>", data->options.language);
  fprintf (f, "<normalize>%s</normalize>", data->options.normalize ? "true" : "false");
  fprintf (f, "<defaults>%s</defaults>", data->options.defaults ? "true" : "false");
  fprintf (f, "</audio-stream>");
}

static void
ogmrip_encoding_dump_subp_stream (OGMRipSubpData *data, FILE *f)
{
  fprintf (f, "<subp-stream nr=\"%d\">", data->nr);
  fprintf (f, "<codec>%s</codec>", ogmrip_plugin_get_subp_codec_name (data->options.codec));
  fprintf (f, "<label>%s</label>", data->options.label);
  fprintf (f, "<charset>%d</charset>", data->options.charset);
  fprintf (f, "<newline>%d</newline>", data->options.newline);
  fprintf (f, "<language>%d</language>", data->options.language);
  fprintf (f, "<spell-check>%s</spell-check>", data->options.spell ? "true" : "false");
  fprintf (f, "<forced-subs>%s</forced-subs>", data->options.forced_subs ? "true" : "false");
  fprintf (f, "<defaults>%s</defaults>", data->options.defaults ? "true" : "false");
  fprintf (f, "</subp-stream>");
}

static void
ogmrip_encoding_dump_chapters (OGMRipEncoding *encoding, FILE *f)
{
  const gchar *label;
  guint nr;

  fprintf (f, "<chapters>");

  for (nr = encoding->priv->start_chap; nr <= encoding->priv->end_chap; nr ++)
    if ((label = ogmrip_encoding_get_chapter_label (encoding, nr)))
      fprintf (f, "<chapter nr=\"%d\">%s</chapter>", nr, label);

  fprintf (f, "</chapters>");
}

static gboolean
ogmrip_encoding_check_video_codec (OGMRipEncoding *encoding, GType container, GType codec, GError **error)
{
  gint format;

  if (codec == G_TYPE_NONE)
    return TRUE;
  
  format = ogmrip_plugin_get_video_codec_format (codec);
  if (format == OGMRIP_FORMAT_COPY)
    format = OGMRIP_FORMAT_MPEG2;

  if (!ogmrip_plugin_can_contain_format (container, format))
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_CONTAINER,
        _("The container and the video codec are incompatible."));

    return FALSE;
  }

  return TRUE;
}

static gboolean
ogmrip_encoding_check_audio_codec (OGMRipEncoding *encoding, GType container, OGMDvdAudioStream *stream, OGMRipAudioOptions *options, GError **error)
{
  gint audio_format, codec_format;

  if (options->codec == G_TYPE_NONE)
    return TRUE;

  switch (ogmdvd_audio_stream_get_format (stream))
  {
    case OGMDVD_AUDIO_FORMAT_AC3:
      audio_format = OGMRIP_FORMAT_AC3;
      break;
    case OGMDVD_AUDIO_FORMAT_DTS:
      audio_format = OGMRIP_FORMAT_DTS;
      break;
    case OGMDVD_AUDIO_FORMAT_LPCM:
      audio_format = OGMRIP_FORMAT_PCM;
      break;
    default:
      audio_format = -1;
      break;
  }

  if (audio_format == OGMRIP_FORMAT_DTS && !ogmrip_check_mplayer_dts ())
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_STREAMS,
        _("Your version of MPlayer does not support DTS streams")); 
    return FALSE;
  }

  codec_format = ogmrip_plugin_get_audio_codec_format (options->codec);
  if (codec_format == OGMRIP_FORMAT_COPY)
    codec_format = audio_format;

  if (codec_format < 0 || !ogmrip_plugin_can_contain_format (container, codec_format))
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_CONTAINER,
        _("The container and the audio codec are incompatible."));
    return FALSE;
  }

  return TRUE;
}

static gboolean
ogmrip_encoding_check_subp_codec (OGMRipEncoding *encoding, GType container, OGMDvdSubpStream *stream, OGMRipSubpOptions *options, GError **error)
{
  if (options->codec == G_TYPE_NONE)
    return TRUE;

  if (options->codec == OGMRIP_TYPE_HARDSUB)
  {
    GSList *link;
    OGMRipSubpData *data;

    for (link = encoding->priv->subp_streams; link; link = link->next)
    {
      data = link->data;
      if (data->options.codec == OGMRIP_TYPE_HARDSUB)
      {
        g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_STREAMS,
            _("It is not possible to hardcode multiple subtitle streams"));
        return FALSE;
      }
    }

    return TRUE;
  }

  if (!ogmrip_plugin_can_contain_subp (container, options->codec))
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_CONTAINER,
        _("The container and the subtitles codec are incompatible."));
    return FALSE;
  }

  return TRUE;
}

static gboolean
ogmrip_encoding_check_audio_file (OGMRipEncoding *encoding, GType container, OGMRipFile *file, GError **error)
{
  if (!ogmrip_plugin_can_contain_format (container, ogmrip_file_get_format (file)))
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_CONTAINER,
        _("The container and the audio file are incompatible."));
    return FALSE;
  }

  return TRUE;
}

static gboolean
ogmrip_encoding_check_subp_file (OGMRipEncoding *encoding, GType container, OGMRipFile *file, GError **error)
{
  if (!ogmrip_plugin_can_contain_format (container, ogmrip_file_get_format (file)))
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_CONTAINER,
        _("The container and the subtitles file are incompatible."));
    return FALSE;
  }

  return TRUE;
}

static gboolean
ogmrip_encoding_check_audio_streams (OGMRipEncoding *encoding, GType container, guint nstreams, GError **error)
{
  if (!ogmrip_plugin_can_contain_n_audio (container, nstreams))
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_AUDIO,
        _("The selected container does not support multiple audio streams"));
    return FALSE;
  }

  return TRUE;
}

static gboolean
ogmrip_encoding_check_subp_streams (OGMRipEncoding *encoding, GType container, guint nstreams, GError **error)
{
  if (!ogmrip_plugin_can_contain_n_subp (container, nstreams))
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_SUBP,
        _("The selected container does not support multiple subtitles streams"));
    return FALSE;
  }

  return TRUE;
}

static gboolean
ogmrip_encoding_check_container (OGMRipEncoding *encoding, GType container, GError **error)
{
  GSList *link;
  gint n;

  if (container == G_TYPE_NONE)
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_FATAL,
        _("No container has been selected."));
    return FALSE;
  }

  if (!ogmrip_encoding_check_video_codec (encoding, container, encoding->priv->video_codec_type, error))
    return FALSE;

  for (link = encoding->priv->audio_streams; link; link = link->next)
  {
    OGMRipAudioData *data;

    data = link->data;

    if (!ogmrip_encoding_check_audio_codec (encoding, container, data->stream, &data->options, error))
      return FALSE;
  }

  for (link = encoding->priv->subp_streams; link; link = link->next)
  {
    OGMRipSubpData *data;

    data = link->data;

    if (!ogmrip_encoding_check_subp_codec (encoding, container, data->stream, &data->options, error))
      return FALSE;
  }

  for (link = encoding->priv->audio_files; link; link = link->next)
    if (!ogmrip_encoding_check_audio_file (encoding, container, link->data, error))
      return FALSE;

  for (link = encoding->priv->subp_files; link; link = link->next)
    if (!ogmrip_encoding_check_subp_file (encoding, container, link->data, error))
      return FALSE;

  n = g_slist_length (encoding->priv->audio_streams) + g_slist_length (encoding->priv->audio_files);
  if (!ogmrip_encoding_check_audio_streams (encoding, container, n, error))
    return FALSE;

  n = g_slist_length (encoding->priv->subp_streams) + g_slist_length (encoding->priv->subp_files);
  if (!ogmrip_encoding_check_subp_streams (encoding, container, n, error))
    return FALSE;

  return TRUE;
}

/*
 * Check if there is enough space on the hard drive
 */
static gboolean
ogmrip_encoding_check_space (OGMRipEncoding *encoding, gint64 rip_size, gint64 tmp_size, GError **error)
{
  const gchar *filename;
  gchar *output_mount_point, *tmp_mount_point;
  gboolean retval = FALSE;

  if (rip_size + tmp_size == 0)
    return TRUE;

  filename = ogmrip_encoding_get_filename (encoding);

  output_mount_point = ogmrip_fs_get_mount_point (filename, error);
  if (!output_mount_point)
  {
    if (error && !(*error))
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
          _("Failed to get mount point of '%s'"), filename);
    return FALSE;
  }

  tmp_mount_point = ogmrip_fs_get_mount_point (ogmrip_fs_get_tmp_dir (), error);
  if (!tmp_mount_point)
  {
    g_free (output_mount_point);
    if (error && !(*error))
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
          _("Failed to get mount point of '%s'"), ogmrip_fs_get_tmp_dir ());
    return FALSE;
  }

  if (g_str_equal (tmp_mount_point, output_mount_point))
  {
    retval = rip_size + tmp_size < ogmrip_fs_get_left_space (output_mount_point, NULL);
    if (!retval)
    {
      gint64 needed = (rip_size + tmp_size) / 1024 / 1024;
      gchar *needed_str = g_strdup_printf ("%" G_GUINT64_FORMAT,needed);

      g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_SIZE, 
          _("Not enough space to store output and temporary files (%sMB needed)."), needed_str);
      g_free (needed_str);
    }
  }
  else
  {
    retval = tmp_size < ogmrip_fs_get_left_space (tmp_mount_point, NULL);
    if (!retval)
    {
      gint64 needed = tmp_size / 1024 / 1024;
      gchar *needed_str = g_strdup_printf ("%" G_GUINT64_FORMAT, needed);

      g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_SIZE, 
          _("Not enough space to store the temporary files (%sMB needed)."), needed_str);
      g_free (needed_str);
    }
    else
    {
      retval = rip_size < ogmrip_fs_get_left_space (output_mount_point, NULL);
      if (!retval)
      {
        gint64 needed = rip_size / 1024 / 1024;
        gchar *needed_str = g_strdup_printf ("%" G_GUINT64_FORMAT, needed);

        g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_SIZE, 
            _("Not enough space to store the output file (%sMB needed)."), needed_str);
        g_free (needed_str);
      }
    }
  }

  g_free (output_mount_point);
  g_free (tmp_mount_point);

  return retval;
}

/*
 * Public API
 */

/**
 * ogmrip_encoding_new:
 * @title: An #OGMDvdTitle
 * @filename: The output filename
 *
 * Creates a new #OGMRipEncoding.
 *
 * Returns: The newly created #OGMRipEncoding, or NULL
 */
OGMRipEncoding *
ogmrip_encoding_new (OGMDvdTitle *title, const gchar *filename)
{
  OGMRipEncoding *encoding;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (filename != NULL, NULL);

  encoding = g_object_new (OGMRIP_TYPE_ENCODING, NULL);

  ogmrip_encoding_set_title (encoding, title);
  ogmrip_encoding_set_filename (encoding, filename);

  ogmrip_encoding_get_rip_size  (encoding);
  ogmrip_encoding_get_dvd_size  (encoding);
  ogmrip_encoding_get_sync_size (encoding);

  return encoding;
}

/**
 * ogmrip_encoding_new_from_file:
 * @filename: An encoding file
 * @error: Return location for error
 *
 * Creates a new #OGMRipEncoding from an XML file.
 *
 * Returns: The newly created #OGMRipEncoding, or NULL
 */
OGMRipEncoding *
ogmrip_encoding_new_from_file (const gchar *filename, GError **error)
{
  OGMRipEncoding *encoding;
  OGMDvdTitle *title;
  OGMDvdDisc *disc;
  xmlNode *root;
  xmlDoc *doc;

  const gchar *id;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  doc = xmlParseFile (filename);
  if (!doc)
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_IMPORT,
        _("The file %s does not seem to contain a valid encoding"), filename);
    return NULL;
  }

  root = xmlDocGetRootElement (doc);

  if (!root)
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_IMPORT,
        _("The file %s does not seem to contain a valid encoding"), filename);
    xmlFreeDoc (doc);
    return NULL;
  }

  encoding = g_object_new (OGMRIP_TYPE_ENCODING, NULL);
  if (!ogmrip_encoding_import (encoding, root))
  {
    g_object_unref (encoding);
    encoding = NULL;
  }

  xmlFreeDoc (doc);

  if (!encoding)
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_IMPORT,
        _("The file %s does not seem to contain a valid encoding"), filename);
    return  NULL;
  }

  disc = ogmdvd_disc_new (encoding->priv->device, error);
  if (!disc)
     return NULL;

  id = ogmdvd_disc_get_id (disc);
  if (!g_str_equal (id, encoding->priv->id))
  {
    g_object_unref (encoding);
    ogmdvd_disc_unref (disc);
    g_set_error (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_ID, _("Device does not contain the expected DVD"));
    return NULL;
  }

  title = ogmdvd_disc_get_nth_title (disc, encoding->priv->ntitle);
  ogmrip_encoding_set_title (encoding, title);
  ogmdvd_title_unref (title);

  ogmrip_encoding_get_rip_size  (encoding);
  ogmrip_encoding_get_dvd_size  (encoding);
  ogmrip_encoding_get_sync_size (encoding);

  ogmdvd_disc_unref (disc);

  return encoding;
}

/**
 * ogmrip_encoding_dump:
 * @encoding: An #OGMRipEncoding
 * @filename: The output filename
 *
 * Dumps @encoding to an XML file.
 *
 * Returns: %TRUE on success, or %FALSE otherwise
 */
gboolean
ogmrip_encoding_dump (OGMRipEncoding *encoding, const gchar *filename)
{
  FILE *f;
  gchar *path;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

  f = fopen (filename, "w");
  if (!f)
    return FALSE;

  g_object_get (encoding->priv->profile, "path", &path, NULL);
  fprintf (f, "<encoding profile=\"%s\" name=\"%s\" output=\"%s\">", 
      path, encoding->priv->label, ogmrip_encoding_get_filename (encoding));
  g_free (path);

  fprintf (f, "<title>");
  fprintf (f, "<device>%s</device>", ogmrip_encoding_get_device (encoding));
  fprintf (f, "<id>%s</id>", encoding->priv->id);
  fprintf (f, "<nr>%d</nr>", encoding->priv->ntitle);
  fprintf (f, "<angle>%d</angle>", encoding->priv->angle);
  fprintf (f, "</title>");

  fprintf (f, "<relative>%s</relative>", encoding->priv->relative ? "true" : "false");
  fprintf (f, "<test>%s</test>", encoding->priv->test ? "true" : "false");

  fprintf (f, "<deinterlacer>%d</deinterlacer>", encoding->priv->deint);

  fprintf (f, "<crop type=\"%d\">", encoding->priv->crop_type);
  fprintf (f, "<x>%d</x>", encoding->priv->crop_x);
  fprintf (f, "<y>%d</y>", encoding->priv->crop_y);
  fprintf (f, "<w>%d</w>", encoding->priv->crop_w);
  fprintf (f, "<h>%d</h>", encoding->priv->crop_h);
  fprintf (f, "</crop>");

  fprintf (f, "<scale type=\"%d\">", encoding->priv->scale_type);
  fprintf (f, "<w>%d</w>", encoding->priv->scale_w);
  fprintf (f, "<h>%d</h>", encoding->priv->scale_h);
  fprintf (f, "</scale>");

  if (encoding->priv->audio_files)
  {
    fprintf (f, "<audio-files>");
    g_slist_foreach (encoding->priv->audio_files, (GFunc) ogmrip_encoding_dump_file, f);
    fprintf (f, "</audio-files>");
  }

  if (encoding->priv->subp_files)
  {
    fprintf (f, "<subp-files>");
    g_slist_foreach (encoding->priv->subp_files, (GFunc) ogmrip_encoding_dump_file, f);
    fprintf (f, "</subp-files>");
  }

  if (encoding->priv->audio_streams)
  {
    fprintf (f, "<audio-streams>");
    g_slist_foreach (encoding->priv->audio_streams, (GFunc) ogmrip_encoding_dump_audio_stream, f);
    fprintf (f, "</audio-streams>");
  }

  if (encoding->priv->subp_streams)
  {
    fprintf (f, "<subp-streams>");
    g_slist_foreach (encoding->priv->subp_streams, (GFunc) ogmrip_encoding_dump_subp_stream, f);
    fprintf (f, "</subp-streams>");
  }

  if (encoding->priv->chapters)
    ogmrip_encoding_dump_chapters (encoding, f);

  fprintf (f, "</encoding>");

  fclose (f);

  return TRUE;
}

/**
 * ogmrip_encoding_equal:
 * @encoding1: An #OGMRipEncoding
 * @encoding2: An #OGMRipEncoding
 *
 * Compares two encodings.
 *
 * Returns: %TRUE if the encodings are equal, %FALSE otherwise
 */
gboolean
ogmrip_encoding_equal (OGMRipEncoding *encoding1, OGMRipEncoding *encoding2)
{
  g_return_val_if_fail (encoding1 == NULL || OGMRIP_IS_ENCODING (encoding1), FALSE);
  g_return_val_if_fail (encoding2 == NULL || OGMRIP_IS_ENCODING (encoding2), FALSE);

  if (!encoding1 || !encoding2)
    return FALSE;

  if (encoding1 == encoding2)
    return TRUE;

  /*
   * start_chap & end_chap ?
   */

  return g_str_equal (encoding1->priv->id, encoding2->priv->id) && encoding1->priv->ntitle == encoding2->priv->ntitle;
}

/**
 * ogmrip_encoding_get_flags:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the flags of the encoding.
 *
 * Returns: The flags, or 0
 */
guint32
ogmrip_encoding_get_flags (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), 0);

  return encoding->priv->flags;
}

/**
 * ogmrip_encoding_get_id:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the id of the DVD source.
 *
 * Returns: The id, or NULL
 */
G_CONST_RETURN gchar *
ogmrip_encoding_get_id (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return encoding->priv->id;
}

/**
 * ogmrip_encoding_get_title:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the DVD title.
 *
 * Returns: An #OGMDvdTitle, or NULL
 */
OGMDvdTitle *
ogmrip_encoding_get_title (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return encoding->priv->title;
}

/**
 * ogmrip_encoding_get_profile:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the profile' section of the encoding.
 *
 * Returns: The profile , or NULL
 */
OGMRipProfile *
ogmrip_encoding_get_profile (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return encoding->priv->profile;
}

/**
 * ogmrip_encoding_set_profile:
 * @encoding: An #OGMRipEncoding
 * @profile: A profile
 *
 * Sets the profile of the encoding
 */
void
ogmrip_encoding_set_profile (OGMRipEncoding *encoding, OGMRipProfile *profile)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));
  g_return_if_fail (G_IS_SETTINGS (profile));

  g_object_ref (profile);
  if (encoding->priv->profile)
    g_object_unref (encoding->priv->profile);
  encoding->priv->profile = profile;

  OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
}

/**
 * ogmrip_encoding_get_label:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the name of the encoding.
 *
 * Returns: The name of the encoding, or NULL
 */
G_CONST_RETURN gchar *
ogmrip_encoding_get_label (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  if (encoding->priv->label)
    return encoding->priv->label;

  if (encoding->priv->title)
    return ogmdvd_disc_get_label (ogmdvd_title_get_disc (encoding->priv->title));

  return NULL;
}

/**
 * ogmrip_encoding_set_label:
 * @encoding: An #OGMRipEncoding
 * @label: The name of the encoding
 *
 * Sets the name of the encoding.
 */
void
ogmrip_encoding_set_label (OGMRipEncoding *encoding, const gchar *label)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->label)
    g_free (encoding->priv->label);
  encoding->priv->label = NULL;

  if (label)
    encoding->priv->label = g_strdup (label);

  if (encoding->priv->container)
    ogmrip_container_set_label (encoding->priv->container, label);
}

/**
 * ogmrip_encoding_get_chapter_label:
 * @encoding: An #OGMRipEncoding
 * @nr: A chapter number, counting from 0
 *
 * Gets the name of the chapter.
 *
 * Returns: The name of the chapter, or NULL
 */
G_CONST_RETURN gchar *
ogmrip_encoding_get_chapter_label (OGMRipEncoding *encoding, guint nr)
{
  OGMRipChapterData *data;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  data = ogmrip_encoding_get_chapter_data (encoding, nr);
  if (!data)
    return NULL;

  return data->label;
}

/**
 * ogmrip_encoding_set_chapter_label:
 * @encoding: An #OGMRipEncoding
 * @nr: A chapter number, counting from 0
 * @label: The name of the encoding
 *
 * Sets the name of the chapter.
 */
void
ogmrip_encoding_set_chapter_label (OGMRipEncoding *encoding, guint nr, const gchar *label)
{
  OGMRipChapterData *data;

  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));
  g_return_if_fail (label != NULL);

  data = ogmrip_encoding_get_chapter_data (encoding, nr);
  if (data)
  {
    if (data->label)
      g_free (data->label);

    data->label = g_strdup (label);
  }
  else
  {
    data = g_new0 (OGMRipChapterData, 1);
    data->nr = nr;
    data->label = g_strdup (label);

    encoding->priv->chapters = g_slist_insert_sorted (encoding->priv->chapters, data,
        (GCompareFunc) ogmrip_compare_chapter_data);
  }
}

/**
 * ogmrip_encoding_get_copy_dvd:
 * @encoding: An #OGMRipEncoding
 *
 * Gets whether the title should be copied on the hard drive.
 *
 * Returns: %TRUE if the title will be copied, %FALSE otherwise
 */
gboolean
ogmrip_encoding_get_copy_dvd (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->copy_dvd;
}

/**
 * ogmrip_encoding_set_copy_dvd:
 * @encoding: An #OGMRipEncoding
 * @copy_dvd: Whether to copy the title
 *
 * Sets whether to copy the title on the hard drive.
 */
void
ogmrip_encoding_set_copy_dvd (OGMRipEncoding *encoding, gboolean copy_dvd)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  encoding->priv->copy_dvd &= copy_dvd;
}

/**
 * ogmrip_encoding_get_ensure_sync:
 * @encoding: An #OGMRipEncoding
 *
 * Gets whether to ensure audio/video synchronization.
 *
 * Returns: %TRUE to ensure A/V sync, %FALSE otherwise
 */
gboolean
ogmrip_encoding_get_ensure_sync (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->ensure_sync;
}

/**
 * ogmrip_encoding_set_ensure_sync:
 * @encoding: An #OGMRipEncoding
 * @ensure_sync: Whether to ensure A/V sync
 *
 * Sets whether to ensure audio/video synchronization.
 */
void
ogmrip_encoding_set_ensure_sync (OGMRipEncoding *encoding, gboolean ensure_sync)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->ensure_sync != ensure_sync)
  {
    encoding->priv->ensure_sync = ensure_sync;

    encoding->priv->sync_size = 0;
    ogmrip_encoding_get_sync_size (encoding);
  }
}

/**
 * ogmrip_encoding_get_keep_tmp_files:
 * @encoding: An #OGMRipEncoding
 *
 * Gets whether to keep the temporary files.
 *
 * Returns: %TRUE to keep the temporary files, %FALSE otherwise
 */
gboolean
ogmrip_encoding_get_keep_tmp_files (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->keep_temp;
}

/**
 * ogmrip_encoding_set_keep_tmp_files:
 * @encoding: An #OGMRipEncoding
 * @keep_tmp_files: Whether to keep the temporary files
 *
 * Sets whether to keep the temporary files.
 */
void
ogmrip_encoding_set_keep_tmp_files (OGMRipEncoding *encoding, gboolean keep_tmp_files)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  encoding->priv->keep_temp = keep_tmp_files;
}

/**
 * ogmrip_encoding_get_filename:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the output file name.
 *
 * Returns: The output filename, or NULL
 */
G_CONST_RETURN gchar *
ogmrip_encoding_get_filename (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return encoding->priv->filename;
}

/**
 * ogmrip_encoding_set_filename:
 * @encoding: An #OGMRipEncoding
 * @filename: A filename
 *
 * Sets the output filename.
 */
void
ogmrip_encoding_set_filename (OGMRipEncoding *encoding, const gchar *filename)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));
  g_return_if_fail (filename != NULL);

  if (encoding->priv->filename)
    g_free (encoding->priv->filename);
  encoding->priv->filename = g_strdup (filename);

  if (encoding->priv->logfile)
    g_free (encoding->priv->logfile);
  encoding->priv->logfile = ogmrip_fs_set_extension (filename, "log");
}

/**
 * ogmrip_encoding_get_logfile:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the log filename.
 *
 * Returns: The log filename, or NULL
 */
G_CONST_RETURN gchar *
ogmrip_encoding_get_logfile (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return encoding->priv->logfile;
}

/**
 * ogmrip_encoding_get_threads:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the number of threads to use for the encoding.
 *
 * Returns: The number of thread, or -1
 */
gint
ogmrip_encoding_get_threads (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return encoding->priv->threads;
}

/**
 * ogmrip_encoding_set_threads:
 * @encoding: An #OGMRipEncoding
 * @threads: The number of threads
 *
 * Sets the number of threads to use for the encoding.
 */
void
ogmrip_encoding_set_threads (OGMRipEncoding *encoding, guint threads)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));
  g_return_if_fail (threads > 0);

  encoding->priv->threads = threads;
}

/**
 * ogmrip_encoding_get_angle:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the angle to encoding.
 *
 * Returns: The angle, or -1
 */
gint
ogmrip_encoding_get_angle (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return encoding->priv->angle;
}

/**
 * ogmrip_encoding_set_angle:
 * @encoding: An #OGMRipEncoding
 * @angle: An angle
 *
 * Sets the angle to encode.
 */
void
ogmrip_encoding_set_angle (OGMRipEncoding *encoding, guint angle)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->angle != angle)
  {
    encoding->priv->angle = angle;

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_container_type:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the type of the container.
 *
 * Returns: the type of the container
 */
GType
ogmrip_encoding_get_container_type (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), G_TYPE_NONE);

  return encoding->priv->container_type;
}

/**
 * ogmrip_encoding_set_container_type:
 * @encoding: An #OGMRipEncoding
 * @type: The type of a container
 * @error: Return location for error
 *
 * Sets the type of the container. If the container cannot contain the video,
 * audio, and/or subtitle codecs, an error will be set.
 *
 * Returns: %TRUE on success, %FALSE if an error was set
 */
gboolean
ogmrip_encoding_set_container_type (OGMRipEncoding *encoding, GType type, GError **error)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);
  g_return_val_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding), FALSE);
  g_return_val_if_fail (g_type_is_a (type, OGMRIP_TYPE_CONTAINER), FALSE);

  if (encoding->priv->container_type != type)
  {
    if (!ogmrip_encoding_check_container (encoding, type, error))
      return FALSE;

    encoding->priv->container_type = type;

    if (encoding->priv->container)
    {
      g_object_unref (encoding->priv->container);
      encoding->priv->container = NULL;
    }

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }

  return TRUE;
}

/**
 * ogmrip_encoding_get_fourcc:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the FOUR character code for the encoding.
 *
 * Returns: The FourCC, or NULL
 */
G_CONST_RETURN gchar *
ogmrip_encoding_get_fourcc (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return encoding->priv->fourcc;
}

/**
 * ogmrip_encoding_set_fourcc:
 * @encoding: An #OGMRipEncoding
 * @fourcc: A FourCC
 *
 * Sets the FOUR character code for the encoding.
 */
void
ogmrip_encoding_set_fourcc (OGMRipEncoding *encoding, const gchar *fourcc)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->fourcc)
  {
    g_free (encoding->priv->fourcc);
    encoding->priv->fourcc = NULL;
  }

  if (fourcc)
    encoding->priv->fourcc = g_strdup (fourcc);

  if (encoding->priv->container)
    ogmrip_container_set_fourcc (encoding->priv->container, fourcc);
}

/**
 * ogmrip_encoding_get_method:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the method of encoding.
 *
 * Returns: An #OGMRipEncodingMethod, or -1
 */
gint
ogmrip_encoding_get_method (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return encoding->priv->method;
}

/**
 * ogmrip_encoding_set_method:
 * @encoding: An #OGMRipEncoding
 * @method: An #OGMRipEncodingMethod
 *
 * Sets the method of encoding.
 */
void
ogmrip_encoding_set_method (OGMRipEncoding *encoding, OGMRipEncodingMethod method)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->method != method)
  {
    encoding->priv->method = method;

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_bitrate:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the average bitrate in bits/seconds for the encoding.
 *
 * Returns: The bitrate, or -1
 */
gint
ogmrip_encoding_get_bitrate (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return encoding->priv->bitrate;
}

/**
 * ogmrip_encoding_set_bitrate:
 * @encoding: An #OGMRipEncoding
 * @bitrate: The bitrate
 *
 * Sets the average bitrate in bits/second for the encoding. The bitrate will
 * be used only if the method of encoding is %OGMRIP_ENCODING_BITRATE.
 */
void
ogmrip_encoding_set_bitrate (OGMRipEncoding *encoding, guint bitrate)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  bitrate  = CLAMP (bitrate, 4000, 24000000);

  if (encoding->priv->bitrate != bitrate)
  {
    encoding->priv->bitrate = bitrate;

    if (encoding->priv->method == OGMRIP_ENCODING_BITRATE)
      OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_target_number:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the number of targets of the encoding.
 *
 * Returns: The number of targets, or -1
 */
gint
ogmrip_encoding_get_target_number (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return encoding->priv->target_number;
}

/**
 * ogmrip_encoding_set_target_number:
 * @encoding: An #OGMRipEncoding
 * @target_number: The number of targets
 *
 * Sets the number of targets of the encoding. This value will be used only if
 * the method of encoding is %OGMRIP_ENCODING_SIZE.
 */
void
ogmrip_encoding_set_target_number (OGMRipEncoding *encoding, guint target_number)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->target_number != target_number)
  {
    encoding->priv->target_number = target_number;

    if (encoding->priv->container)
      ogmrip_container_set_split (encoding->priv->container, target_number, encoding->priv->target_size);

    if (encoding->priv->method == OGMRIP_ENCODING_SIZE)
      OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_target_size:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the size of each target of the encoding.
 *
 * Returns: The size of the targets, or -1
 */
gint
ogmrip_encoding_get_target_size (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return encoding->priv->target_size;
}

/**
 * ogmrip_encoding_set_target_size:
 * @encoding: An #OGMRipEncoding
 * @target_size: The size of the targets
 *
 * Sets the size of each target of an encoding. This value will be used only if
 * the method of encoding is %OGMRIP_ENCODING_SIZE..
 */
void
ogmrip_encoding_set_target_size (OGMRipEncoding *encoding, guint target_size)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->target_size != target_size)
  {
    encoding->priv->target_size = target_size;

    if (encoding->priv->container)
      ogmrip_container_set_split (encoding->priv->container, encoding->priv->target_number, target_size);

    if (encoding->priv->method == OGMRIP_ENCODING_SIZE)
      OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_quantizer:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the quantizer of the encoding.
 *
 * Returns: The quantizer, or -1
 */
gdouble
ogmrip_encoding_get_quantizer (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1.0);

  return encoding->priv->quantizer;
}

/**
 * ogmrip_encoding_set_quantizer:
 * @encoding: An #OGMRipEncoding
 * @quantizer: The quantizer
 *
 * Sets the quantizer of the encoding. The quantizer will be used only if the
 * method of encoding is %OGMRIP_ENCODING_QUANTIZER.
 */
void
ogmrip_encoding_set_quantizer (OGMRipEncoding *encoding, gdouble quantizer)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->quantizer != quantizer)
  {
    encoding->priv->quantizer = CLAMP (quantizer, 0, 31);

    if (encoding->priv->method == OGMRIP_ENCODING_QUANTIZER)
      OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_video_codec_type:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the type of the video codec.
 *
 * Returns: the type of the video codec
 */
GType
ogmrip_encoding_get_video_codec_type (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), G_TYPE_NONE);

  return encoding->priv->video_codec_type;
}

/**
 * ogmrip_encoding_set_video_codec_type:
 * @encoding: An #OGMRipEncoding
 * @type: The type of a video codec
 * @error: Return location for error
 *
 * Sets the type of the vide codec. If the container cannot contain the video
 * codec, an error will be set.
 *
 * Returns: %TRUE on success, %FALSE if an error was set
 */
gboolean
ogmrip_encoding_set_video_codec_type (OGMRipEncoding *encoding, GType type, GError **error)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);
  g_return_val_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding), FALSE);
  g_return_val_if_fail (type == G_TYPE_NONE || g_type_is_a (type, OGMRIP_TYPE_VIDEO_CODEC), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (encoding->priv->video_codec_type != type)
  {
/*
    if (!ogmrip_encoding_check_video_codec (encoding, encoding->priv->container_type, type, error))
      return FALSE;
*/
    encoding->priv->video_codec_type = type;

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }

  return TRUE;
}

/**
 * ogmrip_encoding_get_can_crop:
 * @encoding: An #OGMRipEncoding
 *
 * Gets whether the video should be cropped.
 *
 * Returns: %TRUE if the video can be cropped, %FALSE otherwise
 */
gboolean
ogmrip_encoding_get_can_crop (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->can_crop;
}

/**
 * ogmrip_encoding_set_can_crop:
 * @encoding: An #OGMRipEncoding
 * @can_crop: %TRUE to crop the video
 *
 * Sets whether the video should be cropped.
 */
void
ogmrip_encoding_set_can_crop (OGMRipEncoding *encoding, gboolean can_crop)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  encoding->priv->can_crop = can_crop;
}

/**
 * ogmrip_encoding_get_can_scale:
 * @encoding: An #OGMRipEncoding
 *
 * Gets whether the video should be scaled.
 *
 * Returns: %TRUE if the video can be scaled, %FALSE otherwise
 */
gboolean
ogmrip_encoding_get_can_scale (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->can_scale;
}

/**
 * ogmrip_encoding_set_can_scale:
 * @encoding: An #OGMRipEncoding
 * @can_scale: %TRUE to scale the video
 *
 * Sets whether the video should be scaled.
 */
void
ogmrip_encoding_set_can_scale (OGMRipEncoding *encoding, gboolean can_scale)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  encoding->priv->can_scale = can_scale;
}

/**
 * ogmrip_encoding_get_deblock:
 * @encoding: An #OGMRipEncoding
 *
 * Gets whether a deblocking filter should be applied.
 *
 * Returns: %TRUE to deblock, %FALSE otherwise
 */
gboolean
ogmrip_encoding_get_deblock (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->deblock;
}

/**
 * ogmrip_encoding_set_deblock:
 * @encoding: An #OGMRipEncoding
 * @deblock: %TRUE to deblock
 *
 * Sets whether a deblocking filter should be applied.
 */
void
ogmrip_encoding_set_deblock (OGMRipEncoding *encoding, gboolean deblock)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->deblock != deblock)
  {
    encoding->priv->deblock = deblock;

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_denoise:
 * @encoding: An #OGMRipEncoding
 *
 * Gets whether a noise reducing filter should be applied.
 * 
 * Returns: %TRUE to denoise, %FALSE otherwise
 */
gboolean
ogmrip_encoding_get_denoise (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->denoise;
}

/**
 * ogmrip_encoding_set_denoise:
 * @encoding: An #OGMRipEncoding
 * @denoise: %TRUE to denoise
 *
 * Sets whether a noise reducing filter should be applied.
 */
void
ogmrip_encoding_set_denoise (OGMRipEncoding *encoding, gboolean denoise)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->denoise != denoise)
  {
    encoding->priv->denoise = denoise;

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_dering:
 * @encoding: An #OGMRipEncoding
 *
 * Gets whether a deringing filter should be applied.
 *
 * Returns: %TRUE to dering, %FALSE otherwise
 */
gboolean
ogmrip_encoding_get_dering (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->dering;
}

/**
 * ogmrip_encoding_set_dering:
 * @encoding: An #OGMRipEncoding
 * @dering: %TRUE to dering
 *
 * Sets whether a deringing filter should be applied.
 */
void
ogmrip_encoding_set_dering (OGMRipEncoding *encoding, gboolean dering)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->dering != dering)
  {
    encoding->priv->dering = dering;

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_turbo:
 * @encoding: An #OGMRipEncoding
 *
 * Gets whether to speed up first pass of multi-pass encodings.
 *
 * Returns: %TRUE to enable turbo, %FALSE, otherwise
 */
gboolean
ogmrip_encoding_get_turbo (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->turbo;
}

/**
 * ogmrip_encoding_set_turbo:
 * @encoding: An #OGMRipEncoding
 * @turbo: %TRUE to enable turbo
 *
 * Sets whether to speed up first pass of multi-pass encodings.
 */
void
ogmrip_encoding_set_turbo (OGMRipEncoding *encoding, gboolean turbo)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->turbo != turbo)
  {
    encoding->priv->turbo = turbo;

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_max_size:
 * @encoding: An #OGMRipEncoding
 * @width: A location to return the max width
 * @height: A location to return the max height
 * @expand: A location to return whether to expand the image
 *
 * Gets the maximum size of the image and whether the image should be expanded to this size.
 */
void
ogmrip_encoding_get_max_size (OGMRipEncoding *encoding, guint *width, guint *height, gboolean *expand)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  if (width)
    *width = encoding->priv->max_width;

  if (height)
    *height = encoding->priv->max_height;

  if (expand)
    *expand = encoding->priv->expand;
}

/**
 * ogmrip_encoding_set_max_size:
 * @encoding: An #OGMRipEncoding
 * @width: The max width
 * @height: The max height
 * @expand: %TRUE to expand the image
 *
 * Sets the maximum size of the image and whether the image should be expanded to this size.
 */
void
ogmrip_encoding_set_max_size (OGMRipEncoding *encoding, guint width, guint height, gboolean expand)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->max_width != width || encoding->priv->max_height != height || encoding->priv->expand != expand)
  {
    encoding->priv->max_width = width;
    encoding->priv->max_height = height;
    encoding->priv->expand = expand;

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_min_size:
 * @encoding: An #OGMRipEncoding
 * @width: A location to return the min width
 * @height: A location to return the min height
 *
 * Gets the minimum size of the image.
 */
void
ogmrip_encoding_get_min_size (OGMRipEncoding *encoding, guint *width, guint *height)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  if (width)
    *width = encoding->priv->min_width;

  if (height)
    *height = encoding->priv->min_height;
}

/**
 * ogmrip_encoding_set_min_size:
 * @encoding: An #OGMRipEncoding
 * @width: The min width
 * @height: The min height
 *
 * Sets the minimum size of the image.
 */
void
ogmrip_encoding_set_min_size (OGMRipEncoding *encoding, guint width, guint height)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->min_width != width || encoding->priv->min_height != height)
  {
    encoding->priv->min_width = width;
    encoding->priv->min_height = height;

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_passes:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the number of passes of the encoding.
 *
 * Returns: the number of passes, or -1
 */
gint
ogmrip_encoding_get_passes (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return encoding->priv->passes;
}

/**
 * ogmrip_encoding_set_passes:
 * @encoding: An #OGMRipEncoding
 * @passes: The number of passes
 *
 * Sets the number of passes of the encoding.
 */
void
ogmrip_encoding_set_passes (OGMRipEncoding *encoding, guint passes)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->passes != passes)
  {
    encoding->priv->passes = MAX (passes, 1);

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_quality:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the quality of the video options.
 *
 * Returns: An #OGMRipVideoQuality, or -1
 */
gint
ogmrip_encoding_get_quality (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return encoding->priv->quality;
}

/**
 * ogmrip_encoding_set_quality:
 * @encoding: An #OGMRipEncoding
 * @quality: An #OGMRipVideoQuality
 *
 * Sets the quality of the video options.
 */
void
ogmrip_encoding_set_quality (OGMRipEncoding *encoding, OGMRipVideoQuality quality)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->quality != quality)
  {
    encoding->priv->quality = quality;

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_scaler:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the software scaler of the encoding.
 *
 * Returns: An #OGMRipScalerType, or -1
 */
gint
ogmrip_encoding_get_scaler (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return encoding->priv->scaler;
}

/**
 * ogmrip_encoding_set_scaler:
 * @encoding: An #OGMRipEncoding
 * @scaler: An #OGMRipScalerType
 *
 * Sets the software scaler for the encoding.
 */
void
ogmrip_encoding_set_scaler (OGMRipEncoding *encoding, OGMRipScalerType scaler)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->scaler != scaler)
  {
    encoding->priv->scaler = scaler;

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_bits_per_pixel:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the number of bits per pixel of the encoding.
 *
 * Returns: The number of bits per pixel, or -1
 */
gdouble
ogmrip_encoding_get_bits_per_pixel (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1.0);

  return encoding->priv->bpp;
}

/**
 * ogmrip_encoding_set_bits_per_pixel:
 * @encoding: An #OGMRipEncoding
 * @bpp: The number of bits per pixel
 *
 * Sets the number of bits per pixel for the encoding.
 */
void
ogmrip_encoding_set_bits_per_pixel (OGMRipEncoding *encoding, gdouble bpp)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));
  g_return_if_fail (bpp > 0.0 && bpp <= 1.0);

  if (encoding->priv->bpp != bpp)
  {
    encoding->priv->bpp = bpp;

    if (encoding->priv->method == OGMRIP_ENCODING_SIZE)
      OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_chapters:
 * @encoding: An #OGMRipEncoding
 * @start_chap: A location to return the first chapter
 * @end_chap: A location to return the last chapter
 *
 * Gets the first and last chapters of the encoding.
 */
void
ogmrip_encoding_get_chapters (OGMRipEncoding *encoding, guint *start_chap, gint *end_chap)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  if (start_chap)
    *start_chap = encoding->priv->start_chap;

  if (end_chap)
    *end_chap = encoding->priv->end_chap;
}

/**
 * ogmrip_encoding_set_chapters:
 * @encoding: An #OGMRipEncoding
 * @start_chap: The first chapter
 * @end_chap: The last chapter
 *
 * Sets the first and last chapters of the encoding.
 */
void
ogmrip_encoding_set_chapters (OGMRipEncoding *encoding, guint start_chap, gint end_chap)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (end_chap < 0)
    end_chap = ogmdvd_title_get_n_chapters (encoding->priv->title) - 1;

  if (encoding->priv->start_chap != start_chap || encoding->priv->end_chap != end_chap)
  {
    encoding->priv->start_chap = start_chap;
    encoding->priv->end_chap = end_chap;

    encoding->priv->sync_size = 0;
    encoding->priv->rip_length = 0;
    encoding->priv->rip_size = 0;

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_chapters_language:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the language of the name of the chapters.
 *
 * Returns: A language code, or -1
 */
gint
ogmrip_encoding_get_chapters_language (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return encoding->priv->chap_lang;
}

/**
 * ogmrip_encoding_set_chapters_language:
 * @encoding: An #OGMRipEncoding
 * @language: A language code
 *
 * Sets the language of the name of the chapters.
 */
void
ogmrip_encoding_set_chapters_language (OGMRipEncoding *encoding, guint language)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  encoding->priv->chap_lang = language;
}

/**
 * ogmrip_encoding_get_relative:
 * @encoding: An #OGMRipEncoding
 *
 * Gets whether the bitrate is computed relatively to the full length of the
 * title, or to the length of the selected chapters only.
 *
 * Returns: %TRUE for relative, %FALSE otherwise
 */
gboolean
ogmrip_encoding_get_relative (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->relative;
}

/**
 * ogmrip_encoding_set_relative:
 * @encoding: An #OGMRipEncoding
 * @relative: %TRUE for relative
 *
 * Sets whether the bitrate is computed relatively to the full length of the
 * title, or to the length of the selected chapters only.
 */
void
ogmrip_encoding_set_relative (OGMRipEncoding *encoding, gboolean relative)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  ogmrip_encoding_set_relative_internal (encoding, relative);

  OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
}

/**
 * ogmrip_encoding_get_test:
 * @encoding: An #OGMRipEncoding
 *
 * Gets whether to automatically perform a compressibility test.
 *
 * Returns: %TRUE to perform a compressibility test, %FALSE otherwise
 */
gboolean
ogmrip_encoding_get_test (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->test;
}

/**
 * ogmrip_encoding_set_test:
 * @encoding: An #OGMRipEncoding
 * @test: %TRUE to perform a compressibility test
 *
 * Sets whether to automatically perform a compressibility test.
 */
void
ogmrip_encoding_set_test (OGMRipEncoding *encoding, gboolean test)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  encoding->priv->test = test;
}

/**
 * ogmrip_encoding_get_auto_subp:
 * @encoding: An #OGMRipEncoding
 *
 * Gets whether to automatically hardcode the first subtitle stream having
 * the same language as the first audio track.
 *
 * Returns: %TRUE to hardcode a subtitle track, %FALSE otherwise
 */
gboolean
ogmrip_encoding_get_auto_subp (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);

  return encoding->priv->auto_subp;
}

/**
 * ogmrip_encoding_set_auto_subp:
 * @encoding: An #OGMRipEncoding
 * @auto_subp: %TRUE to hardcode a subtitle track
 *
 * Sets whether to automatically hardcode the first subtitle stream having
 * the same language as the first audio track.
 */
void
ogmrip_encoding_set_auto_subp (OGMRipEncoding *encoding, gboolean auto_subp)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  encoding->priv->auto_subp = auto_subp;
}

/**
 * ogmrip_encoding_get_deinterlacer:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the deinterlacer.
 *
 * Returns: An #OGMRipDeintType, or -1
 */
gint
ogmrip_encoding_get_deinterlacer (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return encoding->priv->deint;
}

/**
 * ogmrip_encoding_set_deinterlacer:
 * @encoding: An #OGMRipEncoding
 * @deint: An #OGMRipDeintType
 *
 * Sets the deinterlacer.
 */
void
ogmrip_encoding_set_deinterlacer (OGMRipEncoding *encoding, OGMRipDeintType deint)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  encoding->priv->deint = deint;
}

/**
 * ogmrip_encoding_get_aspect_ratio:
 * @encoding: An #OGMRipEncoding
 * @numerator: A location to return the numerator of the aspect ratio
 * @denominator: A location to resturn the denominator of the aspect ratio
 *
 * Gets the aspect ratio of the encoding.
 */
void
ogmrip_encoding_get_aspect_ratio (OGMRipEncoding *encoding, guint *numerator, guint *denominator)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  if (!encoding->priv->aspect_num || !encoding->priv->aspect_denom)
    ogmdvd_title_get_aspect_ratio (encoding->priv->title, &encoding->priv->aspect_num, &encoding->priv->aspect_denom);

  if (numerator)
    *numerator = encoding->priv->aspect_num;

  if (denominator)
    *denominator = encoding->priv->aspect_denom;
}

/**
 * ogmrip_encoding_set_aspect_ratio:
 * @encoding: An #OGMRipEncoding
 * @numerator: The numerator of the aspect ratio
 * @denominator: The denominator of the aspect ratio
 *
 * Sets the aspect ratio for the encoding.
 */
void
ogmrip_encoding_set_aspect_ratio (OGMRipEncoding *encoding, guint numerator, guint denominator)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  encoding->priv->aspect_num = numerator;
  encoding->priv->aspect_denom = denominator;
}

/**
 * ogmrip_encoding_get_crop:
 * @encoding: An #OGMRipEncoding:
 * @x: A location to return the cropped horizontal position
 * @y: A location to return the cropped vertical position
 * @w: A location to return the cropped width
 * @h: A location to return the cropped height
 *
 * Gets the cropping parameters of the encoding.
 *
 * Returns: An #OGMRipOptionsType, or -1
 */
gint
ogmrip_encoding_get_crop (OGMRipEncoding *encoding, guint *x, guint *y, guint *w, guint *h)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  if (x)
    *x = encoding->priv->crop_x;

  if (y)
    *y = encoding->priv->crop_y;

  if (w)
    *w = encoding->priv->crop_w;

  if (h)
    *h = encoding->priv->crop_h;

  return encoding->priv->crop_type;
}

/**
 * ogmrip_encoding_set_crop:
 * @encoding: An #OGMRipEncoding
 * @type: An #OGMRipOptionsType
 * @x: The cropped horizontal position
 * @y: The cropped vertical position
 * @w: The cropped width
 * @h: The cropped height
 *
 * Sets the cropping parameters for the encoding.
 */
void
ogmrip_encoding_set_crop (OGMRipEncoding *encoding, OGMRipOptionsType type, guint x, guint y, guint w, guint h)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->crop_x != x || encoding->priv->crop_y != y ||
      encoding->priv->crop_w != w || encoding->priv->crop_h != h ||
      encoding->priv->crop_type != type)
  {
    encoding->priv->crop_x = x;
    encoding->priv->crop_y = y;
    encoding->priv->crop_w = w;
    encoding->priv->crop_h = h;

    if (!x && !y && !w && !h)
      type = OGMRIP_OPTIONS_NONE;

    encoding->priv->crop_type = type;

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_get_scale:
 * @encoding: An #OGMRipEncoding
 * @w: A location to return the scaled width
 * @h: A location to resturn the scaled height
 *
 * Gets the scaling parameters of the encoding.
 *
 * Returns: An #OGMRipOptionsType, or -1
 */
gint
ogmrip_encoding_get_scale (OGMRipEncoding *encoding, guint *w, guint *h)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  if (w)
    *w = encoding->priv->scale_w;

  if (h)
    *h = encoding->priv->scale_h;

  return encoding->priv->scale_type;
}

/**
 * ogmrip_encoding_set_scale:
 * @encoding: An #OGMRipEncoding
 * @type: An #OGMRipOptionsType
 * @w: The scaled width
 * @h: The scaled height
 *
 * Sets the scaling parameters for the encoding.
 */
void
ogmrip_encoding_set_scale (OGMRipEncoding *encoding, OGMRipOptionsType type, guint w, guint h)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding));

  if (encoding->priv->scale_w != w || encoding->priv->scale_h != h ||
      encoding->priv->scale_type != type)
  {
    ogmrip_encoding_set_scale_internal (encoding, type, w, h);

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED | OGMRIP_ENCODING_EXTRACTED);
  }
}

/**
 * ogmrip_encoding_add_audio_stream:
 * @encoding: An #OGMRipEncoding
 * @stream: An #OGMDvdAudioStream
 * @options: An #OGMRipAudioOptions
 * @error: Return location for error
 *
 * Adds an audio stream to the encoding.
 *
 * Returns: %TRUE on success, %FALSE if an error was set
 */
gboolean
ogmrip_encoding_add_audio_stream (OGMRipEncoding *encoding, OGMDvdAudioStream *stream, OGMRipAudioOptions *options, GError **error)
{
  OGMRipAudioData *data;
  guint n;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);
  g_return_val_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding), FALSE);
  g_return_val_if_fail (stream != NULL, FALSE);
  g_return_val_if_fail (options != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (!ogmrip_encoding_check_audio_codec (encoding, encoding->priv->container_type, stream, options, error))
    return FALSE;

  n = g_slist_length (encoding->priv->audio_streams) + g_slist_length (encoding->priv->audio_files) + 1;
  if (!ogmrip_encoding_check_audio_streams (encoding, encoding->priv->container_type, n, error))
    return FALSE;

  data = g_new0 (OGMRipAudioData, 1);

  data->nr = ogmdvd_stream_get_nr (OGMDVD_STREAM (stream));
  data->options = *options;

  if (options->label)
    data->options.label = g_strdup (options->label);

  if (encoding->priv->title)
    data->stream = ogmdvd_title_get_nth_audio_stream (encoding->priv->title, data->nr);

  encoding->priv->audio_streams = g_slist_append (encoding->priv->audio_streams, data);

  if (encoding->priv->method == OGMRIP_ENCODING_SIZE)
    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED);

  OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_EXTRACTED);

  return TRUE;
}

/**
 * ogmrip_encoding_get_n_audio_streams:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the number of audio streams of the encoding.
 * 
 * Returns: The number of audio streams, or -1
 */
gint
ogmrip_encoding_get_n_audio_streams (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return g_slist_length (encoding->priv->audio_streams);
}

/**
 * ogmrip_encoding_get_nth_audio_stream:
 * @encoding: An #OGMRipEncoding
 * @n: The position of the stream, counting from 0
 *
 * Gets the audio stream at the given position.
 *
 * Returns: An #OGMDvdAudioStream, or NULL
 */
OGMDvdAudioStream *
ogmrip_encoding_get_nth_audio_stream (OGMRipEncoding *encoding, guint n)
{
  OGMRipAudioData *data;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  data = g_slist_nth_data (encoding->priv->audio_streams, n);
  if (!data)
    return NULL;

  if (!data->stream && encoding->priv->title)
    data->stream = ogmdvd_title_get_nth_audio_stream (encoding->priv->title, data->nr);

  return data->stream;
}

/**
 * ogmrip_encoding_foreach_audio_streams:
 * @encoding: An #OGMRipEncoding
 * @func: The function to call with each audio streams
 * @data: User data to pass to the function
 *
 * Calls the given function for each audio streams.
 */
void
ogmrip_encoding_foreach_audio_streams (OGMRipEncoding *encoding, OGMRipEncodingAudioFunc func, gpointer data)
{
  GSList *link;
  OGMRipAudioData *audio_data;

  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (func != NULL);

  for (link = encoding->priv->audio_streams; link; link = link->next)
  {
    audio_data = link->data;

    if (!audio_data->stream && encoding->priv->title)
      audio_data->stream = ogmdvd_title_get_nth_audio_stream (encoding->priv->title, audio_data->nr);

    (* func) (encoding, audio_data->stream, &audio_data->options, data);
  }
}

/**
 * ogmrip_encoding_get_nth_audio_options:
 * @encoding: An #OGMRipEncoding
 * @n: The position of the stream, counting from 0
 * @options: A location to return the options
 *
 * Gets the options of the audio stream at the given position.
 *
 * Returns: %TRUE on success, %FALSE if there is no audio stream at the given position
 */
gboolean
ogmrip_encoding_get_nth_audio_options (OGMRipEncoding *encoding, guint n, OGMRipAudioOptions *options)
{
  OGMRipAudioData *data;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);
  g_return_val_if_fail (options != NULL, FALSE);

  data = g_slist_nth_data (encoding->priv->audio_streams, n);
  if (!data)
    return FALSE;

  *options = data->options;

  if (data->options.label)
    data->options.label = g_strdup (data->options.label);

  return TRUE;
}

/**
 * ogmrip_encoding_set_nth_audio_options:
 * @encoding: An #OGMRipEncoding
 * @n: The position of the stream, counting from 0
 * @options: An #OGMRipAudioOptions
 * @error: Return location for error
 *
 * Sets the options of the audio stream at the given position.
 *
 * Returns: %TRUE on success, %FALSE if there is no audio stream at the given position
 */
gboolean
ogmrip_encoding_set_nth_audio_options (OGMRipEncoding *encoding, guint n, OGMRipAudioOptions *options, GError **error)
{
  OGMRipAudioData *data;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);
  g_return_val_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding), FALSE);
  g_return_val_if_fail (options != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  data = g_slist_nth_data (encoding->priv->audio_streams, n);
  if (data)
  {
/*
    if (options->codec != data->options.codec &&
        !ogmrip_encoding_check_audio_codec (encoding, encoding->priv->container_type, data->stream, options, error))
      return FALSE;
*/
    data->options = *options;

    if (data->options.label)
      g_free (data->options.label);

    data->options.label = options->label ? g_strdup (options->label) : NULL;
  }

  return TRUE;
}

/**
 * ogmrip_encoding_add_subp_stream:
 * @encoding: An #OGMRipEncoding
 * @stream: An #OGMDvdSubpStream
 * @options: An #OGMRipSubpOptions
 * @error: Return location for error
 *
 * Adds a subp stream to the encoding.
 *
 * Returns: %TRUE on success, %FALSE if an error was set
 */
gboolean
ogmrip_encoding_add_subp_stream (OGMRipEncoding *encoding, OGMDvdSubpStream *stream, OGMRipSubpOptions *options, GError **error)
{
  OGMRipSubpData *data;
  guint n;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);
  g_return_val_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding), FALSE);
  g_return_val_if_fail (stream != NULL, FALSE);
  g_return_val_if_fail (options != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (!ogmrip_encoding_check_subp_codec (encoding, encoding->priv->container_type, stream, options, error))
    return FALSE;

  n = g_slist_length (encoding->priv->subp_streams) + g_slist_length (encoding->priv->subp_files) + 1;
  if (!ogmrip_encoding_check_subp_streams (encoding, encoding->priv->container_type, n, error))
    return FALSE;

  data = g_new0 (OGMRipSubpData, 1);

  data->nr = ogmdvd_stream_get_nr (OGMDVD_STREAM (stream));

  data->options = *options;

  if (options->label)
    data->options.label = g_strdup (options->label);

  if (encoding->priv->title)
    data->stream = ogmdvd_title_get_nth_subp_stream (encoding->priv->title, data->nr);

  encoding->priv->subp_streams = g_slist_append (encoding->priv->subp_streams, data);

  if (encoding->priv->method == OGMRIP_ENCODING_SIZE)
    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED);

  OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_EXTRACTED);

  return TRUE;
}

/**
 * ogmrip_encoding_get_n_subp_streams:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the number of subp streams of the encoding.
 * 
 * Returns: The number of subp streams, or -1
 */
gint
ogmrip_encoding_get_n_subp_streams (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return g_slist_length (encoding->priv->subp_streams);
}

/**
 * ogmrip_encoding_get_nth_subp_stream:
 * @encoding: An #OGMRipEncoding
 * @n: The position of the stream, counting from 0
 *
 * Gets the subp stream at the given position.
 *
 * Returns: An #OGMDvdSubpStream, or NULL
 */
OGMDvdSubpStream *
ogmrip_encoding_get_nth_subp_stream (OGMRipEncoding *encoding, guint n)
{
  OGMRipSubpData *data;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  data = g_slist_nth_data (encoding->priv->subp_streams, n);
  if (!data)
    return NULL;

  if (!data->stream && encoding->priv->title)
    data->stream = ogmdvd_title_get_nth_subp_stream (encoding->priv->title, data->nr);

  return data->stream;
}

/**
 * ogmrip_encoding_foreach_subp_streams:
 * @encoding: An #OGMRipEncoding
 * @func: The function to call with each subp streams
 * @data: User data to pass to the function
 *
 * Calls the given function for each subp streams.
 */
void
ogmrip_encoding_foreach_subp_streams (OGMRipEncoding *encoding, OGMRipEncodingSubpFunc func, gpointer data)
{
  GSList *link;
  OGMRipSubpData *subp_data;

  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (func != NULL);

  for (link = encoding->priv->subp_streams; link; link = link->next)
  {
    subp_data = link->data;

    if (!subp_data->stream && encoding->priv->title)
      subp_data->stream = ogmdvd_title_get_nth_subp_stream (encoding->priv->title, subp_data->nr);

    (* func) (encoding, subp_data->stream, &subp_data->options, data);
  }
}

/**
 * ogmrip_encoding_get_nth_subp_options:
 * @encoding: An #OGMRipEncoding
 * @n: The position of the stream, counting from 0
 * @options: A location to return the options
 *
 * Gets the options of the subp stream at the given position.
 *
 * Returns: %TRUE on success, %FALSE if there is no subp stream at the given position
 */
gboolean
ogmrip_encoding_get_nth_subp_options (OGMRipEncoding *encoding, guint n, OGMRipSubpOptions *options)
{
  OGMRipSubpData *data;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);
  g_return_val_if_fail (options != NULL, FALSE);

  data = g_slist_nth_data (encoding->priv->subp_streams, n);
  if (!data)
    return FALSE;

  *options = data->options;

  if (data->options.label)
    options->label = g_strdup (data->options.label);

  return TRUE;
}

/**
 * ogmrip_encoding_set_nth_subp_options:
 * @encoding: An #OGMRipEncoding
 * @n: The position of the stream, counting from 0
 * @options: An #OGMRipSubpOptions
 * @error: Return location for error
 *
 * Sets the options of the subp stream at the given position.
 *
 * Returns: %TRUE on success, %FALSE if there is no subp stream at the given position
 */
gboolean
ogmrip_encoding_set_nth_subp_options (OGMRipEncoding *encoding, guint n, OGMRipSubpOptions *options, GError **error)
{
  OGMRipSubpData *data;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);
  g_return_val_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding), FALSE);
  g_return_val_if_fail (options != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  data = g_slist_nth_data (encoding->priv->subp_streams, n);
  if (data)
  {
/*
    if (options->codec != data->options.codec &&
        !ogmrip_encoding_check_subp_codec (encoding, encoding->priv->container_type, data->stream, options, error))
      return FALSE;
*/
    data->options = *options;

    if (data->options.label)
      g_free (data->options.label);

    data->options.label = options->label ? g_strdup (options->label) : NULL;
  }

  return TRUE;
}

/**
 * ogmrip_encoding_add_audio_file:
 * @encoding: An #OGMRipEncoding
 * @file: An #OGMRipFile
 * @error: Return location for error
 *
 * Adds an audio file to the encoding.
 *
 * Returns: %TRUE on success, %FALSE if an error was set
 */
gboolean
ogmrip_encoding_add_audio_file (OGMRipEncoding *encoding, OGMRipFile *file, GError **error)
{
  gint n;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);
  g_return_val_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding), FALSE);
  g_return_val_if_fail (file != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (!ogmrip_encoding_check_audio_file (encoding, encoding->priv->container_type, file, error))
    return FALSE;

  n = g_slist_length (encoding->priv->audio_streams) + g_slist_length (encoding->priv->audio_files) + 1;
  if (!ogmrip_encoding_check_audio_streams (encoding, encoding->priv->container_type, n, error))
    return FALSE;

  ogmrip_file_ref (file);

  encoding->priv->audio_files = g_slist_append (encoding->priv->audio_files, file);

    if (encoding->priv->method == OGMRIP_ENCODING_SIZE)
      OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED);

    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_EXTRACTED);

  return TRUE;
}

/**
 * ogmrip_encoding_get_n_audio_files:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the number of audio files of the encoding.
 * 
 * Returns: The number of audio files, or -1
 */
gint
ogmrip_encoding_get_n_audio_files (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return g_slist_length (encoding->priv->audio_files);
}

/**
 * ogmrip_encoding_get_nth_audio_file:
 * @encoding: An #OGMRipEncoding
 * @n: The position of the file, counting from 0
 *
 * Gets the audio file at the given position.
 *
 * Returns: An #OGMRipFile, or NULL
 */
OGMRipFile *
ogmrip_encoding_get_nth_audio_file (OGMRipEncoding *encoding, guint n)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return g_slist_nth_data (encoding->priv->audio_files, n);
}

/**
 * ogmrip_encoding_foreach_audio_files:
 * @encoding: An #OGMRipEncoding
 * @func: The function to call with each audio file
 * @data: User data to pass to the function
 *
 * Calls the given function for each audio file.
 */
void
ogmrip_encoding_foreach_audio_files (OGMRipEncoding *encoding, OGMRipEncodingFileFunc func, gpointer data)
{
  GSList *link;

  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (func != NULL);

  for (link = encoding->priv->audio_files; link; link = link->next)
    (* func) (encoding, link->data, data);
}

/**
 * ogmrip_encoding_add_subp_file:
 * @encoding: An #OGMRipEncoding
 * @file: An #OGMRipFile
 * @error: Return location for error
 *
 * Adds an subp file to the encoding.
 *
 * Returns: %TRUE on success, %FALSE if an error was set
 */
gboolean
ogmrip_encoding_add_subp_file (OGMRipEncoding *encoding, OGMRipFile *file, GError **error)
{
  gint n;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), FALSE);
  g_return_val_if_fail (!OGMRIP_ENCODING_IS_RUNNING (encoding), FALSE);
  g_return_val_if_fail (file != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (!ogmrip_encoding_check_subp_file (encoding, encoding->priv->container_type, file, error))
    return FALSE;

  n = g_slist_length (encoding->priv->subp_streams) + g_slist_length (encoding->priv->subp_files) + 1;
  if (!ogmrip_encoding_check_subp_streams (encoding, encoding->priv->container_type, n, error))
    return FALSE;

  ogmrip_file_ref (file);

  encoding->priv->subp_files = g_slist_append (encoding->priv->subp_files, file);

  if (encoding->priv->method == OGMRIP_ENCODING_SIZE)
    OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTED);

  OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_EXTRACTED);

  return TRUE;
}

/**
 * ogmrip_encoding_get_n_subp_files:
 * @encoding: An #OGMRipEncoding
 *
 * Gets the number of subp files of the encoding.
 * 
 * Returns: The number of subp files, or -1
 */
gint
ogmrip_encoding_get_n_subp_files (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), -1);

  return g_slist_length (encoding->priv->subp_files);
}

/**
 * ogmrip_encoding_get_nth_subp_file:
 * @encoding: An #OGMRipEncoding
 * @n: The position of the stream, counting from 0
 *
 * Gets the subp file at the given position.
 *
 * Returns: An #OGMRipFile, or NULL
 */
OGMRipFile *
ogmrip_encoding_get_nth_subp_file (OGMRipEncoding *encoding, guint n)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return g_slist_nth_data (encoding->priv->subp_files, n);
}

/**
 * ogmrip_encoding_foreach_subp_files:
 * @encoding: An #OGMRipEncoding
 * @func: The function to call with each subp file
 * @data: User data to pass to the function
 *
 * Calls the given function for each subp file.
 */
void
ogmrip_encoding_foreach_subp_files (OGMRipEncoding *encoding, OGMRipEncodingFileFunc func, gpointer data)
{
  GSList *link;

  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  g_return_if_fail (func != NULL);

  for (link = encoding->priv->subp_files; link; link = link->next)
    (* func) (encoding, link->data, data);
}

/**
 * ogmrip_encoding_check_filename:
 * @encoding: An #OGMRipEncoding
 * @error: Return location for error
 *
 * Checks whether a file with the same name already exists.
 *
 * Returns: %TRUE on success, %FALSE if an error was set
 */
gboolean
ogmrip_encoding_check_filename (OGMRipEncoding *encoding, GError **error)
{
  const gchar *outfile;

  /*
   * TODO What if multiple targets ?
   */
  outfile = ogmrip_encoding_get_filename (encoding);
  if (!g_file_test (outfile, G_FILE_TEST_EXISTS))
    return TRUE;

  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_EXIST, _("A file named '%s' already exists."), outfile);

  return FALSE;
}

/**
 * ogmrip_encoding_extract:
 * @encoding: An #OGMRipEncoding
 * @error: Error location for error
 *
 * Performs all the steps necessary to encode the DVD title.
 *
 * Returns: An #OGMJobResultType
 */
gint
ogmrip_encoding_extract (OGMRipEncoding *encoding, GError **error)
{
  OGMRipContainer *container;
  OGMRipVideoCodec *encoder;
  guint32 old_flags;

  gint result = OGMJOB_RESULT_ERROR;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), OGMJOB_RESULT_ERROR);
  g_return_val_if_fail (error == NULL || *error == NULL, OGMJOB_RESULT_ERROR);

  container = ogmrip_encoding_get_container (encoding);
  if (!container)
    return OGMJOB_RESULT_ERROR;

  if (encoding->priv->profile)
    ogmrip_container_set_profile (container, encoding->priv->profile);

  if (!ogmrip_encoding_open_title (encoding, error))
    return OGMJOB_RESULT_ERROR;

  if (!ogmrip_encoding_check_space (encoding, ogmrip_encoding_get_rip_size (encoding), ogmrip_encoding_get_tmp_size (encoding), error))
    goto extract_cleanup;

  ogmrip_encoding_open_log (encoding);

  ogmjob_log_printf ("ENCODING: %s\n\n", ogmrip_encoding_get_label (encoding));

  g_signal_emit (encoding, signals[RUN], 0);

  OGMRIP_ENCODING_SET_FLAGS (encoding, OGMRIP_ENCODING_EXTRACTING);

  if (encoding->priv->video_codec_type != G_TYPE_NONE)
  {
    result = ogmrip_encoding_analyze_video_stream (encoding, error);
    if (result != OGMJOB_RESULT_SUCCESS)
      goto extract_cleanup;

    result = ogmrip_encoding_extract_chapters (encoding, error);
    if (result != OGMJOB_RESULT_SUCCESS)
      goto extract_cleanup;
  }

  result = ogmrip_encoding_encode_subp_streams (encoding, FALSE, error);
  if (result != OGMJOB_RESULT_SUCCESS)
    goto extract_cleanup;

  result = ogmrip_encoding_encode_audio_streams (encoding, FALSE, error);
  if (result != OGMJOB_RESULT_SUCCESS)
    goto extract_cleanup;

  if (encoding->priv->video_codec_type != G_TYPE_NONE)
  {
    encoder = ogmrip_encoding_get_video_encoder (encoding, error);
    ogmrip_container_set_video (container, encoder);
    g_object_unref (encoder);

    ogmjob_log_printf ("\nSetting video parameters");
    ogmjob_log_printf ("\n------------------------\n");

    ogmrip_encoding_bitrate (encoding, encoder, error);

    if (encoding->priv->can_crop)
      ogmrip_encoding_crop (encoding, encoder, error);

    if (encoding->priv->can_scale)
      ogmrip_encoding_scale (encoding, encoder, error);

    if (encoding->priv->test && encoding->priv->can_scale && encoding->priv->method != OGMRIP_ENCODING_QUANTIZER)
    {
      result = ogmrip_encoding_test_internal (encoding, NULL);
      if (result != OGMJOB_RESULT_SUCCESS)
        goto extract_cleanup;

      ogmrip_video_codec_set_scale_size (encoder,
          encoding->priv->scale_w, encoding->priv->scale_h);
    }

    result = ogmrip_encoding_encode_video_stream (encoding, encoder, error);
    if (result != OGMJOB_RESULT_SUCCESS)
      goto extract_cleanup;
  }

  result = ogmrip_encoding_merge (encoding, container, error);

extract_cleanup:
  old_flags = encoding->priv->flags;
  OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_EXTRACTING | OGMRIP_ENCODING_CANCELING);

  if ((old_flags & OGMRIP_ENCODING_EXTRACTING) != 0)
    g_signal_emit (encoding, signals[COMPLETE], 0, result);

  ogmrip_encoding_close_title (encoding);

  return result;
}

/**
 * ogmrip_encoding_backup:
 * @encoding: An #OGMRipEncoding
 * @error: Error location for error
 *
 * Performs all the steps necessary to copy the DVD title on the hard drive.
 *
 * Returns: An #OGMJobResultType
 */
gint
ogmrip_encoding_backup (OGMRipEncoding *encoding, GError **error)
{
  OGMDvdDisc *disc;
  OGMJobSpawn *spawn;

  gint result = OGMJOB_RESULT_ERROR;
  gchar *path;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), OGMJOB_RESULT_ERROR);
  g_return_val_if_fail (error == NULL || *error == NULL, OGMJOB_RESULT_ERROR);

  if (!ogmrip_encoding_open_title (encoding, error))
    return OGMJOB_RESULT_ERROR;

  path = ogmrip_encoding_get_backup_dir (encoding);

  if (!encoding->priv->copy_dvd)
  {
    result = OGMJOB_RESULT_SUCCESS;
    goto backup_cleanup;
  }

  if (!ogmrip_encoding_check_space (encoding, 0, ogmrip_encoding_get_dvd_size (encoding), error))
    goto backup_cleanup;

  if (!ogmrip_fs_mkdir (path, 0755, error))
    goto backup_cleanup;

  spawn = ogmrip_dvdcpy_new (encoding->priv->title, path);
  if (!spawn)
    goto backup_cleanup;

  ogmrip_encoding_open_log (encoding);

  ogmjob_log_printf ("COPYING: %s\n\n", ogmrip_encoding_get_label (encoding));

  g_signal_emit (encoding, signals[RUN], 0);

  OGMRIP_ENCODING_SET_FLAGS (encoding, OGMRIP_ENCODING_BACKUPING);

  ogmrip_encoding_emit_task (encoding, spawn, NULL, OGMRIP_TASK_RUN, OGMRIP_TASK_BACKUP, (OGMRipTaskDetail) 0);
  result = ogmjob_spawn_run (spawn, error);
  ogmrip_encoding_emit_task (encoding, spawn, NULL, OGMRIP_TASK_COMPLETE, OGMRIP_TASK_BACKUP, (OGMRipTaskDetail) 0);

  OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_BACKUPING);

  g_signal_emit (encoding, signals[COMPLETE], 0, result);

  g_object_unref (spawn);

  if (result != OGMJOB_RESULT_SUCCESS)
  {
    if (result == OGMJOB_RESULT_ERROR && error && !(*error))
      g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_UNKNOWN,
          _("Unknown error while copying the DVD on the hard drive"));

    ogmrip_fs_rmdir (path, TRUE, NULL);

    goto backup_cleanup;
  }

  disc = ogmdvd_disc_new (path, NULL);
  if (!disc)
    result = OGMJOB_RESULT_ERROR;
  ogmdvd_disc_unref (disc);

  OGMRIP_ENCODING_SET_FLAGS (encoding, OGMRIP_ENCODING_BACKUPED);

backup_cleanup:
  ogmrip_encoding_close_title (encoding);

  OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_CANCELING);

  g_free (path);

  return result;
}

/**
 * ogmrip_encoding_test:
 * @encoding: An #OGMRipEncoding
 * @error: Error location for error
 *
 * Performs a compressibility test on the DVD title.
 *
 * Returns: An #OGMJobResultType
 */
gint
ogmrip_encoding_test (OGMRipEncoding *encoding, GError **error)
{
  gint result;

  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), OGMJOB_RESULT_ERROR);
  g_return_val_if_fail (error == NULL || *error == NULL, OGMJOB_RESULT_ERROR);

  if ((encoding->priv->flags & OGMRIP_ENCODING_TESTED) != 0)
    return OGMJOB_RESULT_SUCCESS;

  if (!ogmrip_encoding_open_title (encoding, error))
    return OGMJOB_RESULT_ERROR;

  if (encoding->priv->video_codec_type == G_TYPE_NONE)
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_TEST,
        _("Cannot perform a compressibility test when the video codec is not defined."));
    return OGMJOB_RESULT_ERROR;
  }

  if (encoding->priv->crop_type != OGMRIP_OPTIONS_MANUAL)
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_TEST,
        _("Cannot perform a compressibility test when cropping parameters are not defined."));
    return OGMJOB_RESULT_ERROR;
  }

  if (encoding->priv->scale_type != OGMRIP_OPTIONS_MANUAL)
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_TEST,
        _("Cannot perform a compressibility test when scaling parameters are not defined."));
    return OGMJOB_RESULT_ERROR;
  }

  if (encoding->priv->method == OGMRIP_ENCODING_QUANTIZER)
  {
    g_set_error (error, OGMRIP_ENCODING_ERROR, OGMRIP_ENCODING_ERROR_TEST,
        _("Cannot perform a compressibility test when encoding at constant quantizer."));
    return OGMJOB_RESULT_ERROR;
  }

  g_signal_emit (encoding, signals[RUN], 0);

  OGMRIP_ENCODING_SET_FLAGS (encoding, OGMRIP_ENCODING_TESTING);
  result = ogmrip_encoding_test_internal (encoding, error);
  OGMRIP_ENCODING_UNSET_FLAGS (encoding, OGMRIP_ENCODING_TESTING | OGMRIP_ENCODING_CANCELING);

  g_signal_emit (encoding, signals[COMPLETE], 0, result);

  ogmrip_encoding_close_title (encoding);

  return result;
}

/**
 * ogmrip_encoding_cleanup:
 * @encoding: An #OGMRipEncoding
 *
 * Removes any remaining temporary files.
 */
void
ogmrip_encoding_cleanup (OGMRipEncoding *encoding)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  /* having an orig title means that a copy has been used */
  if (encoding->priv->orig_title)
  {
    const gchar *path;

    path = ogmdvd_disc_get_device (ogmdvd_title_get_disc (encoding->priv->title));

    ogmrip_fs_rmdir (path, TRUE, NULL);

    ogmrip_encoding_set_title (encoding, encoding->priv->orig_title);

    encoding->priv->copy_dvd = TRUE;
  }
}

/**
 * ogmrip_encoding_cancel:
 * @encoding: An #OGMRipEncoding
 *
 * Cancels an encoding.
 */
void
ogmrip_encoding_cancel (OGMRipEncoding *encoding)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));
  
  if (encoding->priv->task.spawn)
  {
    OGMRIP_ENCODING_SET_FLAGS (encoding, OGMRIP_ENCODING_CANCELING);

    ogmjob_spawn_cancel (encoding->priv->task.spawn);
  }
}

/**
 * ogmrip_encoding_suspend:
 * @encoding: An #OGMRipEncoding
 *
 * Suspends an encoding.
 */
void
ogmrip_encoding_suspend (OGMRipEncoding *encoding)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  if (encoding->priv->task.spawn)
    ogmjob_spawn_suspend (encoding->priv->task.spawn);
}

/**
 * ogmrip_encoding_resume:
 * @encoding: An #OGMRipEncoding
 *
 * Suspends an encoding.
 */
void
ogmrip_encoding_resume (OGMRipEncoding *encoding)
{
  g_return_if_fail (OGMRIP_IS_ENCODING (encoding));

  if (encoding->priv->task.spawn)
    ogmjob_spawn_resume (encoding->priv->task.spawn);
}

