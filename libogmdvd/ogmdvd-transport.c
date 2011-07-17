/* OGMDvd - A wrapper library around libdvdread
 * Copyright (C) 2009-2010 Olivier Rolland <billl@users.sourceforge.net>
 *
 * Code from dvd+rw-tools
 * Copyright (C) Andy Polyakov <appro@fy.chalmers.se>
 *
 * dvd+rw-tools is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dvd+rw-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmdvd-transport.h"

#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#ifndef CHECK_CONDITION
#define CHECK_CONDITION 0x01
#endif

#ifndef EMEDIUMTYPE
#define EMEDIUMTYPE EINVAL
#endif

#ifndef	ENOMEDIUM
#define	ENOMEDIUM ENODEV
#endif

#define ERRCODE(s)      ((((s)[2]&0x0F)<<16)|((s)[12]<<8)|((s)[13]))
#define SK(errcode)     (((errcode)>>16)&0xF)
#define ASC(errcode)    (((errcode)>>8)&0xFF)
#define ASCQ(errcode)   ((errcode)&0xFF)

#define CREAM_ON_ERRNO_NAKED(s)                    \
    switch ((s)[12])                               \
    {   case 0x04: errno=EAGAIN;   break;          \
        case 0x20: errno=ENODEV;   break;          \
        case 0x21: if ((s)[13]==0) errno=ENOSPC;   \
                   else            errno=EINVAL;   \
                   break;                          \
        case 0x30: errno=EMEDIUMTYPE;      break;  \
        case 0x3A: errno=ENOMEDIUM;        break;  \
    }
#define CREAM_ON_ERRNO(s) do { CREAM_ON_ERRNO_NAKED(s) } while(0)

inline long
getmsecs ()
{
  struct timeval tv;
  gettimeofday (&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void
sperror (const char *cmd, int err)
{
  int saved_errno = errno;

  if (err == -1)
    fprintf (stderr, ":-( unable to %s: ", cmd);
  else
    fprintf (stderr, ":-[ %s failed with SK=%Xh/ASC=%02Xh/ACQ=%02Xh]: ",
             cmd, SK (err), ASC (err), ASCQ (err));
  errno = saved_errno, perror (NULL);
}

#if defined(HAVE_SG_IO_HDR_T)

#include <limits.h>
#include <mntent.h>

#include <linux/cdrom.h>

#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>

#include <scsi/sg.h>

static const int Dir_xlate[] =
{
  SG_DXFER_NONE,
  SG_DXFER_FROM_DEV,
  SG_DXFER_TO_DEV
};

struct _Scsi_Command
{
  int fd, autoclose;
  char *filename;
  struct cdrom_generic_command cgc;
  union
  {
    struct request_sense s;
    unsigned char u[18];
  } _sense;
  struct sg_io_hdr sg_io;
};

Scsi_Command *
scsi_command_new_from_fd (int fd)
{
  Scsi_Command *cmd;

  cmd = (Scsi_Command *) malloc (sizeof (Scsi_Command));
  cmd->fd = fd;
  cmd->autoclose = 0;
  cmd->filename = NULL;

  return cmd;
}

void
scsi_command_free (Scsi_Command *cmd)
{
  if (cmd->fd >= 0 && cmd->autoclose)
  {
    close (cmd->fd);
    cmd->fd = -1;
  }

  if (cmd->filename)
  {
    free (cmd->filename);
    cmd->filename = NULL;
  }

  free (cmd);
}

void
scsi_command_set (Scsi_Command *cmd, size_t index, unsigned char value)
{
  if (index == 0)
  {
    memset (&cmd->cgc, 0, sizeof (cmd->cgc)), memset (&cmd->_sense, 0, sizeof (cmd->_sense));
    cmd->cgc.quiet = 1;
    cmd->cgc.sense = &cmd->_sense.s;
    memset (&cmd->sg_io, 0, sizeof (cmd->sg_io));
    cmd->sg_io.interface_id = 'S';
    cmd->sg_io.mx_sb_len = sizeof (cmd->_sense);
    cmd->sg_io.cmdp = cmd->cgc.cmd;
    cmd->sg_io.sbp = cmd->_sense.u;
    cmd->sg_io.flags = SG_FLAG_LUN_INHIBIT | SG_FLAG_DIRECT_IO;
  }
  cmd->sg_io.cmd_len = index + 1;
  cmd->cgc.cmd[index] = value;
}

static const int real_dir[] =
{
  CGC_DATA_NONE,
  CGC_DATA_READ,
  CGC_DATA_WRITE
};

int
scsi_command_transport (Scsi_Command *cmd, Direction dir, void *buf, size_t sz)
{
  int ret = 0;

  cmd->sg_io.dxferp = buf;
  cmd->sg_io.dxfer_len = sz;
  cmd->sg_io.dxfer_direction = Dir_xlate[dir];
  if (ioctl (cmd->fd, SG_IO, &cmd->sg_io))
    return -1;

  if ((cmd->sg_io.info & SG_INFO_OK_MASK) != SG_INFO_OK)
  {
    errno = EIO;
    ret = -1;
    if (cmd->sg_io.masked_status & CHECK_CONDITION)
    {
      ret = ERRCODE ((unsigned char *) cmd->sg_io.sbp);
      if (ret == 0)
        ret = -1;
      else
        CREAM_ON_ERRNO ((unsigned char *) cmd->sg_io.sbp);
    }
  }

  return ret;
}

#elif defined(HAVE_SCSIREQ_T)

#include <sys/ioctl.h>
#include <sys/scsiio.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/mount.h>

typedef off_t off64_t;
#define stat64   stat
#define fstat64  fstat
#define open64   open
#define pread64  pread
#define pwrite64 pwrite
#define lseek64  lseek

typedef enum
{
  NONE = 0,
  READ = SCCMD_READ,
  WRITE = SCCMD_WRITE
} Direction;

struct _Scsi_Command
{
  int fd;
  int autoclose;
  char *filename;
  scsireq_t req;
};

Scsi_Command *
scsi_command_new_from_fd (int f)
{
  Scsi_Command *cmd;

  cmd = (Scsi_Command *) malloc (sizeof (Scsi_Command));
  cmd->fd = f;
  cmd->autoclose = 0;
  cmd->filename = NULL;

  return cmd;
}

void
scsi_command_free (Scsi_Command *cmd)
{
 if (cmd->fd >= 0 && cmd->autoclose)
 {
   close (cmd->fd);
   cmd->fd = -1;
 }

 if (cmd->filename)
 {
   free (cmd->filename);
   cmd->filename = NULL;
 }

 free (cmd);
}

void
scsi_command_set (Scsi_Command *cmd, size_t index, unsigned char value)
{
  if (index == 0)
  {
    memset (&req, 0, sizeof (req));
    req.flags = SCCMD_ESCAPE;
    req.timeout = 30000;
    req.senselen = 18; //sizeof(req.sense);
  }
  req.cmdlen = index + 1;
  req.cmd[index] = value;
}

int
scsi_command_transport (Scsi_Command *cmd, Direction dir, void *buf, size_t sz)
{
  int ret = 0;

  req.databuf = (caddr_t) buf;
  req.datalen = sz;
  req.flags |= dir;

  if (ioctl (fd, SCIOCCOMMAND, &req) < 0)
    return -1;
  if (req.retsts == SCCMD_OK)
    return 0;

  errno = EIO;
  ret = -1;
  if (req.retsts == SCCMD_SENSE)
  {
    ret = ERRCODE (req.sense);
    if (ret == 0)
      ret = -1;
    else
      CREAM_ON_ERRNO (req.sense);
  }

  return ret;
}

#elif defined(HAVE_STRUCT_USCSI_CMD)

/* Licensing terms for commercial distribution for Solaris are to be
   settled with Inserve Technology, Åvägen 6, 412 50 GÖTEBORG, Sweden,
   tel. +46-(0)31-86 87 88, see http://www.inserve.se/ for further
  details. */

