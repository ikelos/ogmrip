/* OGMRipBluray - A bluray library for OGMRip
 * Copyright (C) 2012 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-bluray-makemkv.h"
#include "ogmrip-bluray-title.h"
#include "ogmrip-bluray-audio.h"
#include "ogmrip-bluray-subp.h"
#include "ogmrip-bluray-priv.h"

#include <ogmrip-base.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
  OGMRipMedia *media;
  OGMRipMediaCallback callback;
  gpointer user_data;
} ProgressData;

typedef struct
{
  gboolean visible;
  gboolean enabled;
  guint flags;
  gchar *name;
  gchar *device;
  gchar *label;
} DriveData;

enum
{
  ap_iaUnknown=0,
  ap_iaType=1,
  ap_iaName=2,
  ap_iaLangCode=3,
  ap_iaLangName=4,
  ap_iaCodecId=5,
  ap_iaCodecShort=6,
  ap_iaCodecLong=7,
  ap_iaChapterCount=8,
  ap_iaDuration=9,
  ap_iaDiskSize=10,
  ap_iaDiskSizeBytes=11,
  ap_iaStreamTypeExtension=12,
  ap_iaBitrate=13,
  ap_iaAudioChannelsCount=14,
  ap_iaAngleInfo=15,
  ap_iaSourceFileName=16,
  ap_iaAudioSampleRate=17,
  ap_iaAudioSampleSize=18,
  ap_iaVideoSize=19,
  ap_iaVideoAspectRatio=20,
  ap_iaVideoFrameRate=21,
  ap_iaStreamFlags=22,
  ap_iaDateTime=23,
  ap_iaOriginalTitleId=24,
  ap_iaSegmentsCount=25,
  ap_iaSegmentsMap=26,
  ap_iaOutputFileName=27,
  ap_iaMetadataLanguageCode=28,
  ap_iaMetadataLanguageName=29,
  ap_iaTreeInfo=30,
  ap_iaPanelTitle=31,
  ap_iaVolumeName=32,
  ap_iaOrderWeight=33,
  ap_iaOutputFormat=34,
  ap_iaOutputFormatDescription=35,
  ap_iaMaxValue
};

enum
{
  DVD_TYPE_DISK   = 6206,
  BRAY_TYPE_DISK  = 6209,
  HDDVD_TYPE_DISK = 6212,
  MKV_TYPE_FILE   = 6213
};

enum
{
  AP_AVStreamFlag_DirectorsComments          = 1,
  AP_AVStreamFlag_AlternateDirectorsComments = 2,
  AP_AVStreamFlag_ForVisuallyImpaired        = 4,
  AP_AVStreamFlag_CoreAudio                  = 256,
  AP_AVStreamFlag_SecondaryAudio             = 512,
  AP_AVStreamFlag_HasCoreAudio               = 1024,
  AP_AVStreamFlag_DerivedStream              = 2048,
  AP_AVStreamFlag_ForcedSubtitles            = 4096,
  AP_AVStreamFlag_ProfileSecondaryStream     = 16384
};

static OGMBrMakeMKV *default_mmkv;

static void
free_drive_data (DriveData *data)
{
  g_free (data->name);
  g_free (data->device);
  g_free (data->label);
  g_free (data);
}

static gint
parse_bitrate (const gchar *bitrate)
{
  gdouble value;
  gchar *unit;

  value = g_ascii_strtod (bitrate, &unit);
  if (*unit != '\0')
    return -1;

  if (g_str_equal (unit, " b/s"))
    return value;

  if (g_str_equal (unit, " Kb/s"))
    return value * 1024;

  if (g_str_equal (unit, " Mb/s"))
    return value * 1024 * 1024;

  return -1;
}

G_DEFINE_TYPE (OGMBrMakeMKV, ogmbr_makemkv, OGMJOB_TYPE_BIN);

static void
ogmbr_makemkv_finalize (GObject *gobject)
{
  OGMBrMakeMKV *mmkv = OGMBR_MAKEMKV (gobject);

  g_list_foreach (mmkv->priv->drives, (GFunc) free_drive_data, NULL);
  g_list_free (mmkv->priv->drives);

  G_OBJECT_CLASS (ogmbr_makemkv_parent_class)->finalize (gobject);
}

static void
ogmbr_makemkv_init (OGMBrMakeMKV *makemkv)
{
  makemkv->priv = G_TYPE_INSTANCE_GET_PRIVATE (makemkv, OGMBR_TYPE_MAKEMKV, OGMBrMakeMKVPriv);
}

static void
ogmbr_makemkv_class_init (OGMBrMakeMKVClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = ogmbr_makemkv_finalize;

  g_type_class_add_private (klass, sizeof (OGMBrMakeMKVPriv));
}

static void
ogmbr_makemkv_parse_media_info (OGMBrDisc *disc, guint attr, guint code, const gchar *value)
{
  switch (attr)
  {
    case ap_iaType:
      disc->priv->type = code;
      break;
    case ap_iaName:
      /* CINFO:30,0,"Serenity" */
      disc->priv->label = g_strdup (value);
      break;
    case ap_iaVolumeName:
      /* CINFO:32,0,"SERENITY_G51_GLO" */
      /* disc->priv->id = g_strdup (value); */
      break;
    default:
      break;
  }
}

