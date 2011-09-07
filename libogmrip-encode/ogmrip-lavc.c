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

/**
 * SECTION:ogmrip-lavc
 * @title: OGMRipLavc
 * @short_description: Base class for lavc video codecs
 * @include: ogmrip-lavc.h
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-fs.h"
#include "ogmrip-lavc.h"
#include "ogmrip-version.h"
#include "ogmrip-mplayer.h"
#include "ogmrip-plugin.h"
#include "ogmrip-configurable.h"

#include <ogmrip-job.h>

#include <stdio.h>
#include <glib/gstdio.h>

#define OGMRIP_LAVC_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_LAVC, OGMRipLavcPriv))

enum
{
  PROP_0,
  PROP_BUF_SIZE,
  PROP_CMP,
  PROP_DC,
  PROP_DIA,
  PROP_GRAYSCALE,
  PROP_HEADER,
  PROP_KEYINT,
  PROP_LAST_PRED,
  PROP_MAX_BFRAMES,
  PROP_MAX_RATE,
  PROP_MBD,
  PROP_MIN_RATE,
  PROP_MV0,
  PROP_PRECMP,
  PROP_PREDIA,
  PROP_PREME,
  PROP_QNS,
  PROP_QPEL,
  PROP_STRICT,
  PROP_SUBCMP,
  PROP_TRELLIS,
  PROP_V4MV,
  PROP_VB_STRATEGY,
  PROP_VQCOMP
};

struct _OGMRipLavcPriv
{
  guint header;
  guint cmp, precmp, subcmp;
  gint dia, predia;

  guint mbd;
  guint qns;
  guint vb_strategy;
  guint last_pred;
  guint preme;
  gdouble vqcomp;

  gboolean mv0;
  gboolean grayscale;
  gboolean qpel;
  gboolean trellis;
  gboolean v4mv;

  guint dc;
  guint keyint;
  guint max_bframes;
  guint buf_size, min_rate, max_rate;
  guint strict;
};

static void ogmrip_lavc_get_property (GObject      *gobject,
                                      guint        property_id,
                                      GValue       *value,
                                      GParamSpec   *pspec);
static void ogmrip_lavc_set_property (GObject      *gobject,
                                      guint        property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec);
static void ogmrip_lavc_notify       (GObject      *object,
                                      GParamSpec   *pspec);
static gint ogmrip_lavc_run          (OGMJobSpawn  *spawn);

static gdouble
ogmrip_lavc_get_quantizer (OGMRipVideoCodec *video)
{
  gdouble quantizer;

  quantizer = ogmrip_video_codec_get_quantizer (video);

  return CLAMP (quantizer, 2, 31);
}

static const gchar *
ogmrip_lavc_get_codec (OGMRipLavc *lavc)
{
  OGMRipLavcClass *klass;

  klass = OGMRIP_LAVC_GET_CLASS (lavc);

  if (klass->get_codec)
    return (* klass->get_codec) ();

  return NULL;
}

static gchar **
ogmrip_lavc_command (OGMRipVideoCodec *video, guint pass, guint passes, const gchar *log_file)
{
  static const gint strict[] = { 0, 1, -1, -2 };

  OGMRipLavc *lavc;
  OGMRipStream *input;
  GPtrArray *argv;
  GString *options;

  const gchar *output, *codec;
  gint bitrate, vid, threads;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_CODEC (video), NULL);
  g_return_val_if_fail (pass == 1 || log_file != NULL, NULL);

  output = ogmrip_file_get_path (ogmrip_codec_get_output (OGMRIP_CODEC (video)));

  lavc = OGMRIP_LAVC (video);

  argv = ogmrip_mencoder_video_command (video, pass == passes ? output : "/dev/null", pass);

  g_ptr_array_add (argv, g_strdup ("-ovc"));
  g_ptr_array_add (argv, g_strdup ("lavc"));

  options = g_string_new (NULL);

  codec = ogmrip_lavc_get_codec (OGMRIP_LAVC (video));
  if (!codec)
    g_string_assign (options, "vcodec=mpeg4");
  else
    g_string_printf (options, "vcodec=%s", codec);

  g_string_append_printf (options, ":autoaspect:mbd=%u:qns=%u:vb_strategy=%u:last_pred=%u:preme=%u",
      lavc->priv->mbd, lavc->priv->qns, lavc->priv->vb_strategy, lavc->priv->last_pred, lavc->priv->preme);

  if (MPLAYER_CHECK_VERSION (1,0,0,6))
    if (pass != passes && ogmrip_video_codec_get_turbo (video))
      g_string_append (options, ":turbo");

  if (lavc->priv->mv0)
    g_string_append (options, ":mv0");

  if (lavc->priv->qpel)
    g_string_append (options, ":qpel");

  if (ogmrip_plugin_get_video_codec_format (G_TYPE_FROM_INSTANCE (video)) == OGMRIP_FORMAT_MPEG4 && lavc->priv->v4mv)
    g_string_append (options, ":v4mv");

  if (lavc->priv->trellis)
    g_string_append (options, ":trell:cbp");
  if (lavc->priv->grayscale)
    g_string_append (options, ":gray");

  g_string_append_printf (options, ":keyint=%u:dc=%u:vstrict=%d",
      lavc->priv->keyint, lavc->priv->dc, strict[lavc->priv->strict]);

  if (lavc->priv->buf_size > 0)
    g_string_append_printf (options, ":vrc_buf_size=%u", lavc->priv->buf_size);

  if (lavc->priv->min_rate > 0)
    g_string_append_printf (options, ":vrc_minrate=%u", lavc->priv->min_rate);

  if (lavc->priv->max_rate > 0)
    g_string_append_printf (options, ":vrc_maxrate=%u", lavc->priv->max_rate);

  if (lavc->priv->cmp != 0 || lavc->priv->precmp != 0 || lavc->priv->subcmp != 0)
    g_string_append_printf (options, ":precmp=%u:subcmp=%u:cmp=%u", lavc->priv->precmp, lavc->priv->subcmp, lavc->priv->cmp);

  if (lavc->priv->dia != 1 || lavc->priv->predia != 1)
    g_string_append_printf (options, ":dia=%d:predia=%d", lavc->priv->dia, lavc->priv->predia);

  if (lavc->priv->header != 0)
    g_string_append_printf (options, ":vglobal=%d", lavc->priv->header);

  g_string_append_printf (options, ":vmax_b_frames=%d", lavc->priv->max_bframes);

  bitrate = ogmrip_video_codec_get_bitrate (video);
  if (bitrate > 0)
  {
    if (bitrate > 16000)
      g_string_append_printf (options, ":vbitrate=%u", MIN (bitrate, 24000000));
    else
      g_string_append_printf (options, ":vbitrate=%u", MAX (bitrate, 4000) / 1000);
  }
  else
    g_string_append_printf (options, ":vqscale=%.0lf", ogmrip_lavc_get_quantizer (video));

  if (passes > 1 && log_file)
  {
    if (pass == 1)
      g_string_append (options, ":vpass=1");
    else
    {
      if (passes == 2)
        g_string_append (options, ":vpass=2");
      else
        g_string_append (options, ":vpass=3");
    }

    g_ptr_array_add (argv, g_strdup ("-passlogfile"));
    g_ptr_array_add (argv, g_strdup (log_file));
  }

  threads = ogmrip_video_codec_get_threads (video);
  if (threads > 0)
    g_string_append_printf (options, ":threads=%u", CLAMP (threads, 1, 8));

  g_ptr_array_add (argv, g_strdup ("-lavcopts"));
  g_ptr_array_add (argv, g_string_free (options, FALSE));

  input = ogmrip_codec_get_input (OGMRIP_CODEC (video));
  vid = ogmrip_title_get_nr (ogmrip_stream_get_title (input));

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

static void
ogmrip_lavc_configure (OGMRipConfigurable *configurable, OGMRipProfile *profile)
{
  GSettings *settings;

  settings = ogmrip_profile_get_child (profile, "lavc");
  if (settings)
  {
    g_settings_bind (settings, "header", configurable, OGMRIP_LAVC_PROP_HEADER,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "cmp", configurable, OGMRIP_LAVC_PROP_CMP,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "precmp", configurable, OGMRIP_LAVC_PROP_PRECMP,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "subcmp", configurable, OGMRIP_LAVC_PROP_SUBCMP,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "dia", configurable, OGMRIP_LAVC_PROP_DIA,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "predia", configurable, OGMRIP_LAVC_PROP_PREDIA,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    /*
     * TODO min = 1
     */
    g_settings_bind (settings, "keyint", configurable, OGMRIP_LAVC_PROP_KEYINT,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "buf-size", configurable, OGMRIP_LAVC_PROP_BUF_SIZE,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    /*
     * TODO min = 1
     */
    g_settings_bind (settings, "dc", configurable, OGMRIP_LAVC_PROP_DC,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "mbd", configurable, OGMRIP_LAVC_PROP_MBD,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "qns", configurable, OGMRIP_LAVC_PROP_QNS,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "vb-strategy", configurable, OGMRIP_LAVC_PROP_VB_STRATEGY,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "last-pred", configurable, OGMRIP_LAVC_PROP_LAST_PRED,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "preme", configurable, OGMRIP_LAVC_PROP_PREME,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "vqcomp", configurable, OGMRIP_LAVC_PROP_VQCOMP,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "mv0", configurable, OGMRIP_LAVC_PROP_MV0,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "grayscale", configurable, OGMRIP_LAVC_PROP_GRAYSCALE,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "qpel", configurable, OGMRIP_LAVC_PROP_QPEL,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "trellis", configurable, OGMRIP_LAVC_PROP_TRELLIS,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "v4mv", configurable, OGMRIP_LAVC_PROP_V4MV,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "max-bframes", configurable, OGMRIP_LAVC_PROP_MAX_BFRAMES,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "strict", configurable, OGMRIP_LAVC_PROP_STRICT,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "max-rate", configurable, OGMRIP_LAVC_PROP_MAX_RATE,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (settings, "min-rate", configurable, OGMRIP_LAVC_PROP_MIN_RATE,
        G_SETTINGS_BIND_GET | G_SETTINGS_BIND_GET_NO_CHANGES);

    g_object_unref (settings);
  }
}

