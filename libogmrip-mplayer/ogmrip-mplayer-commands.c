/* OGMRipMplayer - A library around mplayer/mencoder for OGMRip
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

static GPtrArray *
ogmrip_command_new (const gchar *command)
{
  GPtrArray *argv;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup (command));

  g_ptr_array_add (argv, g_strdup ("-nocache"));
  g_ptr_array_add (argv, g_strdup ("-noslices"));
  g_ptr_array_add (argv, g_strdup ("-noconfig"));
  g_ptr_array_add (argv, g_strdup ("all"));
  g_ptr_array_add (argv, g_strdup ("-demuxer"));
  g_ptr_array_add (argv, g_strdup ("+lavf"));

  return argv;
}

static GPtrArray *
ogmrip_mplayer_command_new (void)
{
  GPtrArray *argv;

  argv = ogmrip_command_new ("mplayer");
  g_ptr_array_add (argv, g_strdup ("-nolirc"));
  g_ptr_array_add (argv, g_strdup ("-noframedrop"));
  g_ptr_array_add (argv, g_strdup ("-nocorrect-pts"));

  return argv;
}

static GPtrArray *
ogmrip_mencoder_command_new (const gchar *output)
{
  GPtrArray *argv;

  argv = ogmrip_command_new ("mencoder");

  if (output)
  {
    g_ptr_array_add (argv, g_strdup ("-o"));
    g_ptr_array_add (argv, g_strdup (output));
  }

  return argv;
}

static void
ogmrip_command_set_fps (GPtrArray *argv, OGMRipTitle *title)
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
  {
    g_ptr_array_add (argv, g_strdup ("-fps"));
    g_ptr_array_add (argv, g_strdup_printf ("%d/%d", num1, denom1));

    g_ptr_array_add (argv, g_strdup ("-ofps"));
    g_ptr_array_add (argv, g_strdup_printf ("%d/%d", num2, denom2));
  }
}

static void
ogmrip_command_set_chapters (GPtrArray *argv, OGMRipCodec *codec)
{
  OGMRipTitle *title;
  gdouble start, length;
  guint first_chapter, last_chapter;
  gint n_chapters;

  title = ogmrip_stream_get_title (ogmrip_codec_get_input (codec));

  ogmrip_codec_get_chapters (codec, &first_chapter, &last_chapter);

  n_chapters = ogmrip_title_get_n_chapters (title);

  if (first_chapter != 0 || last_chapter != n_chapters - 1)
  {
    gchar *str;

    if (last_chapter != n_chapters - 1)
      str = g_strdup_printf ("%d-%d", first_chapter + 1, last_chapter + 1);
    else
      str = g_strdup_printf ("%d", first_chapter + 1);

    g_ptr_array_add (argv, g_strdup ("-chapter"));
    g_ptr_array_add (argv, str);
  }

  start = ogmrip_codec_get_start_position (codec);
  if (start > 0.0)
  {
    g_ptr_array_add (argv, g_strdup ("-ss"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", start));
  }

  length = ogmrip_codec_get_play_length (codec);
  if (length > 0.0)
  {
    OGMRipVideoStream *video;
    guint num, denom;

    video = ogmrip_title_get_video_stream (title);
    ogmrip_video_stream_get_framerate (video, &num, &denom);

    g_ptr_array_add (argv, g_strdup ("-frames"));
    g_ptr_array_add (argv, g_strdup_printf ("%.0lf", length * num / denom));
  }
}

static void
ogmrip_command_set_audio (GPtrArray *argv, OGMRipStream *stream)
{
  if (!stream)
    g_ptr_array_add (argv, g_strdup ("-nosound"));
  else
  {
    g_ptr_array_add (argv, g_strdup ("-aid"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", ogmrip_stream_get_id (stream)));
  }
}

static void
ogmrip_command_set_subp (GPtrArray *argv, OGMRipStream *stream, gboolean forced)
{
  if (!stream)
    g_ptr_array_add (argv, g_strdup ("-nosub"));
  else
  {
    g_ptr_array_add (argv, g_strdup ("-sid"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", ogmrip_stream_get_id (stream)));
    g_ptr_array_add (argv, g_strdup ("-spuaa"));
    g_ptr_array_add (argv, g_strdup ("20"));

    if (forced)
      g_ptr_array_add (argv, g_strdup ("-forcedsubsonly"));
  }
}

static gboolean
ogmrip_command_set_video_filter (GPtrArray *argv, OGMRipVideoCodec *video)
{
  OGMRipStream *stream;
  GString *options, *postproc;

  guint max_width, max_height;
  guint scale_width, scale_height;
  guint crop_x, crop_y, crop_width, crop_height;
  gboolean scale, expand;

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (video));

  options = g_string_new (NULL);
  postproc = g_string_new (NULL);

  if (ogmrip_video_codec_get_deblock (video))
  {
    if (postproc->len > 0)
      g_string_append_c (postproc, '/');
    g_string_append (postproc, "ha/va");
  }

  if (ogmrip_video_codec_get_dering (video))
  {
    if (postproc->len > 0)
      g_string_append_c (postproc, '/');
    g_string_append (postproc, "dr");
  }

  if (ogmrip_title_get_progressive (ogmrip_stream_get_title (stream)) ||
      ogmrip_title_get_telecine (ogmrip_stream_get_title (stream)))
  {
    if (options->len > 0)
      g_string_append_c (options, ',');
    g_string_append (options, "pullup,softskip");
  }

  if (ogmrip_video_codec_get_crop_size (video, &crop_x, &crop_y, &crop_width, &crop_height))
  {
    if (options->len > 0)
      g_string_append_c (options, ',');
    g_string_append_printf (options, "crop=%u:%u:%u:%u", crop_width, crop_height, crop_x, crop_y);
  }

  if (ogmrip_video_codec_get_deinterlacer (video) != OGMRIP_DEINT_NONE)
  {
    if (options->len > 0)
      g_string_append_c (options, ',');

    g_string_append (options, "yadif=3,mcdeint=2:1:10,framestep=2");

    g_ptr_array_add (argv, g_strdup ("-field-dominance"));
    g_ptr_array_add (argv, g_strdup ("-1")); /* or 1 ? */
  }

  if (postproc->len > 0)
  {
    if (options->len > 0)
      g_string_append_c (options, ',');
    g_string_append_printf (options, "pp=%s", postproc->str);
  }
  g_string_free (postproc, TRUE);

  scale = ogmrip_video_codec_get_scale_size (video, &scale_width, &scale_height);
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

  return scale;
}

