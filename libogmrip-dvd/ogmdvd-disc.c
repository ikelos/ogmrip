/* OGMRipDvd - A DVD library for OGMRip
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/**
 * SECTION:ogmdvd-disc
 * @title: OGMDvdDisc
 * @short_description: Structure describing a video DVD
 * @include: ogmdvd-disc.h
 */

#include "ogmdvd-disc.h"
#include "ogmdvd-priv.h"
#include "ogmdvd-title.h"
#include "ogmdvd-audio.h"
#include "ogmdvd-subp.h"

#include <ogmrip-base.h>
#include <ogmrip-job.h>

#include <sys/ioctl.h>
#include <sys/types.h>

#include <fcntl.h>

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#if defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/cdio.h>
#define CDROMEJECT CDIOCEJECT
#if __FreeBSD_version < 500000
#undef MIN
#undef MAX
#include <sys/param.h>
#include <sys/mount.h>
#endif /* __FreeBSD_version */
#endif /* __FreeBSD__ */

#ifdef __linux__
#include <linux/cdrom.h>
#endif /* __linux__ */

enum
{
  PROP_0,
  PROP_URI
};

static void      ogmrip_media_iface_init  (OGMRipMediaInterface  *iface);
static GObject * ogmdvd_disc_constructor  (GType                 gtype,
                                           guint                 n_properties,
                                           GObjectConstructParam *properties);
static void      ogmdvd_disc_dispose      (GObject               *gobject);
static void      ogmdvd_disc_finalize     (GObject               *gobject);
static void      ogmdvd_disc_get_property (GObject               *gobject,
                                           guint                 prop_id,
                                           GValue                *value,
                                           GParamSpec            *pspec);
static void      ogmdvd_disc_set_property (GObject               *gobject,
                                           guint                 prop_id,
                                           const GValue          *value,
                                           GParamSpec            *pspec);
static void      ogmdvd_disc_close        (OGMRipMedia           *media);
static gboolean  ogmdvd_disc_is_open      (OGMRipMedia           *media);


static GHashTable *open_discs;

static gboolean
ogmdvd_device_tray_is_open (const gchar *device)
{
  gint fd;
#if defined(__linux__) || defined(__FreeBSD__)
  gint status = 0;
#endif
#ifdef __FreeBSD__
  struct ioc_toc_header h;
#endif

  g_return_val_if_fail (device && *device, FALSE);

  fd = g_open (device, O_RDONLY | O_NONBLOCK | O_EXCL, 0);

  if (fd < 0)
    return FALSE;

#if defined(__linux__)
  status = ioctl (fd, CDROM_DRIVE_STATUS, CDSL_CURRENT);
#elif defined(__FreeBSD__)
  status = ioctl (fd, CDIOREADTOCHEADER, &h);
#endif

  close (fd);

#if defined(__linux__)
  return status < 0 ? FALSE : status == CDS_TRAY_OPEN;
#elif defined(__FreeBSD__)
  return status < 0;
#else
  return FALSE;
#endif
}

static dvd_reader_t *
dvd_open_reader (const gchar *device, GError **error)
{
  dvd_reader_t *reader;

  reader = DVDOpen (device);
  if (!reader)
  {
    struct stat buf;

    if (g_stat (device, &buf))
      g_set_error (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_EXIST, _("No such file or directory"));
    else
    {
      if (access (device, R_OK) < 0)
        g_set_error (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_PERM, _("Permission denied to access device"));
      else
      {
        if (S_ISBLK(buf.st_mode))
        {
          if (ogmdvd_device_tray_is_open (device))
            g_set_error (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_TRAY, _("Tray seems to be open"));
          else
            g_set_error (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_DEV, _("Device does not contain a valid DVD video"));
        }
        else if (S_ISDIR(buf.st_mode) || S_ISREG(buf.st_mode))
          g_set_error (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_PATH, _("Path does not contain a valid DVD structure"));
        else
          g_set_error (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_ACCESS, _("No such directory, block device or iso file"));
      }
    }
  }
  return reader;
}

static const gchar *
dvd_reader_get_id (dvd_reader_t *reader)
{
  static gchar str[33];
  guchar id[16];
  gint i;

  if (DVDDiscID (reader, id) < 0)
    return NULL;

  for (i = 0; i < 16; i++)
    sprintf (str + 2 * i, "%02X", id[i]);
  str[32] = '\0';

  return str;
}

