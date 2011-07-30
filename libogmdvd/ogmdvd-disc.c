/* OGMDvd - A wrapper library around libdvdread
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
#include "ogmdvd-enums.h"
#include "ogmdvd-title.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

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

#ifndef HAVE_DVD_FILE_SIZE
uint32_t UDFFindFile( dvd_reader_t *device, char *filename, uint32_t *size );
#endif

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
#ifdef HAVE_DVD_FILE_SIZE
  dvd_file_t *file;
  gint size;

  file = DVDOpenFile (reader, vts, DVD_READ_INFO_FILE);
  size = DVDFileSize (file);
  DVDCloseFile (file);

  size *= DVD_VIDEO_LB_LEN;
#else /* HAVE_DVD_FILE_SIZE */
  gchar filename[FILENAME_MAX];
  guint size;

  if (vts == 0)
    strncpy (filename, "/VIDEO_TS/VIDEO_TS.IFO", FILENAME_MAX);
  else
    snprintf (filename, FILENAME_MAX, "/VIDEO_TS/VTS_%02u_0.IFO", vts);

  if (!UDFFindFile (reader, filename, &size))
    return -1;
#endif /* HAVE_DVD_FILE_SIZE */

  if (size < 0)
    return 0;

  return (gint64) size;
}

static gint64
dvd_reader_get_bup_size (dvd_reader_t *reader, guint vts)
{
#ifdef HAVE_DVD_FILE_SIZE
  dvd_file_t *file;
  gint size;

  file = DVDOpenFile (reader, vts, DVD_READ_INFO_BACKUP_FILE);
  size = DVDFileSize (file);
  DVDCloseFile (file);

  size *= DVD_VIDEO_LB_LEN;
#else /* HAVE_DVD_FILE_SIZE */
  gchar filename[FILENAME_MAX];
  guint size;

  if (vts == 0)
    strncpy (filename, "/VIDEO_TS/VIDEO_TS.BUP", FILENAME_MAX);
  else
    snprintf (filename, FILENAME_MAX, "/VIDEO_TS/VTS_%02u_0.BUP", vts);

  if (!UDFFindFile (reader, filename, &size))
    return 0;
#endif /* HAVE_DVD_FILE_SIZE */

  if (size < 0)
    return 0;

  return (gint64) size;
}

static gint64
dvd_reader_get_menu_size (dvd_reader_t *reader, guint vts)
{
#ifdef HAVE_DVD_FILE_SIZE
  dvd_file_t *file;
  gint size;

  file = DVDOpenFile (reader, vts, DVD_READ_MENU_VOBS);
  size = DVDFileSize (file);
  DVDCloseFile (file);

  size *= DVD_VIDEO_LB_LEN;
#else /* HAVE_DVD_FILE_SIZE */
  gchar filename[FILENAME_MAX];
  guint size;

  if (vts == 0)
    strncpy (filename, "/VIDEO_TS/VIDEO_TS.VOB", FILENAME_MAX);
  else
    snprintf (filename, FILENAME_MAX, "/VIDEO_TS/VTS_%02u_0.VOB", vts);

  if (!UDFFindFile (reader, filename, &size))
    return 0;
#endif /* HAVE_DVD_FILE_SIZE */

  if (size < 0)
    return 0;

  return (gint64) size;
}