#include <sys/scsi/impl/uscsi.h>

struct _Scsi_Command
{
  int fd;
  int autoclose;
  char *filename;
  struct uscsi_cmd ucmd;
  unsigned char cdb[16];
  unsigned char _sense[18];
};

Scsi_Command *
scsi_command_new_from_fd (int f)
{
  Scsi_Command *cmd;

  cmd = (Scsi_Command *) malloc (sizeof (Scsi_Command));
  cmd->fd = f;
  cmd->autoclose = 0;
  cmd->filename = NULL;

  return cmd;
}

void
scsi_command_free (Scsi_Command *cmd)
{
 if (cmd->fd >= 0 && cmd->autoclose)
 {
   close (cmd->fd);
   cmd->fd = -1;
 }

 if (cmd->filename)
 {
   free (cmd->filename);
   cmd->filename = NULL;
 }

 free (cmd);
}

void
scsi_command_set (Scsi_Command *cmd, size_t index, unsigned char value)
{
  if (index == 0)
  {
    memset (&cmd->ucmd, 0, sizeof (cmd->ucmd));
    memset (cmd->cdb, 0, sizeof (cmd->cdb));
    memset (cmd->_sense, 0, sizeof (cmd->_sense));
    cmd->ucmd.uscsi_cdb = (caddr_t) cmd->cdb;
    cmd->ucmd.uscsi_rqbuf = (caddr_t) cmd->_sense;
    cmd->ucmd.uscsi_rqlen = sizeof (cmd->_sense);
    cmd->ucmd.uscsi_flags = USCSI_SILENT | USCSI_DIAGNOSE |
      USCSI_ISOLATE | USCSI_RQENABLE;
    cmd->ucmd.uscsi_timeout = 60;
  }
  cmd->ucmd.uscsi_cdblen = index + 1;
  cmd->cdb[index] = value;
}