static gint64
dvd_reader_get_ifo_size (dvd_reader_t *reader, guint vts)
{
  dvd_file_t *file;
  gint size;

  file = DVDOpenFile (reader, vts, DVD_READ_INFO_FILE);
  size = DVDFileSize (file) * DVD_VIDEO_LB_LEN;
  DVDCloseFile (file);

  if (size < 0)
    return 0;

  return (gint64) size;
}

static gint64
dvd_reader_get_bup_size (dvd_reader_t *reader, guint vts)
{
  dvd_file_t *file;
  gint size;

  file = DVDOpenFile (reader, vts, DVD_READ_INFO_BACKUP_FILE);
  size = DVDFileSize (file) * DVD_VIDEO_LB_LEN;
  DVDCloseFile (file);

  if (size < 0)
    return 0;

  return (gint64) size;
}

static gint64
dvd_reader_get_menu_size (dvd_reader_t *reader, guint vts)
{
  dvd_file_t *file;
  gint size;

  file = DVDOpenFile (reader, vts, DVD_READ_MENU_VOBS);
  size = DVDFileSize (file) * DVD_VIDEO_LB_LEN;
  DVDCloseFile (file);

  if (size < 0)
    return 0;

  return (gint64) size;
}

static gint64
dvd_reader_get_vob_size (dvd_reader_t *reader, guint vts)
{
  gint64 fullsize;

  dvd_file_t *file;

  file = DVDOpenFile (reader, vts, DVD_READ_TITLE_VOBS);
  fullsize = DVDFileSize (file) * DVD_VIDEO_LB_LEN;
  DVDCloseFile (file);

  if (fullsize < 0)
    return 0;

  return fullsize;
}

static guint64
dvd_reader_get_vts_size (dvd_reader_t *reader, guint vts)
{
  guint64 size, fullsize;

  fullsize  = dvd_reader_get_ifo_size  (reader, vts);
  fullsize += dvd_reader_get_bup_size  (reader, vts);
  fullsize += dvd_reader_get_menu_size (reader, vts);

  if (vts > 0)
  {
    if ((size = dvd_reader_get_vob_size (reader, vts)) == 0)
      return 0;
    fullsize += size;
  }

  return fullsize;
}

static OGMDvdDisc *
ogmdvd_disc_get_open (const gchar *device)
{
  if (!open_discs)
    return NULL;

  return g_hash_table_lookup (open_discs, device);
}

static gchar *
ogmdvd_disc_get_title_name (OGMDvdDisc *disc)
{
  FILE *f = 0;
  gchar label[33];
  int  i;

  f = fopen (disc->priv->device, "r");
  if (!f)
    return g_strdup ("Unknown");


  if (fseek (f, 32808, SEEK_SET) < 0)
  {
    fclose (f);
    return g_strdup ("Unknown");
  }

  if (32 != (i = fread (label, 1, 32, f)))
  {
    fclose (f);
    return g_strdup ("Unknown");
  }

  fclose (f);

  label[32] = '\0';
  while (i-- > 2)
  {
    if (label[i] == ' ')
      label[i] = '\0';
  }

  return g_convert (label, -1, "UTF8", "LATIN1", NULL, NULL, NULL);;
}

static OGMDvdAudioStream *
ogmdvd_audio_stream_new (OGMDvdTitle *title, ifo_handle_t *vts_file, guint nr, guint real_nr)
{
  OGMDvdAudioStream *stream;
  audio_attr_t *attr;
  guint16 pgcn;

  stream = g_object_new (OGMDVD_TYPE_AUDIO_STREAM, NULL);
  stream->priv->title = OGMRIP_TITLE (title);

  g_object_add_weak_pointer (G_OBJECT (title), (gpointer *) &stream->priv->title);

  attr = &vts_file->vtsi_mat->vts_audio_attr[real_nr];
  stream->priv->format = attr->audio_format;
  stream->priv->channels = attr->channels;
  stream->priv->quantization = attr->quantization;
  stream->priv->code_extension = attr->code_extension;
  stream->priv->lang_code = attr->lang_code;

  pgcn = vts_file->vts_ptt_srpt->title[title->priv->ttn - 1].ptt[0].pgcn;
  stream->priv->id = vts_file->vts_pgcit->pgci_srp[pgcn - 1].pgc->audio_control[real_nr] >> 8 & 7;

  return stream;
}

