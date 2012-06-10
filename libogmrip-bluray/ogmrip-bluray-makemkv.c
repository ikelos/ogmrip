/* OGMRipBluray - A bluray library for OGMBr
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

#include "ogmrip-bluray-audio.h"
#include "ogmrip-bluray-makemkv.h"
#include "ogmrip-bluray-priv.h"
#include "ogmrip-bluray-subp.h"
#include "ogmrip-bluray-title.h"

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>

#define VERSION "G000D"
#define SEM_TIMEOUT 9

#define MAKEMKV_IS_READY(mmkv, error) \
  (((mmkv)->priv->m_mem && (mmkv)->priv->m_mem->start == 1) || makemkv_spawn ((mmkv), (error)))

#define MAKEMKV_IS_RUNNING(mmkv) \
  (g_main_loop_is_running ((mmkv)->priv->loop))

typedef enum
{
  apNop=0,
  apReturn,
  apClientDone,
  apCallSignalExit,
  apCallOnIdle,
  apCallCancelAllJobs,
  apCallSetOutputFolder,
  apCallUpdateAvailableDrives,
  apCallOpenFile,
  apCallOpenCdDisk,
  apCallOpenTitleCollection,
  apCallCloseDisk,
  apCallEjectDisk,
  apCallSaveAllSelectedTitlesToMkv,
  apCallGetUiItemState,
  apCallSetUiItemState,
  apCallGetUiItemInfo,
  apCallGetSettingInt,
  apCallGetSettingString,
  apCallSetSettingInt,
  apCallSetSettingString,
  apCallSaveSettings,
  apCallAppGetString,
  apCallStartStreaming,
  apCallBackupDisc,
  apCallGetInterfaceLanguage,
  apCallGetInterfaceLanguageData,
  apCallGetProfileString,
  apCallSetUiItemInfo,
  apCallSetProfile,

  apBackEnterJobMode=192,
  apBackLeaveJobMode,
  apBackUpdateDrive,
  apBackUpdateCurrentBar,
  apBackUpdateTotalBar,
  apBackUpdateLayout,
  apBackSetTotalName,
  apBackUpdateCurrentInfo,
  apBackReportUiMesaage,
  apBackExit,

  apBackSetTitleCollInfo,
  apBackSetTitleInfo,
  apBackSetTrackInfo,

  apBackFatalCommError =250,
  apBackOutOfMem,

  apUnknown
} AP_CMD;

enum
{
  mttUnknown,
  mttVideo,
  mttAudio,
  mttSubtitle
};

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

typedef struct
{
  uint32_t cmd;
  uint8_t  start;
  uint8_t  abort_nomem;
  uint8_t  pad0[2];
  uint64_t args[32];
  gunichar2  strbuf[32768];
} __attribute((packed)) AP_SHMEM;

struct _OGMBrMakeMKVPriv
{
  /* shared memory */
  volatile AP_SHMEM *m_mem;
  /* semaphores */
  sem_t *sem[2];
  /* result */
  AP_CMD res;
  /* is shutting down */
  gboolean m_shutdown;
  /* timeout */
  GSource *timeout;
  GMainContext *context;
  GMainLoop *loop;
  gboolean m_in_idle;
  /* drives */
  GList *drives;
  gint opened;
};

typedef struct
{
  OGMBrMakeMKVProgress callback;
  gpointer user_data;
  gdouble fraction;
} ProgressData;

static OGMBrMakeMKV *default_mmkv = NULL;

static ProgressData *
progress_data_new (OGMBrMakeMKVProgress callback, gpointer user_data, gdouble fraction)
{
  ProgressData *data;

  data = g_new0 (ProgressData, 1);
  data->callback = callback;
  data->user_data = user_data;
  data->fraction = fraction;

  return data;
}

static void
progress_data_free (ProgressData *data)
{
  g_free (data);
}

static void
makemkv_free_drives (OGMBrMakeMKV *mmkv)
{
  while (mmkv->priv->drives)
  {
    OGMBrDrive *drive = mmkv->priv->drives->data;

    mmkv->priv->drives = g_list_remove_link (mmkv->priv->drives, mmkv->priv->drives);

    g_free (drive->name);
    g_free (drive->device);
    g_free (drive->title);
    g_free (drive);
  }
}

static gint
makemkv_compare_drive_by_device (OGMBrDrive *drive, const gchar *device)
{
  return strcmp (drive->device, device);
}

static gboolean
makemkv_progress_in_main_cb (ProgressData *data)
{
  data->callback (data->fraction, data->user_data);

  return FALSE;
}

static gboolean
makemkv_open_shmem (OGMBrMakeMKV *mmkv, const gchar *name)
{
  int fd;
  struct stat st;

  fd = shm_open (name, O_RDWR, 0);
  if (fd < 0)
    return FALSE;

  shm_unlink (name);

  if (fstat (fd, &st))
    return FALSE;

  mmkv->priv->m_mem = mmap (NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mmkv->priv->m_mem == MAP_FAILED)
    return FALSE;

  return TRUE;
}

static gboolean
makemkv_open_sgrp (OGMBrMakeMKV *mmkv)
{
  const guint64 *data = (guint64 *) mmkv->priv->m_mem->args;
  guint i;

  for (i = 0; i <= 1; i ++)
  {
    const char* name;

    name = ((const char*) data) + data[i];
    mmkv->priv->sem[i] = sem_open (name, O_RDWR);
    sem_unlink (name);

    if (mmkv->priv->sem[i] == SEM_FAILED)
      return FALSE;
  }

  return TRUE;
}

