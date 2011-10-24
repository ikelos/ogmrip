/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ogmrip-encode.h>
#include <ogmrip-mplayer.h>

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#define OGMRIP_TYPE_VP8          (ogmrip_vp8_get_type ())
#define OGMRIP_VP8(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_VP8, OGMRipVp8))
#define OGMRIP_VP8_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_VP8, OGMRipVp8Class))
#define OGMRIP_IS_VP8(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_VP8))
#define OGMRIP_IS_VP8_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_VP8))

typedef struct _OGMRipVp8      OGMRipVp8;
typedef struct _OGMRipVp8Class OGMRipVp8Class;

struct _OGMRipVp8
{
  OGMRipVideoCodec parent_instance;

  gboolean best;
  gint minsection_pct;
  gint maxsection_pct;
  gint token_parts;
  gint drop_frame;
  gint min_q;
  gint max_q;
  gint profile;
  gint cpu_used;
};

struct _OGMRipVp8Class
{
  OGMRipVideoCodecClass parent_class;
};

static GType ogmrip_vp8_get_type (void);
static void  ogmrip_vp8_notify   (GObject     *gobject,
                                  GParamSpec  *pspec);
static gint  ogmrip_vp8_run      (OGMJobSpawn *spawn);

static gboolean have_vpxenc = FALSE;

