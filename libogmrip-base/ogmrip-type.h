/* OGMRipBase - A foundation library for OGMRip
 * Copyright (C) 2012 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_TYPE_H__
#define __OGMRIP_TYPE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_TYPE_INFO            (ogmrip_type_info_get_type ())
#define OGMRIP_TYPE_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_TYPE_INFO, OGMRipTypeInfo))
#define OGMRIP_TYPE_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_TYPE_INFO, OGMRipTypeInfoClass))
#define OGMRIP_IS_TYPE_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_TYPE_INFO))
#define OGMRIP_IS_TYPE_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_TYPE_INFO))

typedef struct _OGMRipTypeInfo      OGMRipTypeInfo;
typedef struct _OGMRipTypeInfoClass OGMRipTypeInfoClass;
typedef struct _OGMRipTypeInfoPriv  OGMRipTypeInfoPriv;

struct _OGMRipTypeInfo
{
  GInitiallyUnowned parent_instance;

  OGMRipTypeInfoPriv *priv;
};

struct _OGMRipTypeInfoClass
{
  GInitiallyUnownedClass parent_class;
};

GType            ogmrip_type_info_get_type (void);
OGMRipTypeInfo * ogmrip_type_info_lookup   (GType          gtype);

void             ogmrip_type_register      (GType          gtype,
                                            OGMRipTypeInfo *info);
const gchar *    ogmrip_type_name          (GType          gtype);
const gchar *    ogmrip_type_description   (GType          gtype);
GParamSpec *     ogmrip_type_property      (GType          gtype,
                                            const gchar    *property);
GType            ogmrip_type_from_name     (const gchar    *name);
GType *          ogmrip_type_children      (GType          gtype,
                                            guint          *n);
void             ogmrip_type_add_extension (GType          gtype,
                                            GType          extension);
GType            ogmrip_type_get_extension (GType          gtype,
                                            GType          iface);

G_END_DECLS

#endif /* __OGMRIP_TYPE_H__ */