static void
ogmrip_configurable_iface_init (OGMRipConfigurableInterface *iface)
{
  iface->configure = ogmrip_lavc_configure;
}

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (OGMRipLavc, ogmrip_lavc, OGMRIP_TYPE_VIDEO_CODEC,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_CONFIGURABLE, ogmrip_configurable_iface_init))

static void
ogmrip_lavc_class_init (OGMRipLavcClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_lavc_get_property;
  gobject_class->set_property = ogmrip_lavc_set_property;
  gobject_class->notify = ogmrip_lavc_notify;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmrip_lavc_run;

  g_object_class_install_property (gobject_class, PROP_HEADER,
      g_param_spec_uint (OGMRIP_LAVC_PROP_HEADER, "Header property", "Set header",
        0, 3, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CMP,
      g_param_spec_uint (OGMRIP_LAVC_PROP_CMP, "Cmp property", "Set cmp",
        0, 2000, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PRECMP,
      g_param_spec_uint (OGMRIP_LAVC_PROP_PRECMP, "Precmp property", "Set precmp",
        0, 2000, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SUBCMP,
      g_param_spec_uint (OGMRIP_LAVC_PROP_SUBCMP, "Subcmp property", "Set subcmp",
        0, 2000, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DIA,
      g_param_spec_int (OGMRIP_LAVC_PROP_DIA, "Dia property", "Set dia",
        -99, 6, 1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PREDIA,
      g_param_spec_int (OGMRIP_LAVC_PROP_PREDIA, "Predia property", "Set predia",
        -99, 6, 1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MBD,
      g_param_spec_uint (OGMRIP_LAVC_PROP_MBD, "Mbd property", "Set mbd",
        0, 2, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QNS,
      g_param_spec_uint (OGMRIP_LAVC_PROP_QNS, "Qns property", "Set qns",
        0, 3, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VB_STRATEGY,
      g_param_spec_uint (OGMRIP_LAVC_PROP_VB_STRATEGY, "VB strategy property", "Set bv strategy",
        0, 2, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LAST_PRED,
      g_param_spec_uint (OGMRIP_LAVC_PROP_LAST_PRED, "Last pref property", "Set last pred",
        0, 99, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PREME,
      g_param_spec_uint (OGMRIP_LAVC_PROP_PREME, "Preme property", "Set preme",
        0, 2, 1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VQCOMP,
      g_param_spec_double (OGMRIP_LAVC_PROP_VQCOMP, "Vqcomp property", "Set vqcomp",
        0.0, 1.0, 0.5, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MV0,
      g_param_spec_boolean (OGMRIP_LAVC_PROP_MV0, "Mv0 property", "Set mv0",
        FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DC,
      g_param_spec_uint (OGMRIP_LAVC_PROP_DC, "DC property", "Set dc",
        1, G_MAXUINT, 8, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_KEYINT,
      g_param_spec_uint (OGMRIP_LAVC_PROP_KEYINT, "Keyint property", "Set keyint",
        1, G_MAXUINT, 250, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BUF_SIZE,
      g_param_spec_uint (OGMRIP_LAVC_PROP_BUF_SIZE, "Buffer size property", "Set buffer size",
        0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_BFRAMES,
      g_param_spec_uint (OGMRIP_LAVC_PROP_MAX_BFRAMES, "Max B-frames property", "Set max B-frames",
        0, 4, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MIN_RATE,
      g_param_spec_uint (OGMRIP_LAVC_PROP_MIN_RATE, "Min rate property", "Set min rate",
        0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_RATE,
      g_param_spec_uint (OGMRIP_LAVC_PROP_MAX_RATE, "Max rate property", "Set max rate",
        0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_STRICT,
      g_param_spec_uint (OGMRIP_LAVC_PROP_STRICT, "Strict property", "Set strict",
        0, 3, 2, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_GRAYSCALE,
      g_param_spec_boolean (OGMRIP_LAVC_PROP_GRAYSCALE, "Grayscale property", "Set grayscale",
        FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QPEL,
      g_param_spec_boolean (OGMRIP_LAVC_PROP_QPEL, "Qpel property", "Set qpel",
        FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TRELLIS,
      g_param_spec_boolean (OGMRIP_LAVC_PROP_TRELLIS, "Trellis property", "Set trellis",
        TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_V4MV,
      g_param_spec_boolean (OGMRIP_LAVC_PROP_V4MV, "4MV property", "Set 4mv",
        TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipLavcPriv));
}

static void
ogmrip_lavc_init (OGMRipLavc *lavc)
{
  lavc->priv = OGMRIP_LAVC_GET_PRIVATE (lavc);
  lavc->priv->cmp = 2;
  lavc->priv->precmp = 2;
  lavc->priv->subcmp = 2;
  lavc->priv->dia = 2;
  lavc->priv->predia = 2;
  lavc->priv->keyint = 250;
  lavc->priv->strict = 2;
  lavc->priv->dc = 8;
  lavc->priv->trellis = TRUE;
  lavc->priv->v4mv = TRUE;
}

static void
ogmrip_lavc_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id) 
  {
    case PROP_HEADER:
      g_value_set_uint (value, ogmrip_lavc_get_header (OGMRIP_LAVC (gobject)));
      break;
    case PROP_CMP:
      g_value_set_uint (value, OGMRIP_LAVC (gobject)->priv->cmp);
      break;
    case PROP_PRECMP:
      g_value_set_uint (value, OGMRIP_LAVC (gobject)->priv->precmp);
      break;
    case PROP_SUBCMP:
      g_value_set_uint (value, OGMRIP_LAVC (gobject)->priv->subcmp);
      break;
    case PROP_DIA:
      g_value_set_int (value, OGMRIP_LAVC (gobject)->priv->dia);
      break;
    case PROP_PREDIA:
      g_value_set_int (value, OGMRIP_LAVC (gobject)->priv->predia);
      break;
    case PROP_MBD:
      g_value_set_uint (value, ogmrip_lavc_get_mbd (OGMRIP_LAVC (gobject)));
      break;
    case PROP_QNS:
      g_value_set_uint (value, ogmrip_lavc_get_qns (OGMRIP_LAVC (gobject)));
      break;
    case PROP_VB_STRATEGY:
      g_value_set_uint (value, ogmrip_lavc_get_vb_strategy (OGMRIP_LAVC (gobject)));
      break;
    case PROP_LAST_PRED:
      g_value_set_uint (value, ogmrip_lavc_get_last_pred (OGMRIP_LAVC (gobject)));
      break;
    case PROP_PREME:
      g_value_set_uint (value, ogmrip_lavc_get_preme (OGMRIP_LAVC (gobject)));
      break;
    case PROP_VQCOMP:
      g_value_set_double (value, ogmrip_lavc_get_vqcomp (OGMRIP_LAVC (gobject)));
      break;
    case PROP_MV0:
      g_value_set_boolean (value, ogmrip_lavc_get_mv0 (OGMRIP_LAVC (gobject)));
      break;
    case PROP_DC:
      g_value_set_uint (value, ogmrip_lavc_get_dc (OGMRIP_LAVC (gobject)));
      break;
    case PROP_KEYINT:
      g_value_set_uint (value, ogmrip_lavc_get_keyint (OGMRIP_LAVC (gobject)));
      break;
    case PROP_BUF_SIZE:
      g_value_set_uint (value, ogmrip_lavc_get_buf_size (OGMRIP_LAVC (gobject)));
      break;
    case PROP_MAX_BFRAMES:
      g_value_set_uint (value, ogmrip_lavc_get_max_bframes (OGMRIP_LAVC (gobject)));
      break;
    case PROP_MIN_RATE:
      g_value_set_uint (value, ogmrip_lavc_get_min_rate (OGMRIP_LAVC (gobject)));
      break;
    case PROP_MAX_RATE:
      g_value_set_uint (value, ogmrip_lavc_get_max_rate (OGMRIP_LAVC (gobject)));
      break;
    case PROP_STRICT:
      g_value_set_uint (value, ogmrip_lavc_get_strict (OGMRIP_LAVC (gobject)));
      break;
    case PROP_GRAYSCALE:
      g_value_set_boolean (value, ogmrip_lavc_get_grayscale (OGMRIP_LAVC (gobject)));
      break;
    case PROP_QPEL:
      g_value_set_boolean (value, ogmrip_lavc_get_qpel (OGMRIP_LAVC (gobject)));
      break;
    case PROP_TRELLIS:
      g_value_set_boolean (value, ogmrip_lavc_get_trellis (OGMRIP_LAVC (gobject)));
      break;
    case PROP_V4MV:
      g_value_set_boolean (value, ogmrip_lavc_get_v4mv (OGMRIP_LAVC (gobject)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_lavc_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id) 
  {
    case PROP_HEADER:
      ogmrip_lavc_set_header (OGMRIP_LAVC (gobject), g_value_get_uint (value));
      break;
    case PROP_CMP:
      OGMRIP_LAVC (gobject)->priv->cmp = g_value_get_uint (value);
      break;
    case PROP_PRECMP:
      OGMRIP_LAVC (gobject)->priv->precmp = g_value_get_uint (value);
      break;
    case PROP_SUBCMP:
      OGMRIP_LAVC (gobject)->priv->subcmp = g_value_get_uint (value);
      break;
    case PROP_DIA:
      OGMRIP_LAVC (gobject)->priv->dia = g_value_get_int (value);
      break;
    case PROP_PREDIA:
      OGMRIP_LAVC (gobject)->priv->predia = g_value_get_int (value);
      break;
    case PROP_MBD:
      ogmrip_lavc_set_mbd (OGMRIP_LAVC (gobject), g_value_get_uint (value));
      break;
    case PROP_QNS:
      ogmrip_lavc_set_qns (OGMRIP_LAVC (gobject), g_value_get_uint (value));
      break;
    case PROP_VB_STRATEGY:
      ogmrip_lavc_set_vb_strategy (OGMRIP_LAVC (gobject), g_value_get_uint (value));
      break;
    case PROP_LAST_PRED:
      ogmrip_lavc_set_last_pred (OGMRIP_LAVC (gobject), g_value_get_uint (value));
      break;
    case PROP_PREME:
      ogmrip_lavc_set_preme (OGMRIP_LAVC (gobject), g_value_get_uint (value));
      break;
    case PROP_VQCOMP:
      ogmrip_lavc_set_vqcomp (OGMRIP_LAVC (gobject), g_value_get_double (value));
      break;
    case PROP_MV0:
      ogmrip_lavc_set_mv0 (OGMRIP_LAVC (gobject), g_value_get_boolean (value));
      break;
    case PROP_DC:
      ogmrip_lavc_set_dc (OGMRIP_LAVC (gobject), g_value_get_uint (value));
      break;
    case PROP_KEYINT:
      ogmrip_lavc_set_keyint (OGMRIP_LAVC (gobject), g_value_get_uint (value));
      break;
    case PROP_BUF_SIZE:
      ogmrip_lavc_set_buf_size (OGMRIP_LAVC (gobject), g_value_get_uint (value));
      break;
    case PROP_MAX_BFRAMES:
      ogmrip_lavc_set_max_bframes (OGMRIP_LAVC (gobject), g_value_get_uint (value));
      break;
    case PROP_MIN_RATE:
      ogmrip_lavc_set_min_rate (OGMRIP_LAVC (gobject), g_value_get_uint (value));
      break;
    case PROP_MAX_RATE:
      ogmrip_lavc_set_max_rate (OGMRIP_LAVC (gobject), g_value_get_uint (value));
      break;
    case PROP_STRICT:
      ogmrip_lavc_set_strict (OGMRIP_LAVC (gobject), g_value_get_uint (value));
      break;
    case PROP_GRAYSCALE:
      ogmrip_lavc_set_grayscale (OGMRIP_LAVC (gobject), g_value_get_boolean (value));
      break;
    case PROP_QPEL:
      ogmrip_lavc_set_qpel (OGMRIP_LAVC (gobject), g_value_get_boolean (value));
      break;
    case PROP_TRELLIS:
      ogmrip_lavc_set_trellis (OGMRIP_LAVC (gobject), g_value_get_boolean (value));
      break;
    case PROP_V4MV:
      ogmrip_lavc_set_v4mv (OGMRIP_LAVC (gobject), g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_lavc_set_quality (OGMRipLavc *lavc, OGMRipQualityType quality)
{
  ogmrip_lavc_init (lavc);

  switch (quality)
  {
    case OGMRIP_QUALITY_EXTREME:
      lavc->priv->mbd = 2;
      lavc->priv->vb_strategy = 1;
      lavc->priv->last_pred = 3;
      lavc->priv->max_bframes = 2;
      lavc->priv->preme = 2;
      lavc->priv->qns = 2;
      lavc->priv->vqcomp = 0.5;
      lavc->priv->mv0 = TRUE;
      break;
    case OGMRIP_QUALITY_HIGH:
      lavc->priv->mbd = 2;
      lavc->priv->vb_strategy = 1;
      lavc->priv->max_bframes = 2;
      lavc->priv->last_pred = 2;
      lavc->priv->preme = 1;
      lavc->priv->qns = 0;
      lavc->priv->vqcomp = 0.6;
      lavc->priv->mv0 = FALSE;
      break;
    case OGMRIP_QUALITY_NORMAL:
      lavc->priv->mbd = 2;
      lavc->priv->vb_strategy = 0;
      lavc->priv->last_pred = 0;
      lavc->priv->max_bframes = 0;
      lavc->priv->preme = 1;
      lavc->priv->qns = 0;
      lavc->priv->vqcomp = 0.5;
      lavc->priv->mv0 = FALSE;
      break;
    case OGMRIP_QUALITY_USER:
      break;
  }
}

static void
ogmrip_lavc_notify (GObject *object, GParamSpec *pspec)
{
  if (g_str_equal (pspec->name, "quality"))
  {
    OGMRipQualityType quality;

    g_object_get (object, "quality", &quality, NULL);
    ogmrip_lavc_set_quality (OGMRIP_LAVC (object), quality);
  }
}

static gint
ogmrip_lavc_run (OGMJobSpawn *spawn)
{
  OGMJobSpawn *queue, *child;
  gchar **argv, *log_file;
  gint pass, passes, result;

  queue = ogmjob_queue_new ();
  ogmjob_container_add (OGMJOB_CONTAINER (spawn), queue);
  g_object_unref (queue);

  passes = ogmrip_video_codec_get_passes (OGMRIP_VIDEO_CODEC (spawn));

  log_file = NULL;
  if (passes > 1)
    log_file = ogmrip_fs_mktemp ("log.XXXXXX", NULL);

  for (pass = 0; pass < passes; pass ++)
  {
    argv = ogmrip_lavc_command (OGMRIP_VIDEO_CODEC (spawn), pass + 1, passes, log_file);
    if (!argv)
      return OGMJOB_RESULT_ERROR;

    child = ogmjob_exec_newv (argv);
    ogmjob_exec_add_watch_full (OGMJOB_EXEC (child), (OGMJobWatch) ogmrip_mencoder_codec_watch, spawn, TRUE, FALSE, FALSE);
    ogmjob_container_add (OGMJOB_CONTAINER (queue), child);
    g_object_unref (child);
  }

  result = OGMJOB_SPAWN_CLASS (ogmrip_lavc_parent_class)->run (spawn);

  ogmjob_container_remove (OGMJOB_CONTAINER (spawn), queue);

  g_unlink (log_file);
  g_free (log_file);

  return result;
}

/**
 * ogmrip_init_lavc_plugin:
 *
 * Initialises the LAVC plugin. This function should be called
 * when initialising a plugin of the LAVC family.
 */
void
ogmrip_init_lavc_plugin (void)
{
}

/**
 * ogmrip_lavc_new:
 * @title: An #OGMRipTitle
 * @output: The output file
 *
 * Creates a new #OGMRipLavc
 *
 * Returns: The new #OGMRipLavc
 */
OGMJobSpawn *
ogmrip_lavc_new (OGMRipTitle *title, const gchar *output)
{
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (output && *output, NULL);

  return g_object_new (OGMRIP_TYPE_LAVC, "input", title, "output", output, NULL);
}

/**
 * ogmrip_lavc_set_cmp:
 * @lavc: An #OGMRipLavc
 * @cmp: The comparison function for full pel motion estimation
 * @precmp: The comparison function for motion estimation pre pass
 * @subcmp: The comparison function for sub pel motion estimation
 *
 * Sets the comparison function for full pel, pre pass and sub pel motion estimation
 */
void
ogmrip_lavc_set_cmp (OGMRipLavc *lavc, guint cmp, guint precmp, guint subcmp)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->cmp = MIN (cmp, 2000);
  lavc->priv->precmp = MIN (precmp, 2000);
  lavc->priv->subcmp = MIN (subcmp, 2000);
}

/**
 * ogmrip_lavc_get_cmp:
 * @lavc: An #OGMRipLavc
 * @cmp: A pointer to store the comparison function for full pel motion estimation
 * @precmp: A pointer to store the comparison function for motion estimation pre pass
 * @subcmp: A pointer to store the comparison function for sub pel motion estimation
 *
 * Gets the comparison function for full pel, pre pass and sub pel motion estimation
 */
void
ogmrip_lavc_get_cmp (OGMRipLavc *lavc, guint *cmp, guint *precmp, guint *subcmp)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));
  g_return_if_fail (precmp != NULL);
  g_return_if_fail (subcmp != NULL);
  g_return_if_fail (cmp != NULL);

  *cmp = lavc->priv->cmp;
  *precmp = lavc->priv->precmp;
  *subcmp = lavc->priv->subcmp;
}

/**
 * ogmrip_lavc_set_dia:
 * @lavc: An #OGMRipLavc
 * @dia: The diamond type and size for full pel motion estimation
 * @predia: The diamond type and size for motion estimation pre-pass
 *
 * Sets the diamond type and size for full pel and pre pass motion estimation
 */
void
ogmrip_lavc_set_dia (OGMRipLavc *lavc, gint dia, gint predia)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->dia = CLAMP (dia, -99, 6);
  lavc->priv->predia = CLAMP (predia, -99, 6);
}

/**
 * ogmrip_lavc_get_dia:
 * @lavc: An #OGMRipLavc
 * @dia: A pointer to store the diamond type and size for full pel motion estimation
 * @predia: A pointer to store the diamond type and size for motion estimation pre-pass
 *
 * Gets the diamond type and size for full pel and pre pass motion estimation
 */
void
ogmrip_lavc_get_dia (OGMRipLavc *lavc, gint *dia, gint *predia)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));
  g_return_if_fail (predia != NULL);
  g_return_if_fail (dia != NULL);

  *dia = lavc->priv->dia;
  *predia = lavc->priv->predia;
}

/**
 * ogmrip_lavc_set_header:
 * @lavc: An #OGMRipLavc
 * @header: The #OGMRipLavcHeaderType
 *
 * Sets the global video header type.
 */
void
ogmrip_lavc_set_header (OGMRipLavc *lavc, OGMRipLavcHeaderType header)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->header = CLAMP (header, OGMRIP_LAVC_HEADER_AUTO, OGMRIP_LAVC_HEADER_COMBINE);
}

/**
 * ogmrip_lavc_get_header:
 * @lavc: An #OGMRipLavc
 *
 * Gets the global video header type.
 *
 * Returns: The current #OGMRipLavcHeaderType, or -1
 */
gint
ogmrip_lavc_get_header (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), -1);

  return lavc->priv->header;
}

/**
 * ogmrip_lavc_set_keyint:
 * @lavc: An #OGMRipLavc
 * @keyint: An intervale
 *
 * Sets the maximum interval between key frames
 */
void
ogmrip_lavc_set_keyint (OGMRipLavc *lavc, guint keyint)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->keyint = CLAMP (keyint, 0, 300);
}

/**
 * ogmrip_lavc_get_keyint:
 * @lavc: An #OGMRipLavc
 *
 * Gets the maximum interval between key frames
 *
 * Returns: The interval, or -1
 */
gint
ogmrip_lavc_get_keyint (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), -1);

  return lavc->priv->keyint;
}

/**
 * ogmrip_lavc_set_buf_size:
 * @lavc: An #OGMRipLavc
 * @buf_size: A buffer size
 *
 * Sets the buffer size in kb
 */
void
ogmrip_lavc_set_buf_size (OGMRipLavc *lavc, guint buf_size)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->buf_size = buf_size;
}

/**
 * ogmrip_lavc_get_buf_size:
 * @lavc: An #OGMRipLavc
 *
 * Gets the buffer size in kb
 *
 * Returns: The buffer size, or -1
 */
gint
ogmrip_lavc_get_buf_size (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), -1);

  return lavc->priv->buf_size;
}

/**
 * ogmrip_lavc_set_max_bframes:
 * @lavc: An #OGMRipLavc
 * @max_bframes: The max number of B-frames
 *
 * Sets the maximum number of B-frames
 */
void
ogmrip_lavc_set_max_bframes (OGMRipLavc *lavc, guint max_bframes)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->max_bframes = max_bframes;
}

/**
 * ogmrip_lavc_get_max_bframes:
 * @lavc: An #OGMRipLavc
 *
 * Gets the maximum number of B-frames
 *
 * Returns: The maximum number of B-frames, or -1
 */
gint
ogmrip_lavc_get_max_bframes (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), -1);

  return lavc->priv->max_bframes;
}

/**
 * ogmrip_lavc_set_min_rate:
 * @lavc: An #OGMRipLavc
 * @min_rate: A bitrate
 *
 * Sets the minimum bitrate in kbps
 */
void
ogmrip_lavc_set_min_rate (OGMRipLavc *lavc, guint min_rate)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->min_rate = min_rate;
}

/**
 * ogmrip_lavc_get_min_rate:
 * @lavc: An #OGMRipLavc
 *
 * Gets the minimum bitrate in kbps
 *
 * Returns: The bitrate, or -1
 */
gint
ogmrip_lavc_get_min_rate (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), -1);

  return lavc->priv->min_rate;
}

/**
 * ogmrip_lavc_set_max_rate:
 * @lavc: An #OGMRipLavc
 * @max_rate: A bitrate
 *
 * Sets the maximum bitrate in kbps
 */
void
ogmrip_lavc_set_max_rate (OGMRipLavc *lavc, guint max_rate)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->max_rate = max_rate;
}

/**
 * ogmrip_lavc_get_max_rate:
 * @lavc: An #OGMRipLavc
 *
 * Gets the maximum bitrate in kbps
 *
 * Returns: The bitrate, or -1
 */
gint
ogmrip_lavc_get_max_rate (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), -1);

  return lavc->priv->max_rate;
}