void
ogmrip_mplayer_set_input (GPtrArray *argv, OGMRipTitle *title, gint angle)
{
  const gchar *uri;

  uri = ogmrip_media_get_uri (ogmrip_title_get_media (title));
  if (g_str_has_prefix (uri, "file://"))
    g_ptr_array_add (argv, g_strdup (uri + 7));
  else if (g_str_has_prefix (uri, "dvd://"))
  {
    if (angle > 0)
    {
      g_ptr_array_add (argv, g_strdup ("-dvdangle"));
      g_ptr_array_add (argv, g_strdup_printf ("%d", angle));
    }

    g_ptr_array_add (argv, g_strdup ("-dvd-device"));
    g_ptr_array_add (argv, g_strdup (uri + 6));
    g_ptr_array_add (argv, g_strdup_printf ("dvd://%d", ogmrip_title_get_id (title) + 1));
  }
  else if (g_str_has_prefix (uri, "br://"))
  {
    if (angle > 0)
    {
      g_ptr_array_add (argv, g_strdup ("-bluray-angle"));
      g_ptr_array_add (argv, g_strdup_printf ("%d", angle));
    }

    g_ptr_array_add (argv, g_strdup ("-bluray-device"));
    g_ptr_array_add (argv, g_strdup (uri + 5));
    g_ptr_array_add (argv, g_strdup_printf ("br://%d", ogmrip_title_get_id (title) + 1));
  }
  else
    g_warning ("Unknown scheme for '%s'", uri);
}

static gboolean
ogmrip_mplayer_watch_stderr (OGMJobTask *task, const gchar *buffer, OGMRipVideoCodec *video, GError **error)
{
  if (g_str_equal (buffer, "Error while decoding frame!"))
  {
    g_set_error (error, OGMRIP_CODEC_ERROR, OGMRIP_CODEC_ERROR_DECODE, _("Error while decoding frame"));
    return FALSE;
  }

  return TRUE;
}

