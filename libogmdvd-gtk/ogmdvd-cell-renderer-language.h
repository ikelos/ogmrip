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

#ifndef __OGMDVD_CELL_RENDERER_LANGUAGE_H__
#define __OGMDVD_CELL_RENDERER_LANGUAGE_H__

#include <gtk/gtk.h>

#define OGMDVD_TYPE_CELL_RENDERER_LANGUAGE             (ogmdvd_cell_renderer_language_get_type())
#define OGMDVD_CELL_RENDERER_LANGUAGE(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),  OGMDVD_TYPE_CELL_RENDERER_LANGUAGE, OGMDvdCellRendererLanguage))
#define OGMDVD_CELL_RENDERER_LANGUAGE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  OGMDVD_TYPE_CELL_RENDERER_LANGUAGE, OGMDvdCellRendererLanguageClass))
#define OGMDVD_IS_CELL_RENDERER_LANGUAGE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMDVD_TYPE_CELL_RENDERER_LANGUAGE))
#define OGMDVD_IS_CELL_RENDERER_LANGUAGE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  OGMDVD_TYPE_CELL_RENDERER_LANGUAGE))
#define OGMDVD_CELL_RENDERER_LANGUAGE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  OGMDVD_TYPE_CELL_RENDERER_LANGUAGE, OGMDvdCellRendererLanguageClass))

typedef struct _OGMDvdCellRendererLanguage      OGMDvdCellRendererLanguage;
typedef struct _OGMDvdCellRendererLanguageClass OGMDvdCellRendererLanguageClass;
typedef struct _OGMDvdCellRendererLanguagePriv  OGMDvdCellRendererLanguagePriv;

struct _OGMDvdCellRendererLanguage
{
  GtkCellRendererText parent;

  OGMDvdCellRendererLanguagePriv *priv;
};

struct _OGMDvdCellRendererLanguageClass
{
  GtkCellRendererTextClass parent_class;
};

GType             ogmdvd_cell_renderer_language_get_type (void);
GtkCellRenderer * ogmdvd_cell_renderer_language_new      (void);

#endif /* __OGMDVD_CELL_RENDERER_LANGUAGE_H__ */