/**
 * ogmrip_lavc_set_strict:
 * @lavc: An #OGMRipLavc
 * @strict: The strictness
 *
 * Sets the strict standard compliancy
 */
void
ogmrip_lavc_set_strict (OGMRipLavc *lavc, guint strict)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->strict = CLAMP (strict, 0, 3);
}

/**
 * ogmrip_lavc_get_strict:
 * @lavc: An #OGMRipLavc
 *
 * Gets the strict standard compliancy
 *
 * Returns: The strictness, or -1
 */
gint
ogmrip_lavc_get_strict (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), -1);

  return lavc->priv->strict;
}

/**
 * ogmrip_lavc_set_dc:
 * @lavc: An #OGMRipLavc
 * @dc: A precision
 *
 * Sets the intra DC precision in bits
 */
void
ogmrip_lavc_set_dc (OGMRipLavc *lavc, guint dc)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->dc = dc;
}

/**
 * ogmrip_lavc_get_dc:
 * @lavc: An #OGMRipLavc
 *
 * Gets the intra DC precision in bits
 *
 * Returns: The precision, or -1
 */
gint
ogmrip_lavc_get_dc (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), -1);

  return lavc->priv->dc;
}

/**
 * ogmrip_lavc_set_mbd:
 * @lavc: An #OGMRipLavc
 * @mbd: The mbd
 *
 * Sets the macroblock decision algorithm
 */
