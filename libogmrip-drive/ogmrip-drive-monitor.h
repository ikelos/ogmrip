/* OGMRipDrive - An optical drive library form OGMRip
 * Copyright (C) 2009-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_DRIVE_MONITOR_H__
#define __OGMRIP_DRIVE_MONITOR_H__

#include <ogmrip-drive-object.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_MONITOR             (ogmrip_monitor_get_type ())
#define OGMRIP_MONITOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MONITOR, OGMRipMonitor))
#define OGMRIP_MONITOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_MONITOR, OGMRipMonitorClass))
#define OGMRIP_IS_MONITOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_MONITOR))
#define OGMRIP_IS_MONITOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_MONITOR))
#define OGMRIP_MONITOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_MONITOR, OGMRipMonitorClass))

typedef struct _OGMRipMonitorClass OGMRipMonitorClass;
typedef struct _OGMRipMonitor      OGMRipMonitor;
typedef struct _OGMRipMonitorPriv  OGMRipMonitorPriv;

struct _OGMRipMonitor
{
  GObject parent_instance;

  OGMRipMonitorPriv *priv;
};

struct _OGMRipMonitorClass
{
  GObjectClass parent_class;

  void (* drive_added)    (OGMRipMonitor *monitor,
                           OGMRipDrive   *drive);
  void (* drive_removed)  (OGMRipMonitor *monitor,
                           OGMRipDrive   *drive);
};

GType           ogmrip_monitor_get_type    (void) G_GNUC_CONST;

OGMRipMonitor * ogmrip_monitor_get_default (void);

GSList *        ogmrip_monitor_get_drives (OGMRipMonitor *monitor);
OGMRipDrive *   ogmrip_monitor_get_drive  (OGMRipMonitor *monitor,
                                           const gchar   *device);

G_END_DECLS

#endif /* __OGMRIP_DRIVE_MONITOR_H__ */