static gint64
dvd_reader_get_vob_size (dvd_reader_t *reader, guint vts)
{
  gint64 fullsize;

#ifdef HAVE_DVD_FILE_SIZE
  dvd_file_t *file;

  file = DVDOpenFile (reader, vts, DVD_READ_TITLE_VOBS);
  fullsize = DVDFileSize (file);
  DVDCloseFile (file);

  fullsize *= DVD_VIDEO_LB_LEN;
#else /* HAVE_DVD_FILE_SIZE */
  gchar filename[FILENAME_MAX];
  guint vob, size;

  if (vts == 0)
    return 0;

  vob = 1; fullsize = 0;
  while (1)
  {
    snprintf (filename, FILENAME_MAX, "/VIDEO_TS/VTS_%02u_%u.VOB", vts, vob++);
    if (!UDFFindFile (reader, filename, &size))
      break;
    fullsize += size;
  }

  if (vob == 1)
    return 0;
#endif /* HAVE_DVD_FILE_SIZE */

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

static OGMDvdVideoStream *
ogmdvd_video_stream_new (OGMDvdTitle *title, ifo_handle_t *vts_file)
{
  OGMDvdVideoStream *stream;
  video_attr_t *attr;

  stream = g_new0 (OGMDvdVideoStream, 1);
  stream->stream.title = title;
  stream->stream.id = 1;
  stream->stream.nr = 1;

  attr = &vts_file->vtsi_mat->vts_video_attr;
  stream->video_format = attr->video_format;
  stream->picture_size = attr->picture_size;
  stream->display_aspect_ratio = attr->display_aspect_ratio;
  stream->permitted_df = attr->permitted_df;

  return stream;
}

static OGMDvdStream *
ogmdvd_audio_stream_new (OGMDvdTitle *title, ifo_handle_t *vts_file, guint nr, guint real_nr)
{
  OGMDvdAudioStream *stream;

  audio_attr_t *attr;

  stream = g_new0 (OGMDvdAudioStream, 1);
  stream->stream.title = title;
  stream->stream.nr = nr;

  attr = &vts_file->vtsi_mat->vts_audio_attr[real_nr];
  stream->format = attr->audio_format;
  stream->channels = attr->channels;
  stream->quantization = attr->quantization;
  stream->code_extension = attr->code_extension;
  stream->lang_code = attr->lang_code;

  stream->stream.id = vts_file->vts_pgcit->pgci_srp[title->ttn - 1].pgc->audio_control[real_nr] >> 8 & 7;

  return (OGMDvdStream *) stream;
}

static OGMDvdStream *
ogmdvd_subp_stream_new (OGMDvdTitle *title, ifo_handle_t *vts_file, guint nr, guint real_nr)
{
  OGMDvdSubpStream *stream;
  subp_attr_t *attr;

  stream = g_new0 (OGMDvdSubpStream, 1);
  stream->stream.title = title;
  stream->stream.nr = nr;

  attr = &vts_file->vtsi_mat->vts_subp_attr[real_nr];
  stream->lang_extension = attr->lang_extension;
  stream->lang_code = attr->lang_code;

  stream->stream.id = nr;
  if (title->video_stream->display_aspect_ratio == 0) /* 4:3 */
    stream->stream.id = vts_file->vts_pgcit->pgci_srp[title->ttn - 1].pgc->subp_control[real_nr] >> 24 & 31;
  else if (title->video_stream->display_aspect_ratio == 3) /* 16:9 */
    stream->stream.id = vts_file->vts_pgcit->pgci_srp[title->ttn - 1].pgc->subp_control[real_nr] >> 8 & 31;

  return (OGMDvdStream *) stream;
}

static gulong
ogmdvd_title_get_chapter_length (OGMDvdTitle *title, ifo_handle_t *vts_file, guint nr)
{
  glong total = 0;
  guint8 first_cell, last_cell;
  guint16 pgcn, pgn;
  pgc_t *pgc;

  pgcn = vts_file->vts_ptt_srpt->title[title->ttn - 1].ptt[nr].pgcn;
  pgn = vts_file->vts_ptt_srpt->title[title->ttn - 1].ptt[nr].pgn;
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
  OGMDvdStream *stream;

  ifo_handle_t *vts_file;
  pgc_t *pgc;

  guint16 pgcn;
  guint8 i;

  vts_file = ifoOpen (reader, vmg_file->tt_srpt->title[nr].title_set_nr);
  if (!vts_file)
    return NULL;

  title = g_new0 (OGMDvdTitle, 1);
  title->disc = disc;
  title->ttn = vmg_file->tt_srpt->title[nr].vts_ttn;
  title->nr = nr;

  pgcn = vts_file->vts_ptt_srpt->title[title->ttn - 1].ptt[0].pgcn;
  pgc = vts_file->vts_pgcit->pgci_srp[pgcn - 1].pgc;

  memcpy (title->palette, pgc->palette, 16 * sizeof (uint32_t));
  title->playback_time = pgc->playback_time;

  title->nr_of_chapters = vts_file->vts_ptt_srpt->title[title->ttn - 1].nr_of_ptts;
  title->nr_of_angles = vmg_file->tt_srpt->title[nr].nr_of_angles;
  title->title_set_nr = vmg_file->tt_srpt->title[nr].title_set_nr;
  title->vts_size = dvd_reader_get_vts_size (reader, title->title_set_nr);

  title->video_stream = ogmdvd_video_stream_new (title, vts_file);

  for (i = 0; i < vts_file->vtsi_mat->nr_of_vts_audio_streams; i++)
  {
    if (vts_file->vts_pgcit->pgci_srp[title->ttn - 1].pgc->audio_control[i] & 0x8000)
    {
      stream = ogmdvd_audio_stream_new (title, vts_file, title->nr_of_audio_streams, i);
      if (stream)
        title->audio_streams = g_slist_append (title->audio_streams, stream);
      title->nr_of_audio_streams ++;
    }
  }

  for (i = 0; i < vts_file->vtsi_mat->nr_of_vts_subp_streams; i++)
  {
    if (vts_file->vts_pgcit->pgci_srp[title->ttn - 1].pgc->subp_control[i] & 0x80000000)
    {
      stream = ogmdvd_subp_stream_new (title, vts_file, title->nr_of_subp_streams, i);
      if (stream)
        title->subp_streams = g_slist_append (title->subp_streams, stream);
      title->nr_of_subp_streams ++;
    }
  }

  title->length_of_chapters = g_new0 (gulong, title->nr_of_chapters);
  for (i = 0; i < title->nr_of_chapters; i ++)
    title->length_of_chapters[i] = ogmdvd_title_get_chapter_length (title, vts_file, i);

  ifoClose (vts_file);

  return title;
}

static void
ogmdvd_title_free (OGMDvdTitle *title)
{
  ogmdvd_title_close (title);

  if (title->bitrates)
  {
    g_free (title->bitrates);
    title->bitrates = NULL;
  }

  if (title->audio_streams)
  {
    g_slist_foreach (title->audio_streams, (GFunc) g_free, NULL);
    g_slist_free (title->audio_streams);
    title->audio_streams = NULL;
  }

  if (title->subp_streams)
  {
    g_slist_foreach (title->subp_streams, (GFunc) g_free, NULL);
    g_slist_free (title->subp_streams);
    title->subp_streams = NULL;
  }

  if (title->length_of_chapters)
  {
    g_free (title->length_of_chapters);
    title->length_of_chapters = NULL;
  }

  title->disc = NULL;

  g_free (title);
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

  f = fopen (disc->device, "r");
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

static void
ogmdvd_disc_free (OGMDvdDisc *disc)
{
  ogmdvd_disc_close (disc);

  if (disc->titles)
  {
    g_slist_foreach (disc->titles, (GFunc) ogmdvd_title_free, NULL);
    g_slist_free (disc->titles);
    disc->titles = NULL;
  }

  if (disc->device)
  {
    g_free (disc->device);
    disc->device = NULL;
  }

  if (disc->label)
  {
    g_free (disc->label);
    disc->label = NULL;
  }

  if (disc->id)
  {
    g_free (disc->id);
    disc->id = NULL;
  }

  g_free (disc);
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
OGMDvdDisc *
ogmdvd_disc_new (const gchar *device, GError **error)
{
  OGMDvdDisc *disc;
  OGMDvdTitle *title;
  dvd_reader_t *reader;
  ifo_handle_t *vmg_file;

  const gchar *id;
  guint nr;

  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (!device || !*device)
    device = "/dev/dvd";

  reader = dvd_open_reader (device, error);
  if (!reader)
    return NULL;
  id = dvd_reader_get_id (reader);

  disc = ogmdvd_disc_get_open (device);
  if (disc)
  {
    if (g_str_equal (disc->id, id))
    {
      DVDClose (reader);
      ogmdvd_disc_ref (disc);
      return disc;
    }

    ogmdvd_disc_close (disc);
  }

  vmg_file = ifoOpen (reader, 0);
  if (!vmg_file)
  {
    g_set_error (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_VMG, _("Cannot open video manager"));
    return NULL;
  }

  disc = g_new0 (OGMDvdDisc, 1);

  disc->ref = 1;

  disc->device = g_strdup (device);
  disc->id = id ? g_strdup (id) : NULL;

  disc->label = ogmdvd_disc_get_title_name (disc);

  disc->ntitles = vmg_file->tt_srpt->nr_of_srpts;
  for (nr = 0; nr < disc->ntitles; nr ++)
  {
    title = ogmdvd_title_new (disc, reader, vmg_file, nr);
    if (title)
      disc->titles = g_slist_append (disc->titles, title);
  }

  disc->vmg_size = dvd_reader_get_vts_size (reader, 0);

  ifoClose (vmg_file);
  DVDClose (reader);

  if (!disc->titles)
  {
    ogmdvd_disc_unref (disc);
    return NULL;
  }

  return disc;
}

/**
 * ogmdvd_disc_open:
 * @disc: A #OGMDvdDisc
 * @error: Location to store the error occuring, or NULL to ignore errors.
 *
 * Opens @disc.
 *
 * Returns: #TRUE on success
 */
gboolean
ogmdvd_disc_open (OGMDvdDisc *disc, GError **error)
{
  dvd_reader_t *reader;

  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (ogmdvd_disc_is_open (disc))
    return TRUE;

  ogmdvd_disc_close (disc);

  reader = dvd_open_reader (disc->device, error);
  if (!reader)
    return FALSE;

  if (disc->id && !g_str_equal (disc->id, dvd_reader_get_id (reader)))
  {
    DVDClose (reader);
    g_set_error (error, OGMDVD_DISC_ERROR, OGMDVD_DISC_ERROR_ID, _("Device does not contain the expected DVD"));
    return FALSE;
  }

  disc->reader = reader;
  disc->vmg_file = ifoOpen (disc->reader, 0);

  if (!open_discs)
    open_discs = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_insert (open_discs, disc->device, disc);

  return TRUE;
}

/**
 * ogmdvd_disc_close:
 * @disc: A #OGMDvdDisc
 *
 * Closes @disc.
 */
void
ogmdvd_disc_close (OGMDvdDisc *disc)
{
  g_return_if_fail (disc != NULL);

  if (disc->vmg_file)
  {
    ifoClose (disc->vmg_file);
    disc->vmg_file = NULL;
  }

  if (disc->reader)
  {
    DVDClose (disc->reader);
    disc->reader = NULL;
  }

  if (open_discs)
    g_hash_table_remove (open_discs, disc->device);
}

/**
 * ogmdvd_disc_is_open:
 * @disc: A #OGMDvdDisc
 *
 * Returns whether the DVD device is open or not.
 *
 * Returns: #TRUE if the DVD device is open
 */
gboolean
ogmdvd_disc_is_open (OGMDvdDisc *disc)
{
  g_return_val_if_fail (disc != NULL, FALSE);

  return disc->reader != NULL;
}

/**
 * ogmdvd_disc_ref:
 * @disc: An #OGMDvdDisc
 *
 * Increments the reference count of an #OGMDvdDisc.
 */
void
ogmdvd_disc_ref (OGMDvdDisc *disc)
{
  g_return_if_fail (disc != NULL);

  disc->ref ++;
}

/**
 * ogmdvd_disc_unref:
 * @disc: An #OGMDvdDisc
 *
 * Decrements the reference count of an #OGMDvdDisc.
 */
void
ogmdvd_disc_unref (OGMDvdDisc *disc)
{
  g_return_if_fail (disc != NULL);

  if (disc->ref > 0)
  {
    disc->ref --;
    if (disc->ref == 0)
      ogmdvd_disc_free (disc);
  }
}

/**
 * ogmdvd_disc_get_label:
 * @disc: An #OGMDvdDisc
 *
 * Returns the label of the DVD.
 *
 * Returns: The label of the DVD, or NULL
 */
const gchar *
ogmdvd_disc_get_label (OGMDvdDisc *disc)
{
  g_return_val_if_fail (disc != NULL, NULL);

  return disc->label;
}

/**
 * ogmdvd_disc_get_id:
 * @disc: An #OGMDvdDisc
 *
 * Returns a unique 128 bit disc identifier.
 *
 * Returns: The identifier, or NULL
 */
const gchar *
ogmdvd_disc_get_id (OGMDvdDisc *disc)
{
  g_return_val_if_fail (disc != NULL, NULL);

  return disc->id;
}

/**
 * ogmdvd_disc_get_device:
 * @disc: An #OGMDvdDisc
 *
 * Returns the DVD device.
 *
 * Returns: The device of the DVD.
 */
const gchar *
ogmdvd_disc_get_device (OGMDvdDisc *disc)
{
  g_return_val_if_fail (disc != NULL, NULL);

  return disc->device;
}

/**
 * ogmdvd_disc_get_vmg_size:
 * @disc: An #OGMDvdDisc
 *
 * Returns the size of the video manager in bytes.
 *
 * Returns: The size in bytes, or -1
 */
gint64
ogmdvd_disc_get_vmg_size (OGMDvdDisc *disc)
{
  g_return_val_if_fail (disc != NULL, -1);

  return disc->vmg_size;
}

/**
 * ogmdvd_disc_get_n_titles:
 * @disc: An #OGMDvdDisc
 *
 * Returns the number of video titles of this DVD.
 *
 * Returns: The number of video titles, or -1
 */
gint
ogmdvd_disc_get_n_titles (OGMDvdDisc *disc)
{
  g_return_val_if_fail (disc != NULL, -1);

  return disc->ntitles;
}

static gint
ogmdvd_title_find_by_nr (OGMDvdTitle *title, guint nr)
{
  return title->nr - nr;
}

/**
 * ogmdvd_disc_get_nth_title:
 * @disc: An #OGMDvdDisc
 * @nr: The title number
 *
 * Returns the video title at position nr. The first nr is 0.
 *
 * Returns: The #OGMDvdTitle, or NULL
 */
OGMDvdTitle *
ogmdvd_disc_get_nth_title (OGMDvdDisc *disc, guint nr)
{
  GSList *link;

  g_return_val_if_fail (disc != NULL, NULL);
  g_return_val_if_fail (nr >= 0 && nr < disc->ntitles, NULL);

  link = g_slist_find_custom (disc->titles, GUINT_TO_POINTER (nr), (GCompareFunc) ogmdvd_title_find_by_nr);
  if (!link)
    return NULL;

  return link->data;
}

/**
 * ogmdvd_disc_get_titles:
 * @disc: An #OGMDvdDisc
 *
 * Returns a list of all the titles of @disc.
 *
 * Returns: A #GSList, or NULL
 */
GSList *
ogmdvd_disc_get_titles (OGMDvdDisc *disc)
{
  g_return_val_if_fail (disc != NULL, NULL);

  g_slist_foreach (disc->titles, (GFunc) ogmdvd_title_ref, NULL);

  return g_slist_copy (disc->titles);;
}