void
ogmrip_lavc_set_mbd (OGMRipLavc *lavc, guint mbd)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->mbd = mbd;
}

/**
 * ogmrip_lavc_get_mbd:
 * @lavc: An #OGMRipLavc
 *
 * Gets the macroblock decision algorithm
 *
 * Returns: The algorithm, or -1
 */
gint
ogmrip_lavc_get_mbd (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), -1);

  return lavc->priv->mbd;
}

/**
 * ogmrip_lavc_set_qns:
 * @lavc: An #OGMRipLavc
 * @qns: The shaping
 *
 * Sets the quantizer noise shaping
 */
void
ogmrip_lavc_set_qns (OGMRipLavc *lavc, guint qns)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->qns = qns;
}

/**
 * ogmrip_lavc_get_qns:
 * @lavc: An #OGMRipLavc
 *
 * Gets the quantizer noise shaping
 *
 * Returns: The shaping, or -1
 */
gint
ogmrip_lavc_get_qns (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), -1);

  return lavc->priv->qns;
}

/**
 * ogmrip_lavc_set_vb_strategy:
 * @lavc: An #OGMRipLavc
 * @vb_strategy: A strategy
 *
 * Sets the strategy to choose between I/P/B-frames
 */
void
ogmrip_lavc_set_vb_strategy (OGMRipLavc *lavc, guint vb_strategy)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->vb_strategy = vb_strategy;
}

