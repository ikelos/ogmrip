/* dvdcpy - A DVD backup tool
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

enum
{
  ABORT,
  BLANK,
  SKIP
};

static ssize_t current_size, total_size;
static unsigned int strategy = ABORT;

#ifndef HAVE_DVD_FILE_STAT
uint32_t UDFFindFile (dvd_reader_t *, const char *, uint32_t *);

typedef struct
{
  off_t size;
  int nr_parts;
  off_t parts_size[9];
} dvd_stat_t;

int
DVDFileStat (dvd_reader_t *reader, int vts, dvd_read_domain_t domain, dvd_stat_t *statbuf)
{
  char filename[FILENAME_MAX];
  uint32_t bytes;

  if (!reader)
    return -1;

  if (domain == DVD_READ_TITLE_VOBS && vts == 0)
    return -1;

  memset (statbuf, 0, sizeof (dvd_stat_t));

  if (domain == DVD_READ_TITLE_VOBS)
  {
    unsigned int vob;

    vob = 1;
    while (1)
    {
      snprintf (filename, FILENAME_MAX, "/VIDEO_TS/VTS_%02u_%u.VOB", vts, vob++);
      if (!UDFFindFile (reader, filename, &bytes))
        break;
      statbuf->size += (off_t) bytes;
      statbuf->parts_size[statbuf->nr_parts++] = (off_t) bytes;
    }

    if (vob == 1)
      return -1;

    return 0;
  }

  switch (domain)
  {
    case DVD_READ_INFO_FILE:
      if (vts == 0)
        strncpy (filename, "/VIDEO_TS/VIDEO_TS.IFO", FILENAME_MAX);
      else
        snprintf (filename, FILENAME_MAX, "/VIDEO_TS/VTS_%02u_0.IFO", vts);
      break;
    case DVD_READ_INFO_BACKUP_FILE:
      if (vts == 0)
        strncpy (filename, "/VIDEO_TS/VIDEO_TS.BUP", FILENAME_MAX);
      else
        snprintf (filename, FILENAME_MAX, "/VIDEO_TS/VTS_%02u_0.BUP", vts);
      break;
    case DVD_READ_MENU_VOBS:
      if (vts == 0)
        strncpy (filename, "/VIDEO_TS/VIDEO_TS.VOB", FILENAME_MAX);
      else
        snprintf (filename, FILENAME_MAX, "/VIDEO_TS/VTS_%02u_0.VOB", vts);
      break;
    case DVD_READ_TITLE_VOBS:
      break;
  }

  if (!UDFFindFile (reader, filename, &bytes))
    return -1;

  statbuf->size = (off_t) bytes;

  return 0;
}
#endif /* HAVE_DVD_FILE_STAT */

static ssize_t
dvd_file_size (dvd_reader_t *reader, unsigned int vts, dvd_read_domain_t domain)
{
  dvd_stat_t statbuf;

  if (DVDFileStat (reader, vts, domain, &statbuf) < 0)
  {
    fprintf (stderr, "Error: Cannot stat VOB for VTS %u\n", vts);
    return -1;
  }

  return statbuf.size / (uint32_t) DVD_VIDEO_LB_LEN;
}

/*
 * Usage
 */