static gchar **
ogmrip_yuv4mpeg_command (OGMRipVideoCodec *video, const gchar *fifo)
{
  OGMRipTitle *title;
  GPtrArray *argv;
  gint vid;

  argv = ogmrip_mplayer_video_command (video, fifo);

  g_ptr_array_add (argv, g_strdup ("-vo"));
  if (MPLAYER_CHECK_VERSION (1,0,0,6))
    g_ptr_array_add (argv, g_strdup_printf ("yuv4mpeg:file=%s", fifo));
  else
    g_ptr_array_add (argv, g_strdup ("yuv4mpeg"));

  title = ogmrip_stream_get_title (ogmrip_codec_get_input (OGMRIP_CODEC (video)));
  vid = ogmrip_title_get_nr (title);

  if (MPLAYER_CHECK_VERSION (1,0,0,1))
    g_ptr_array_add (argv, g_strdup_printf ("dvd://%d", vid + 1));
  else
  {
    g_ptr_array_add (argv, g_strdup ("-dvd"));
    g_ptr_array_add (argv, g_strdup_printf ("%d", vid + 1));
  }

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static gchar **
ogmrip_vp8_command (OGMRipVideoCodec *video, const gchar *fifo, guint pass, guint passes, const gchar *log_file)
{
  GPtrArray *argv;
  const gchar *output;
  guint width, height, num, denom;
  gint bitrate, threads;

  argv = g_ptr_array_new ();

  if (have_vpxenc)
    g_ptr_array_add (argv, g_strdup ("vpxenc"));
  else
    g_ptr_array_add (argv, g_strdup ("ivfenc"));

  ogmrip_video_codec_get_scale_size (video, &width, &height);
  g_ptr_array_add (argv, g_strdup_printf ("--width=%u", width));
  g_ptr_array_add (argv, g_strdup_printf ("--height=%u", height));

  ogmrip_video_codec_get_framerate (video, &num, &denom);
  g_ptr_array_add (argv, g_strdup_printf ("--timebase=%u/%u", denom, num));

  bitrate = ogmrip_video_codec_get_bitrate (video);
  if (bitrate > 0)
  {
    g_ptr_array_add (argv, g_strdup_printf ("--target-bitrate=%d", bitrate / 1000));

    if (OGMRIP_VP8 (video)->min_q >= 0)
      g_ptr_array_add (argv, g_strdup_printf ("--min-q=%u", OGMRIP_VP8 (video)->min_q));

    if (OGMRIP_VP8 (video)->max_q >= 0)
      g_ptr_array_add (argv, g_strdup_printf ("--max-q=%u", OGMRIP_VP8 (video)->max_q));
  }
  else
  {
    gdouble quantizer;

    quantizer = 48. * ogmrip_video_codec_get_quantizer (video) / 31.;
    g_ptr_array_add (argv, g_strdup_printf ("--min-q=%.0lf", quantizer));
    g_ptr_array_add (argv, g_strdup_printf ("--max-q=%.0lf", quantizer + 15.));
  }

  g_ptr_array_add (argv, g_strdup_printf ("--passes=%u", passes));

  if (passes > 1 && log_file)
  {
    g_ptr_array_add (argv, g_strdup_printf ("--pass=%u", pass));
    g_ptr_array_add (argv, g_strdup_printf ("--fpf=%s", log_file));

    if (OGMRIP_VP8 (video)->minsection_pct >= 0)
      g_ptr_array_add (argv, g_strdup_printf ("--minsection-pct=%u", OGMRIP_VP8 (video)->minsection_pct));

    if (OGMRIP_VP8 (video)->maxsection_pct >= 0)
      g_ptr_array_add (argv, g_strdup_printf ("--maxsection-pct=%u", OGMRIP_VP8 (video)->maxsection_pct));
  }

  g_ptr_array_add (argv, g_strdup ("--end-usage=0"));
  g_ptr_array_add (argv, g_strdup ("--auto-alt-ref=1"));
  g_ptr_array_add (argv, g_strdup ("--lag-in-frames=16"));
  g_ptr_array_add (argv, g_strdup ("--kf-min-dist=0"));
  g_ptr_array_add (argv, g_strdup ("--kf-max-dist=360"));
  g_ptr_array_add (argv, g_strdup ("--static-thresh=0"));

  g_ptr_array_add (argv, g_strdup (OGMRIP_VP8 (video)->best ? "--best" : "--good"));

  threads = ogmrip_video_codec_get_threads (video);
  if (threads > 0)
    g_ptr_array_add (argv, g_strdup_printf ("--threads=%u", threads));

  if (OGMRIP_VP8 (video)->token_parts >= 0)
    g_ptr_array_add (argv, g_strdup_printf ("--token-parts=%u", OGMRIP_VP8 (video)->token_parts));

  if (OGMRIP_VP8 (video)->drop_frame >= 0)
    g_ptr_array_add (argv, g_strdup_printf ("--drop-frame=%u", OGMRIP_VP8 (video)->drop_frame));

  if (OGMRIP_VP8 (video)->profile >= 0)
    g_ptr_array_add (argv, g_strdup_printf ("--profile=%u", OGMRIP_VP8 (video)->profile));

  if (OGMRIP_VP8 (video)->cpu_used >= 0)
    g_ptr_array_add (argv, g_strdup_printf ("--cpu-used=%u", OGMRIP_VP8 (video)->cpu_used));

  g_ptr_array_add (argv, g_strdup (fifo));

  output = ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (video)));
  g_ptr_array_add (argv, g_strdup (output));

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

G_DEFINE_TYPE (OGMRipVp8, ogmrip_vp8, OGMRIP_TYPE_VIDEO_CODEC)

static void
ogmrip_vp8_class_init (OGMRipVp8Class *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->notify = ogmrip_vp8_notify;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmrip_vp8_run;
}

static void
ogmrip_vp8_init (OGMRipVp8 *vp8)
{
  vp8->best = FALSE;
  vp8->minsection_pct = -1;
  vp8->maxsection_pct = -1;
  vp8->token_parts = -1;
  vp8->drop_frame = -1;
  vp8->min_q = -1;
  vp8->max_q = -1;
  vp8->profile = -1;
  vp8->cpu_used = -1;
}