/**
 * ogmrip_lavc_get_vb_strategy:
 * @lavc: An #OGMRipLavc
 *
 * Gets the strategy to choose between I/P/B-frames
 *
 * Returns: The strategy, or -1
 */
gint
ogmrip_lavc_get_vb_strategy (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), -1);

  return lavc->priv->vb_strategy;
}

/**
 * ogmrip_lavc_set_last_pred:
 * @lavc: An #OGMRipLavc
 * @last_pred: The last_pred
 *
 * Sets the amount of motion predictors from the previous frame
 */
void
ogmrip_lavc_set_last_pred (OGMRipLavc *lavc, guint last_pred)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->last_pred = last_pred;
}

/**
 * ogmrip_lavc_get_last_pred:
 * @lavc: An #OGMRipLavc
 *
 * Gets the amount of motion predictors from the previous frame
 *
 * Returns: The amount, or -1
 */
gint
ogmrip_lavc_get_last_pred (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), -1);

  return lavc->priv->last_pred;
}

/**
 * ogmrip_lavc_set_preme:
 * @lavc: An #OGMRipLavc
 * @preme: The estimation
 *
 * Sets the motion estimation pre-pass
 */
void
ogmrip_lavc_set_preme (OGMRipLavc *lavc, guint preme)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->preme = preme;
}

