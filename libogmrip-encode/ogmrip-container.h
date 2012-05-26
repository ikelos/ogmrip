/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_CONTAINER_H__
#define __OGMRIP_CONTAINER_H__

#include <ogmrip-job.h>
#include <ogmrip-file.h>
#include <ogmrip-profile.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_CONTAINER           (ogmrip_container_get_type ())
#define OGMRIP_CONTAINER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_CONTAINER, OGMRipContainer))
#define OGMRIP_CONTAINER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_CONTAINER, OGMRipContainerClass))
#define OGMRIP_IS_CONTAINER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_CONTAINER))
#define OGMRIP_IS_CONTAINER_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_CONTAINER))
#define OGMRIP_CONTAINER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_CONTAINER, OGMRipContainerClass))

typedef struct _OGMRipContainer      OGMRipContainer;
typedef struct _OGMRipContainerPriv  OGMRipContainerPriv;
typedef struct _OGMRipContainerClass OGMRipContainerClass;

struct _OGMRipContainer
{
  OGMJobBin parent_instance;

  OGMRipContainerPriv *priv;
};

struct _OGMRipContainerClass
{
  OGMJobBinClass parent_class;

  /* vtable */
  gint (* get_overhead) (OGMRipContainer *container);
};

typedef void (* OGMRipContainerFunc) (OGMRipContainer  *container,
                                      OGMRipFile       *file,
                                      gpointer         data);

GType         ogmrip_container_get_type     (void);
GFile *       ogmrip_container_get_output   (OGMRipContainer     *container);
void          ogmrip_container_set_output   (OGMRipContainer     *container,
                                             GFile               *file);
const gchar * ogmrip_container_get_label    (OGMRipContainer     *container);
void          ogmrip_container_set_label    (OGMRipContainer     *container,
                                             const gchar         *label);
const gchar * ogmrip_container_get_fourcc   (OGMRipContainer     *container);
void          ogmrip_container_set_fourcc   (OGMRipContainer     *container,
                                             const gchar         *fourcc);
gint          ogmrip_container_get_overhead (OGMRipContainer     *container);
void          ogmrip_container_add_file     (OGMRipContainer     *container,
                                             OGMRipFile          *file);
void          ogmrip_container_remove_file  (OGMRipContainer     *container,
                                             OGMRipFile          *file);
GList *       ogmrip_container_get_files    (OGMRipContainer     *container);
OGMRipFile *  ogmrip_container_get_nth_file (OGMRipContainer     *container,
                                             gint                n);
gint          ogmrip_container_get_n_files  (OGMRipContainer     *container);
void          ogmrip_container_foreach_file (OGMRipContainer     *container,
                                             OGMRipContainerFunc func,
                                             gpointer            data);
void          ogmrip_container_set_split    (OGMRipContainer     *container,
                                             guint               number,
                                             guint               size);
void          ogmrip_container_get_split    (OGMRipContainer     *container,
                                             guint               *number,
                                             guint               *size);
glong         ogmrip_container_get_sync     (OGMRipContainer     *container);

void          ogmrip_register_container     (GType               gtype,
                                             const gchar         *name,
                                             const gchar         *description,
                                             OGMRipFormat        *format,
                                             const gchar         *property,
                                             ...);
gboolean      ogmrip_container_contains     (GType               gtype,
                                             OGMRipFormat        format);
gboolean      ogmrip_container_get_schema   (GType               gtype,
                                             const gchar         **id,
                                             const gchar         **name);

G_END_DECLS

#endif /* __OGMRIP_CONTAINER_H__ */