static void
ogmbr_title_parse_duration (OGMBrTitle *title, const gchar *duration)
{
  guint h, m, s;

  if (sscanf (duration, "%u:%u:%u", &h, &m, &s) == 3)
    title->priv->length = h * 3600 + m * 60 + s;
}

static void
ogmbr_makemkv_parse_title_info (OGMBrDisc *disc, guint nr, guint attr, const gchar *value)
{
  OGMBrTitle *title;

  title = g_list_nth_data (disc->priv->titles, nr);
  if (!title)
  {
    title = g_object_new (OGMBR_TYPE_TITLE, NULL);
    disc->priv->titles = g_list_append (disc->priv->titles, title);
    disc->priv->ntitles ++;

    title->priv->media = OGMRIP_MEDIA (disc);
    title->priv->id = nr;
  }

  switch (attr)
  {
    case ap_iaChapterCount:
      /* TINFO:0,8,0,"4" */
      title->priv->nchapters = atoi (value);
      break;
    case ap_iaDuration:
      /* TINFO:0,9,0,"0:08:00" */
      ogmbr_title_parse_duration (title, value);
      break;
    case ap_iaDiskSizeBytes:
      /* TINFO:0,11,0,"577007616" */
      title->priv->size = atoi (value);
      break;
    default:
      break;
  }
}

static void
ogmbr_title_parse_video_format (OGMBrTitle *title, const gchar *codec)
{
  /*
   * TODO V_MS/VFW/FOURCC
  */
  if (g_str_equal (codec, "V_MPEG4/ISO/AVC"))
    title->priv->format = OGMRIP_FORMAT_H264;
  else if (g_str_equal (codec, "V_MPEG2"))
    title->priv->format = OGMRIP_FORMAT_MPEG2;
  else
    title->priv->format = OGMRIP_FORMAT_UNDEFINED;
}

static void
ogmbr_title_parse_aspect_ratio (OGMBrTitle *title, const gchar *aspect)
{
  guint num, denom;

  if (sscanf (aspect, "%u:%u", &num, &denom) == 2)
  {
    title->priv->aspect_num = num;
    title->priv->aspect_denom = denom;
  }
}

static void
ogmbr_title_parse_framerate (OGMBrTitle *title, const gchar *rate)
{
  guint a, b, num, denom;

  if (sscanf (rate, "%u.%u (%u/%u)", &a, &b, &num, &denom) == 4)
  {
    title->priv->rate_num = num;
    title->priv->rate_denom = denom;
  }
}

static void
ogmbr_title_parse_raw_size (OGMBrTitle *title, const gchar *size)
{
  guint width, height;

  if (sscanf (size, "%ux%u", &width, &height) == 2)
  {
    title->priv->raw_width = width;
    title->priv->raw_height = height;
  }
}

static void
ogmbr_title_parse_bitrate (OGMBrTitle *title, const gchar *bitrate)
{
  title->priv->bitrate = parse_bitrate (bitrate);
}

