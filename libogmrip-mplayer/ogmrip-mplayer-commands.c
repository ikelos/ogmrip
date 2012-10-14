/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-mplayer-commands.h"
#include "ogmrip-mplayer-version.h"

#include <ogmrip-base.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

static const gchar *deinterlacer[] = { "lb", "li", "ci", "md", "fd", "l5", "kerndeint", "yadif" };

void
ogmrip_mplayer_set_input (GPtrArray *argv, OGMRipTitle *title)
{
  const gchar *uri;

  uri = ogmrip_media_get_uri (ogmrip_title_get_media (title));
  if (g_str_has_prefix (uri, "file://"))
    g_ptr_array_add (argv, g_strdup (uri + 7));
  else if (g_str_has_prefix (uri, "dvd://"))
  {
    g_ptr_array_add (argv, g_strdup ("-dvd-device"));
    g_ptr_array_add (argv, g_strdup (uri + 6));
    g_ptr_array_add (argv, g_strdup_printf ("dvd://%d", ogmrip_title_get_nr (title) + 1));
  }
  else if (g_str_has_prefix (uri, "br://"))
  {
    g_ptr_array_add (argv, g_strdup ("-bluray-device"));
    g_ptr_array_add (argv, g_strdup (uri + 5));
    g_ptr_array_add (argv, g_strdup_printf ("br://%d", ogmrip_title_get_nr (title)));
  }
  else
    g_warning ("Unknown scheme for '%s'", uri);

}

static gint
ogmrip_mplayer_map_audio_id (OGMRipStream *astream)
{
  const gchar *uri;
  gint aid;

  aid = ogmrip_stream_get_id (astream);

  uri = ogmrip_media_get_uri (ogmrip_title_get_media (ogmrip_stream_get_title (astream)));
  if (!g_str_has_prefix (uri, "dvd://"))
    return aid;

  switch (ogmrip_stream_get_format (astream))
  {
    case OGMRIP_FORMAT_AC3:
      aid += 128;
      break;
    case OGMRIP_FORMAT_DTS:
      aid += 136;
      break;
    case OGMRIP_FORMAT_PCM:
      aid += 160;
      break;
    default:
      break;
  }

  return aid;
}

static glong
ogmrip_mplayer_get_frames (OGMRipCodec *codec)
{
  OGMRipStream *stream;
  guint num, denom;
  gdouble length;

  length = ogmrip_codec_get_length (codec, NULL);

  stream = ogmrip_codec_get_input (codec);
  ogmrip_video_stream_get_framerate (OGMRIP_VIDEO_STREAM (stream), &num, &denom);

  return length / (gdouble) denom * num;
}

