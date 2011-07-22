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

/**
 * SECTION:ogmrip-mplayer
 * @title: Mplayer
 * @short_description: Common wrapper functions for mplayer and mencoder
 * @include: ogmrip-mplayer.h
 */

#include "ogmrip-mplayer.h"
#include "ogmrip-version.h"
#include "ogmrip-plugin.h"
#include "ogmrip-enums.h"
#include "ogmrip-fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib/gstdio.h>

static const gchar *deinterlacer[] = { "lb", "li", "ci", "md", "fd", "l5", "kerndeint", "yadif" };

static gint
ogmrip_mplayer_map_audio_id (OGMDvdStream *astream)
{
  gint aid;

  aid = ogmdvd_stream_get_id (astream);

  switch (ogmdvd_audio_stream_get_format (OGMDVD_AUDIO_STREAM (astream)))
  {
    case OGMDVD_AUDIO_FORMAT_MPEG1:
    case OGMDVD_AUDIO_FORMAT_MPEG2EXT:
      break;
    case OGMDVD_AUDIO_FORMAT_LPCM:
      aid += 160;
      break;
    case OGMDVD_AUDIO_FORMAT_DTS:
      aid += 136;
      break;
    default:
      aid += 128;
      break;
  }

  return aid;
}

static glong
ogmrip_mplayer_get_frames (OGMRipCodec *codec)
{
  OGMDvdStream *stream;
  guint num, denom;
  gdouble length;

  length = ogmrip_codec_get_length (codec, NULL);

  stream = ogmrip_codec_get_input (codec);
  ogmdvd_video_stream_get_framerate (OGMDVD_VIDEO_STREAM (stream), &num, &denom);

  return length / (gdouble) denom * num;
}

static gchar *
ogmrip_mplayer_get_output_fps (OGMRipCodec *codec, OGMDvdTitle *title)
{
  OGMDvdVideoStream *stream;
  guint num1, denom1, num2, denom2;

  stream = ogmdvd_title_get_video_stream (title);
  ogmdvd_video_stream_get_framerate (stream, &num1, &denom1);

  if (ogmrip_codec_get_telecine (codec) || ogmrip_codec_get_progressive (codec))
  {
    num2 = 24000;
    denom2 = 1001;
  }
  else
  {
    num2 = num1;
    denom2 = denom1;
  }

  if (num1 != num2 || denom2 != denom2)
    return g_strdup_printf ("%d/%d", num2, denom2);

  return NULL;
}

static gchar *
ogmrip_mplayer_get_chapters (OGMRipCodec *codec, OGMDvdTitle *title)
{
  guint start, end;
  gint n_chap;

  ogmrip_codec_get_chapters (codec, &start, &end);

  n_chap = ogmdvd_title_get_n_chapters (title);

  if (start != 0 || end != n_chap - 1)
  {
    gchar *str;

    if (end != n_chap - 1)
    {
      ogmdvd_title_get_n_chapters (title);
      str = g_strdup_printf ("%d-%d", start + 1, end + 1);
    }
    else
      str = g_strdup_printf ("%d", start + 1);

    return str;
  }

  return NULL;
}

static gboolean
ogmrip_mplayer_check_mcdeint (void)
{
  static gint have_mcdeint = -1;

  if (have_mcdeint < 0) 
  {
    gchar *output = NULL;

    have_mcdeint = 0;
    if (g_spawn_command_line_sync ("mplayer -vf help", &output, NULL, NULL, NULL))
    {
      if (output && strstr (output, "mcdeint"))
        have_mcdeint = 1;
      g_free (output);
    }
  }

  return have_mcdeint == 1;
}

static void
ogmrip_mplayer_set_deint (OGMRipVideoCodec *video, GPtrArray *argv, GString *options, GString *pp)
{
  OGMRipDeintType deint;

  deint = ogmrip_video_codec_get_deinterlacer (video);
  if (deint != OGMRIP_DEINT_NONE)
  {
    if (deint == OGMRIP_DEINT_KERNEL || deint == OGMRIP_DEINT_YADIF)
    {
      if (options->len > 0)
        g_string_append_c (options, ',');
      g_string_append (options, deinterlacer[deint - 1]);

      if (deint == OGMRIP_DEINT_YADIF)
      {
        if (MPLAYER_CHECK_VERSION (1,0,2,0))
        {
          g_string_append (options, "=3");

          if (ogmrip_mplayer_check_mcdeint ())
            g_string_append (options, ",mcdeint=2:1:10");

          g_string_append (options, ",framestep=2");

          g_ptr_array_add (argv, g_strdup ("-field-dominance"));
          g_ptr_array_add (argv, g_strdup ("-1")); /* or 1 ? */
        }
        else
          g_string_append (options, "=0");
      }
    }
    else
    {
      if (pp->len > 0)
        g_string_append_c (pp, '/');
      g_string_append (pp, deinterlacer[deint - 1]);
    }
  }
}

