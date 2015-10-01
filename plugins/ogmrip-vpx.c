/* OGMRipVpx - A VPX plugin for OGMRip
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ogmrip-base.h>
#include <ogmrip-encode.h>
#include <ogmrip-mplayer.h>
#include <ogmrip-module.h>

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#define OGMRIP_TYPE_VPX          (ogmrip_vpx_get_type ())
#define OGMRIP_VPX(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_VPX, OGMRipVpx))
#define OGMRIP_VPX_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_VPX, OGMRipVpxClass))
#define OGMRIP_IS_VPX(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_VPX))
#define OGMRIP_IS_VPX_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_VPX))

typedef enum
{
  OGMRIP_VP8,
  OGMRIP_VP9
} OGMRipVpxCodec;

typedef struct _OGMRipVpx      OGMRipVpx;
typedef struct _OGMRipVpxClass OGMRipVpxClass;

struct _OGMRipVpx
{
  OGMRipVideoCodec parent_instance;

  OGMRipVpxCodec codec;

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

struct _OGMRipVpxClass
{
  OGMRipVideoCodecClass parent_class;
};

enum
{
  PROP_0,
  PROP_PASSES,
  PROP_CODEC
};

GType ogmrip_vpx_get_type (void);

static OGMJobTask *
ogmrip_vpx_command (OGMRipVpx *vpx, const gchar *fifo, guint pass, guint passes, const gchar *log_file)
{
  OGMJobTask *task;

  GPtrArray *argv;
  const gchar *output;
  guint width, height, num, denom;
  gint bitrate, threads;

  argv = g_ptr_array_new_full (20, g_free);

  g_ptr_array_add (argv, g_strdup ("vpxenc"));
  g_ptr_array_add (argv, g_strdup ("--ivf"));

  switch (vpx->codec)
  {
    case OGMRIP_VP8:
      g_ptr_array_add (argv, g_strdup ("--codec=vp8"));
      break;
    case OGMRIP_VP9:
      g_ptr_array_add (argv, g_strdup ("--codec=vp9"));
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  ogmrip_video_codec_get_scale_size (OGMRIP_VIDEO_CODEC (vpx), &width, &height);
  g_ptr_array_add (argv, g_strdup_printf ("--width=%u", width));
  g_ptr_array_add (argv, g_strdup_printf ("--height=%u", height));

  ogmrip_video_codec_get_framerate (OGMRIP_VIDEO_CODEC (vpx), &num, &denom);
  g_ptr_array_add (argv, g_strdup_printf ("--timebase=%u/%u", denom, num));

  bitrate = ogmrip_video_codec_get_bitrate (OGMRIP_VIDEO_CODEC (vpx));
  if (bitrate > 0)
  {
    g_ptr_array_add (argv, g_strdup_printf ("--target-bitrate=%d", bitrate / 1000));

    if (vpx->min_q >= 0)
      g_ptr_array_add (argv, g_strdup_printf ("--min-q=%u", vpx->min_q));

    if (vpx->max_q >= 0)
      g_ptr_array_add (argv, g_strdup_printf ("--max-q=%u", vpx->max_q));
  }
  else
  {
    gdouble quantizer;

    quantizer = 48. * ogmrip_video_codec_get_quantizer (OGMRIP_VIDEO_CODEC (vpx)) / 31.;
    g_ptr_array_add (argv, g_strdup_printf ("--min-q=%.0lf", quantizer));
    g_ptr_array_add (argv, g_strdup_printf ("--max-q=%.0lf", quantizer + 15.));
  }

  g_ptr_array_add (argv, g_strdup_printf ("--passes=%u", passes));

  if (passes > 1 && log_file)
  {
    g_ptr_array_add (argv, g_strdup_printf ("--pass=%u", pass));
    g_ptr_array_add (argv, g_strdup_printf ("--fpf=%s", log_file));

    if (vpx->minsection_pct >= 0)
      g_ptr_array_add (argv, g_strdup_printf ("--minsection-pct=%u", vpx->minsection_pct));

    if (vpx->maxsection_pct >= 0)
      g_ptr_array_add (argv, g_strdup_printf ("--maxsection-pct=%u", vpx->maxsection_pct));
  }

  g_ptr_array_add (argv, g_strdup ("--end-usage=0"));
  g_ptr_array_add (argv, g_strdup ("--auto-alt-ref=1"));
  g_ptr_array_add (argv, g_strdup ("--lag-in-frames=16"));
  g_ptr_array_add (argv, g_strdup ("--kf-min-dist=0"));
  g_ptr_array_add (argv, g_strdup ("--kf-max-dist=360"));
  g_ptr_array_add (argv, g_strdup ("--static-thresh=0"));

  g_ptr_array_add (argv, g_strdup (vpx->best ? "--best" : "--good"));

  threads = ogmrip_video_codec_get_threads (OGMRIP_VIDEO_CODEC (vpx));
  if (!threads)
    threads = ogmrip_get_nprocessors ();
  if (threads > 0)
    g_ptr_array_add (argv, g_strdup_printf ("--threads=%u", threads));

  if (vpx->token_parts >= 0)
    g_ptr_array_add (argv, g_strdup_printf ("--token-parts=%u", vpx->token_parts));

  if (vpx->drop_frame >= 0)
    g_ptr_array_add (argv, g_strdup_printf ("--drop-frame=%u", vpx->drop_frame));

  if (vpx->profile >= 0)
    g_ptr_array_add (argv, g_strdup_printf ("--profile=%u", vpx->profile));

  if (vpx->cpu_used >= 0)
    g_ptr_array_add (argv, g_strdup_printf ("--cpu-used=%u", vpx->cpu_used));

  output = ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (vpx)));
  g_ptr_array_add (argv, g_strdup ("-o"));
  g_ptr_array_add (argv, g_strdup (output));

  g_ptr_array_add (argv, g_strdup (fifo));

  g_ptr_array_add (argv, NULL);

  task = ogmjob_spawn_newv ((gchar **) argv->pdata);
  g_ptr_array_free (argv, TRUE);

  return task;
}

static OGMJobTask *
ogmrip_vpx_pipeline (OGMRipVpx *vpx, const gchar *fifo, guint pass, guint passes, const gchar *log_file)
{
  OGMJobTask *pipeline, *child;

  pipeline = ogmjob_pipeline_new ();

  child = ogmrip_video_encoder_new (OGMRIP_VIDEO_CODEC (vpx), OGMRIP_ENCODER_YUV, NULL, NULL, fifo);
  ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
  g_object_unref (child);

  child = ogmrip_vpx_command (vpx, fifo, pass + 1, passes, log_file);
  ogmjob_container_add (OGMJOB_CONTAINER (pipeline), child);
  g_object_unref (child);

  return pipeline;
}

G_DEFINE_TYPE (OGMRipVpx, ogmrip_vpx, OGMRIP_TYPE_VIDEO_CODEC)

static void
ogmrip_vpx_notify (GObject *gobject, GParamSpec *pspec)
{
  if (g_str_equal (pspec->name, "quality"))
  {
    OGMRipVpx *vpx = OGMRIP_VPX (gobject);

    switch (ogmrip_video_codec_get_quality (OGMRIP_VIDEO_CODEC (vpx)))
    {
      case OGMRIP_QUALITY_EXTREME:
        vpx->best = FALSE;
        vpx->minsection_pct = 5;
        vpx->maxsection_pct = 800;
        vpx->token_parts = 2;
        vpx->drop_frame = 0;
        vpx->min_q = 0;
        vpx->max_q = 60;
        vpx->profile = -1;
        vpx->cpu_used = 0;
        break;
      case OGMRIP_QUALITY_HIGH:
        vpx->best = FALSE;
        vpx->minsection_pct = 5;
        vpx->maxsection_pct = 800;
        vpx->token_parts = 2;
        vpx->drop_frame = -1;
        vpx->min_q = 0;
        vpx->max_q = 60;
        vpx->profile = -1;
        vpx->cpu_used = 1;
        break;
      default:
        vpx->best = FALSE;
        vpx->minsection_pct = 15;
        vpx->maxsection_pct = 400;
        vpx->token_parts = -1;
        vpx->drop_frame = -1;
        vpx->min_q = 4;
        vpx->max_q = 63;
        vpx->profile = 1;
        vpx->cpu_used = 0;
        break;
    }
  }
}

static void
ogmrip_vpx_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_PASSES:
      g_value_set_uint (value, ogmrip_video_codec_get_passes (OGMRIP_VIDEO_CODEC (gobject)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_vpx_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_PASSES:
      ogmrip_video_codec_set_passes (OGMRIP_VIDEO_CODEC (gobject), g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static gboolean
ogmrip_vpx_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  OGMJobTask *queue, *pipeline;
  gchar *fifo, *log_file;
  gint pass, passes;
  gboolean result;

  fifo = ogmrip_fs_mkftemp ("fifo.XXXXXX", error);
  if (!fifo)
    return FALSE;

  passes = ogmrip_video_codec_get_passes (OGMRIP_VIDEO_CODEC (task));

  log_file = NULL;
  if (passes > 1)
  {
    log_file = ogmrip_fs_mktemp ("log.XXXXXX", error);
    if (!log_file)
    {
      g_unlink (fifo);
      g_free (fifo);

      return FALSE;
    }
  }

  queue = ogmjob_queue_new ();
  ogmjob_container_add (OGMJOB_CONTAINER (task), queue);
  g_object_unref (queue);

  for (pass = 0; pass < passes; pass ++)
  {
    pipeline = ogmrip_vpx_pipeline (OGMRIP_VPX (task), fifo, pass, passes, log_file);
    ogmjob_container_add (OGMJOB_CONTAINER (queue), pipeline);
    g_object_unref (pipeline);
  }

  result = OGMJOB_TASK_CLASS (ogmrip_vpx_parent_class)->run (task, cancellable, error);

  ogmjob_container_remove (OGMJOB_CONTAINER (task), queue);

  g_unlink (fifo);
  g_free (fifo);

  g_unlink (log_file);
  g_free (log_file);

  return result;
}

static void
ogmrip_vpx_class_init (OGMRipVpxClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobTaskClass *task_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->notify = ogmrip_vpx_notify;
  gobject_class->get_property = ogmrip_vpx_get_property;
  gobject_class->set_property = ogmrip_vpx_set_property;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmrip_vpx_run;

  g_object_class_install_property (gobject_class, PROP_PASSES,
      g_param_spec_uint ("passes", "Passes property", "Set the number of passes",
        1, 2, 1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
ogmrip_vpx_init (OGMRipVpx *vpx)
{
  vpx->codec = OGMRIP_VP8;
  vpx->best = FALSE;
  vpx->minsection_pct = -1;
  vpx->maxsection_pct = -1;
  vpx->token_parts = -1;
  vpx->drop_frame = -1;
  vpx->min_q = -1;
  vpx->max_q = -1;
  vpx->profile = -1;
  vpx->cpu_used = -1;
}

#define OGMRIP_TYPE_VP9 (ogmrip_vp9_get_type ())

typedef struct _OGMRipVp9      OGMRipVp9;
typedef struct _OGMRipVp9Class OGMRipVp9Class;

struct _OGMRipVp9
{
  OGMRipVpx parent_instance;
};

struct _OGMRipVp9Class
{
  OGMRipVpxClass parent_class;
};

GType ogmrip_vp9_get_type (void);

G_DEFINE_TYPE (OGMRipVp9, ogmrip_vp9, OGMRIP_TYPE_VPX)

static void
ogmrip_vp9_class_init (OGMRipVp9Class *klass)
{
}

static void
ogmrip_vp9_init (OGMRipVp9 *vp9)
{
  OGMRIP_VPX (vp9)->codec = OGMRIP_VP9;
}

void
ogmrip_module_load (OGMRipModule *module)
{
  gchar *filename;

  filename = g_find_program_in_path ("vpxenc");
  if (!filename)
  {
    g_warning (_("vpxenc is missing"));
    return;
  }
  g_free (filename);

  ogmrip_register_codec (OGMRIP_TYPE_VPX,
      "vp8", _("Vp8"), OGMRIP_FORMAT_VP8, NULL);

  ogmrip_register_codec (OGMRIP_TYPE_VP9,
      "vp9", _("Vp9"), OGMRIP_FORMAT_VP9, NULL);
}