static gchar *
ogmrip_mplayer_get_output_fps (OGMRipCodec *codec, OGMRipTitle *title)
{
  OGMRipVideoStream *stream;
  guint num1, denom1, num2, denom2;

  stream = ogmrip_title_get_video_stream (title);
  ogmrip_video_stream_get_framerate (stream, &num1, &denom1);

  if (ogmrip_title_get_telecine (title) || ogmrip_title_get_progressive (title))
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
ogmrip_mplayer_get_chapters (OGMRipCodec *codec, OGMRipTitle *title)
{
  guint start, end;
  gint n_chap;

  ogmrip_codec_get_chapters (codec, &start, &end);

  n_chap = ogmrip_title_get_n_chapters (title);

  if (start != 0 || end != n_chap - 1)
  {
    gchar *str;

    if (end != n_chap - 1)
    {
      ogmrip_title_get_n_chapters (title);
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
        g_string_append (options, "=3");

        if (ogmrip_mplayer_check_mcdeint ())
          g_string_append (options, ",mcdeint=2:1:10");

        g_string_append (options, ",framestep=2");

        g_ptr_array_add (argv, g_strdup ("-field-dominance"));
        g_ptr_array_add (argv, g_strdup ("-1")); /* or 1 ? */
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

gboolean
ogmrip_mencoder_codec_watch (OGMJobTask *task, const gchar *buffer, OGMRipCodec *codec, GError **error)
{
  gint frames, progress;
  gdouble seconds;
  gchar pos[10];

  if (sscanf (buffer, "Pos:%s %df (%d%%)", pos, &frames, &progress) == 3)
  {
    seconds = strtod (pos, NULL);
    ogmjob_task_set_progress (task, seconds / ogmrip_codec_get_length (codec, NULL));
  }

  return TRUE;
}

gboolean
ogmrip_mencoder_container_watch (OGMJobTask *task, const gchar *buffer, OGMRipContainer *container, GError **error)
{
  gint frames, progress;
  gchar pos[10];

  if (sscanf (buffer, "Pos:%s %df (%d%%)", pos, &frames, &progress) == 3)
    ogmjob_task_set_progress (task, progress / 100.);

  return TRUE;
}

GPtrArray *
ogmrip_mplayer_wav_command (OGMRipAudioCodec *audio, gboolean header, const gchar *output)
{
  OGMRipStream *stream;

  GPtrArray *argv;
  GString *options;

  gint srate;
  gdouble start, length;
  gchar *chap;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), NULL);
  g_return_val_if_fail (output != NULL, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("mplayer"));
  g_ptr_array_add (argv, g_strdup ("-nolirc"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-noframedrop"));

  g_ptr_array_add (argv, g_strdup ("-noconfig"));
  g_ptr_array_add (argv, g_strdup ("all"));

  length = ogmrip_codec_get_play_length (OGMRIP_CODEC (audio));

  if (length <= 0.0)
  {
    g_ptr_array_add (argv, g_strdup ("-vc"));
    g_ptr_array_add (argv, g_strdup ("dummy"));
  }

  g_ptr_array_add (argv, g_strdup ("-vo"));
  g_ptr_array_add (argv, g_strdup ("null"));
  g_ptr_array_add (argv, g_strdup ("-ao"));

  options = g_string_new ("pcm");

  if (ogmrip_audio_codec_get_fast (audio))
    g_string_append (options, ":fast");

  g_string_append (options, header ? ":waveheader" : ":nowaveheader");
  g_string_append_printf (options, ":file=%s", output);

  g_ptr_array_add (argv, g_string_free (options, FALSE));

  g_ptr_array_add (argv, g_strdup ("-format"));
  g_ptr_array_add (argv, g_strdup ("s16le"));

  options = g_string_new (NULL);

  if (ogmrip_audio_codec_get_normalize (audio))
    g_string_append (options, "volnorm=1");

  srate = ogmrip_audio_codec_get_sample_rate (audio);
  if (srate != 48000)
  {
    g_ptr_array_add (argv, g_strdup ("-srate"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", srate));

    if (options->len > 0)
      g_string_append_c (options, ',');
    g_string_append_printf (options, "lavcresample=%d", srate);
  }

  if (options->len == 0)
    g_string_free (options, TRUE);
  else
  {
    g_ptr_array_add (argv, g_strdup ("-af"));
    g_ptr_array_add (argv, g_string_free (options, FALSE));
  }

  g_ptr_array_add (argv, g_strdup ("-channels"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", ogmrip_audio_codec_get_channels (audio) + 1));

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (audio));

  chap = ogmrip_mplayer_get_chapters (OGMRIP_CODEC (audio), ogmrip_stream_get_title (stream));
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
    OGMRipVideoStream *video;
    guint num, denom;

    video = ogmrip_title_get_video_stream (ogmrip_stream_get_title (stream));
    ogmrip_video_stream_get_framerate (video, &num, &denom);

    g_ptr_array_add (argv, g_strdup ("-frames"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", length * num / denom));
  }

  g_ptr_array_add (argv, g_strdup ("-aid"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", ogmrip_mplayer_map_audio_id (stream)));

  ogmrip_mplayer_set_input (argv, ogmrip_stream_get_title (stream));

  g_ptr_array_add (argv, NULL);

  return argv;
}

gboolean
ogmrip_mplayer_wav_watch (OGMJobTask *task, const gchar *buffer, OGMRipAudioCodec *audio, GError **error)
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

    ogmjob_task_set_progress (task, (secs - start) / ogmrip_codec_get_length (OGMRIP_CODEC (audio), NULL));
  }

  return TRUE;
}

GPtrArray *
ogmrip_mencoder_audio_command (OGMRipAudioCodec *audio, const gchar *output)
{
  GPtrArray *argv;
  OGMRipStream *stream;

  gdouble start, length;
  gchar *ofps, *chap;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), NULL);

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (audio));

  argv = g_ptr_array_new ();

  g_ptr_array_add (argv, g_strdup ("mencoder"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));

  g_ptr_array_add (argv, g_strdup ("-noconfig"));
  g_ptr_array_add (argv, g_strdup ("all"));

  ofps = ogmrip_mplayer_get_output_fps (OGMRIP_CODEC (audio), ogmrip_stream_get_title (stream));
  if (ofps)
  {
    g_ptr_array_add (argv, g_strdup ("-ofps"));
    g_ptr_array_add (argv, ofps);
  }

  chap = ogmrip_mplayer_get_chapters (OGMRIP_CODEC (audio), ogmrip_stream_get_title (stream));
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
    OGMRipVideoStream *video;
    guint num, denom;

    video = ogmrip_title_get_video_stream (ogmrip_stream_get_title (stream));
    ogmrip_video_stream_get_framerate (video, &num, &denom);

    g_ptr_array_add (argv, g_strdup ("-frames"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", length * num / denom));
  }

  g_ptr_array_add (argv, g_strdup ("-aid"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", ogmrip_mplayer_map_audio_id (stream)));

  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (output));

  ogmrip_mplayer_set_input (argv, ogmrip_stream_get_title (stream));

  return argv;
}

GPtrArray *
ogmrip_mencoder_video_command (OGMRipVideoCodec *video, const gchar *output, guint pass)
{
  GPtrArray *argv;
  OGMRipStream *stream;

  gdouble start, length;
  gchar *ofps, *chap;
  gboolean scale;

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (video));

  argv = g_ptr_array_new ();

  g_ptr_array_add (argv, g_strdup ("mencoder"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-noslices"));

  g_ptr_array_add (argv, g_strdup ("-noconfig"));
  g_ptr_array_add (argv, g_strdup ("all"));

  scale = FALSE;

  if (ogmrip_codec_format (G_OBJECT_TYPE (video)) == OGMRIP_FORMAT_COPY)
    g_ptr_array_add (argv, g_strdup ("-nosound"));
  else
  {
    OGMRipAudioStream *astream;
    OGMRipSubpStream *sstream;
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
      g_ptr_array_add (argv, g_strdup_printf ("%d", ogmrip_mplayer_map_audio_id (OGMRIP_STREAM (astream))));
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
            ogmrip_stream_get_id (OGMRIP_STREAM (sstream))));

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

    if (ogmrip_title_get_telecine (ogmrip_stream_get_title (stream)))
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

      if (ogmrip_title_get_interlaced (ogmrip_stream_get_title (stream)) &&
          ogmrip_video_codec_get_deinterlacer (video) == OGMRIP_DEINT_NONE)
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

    ofps = ogmrip_mplayer_get_output_fps (OGMRIP_CODEC (video), ogmrip_stream_get_title (stream));
    if (ofps)
    {
      g_ptr_array_add (argv, g_strdup ("-ofps"));
      g_ptr_array_add (argv, ofps);
    }
  }

  g_ptr_array_add (argv, g_strdup (scale ? "-zoom": "-nozoom"));

  chap = ogmrip_mplayer_get_chapters (OGMRIP_CODEC (video), ogmrip_stream_get_title (stream));
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

    ogmrip_video_stream_get_framerate (OGMRIP_VIDEO_STREAM (stream), &num, &denom);

    g_ptr_array_add (argv, g_strdup ("-frames"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", length * num / denom));
  }

  g_ptr_array_add (argv, g_strdup ("-dvdangle"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", ogmrip_video_codec_get_angle (video)));

  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (output));

  return argv;
}

gboolean
ogmrip_mplayer_video_watch (OGMJobTask *task, const gchar *buffer, OGMRipVideoCodec *video, GError **error)
{
  gchar v[10];
  gint frame, decoded;

  if (sscanf (buffer, "V:%s %d/%d", v, &frame, &decoded) == 3)
    ogmjob_task_set_progress (task, decoded / (gdouble) ogmrip_mplayer_get_frames (OGMRIP_CODEC (video)));

  return TRUE;
}

GPtrArray *
ogmrip_mplayer_video_command (OGMRipVideoCodec *video, const gchar *output)
{
  OGMRipStream *stream;
  OGMRipSubpStream *sstream;
  GPtrArray *argv;

  gdouble start, length;
  gboolean forced, scale;
  gchar *chap;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), NULL);
  g_return_val_if_fail (output != NULL, NULL);

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (video));

  argv = g_ptr_array_new ();

  g_ptr_array_add (argv, g_strdup ("mplayer"));
  g_ptr_array_add (argv, g_strdup ("-nolirc"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-noframedrop"));
  g_ptr_array_add (argv, g_strdup ("-nosound"));
  g_ptr_array_add (argv, g_strdup ("-noslices"));

  g_ptr_array_add (argv, g_strdup ("-noconfig"));
  g_ptr_array_add (argv, g_strdup ("all"));

  sstream = ogmrip_video_codec_get_hard_subp (video, &forced);
  if (sstream)
  {
    g_ptr_array_add (argv, g_strdup ("-spuaa"));
    g_ptr_array_add (argv, g_strdup ("20"));
    g_ptr_array_add (argv, g_strdup ("-sid"));
    g_ptr_array_add (argv, g_strdup_printf ("%d",
          ogmrip_stream_get_id (OGMRIP_STREAM (sstream))));

    if (forced)
      g_ptr_array_add (argv, g_strdup ("-forcedsubsonly"));
  }
  else if (ogmrip_check_mplayer_nosub ())
    g_ptr_array_add (argv, g_strdup ("-nosub"));

  scale = FALSE;

  if (ogmrip_codec_format (G_OBJECT_TYPE (video)) != OGMRIP_FORMAT_COPY)
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

    if (ogmrip_title_get_telecine (ogmrip_stream_get_title (stream)))
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

      if (ogmrip_title_get_interlaced (ogmrip_stream_get_title (stream)) &&
          ogmrip_video_codec_get_deinterlacer (video) == OGMRIP_DEINT_NONE)
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

  chap = ogmrip_mplayer_get_chapters (OGMRIP_CODEC (video), ogmrip_stream_get_title (stream));
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

    ogmrip_video_stream_get_framerate (OGMRIP_VIDEO_STREAM (stream), &num, &denom);
    g_ptr_array_add (argv, g_strdup ("-frames"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", length * num / denom));
  }

  return argv;
}

gboolean
ogmrip_mencoder_vobsub_watch (OGMJobTask *task, const gchar *buffer, OGMRipSubpCodec *subp, GError **error)
{
  gint frames, progress;
  gdouble seconds;
  gchar pos[10];

  if (sscanf (buffer, "Pos:%s %df (%d%%)", pos, &frames, &progress) == 3)
  {
    seconds = strtod (pos, NULL);
    ogmjob_task_set_progress (task, 0.98 * seconds / ogmrip_codec_get_length (OGMRIP_CODEC (subp), NULL));
  }

  return TRUE;
}

GPtrArray *
ogmrip_mencoder_vobsub_command (OGMRipSubpCodec *subp, const gchar *output)
{
  OGMRipStream *stream;

  GPtrArray *argv;
  gdouble start, length;
  gchar *ofps, *chap;
  gint sid;

  g_return_val_if_fail (OGMRIP_IS_SUBP_CODEC (subp), NULL);
  g_return_val_if_fail (output != NULL, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("mencoder"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-nosound"));

  g_ptr_array_add (argv, g_strdup ("-noconfig"));
  g_ptr_array_add (argv, g_strdup ("all"));

  g_ptr_array_add (argv, g_strdup ("-of"));
  g_ptr_array_add (argv, g_strdup ("rawaudio"));

  g_ptr_array_add (argv, g_strdup ("-ovc"));
  g_ptr_array_add (argv, g_strdup ("copy"));

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (subp));

  ofps = ogmrip_mplayer_get_output_fps (OGMRIP_CODEC (subp), ogmrip_stream_get_title (stream));
  if (ofps)
  {
    g_ptr_array_add (argv, g_strdup ("-ofps"));
    g_ptr_array_add (argv, ofps);
  }

  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup ("/dev/null"));

  sid = ogmrip_stream_get_id (OGMRIP_STREAM (stream));
  g_ptr_array_add (argv, g_strdup ("-vobsubout"));
  g_ptr_array_add (argv, g_strdup (output));
  g_ptr_array_add (argv, g_strdup ("-vobsuboutindex"));
  g_ptr_array_add (argv, g_strdup ("0"));
  g_ptr_array_add (argv, g_strdup ("-sid"));
  g_ptr_array_add (argv, g_strdup_printf ("%d", sid));

  chap = ogmrip_mplayer_get_chapters (OGMRIP_CODEC (subp), ogmrip_stream_get_title (stream));
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
    OGMRipVideoStream *video;
    guint num, denom;

    video = ogmrip_title_get_video_stream (ogmrip_stream_get_title (stream));
    ogmrip_video_stream_get_framerate (video, &num, &denom);

    g_ptr_array_add (argv, g_strdup ("-frames"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", length * num / denom));
  }

  ogmrip_mplayer_set_input (argv, ogmrip_stream_get_title (stream));

  g_ptr_array_add (argv, NULL);

  return argv;
}

static void
ogmrip_mencoder_container_append_audio_file (OGMRipContainer *container, 
    const gchar *filename, gint format, GPtrArray *argv)
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
      else if (format != OGMRIP_FORMAT_AC3 && format != OGMRIP_FORMAT_DTS)
      {
        g_ptr_array_add (argv, g_strdup ("-audio-demuxer"));
        g_ptr_array_add (argv, g_strdup ("audio"));
      }

      g_ptr_array_add (argv, g_strdup ("-audiofile"));
      g_ptr_array_add (argv, g_strdup (filename));

      if (format == OGMRIP_FORMAT_AC3 || format == OGMRIP_FORMAT_DTS)
      {
        g_ptr_array_add (argv, g_strdup ("-audio-demuxer"));
        g_ptr_array_add (argv, g_strdup ("rawaudio"));
        g_ptr_array_add (argv, g_strdup ("-rawaudio"));
        if (format == OGMRIP_FORMAT_AC3)
          g_ptr_array_add (argv, g_strdup ("format=0x2000"));
        else
          g_ptr_array_add (argv, g_strdup ("format=0x2001"));
      }
    }
  }
}

static void
ogmrip_mencoder_container_foreach_file (OGMRipContainer *container, OGMRipFile *file, GPtrArray *argv)
{
  if (OGMRIP_IS_AUDIO_STREAM (file))
  {
    gint format;
    gchar *filename;

    format = ogmrip_stream_get_format (OGMRIP_STREAM (file));
    filename = g_strdup (ogmrip_file_get_path (file));

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

    ogmrip_mencoder_container_append_audio_file (container, filename, format, argv);
    g_free (filename);
  }
}

GPtrArray *
ogmrip_mencoder_container_command (OGMRipContainer *container)
{
  GPtrArray *argv;
  const gchar *str;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("mencoder"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-noskip"));

  g_ptr_array_add (argv, g_strdup ("-noconfig"));
  g_ptr_array_add (argv, g_strdup ("all"));

  g_ptr_array_add (argv, g_strdup ("-mc"));
  g_ptr_array_add (argv, g_strdup ("0"));

  g_ptr_array_add (argv, g_strdup ("-ovc"));
  g_ptr_array_add (argv, g_strdup ("copy"));
  g_ptr_array_add (argv, g_strdup ("-oac"));
  g_ptr_array_add (argv, g_strdup ("copy"));

  str = ogmrip_container_get_fourcc (container);
  if (str)
  {
    g_ptr_array_add (argv, g_strdup ("-ffourcc"));
    g_ptr_array_add (argv, g_strdup (str));
  }

  str = ogmrip_container_get_label (container);
  if (str)
  {
    g_ptr_array_add (argv, g_strdup ("-info"));
    g_ptr_array_add (argv, g_strdup_printf ("name=%s", str));
  }

  ogmrip_container_foreach_file (container, 
      (OGMRipContainerFunc) ogmrip_mencoder_container_foreach_file, argv);

  return argv;
}

gboolean
ogmrip_mplayer_watch_stderr (OGMJobTask *task, const gchar *buffer, OGMRipVideoCodec *video, GError **error)
{
  if (g_str_equal (buffer, "Error while decoding frame!"))
  {
    g_set_error (error, OGMRIP_CODEC_ERROR, OGMRIP_CODEC_ERROR_DECODE, _("Error while decoding frame"));
    return FALSE;
  }

  return TRUE;
}

GPtrArray *
ogmrip_mplayer_grab_frame_command (OGMRipTitle *title, guint position, gboolean deint)
{
  GPtrArray *argv;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("mplayer"));
  g_ptr_array_add (argv, g_strdup ("-nolirc"));
  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-nosound"));
  g_ptr_array_add (argv, g_strdup ("-nozoom"));

  if (ogmrip_check_mplayer_nosub ())
    g_ptr_array_add (argv, g_strdup ("-nosub"));

  g_ptr_array_add (argv, g_strdup ("-vo"));

  g_ptr_array_add (argv, g_strdup_printf ("jpeg:outdir=%s", ogmrip_fs_get_tmp_dir ()));

  g_ptr_array_add (argv, g_strdup ("-frames"));
  g_ptr_array_add (argv, g_strdup ("1"));

  g_ptr_array_add (argv, g_strdup ("-speed"));
  g_ptr_array_add (argv, g_strdup ("100"));

  if (deint)
  {
    g_ptr_array_add (argv, g_strdup ("-vf"));
    g_ptr_array_add (argv, g_strdup ("pp=lb"));
  }
/*
  time_ = (gint) (frame * dialog->priv->rate_denominator / (gdouble) dialog->priv->rate_numerator);
*/
  g_ptr_array_add (argv, g_strdup ("-ss"));
  g_ptr_array_add (argv, g_strdup_printf ("%u", position));

  ogmrip_mplayer_set_input (argv, title);

  g_ptr_array_add (argv, NULL);

  return argv;
}

