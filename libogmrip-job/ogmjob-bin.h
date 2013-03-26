/* OGMJob - A library to spawn processes
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

#ifndef __OGMJOB_BIN_H__
#define __OGMJOB_BIN_H__

#include <ogmjob-container.h>

G_BEGIN_DECLS

#define OGMJOB_TYPE_BIN           (ogmjob_bin_get_type ())
#define OGMJOB_BIN(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMJOB_TYPE_BIN, OGMJobBin))
#define OGMJOB_BIN_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), OGMJOB_TYPE_BIN, OGMJobBinClass))
#define OGMJOB_IS_BIN(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMJOB_TYPE_BIN))
#define OGMJOB_IS_BIN_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMJOB_TYPE_BIN))
#define OGMJOB_BIN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMJOB_TYPE_BIN, OGMJobBinClass))

typedef struct _OGMJobBin      OGMJobBin;
typedef struct _OGMJobBinPriv  OGMJobBinPriv;
typedef struct _OGMJobBinClass OGMJobBinClass;

struct _OGMJobBin
{
  OGMJobContainer parent_instance;

  OGMJobBinPriv *priv;
};

struct _OGMJobBinClass
{
  OGMJobContainerClass parent_class;
};

GType        ogmjob_bin_get_type  (void);

OGMJobTask * ogmjob_bin_get_child (OGMJobBin *bin);

G_END_DECLS

#endif /* __OGMJOB_BIN_H__ */