static gboolean
ogmrip_mplayer_wav_watch (OGMJobTask *task, const gchar *buffer, OGMRipAudioCodec *audio, GError **error)
{
  gchar a[12], v[12];
  static gdouble start;
  gdouble secs;

  if (g_str_equal (buffer, "Starting playback..."))
    start = 0;
  else if (sscanf (buffer, "A: %s V: %s", a, v) == 2)
  {
    secs = strtod (a, NULL);
    if (!start)
      start = secs;

    ogmjob_task_set_progress (task, (secs - start) / ogmrip_codec_get_length (OGMRIP_CODEC (audio), NULL));
  }

  return TRUE;
}

OGMJobTask *
ogmrip_mplayer_wav_command (OGMRipAudioCodec *audio, gboolean header, const gchar *output)
{
  OGMJobTask *task;
  OGMRipStream *stream;

  GPtrArray *argv;
  GString *options;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), NULL);
  g_return_val_if_fail (output != NULL, NULL);

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (audio));

  argv = ogmrip_mplayer_command_new ();

  g_ptr_array_add (argv, g_strdup ("-benchmark"));
  g_ptr_array_add (argv, g_strdup ("-vc"));
  g_ptr_array_add (argv, g_strdup ("null"));
  g_ptr_array_add (argv, g_strdup ("-vo"));
  g_ptr_array_add (argv, g_strdup ("null"));

  g_ptr_array_add (argv, g_strdup ("-ao"));

  options = g_string_new ("pcm");

  if (ogmrip_audio_codec_get_fast (audio))
    g_string_append (options, ":fast");

  g_string_append (options, header ? ":waveheader" : ":nowaveheader");
  g_string_append_printf (options, ":file=%s", output);

  g_ptr_array_add (argv, g_string_free (options, FALSE));

  options = g_string_new (NULL);

  g_string_append_printf (options, "format=s16le,channels=%d,resample=%d", 
      ogmrip_audio_codec_get_channels (audio) + 1,
      ogmrip_audio_codec_get_sample_rate (audio));

  if (ogmrip_audio_codec_get_normalize (audio))
    g_string_append (options, ",volnorm=1");

  if (options->len > 0)
  {
    g_ptr_array_add (argv, g_strdup ("-af"));
    g_ptr_array_add (argv, g_strdup (options->str));
  }
  g_string_free (options, TRUE);

  ogmrip_command_set_audio (argv, stream);
  ogmrip_command_set_chapters (argv, OGMRIP_CODEC (audio));
  ogmrip_mplayer_set_input (argv, ogmrip_stream_get_title (stream), 0);

  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (task),
      (OGMJobWatch) ogmrip_mplayer_wav_watch, audio);
  ogmjob_spawn_set_watch_stderr (OGMJOB_SPAWN (task),
      (OGMJobWatch) ogmrip_mplayer_watch_stderr, audio);

  return task;
}

static gboolean
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

OGMJobTask *
ogmrip_mencoder_audio_command (OGMRipAudioCodec *audio, const gchar * const *options, const gchar *output)
{
  OGMJobTask *task;
  OGMRipStream *stream;
  GPtrArray *argv;

  g_return_val_if_fail (OGMRIP_IS_AUDIO_CODEC (audio), NULL);

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (audio));

  argv = ogmrip_mencoder_command_new (output);

  g_ptr_array_add (argv, g_strdup ("-of"));
  g_ptr_array_add (argv, g_strdup ("rawaudio"));

  if (options)
  {
    guint i;

    for (i = 0; options[i]; i ++)
      g_ptr_array_add (argv, g_strdup (options[i]));
  }

  ogmrip_command_set_audio (argv, stream);
  ogmrip_command_set_fps (argv, ogmrip_stream_get_title (stream));
  ogmrip_command_set_chapters (argv, OGMRIP_CODEC (audio));
  ogmrip_mplayer_set_input (argv, ogmrip_stream_get_title (stream), 0);

  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (task),
      (OGMJobWatch) ogmrip_mencoder_codec_watch, audio);
  ogmjob_spawn_set_watch_stderr (OGMJOB_SPAWN (task),
      (OGMJobWatch) ogmrip_mplayer_watch_stderr, audio);

  return task;
}