static gboolean
makemkv_sem_inc (OGMBrMakeMKV *mmkv)
{
  return sem_post (mmkv->priv->sem[0]) == 0;
}

static gboolean
makemkv_sem_dec (OGMBrMakeMKV *mmkv)
{
  struct timespec tm;
  struct timeval  tv;

  gettimeofday (&tv, NULL);

  tm.tv_sec  = tv.tv_sec + SEM_TIMEOUT;
  tm.tv_nsec = tv.tv_usec * 1000;

  return sem_timedwait (mmkv->priv->sem[1], &tm) == 0;
}

static AP_CMD
makemkv_fatal_comm_error (OGMBrMakeMKV *mmkv)
{
  mmkv->priv->m_shutdown = TRUE;

  if (mmkv->priv->m_mem->abort_nomem)
    return apBackOutOfMem;

  return apBackFatalCommError ;
}

static AP_CMD
makemkv_transact (OGMBrMakeMKV *mmkv, AP_CMD cmd)
{
  if (mmkv->priv->m_shutdown)
    return apReturn;

  mmkv->priv->m_mem->cmd = cmd;

  if (mmkv->priv->m_mem->abort_nomem)
    return makemkv_fatal_comm_error (mmkv);

  if (!makemkv_sem_inc (mmkv))
    return makemkv_fatal_comm_error (mmkv);

  if (mmkv->priv->m_mem->abort_nomem)
    return makemkv_fatal_comm_error (mmkv);

  if (!makemkv_sem_dec (mmkv))
    return makemkv_fatal_comm_error (mmkv);

  if (mmkv->priv->m_mem->abort_nomem)
    return makemkv_fatal_comm_error (mmkv);

  return (AP_CMD) mmkv->priv->m_mem->cmd;
}

static gboolean
makemkv_check_version (OGMBrMakeMKV *mmkv, const gchar *available_versions)
{
  gboolean retval = FALSE;
  gchar **versions;
  guint i;

  versions = g_strsplit (available_versions, "/", -1);
  for (i = 0; !retval && versions[i]; i ++)
    retval = g_str_equal (versions[i], VERSION);
  g_strfreev (versions);

  return retval;
}

static void
makemkv_exit (OGMBrMakeMKV *mmkv)
{
  g_message ("Exit");

  mmkv->priv->m_shutdown = TRUE;
}

static void
makemkv_enter_job (OGMBrMakeMKV *mmkv)
{
  g_message ("Enter");

  g_main_loop_run (mmkv->priv->loop);
}

static void
makemkv_leave_job (OGMBrMakeMKV *mmkv)
{
  g_message ("Leave");

  g_main_loop_quit (mmkv->priv->loop);
}

static void
makemkv_update_drive (OGMBrMakeMKV *mmkv)
{
  if (mmkv->priv->m_mem->args[3] != 0)
  {
    OGMBrDrive *drive;
    const gunichar2 *str;
    glong len;

    g_message ("Update drive");

    drive = g_new0 (OGMBrDrive, 1);
    drive->id = mmkv->priv->m_mem->args[0];

    str = (gunichar2 *) mmkv->priv->m_mem->strbuf;
    if (mmkv->priv->m_mem->args[1] != 0)
    {
      drive->name = g_utf16_to_utf8 (str, -1, &len, NULL, NULL);
      str += len + 1;
    }

    if ((mmkv->priv->m_mem->args[4] & 1) != 0)
    {
      drive->title = g_utf16_to_utf8 (str, -1, &len, NULL, NULL);
      str += len + 1;
    }

    if ((mmkv->priv->m_mem->args[4] & 2) != 0)
    {
      drive->device = g_utf16_to_utf8 (str, -1, &len, NULL, NULL);
      str += len + 1;
    }

    mmkv->priv->drives = g_list_prepend (mmkv->priv->drives, drive);
  }

  mmkv->priv->drives = g_list_reverse (mmkv->priv->drives);
}

static void
makemkv_error (OGMBrMakeMKV *mmkv, GError **error)
{
  if (mmkv->priv->res == apBackFatalCommError)
    g_set_error (error, OGMBR_MAKEMKV_ERROR, OGMBR_MAKEMKV_ERROR_COMM,
        "Fatal error occurred, program will now exit");
  else
    g_set_error (error, OGMBR_MAKEMKV_ERROR, OGMBR_MAKEMKV_ERROR_MEM,
        "Not enough memory for application to continue, program will now exit");
}

static void
makemkv_update_total (OGMBrMakeMKV *mmkv)
{
  ProgressData *data;

  data = g_object_get_data (G_OBJECT (mmkv), "progress");
  if (data)
  {
    data->fraction = mmkv->priv->m_mem->args[0] / 65536.;
    if (data->callback)
      data->callback (data->fraction, data->user_data);
  }
}

static void
makemkv_set_total_name (OGMBrMakeMKV *mmkv)
{
/*
  g_message ("Set total name : %lu", (gulong) mmkv->priv->m_mem->args[0]);
*/
}

static void
makemkv_update_layout (OGMBrMakeMKV *mmkv)
{
/*
  guint index = (guint) mmkv->priv->m_mem->args[1], flags = (guint) mmkv->priv->m_mem->args[2], size = (guint) mmkv->priv->m_mem->args[3], i;
  gulong name = (gulong) mmkv->priv->m_mem->args[0];

  g_message ("Update layout : %lu, %u, %u, %u", name, index, flags, size);

  for (i = 0; i < size; i ++)
    g_message ("  name %u : %lu", i, (gulong) mmkv->priv->m_mem->args[4 + i]);
*/
}