/**
 * ogmrip_lavc_get_preme:
 * @lavc: An #OGMRipLavc
 *
 * Gets the motion estimation pre-pass
 *
 * Returns: The estimation, or -1
 */
gint
ogmrip_lavc_get_preme (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), -1);

  return lavc->priv->preme;
}

/**
 * ogmrip_lavc_set_vqcomp:
 * @lavc: An #OGMRipLavc
 * @vqcomp: The vqcomp
 *
 * Sets the quantizer compression
 */
void
ogmrip_lavc_set_vqcomp (OGMRipLavc *lavc, gdouble vqcomp)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->vqcomp = vqcomp;
}

/**
 * ogmrip_lavc_get_vqcomp:
 * @lavc: An #OGMRipLavc
 *
 * Gets the quantizer compression
 *
 * Returns: The compression, or -1
 */
gdouble
ogmrip_lavc_get_vqcomp (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), -1.0);

  return lavc->priv->vqcomp;
}

/**
 * ogmrip_lavc_set_mv0:
 * @lavc: An #OGMRipLavc
 * @mv0: TRUE to enable mv0
 *
 * Try to encode each MB with MV=<0,0>
 */
void
ogmrip_lavc_set_mv0 (OGMRipLavc *lavc, gboolean mv0)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->mv0 = mv0;
}