OGMJobTask *
ogmrip_mencoder_video_command (OGMRipVideoCodec *video, const gchar * const *options, const gchar *output)
{
  OGMJobTask *task;
  OGMRipStream *stream;

  GPtrArray *argv;
  gboolean scale = FALSE;

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (video));

  argv = ogmrip_mencoder_command_new (output);

  if (ogmrip_codec_format (G_OBJECT_TYPE (video)) == OGMRIP_FORMAT_COPY)
    ogmrip_command_set_audio (argv, NULL);
  else
  {
    OGMRipAudioStream *astream;
    OGMRipSubpStream *sstream;
    OGMRipScalerType scaler;
    gboolean forced;

    astream = ogmrip_video_codec_get_ensure_sync (video);
    ogmrip_command_set_audio (argv, OGMRIP_STREAM (astream));

    if (astream)
    {
      g_ptr_array_add (argv, g_strdup ("-oac"));
      g_ptr_array_add (argv, g_strdup ("pcm"));
      g_ptr_array_add (argv, g_strdup ("-srate"));
      g_ptr_array_add (argv, g_strdup ("8000"));
      g_ptr_array_add (argv, g_strdup ("-af"));
      g_ptr_array_add (argv, g_strdup ("format=s16le,channels=1,resample=8000"));
    }

    sstream = ogmrip_video_codec_get_hard_subp (video, &forced);
    ogmrip_command_set_subp (argv, OGMRIP_STREAM (sstream), forced);

    scaler = ogmrip_video_codec_get_scaler (video);
    g_ptr_array_add (argv, g_strdup ("-sws"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", MAX (scaler, 0)));

    scale = ogmrip_command_set_video_filter (argv, video);

    ogmrip_command_set_fps (argv, ogmrip_stream_get_title (stream));
  }

  g_ptr_array_add (argv, g_strdup (scale ? "-zoom": "-nozoom"));

  if (options)
  {
    guint i;

    for (i = 0; options[i]; i ++)
      g_ptr_array_add (argv, g_strdup (options[i]));
  }

  ogmrip_command_set_chapters (argv, OGMRIP_CODEC (video));
  ogmrip_mplayer_set_input (argv,
      ogmrip_stream_get_title (stream),
      ogmrip_video_codec_get_angle (video));

  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (task),
      (OGMJobWatch) ogmrip_mencoder_codec_watch, video);
  ogmjob_spawn_set_watch_stderr (OGMJOB_SPAWN (task),
      (OGMJobWatch) ogmrip_mplayer_watch_stderr, video);

  return task;
}

static gboolean
ogmrip_mplayer_video_watch (OGMJobTask *task, const gchar *buffer, OGMRipVideoCodec *video, GError **error)
{
  gchar v[10];
  gint frame, decoded;

  if (sscanf (buffer, "V:%s %d/%d", v, &frame, &decoded) == 3)
    ogmjob_task_set_progress (task, decoded / (gdouble) ogmrip_mplayer_get_frames (OGMRIP_CODEC (video)));

  return TRUE;
}

OGMJobTask *
ogmrip_mplayer_video_command (OGMRipVideoCodec *video, const gchar * const *options, const gchar *output)
{
  OGMJobTask *task;
  OGMRipSubpStream *sstream;

  GPtrArray *argv;
  gboolean scale = FALSE;

  gboolean forced;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), NULL);
  g_return_val_if_fail (output != NULL, NULL);

  argv = ogmrip_mplayer_command_new ();

  ogmrip_command_set_audio (argv, NULL);

  sstream = ogmrip_video_codec_get_hard_subp (video, &forced);
  ogmrip_command_set_subp (argv, OGMRIP_STREAM (sstream), forced);

  if (ogmrip_codec_format (G_OBJECT_TYPE (video)) != OGMRIP_FORMAT_COPY)
  {
    OGMRipScalerType scaler;

    scaler = ogmrip_video_codec_get_scaler (video);
    g_ptr_array_add (argv, g_strdup ("-sws"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", MAX (scaler, 0)));

    scale = ogmrip_command_set_video_filter (argv, video);
  }

  g_ptr_array_add (argv, g_strdup (scale ? "-zoom" : "-nozoom"));

  if (options)
  {
    guint i;

    for (i = 0; options[i]; i ++)
      g_ptr_array_add (argv, g_strdup (options[i]));
  }

  ogmrip_command_set_chapters (argv, OGMRIP_CODEC (video));
  ogmrip_mplayer_set_input (argv,
      ogmrip_stream_get_title (ogmrip_codec_get_input (OGMRIP_CODEC (video))),
      ogmrip_video_codec_get_angle (video));

  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (task),
      (OGMJobWatch) ogmrip_mplayer_video_watch, video);
  ogmjob_spawn_set_watch_stderr (OGMJOB_SPAWN (task),
      (OGMJobWatch) ogmrip_mplayer_watch_stderr, video);

  return task;
}