static void
makemkv_current_info (OGMBrMakeMKV *mmkv)
{
/*
  guint index = (guint) mmkv->priv->m_mem->args[0];
  gchar *str;

  str = g_utf16_to_utf8 ((gunichar2 *) mmkv->priv->m_mem->strbuf, -1, NULL, NULL, NULL);
  g_message ("Update current info : %u, %s", index, str);
  g_free (str);
*/
}

static void
makemkv_report_message (OGMBrMakeMKV *mmkv)
{
/*
  gulong code = (gulong) mmkv->priv->m_mem->args[0], flags = (gulong) mmkv->priv->m_mem->args[2];
  gchar *str;

  str = g_utf16_to_utf8 ((gunichar2 *) mmkv->priv->m_mem->strbuf, -1, NULL, NULL, NULL);
  g_message ("Report ui message : %lu, %lu, %s", code, flags, str);
  g_free (str);
*/
  mmkv->priv->m_mem->args[0] = 0;
}

static void
makemkv_collection_info (OGMBrMakeMKV *mmkv)
{
  OGMBrDisc *disc;

  disc = g_object_get_data (G_OBJECT (mmkv), "disc");
  g_assert (disc != NULL);

  disc->priv->handle = mmkv->priv->m_mem->args[0];
  disc->priv->ntitles = (guint) mmkv->priv->m_mem->args[1];
/*
  g_message ("Collection info : %llu, %u", handle, ntitles);
*/
}

static void
makemkv_title_info (OGMBrMakeMKV *mmkv)
{
  OGMBrDisc *disc;
  OGMBrTitle *title;

  disc = g_object_get_data (G_OBJECT (mmkv), "disc");
  g_assert (disc != NULL);

  title = g_object_new (OGMBR_TYPE_TITLE, NULL);
  disc->priv->titles = g_list_append (disc->priv->titles, title);

  title->priv->media = OGMRIP_MEDIA (disc);
  title->priv->id = (guint) mmkv->priv->m_mem->args[0];
  title->priv->handle = mmkv->priv->m_mem->args[1];
/*
  g_message ("Title info : %u, %llu, %u", id, handle, ntracks);
*/
}

static void
makemkv_video_track_info (OGMBrMakeMKV *mmkv, OGMBrDisc *disc, OGMBrTitle *title)
{
  title->priv->video_id = (guint) mmkv->priv->m_mem->args[1];
  title->priv->video_handle = mmkv->priv->m_mem->args[2];
}

static void
makemkv_audio_track_info (OGMBrMakeMKV *mmkv, OGMBrDisc *disc, OGMBrTitle *title)
{
  OGMBrAudioStream *audio;

  audio = g_object_new (OGMBR_TYPE_AUDIO_STREAM, NULL);
  title->priv->audio_streams = g_list_append (title->priv->audio_streams, audio);
  title->priv->naudio_streams ++;

  audio->priv->title = OGMRIP_TITLE (title);
  audio->priv->id = (guint) mmkv->priv->m_mem->args[1];
  audio->priv->handle = mmkv->priv->m_mem->args[2];
}

static void
makemkv_subp_track_info (OGMBrMakeMKV *mmkv, OGMBrDisc *disc, OGMBrTitle *title)
{
  OGMBrSubpStream *subp;

  subp = g_object_new (OGMBR_TYPE_SUBP_STREAM, NULL);
  title->priv->subp_streams = g_list_append (title->priv->subp_streams, subp);
  title->priv->nsubp_streams ++;

  subp->priv->title = OGMRIP_TITLE (title);
  subp->priv->id = (guint) mmkv->priv->m_mem->args[1];
  subp->priv->handle = mmkv->priv->m_mem->args[2];
}

static void
makemkv_track_info (OGMBrMakeMKV *mmkv)
{
  OGMBrDisc *disc;
  OGMBrTitle *title;

  guint id = (guint) mmkv->priv->m_mem->args[0], type = (guint) mmkv->priv->m_mem->args[3];

  disc = g_object_get_data (G_OBJECT (mmkv), "disc");
  g_assert (disc != NULL);

  title = g_list_nth_data (disc->priv->titles, id);
  g_assert (title != NULL);

  switch (type)
  {
    case mttVideo:
      makemkv_video_track_info (mmkv, disc, title);
      break;
    case mttAudio:
      makemkv_audio_track_info (mmkv, disc, title);
      break;
    case mttSubtitle:
      makemkv_subp_track_info (mmkv, disc, title);
      break;
    default:
      break;
  }
/*
  g_message ("Track info : %u, %u, %llu, %u", id, track, handle, type);
*/
}

static gboolean
makemkv_run_command (OGMBrMakeMKV *mmkv, AP_CMD cmd, GCancellable *cancellable, GError **error)
{
  while ((mmkv->priv->res = makemkv_transact (mmkv, cmd)) != apReturn)
  {
    if (g_cancellable_set_error_if_cancelled (cancellable, error))
    {
      makemkv_run_command (mmkv, apCallCancelAllJobs, NULL, NULL);
      return FALSE;
    }

    switch (mmkv->priv->res)
    {
      case apNop:
        break;
      case apBackEnterJobMode:
        makemkv_enter_job (mmkv);
        break;
      case apBackLeaveJobMode:
        makemkv_leave_job (mmkv);
        break;
      case apBackExit:
        makemkv_exit (mmkv);
        return FALSE;
        break;
      case apBackFatalCommError:
        makemkv_error (mmkv, error);
        return FALSE;
        break;
      case apBackOutOfMem:
        makemkv_error (mmkv, error);
        return FALSE;
        break;
      case apBackUpdateDrive:
        makemkv_update_drive (mmkv);
        break;
      case apBackSetTotalName:
        makemkv_set_total_name (mmkv);
        break;
      case apBackUpdateLayout:
        makemkv_update_layout (mmkv);
        break;
      case apBackUpdateCurrentInfo:
        makemkv_current_info (mmkv);
        break;
      case apBackReportUiMesaage:
        makemkv_report_message (mmkv);
        break;
      case apBackUpdateCurrentBar:
        break;
      case apBackUpdateTotalBar:
        makemkv_update_total (mmkv);
        break;
      case apBackSetTitleCollInfo:
        makemkv_collection_info (mmkv);
        break;
      case apBackSetTitleInfo:
        makemkv_title_info (mmkv);
        break;
      case apBackSetTrackInfo:
        makemkv_track_info (mmkv);
        break;
      default:
        mmkv->priv->m_mem->args[0] = 0;
        break;
    }

    cmd = apClientDone;
  }

  return TRUE;
}