static void
ogmbr_video_stream_parse_track_info (OGMBrTitle *title, guint attr, const gchar *value)
{
  switch (attr)
  {
    case ap_iaCodecId:
      /* SINFO:1,0,5,0,"V_MS/VFW/FOURCC" */
      ogmbr_title_parse_video_format (title, value);
      break;
    case ap_iaVideoSize:
      /* SINFO:1,0,19,0,"1920x1080" */
      ogmbr_title_parse_raw_size (title, value);
      break;
    case ap_iaVideoAspectRatio:
      /* SINFO:1,0,20,0,"16:9" */
      ogmbr_title_parse_aspect_ratio (title, value);
      break;
    case ap_iaVideoFrameRate:
      /* SINFO:1,0,21,0,"23.976 (24000/1001)" */
      ogmbr_title_parse_framerate (title, value);
      break;
    case ap_iaBitrate:
      /* SINFO:0,1,13,0,"192 Kb/s" */
      ogmbr_title_parse_bitrate (title, value);
      break;
    case ap_iaStreamFlags:
      break;
    default:
      break;
  }
}

static void
ogmbr_audio_stream_parse_flags (OGMBrAudioStream *audio, gint flags)
{
  if (flags & AP_AVStreamFlag_DirectorsComments)
    audio->priv->content = OGMRIP_AUDIO_CONTENT_COMMENTS1;
  else if (flags & AP_AVStreamFlag_AlternateDirectorsComments)
    audio->priv->content = OGMRIP_AUDIO_CONTENT_COMMENTS2;
  else if (flags & AP_AVStreamFlag_ForVisuallyImpaired)
    audio->priv->content = OGMRIP_AUDIO_CONTENT_IMPAIRED;
  else
    audio->priv->content = OGMRIP_AUDIO_CONTENT_UNDEFINED;
}

static void
ogmbr_audio_stream_parse_bitrate (OGMBrAudioStream *audio, const gchar *bitrate)
{
  audio->priv->bitrate = parse_bitrate (bitrate);
}

static void
ogmbr_audio_stream_parse_format (OGMBrAudioStream *audio, const gchar *codec)
{
  /*
   * TODO A_EAC3, A_TRUEHD, A_MS/ACM
   */

  if (g_str_equal (codec, "A_AC3"))
    audio->priv->format = OGMRIP_FORMAT_AC3;
  else if (g_str_equal (codec, "A_DTS"))
    audio->priv->format = OGMRIP_FORMAT_DTS;
  else if (g_str_equal (codec, "A_PCM/INT/LIT") || g_str_equal (codec, "A_PCM/INT/BIG"))
    audio->priv->format = OGMRIP_FORMAT_PCM;
  else if (g_str_equal (codec, "A_FLAC"))
    audio->priv->format = OGMRIP_FORMAT_FLAC;
  else
    audio->priv->format = OGMRIP_FORMAT_UNDEFINED;
}

static void
ogmbr_audio_stream_parse_language (OGMBrAudioStream *audio, const gchar *language)
{
  audio->priv->language = ogmrip_language_from_iso639_2 (language);
}

static void
ogmbr_audio_stream_parse_track_info (OGMBrAudioStream *audio, guint attr, const gchar *value)
{
  switch (attr)
  {
    case ap_iaCodecId:
      /* SINFO:0,1,5,0,"A_AC3" */
      ogmbr_audio_stream_parse_format (audio, value);
      break;
    case ap_iaLangCode:
      /* SINFO:0,1,3,0,"eng" */
      ogmbr_audio_stream_parse_language (audio, value);
      break;
    case ap_iaBitrate:
      /* SINFO:0,1,13,0,"192 Kb/s" */
      ogmbr_audio_stream_parse_bitrate (audio, value);
      break;
    case ap_iaAudioChannelsCount:
      /* SINFO:0,1,14,0,"2" */
      audio->priv->channels = atoi (value);
      break;
    case ap_iaAudioSampleRate:
      /* SINFO:0,1,17,0,"48000" */
      audio->priv->samplerate = atoi (value);
      break;
    case ap_iaStreamFlags:
      /* SINFO:0,1,22,0,"0" */
      ogmbr_audio_stream_parse_flags (audio, atoi (value));
      break;
    default:
      break;
  }
}

static void
ogmbr_subp_stream_parse_flags (OGMBrSubpStream *subp, gint flags)
{
  subp->priv->content = OGMRIP_SUBP_CONTENT_UNDEFINED;
}