static gint
ogmrip_mplayer_audio_file_get_demuxer (OGMRipAudioFile *audio)
{
  gint demuxer = OGMRIP_AUDIO_DEMUXER_AUTO;

  switch (ogmrip_file_get_format (OGMRIP_FILE (audio)))
  {
    case OGMRIP_FORMAT_AC3:
      demuxer = OGMRIP_AUDIO_DEMUXER_AC3;
      break;
    case OGMRIP_FORMAT_DTS:
      demuxer = OGMRIP_AUDIO_DEMUXER_DTS;
      break;
  }

  return demuxer;
}

/**
 * ogmrip_mencoder_codec_watch:
 * @exec: An #OGMJobExec
 * @buffer: The buffer to parse
 * @codec: An #OGMRipCodec
 *
 * This function parses the output of mencoder when encoding a stream and
 * returns the progress made.
 *
 * Returns: The progress made, or -1.0
 */
gdouble
ogmrip_mencoder_codec_watch (OGMJobExec *exec, const gchar *buffer, OGMRipCodec *codec)
{
  gint frames, progress;
  gdouble seconds;
  gchar pos[10];

  if (sscanf (buffer, "Pos:%s %df (%d%%)", pos, &frames, &progress) == 3)
  {
    seconds = strtod (pos, NULL);
    return seconds / ogmrip_codec_get_length (codec, NULL);
  }

  return -1.0;
}

/**
 * ogmrip_mencoder_container_watch:
 * @exec: An #OGMJobExec
 * @buffer: The buffer to parse
 * @container: An #OGMRipContainer
 *
 * This function parses the output of mencoder when merging streams and returns
 * the progress made.
 *
 * Returns: The progress made, or -1.0
 */
gdouble
ogmrip_mencoder_container_watch (OGMJobExec *exec, const gchar *buffer, OGMRipContainer *container)
{
  gint frames, progress;
  gchar pos[10];

  if (sscanf (buffer, "Pos:%s %df (%d%%)", pos, &frames, &progress) == 3)
    return progress / 100.;

  return -1.0;
}

/**
 * ogmrip_mplayer_wav_command:
 * @audio: An #OGMRipAudioCodec
 * @header: Whether to add the PCM header
 * @output: The output file, or NULL
 *
 * This function creates the command line for encoding an audio stream in PCM
 * or WAV.
 *
 * Returns: A new #GPtrArray, or NULL
 */