static const int real_dir[] =
{
  0,
  USCSI_READ,
  USCSI_WRITE
};

int
scsi_command_transport (Scsi_Command *cmd, Direction dir, void *buf, size_t sz)
{
  int ret = 0;

  cmd->ucmd.uscsi_bufaddr = (caddr_t) buf;
  cmd->ucmd.uscsi_buflen = sz;
  cmd->ucmd.uscsi_flags |= dir;
  if (ioctl (cmd->fd, USCSICMD, &cmd->ucmd))
  {
    if (errno == EIO && cmd->_sense[0] == 0) /* USB seems to be broken... */
    {
      size_t residue = cmd->ucmd.uscsi_resid;
      memset (cmd->cdb, 0, sizeof (cmd->cdb));
      cmd->cdb[0] = 0x03;          /* REQUEST SENSE */
      cmd->cdb[4] = sizeof (cmd->_sense);
      cmd->ucmd.uscsi_cdblen = 6;
      cmd->ucmd.uscsi_bufaddr = (caddr_t) cmd->_sense;
      cmd->ucmd.uscsi_buflen = sizeof (cmd->_sense);
      cmd->ucmd.uscsi_flags = USCSI_SILENT | USCSI_DIAGNOSE |
        USCSI_ISOLATE | USCSI_READ;
      cmd->ucmd.uscsi_timeout = 1;
      ret = ioctl (cmd->fd, USCSICMD, &cmd->ucmd);
      cmd->ucmd.uscsi_resid = residue;
      if (ret)
        return -1;
    }
    ret = ERRCODE (cmd->_sense);
    if (ret == 0)
      ret = -1;
    /*else  CREAM_ON_ERRNO(_sense); */
  }
  return ret;
}

#elif defined(HAVE_STRUCT_CAM_DEVICE)

#include <camlib.h>
#include <dirent.h>

#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/param.h>

#include <cam/scsi/scsi_message.h>
#include <cam/scsi/scsi_pass.h>

typedef off_t off64_t;
#define stat64   stat
#define fstat64  fstat
#define open64   open
#define pread64  pread
#define pwrite64 pwrite
#define lseek64  lseek

#define ioctl_fd (((struct cam_device *)ioctl_handle)->fd)

struct _Scsi_Command
{
  int fd;
  int autoclose;
  char *filename;
  struct cam_device *cam;
  union ccb ccb;
};

Scsi_Command *
scsi_command_new_from_fd (int f)
{
  Scsi_Command *cmd;

  char pass[32]; /* periph_name is 16 chars long */

  cmd = (Scsi_Command *) malloc (sizeof (Scsi_Command));
  cmd->fd = -1;
  cmd->autoclose = 1;
  cmd->filename = NULL;

  memset (&cmd->ccb, 0, sizeof (cmd->ccb));
  cmd->ccb.ccb_h.func_code = XPT_GDEVLIST;
  if (ioctl (f, CAMGETPASSTHRU, &cmd->ccb) < 0)
  {
    free (cmd);
    return NULL;
  }

  sprintf (pass, "/dev/%.15s%u", cmd->ccb.cgdl.periph_name,
           cmd->ccb.cgdl.unit_number);
  cmd->cam = cam_open_pass (pass, O_RDWR, NULL);

  return cmd;
}