static void
ogmbr_subp_stream_parse_format (OGMBrSubpStream *subp, const gchar *codec)
{
  if (g_str_equal (codec, "S_HDMV/PGS"))
    subp->priv->format = OGMRIP_FORMAT_PGS;
  else if (g_str_equal (codec, "S_VOBSUB"))
    subp->priv->format = OGMRIP_FORMAT_VOBSUB;
  else
    subp->priv->format = OGMRIP_FORMAT_UNDEFINED;
}

static void
ogmbr_subp_stream_parse_language (OGMBrSubpStream *subp, const gchar *language)
{
  subp->priv->language = ogmrip_language_from_iso639_2 (language);
}

static void
ogmbr_subp_stream_parse_track_info (OGMBrSubpStream *subp, guint attr, const gchar *value)
{
  switch (attr)
  {
    case ap_iaCodecId:
      /* SINFO:0,2,5,0,"S_HDMV/PGS" */
      ogmbr_subp_stream_parse_format (subp, value);
      break;
    case ap_iaLangCode:
      /* SINFO:0,2,3,0,"eng" */
      ogmbr_subp_stream_parse_language (subp, value);
      break;
    case ap_iaStreamFlags:
      /* SINFO:0,2,22,0,"0" */
      ogmbr_subp_stream_parse_flags (subp, atoi (value));
      break;
    default:
      break;
  }
}

static void
ogmbr_makemkv_parse_track_info (OGMBrDisc *disc, guint nr, guint tr, guint attr, guint code, const gchar *value)
{
  OGMBrTitle *title;
  OGMRipStream *stream;

  title = g_list_nth_data (disc->priv->titles, nr);
  g_assert (title != NULL);

  stream = g_list_nth_data (title->priv->streams, tr);
  g_assert (stream != NULL || attr == ap_iaType);

  if (!stream)
  {
    g_assert (attr == ap_iaType);

    switch (code)
    {
      case 6201: /* PP_TTREE_VIDEO=6201 */
        stream = OGMRIP_STREAM (title);
        title->priv->vid = tr;
        break;
      case 6202: /* APP_TTREE_AUDIO=6202 */
        stream = g_object_new (OGMBR_TYPE_AUDIO_STREAM, NULL);
        OGMBR_AUDIO_STREAM (stream)->priv->id = tr;
        OGMBR_AUDIO_STREAM (stream)->priv->title = OGMRIP_TITLE (title);
        title->priv->audio_streams = g_list_append (title->priv->audio_streams, stream);
        title->priv->naudio_streams ++;
        break;
      case 6203: /* APP_TTREE_SUBPICTURE=6203 */
        stream = g_object_new (OGMBR_TYPE_SUBP_STREAM, NULL);
        OGMBR_SUBP_STREAM (stream)->priv->id = tr;
        OGMBR_SUBP_STREAM (stream)->priv->title = OGMRIP_TITLE (title);
        title->priv->subp_streams = g_list_append (title->priv->subp_streams, stream);
        title->priv->nsubp_streams ++;
        break;
      default:
        g_assert_not_reached ();
        break;
    }
    title->priv->streams = g_list_append (title->priv->streams, stream);
  }
  
  if (OGMRIP_IS_VIDEO_STREAM (stream))
    ogmbr_video_stream_parse_track_info (title, attr, value);
  else if (OGMRIP_IS_AUDIO_STREAM (stream))
    ogmbr_audio_stream_parse_track_info (OGMBR_AUDIO_STREAM (stream), attr, value);
  else if (OGMRIP_IS_SUBP_STREAM (stream))
    ogmbr_subp_stream_parse_track_info (OGMBR_SUBP_STREAM (stream), attr, value);
}

static gboolean
ogmbr_makemkv_parse_prgv (OGMJobTask *task, const gchar *buffer)
{
  guint current, total, max;

  if (sscanf (buffer, "PRGV:%u,%u,%u", &current, &total, &max) != 3)
    return FALSE;

  ogmjob_task_set_progress (task, total / (gdouble) max);

  return TRUE;
}