static OGMDvdSubpStream *
ogmdvd_subp_stream_new (OGMDvdTitle *title, ifo_handle_t *vts_file, guint nr, guint real_nr)
{
  OGMDvdSubpStream *stream;
  subp_attr_t *attr;
  guint16 pgcn;

  stream = g_object_new (OGMDVD_TYPE_SUBP_STREAM, NULL);
  stream->priv->title = OGMRIP_TITLE (title);

  g_object_add_weak_pointer (G_OBJECT (title), (gpointer *) &stream->priv->title);

  attr = &vts_file->vtsi_mat->vts_subp_attr[real_nr];
  stream->priv->lang_extension = attr->lang_extension;
  stream->priv->lang_code = attr->lang_code;

  pgcn = vts_file->vts_ptt_srpt->title[title->priv->ttn - 1].ptt[0].pgcn;

  stream->priv->id = nr;
  if (title->priv->display_aspect_ratio == 0) /* 4:3 */
    stream->priv->id = vts_file->vts_pgcit->pgci_srp[pgcn - 1].pgc->subp_control[real_nr] >> 24 & 31;
  else if (title->priv->display_aspect_ratio == 3) /* 16:9 */
    stream->priv->id = vts_file->vts_pgcit->pgci_srp[pgcn - 1].pgc->subp_control[real_nr] >> 8 & 31;

  return stream;
}

static gulong
ogmdvd_title_get_chapter_length (OGMDvdTitle *title, ifo_handle_t *vts_file, guint nr)
{
  glong total = 0;
  guint8 first_cell, last_cell;
  guint16 pgcn, pgn;
  pgc_t *pgc;

  pgcn = vts_file->vts_ptt_srpt->title[title->priv->ttn - 1].ptt[nr].pgcn;
  pgn = vts_file->vts_ptt_srpt->title[title->priv->ttn - 1].ptt[nr].pgn;
  pgc = vts_file->vts_pgcit->pgci_srp[pgcn - 1].pgc;

  first_cell = pgc->program_map[pgn - 1];
  if (pgn < pgc->nr_of_programs)
    last_cell = pgc->program_map[pgn];
  else
    last_cell = 0;

  do
  {
    if (pgc->cell_playback[first_cell - 1].block_type != BLOCK_TYPE_ANGLE_BLOCK ||
        pgc->cell_playback[first_cell - 1].block_type == BLOCK_MODE_FIRST_CELL)
      total += ogmdvd_time_to_msec (&pgc->cell_playback[first_cell - 1].playback_time);

    first_cell ++;
  }
  while (first_cell < last_cell);

  return total;
}

