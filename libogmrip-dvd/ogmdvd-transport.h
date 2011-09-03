/* OGMDvd - A wrapper library around libdvdread
 * Copyright (C) 2009-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMDVD_TRANSPORT_H__
#define __OGMDVD_TRANSPORT_H__

#include <stdio.h>

typedef enum
{
  NONE,
  READ,
  WRITE
} Direction;

typedef struct _Scsi_Command Scsi_Command;

Scsi_Command *  scsi_command_new_from_fd   (int fd);
void            scsi_command_free          (Scsi_Command  *cmd);

void            scsi_command_set           (Scsi_Command  *cmd,
                                            size_t        index,
                                            unsigned char value);
int             scsi_command_transport     (Scsi_Command  *cmd,
                                            Direction     dir,
                                            void          *buf,
                                            size_t        sz);

#endif /* __OGMDVD_TRANSPORT_H__ */

