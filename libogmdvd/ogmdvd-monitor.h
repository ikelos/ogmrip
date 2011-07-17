/* OGMDvd - A wrapper library around libdvdread
 * Copyright (C) 2009-2010 Olivier Rolland <billl@users.sourceforge.net>
 *
 * Code from libbrasero-media
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Libbrasero-media is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Libbrasero-media is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __OGMDVD_MONITOR_H__
#define __OGMDVD_MONITOR_H__

#include <glib-object.h>

#include <ogmdvd-drive.h>

G_BEGIN_DECLS

#define OGMDVD_TYPE_MONITOR             (ogmdvd_monitor_get_type ())
#define OGMDVD_MONITOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMDVD_TYPE_MONITOR, OGMDvdMonitor))
#define OGMDVD_MONITOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMDVD_TYPE_MONITOR, OGMDvdMonitorClass))
#define OGMDVD_IS_MONITOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMDVD_TYPE_MONITOR))
#define OGMDVD_IS_MONITOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMDVD_TYPE_MONITOR))
#define OGMDVD_MONITOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMDVD_TYPE_MONITOR, OGMDvdMonitorClass))

typedef struct _OGMDvdMonitorClass OGMDvdMonitorClass;
typedef struct _OGMDvdMonitor      OGMDvdMonitor;
typedef struct _OGMDvdMonitorPriv  OGMDvdMonitorPriv;

struct _OGMDvdMonitor
{
  GObject parent_instance;

  OGMDvdMonitorPriv *priv;
};

struct _OGMDvdMonitorClass
{
  GObjectClass parent_class;

  void (* drive_added)    (OGMDvdMonitor *monitor,
                           OGMDvdDrive   *drive);
  void (* drive_removed)  (OGMDvdMonitor *monitor,
                           OGMDvdDrive   *drive);
};

GType           ogmdvd_monitor_get_type    (void) G_GNUC_CONST;

OGMDvdMonitor * ogmdvd_monitor_get_default (void);

GSList *        ogmdvd_monitor_get_drives (OGMDvdMonitor *monitor);
OGMDvdDrive *   ogmdvd_monitor_get_drive  (OGMDvdMonitor *monitor,
                                           const gchar   *device);

G_END_DECLS

#endif /* __OGMDVD_MONITOR_H__ */