static gchar *
makemkv_get_info (OGMBrMakeMKV *mmkv, guint64 handle, guint info)
{
  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), NULL);

  mmkv->priv->m_mem->args[0] = handle;
  mmkv->priv->m_mem->args[1] = info;
  makemkv_run_command (mmkv, apCallGetUiItemInfo, NULL, NULL);

  if (mmkv->priv->m_mem->args[0] != 0)
    g_message ("info: %d", (unsigned int) mmkv->priv->m_mem->args[0]);
  else if (mmkv->priv->m_mem->args[1] != 0)
  {
    gchar *str;

    str = g_utf16_to_utf8 ((gunichar2 *) mmkv->priv->m_mem->strbuf, -1, NULL, NULL, NULL);
    g_message ("info: %s", str);
    g_free (str);
  }

  return NULL;
}

static gboolean
makemkv_timeout (OGMBrMakeMKV *mmkv)
{
  gboolean res = TRUE;

  if (!mmkv->priv->m_in_idle)
  {
    mmkv->priv->m_in_idle = TRUE;
    res = makemkv_run_command (mmkv, apCallOnIdle, NULL, NULL);
    mmkv->priv->m_in_idle = FALSE;
  }

  return res;
}

static gboolean
makemkv_spawn (OGMBrMakeMKV *mmkv, GError **error)
{
  gchar *argv[4], str[512], *name;
  gint fd, i, r;

  argv[0] = "makemkvcon";
  argv[1] = "guiserver";
  argv[2] = VERSION;
  argv[3] = NULL;

  if (!g_spawn_async_with_pipes (NULL, argv, NULL,
        G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL,
        NULL, NULL, NULL, NULL, &fd, NULL, error))
    return FALSE;

  for (i = 0; i < 512 - 1; i++)
  {
    if ((r = read (fd, str + i, 1)) != 1)
    {
      close (fd);
      return FALSE;
    }

    if (str[i] == '$')
      break;
  }
  close (fd);

  str[i] = 0;
  name = strchr (str,':');
  if (!name)
  {
    g_error ("Cannot retrieve shared memory object");
    return FALSE;
  }
  *name = 0;

  if (!makemkv_check_version (mmkv, str))
  {
    g_error ("Versions are not compatible");
    return FALSE;
  }

  if (!makemkv_open_shmem (mmkv, name + 1))
  {
    g_error ("Cannot open shared memory object");
    return FALSE;
  }

  if (!makemkv_open_sgrp (mmkv))
  {
    g_error ("Cannot initialize semaphore");
    return FALSE;
  }

  mmkv->priv->timeout = g_timeout_source_new (100);
  g_source_set_callback (mmkv->priv->timeout, (GSourceFunc) makemkv_timeout, mmkv, NULL);
  g_source_attach (mmkv->priv->timeout, mmkv->priv->context);

  mmkv->priv->m_mem->start = 1;

  return TRUE;
}

GQuark
ogmbr_makemkv_error_quark (void)
{
  return g_quark_from_static_string ("ogmbr-makemkv-error-quark");
}

G_DEFINE_TYPE (OGMBrMakeMKV, ogmbr_makemkv, G_TYPE_OBJECT);

static GObject *
ogmbr_makemkv_constructor (GType type, guint n_params, GObjectConstructParam *params)
{
  GObject *gobject;

  if (default_mmkv)
    return g_object_ref (default_mmkv);

  gobject = G_OBJECT_CLASS (ogmbr_makemkv_parent_class)->constructor (type, n_params, params);

  default_mmkv = OGMBR_MAKEMKV (gobject);
  g_object_add_weak_pointer (gobject, (gpointer *) &default_mmkv);

  return gobject;
}

static void
ogmbr_makemkv_finalize (GObject *gobject)
{
  OGMBrMakeMKV *mmkv = OGMBR_MAKEMKV (gobject);

  makemkv_free_drives (mmkv);

  if (mmkv->priv->m_mem && mmkv->priv->m_mem->start == 1)
  {
    if (mmkv->priv->opened >= 0)
      ogmbr_makemkv_close_disc (mmkv, NULL, NULL);

    makemkv_run_command (mmkv, apCallSignalExit, NULL, NULL);
  }

  if (mmkv->priv->sem[0] != SEM_FAILED)
    sem_close (mmkv->priv->sem[0]);

  if (mmkv->priv->sem[1] != SEM_FAILED)
    sem_close (mmkv->priv->sem[1]);

  if (mmkv->priv->timeout)
  {
    g_source_destroy (mmkv->priv->timeout);
    g_source_unref (mmkv->priv->timeout);
  }

  g_main_loop_unref (mmkv->priv->loop);
  g_main_context_unref (mmkv->priv->context);

  G_OBJECT_CLASS (ogmbr_makemkv_parent_class)->finalize (gobject);
}

