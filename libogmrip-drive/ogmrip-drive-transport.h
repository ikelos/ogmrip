/* OGMRipDrive - An optical drive library form OGMRip
 * Copyright (C) 2009-2011 Olivier Rolland <billl@users.sourceforge.net>
 *
 * Code from dvd+rw-tools
 * Copyright (C) Andy Polyakov <appro@fy.chalmers.se>
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

#ifndef __OGMRIP_DRIVE_TRANSPORT_H__
#define __OGMRIP_DRIVE_TRANSPORT_H__

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

#endif /* __OGMRIP_DRIVE_TRANSPORT_H__ */

