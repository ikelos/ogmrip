/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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
 * You should have received a test of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/**
 * SECTION:ogmrip-test
 * @title: OGMRipTest
 * @short_description: A codec to test a media
 * @include: ogmrip-test.h
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-test.h"
#include "ogmrip-profile-keys.h"

#include <ogmrip-base.h>

#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#define SAMPLE_LENGTH  10.0
#define SAMPLE_PERCENT 0.05

#define ROUND(x) ((gint) ((x) + 0.5) != (gint) (x) ? ((gint) ((x) + 0.5)) : ((gint) (x)))

#define OGMRIP_TEST_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_TEST, OGMRipTestPriv))

struct _OGMRipTestPriv
{
  OGMRipEncoding *encoding;

  gdouble fraction;
  gdouble step; 
};

enum
{
  PROP_0,
  PROP_ENCODING
};

static void     ogmrip_test_constructed  (GObject      *gobject);
static void     ogmrip_test_dispose      (GObject      *gobject);
static void     ogmrip_test_get_property (GObject      *gobject,
                                          guint        property_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);
static void     ogmrip_test_set_property (GObject      *gobject,
                                          guint        property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static gboolean ogmrip_test_run          (OGMJobTask   *task,
                                          GCancellable *cancellable,
                                          GError       **error);

GQuark
ogmrip_test_error_quark (void)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_static_string ("ogmrip-test-error-quark");

  return quark;
}

G_DEFINE_TYPE (OGMRipTest, ogmrip_test, OGMJOB_TYPE_TASK)

static void
ogmrip_test_class_init (OGMRipTestClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobTaskClass *task_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_test_constructed;
  gobject_class->dispose = ogmrip_test_dispose;
  gobject_class->get_property = ogmrip_test_get_property;
  gobject_class->set_property = ogmrip_test_set_property;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_test_run;

  g_object_class_install_property (gobject_class, PROP_ENCODING, 
        g_param_spec_object ("encoding", "Encoding property", "Set encoding", 
           OGMRIP_TYPE_ENCODING, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipTestPriv));
}

static void
ogmrip_test_init (OGMRipTest *test)
{
  test->priv = OGMRIP_TEST_GET_PRIVATE (test);
}

static void
ogmrip_test_constructed (GObject *gobject)
{
  if (!OGMRIP_TEST (gobject)->priv->encoding)
    g_error ("No encoding specified");

  G_OBJECT_CLASS (ogmrip_test_parent_class)->constructed (gobject);
}

static void
ogmrip_test_dispose (GObject *gobject)
{
  OGMRipTest *test = OGMRIP_TEST (gobject);

  if (test->priv->encoding)
  {
    g_object_unref (test->priv->encoding);
    test->priv->encoding = NULL;
  }

  G_OBJECT_CLASS (ogmrip_test_parent_class)->dispose (gobject);
}

static void
ogmrip_test_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipTest *test = OGMRIP_TEST (gobject);

  switch (property_id) 
  {
    case PROP_ENCODING:
      g_value_set_object (value, test->priv->encoding);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_test_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipTest *test = OGMRIP_TEST (gobject);

  switch (property_id) 
  {
    case PROP_ENCODING:
      test->priv->encoding = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

typedef struct
{
  gint method;
  gdouble length;
  gboolean relative;
} OGMRipEncodingInfo;

static void
ogmrip_test_save_encoding_info (OGMRipEncoding *encoding, OGMRipEncodingInfo *info)
{
  OGMRipCodec *codec;

  info->relative = ogmrip_encoding_get_relative (encoding);
  ogmrip_encoding_set_relative (encoding, TRUE);

  codec = ogmrip_encoding_get_video_codec (encoding);
  info->length = ogmrip_codec_get_length (codec, NULL);

  ogmrip_profile_get (ogmrip_encoding_get_profile (encoding),
      OGMRIP_PROFILE_GENERAL, OGMRIP_PROFILE_ENCODING_METHOD, "u", &info->method);
}

static void
ogmrip_test_restore_encoding_info (OGMRipEncoding *encoding, OGMRipEncodingInfo *info)
{
  ogmrip_encoding_set_relative (encoding, info->relative);
}

typedef struct
{
  gint passes, bitrate;
  guint start_chap, end_chap;
  gdouble start_position, play_length;
} OGMRipCodecInfo;

static void
ogmrip_test_save_codec_info (OGMRipCodec *codec, OGMRipCodecInfo *info)
{
  info->play_length = ogmrip_codec_get_play_length (codec);
  info->start_position = ogmrip_codec_get_start_position (codec);
  ogmrip_codec_get_chapters (codec, &info->start_chap, &info->end_chap);

  if (OGMRIP_IS_VIDEO_CODEC (codec))
  {
    info->passes = ogmrip_video_codec_get_passes (OGMRIP_VIDEO_CODEC (codec));
    info->bitrate = ogmrip_video_codec_get_bitrate (OGMRIP_VIDEO_CODEC (codec));
  }
}

static void
ogmrip_test_restore_codec_info (OGMRipCodec *codec, OGMRipCodecInfo *info)
{
  ogmrip_codec_set_play_length (codec, info->play_length);
  ogmrip_codec_set_start_position (codec, info->start_position);
  ogmrip_codec_set_chapters (codec, info->start_chap, info->end_chap);

  if (OGMRIP_IS_VIDEO_CODEC (codec))
  {
    ogmrip_video_codec_set_passes (OGMRIP_VIDEO_CODEC (codec), info->passes);
    ogmrip_video_codec_set_bitrate (OGMRIP_VIDEO_CODEC (codec), info->bitrate);
  }
}

static gboolean
ogmrip_test_encode_audio (OGMRipTest *test, gdouble start_position, gdouble play_length, GCancellable *cancellable, GError **error)
{
  GList *list, *link;
  gboolean result = FALSE;

  list = ogmrip_encoding_get_audio_codecs (test->priv->encoding);
  if (!list)
    return TRUE;

  for (link = list; link; link = link->next)
  {
    OGMRipCodecInfo info;

    ogmrip_test_save_codec_info (link->data, &info);

    ogmrip_codec_set_start_position (link->data, start_position);
    ogmrip_codec_set_play_length (link->data, play_length);

    result  = ogmjob_task_run (link->data, cancellable, error);

    ogmrip_test_restore_codec_info (link->data, &info);

    if (!result)
      break;
  }
  g_list_free (list);

  return result;
}

static gboolean
ogmrip_test_encode_subp (OGMRipTest *test, gdouble start_position, gdouble play_length, GCancellable *cancellable, GError **error)
{
  GList *list, *link;
  gboolean result = FALSE;

  list = ogmrip_encoding_get_subp_codecs (test->priv->encoding);
  if (!list)
    return TRUE;

  for (link = list; link; link = link->next)
  {
    OGMRipCodecInfo info;

    ogmrip_test_save_codec_info (link->data, &info);

    ogmrip_codec_set_start_position (link->data, start_position);
    ogmrip_codec_set_play_length (link->data, play_length);

    result  = ogmjob_task_run (link->data, cancellable, error);

    ogmrip_test_restore_codec_info (link->data, &info);

    if (!result)
      break;
  }
  g_list_free (list);

  return result;
}

static gboolean
ogmrip_test_encode_video (OGMRipTest *test, gdouble start_position, gdouble play_length, gint *bitrate, GCancellable *cancellable, GError **error)
{
  OGMRipCodec *codec;
  OGMRipCodecInfo info;
  OGMRipFile *output;
  OGMRipMedia *file;
  gboolean result;

  codec = ogmrip_encoding_get_video_codec (test->priv->encoding);

  ogmrip_test_save_codec_info (codec, &info);

  ogmrip_video_codec_set_quantizer (OGMRIP_VIDEO_CODEC (codec), 2);
  ogmrip_video_codec_set_passes (OGMRIP_VIDEO_CODEC (codec), 1);
  ogmrip_codec_set_start_position  (codec, start_position);
  ogmrip_codec_set_play_length (codec, play_length);

  result = ogmjob_task_run (OGMJOB_TASK (codec), cancellable, error);

  ogmrip_test_restore_codec_info (codec, &info);

  if (!result)
    return FALSE;

  test->priv->fraction += test->priv->step;
  ogmjob_task_set_progress (OGMJOB_TASK (test), test->priv->fraction);

  output = ogmrip_codec_get_output (codec);

  file = ogmrip_video_file_new (ogmrip_stream_get_uri (OGMRIP_STREAM (output)));
  if (!file)
    return FALSE;

  *bitrate = ogmrip_video_stream_get_bitrate (OGMRIP_VIDEO_STREAM (file));

  g_unlink (ogmrip_file_get_path (OGMRIP_FILE (output)));
  g_object_unref (file);

  return TRUE;
}

static void
ogmrip_test_set_scale_size (OGMRipTest *test, guint optimal_bitrate, guint user_bitrate)
{
  OGMRipCodec *codec;
  gdouble ratio, user_quality, optimal_quality, cfactor;
  guint crop_x, crop_y, crop_w, crop_h, scale_w, scale_h;
  guint raw_w, raw_h, fnum, fdenom, anum, adenom;

  codec = ogmrip_encoding_get_video_codec (test->priv->encoding);

  ogmrip_video_codec_get_raw_size (OGMRIP_VIDEO_CODEC (codec), &raw_w, &raw_h);
  ogmrip_log_printf ("Raw size: %ux%u\n", raw_w, raw_h);

  ogmrip_video_codec_get_crop_size (OGMRIP_VIDEO_CODEC (codec), &crop_x, &crop_y, &crop_w, &crop_h);
  ogmrip_log_printf ("Crop size: %u, %u %ux%u\n\n", crop_x, crop_y, crop_w, crop_h);

  ogmrip_video_codec_get_scale_size (OGMRIP_VIDEO_CODEC (codec), &scale_w, &scale_h);

  ogmrip_video_codec_get_aspect_ratio (OGMRIP_VIDEO_CODEC (codec), &anum, &adenom);
  ogmrip_video_codec_get_framerate (OGMRIP_VIDEO_CODEC (codec), &fnum, &fdenom);

  ratio = crop_w / (gdouble) crop_h * raw_h / raw_w * anum / adenom;
  optimal_quality = optimal_bitrate / (gdouble) scale_w / scale_h / fnum * fdenom;

  ogmrip_log_printf ("User bitrate: %d\n", user_bitrate);
  ogmrip_log_printf ("Optimal bitrate: %d\n", optimal_bitrate);
  ogmrip_log_printf ("Optimal quality: %.02lf\n\n", optimal_quality);

  user_quality = user_bitrate / (gdouble) scale_w / scale_h / fnum * fdenom;
  cfactor = user_quality / optimal_quality * 100;

  ogmrip_log_printf ("Scale size: %ux%u\n", scale_w, scale_h);
  ogmrip_log_printf ("User quality: %.02lf\n", user_quality);
  ogmrip_log_printf ("Compressibility factor: %.0lf%%\n\n", cfactor);

  while (cfactor > 65.0)
  {
    scale_w += 16;
    scale_h = 16 * ROUND (scale_w / ratio / 16);

    user_quality = user_bitrate / (gdouble) scale_w / scale_h / fnum * fdenom;
    cfactor = user_quality / optimal_quality * 100;

    ogmrip_log_printf ("Scale size: %ux%u\n", scale_w, scale_h);
    ogmrip_log_printf ("User quality: %.02lf\n", user_quality);
    ogmrip_log_printf ("Compressibility factor: %.0lf%%\n\n", cfactor);
  }

  while (cfactor < 35.0)
  {
    scale_w -= 16;
    scale_h = 16 * ROUND (scale_w / ratio / 16);

    user_quality = user_bitrate / (gdouble) scale_w / scale_h / fnum * fdenom;
    cfactor = user_quality / optimal_quality * 100;

    ogmrip_log_printf ("Scale size: %ux%u\n", scale_w, scale_h);
    ogmrip_log_printf ("User quality: %.02lf\n", user_quality);
    ogmrip_log_printf ("Compressibility factor: %.0lf%%\n\n", cfactor);
  }

  ogmrip_video_codec_set_scale_size (OGMRIP_VIDEO_CODEC (codec), scale_w, scale_h);
}

static gboolean
ogmrip_test_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMRipTest *test = OGMRIP_TEST (task);
  gboolean result = TRUE;

  OGMRipEncodingInfo info;
  guint files, optimal_bitrate = 0, user_bitrate = 0;
  gdouble start;

  ogmrip_test_save_encoding_info (test->priv->encoding, &info);

  if (info.method == OGMRIP_ENCODING_QUANTIZER)
  {
    g_set_error (error, OGMRIP_TEST_ERROR, OGMRIP_TEST_ERROR_QUANTIZER,
        _("Cannot perform a compressibility test when encoding at fixed quantizer"));
    return FALSE;
  }

  test->priv->fraction = 0.0;
  test->priv->step = SAMPLE_LENGTH / info.length;
  if (info.method == OGMRIP_ENCODING_SIZE)
    test->priv->step /= 1 +
      ogmrip_encoding_get_n_audio_codecs (test->priv->encoding) +
      ogmrip_encoding_get_n_subp_codecs (test->priv->encoding);

  /*
   * If at least one file has been decoded, an error is non-blocking
   */

  for (start = 0.0, files = 0; start + SAMPLE_LENGTH * SAMPLE_PERCENT < info.length; start += SAMPLE_LENGTH)
  {
    gint bitrate;

    if (info.method == OGMRIP_ENCODING_SIZE)
    {
      /*
       * Encode subtitles
       */
      result = ogmrip_test_encode_subp (test, start, SAMPLE_LENGTH * SAMPLE_PERCENT, cancellable, error);
      if (!result && (!files || g_cancellable_is_cancelled (cancellable)))
        break;
      g_clear_error (error);

      /*
       * Encoding audio
       */
      if (result)
      {
        result = ogmrip_test_encode_audio (test, start, SAMPLE_LENGTH * SAMPLE_PERCENT, cancellable, error);
        if (!result && (!files || g_cancellable_is_cancelled (cancellable)))
          break;
        g_clear_error (error);
      }
    }

    /*
     * Encode video
     */
    if (result)
    {
      result = ogmrip_test_encode_video (test, start, SAMPLE_LENGTH * SAMPLE_PERCENT, &bitrate, cancellable, error);
      if (!result && (!files || g_cancellable_is_cancelled (cancellable)))
        break;
      g_clear_error (error);
    }

    /*
     * Compute optimal and user bitrate
     */
    if (result && bitrate > 0)
    {
      optimal_bitrate += bitrate;

      if (info.method == OGMRIP_ENCODING_SIZE)
        user_bitrate += ogmrip_encoding_autobitrate (test->priv->encoding);

      files ++;
    }

    result = TRUE;
  }

  if (result && files > 0)
  {
    optimal_bitrate /= files;

    if (info.method == OGMRIP_ENCODING_SIZE)
      user_bitrate /= files;
    else
    {
      OGMRipCodec *codec;

      codec = ogmrip_encoding_get_video_codec (test->priv->encoding);
      user_bitrate = ogmrip_video_codec_get_bitrate (OGMRIP_VIDEO_CODEC (codec));
    }

    /*
     * Compute optimal scaling parameters
     */
    if (optimal_bitrate > 0 && user_bitrate > 0)
      ogmrip_test_set_scale_size (test, optimal_bitrate, user_bitrate);
  }

  ogmrip_test_restore_encoding_info (test->priv->encoding, &info);

  return result;
}

OGMJobTask *
ogmrip_test_new (OGMRipEncoding *encoding)
{
  return g_object_new (OGMRIP_TYPE_TEST, "encoding", encoding, NULL);
}