static void
ogmbr_makemkv_init (OGMBrMakeMKV *mmkv)
{
  mmkv->priv = G_TYPE_INSTANCE_GET_PRIVATE (mmkv, OGMBR_TYPE_MAKEMKV, OGMBrMakeMKVPriv);

  mmkv->priv->opened = -1;
  mmkv->priv->sem[0] = SEM_FAILED;
  mmkv->priv->sem[1] = SEM_FAILED;

  mmkv->priv->context = g_main_context_ref_thread_default ();
  mmkv->priv->loop = g_main_loop_new (mmkv->priv->context, FALSE);
}

static void
ogmbr_makemkv_class_init (OGMBrMakeMKVClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructor = ogmbr_makemkv_constructor;
  gobject_class->finalize = ogmbr_makemkv_finalize;

  g_type_class_add_private (klass, sizeof (OGMBrMakeMKVPriv));
}

OGMBrMakeMKV *
ogmbr_makemkv_get_default (void)
{
  return g_object_new (OGMBR_TYPE_MAKEMKV, NULL);
}

GList *
ogmbr_makemkv_update_drives (OGMBrMakeMKV *mmkv, GCancellable *cancellable, GError **error)
{
  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (MAKEMKV_IS_RUNNING (mmkv) || !MAKEMKV_IS_READY (mmkv, error))
    return FALSE;

  makemkv_free_drives (mmkv);

  if (!makemkv_run_command (mmkv, apCallUpdateAvailableDrives, cancellable, error))
    return NULL;

  if (mmkv->priv->m_mem->args[0] == 0)
  {
    makemkv_free_drives (mmkv);
    return NULL;
  }

  return g_list_copy (mmkv->priv->drives);
}

static gboolean
ogmbr_makemkv_update_drives_thread (GIOSchedulerJob *job, GCancellable *cancellable, GSimpleAsyncResult *simple)
{
  GError *error = NULL;
  OGMBrMakeMKV *mmkv;
  GList *drives;

  mmkv = g_simple_async_result_get_op_res_gpointer (simple);

  drives = ogmbr_makemkv_update_drives (mmkv, cancellable, &error);
  g_simple_async_result_set_op_res_gpointer (simple, drives, NULL);

  if (!drives && error)
    g_simple_async_result_take_error (simple, error);

  g_simple_async_result_complete_in_idle (simple);

  return FALSE;
}

void
ogmbr_makemkv_update_drives_async (OGMBrMakeMKV *mmkv, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  GSimpleAsyncResult *simple;

  g_return_if_fail (OGMBR_IS_MAKEMKV (mmkv));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  simple = g_simple_async_result_new (G_OBJECT (mmkv), callback, user_data, ogmbr_makemkv_update_drives_async);
  g_simple_async_result_set_op_res_gpointer (simple, g_object_ref (mmkv), g_object_unref);

  g_io_scheduler_push_job ((GIOSchedulerJobFunc) ogmbr_makemkv_update_drives_thread,
      simple, g_object_unref, G_PRIORITY_DEFAULT, cancellable);
}

GList *
ogmbr_makemkv_update_drives_finish (OGMBrMakeMKV *mmkv, GSimpleAsyncResult *res, GError **error)
{
  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (res), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  g_warn_if_fail (g_simple_async_result_get_source_tag (res) == ogmbr_makemkv_update_drives_async);

  if (g_simple_async_result_propagate_error (res, error))
    return NULL;

  return g_simple_async_result_get_op_res_gpointer (res);
}

gboolean
ogmbr_makemkv_open_disc (OGMBrMakeMKV *mmkv, OGMBrDisc *disc, GCancellable *cancellable,
    OGMBrMakeMKVProgress progress_cb, gpointer progress_data, GError **error)
{
  GList *link1, *link2;
  OGMBrDrive *drive;
  gboolean res;

  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  link1 = g_list_find_custom (mmkv->priv->drives, disc->priv->device,
      (GCompareFunc) makemkv_compare_drive_by_device);
  if (!link1)
    return FALSE;
  drive = link1->data;

  if (mmkv->priv->opened >= 0)
    return mmkv->priv->opened == drive->id;

  if (MAKEMKV_IS_RUNNING (mmkv) || !MAKEMKV_IS_READY (mmkv, error))
    return FALSE;

  g_object_set_data_full (G_OBJECT (mmkv), "disc",
      g_object_ref (disc), g_object_unref);

  g_object_set_data_full (G_OBJECT (mmkv), "progress",
      progress_data_new (progress_cb, progress_data, 0.0),
      (GDestroyNotify) progress_data_free);

  mmkv->priv->m_mem->args[0] = drive->id;
  res = makemkv_run_command (mmkv, apCallOpenCdDisk, cancellable, error);

  g_object_set_data (G_OBJECT (mmkv), "disc", NULL);
  g_object_set_data (G_OBJECT (mmkv), "progress", NULL);

  if (!res || mmkv->priv->m_mem->args[0] == 0)
    return FALSE;

  if (!disc->priv->label)
    disc->priv->label = g_strdup (drive->title);

  mmkv->priv->opened = drive->id;

  makemkv_get_info (mmkv, disc->priv->handle, ap_iaDiscSizeBytes);

  for (link1 = disc->priv->titles; link1; link1 = link1->next)
  {
    OGMBrTitle *title = link1->data;

    makemkv_get_info (mmkv, title->priv->handle, ap_iaVideoSize);
    makemkv_get_info (mmkv, title->priv->handle, ap_iaDuration);
    makemkv_get_info (mmkv, title->priv->handle, ap_iaAngleInfo);
    makemkv_get_info (mmkv, title->priv->handle, ap_iaChapterCount);

    makemkv_get_info (mmkv, title->priv->video_handle, ap_iaVideoFrameRate);
    makemkv_get_info (mmkv, title->priv->video_handle, ap_iaVideoAspectRatio);

    for (link2 = title->priv->audio_streams; link2; link2 = link2->next)
    {
      OGMBrAudioStream *audio = link2->data;

      makemkv_get_info (mmkv, audio->priv->handle, ap_iaCodecId);
      makemkv_get_info (mmkv, audio->priv->handle, ap_iaBitrate);
      makemkv_get_info (mmkv, audio->priv->handle, ap_iaAudioChannelsCount);
      makemkv_get_info (mmkv, audio->priv->handle, ap_iaLangCode);
      makemkv_get_info (mmkv, audio->priv->handle, ap_iaAudioSampleRate);
    }

    for (link2 = title->priv->subp_streams; link2; link2 = link2->next)
    {
      OGMBrSubpStream *subp = link2->data;

      makemkv_get_info (mmkv, subp->priv->handle, ap_iaCodecId);
      makemkv_get_info (mmkv, subp->priv->handle, ap_iaLangCode);
    }
  }

  return TRUE;
}