static void
ogmrip_vp8_notify (GObject *gobject, GParamSpec *pspec)
{
  if (g_str_equal (pspec->name, "quality"))
  {
    OGMRipVp8 *vp8 = OGMRIP_VP8 (gobject);

    switch (ogmrip_video_codec_get_quality (OGMRIP_VIDEO_CODEC (vp8)))
    {
      case OGMRIP_QUALITY_EXTREME:
        vp8->best = FALSE;
        vp8->minsection_pct = 5;
        vp8->maxsection_pct = 800;
        vp8->token_parts = 2;
        vp8->drop_frame = 0;
        vp8->min_q = 0;
        vp8->max_q = 60;
        vp8->profile = -1;
        vp8->cpu_used = 0;
        break;
      case OGMRIP_QUALITY_HIGH:
        vp8->best = FALSE;
        vp8->minsection_pct = 5;
        vp8->maxsection_pct = 800;
        vp8->token_parts = 2;
        vp8->drop_frame = -1;
        vp8->min_q = 0;
        vp8->max_q = 60;
        vp8->profile = -1;
        vp8->cpu_used = 1;
        break;
      default:
        vp8->best = FALSE;
        vp8->minsection_pct = 15;
        vp8->maxsection_pct = 400;
        vp8->token_parts = -1;
        vp8->drop_frame = -1;
        vp8->min_q = 4;
        vp8->max_q = 63;
        vp8->profile = 1;
        vp8->cpu_used = 0;
        break;
    }
  }
}

static OGMJobSpawn *
ogmrip_vp8_pipeline (OGMJobSpawn *spawn, const gchar *fifo, guint pass, guint passes, const gchar *log_file)
{
  OGMJobSpawn *pipeline, *child;
  gchar **argv;

  pipeline = ogmjob_pipeline_new ();

  argv = ogmrip_yuv4mpeg_command (OGMRIP_VIDEO_CODEC (spawn), fifo);
  if (argv)
  {
    child = ogmjob_exec_newv (argv);
    ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_mplayer_video_watch, spawn, TRUE, FALSE, FALSE);
    ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
    g_object_unref (child);

    argv = ogmrip_vp8_command (OGMRIP_VIDEO_CODEC (spawn), fifo, pass + 1, passes, log_file);
    if (argv)
    {
      child = ogmjob_exec_newv (argv);
      ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
      g_object_unref (child);
    }
  }

  return pipeline;
}

static gint
ogmrip_vp8_run (OGMJobSpawn *spawn)
{
  GError *error = NULL;
  OGMJobSpawn *queue, *pipeline;
  gint pass, passes, result;
  gchar *fifo, *log_file;

  result = OGMJOB_RESULT_ERROR;

  fifo = ogmrip_fs_mkftemp ("fifo.XXXXXX", &error);
  if (!fifo)
  {
    ogmjob_spawn_propagate_error (spawn, error);
    return OGMJOB_RESULT_ERROR;
  }

  queue = ogmjob_queue_new ();
  ogmjob_container_add (OGMJOB_CONTAINER (spawn), queue);
  g_object_unref (queue);

  passes = ogmrip_video_codec_get_passes (OGMRIP_VIDEO_CODEC (spawn));

  log_file = passes > 1 ? ogmrip_fs_mktemp ("log.XXXXXX", NULL) : NULL;

  for (pass = 0; pass < passes; pass ++)
  {
    pipeline = ogmrip_vp8_pipeline (spawn, fifo, pass, passes, log_file);
    ogmjob_container_add (OGMJOB_CONTAINER (queue), pipeline);
    g_object_unref (pipeline);
  }

  result = OGMJOB_SPAWN_CLASS (ogmrip_vp8_parent_class)->run (spawn);

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), queue);

  if (g_file_test (fifo, G_FILE_TEST_EXISTS))
    g_unlink (fifo);
  g_free (fifo);

  if (g_file_test (log_file, G_FILE_TEST_EXISTS))
    g_unlink (log_file);
  g_free (log_file);

  return result;
}

void
ogmrip_module_load (OGMRipModule *module)
{
  gchar *filename;

  filename = g_find_program_in_path ("vpxenc");
  if (filename)
    have_vpxenc = TRUE;
  else
  {
    filename = g_find_program_in_path ("ivfenc");
    if (!filename)
    {
      g_warning (_("vpxenc is missing"));
      return;
    }
  }
  g_free (filename);
/*
  2,
*/
  ogmrip_register_codec (OGMRIP_TYPE_VP8,
      "vp8", N_("VP8"), OGMRIP_FORMAT_VP8);
}