/**
 * ogmrip_lavc_get_mv0:
 * @lavc: An #OGMRipLavc
 *
 * Gets whether to try to encode each MB with MV=<0,0>
 *
 * Returns: TRUE if mv0 is enabled
 */
gboolean
ogmrip_lavc_get_mv0 (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), FALSE);

  return lavc->priv->mv0;
}

/**
 * ogmrip_lavc_set_grayscale:
 * @lavc: An #OGMRipLavc
 * @grayscale: TRUE to enable grayscale
 *
 * Try to encode grayscale
 */
void
ogmrip_lavc_set_grayscale (OGMRipLavc *lavc, gboolean grayscale)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->grayscale = grayscale;
}

/**
 * ogmrip_lavc_get_grayscale:
 * @lavc: An #OGMRipLavc
 *
 * Gets whether to encode grayscale
 *
 * Returns: TRUE if grayscale is enabled
 */
gboolean
ogmrip_lavc_get_grayscale (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), FALSE);

  return lavc->priv->grayscale;
}

/**
 * ogmrip_lavc_set_qpel:
 * @lavc: An #OGMRipLavc
 * @qpel: TRUE to enable qpel
 *
 * Sets whether to encode using quarter pel
 */
void
ogmrip_lavc_set_qpel (OGMRipLavc *lavc, gboolean qpel)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->qpel = qpel;
}