static OGMDvdTitle *
ogmdvd_title_new (OGMDvdDisc *disc, dvd_reader_t *reader, ifo_handle_t *vmg_file, guint nr)
{
  OGMDvdTitle *title;

  ifo_handle_t *vts_file;
  pgc_t *pgc;

  guint16 pgcn;
  guint8 i;

  vts_file = ifoOpen (reader, vmg_file->tt_srpt->title[nr].title_set_nr);
  if (!vts_file)
    return NULL;

  title = g_object_new (OGMDVD_TYPE_TITLE, NULL);
  title->priv->disc = OGMRIP_MEDIA (disc);
  title->priv->ttn = vmg_file->tt_srpt->title[nr].vts_ttn;
  title->priv->nr = nr;

  g_object_add_weak_pointer (G_OBJECT (disc), (gpointer *) &title->priv->disc);

  pgcn = vts_file->vts_ptt_srpt->title[title->priv->ttn - 1].ptt[0].pgcn;
  pgc = vts_file->vts_pgcit->pgci_srp[pgcn - 1].pgc;

  memcpy (title->priv->palette, pgc->palette, 16 * sizeof (uint32_t));
  title->priv->playback_time = pgc->playback_time;

  title->priv->nr_of_chapters = vts_file->vts_ptt_srpt->title[title->priv->ttn - 1].nr_of_ptts;
  title->priv->nr_of_angles = vmg_file->tt_srpt->title[nr].nr_of_angles;
  title->priv->title_set_nr = vmg_file->tt_srpt->title[nr].title_set_nr;
  title->priv->vts_size = dvd_reader_get_vts_size (reader, title->priv->title_set_nr);

  title->priv->video_format = vts_file->vtsi_mat->vts_video_attr.video_format;
  title->priv->picture_size = vts_file->vtsi_mat->vts_video_attr.picture_size;
  title->priv->display_aspect_ratio = vts_file->vtsi_mat->vts_video_attr.display_aspect_ratio;
  title->priv->permitted_df = vts_file->vtsi_mat->vts_video_attr.permitted_df;

  ogmrip_video_stream_get_resolution (OGMRIP_VIDEO_STREAM (title), &title->priv->crop_w, &title->priv->crop_h);

  for (i = 0; i < vts_file->vtsi_mat->nr_of_vts_audio_streams; i++)
  {
    if (vts_file->vts_pgcit->pgci_srp[pgcn - 1].pgc->audio_control[i] & 0x8000)
    {
      OGMDvdAudioStream *stream;

      stream = ogmdvd_audio_stream_new (title, vts_file, title->priv->nr_of_audio_streams, i);
      if (stream)
        title->priv->audio_streams = g_list_append (title->priv->audio_streams, stream);
      title->priv->nr_of_audio_streams ++;
    }
  }

  for (i = 0; i < vts_file->vtsi_mat->nr_of_vts_subp_streams; i++)
  {
    if (vts_file->vts_pgcit->pgci_srp[pgcn - 1].pgc->subp_control[i] & 0x80000000)
    {
      OGMDvdSubpStream *stream;

      stream = ogmdvd_subp_stream_new (title, vts_file, title->priv->nr_of_subp_streams, i);
      if (stream)
        title->priv->subp_streams = g_list_append (title->priv->subp_streams, stream);
      title->priv->nr_of_subp_streams ++;
    }
  }

  title->priv->length_of_chapters = g_new0 (gulong, title->priv->nr_of_chapters);
  for (i = 0; i < title->priv->nr_of_chapters; i ++)
    title->priv->length_of_chapters[i] = ogmdvd_title_get_chapter_length (title, vts_file, i);

  ifoClose (vts_file);

  return title;
}

G_DEFINE_TYPE_WITH_CODE (OGMDvdDisc, ogmdvd_disc, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_MEDIA, ogmrip_media_iface_init));

static void
ogmdvd_disc_init (OGMDvdDisc *disc)
{
  disc->priv = G_TYPE_INSTANCE_GET_PRIVATE (disc, OGMDVD_TYPE_DISC, OGMDvdDiscPriv);
}

static void
ogmdvd_disc_class_init (OGMDvdDiscClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructor = ogmdvd_disc_constructor;
  gobject_class->dispose = ogmdvd_disc_dispose;
  gobject_class->finalize = ogmdvd_disc_finalize;
  gobject_class->set_property = ogmdvd_disc_set_property;
  gobject_class->get_property = ogmdvd_disc_get_property;

  g_object_class_override_property (gobject_class, PROP_URI, "uri");

  g_type_class_add_private (klass, sizeof (OGMDvdDiscPriv));
}

static GObject *
ogmdvd_disc_constructor (GType gtype, guint n_properties, GObjectConstructParam *properties)
{
  GObject *gobject;
  OGMDvdDisc *disc, *old;
  OGMDvdTitle *title;
  dvd_reader_t *reader;
  ifo_handle_t *vmg_file;

  const gchar *id;
  guint nr;

  gobject = G_OBJECT_CLASS (ogmdvd_disc_parent_class)->constructor (gtype, n_properties, properties);

  disc = OGMDVD_DISC (gobject);

  if (!disc->priv->uri)
    disc->priv->uri = g_strdup ("dvd:///dev/dvd");

  if (!g_str_has_prefix (disc->priv->uri, "dvd://"))
  {
    g_object_unref (disc);
    return NULL;
  }

  disc->priv->device = g_strdup (disc->priv->uri + 6);

  reader = dvd_open_reader (disc->priv->device, NULL);
  if (!reader)
  {
    g_object_unref (disc);
    return NULL;
  }
  id = dvd_reader_get_id (reader);
  if (!id)
    return NULL;

  old = ogmdvd_disc_get_open (disc->priv->device);
  if (old)
  {
    gchar *old_id = old->priv->real_id ? old->priv->real_id : old->priv->id;

    if (g_str_equal (old_id, id))
    {
      DVDClose (reader);
      g_object_ref (old);
      g_object_unref (disc);
      return G_OBJECT (old);
    }

    ogmdvd_disc_close (OGMRIP_MEDIA (old));
  }

  vmg_file = ifoOpen (reader, 0);
  if (!vmg_file)
  {
    g_object_unref (disc);
    return NULL;
  }

  disc->priv->id = g_strdup (id);
  disc->priv->label = ogmdvd_disc_get_title_name (disc);

  disc->priv->ntitles = vmg_file->tt_srpt->nr_of_srpts;
  for (nr = 0; nr < disc->priv->ntitles; nr ++)
  {
    title = ogmdvd_title_new (disc, reader, vmg_file, nr);
    if (title)
      disc->priv->titles = g_list_append (disc->priv->titles, title);
  }

  disc->priv->vmg_size = dvd_reader_get_vts_size (reader, 0);

  ifoClose (vmg_file);
  DVDClose (reader);

  if (!disc->priv->titles)
  {
    g_object_unref (disc);
    return NULL;
  }

  return gobject;
}