GPtrArray *
ogmrip_mplayer_wav_command (OGMRipAudioCodec *audio, gboolean header, const gchar *output)
{
  OGMDvdStream *stream;

  GPtrArray *argv;
  GString *options;

  gdouble start, length;
  const gchar *device;
  gchar *chap;
  gint vid;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), NULL);

  if (!output)
    output = ogmrip_codec_get_output (OGMRIP_CODEC (audio));

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("mplayer"));
  g_ptr_array_add (argv, g_strdup ("-nolirc"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-noframedrop"));

  if (MPLAYER_CHECK_VERSION (1,0,3,0))
  {
    g_ptr_array_add (argv, g_strdup ("-noconfig"));
    g_ptr_array_add (argv, g_strdup ("all"));
  }

  length = ogmrip_codec_get_play_length (OGMRIP_CODEC (audio));

  if (length <= 0.0)
  {
    g_ptr_array_add (argv, g_strdup ("-vc"));
    g_ptr_array_add (argv, g_strdup ("dummy"));
  }

  g_ptr_array_add (argv, g_strdup ("-vo"));
  g_ptr_array_add (argv, g_strdup ("null"));
  g_ptr_array_add (argv, g_strdup ("-ao"));

  if (MPLAYER_CHECK_VERSION (1,0,0,8))
  {
    options = g_string_new ("pcm");

    if (ogmrip_audio_codec_get_fast (audio))
      g_string_append (options, ":fast");

    if (header)
      g_string_append (options, ":waveheader");
    else
      g_string_append (options, ":nowaveheader");

    g_string_append_printf (options, ":file=%s", output);

    g_ptr_array_add (argv, g_string_free (options, FALSE));
  }
  else if (MPLAYER_CHECK_VERSION (1,0,0,7))
  {
    if (header)
      g_ptr_array_add (argv, g_strdup_printf ("pcm:waveheader:file=%s", output));
    else
      g_ptr_array_add (argv, g_strdup_printf ("pcm:nowaveheader:file=%s", output));
  }
  else
  {
    g_ptr_array_add (argv, g_strdup ("pcm"));
    if (header)
      g_ptr_array_add (argv, g_strdup ("-waveheader"));
    else
      g_ptr_array_add (argv, g_strdup ("-nowaveheader"));
    g_ptr_array_add (argv, g_strdup ("-aofile"));
    g_ptr_array_add (argv, g_strdup (output));
  }

  g_ptr_array_add (argv, g_strdup ("-format"));
  g_ptr_array_add (argv, g_strdup ("s16le"));

  options = g_string_new (NULL);

  if (ogmrip_audio_codec_get_normalize (audio))
  {
    if (MPLAYER_CHECK_VERSION (1,0,0,8))
      g_string_append (options, "volnorm=1");
    else if (MPLAYER_CHECK_VERSION (1,0,0,6))
      g_string_append (options, "volnorm");
    else
      g_string_append (options, "list=volnorm");
  }

  if (MPLAYER_CHECK_VERSION (1,0,0,6))
  {
    gint srate = ogmrip_audio_codec_get_sample_rate (audio);
    if (srate != 48000)
    {
      g_ptr_array_add (argv, g_strdup ("-srate"));
      g_ptr_array_add (argv, g_strdup_printf ("%d", srate));

      if (options->len > 0)
        g_string_append_c (options, ',');
      g_string_append_printf (options, "lavcresample=%d", srate);
    }
  }

  if (options->len == 0)
    g_string_free (options, TRUE);
  else
  {
    if (MPLAYER_CHECK_VERSION (1,0,0,6))
      g_ptr_array_add (argv, g_strdup ("-af"));
    else
      g_ptr_array_add (argv, g_strdup ("-aop"));

    g_ptr_array_add (argv, g_string_free (options, FALSE));
  }

  g_ptr_array_add (argv, g_strdup ("-channels"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", ogmrip_audio_codec_get_channels (audio) + 1));

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (audio));

  chap = ogmrip_mplayer_get_chapters (OGMRIP_CODEC (audio), ogmdvd_stream_get_title (stream));
  if (chap)
  {
    g_ptr_array_add (argv, g_strdup ("-chapter"));
    g_ptr_array_add (argv, chap);
  }

  start = ogmrip_codec_get_start_position (OGMRIP_CODEC (audio));
  if (start > 0.0)
  {
    g_ptr_array_add (argv, g_strdup ("-ss"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", start));
  }

  if (length > 0.0)
  {
    OGMDvdVideoStream *video;
    guint num, denom;

    video = ogmdvd_title_get_video_stream (ogmdvd_stream_get_title (stream));
    ogmdvd_video_stream_get_framerate (video, &num, &denom);

    g_ptr_array_add (argv, g_strdup ("-frames"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", length * num / denom));
  }

  g_ptr_array_add (argv, g_strdup ("-aid"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", ogmrip_mplayer_map_audio_id (stream)));

  device = ogmdvd_disc_get_device (ogmdvd_title_get_disc (ogmdvd_stream_get_title (stream)));
  g_ptr_array_add (argv, g_strdup ("-dvd-device"));
  g_ptr_array_add (argv, g_strdup (device));

  vid = ogmdvd_title_get_nr (ogmdvd_stream_get_title (stream));

  if (MPLAYER_CHECK_VERSION (1,0,0,1))
    g_ptr_array_add (argv, g_strdup_printf ("dvd://%d", vid + 1));
  else
  {
    g_ptr_array_add (argv, g_strdup ("-dvd"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", vid + 1));
  }

  g_ptr_array_add (argv, NULL);

  return argv;
}

/**
 * ogmrip_mplayer_wav_watch:
 * @exec: An #OGMJobExec
 * @buffer: The buffer to parse
 * @audio: An #OGMRipAudioCodec
 *
 * This function parses the output of mplayer when encoding an audio stream in
 * WAV or PCM and returns the progress made.
 *
 * Returns: The progress made, or -1.0
 */
gdouble
ogmrip_mplayer_wav_watch (OGMJobExec *exec, const gchar *buffer, OGMRipAudioCodec *audio)
{
  gchar pos[12], pos_time[12], total[12];
  static gdouble start;
  gdouble secs;

  if (g_str_equal (buffer, "Starting playback..."))
    start = 0;
  else if (sscanf (buffer, "A: %s %s of %s", pos, pos_time, total) == 3)
  {
    secs = strtod (pos, NULL);
    if (!start)
      start = secs;

    return (secs - start) / ogmrip_codec_get_length (OGMRIP_CODEC (audio), NULL);
  }

  return -1.0;
}

/**
 * ogmrip_mencoder_audio_command:
 * @audio: An #OGMRipAudioCodec
 * @output: The output file, or NULL
 *
 * This function creates the common part of the command line when using mencoder
 * to encode an audio stream.
 *
 * Returns: A new #GPtrArray, or NULL
 */
GPtrArray *
ogmrip_mencoder_audio_command (OGMRipAudioCodec *audio, const gchar *output)
{
  GPtrArray *argv;
  OGMDvdStream *stream;

  const gchar *device;
  gdouble start, length;
  gchar *ofps, *chap;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), NULL);

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (audio));

  argv = g_ptr_array_new ();

  g_ptr_array_add (argv, g_strdup ("mencoder"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));

  if (MPLAYER_CHECK_VERSION (1,0,3,0))
  {
    g_ptr_array_add (argv, g_strdup ("-noconfig"));
    g_ptr_array_add (argv, g_strdup ("all"));
  }

  ofps = ogmrip_mplayer_get_output_fps (OGMRIP_CODEC (audio), ogmdvd_stream_get_title (stream));
  if (ofps)
  {
    g_ptr_array_add (argv, g_strdup ("-ofps"));
    g_ptr_array_add (argv, ofps);
  }

  chap = ogmrip_mplayer_get_chapters (OGMRIP_CODEC (audio), ogmdvd_stream_get_title (stream));
  if (chap)
  {
    g_ptr_array_add (argv, g_strdup ("-chapter"));
    g_ptr_array_add (argv, chap);
  }

  start = ogmrip_codec_get_start_position (OGMRIP_CODEC (audio));
  if (start > 0.0)
  {
    g_ptr_array_add (argv, g_strdup ("-ss"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", start));
  }

  length = ogmrip_codec_get_play_length (OGMRIP_CODEC (audio));
  if (length > 0.0)
  {
    OGMDvdVideoStream *video;
    guint num, denom;

    video = ogmdvd_title_get_video_stream (ogmdvd_stream_get_title (stream));
    ogmdvd_video_stream_get_framerate (video, &num, &denom);

    g_ptr_array_add (argv, g_strdup ("-frames"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", length * num / denom));
  }

  g_ptr_array_add (argv, g_strdup ("-aid"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", ogmrip_mplayer_map_audio_id (stream)));

  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (output));

  device = ogmdvd_disc_get_device (ogmdvd_title_get_disc (ogmdvd_stream_get_title (stream)));
  g_ptr_array_add (argv, g_strdup ("-dvd-device"));
  g_ptr_array_add (argv, g_strdup (device));

  return argv;
}

/**
 * ogmrip_mencoder_video_command:
 * @video: An #OGMRipVideoCodec
 * @output: The output file, or NULL
 * @pass: The number of passes
 *
 * This function creates the common part of the command line when using mencoder
 * to encode a video stream.
 *
 * Returns: A new #GPtrArray, or NULL
 */
GPtrArray *
ogmrip_mencoder_video_command (OGMRipVideoCodec *video, const gchar *output, guint pass)
{
  GPtrArray *argv;
  OGMDvdStream *stream;

  gint format;
  const gchar *device;
  gdouble start, length;
  gchar *ofps, *chap;
  gboolean scale;

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (video));

  format = ogmrip_plugin_get_video_codec_format (G_OBJECT_TYPE (video));

  argv = g_ptr_array_new ();

  g_ptr_array_add (argv, g_strdup ("mencoder"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-noslices"));

  if (MPLAYER_CHECK_VERSION (1,0,3,0))
  {
    g_ptr_array_add (argv, g_strdup ("-noconfig"));
    g_ptr_array_add (argv, g_strdup ("all"));
  }

  scale = FALSE;

  if (format == OGMRIP_FORMAT_COPY)
    g_ptr_array_add (argv, g_strdup ("-nosound"));
  else
  {
    OGMDvdAudioStream *astream;
    OGMDvdSubpStream *sstream;
    OGMRipScalerType scaler;

    GString *options, *pp;
    guint max_width, max_height;
    guint scale_width, scale_height;
    guint crop_x, crop_y, crop_width, crop_height;
    gboolean crop, expand, forced;

    astream = ogmrip_video_codec_get_ensure_sync (video);
    if (astream)
    {
      g_ptr_array_add (argv, g_strdup ("-oac"));
      g_ptr_array_add (argv, g_strdup ("pcm"));
      g_ptr_array_add (argv, g_strdup ("-srate"));
      g_ptr_array_add (argv, g_strdup ("8000"));
      g_ptr_array_add (argv, g_strdup ("-af"));
      g_ptr_array_add (argv, g_strdup ("channels=1,lavcresample=8000"));
      g_ptr_array_add (argv, g_strdup ("-aid"));
      g_ptr_array_add (argv, g_strdup_printf ("%d", ogmrip_mplayer_map_audio_id (OGMDVD_STREAM (astream))));
    }
    else
      g_ptr_array_add (argv, g_strdup ("-nosound"));

    sstream = ogmrip_video_codec_get_hard_subp (video, &forced);
    if (sstream)
    {
      g_ptr_array_add (argv, g_strdup ("-spuaa"));
      g_ptr_array_add (argv, g_strdup ("20"));
      g_ptr_array_add (argv, g_strdup ("-sid"));
      g_ptr_array_add (argv, g_strdup_printf ("%d",
            ogmdvd_stream_get_id (OGMDVD_STREAM (sstream))));

      if (forced)
        g_ptr_array_add (argv, g_strdup ("-forcedsubsonly"));
    }
    else if (ogmrip_check_mplayer_nosub ())
      g_ptr_array_add (argv, g_strdup ("-nosub"));

    scaler = ogmrip_video_codec_get_scaler (video);
    g_ptr_array_add (argv, g_strdup ("-sws"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", MAX (scaler, 0)));

    scale = ogmrip_video_codec_get_scale_size (video, &scale_width, &scale_height);

    options = g_string_new (NULL);
    pp = g_string_new (NULL);

    if (ogmrip_video_codec_get_deblock (video))
    {
      if (pp->len > 0)
        g_string_append_c (pp, '/');
      g_string_append (pp, "ha/va");
    }

    if (ogmrip_video_codec_get_dering (video))
    {
      if (pp->len > 0)
        g_string_append_c (pp, '/');
      g_string_append (pp, "dr");
    }

    if (ogmrip_codec_get_telecine (OGMRIP_CODEC (video)))
    {
      if (options->len > 0)
        g_string_append_c (options, ',');
      g_string_append (options, "pullup,softskip");
    }

    crop = ogmrip_video_codec_get_crop_size (video, &crop_x, &crop_y, &crop_width, &crop_height);
    if (crop)
    {
      if (options->len > 0)
        g_string_append_c (options, ',');
      g_string_append_printf (options, "crop=%u:%u:%u:%u", crop_width, crop_height, crop_x, crop_y);
    }

    ogmrip_mplayer_set_deint (video, argv, options, pp);

    if (pp->len > 0)
    {
      if (options->len > 0)
        g_string_append_c (options, ',');
      g_string_append_printf (options, "pp=%s", pp->str);
    }
    g_string_free (pp, TRUE);

    if (scale)
    {
      if (options->len > 0)
        g_string_append_c (options, ',');
      g_string_append_printf (options, "scale=%u:%u", scale_width, scale_height);

      if (ogmrip_video_codec_is_interlaced (video) > 0 && ogmrip_video_codec_get_deinterlacer (video) == OGMRIP_DEINT_NONE)
        g_string_append (options, ":1");
    }

    if (ogmrip_video_codec_get_max_size (video, &max_width, &max_height, &expand) && expand)
    {
      if (options->len > 0)
        g_string_append_c (options, ',');
      g_string_append_printf (options, "expand=%u:%u", max_width, max_height);
    }

    if (ogmrip_video_codec_get_denoise (video))
    {
      if (options->len > 0)
        g_string_append_c (options, ',');
      g_string_append (options, "hqdn3d=2:1:2");
    }

    if (options->len > 0)
      g_string_append_c (options, ',');
    g_string_append (options, "harddup");

    if (options->len == 0)
      g_string_free (options, TRUE);
    else
    {
      g_ptr_array_add (argv, g_strdup ("-vf"));
      g_ptr_array_add (argv, g_string_free (options, FALSE));
    }

    ofps = ogmrip_mplayer_get_output_fps (OGMRIP_CODEC (video), ogmdvd_stream_get_title (stream));
    if (ofps)
    {
      g_ptr_array_add (argv, g_strdup ("-ofps"));
      g_ptr_array_add (argv, ofps);
    }
  }

  g_ptr_array_add (argv, g_strdup (scale ? "-zoom": "-nozoom"));

  chap = ogmrip_mplayer_get_chapters (OGMRIP_CODEC (video), ogmdvd_stream_get_title (stream));
  if (chap)
  {
    g_ptr_array_add (argv, g_strdup ("-chapter"));
    g_ptr_array_add (argv, chap);
  }

  start = ogmrip_codec_get_start_position (OGMRIP_CODEC (video));
  if (start > 0.0)
  {
    g_ptr_array_add (argv, g_strdup ("-ss"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", start));
  }

  length = ogmrip_codec_get_play_length (OGMRIP_CODEC (video));
  if (length > 0.0)
  {
    guint num, denom;

    ogmdvd_video_stream_get_framerate (OGMDVD_VIDEO_STREAM (stream), &num, &denom);

    g_ptr_array_add (argv, g_strdup ("-frames"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", length * num / denom));
  }

  g_ptr_array_add (argv, g_strdup ("-dvdangle"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", ogmrip_video_codec_get_angle (video)));

  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (output));

  device = ogmdvd_disc_get_device (ogmdvd_title_get_disc (ogmdvd_stream_get_title (stream)));
  g_ptr_array_add (argv, g_strdup ("-dvd-device"));
  g_ptr_array_add (argv, g_strdup (device));

  return argv;
}

/**
 * ogmrip_mplayer_video_watch:
 * @exec: An #OGMJobExec
 * @buffer: The buffer to parse
 * @video: An #OGMRipVideoCodec
 *
 * This function parses the output of mplayer when encoding a video stream and
 * returns the progress made.
 *
 * Returns: The progress made, or -1.0
 */
gdouble
ogmrip_mplayer_video_watch (OGMJobExec *exec, const gchar *buffer, OGMRipVideoCodec *video)
{
  gchar v[10];
  gint frame, decoded;

  if (sscanf (buffer, "V:%s %d/%d", v, &frame, &decoded) == 3)
    return decoded / (gdouble) ogmrip_mplayer_get_frames (OGMRIP_CODEC (video));

  return -1.0;
}

/**
 * ogmrip_mplayer_video_command:
 * @video: An #OGMRipVideoCodec
 * @output: The output file, or NULL
 *
 * This function creates the common part of the command line when using mplayer
 * to encode a video stream.
 *
 * Returns: A new #GPtrArray, or NULL
 */
GPtrArray *
ogmrip_mplayer_video_command (OGMRipVideoCodec *video, const gchar *output)
{
  OGMDvdStream *stream;
  OGMDvdSubpStream *sstream;
  GPtrArray *argv;

  gint format;
  const gchar *device;
  gdouble start, length;
  gboolean forced, scale;
  gchar *chap;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), NULL);

  if (!output)
    output = ogmrip_codec_get_output (OGMRIP_CODEC (video));
  g_return_val_if_fail (output != NULL, NULL);

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (video));

  format = ogmrip_plugin_get_video_codec_format (G_OBJECT_TYPE (video));

  argv = g_ptr_array_new ();

  g_ptr_array_add (argv, g_strdup ("mplayer"));
  g_ptr_array_add (argv, g_strdup ("-nolirc"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-noframedrop"));
  g_ptr_array_add (argv, g_strdup ("-nosound"));
  g_ptr_array_add (argv, g_strdup ("-noslices"));

  if (MPLAYER_CHECK_VERSION (1,0,3,0))
  {
    g_ptr_array_add (argv, g_strdup ("-noconfig"));
    g_ptr_array_add (argv, g_strdup ("all"));
  }

  sstream = ogmrip_video_codec_get_hard_subp (video, &forced);
  if (sstream)
  {
    g_ptr_array_add (argv, g_strdup ("-spuaa"));
    g_ptr_array_add (argv, g_strdup ("20"));
    g_ptr_array_add (argv, g_strdup ("-sid"));
    g_ptr_array_add (argv, g_strdup_printf ("%d",
          ogmdvd_stream_get_id (OGMDVD_STREAM (sstream))));

    if (forced)
      g_ptr_array_add (argv, g_strdup ("-forcedsubsonly"));
  }
  else if (ogmrip_check_mplayer_nosub ())
    g_ptr_array_add (argv, g_strdup ("-nosub"));

  scale = FALSE;

  if (format != OGMRIP_FORMAT_COPY)
  {
    OGMRipScalerType scaler;

    GString *options, *pp;
    guint max_width, max_height;
    guint crop_x, crop_y, crop_width, crop_height;
    guint scale_width, scale_height;
    gboolean crop, expand;

    scaler = ogmrip_video_codec_get_scaler (video);
    g_ptr_array_add (argv, g_strdup ("-sws"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", MAX (scaler, 0)));

    scale = ogmrip_video_codec_get_scale_size (video, &scale_width, &scale_height);

    options = g_string_new (NULL);
    pp = g_string_new (NULL);

    if (ogmrip_video_codec_get_deblock (video))
    {
      if (pp->len > 0)
        g_string_append_c (pp, '/');
      g_string_append (pp, "ha/va");
    }

    if (ogmrip_video_codec_get_dering (video))
    {
      if (pp->len > 0)
        g_string_append_c (pp, '/');
      g_string_append (pp, "dr");
    }

    if (ogmrip_codec_get_telecine (OGMRIP_CODEC (video)))
    {
      if (options->len > 0)
        g_string_append_c (options, ',');
      g_string_append (options, "pullup,softskip");
    }

    crop = ogmrip_video_codec_get_crop_size (video, &crop_x, &crop_y, &crop_width, &crop_height);
    if (crop)
    {
      if (options->len > 0)
        g_string_append_c (options, ',');
      g_string_append_printf (options, "crop=%u:%u:%u:%u", crop_width, crop_height, crop_x, crop_y);
    }

    ogmrip_mplayer_set_deint (video, argv, options, pp);

    if (pp->len > 0)
    {
      if (options->len > 0)
        g_string_append_c (options, ',');
      g_string_append_printf (options, "pp=%s", pp->str);
    }
    g_string_free (pp, TRUE);

    if (scale)
    {
      if (options->len > 0)
        g_string_append_c (options, ',');
      g_string_append_printf (options, "scale=%u:%u", scale_width, scale_height);

      if (ogmrip_video_codec_is_interlaced (video) > 0 && ogmrip_video_codec_get_deinterlacer (video) == OGMRIP_DEINT_NONE)
        g_string_append (options, ":1");
    }

    if (ogmrip_video_codec_get_max_size (video, &max_width, &max_height, &expand) && expand)
    {
      if (options->len > 0)
        g_string_append_c (options, ',');
      g_string_append_printf (options, "expand=%u:%u", max_width, max_height);
    }

    if (ogmrip_video_codec_get_denoise (video))
    {
      if (options->len > 0)
        g_string_append_c (options, ',');
      g_string_append (options, "hqdn3d=2:1:2");
    }

    if (options->len > 0)
      g_string_append_c (options, ',');
    g_string_append (options, "harddup");

    if (options->len == 0)
      g_string_free (options, TRUE);
    else
    {
      g_ptr_array_add (argv, g_strdup ("-vf"));
      g_ptr_array_add (argv, g_string_free (options, FALSE));
    }
  }

  g_ptr_array_add (argv, g_strdup (scale ? "-zoom" : "-nozoom"));

  g_ptr_array_add (argv, g_strdup ("-dvdangle"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", ogmrip_video_codec_get_angle (video)));

  chap = ogmrip_mplayer_get_chapters (OGMRIP_CODEC (video), ogmdvd_stream_get_title (stream));
  if (chap)
  {
    g_ptr_array_add (argv, g_strdup ("-chapter"));
    g_ptr_array_add (argv, chap);
  }

  start = ogmrip_codec_get_start_position (OGMRIP_CODEC (video));
  if (start > 0.0)
  {
    g_ptr_array_add (argv, g_strdup ("-ss"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", start));
  }

  length = ogmrip_codec_get_play_length (OGMRIP_CODEC (video));
  if (length > 0.0)
  {
    guint num, denom;

    ogmdvd_video_stream_get_framerate (OGMDVD_VIDEO_STREAM (stream), &num, &denom);
    g_ptr_array_add (argv, g_strdup ("-frames"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", length * num / denom));
  }

  device = ogmdvd_disc_get_device (ogmdvd_title_get_disc (ogmdvd_stream_get_title (stream)));
  g_ptr_array_add (argv, g_strdup ("-dvd-device"));
  g_ptr_array_add (argv, g_strdup (device));

  return argv;
}

/**
 * ogmrip_mencoder_vobsub_watch:
 * @exec: An #OGMJobExec
 * @buffer: The buffer to parse
 * @subp: An #OGMRipSubpCodec
 *
 * This function parses the output of mencoder when extracting VobSub subtitles
 * and returns the progress made.
 *
 * Returns: The progress made, or -1.0
 */
gdouble
ogmrip_mencoder_vobsub_watch (OGMJobExec *exec, const gchar *buffer, OGMRipSubpCodec *subp)
{
  gint frames, progress;
  gdouble seconds;
  gchar pos[10];

  if (sscanf (buffer, "Pos:%s %df (%d%%)", pos, &frames, &progress) == 3)
  {
    seconds = strtod (pos, NULL);
    return 0.98 * seconds / ogmrip_codec_get_length (OGMRIP_CODEC (subp), NULL);
  }

  return -1.0;
}

/**
 * ogmrip_mencoder_vobsub_command:
 * @subp: An #OGMRipSubpCodec
 * @output: The output file, or NULL
 *
 * This function creates the command line for extracting VobSub subtitles
 * using mencoder.
 *
 * Returns: A new #GPtrArray, or NULL
 */
GPtrArray *
ogmrip_mencoder_vobsub_command (OGMRipSubpCodec *subp, const gchar *output)
{
  OGMDvdStream *stream;

  GPtrArray *argv;
  gdouble start, length;
  const gchar *device;
  gchar *ofps, *chap;
  gint vid, sid;

  g_return_val_if_fail (OGMRIP_IS_SUBP_CODEC (subp), NULL);

  if (!output)
    output = ogmrip_codec_get_output (OGMRIP_CODEC (subp));

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("mencoder"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-nosound"));

  if (MPLAYER_CHECK_VERSION (1,0,3,0))
  {
    g_ptr_array_add (argv, g_strdup ("-noconfig"));
    g_ptr_array_add (argv, g_strdup ("all"));
  }

  if (MPLAYER_CHECK_VERSION (1,0,0,8))
  {
    g_ptr_array_add (argv, g_strdup ("-of"));
    g_ptr_array_add (argv, g_strdup ("rawaudio"));
  }

  g_ptr_array_add (argv, g_strdup ("-ovc"));
  g_ptr_array_add (argv, g_strdup ("copy"));

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (subp));

  ofps = ogmrip_mplayer_get_output_fps (OGMRIP_CODEC (subp), ogmdvd_stream_get_title (stream));
  if (ofps)
  {
    g_ptr_array_add (argv, g_strdup ("-ofps"));
    g_ptr_array_add (argv, ofps);
  }

  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup ("/dev/null"));

  sid = ogmdvd_stream_get_id (OGMDVD_STREAM (stream));
  g_ptr_array_add (argv, g_strdup ("-vobsubout"));
  g_ptr_array_add (argv, g_strdup (output));
  g_ptr_array_add (argv, g_strdup ("-vobsuboutindex"));
  g_ptr_array_add (argv, g_strdup ("0"));
  g_ptr_array_add (argv, g_strdup ("-sid"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", sid));

  chap = ogmrip_mplayer_get_chapters (OGMRIP_CODEC (subp), ogmdvd_stream_get_title (stream));
  if (chap)
  {
    g_ptr_array_add (argv, g_strdup ("-chapter"));
    g_ptr_array_add (argv, chap);
  }

  start = ogmrip_codec_get_start_position (OGMRIP_CODEC (subp));
  if (start > 0.0)
  {
    g_ptr_array_add (argv, g_strdup ("-ss"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", start));
  }

  length = ogmrip_codec_get_play_length (OGMRIP_CODEC (subp));
  if (length > 0.0)
  {
    OGMDvdVideoStream *video;
    guint num, denom;

    video = ogmdvd_title_get_video_stream (ogmdvd_stream_get_title (stream));
    ogmdvd_video_stream_get_framerate (video, &num, &denom);

    g_ptr_array_add (argv, g_strdup ("-frames"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", length * num / denom));
  }

  device = ogmdvd_disc_get_device (ogmdvd_title_get_disc (ogmdvd_stream_get_title (stream)));
  g_ptr_array_add (argv, g_strdup ("-dvd-device"));
  g_ptr_array_add (argv, g_strdup (device));

  vid = ogmdvd_title_get_nr (ogmdvd_stream_get_title (stream));

  if (MPLAYER_CHECK_VERSION (1,0,0,1))
    g_ptr_array_add (argv, g_strdup_printf ("dvd://%d", vid + 1));
  else
  {
    g_ptr_array_add (argv, g_strdup ("-dvd"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", vid + 1));
  }

  g_ptr_array_add (argv, NULL);

  return argv;
}

static void
ogmrip_mencoder_container_append_audio_file (OGMRipContainer *container, 
    const gchar *filename, guint demuxer, gint format, gint language, GPtrArray *argv)
{
  if (filename)
  {
    struct stat buf;

    if (g_stat (filename, &buf) == 0 && buf.st_size > 0)
    {
      if (format == OGMRIP_FORMAT_AAC)
      {
        g_ptr_array_add (argv, g_strdup ("-fafmttag"));
        g_ptr_array_add (argv, g_strdup ("0x706D"));
      }
      else if (demuxer == OGMRIP_AUDIO_DEMUXER_AUTO)
      {
        g_ptr_array_add (argv, g_strdup ("-audio-demuxer"));
        if (MPLAYER_CHECK_VERSION (1,0,1,0))
          g_ptr_array_add (argv, g_strdup ("audio"));
        else
          g_ptr_array_add (argv, g_strdup ("17"));
      }

      if (MPLAYER_CHECK_VERSION (1,0,0,8))
      {
        g_ptr_array_add (argv, g_strdup ("-audiofile"));
        g_ptr_array_add (argv, g_strdup (filename));

        if (demuxer != OGMRIP_AUDIO_DEMUXER_AUTO)
        {
          g_ptr_array_add (argv, g_strdup ("-audio-demuxer"));
          g_ptr_array_add (argv, g_strdup ("rawaudio"));
          g_ptr_array_add (argv, g_strdup ("-rawaudio"));
          g_ptr_array_add (argv, g_strdup_printf ("format=0x%x", demuxer));
        }
      }
      else if (demuxer == OGMRIP_AUDIO_DEMUXER_AUTO)
      {
        g_ptr_array_add (argv, g_strdup ("-audiofile"));
        g_ptr_array_add (argv, g_strdup (filename));
      }
    }
  }
}

static void
ogmrip_mencoder_container_foreach_audio (OGMRipContainer *container, 
    OGMRipCodec *codec, guint demuxer, gint language, GPtrArray *argv)
{
  ogmrip_mencoder_container_append_audio_file (container,
      ogmrip_codec_get_output (codec), demuxer,
      ogmrip_plugin_get_audio_codec_format (G_OBJECT_TYPE (codec)), language, argv);
}

static void
ogmrip_mencoder_container_foreach_file (OGMRipContainer *container, OGMRipFile *file, GPtrArray *argv)
{
  gchar *filename;

  filename = ogmrip_file_get_filename (file);
  if (filename)
  {
    if (ogmrip_file_get_type (file) == OGMRIP_FILE_TYPE_AUDIO)
    {
      gint demuxer, format;

      format = ogmrip_file_get_format (file);
      if (format == OGMRIP_FORMAT_AAC && !g_str_has_suffix (filename, ".aac"))
      {
        gchar *s1, *s2;

        s1 = g_path_get_basename (filename);
        s2 = g_build_filename (g_get_tmp_dir (), s1, NULL);
        g_free (s1);

        s1 = g_strconcat (s2, ".aac", NULL);
        g_free (s2);

        if (symlink (filename, s1) < 0)
          g_free (s1);
        else
        {
          g_free (filename);
          filename = s1;
        }
      }

      demuxer = ogmrip_mplayer_audio_file_get_demuxer (OGMRIP_AUDIO_FILE (file));
      ogmrip_mencoder_container_append_audio_file (container, filename, demuxer, format, -1, argv);
    }
  }
  g_free (filename);
}

/**
 * ogmrip_mencoder_container_command:
 * @container: An #OGMRipContainer
 *
 * This function creates the common part of the command line to merge streams
 * using mencoder.
 *
 * Returns: A new #GPtrArray, or NULL
 */
GPtrArray *
ogmrip_mencoder_container_command (OGMRipContainer *container)
{
  GPtrArray *argv;
  const gchar *fourcc;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("mencoder"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-noskip"));

  if (MPLAYER_CHECK_VERSION (1,0,3,0))
  {
    g_ptr_array_add (argv, g_strdup ("-noconfig"));
    g_ptr_array_add (argv, g_strdup ("all"));
  }

  g_ptr_array_add (argv, g_strdup ("-mc"));
  g_ptr_array_add (argv, g_strdup ("0"));

  g_ptr_array_add (argv, g_strdup ("-ovc"));
  g_ptr_array_add (argv, g_strdup ("copy"));
  g_ptr_array_add (argv, g_strdup ("-oac"));
  g_ptr_array_add (argv, g_strdup ("copy"));

  fourcc = ogmrip_container_get_fourcc (container);
  if (fourcc)
  {
    g_ptr_array_add (argv, g_strdup ("-ffourcc"));
    g_ptr_array_add (argv, g_strdup (fourcc));
  }

  if (MPLAYER_CHECK_VERSION (1,0,0,8))
  {
    const gchar *label;

    label = ogmrip_container_get_label (container);
    if (label)
    {
      g_ptr_array_add (argv, g_strdup ("-info"));
      g_ptr_array_add (argv, g_strdup_printf ("name=%s", label));
    }
  }

  ogmrip_container_foreach_audio (container, 
      (OGMRipContainerCodecFunc) ogmrip_mencoder_container_foreach_audio, argv);
  ogmrip_container_foreach_file (container, 
      (OGMRipContainerFileFunc) ogmrip_mencoder_container_foreach_file, argv);

  return argv;
}