typedef struct
{
  OGMBrMakeMKV *mmkv;
  OGMBrDisc *disc;
} DiskAsyncData;

static DiskAsyncData *
disc_async_data_new (OGMBrMakeMKV *mmkv, OGMBrDisc *disc)
{
  DiskAsyncData *data;

  data = g_new0 (DiskAsyncData, 1);
  data->mmkv = g_object_ref (mmkv);
  data->disc = g_object_ref (disc);
  
  return data;
}

static void
disc_async_data_free (DiskAsyncData *data)
{
  g_object_unref (data->mmkv);
  g_object_unref (data->disc);
  g_free (data);
}

static gboolean
ogmbr_makemkv_open_disc_thread (GIOSchedulerJob *job, GCancellable *cancellable, GSimpleAsyncResult *simple)
{
  GError *error = NULL;
  DiskAsyncData *data;
  gboolean res;

  data = g_simple_async_result_get_op_res_gpointer (simple);

  res = ogmbr_makemkv_open_disc (data->mmkv, data->disc, cancellable, NULL, NULL, &error);
  g_simple_async_result_set_op_res_gboolean (simple, res);

  if (!res && error)
    g_simple_async_result_take_error (simple, error);

  g_simple_async_result_complete_in_idle (simple);

  return FALSE;
}

void
ogmbr_makemkv_open_disc_async (OGMBrMakeMKV *mmkv, OGMBrDisc *disc, GCancellable *cancellable,
    OGMBrMakeMKVProgress progress_cb, gpointer progress_data, GAsyncReadyCallback callback, gpointer user_data)
{
  GSimpleAsyncResult *simple;

  g_return_if_fail (OGMBR_IS_MAKEMKV (mmkv));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  simple = g_simple_async_result_new (G_OBJECT (mmkv), callback, user_data, ogmbr_makemkv_open_disc_async);
  g_simple_async_result_set_op_res_gpointer (simple, disc_async_data_new (mmkv, disc), (GDestroyNotify) disc_async_data_free);

  g_io_scheduler_push_job ((GIOSchedulerJobFunc) ogmbr_makemkv_open_disc_thread,
      simple, g_object_unref, G_PRIORITY_DEFAULT, cancellable);
}

gboolean
ogmbr_makemkv_open_disc_finish (OGMBrMakeMKV *mmkv, GSimpleAsyncResult *res, GError **error)
{
  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (res), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  g_warn_if_fail (g_simple_async_result_get_source_tag (res) == ogmbr_makemkv_open_disc_async);

  if (g_simple_async_result_propagate_error (res, error))
    return FALSE;

  return g_simple_async_result_get_op_res_gboolean (res);
}

gboolean
ogmbr_makemkv_close_disc (OGMBrMakeMKV *mmkv, GCancellable *cancellable, GError **error)
{
  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (!mmkv->priv->opened < 0)
    return TRUE;

  if (MAKEMKV_IS_RUNNING (mmkv) || !MAKEMKV_IS_READY (mmkv, error))
    return FALSE;

  if (!makemkv_run_command (mmkv, apCallCloseDisk, cancellable, error))
    return FALSE;

  mmkv->priv->opened = -1;

  return mmkv->priv->m_mem->args[0] != 0;
}

static gboolean
ogmbr_makemkv_close_disc_thread (GIOSchedulerJob *job, GCancellable *cancellable, GSimpleAsyncResult *simple)
{
  GError *error = NULL;
  DiskAsyncData *data;
  gboolean res;

  data = g_simple_async_result_get_op_res_gpointer (simple);

  res = ogmbr_makemkv_close_disc (data->mmkv, cancellable, &error);
  g_simple_async_result_set_op_res_gboolean (simple, res);

  if (!res && error)
    g_simple_async_result_take_error (simple, error);

  g_simple_async_result_complete_in_idle (simple);

  return FALSE;
}

void
ogmbr_makemkv_close_disc_async (OGMBrMakeMKV *mmkv, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  GSimpleAsyncResult *simple;

  g_return_if_fail (OGMBR_IS_MAKEMKV (mmkv));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  simple = g_simple_async_result_new (G_OBJECT (mmkv), callback, user_data, ogmbr_makemkv_close_disc_async);
  g_simple_async_result_set_op_res_gpointer (simple, disc_async_data_new (mmkv, 0), (GDestroyNotify) disc_async_data_free);

  g_io_scheduler_push_job ((GIOSchedulerJobFunc) ogmbr_makemkv_close_disc_thread,
      simple, g_object_unref, G_PRIORITY_DEFAULT, cancellable);
}