static gboolean
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

OGMJobTask *
ogmrip_mencoder_vobsub_command (OGMRipSubpCodec *subp, const gchar *output)
{
  OGMJobTask *task;
  OGMRipStream *stream;

  GPtrArray *argv;

  g_return_val_if_fail (OGMRIP_IS_SUBP_CODEC (subp), NULL);
  g_return_val_if_fail (output != NULL, NULL);

  argv = ogmrip_mencoder_command_new ("/dev/null");

  g_ptr_array_add (argv, g_strdup ("-of"));
  g_ptr_array_add (argv, g_strdup ("rawaudio"));

  g_ptr_array_add (argv, g_strdup ("-ovc"));
  g_ptr_array_add (argv, g_strdup ("copy"));

  ogmrip_command_set_audio (argv, NULL);

  stream = ogmrip_codec_get_input (OGMRIP_CODEC (subp));
  ogmrip_command_set_subp (argv, stream, FALSE);

  g_ptr_array_add (argv, g_strdup ("-vobsubout"));
  g_ptr_array_add (argv, g_strdup (output));
  g_ptr_array_add (argv, g_strdup ("-vobsuboutindex"));
  g_ptr_array_add (argv, g_strdup ("0"));

  ogmrip_command_set_chapters (argv, OGMRIP_CODEC (subp));
  ogmrip_command_set_fps (argv, ogmrip_stream_get_title (stream));
  ogmrip_mplayer_set_input (argv, ogmrip_stream_get_title (stream), 0);

  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (task),
      (OGMJobWatch) ogmrip_mencoder_vobsub_watch, subp);
  ogmjob_spawn_set_watch_stderr (OGMJOB_SPAWN (task),
      (OGMJobWatch) ogmrip_mplayer_watch_stderr, subp);

  return task;
}

static gboolean
ogmrip_mencoder_container_watch (OGMJobTask *task, const gchar *buffer, OGMRipContainer *container, GError **error)
{
  gint frames, progress;
  gchar pos[10];

  if (sscanf (buffer, "Pos:%s %df (%d%%)", pos, &frames, &progress) == 3)
    ogmjob_task_set_progress (task, progress / 100.);

  return TRUE;
}

OGMJobTask *
ogmrip_mencoder_extract_command (OGMRipContainer *container, const gchar *input, const gchar *output)
{
  OGMJobTask *task;
  GPtrArray *argv;

  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (output != NULL, NULL);

  argv = ogmrip_mencoder_command_new (output);
  g_ptr_array_add (argv, g_strdup ("-noskip"));
  g_ptr_array_add (argv, g_strdup ("-mc"));
  g_ptr_array_add (argv, g_strdup ("0"));

  ogmrip_command_set_audio (argv, NULL);
  ogmrip_command_set_subp  (argv, NULL, FALSE);

  g_ptr_array_add (argv, g_strdup ("-ovc"));
  g_ptr_array_add (argv, g_strdup ("copy"));

  g_ptr_array_add (argv, g_strdup ("-of"));
  g_ptr_array_add (argv, g_strdup ("rawvideo"));

  g_ptr_array_add (argv, g_strdup (input));

  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  ogmjob_spawn_set_watch_stdout (OGMJOB_SPAWN (task),
      (OGMJobWatch) ogmrip_mencoder_container_watch, container);

  return task;
}