static gboolean
ogmbr_makemkv_parse_drv (OGMBrMakeMKV *mmkv, const gchar *buffer)
{
  DriveData *drive;
  guint index, visible, enabled, flags;
  gchar name[1024], label[1024], device[1024];

  if (sscanf (buffer, "DRV:%u,%u,%u,%u,\"%s\",\"%s\",\"%s\"",
        &index, &visible, &enabled, &flags, name, label, device) != 6)
    return FALSE;

  drive = g_new0 (DriveData, 1);
  drive->visible = visible > 0;
  drive->enabled = enabled > 0;
  drive->flags = flags;
  drive->name = g_strdup (name);
  drive->label = g_strdup (label);
  drive->device = g_strdup (device);

  mmkv->priv->drives = g_list_append (mmkv->priv->drives, drive);

  return TRUE;
}

static gboolean
ogmbr_makemkv_parse_device (OGMBrDisc *disc, const gchar *buffer)
{
  guint index, visible, enabled, flags;
  gchar name[1024], label[1024], device[1024];

  if (sscanf (buffer, "DRV:%u,%u,%u,%u,\"%s\",\"%s\",\"%s\"",
        &index, &visible, &enabled, &flags, name, label, device) != 6)
    return FALSE;

  if (g_str_equal (disc->priv->device, device))
    disc->priv->nr = index;

  return TRUE;
}

static gboolean
ogmbr_makemkv_parse_cinfo (OGMBrDisc *disc, const gchar *buffer)
{
  guint attr, code;
  gchar value[1024];

  if (sscanf (buffer, "CINFO:%u,%u,\"%s\"", &attr, &code, value) != 3)
    return FALSE;

  value[strlen (value) - 1] = '\0';
  ogmbr_makemkv_parse_media_info (disc, attr, code, value);

  return TRUE;
}

static gboolean
ogmbr_makemkv_parse_tinfo (OGMBrDisc *disc, const gchar *buffer)
{
  guint nr, attr, code;
  gchar value[1024];

  if (sscanf (buffer, "TINFO:%u,%u,%u,\"%s\"", &nr, &attr, &code, value) != 4)
    return FALSE;

  value[strlen (value) - 1] = '\0';
  ogmbr_makemkv_parse_title_info (disc, nr, attr, value);

  return TRUE;
}

static gboolean
ogmbr_makemkv_parse_sinfo (OGMBrDisc *disc, const gchar *buffer)
{
  guint nr, tr, attr, code;
  gchar value[1024];

  if (sscanf (buffer, "SINFO:%u,%u,%u,%u,\"%s\"", &nr, &tr, &attr, &code, value) != 5)
    return FALSE;

  value[strlen (value) - 1] = '\0';
  ogmbr_makemkv_parse_track_info (disc, nr, tr, attr, code, value);

  return TRUE;
}

OGMBrMakeMKV *
ogmbr_makemkv_get_default (void)
{
  if (!default_mmkv)
    default_mmkv = g_object_new (OGMBR_TYPE_MAKEMKV, NULL);
  else
    g_object_ref (default_mmkv);

  return default_mmkv;
}

static gboolean
ogmbr_makemkv_update_drives_watch (OGMJobTask *task, const gchar *buffer, OGMBrMakeMKV *mmkv, GError **error)
{
  if (ogmbr_makemkv_parse_prgv (task, buffer))
    return TRUE;

  if (ogmbr_makemkv_parse_drv (mmkv, buffer))
    return TRUE;

  return TRUE;
}

gboolean
ogmbr_makemkv_update_drives (OGMBrMakeMKV *mmkv, GCancellable *cancellable, GError **error)
{
  GPtrArray *argv;
  OGMJobTask *spawn;
  gboolean retval;

  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("makemkvcon"));
  g_ptr_array_add (argv, g_strdup ("--robot"));
  g_ptr_array_add (argv, g_strdup ("--noscan"));
  g_ptr_array_add (argv, g_strdup ("--cache=1"));
  g_ptr_array_add (argv, g_strdup ("--progress=-same"));
  g_ptr_array_add (argv, g_strdup ("info"));
  g_ptr_array_add (argv, g_strdup ("disc:9999"));
  g_ptr_array_add (argv, NULL);

  spawn = ogmjob_spawn_newv ((gchar **) g_ptr_array_free (argv, FALSE));
  ogmjob_container_add (OGMJOB_CONTAINER (mmkv), spawn);
  g_object_unref (spawn);

  ogmjob_spawn_set_watch (OGMJOB_SPAWN (spawn), OGMJOB_STREAM_OUTPUT,
      (OGMJobWatch) ogmbr_makemkv_update_drives_watch, mmkv, NULL);

  retval = ogmjob_task_run (OGMJOB_TASK (mmkv), cancellable, error);
  ogmjob_task_set_progress (spawn, 1.0);

  ogmjob_container_remove (OGMJOB_CONTAINER (mmkv), spawn);

  return retval;
}