/**
 * ogmrip_lavc_get_qpel:
 * @lavc: An #OGMRipLavc
 *
 * Gets whether to encode using quarter pel
 *
 * Returns: TRUE if qpel is enabled
 */
gboolean
ogmrip_lavc_get_qpel (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), FALSE);

  return lavc->priv->qpel;
}

/**
 * ogmrip_lavc_set_trellis:
 * @lavc: An #OGMRipLavc
 * @trellis: TRUE to enable trellis
 *
 * Sets trellis searched quantization
 */
void
ogmrip_lavc_set_trellis (OGMRipLavc *lavc, gboolean trellis)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->trellis = trellis;
}

/**
 * ogmrip_lavc_get_trellis:
 * @lavc: An #OGMRipLavc
 *
 * Gets trellis searched quantization
 *
 * Returns: TRUE if trellis is enabled
 */
gboolean
ogmrip_lavc_get_trellis (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), FALSE);

  return lavc->priv->trellis;
}

/**
 * ogmrip_lavc_set_v4mv:
 * @lavc: An #OGMRipLavc
 * @v4mv: TRUE to enable v4mv
 *
 * Sets whether to allow 4 motion vectors per macroblock
 */
void
ogmrip_lavc_set_v4mv (OGMRipLavc *lavc, gboolean v4mv)
{
  g_return_if_fail (OGMRIP_IS_LAVC (lavc));

  lavc->priv->v4mv = v4mv;
}

/**
 * ogmrip_lavc_get_v4mv:
 * @lavc: An #OGMRipLavc
 *
 * Gets whether to allow 4 motion vectors per macroblock
 *
 * Returns: TRUE if v4mv is enabled
 */
gboolean
ogmrip_lavc_get_v4mv (OGMRipLavc *lavc)
{
  g_return_val_if_fail (OGMRIP_IS_LAVC (lavc), FALSE);

  return lavc->priv->v4mv;
}