static void
usage (const char *name)
{
  fprintf (stdout, "Usage:\n");
  fprintf (stdout, "  %s [OPTION...] <DVD DEVICE>\n", name);
  fprintf (stdout, "\n");
  fprintf (stdout, "Help Options:\n");
  fprintf (stdout, "  -h, --help                Show help options\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "Application Options:\n");
  fprintf (stdout, "  -o, --output output       Specify the output directory\n");
  fprintf (stdout, "  -t, --title title         Select the title to copy. This option may be\n");
  fprintf (stdout, "                            specified multiple times (default: all titles)\n");
  fprintf (stdout, "  -s, --strategy strategy   Select the strategy to use on error: skip, blank or\n");
  fprintf (stdout, "                            abort (default: abort)\n");
  fprintf (stdout, "  -m, --menu                Copy the DVD menu\n");
  fprintf (stdout, "\n");
}

/*
 * Helper functions
 */

static int
make_dir (const char *dir)
{
  struct stat buf;

  if (mkdir (dir, 0755) < 0)
    if (stat (dir, &buf) < 0 || !S_ISDIR (buf.st_mode))
      return -1;

  return 0;
}

static int
make_path (const char *path)
{
  char *dir, *slash;
  int len;

  if (path[0] == '/' && path[1] == '\0')
    return 0;

  slash = dir = strdup (path);

  len = strlen (dir);
  while (len > 1 && dir[--len] == '/')
    dir[len] = '\0';

  while (1)
  {
    while (*slash == '/')
      slash ++;

    slash = strchr (slash, '/');
    if (!slash)
      break;

    *slash = '\0';

    if (make_dir (dir) < 0)
    {
      free (dir);
      return -1;
    }

    *slash++ = '/';
  }

  if (make_dir (dir) < 0)
  {
    free (dir);
    return -1;
  }

  free (dir);

  return 0;
}

static int
make_pathes (const char *path1, ...)
{
  va_list ap;
  char path[FILENAME_MAX];
  size_t len;

  strncpy (path, path1, FILENAME_MAX);
  len = strlen(path);

  va_start (ap, path1);
  while (path1)
  {
    if (make_path (path) < 0)
    {
      va_end (ap);
      return -1;
    }

    path1 = va_arg (ap, const char *);
    if (path1)
    {
      strncat (path, "/", FILENAME_MAX - len - 1);
      strncat (path, path1, FILENAME_MAX - len - 2);
      len += strlen (path1) + 1;
    }
  }
  va_end (ap);

  return 0;
}

static long long
get_space_left (const char *path)
{
#ifdef HAVE_STATVFS
  struct statvfs buf;
#else
  struct statfs buf;
#endif

#ifdef HAVE_STATVFS
  if (statvfs (path, &buf) < 0)
    return -1;
#else
  if (statfs (path, &buf) < 0)
    return -1;
#endif

  return (long long) buf.f_bsize * (long long) buf.f_bavail / DVD_VIDEO_LB_LEN;
}

static ssize_t
read_blocks (dvd_file_t *file, size_t offset, size_t size, unsigned char *buffer)
{
  ssize_t count, skip;

  for (skip = 0; skip < size; skip ++)
  {
    count = DVDReadBlocks (file, offset + skip, size - skip, buffer);
    if (count > 0)
      break;

    if (strategy == ABORT)
      return count;

    if (strategy == BLANK)
      memset (buffer + skip, 0, DVD_VIDEO_LB_LEN);
  }

  if (skip)
    fprintf (stderr, "Warning: %s %zd blocks at offset %zd\n",
        strategy == SKIP ? "Skipped" : "Blanked", skip, offset);

  return count + skip;
}

static ssize_t
write_blocks (int fd, const void *buffer, size_t nblocks)
{
  ssize_t written, total_written;
  size_t nbytes;

  total_written = 0;
  nbytes = nblocks * DVD_VIDEO_LB_LEN;

  while (total_written != nbytes)
  {
    written = write (fd, buffer + total_written, nbytes - total_written);
    if (written < 0)
      return -1;
    total_written += written;
  }

  return nblocks;
}

static ssize_t
dvd_copy_blocks (dvd_file_t *file, size_t offset, size_t size, int fd)
{
  unsigned char buffer[512 * DVD_VIDEO_LB_LEN];
  unsigned int start;
  ssize_t count, len;

  for (count = 512, start = offset; offset < start + size; offset += count)
  {
    if (offset + count > start + size)
      count = start + size - offset;
    len = count;

    count = read_blocks (file, offset, len, buffer);
    if (count < 0)
    {
      fprintf (stderr, "Error: Cannot read data\n");
      return -1;
    }

    if (write_blocks (fd, buffer, count) < 0)
    {
      fprintf (stderr, "Error: Cannot write data: %s\n", strerror (errno));
      return -1;
    }

    current_size += count;
    fprintf (stdout, "\r%zd/%zd blocks written (%zd%%)", current_size, total_size, current_size * 100 / total_size);
    fflush (stdout);
  }

  fprintf (stdout, "\r%zd/%zd blocks written (%zd%%)", current_size, total_size, current_size * 100 / total_size);
  fflush (stdout);

  return size;
}

/*
 * Get size functions
 */

static ssize_t
dvd_get_vts_size (dvd_reader_t *reader, unsigned int vts)
{
  ssize_t size, fullsize;

  if ((size  = dvd_file_size (reader, vts, DVD_READ_INFO_FILE)) < 0)
    return -1;
  fullsize = size;

  if ((size = dvd_file_size (reader, vts, DVD_READ_INFO_BACKUP_FILE)) > 0)
    fullsize += size;

  if ((size = dvd_file_size (reader, vts, DVD_READ_MENU_VOBS)) > 0)
    fullsize += size;

  if (vts > 0)
  {
    if ((size = dvd_file_size (reader, vts, DVD_READ_TITLE_VOBS)) < 0)
      return -1;
    fullsize += size;
  }

  return fullsize;
}

static ssize_t
dvd_get_vmg_size (dvd_reader_t *reader)
{
  ssize_t size, fullsize;

  if ((size  = dvd_file_size (reader, 0, DVD_READ_INFO_FILE)) < 0)
    return -1;
  fullsize = size;

  if ((size = dvd_file_size (reader, 0, DVD_READ_INFO_BACKUP_FILE)) > 0)
    fullsize += size;

  if ((size = dvd_file_size (reader, 0, DVD_READ_MENU_VOBS)) > 0)
    fullsize += size;

  return fullsize;
}

static ssize_t
dvd_get_size (dvd_reader_t *reader, ifo_handle_t *vmg_file, unsigned int vmg, unsigned int *vtss)
{
  ssize_t fullsize, size;
  unsigned int vts;

  if ((fullsize = dvd_get_vmg_size (reader)) < 0)
    return -1;

  for (vts = 0; vts < vmg_file->vmgi_mat->vmg_nr_of_title_sets; vts ++)
  {
    if (vtss[vts])
    {
      if ((size = dvd_get_vts_size (reader, vts + 1)) < 0)
        return -1;
      fullsize += size;
    }
  }

  return fullsize;
}

/*
 * Copy functions
 */

static ssize_t
dvd_copy_ifo (dvd_reader_t *reader, unsigned int vts, const char *output)
{
  char filename[FILENAME_MAX];
  ssize_t size, copied;
  struct stat buf;
  int fd;

  dvd_file_t *file;
  dvd_stat_t dvd_stat;

  if (DVDFileStat (reader, vts, DVD_READ_INFO_FILE, &dvd_stat) < 0)
  {
    fprintf (stderr, "Error: Cannot stat IFO for VTS %u\n", vts);
    return -1;
  }

  if (vts == 0)
    snprintf (filename, FILENAME_MAX, "%s/VIDEO_TS/VIDEO_TS.IFO", output);
  else
    snprintf (filename, FILENAME_MAX, "%s/VIDEO_TS/VTS_%02u_0.IFO", output, vts);

  if (stat (filename, &buf) < 0)
    fd = open (filename, O_WRONLY | O_CREAT, 0644);
  else
  {
    if (!S_ISREG (buf.st_mode))
    {
      fprintf (stderr, "Error: %s already exists and is not a regular file\n", filename);
      return -1;
    }
    fd = open (filename, O_WRONLY | O_TRUNC, 0644);
  }

  if (fd < 0)
  {
    fprintf (stderr, "Error: Cannot open %s\n", filename);
    return -1;
  }

  file = DVDOpenFile (reader, vts, DVD_READ_INFO_FILE);
  if (!file)
  {
    fprintf (stderr, "Error: Cannot open IFO for VTS %u\n", vts);
    close (fd);
    return -1;
  }

  size = dvd_stat.size / (uint32_t) DVD_VIDEO_LB_LEN;

  fprintf (stdout, "Copying IFO for VTS %u\n", vts);
  copied = dvd_copy_blocks (file, 0, size, fd);
  fprintf (stdout, "\n");

  DVDCloseFile (file);

  close (fd);

  if (copied != size)
  {
    fprintf (stderr, "Error: Failed to copy IFO for VTS %u\n", vts);
    return -1;
  }

  return size;
}

static ssize_t
dvd_copy_bup (dvd_reader_t *reader, unsigned int vts, const char *output)
{
  char filename[FILENAME_MAX];
  ssize_t size, copied;
  struct stat buf;
  int fd;

  dvd_file_t *file;

  if ((size = dvd_file_size (reader, vts, DVD_READ_INFO_BACKUP_FILE)) <= 0)
    return size;

  if (vts == 0)
    snprintf (filename, FILENAME_MAX, "%s/VIDEO_TS/VIDEO_TS.BUP", output);
  else
    snprintf (filename, FILENAME_MAX, "%s/VIDEO_TS/VTS_%02u_0.BUP", output, vts);

  if (stat (filename, &buf) < 0)
    fd = open (filename, O_WRONLY | O_CREAT, 0644);
  else
  {
    if (!S_ISREG (buf.st_mode))
    {
      fprintf (stderr, "Error: %s already exists and is not a regular file\n", filename);
      return -1;
    }
    fd = open (filename, O_WRONLY | O_TRUNC, 0644);
  }

  if (fd < 0)
  {
    fprintf (stderr, "Error: Cannot open %s\n", filename);
    return -1;
  }

  file = DVDOpenFile (reader, vts, DVD_READ_INFO_BACKUP_FILE);
  if (!file)
  {
    fprintf (stderr, "Error: Cannot open BUP for VTS %u\n", vts);
    close (fd);
    return -1;
  }

  fprintf (stdout, "Copying BUP for VTS %u\n", vts);
  copied = dvd_copy_blocks (file, 0, size, fd);
  fprintf (stdout, "\n");

  DVDCloseFile (file);

  close (fd);

  if (copied != size)
  {
    fprintf (stderr, "Error: Failed to copy BUP for VTS %u\n", vts);
    return -1;
  }

  return size;
}

static ssize_t
dvd_copy_menu (dvd_reader_t *reader, unsigned int vts, const char *output)
{
  char filename[FILENAME_MAX];
  ssize_t size, copied;
  struct stat buf;
  int fd;

  dvd_file_t *file;

  if ((size = dvd_file_size (reader, vts, DVD_READ_MENU_VOBS)) <= 0)
    return size;

  if (vts == 0)
    snprintf (filename, FILENAME_MAX, "%s/VIDEO_TS/VIDEO_TS.VOB", output);
  else
    snprintf (filename, FILENAME_MAX, "%s/VIDEO_TS/VTS_%02u_0.VOB", output, vts);

  if (stat (filename, &buf) < 0)
    fd = open (filename, O_WRONLY | O_CREAT, 0644);
  else
  {
    if (!S_ISREG (buf.st_mode))
    {
      fprintf (stderr, "Error: %s already exists and is not a regular file\n", filename);
      return -1;
    }
    fd = open (filename, O_WRONLY | O_TRUNC, 0644);
  }

  if (fd < 0)
  {
    fprintf (stderr, "Error: Cannot open %s\n", filename);
    return -1;
  }

  file = DVDOpenFile (reader, vts, DVD_READ_MENU_VOBS);
  if (!file)
  {
    fprintf (stderr, "Error: Cannot open menu for VTS %u\n", vts);
    close (fd);
    return -1;
  }

  fprintf (stdout, "Copying menu for VTS %u\n", vts);
  copied = dvd_copy_blocks (file, 0, size, fd);
  fprintf (stdout, "\n");

  DVDCloseFile (file);

  close (fd);

  if (copied != size)
  {
    fprintf (stderr, "Error: Failed to copy menu for VTS %u\n", vts);
    return -1;
  }

  return size;
}

static long long
dvd_copy_vob (dvd_reader_t *reader, unsigned int vts, const char *output)
{
  char filename[FILENAME_MAX];
  struct stat buf;
  ssize_t size, copied;
  size_t offset;
  int vob, fd;

  dvd_file_t *dvd_file;
  dvd_stat_t dvd_stat;

  if (DVDFileStat (reader, vts, DVD_READ_TITLE_VOBS, &dvd_stat) < 0)
  {
    fprintf (stderr, "Error: Cannot stat VOB for VTS %u\n", vts);
    return -1;
  }

  dvd_file = DVDOpenFile (reader, vts, DVD_READ_TITLE_VOBS);
  if (!dvd_file)
  {
    fprintf (stderr, "Error: Cannot open VOB for VTS %u\n", vts);
    return -1;
  }

  for (vob = 0, offset = 0; vob < dvd_stat.nr_parts; vob ++)
  {
    snprintf (filename, FILENAME_MAX, "%s/VIDEO_TS/VTS_%02u_%u.VOB", output, vts, vob + 1);

    if (stat (filename, &buf) < 0)
      fd = open (filename, O_WRONLY | O_CREAT, 0644);
    else
    {
      if (!S_ISREG (buf.st_mode))
      {
        DVDCloseFile (dvd_file);

        fprintf (stderr, "Error: %s already exists and is not a regular file\n", filename);
        return -1;
      }
      fd = open (filename, O_WRONLY | O_TRUNC, 0644);
    }

    if (fd < 0)
    {
      DVDCloseFile (dvd_file);

      fprintf (stderr, "Error: Cannot open %s\n", filename);
      return -1;
    }

    size = dvd_stat.parts_size[vob] / DVD_VIDEO_LB_LEN;

    fprintf (stdout, "Copying VOB %u for VTS %u\n", vob + 1, vts);
    copied = dvd_copy_blocks (dvd_file, offset, size, fd);
    fprintf (stdout, "\n");

    close (fd);

    if (copied != size)
    {
      fprintf (stderr, "Error: Failed to copy VOB part %u for VTS %u\n", vob, vts);
      break;
    }

    offset += size;
  }

  DVDCloseFile (dvd_file);

  if (offset != dvd_stat.size / (uint32_t) DVD_VIDEO_LB_LEN)
  {
    fprintf (stderr, "Error: Failed to copy VOB for VTS %u\n", vts);
    return -1;
  }

  return offset;
}

static ssize_t
dvd_copy_vmg (dvd_reader_t *reader, const char *output)
{
  ssize_t fullsize, size;

  if ((size = dvd_copy_ifo (reader, 0, output)) < 0)
    return -1;
  fullsize = size;

  if ((size = dvd_copy_bup (reader, 0, output)) > 0)
    fullsize += size;

  if ((size = dvd_copy_menu (reader, 0, output)) > 0)
    fullsize += size;

  return fullsize;
}

static ssize_t
dvd_copy_vts (dvd_reader_t *reader, unsigned int vts, const char *output)
{
  ssize_t fullsize, size;

  if ((size = dvd_copy_ifo (reader, vts, output)) < 0)
    return -1;
  fullsize = size;

  if ((size = dvd_copy_bup (reader, vts, output)) > 0)
    fullsize += size;

  if ((size = dvd_copy_menu (reader, vts, output)) > 0)
    fullsize += size;

  if ((size = dvd_copy_vob (reader, vts, output)) < 0)
    return -1;
  fullsize += size;

  return fullsize;
}

static ssize_t
dvd_copy (dvd_reader_t *reader, ifo_handle_t *vmg_file, unsigned int vmg, unsigned int *vtss, const char *output)
{
  ssize_t fullsize = 0, size;
  unsigned int vts;

  if (vmg)
  {
    if ((size = dvd_copy_vmg (reader, output)) < 0)
      return -1;
    fullsize = size;
  }

  for (vts = 0; vts < vmg_file->vmgi_mat->vmg_nr_of_title_sets; vts++)
  {
    if (vtss[vts])
    {
      if ((size = dvd_copy_vts (reader, vts+1, output)) < 0)
        return -1;
      fullsize += size;
    }
  }

  return fullsize;
}

static const char *shortopts = "hmo:s:t:";
static const struct option longopts[] =
{
  { "help",     no_argument,       NULL, 'h' },
  { "menu",     no_argument,       NULL, 'm' },
  { "output",   required_argument, NULL, 'o' },
  { "strategy", required_argument, NULL, 's' },
  { "title",    required_argument, NULL, 't' },
  { NULL,       0,                 NULL,  0  }
};

int
main (int argc, char *argv[])
{
  unsigned int i, vmg, title, titles[100], vtss[100];

  dvd_reader_t *reader;
  ifo_handle_t *vmg_file;
  int c, optidx;
  char *output;

  if (argc < 2)
  {
    usage (argv[0]);
    return EXIT_FAILURE;
  }

  title = vmg = 0;
  output = "backup";

  memset (titles, 0, 100 * sizeof (int));

  while ((c = getopt_long (argc, argv, shortopts, longopts, &optidx)) != EOF)
  {
    switch (c)
    {
      case 'h':
        usage (argv[0]);
        return EXIT_SUCCESS;
        break;
      case 'm':
        vmg = 1;
        break;
      case 'o':
        output = optarg;
        break;
      case 's':
        if (strcmp (optarg, "blank") == 0)
          strategy = BLANK;
        else if (strcmp (optarg, "skip") == 0)
          strategy = SKIP;
        else if (strcmp (optarg, "abort") == 0)
          strategy = ABORT;
        else
        {
          fprintf (stderr, "Error: unknown strategy: %s\n", optarg);
          return EXIT_FAILURE; 
        }
        break;
      case 't':
        title = atoi (optarg);
        if (title < 1 || title > 99)
        {
          fprintf (stderr, "Error: title must be greater than 0 and lesser than 100\n");
          return EXIT_FAILURE; 
        }
        titles[title - 1] = 1;
        break;
      default:
        usage (argv[0]);
        return EXIT_FAILURE;
        break;
    }
  }

  if (optind != argc - 1)
  {
    usage (argv[0]);
    return EXIT_FAILURE;
  }

  reader = DVDOpen (argv[optind]);
  if (!reader)
  {
    fprintf (stderr, "Error: Cannot open DVD device\n");
    return EXIT_FAILURE; 
  }

  vmg_file = ifoOpen (reader, 0);
  if (!vmg_file)
  {
    fprintf (stderr, "Error: Cannot open VMG info\n");
    DVDClose (reader);
    return EXIT_FAILURE;
  }

  if (!title)
    vmg = 1;

  for (i = 0; i < 100; i++)
  {
    if (titles[i] && i >= vmg_file->tt_srpt->nr_of_srpts)
    {
      fprintf (stderr, "Error: %d is not a valid title\n", i + 1);
      ifoClose (vmg_file);
      DVDClose (reader);
      return EXIT_FAILURE;
    }
  }

  memset (vtss, 0, vmg_file->vmgi_mat->vmg_nr_of_title_sets * sizeof (int));
  for (i = 0; i < vmg_file->tt_srpt->nr_of_srpts; i++)
    vtss[vmg_file->tt_srpt->title[i].title_set_nr - 1] |= !title || titles[i];

  if (make_pathes (output, "VIDEO_TS", NULL) < 0)
  {
    fprintf (stderr, "Error: Cannot create DVD structure\n");
    ifoClose (vmg_file);
    DVDClose (reader);
    return EXIT_FAILURE;
  }

  if ((total_size = dvd_get_size (reader, vmg_file, vmg, vtss)) < 0)
  {
    fprintf (stderr, "Error: Cannot get the size of the DVD\n");
    ifoClose (vmg_file);
    DVDClose (reader);
    return EXIT_FAILURE;
  }

  if (total_size > get_space_left (output))
  {
    fprintf (stderr, "Error: Not enough space left on device (%zd KB needed)\n", total_size * 2);
    ifoClose (vmg_file);
    DVDClose (reader);
    return EXIT_FAILURE;
  }

  if ((dvd_copy (reader, vmg_file, vmg, vtss, output)) < 0)
  {
    ifoClose (vmg_file);
    DVDClose (reader);
    return EXIT_FAILURE;
  }

  ifoClose (vmg_file);
  DVDClose (reader);

  fprintf (stdout, "Copy completed successfully\n");

  return EXIT_SUCCESS;
}