static gboolean
ogmbr_makemkv_open_disc_watch (OGMJobTask *task, const gchar *buffer, OGMBrDisc *disc, GError **error)
{
  if (ogmbr_makemkv_parse_prgv (task, buffer))
    return TRUE;

  if (ogmbr_makemkv_parse_device (disc, buffer))
    return TRUE;

  if (ogmbr_makemkv_parse_cinfo (disc, buffer))
    return TRUE;

  if (ogmbr_makemkv_parse_tinfo (disc, buffer))
    return TRUE;

  if (ogmbr_makemkv_parse_sinfo (disc, buffer))
    return TRUE;

  return TRUE;
}

gboolean
ogmbr_makemkv_open_disc (OGMBrMakeMKV *mmkv, OGMBrDisc *disc, GCancellable *cancellable, GError **error)
{
  GPtrArray *argv;
  OGMJobTask *spawn;
  gboolean retval;

  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (OGMBR_IS_DISC (disc), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("makemkvcon"));
  g_ptr_array_add (argv, g_strdup ("--robot"));
  g_ptr_array_add (argv, g_strdup ("--noscan"));
  g_ptr_array_add (argv, g_strdup ("--cache=1"));
  g_ptr_array_add (argv, g_strdup ("--progress=-same"));
  g_ptr_array_add (argv, g_strdup ("info"));
  g_ptr_array_add (argv, g_strdup_printf ("dev:%s", disc->priv->device));
  g_ptr_array_add (argv, NULL);

  spawn = ogmjob_spawn_newv ((gchar **) g_ptr_array_free (argv, FALSE));
  ogmjob_container_add (OGMJOB_CONTAINER (mmkv), spawn);
  g_object_unref (spawn);

  ogmjob_spawn_set_watch (OGMJOB_SPAWN (spawn), OGMJOB_STREAM_OUTPUT,
      (OGMJobWatch) ogmbr_makemkv_open_disc_watch, disc, NULL);

  retval = ogmjob_task_run (OGMJOB_TASK (mmkv), cancellable, error);
  ogmjob_task_set_progress (spawn, 1.0);

  ogmjob_container_remove (OGMJOB_CONTAINER (mmkv), spawn);

  return retval;
}

static gboolean
ogmbr_makemkv_backup_disc_watch (OGMJobTask *task, const gchar *buffer, OGMBrDisc *disc, GError **error)
{
  if (ogmbr_makemkv_parse_prgv (task, buffer))
    return TRUE;

  return TRUE;
}

gboolean
ogmbr_makemkv_backup_disc (OGMBrMakeMKV *mmkv, OGMBrDisc *disc, const gchar *output, GCancellable *cancellable, GError **error)
{
  GPtrArray *argv;
  OGMJobTask *spawn;
  gboolean retval;

  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (OGMBR_IS_DISC (disc), FALSE);
  g_return_val_if_fail (output != NULL, FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (disc->priv->nr < 0)
    return FALSE;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("makemkvcon"));
  g_ptr_array_add (argv, g_strdup ("--robot"));
  g_ptr_array_add (argv, g_strdup ("--noscan"));
  g_ptr_array_add (argv, g_strdup ("--cache=16"));
  g_ptr_array_add (argv, g_strdup ("--decrypt"));
  g_ptr_array_add (argv, g_strdup ("--progress=-same"));
  g_ptr_array_add (argv, g_strdup ("backup"));
  g_ptr_array_add (argv, g_strdup_printf ("disc:%d", disc->priv->nr));
  g_ptr_array_add (argv, g_strdup_printf ("%s", output));
  g_ptr_array_add (argv, NULL);

  spawn = ogmjob_spawn_newv ((gchar **) g_ptr_array_free (argv, FALSE));
  ogmjob_container_add (OGMJOB_CONTAINER (mmkv), spawn);
  g_object_unref (spawn);

  ogmjob_spawn_set_watch (OGMJOB_SPAWN (spawn), OGMJOB_STREAM_OUTPUT,
      (OGMJobWatch) ogmbr_makemkv_backup_disc_watch, disc, NULL);

  retval = ogmjob_task_run (OGMJOB_TASK (mmkv), cancellable, error);
  ogmjob_task_set_progress (spawn, 1.0);

  ogmjob_container_remove (OGMJOB_CONTAINER (mmkv), spawn);

  return retval;
}