void
scsi_command_free (Scsi_Command *cmd)
{
  if (cmd->cam && cmd->autoclose)
  {
    cam_close_device (cmd->cam);
    cmd->cam = NULL;
  }

  if (cmd->fd >= 0)
    close (cmd->fd);

  if (cmd->filename)
  {
    free (cmd->filename);
    cmd->filename = NULL;
  }

  free (cmd);
}

void
scsi_command_set (Scsi_Command *cmd, size_t index, unsigned char value)
{
  if (index == 0)
  {
    memset (&cmd->ccb, 0, sizeof (cmd->ccb));
    cmd->ccb.ccb_h.path_id = cmd->cam->path_id;
    cmd->ccb.ccb_h.target_id = cmd->cam->target_id;
    cmd->ccb.ccb_h.target_lun = cmd->cam->target_lun;
    cam_fill_csio (&(cmd->ccb.csio), 1,  /* retries */
                   NULL,      /* cbfncp */
                   CAM_DEV_QFRZDIS, /* flags */
                   MSG_SIMPLE_Q_TAG,  /* tag_action */
                   NULL,      /* data_ptr */
                   0,         /* dxfer_len */
                   sizeof (cmd->ccb.csio.sense_data),  /* sense_len */
                   0,         /* cdb_len */
                   30 * 1000);  /* timeout */
  }
  cmd->ccb.csio.cdb_len = index + 1;
  cmd->ccb.csio.cdb_io.cdb_bytes[index] = value;
}

static const int real_dir[] =
{
  CAM_DIR_NONE,
  CAM_DIR_IN,
  CAM_DIR_OUT
};

int
scsi_command_transport (Scsi_Command *cmd, Direction dir, void *buf, size_t sz)
{
  int ret = 0;

  cmd->ccb.csio.ccb_h.flags |= real_dir[dir];
  cmd->ccb.csio.data_ptr = (u_int8_t *) buf;
  cmd->ccb.csio.dxfer_len = sz;

  if ((ret = cam_send_ccb (cmd->cam, &cmd->ccb)) < 0)
    return -1;

  if ((cmd->ccb.ccb_h.status & CAM_STATUS_MASK) == CAM_REQ_CMP)
    return 0;

  unsigned char *sense = (unsigned char *) &cmd->ccb.csio.sense_data;

  errno = EIO;
  /* FreeBSD 5-CURRENT since 2003-08-24, including 5.2 fails to
     pull sense data automatically, at least for ATAPI transport,
     so I reach for it myself... */
  if ((cmd->ccb.csio.scsi_status == SCSI_STATUS_CHECK_COND) &&
      !(cmd->ccb.ccb_h.status & CAM_AUTOSNS_VALID))
  {
    u_int8_t _sense[18];
    u_int32_t resid = cmd->ccb.csio.resid;

    memset (_sense, 0, sizeof (_sense));

    scsi_command_set (cmd, 0, 0x03); /* REQUEST SENSE */
    cmd->ccb.csio.cdb_io.cdb_bytes[4] = sizeof (_sense);
    cmd->ccb.csio.cdb_len = 6;
    cmd->ccb.csio.ccb_h.flags |= CAM_DIR_IN | CAM_DIS_AUTOSENSE;
    cmd->ccb.csio.data_ptr = _sense;
    cmd->ccb.csio.dxfer_len = sizeof (_sense);
    cmd->ccb.csio.sense_len = 0;
    ret = cam_send_ccb (cmd->cam, &cmd->ccb);

    cmd->ccb.csio.resid = resid;
    if (ret < 0)
      return -1;
    if ((cmd->ccb.ccb_h.status & CAM_STATUS_MASK) != CAM_REQ_CMP)
      return errno = EIO, -1;

    memcpy (sense, _sense, sizeof (_sense));
  }

  ret = ERRCODE (sense);
  if (ret == 0)
    ret = -1;
  else
    CREAM_ON_ERRNO (sense);

  return ret;
}

#endif