gboolean
ogmbr_makemkv_close_disc_finish (OGMBrMakeMKV *mmkv, GSimpleAsyncResult *res, GError **error)
{
  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (res), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  g_warn_if_fail (g_simple_async_result_get_source_tag (res) == ogmbr_makemkv_close_disc_async);

  if (g_simple_async_result_propagate_error (res, error))
    return FALSE;

  return g_simple_async_result_get_op_res_gboolean (res);
}

gboolean
ogmbr_makemkv_eject_disc (OGMBrMakeMKV *mmkv, OGMBrDisc *disc, GCancellable *cancellable, GError **error)
{
  GList *link;
  OGMBrDrive *drive;

  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (MAKEMKV_IS_RUNNING (mmkv) || !MAKEMKV_IS_READY (mmkv, error))
    return FALSE;

  link = g_list_find_custom (mmkv->priv->drives, disc->priv->device,
      (GCompareFunc) makemkv_compare_drive_by_device);
  if (!link)
    return FALSE;
  drive = link->data;

  if (mmkv->priv->opened == drive->id)
    ogmbr_makemkv_close_disc (mmkv, cancellable, error);

  mmkv->priv->m_mem->args[0] = drive->id;
  if (!makemkv_run_command (mmkv, apCallEjectDisk, cancellable, error))
    return FALSE;

  return mmkv->priv->m_mem->args[0] != 0;
}

static gboolean
ogmbr_makemkv_eject_disc_thread (GIOSchedulerJob *job, GCancellable *cancellable, GSimpleAsyncResult *simple)
{
  GError *error = NULL;
  DiskAsyncData *data;
  gboolean res;

  data = g_simple_async_result_get_op_res_gpointer (simple);

  res = ogmbr_makemkv_eject_disc (data->mmkv, data->disc, cancellable, &error);
  g_simple_async_result_set_op_res_gboolean (simple, res);

  if (!res && error)
    g_simple_async_result_take_error (simple, error);

  g_simple_async_result_complete_in_idle (simple);

  return FALSE;
}

void
ogmbr_makemkv_eject_disc_async (OGMBrMakeMKV *mmkv, OGMBrDisc *disc, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  GSimpleAsyncResult *simple;

  g_return_if_fail (OGMBR_IS_MAKEMKV (mmkv));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  simple = g_simple_async_result_new (G_OBJECT (mmkv), callback, user_data, ogmbr_makemkv_eject_disc_async);
  g_simple_async_result_set_op_res_gpointer (simple, disc_async_data_new (mmkv, disc), (GDestroyNotify) disc_async_data_free);

  g_io_scheduler_push_job ((GIOSchedulerJobFunc) ogmbr_makemkv_eject_disc_thread,
      simple, g_object_unref, G_PRIORITY_DEFAULT, cancellable);
}

gboolean
ogmbr_makemkv_eject_disc_finish (OGMBrMakeMKV *mmkv, GSimpleAsyncResult *res, GError **error)
{
  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (res), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  g_warn_if_fail (g_simple_async_result_get_source_tag (res) == ogmbr_makemkv_eject_disc_async);

  if (g_simple_async_result_propagate_error (res, error))
    return FALSE;

  return g_simple_async_result_get_op_res_gboolean (res);
}

gboolean
ogmbr_makemkv_backup_disc (OGMBrMakeMKV *mmkv, OGMBrDisc *disc, const gchar *output, gboolean decrypt,
    GCancellable *cancellable, OGMBrMakeMKVProgress progress_cb, gpointer progress_data, GError **error)
{
  GList *link;
  OGMBrDrive *drive;
  gboolean retval;
  gunichar2 *utf16;
  glong len;

  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (MAKEMKV_IS_RUNNING (mmkv) || !MAKEMKV_IS_READY (mmkv, error))
    return FALSE;

  link = g_list_find_custom (mmkv->priv->drives, disc->priv->device,
      (GCompareFunc) makemkv_compare_drive_by_device);
  if (!link)
    return FALSE;
  drive = link->data;

  g_object_set_data_full (G_OBJECT (mmkv), "progress",
      progress_data_new (progress_cb, progress_data, 0.0),
      (GDestroyNotify) progress_data_free);

  mmkv->priv->m_mem->args[0] = drive->id;
  mmkv->priv->m_mem->args[1] = decrypt ? 1 : 0;

  utf16 = g_utf8_to_utf16 (output, -1, NULL, &len, NULL);
  memcpy ((void *) mmkv->priv->m_mem->strbuf, utf16, (len + 1) * sizeof (gunichar2));
  g_free (utf16);

  retval = makemkv_run_command (mmkv, apCallBackupDisc, cancellable, error);

  g_object_set_data (G_OBJECT (mmkv), "progress", NULL);

  return retval && mmkv->priv->m_mem->args[0] != 0;
}

typedef struct
{
  OGMBrMakeMKV *mmkv;
  OGMBrDisc *disc;
  gchar *folder;
  gboolean decrypt;
  OGMBrMakeMKVProgress progress_cb;
  gpointer progress_data;
  GIOSchedulerJob *job;
} BackupAsyncData;

static BackupAsyncData *
backup_async_data_new (OGMBrMakeMKV *mmkv, OGMBrDisc *disc, const gchar *folder, gboolean decrypt, OGMBrMakeMKVProgress progress_cb, gpointer progress_data)
{
  BackupAsyncData *data;

  data = g_new0 (BackupAsyncData, 1);
  data->mmkv = g_object_ref (mmkv);
  data->disc = g_object_ref (disc);
  data->folder = g_strdup (folder);
  data->decrypt = decrypt;
  data->progress_cb = progress_cb;
  data->progress_data = progress_data;

  return data;
}