static void
ogmdvd_disc_dispose (GObject *gobject)
{
  OGMDvdDisc *disc = OGMDVD_DISC (gobject);

  if (disc->priv->titles)
  {
    g_list_foreach (disc->priv->titles, (GFunc) g_object_unref, NULL);
    g_list_free (disc->priv->titles);
    disc->priv->titles = NULL;
  }

  G_OBJECT_CLASS (ogmdvd_disc_parent_class)->dispose (gobject);
}

static void
ogmdvd_disc_finalize (GObject *gobject)
{
  OGMDvdDisc *disc = OGMDVD_DISC (gobject);

#ifdef G_ENABLE_DEBUG
  g_message ("Finalizing %s", G_OBJECT_TYPE_NAME (gobject));
#endif

  ogmdvd_disc_close (OGMRIP_MEDIA (disc));

  g_free (disc->priv->uri);
  g_free (disc->priv->device);
  g_free (disc->priv->label);
  g_free (disc->priv->id);
  g_free (disc->priv->real_id);

  G_OBJECT_CLASS (ogmdvd_disc_parent_class)->finalize (gobject);
}

static void
ogmdvd_disc_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  OGMDvdDisc *disc = OGMDVD_DISC (gobject);

  switch (prop_id)
  {
    case PROP_URI:
      g_value_set_string (value, disc->priv->uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmdvd_disc_set_uri (OGMDvdDisc *disc, const gchar *str)
{
  gchar *name, *path;

  name = g_path_get_basename (str);
  if (g_ascii_strcasecmp (name, "video_ts") != 0)
    path = g_strdup (str);
  else
    path = g_path_get_dirname (str);
  g_free (name);

  if (g_str_has_prefix (path, "dvd://"))
    disc->priv->uri = g_strdup (path);
  else
    disc->priv->uri = g_strdup_printf ("dvd://%s", path);

  g_free (path);
}

static void
ogmdvd_disc_set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  OGMDvdDisc *disc = OGMDVD_DISC (gobject);

  switch (prop_id)
  {
    case PROP_URI:
      ogmdvd_disc_set_uri (disc, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static gboolean
ogmdvd_disc_open (OGMRipMedia *media, GCancellable *cancellable, OGMRipMediaCallback callback, gpointer user_data, GError **error)
{
  OGMDvdDisc *disc = OGMDVD_DISC (media);
  dvd_reader_t *reader;
  const gchar *id, *current_id;

  if (ogmdvd_disc_is_open (media))
    return TRUE;

  ogmdvd_disc_close (media);

  reader = dvd_open_reader (disc->priv->device, error);
  if (!reader)
    return FALSE;

  id = disc->priv->real_id ? disc->priv->real_id : disc->priv->id;
  current_id = dvd_reader_get_id (reader);

  if (!current_id || !g_str_equal (id, current_id))
  {
    DVDClose (reader);
    g_set_error (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_ID,
        _("Device does not contain the expected DVD"));
    return FALSE;
  }

  disc->priv->reader = reader;
  disc->priv->vmg_file = ifoOpen (disc->priv->reader, 0);

  if (!open_discs)
    open_discs = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_insert (open_discs, disc->priv->device, disc);

  return TRUE;
}

static void
ogmdvd_disc_close (OGMRipMedia *media)
{
  OGMDvdDisc *disc = OGMDVD_DISC (media);

  if (disc->priv->vmg_file)
  {
    ifoClose (disc->priv->vmg_file);
    disc->priv->vmg_file = NULL;
  }

  if (disc->priv->reader)
  {
    DVDClose (disc->priv->reader);
    disc->priv->reader = NULL;
  }

  if (open_discs)
    g_hash_table_remove (open_discs, disc->priv->device);
}

static gboolean
ogmdvd_disc_is_open (OGMRipMedia *media)
{
  return OGMDVD_DISC (media)->priv->reader != NULL;
}

static const gchar *
ogmdvd_disc_get_label (OGMRipMedia *media)
{
  return OGMDVD_DISC (media)->priv->label;
}

static const gchar *
ogmdvd_disc_get_id (OGMRipMedia *media)
{
  return OGMDVD_DISC (media)->priv->id;
}

static const gchar *
ogmdvd_disc_get_uri (OGMRipMedia *media)
{
  return OGMDVD_DISC (media)->priv->uri;
}

static gint64
ogmdvd_disc_get_vmg_size (OGMRipMedia *media)
{
  return OGMDVD_DISC (media)->priv->vmg_size;
}

static gint
ogmdvd_disc_get_n_titles (OGMRipMedia *media)
{
  return OGMDVD_DISC (media)->priv->ntitles;
}

static OGMRipTitle *
ogmdvd_disc_get_title (OGMRipMedia *media, guint id)
{
  GList *link;

  for (link = OGMDVD_DISC (media)->priv->titles; link; link = link->next)
  {
    OGMDvdTitle *title = link->data;

    if (title->priv->nr == id)
      return link->data;
  }

  return NULL;
}

static GList *
ogmdvd_disc_get_titles (OGMRipMedia *media)
{
  return g_list_copy (OGMDVD_DISC (media)->priv->titles);
}

OGMRipMedia *
ogmdvd_disc_copy_internal (OGMRipMedia *media, const gchar *path, GCancellable *cancellable,
    OGMRipMediaCallback callback, gpointer user_data, GError **error)
{
  return (OGMRipMedia *) ogmdvd_disc_copy (OGMDVD_DISC (media), NULL, path, cancellable, callback, user_data, error);
}

static void
ogmrip_media_iface_init (OGMRipMediaInterface *iface)
{
  iface->open          = ogmdvd_disc_open;
  iface->close         = ogmdvd_disc_close;
  iface->is_open       = ogmdvd_disc_is_open;
  iface->get_label     = ogmdvd_disc_get_label;
  iface->get_id        = ogmdvd_disc_get_id;
  iface->get_uri       = ogmdvd_disc_get_uri;
  iface->get_size      = ogmdvd_disc_get_vmg_size;
  iface->get_n_titles  = ogmdvd_disc_get_n_titles;
  iface->get_title     = ogmdvd_disc_get_title;
  iface->get_titles    = ogmdvd_disc_get_titles;
  iface->copy          = ogmdvd_disc_copy_internal;
}

/**
 * ogmdvd_error_quark:
 *
 * The function description goes here.
 *
 * Returns: the #GQuark
 */
GQuark
ogmdvd_error_quark (void)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_static_string ("ogmdvd-error-quark");

  return quark;
}

/**
 * ogmdvd_disc_new:
 * @device: A DVD device, or NULL to use /dev/dvd.
 * @error: Location to store the error occuring, or NULL to ignore errors.
 *
 * Creates a new #OGMDvdDisc for the given a DVD device.
 *
 * Returns: The new #OGMDvdDisc, or NULL
 */
OGMRipMedia *
ogmdvd_disc_new (const gchar *device, GError **error)
{
  OGMRipMedia *media;
  gchar *uri;

  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  uri = g_strdup_printf ("dvd://%s", device);
  media = g_object_new (OGMDVD_TYPE_DISC, "uri", uri, NULL);
  g_free (uri);

  return media;
}

void
ogmrip_dvd_register_media (void)
{
  OGMRipTypeInfo *info;

  info = g_object_new (OGMRIP_TYPE_TYPE_INFO, "name", "DVD", "description", "DVD", NULL);
  ogmrip_type_register (OGMDVD_TYPE_DISC, info);
}