gboolean
ogmbr_makemkv_copy_disc (OGMBrMakeMKV *mmkv, OGMBrDisc *disc, const gchar *output, GCancellable *cancellable, GError **error)
{
  GPtrArray *argv;
  OGMJobTask *spawn;
  gboolean retval;

  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (OGMBR_IS_DISC (disc), FALSE);
  g_return_val_if_fail (output != NULL, FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("makemkvcon"));
  g_ptr_array_add (argv, g_strdup ("--robot"));
  g_ptr_array_add (argv, g_strdup ("--noscan"));
  g_ptr_array_add (argv, g_strdup ("--progress=-same"));
  g_ptr_array_add (argv, g_strdup ("mkv"));
  g_ptr_array_add (argv, g_strdup_printf ("dev:%s", disc->priv->device));
  g_ptr_array_add (argv, g_strdup ("all"));
  g_ptr_array_add (argv, g_strdup_printf ("%s", output));
  g_ptr_array_add (argv, NULL);

  spawn = ogmjob_spawn_newv ((gchar **) g_ptr_array_free (argv, FALSE));
  ogmjob_container_add (OGMJOB_CONTAINER (mmkv), spawn);
  g_object_unref (spawn);

  ogmjob_spawn_set_watch (OGMJOB_SPAWN (spawn), OGMJOB_STREAM_OUTPUT,
      (OGMJobWatch) ogmbr_makemkv_backup_disc_watch, disc, NULL);

  retval = ogmjob_task_run (OGMJOB_TASK (mmkv), cancellable, error);
  ogmjob_task_set_progress (spawn, 1.0);

  ogmjob_container_remove (OGMJOB_CONTAINER (mmkv), spawn);

  return retval;
}

gboolean
ogmbr_makemkv_copy_title (OGMBrMakeMKV *mmkv, OGMBrTitle *title, const gchar *output, GCancellable *cancellable, GError **error)
{
  OGMBrDisc *disc = OGMBR_DISC (title->priv->media);
  GPtrArray *argv;
  OGMJobTask *spawn;
  gboolean retval;

  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (OGMBR_IS_TITLE (title), FALSE);
  g_return_val_if_fail (output != NULL, FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, g_strdup ("makemkvcon"));
  g_ptr_array_add (argv, g_strdup ("--robot"));
  g_ptr_array_add (argv, g_strdup ("--noscan"));
  g_ptr_array_add (argv, g_strdup ("--progress=-same"));
  g_ptr_array_add (argv, g_strdup ("mkv"));
  g_ptr_array_add (argv, g_strdup_printf ("dev:%s", disc->priv->device));
  g_ptr_array_add (argv, g_strdup_printf ("%d", title->priv->id));
  g_ptr_array_add (argv, g_strdup_printf ("%s", output));
  g_ptr_array_add (argv, NULL);

  spawn = ogmjob_spawn_newv ((gchar **) g_ptr_array_free (argv, FALSE));
  ogmjob_container_add (OGMJOB_CONTAINER (mmkv), spawn);
  g_object_unref (spawn);

  ogmjob_spawn_set_watch (OGMJOB_SPAWN (spawn), OGMJOB_STREAM_OUTPUT,
      (OGMJobWatch) ogmbr_makemkv_backup_disc_watch, disc, NULL);

  retval = ogmjob_task_run (OGMJOB_TASK (mmkv), cancellable, error);
  ogmjob_task_set_progress (spawn, 1.0);

  ogmjob_container_remove (OGMJOB_CONTAINER (mmkv), spawn);

  return retval;
}