static void
backup_async_data_free (BackupAsyncData *data)
{
  g_object_unref (data->mmkv);
  g_object_unref (data->disc);
  g_free (data->folder);
  g_free (data);
}

static void
ogmbr_makemkv_backup_progress_cb (OGMBrMakeMKV *mmkv, gdouble fraction, BackupAsyncData *data)
{
  g_io_scheduler_job_send_to_mainloop_async (data->job,
      (GSourceFunc) makemkv_progress_in_main_cb,
      progress_data_new (data->progress_cb, data->progress_data, fraction),
      (GDestroyNotify) progress_data_free);
}

static gboolean
ogmbr_makemkv_backup_disc_thread (GIOSchedulerJob *job, GCancellable *cancellable, GSimpleAsyncResult *simple)
{
  GError *error = NULL;
  BackupAsyncData *data;
  gboolean res;

  data = g_simple_async_result_get_op_res_gpointer (simple);
  data->job = job;

  res = ogmbr_makemkv_backup_disc (data->mmkv, data->disc, data->folder, data->decrypt, cancellable,
      data->progress_cb ? (OGMBrMakeMKVProgress) ogmbr_makemkv_backup_progress_cb : NULL, data, &error);
  g_simple_async_result_set_op_res_gboolean (simple, res);

  if (!res && error)
    g_simple_async_result_take_error (simple, error);

  g_simple_async_result_complete_in_idle (simple);

  return FALSE;
}

void
ogmbr_makemkv_backup_disc_async (OGMBrMakeMKV *mmkv, OGMBrDisc *disc, const gchar *folder, gboolean decrypt,
    GCancellable *cancellable, OGMBrMakeMKVProgress progress_cb, gpointer progress_data, GAsyncReadyCallback callback, gpointer user_data)
{
  GSimpleAsyncResult *simple;

  g_return_if_fail (OGMBR_IS_MAKEMKV (mmkv));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  simple = g_simple_async_result_new (G_OBJECT (mmkv), callback, user_data, ogmbr_makemkv_backup_disc_async);
  g_simple_async_result_set_op_res_gpointer (simple,
      backup_async_data_new (mmkv, disc, folder, decrypt, progress_cb, progress_data), (GDestroyNotify) backup_async_data_free);

  g_io_scheduler_push_job ((GIOSchedulerJobFunc) ogmbr_makemkv_backup_disc_thread,
      simple, g_object_unref, G_PRIORITY_DEFAULT, cancellable);
}

gboolean
ogmbr_makemkv_backup_disc_finish (OGMBrMakeMKV *mmkv, GSimpleAsyncResult *res, GError **error)
{
  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);
  g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (res), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  g_warn_if_fail (g_simple_async_result_get_source_tag (res) == ogmbr_makemkv_backup_disc_async);

  if (g_simple_async_result_propagate_error (res, error))
    return FALSE;

  return g_simple_async_result_get_op_res_gboolean (res);
}
/*
void
ogmbr_makemkv_set_output (OGMBrMakeMKV *mmkv, const gchar *name)
{
  gunichar2 *utf16;
  glong len;

  g_return_if_fail (OGMBR_IS_MAKEMKV (mmkv));

  utf16 = g_utf8_to_utf16 (name, -1, NULL, &len, NULL);
  memcpy ((void *) mmkv->priv->m_mem->strbuf, utf16, (len + 1) * sizeof (gunichar2));
  g_free (utf16);

  makemkv_run_command (mmkv, apCallSetOutputFolder, NULL, NULL);
}

gboolean
ogmbr_makemkv_open_file (OGMBrMakeMKV *mmkv, const gchar *name)
{
  gunichar2 *utf16;
  glong len;

  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);

  utf16 = g_utf8_to_utf16 (name, -1, NULL, &len, NULL);
  memcpy ((void *) mmkv->priv->m_mem->strbuf, utf16, (len + 1) * sizeof (gunichar2));
  g_free (utf16);

  makemkv_run_command (mmkv, apCallOpenFile, NULL, NULL);

  return mmkv->priv->m_mem->args[0] != 0;
}

gboolean
ogmbr_makemkv_open_title_collection (OGMBrMakeMKV *mmkv, const gchar *source)
{
  gunichar2 *utf16;
  glong len;

  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);

  utf16 = g_utf8_to_utf16 (source, -1, NULL, &len, NULL);
  memcpy ((void *) mmkv->priv->m_mem->strbuf, utf16, (len + 1) * sizeof (gunichar2));
  g_free (utf16);

  makemkv_run_command (mmkv, apCallOpenTitleCollection, NULL, NULL);

  return mmkv->priv->m_mem->args[0] != 0;
}

gboolean
ogmbr_makemkv_save_all_selected_titles_to_mkv (OGMBrMakeMKV *mmkv)
{
  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);

  makemkv_run_command (mmkv, apCallSaveAllSelectedTitlesToMkv, NULL, NULL);

  return mmkv->priv->m_mem->args[0] != 0;
}

gboolean
ogmbr_makemkv_start_streaming (OGMBrMakeMKV *mmkv)
{
  g_return_val_if_fail (OGMBR_IS_MAKEMKV (mmkv), FALSE);

  makemkv_run_command (mmkv, apCallStartStreaming, NULL, NULL);

  return mmkv->priv->m_mem->args[0] != 0;
}
*/

