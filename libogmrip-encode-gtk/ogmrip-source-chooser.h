/* OGMRip - A wrapper library around libdvdread
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

#ifndef __OGMRIP_SOURCE_CHOOSER_H__
#define __OGMRIP_SOURCE_CHOOSER_H__

#include <ogmdvd.h>
#include <ogmrip.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * OGMRipSourceType:
 * @OGMRIP_SOURCE_INVALID: This is not a valid source
 * @OGMRIP_SOURCE_NONE: No source is selected
 * @OGMRIP_SOURCE_STREAM: The source is a DVD stream
 * @OGMRIP_SOURCE_FILE: The source is a file
 *
 * The type of a source
 */
typedef enum
{
  OGMRIP_SOURCE_INVALID = -1,
  OGMRIP_SOURCE_NONE,
  OGMRIP_SOURCE_STREAM,
  OGMRIP_SOURCE_FILE
} OGMRipSourceType;

typedef union
{
  OGMDvdStream *stream;
  OGMRipFile *file;
} OGMRipSource;

#define OGMRIP_TYPE_SOURCE_CHOOSER            (ogmrip_source_chooser_get_type ())
#define OGMRIP_SOURCE_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_SOURCE_CHOOSER, OGMRipSourceChooser))
#define OGMRIP_IS_SOURCE_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_SOURCE_CHOOSER))
#define OGMRIP_SOURCE_CHOOSER_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMRIP_TYPE_SOURCE_CHOOSER, OGMRipSourceChooserInterface))

typedef struct _OGMRipSourceChooser          OGMRipSourceChooser;
typedef struct _OGMRipSourceChooserInterface OGMRipSourceChooserInterface;

struct _OGMRipSourceChooserInterface
{
  GTypeInterface base_iface;

  OGMRipSource * (* get_active)      (OGMRipSourceChooser *chooser,
                                      OGMRipSourceType    *type);
  void           (* set_active)      (OGMRipSourceChooser *chooser,
                                      OGMRipSource        *source);
  void           (* select_language) (OGMRipSourceChooser *chooser,
                                      gint                language);
};

GType          ogmrip_source_chooser_get_type        (void);
void           ogmrip_source_chooser_set_title       (OGMRipSourceChooser *chooser,
                                                      OGMDvdTitle         *title);
OGMDvdTitle *  ogmrip_source_chooser_get_title       (OGMRipSourceChooser *chooser);
OGMRipSource * ogmrip_source_chooser_get_active      (OGMRipSourceChooser *chooser,
                                                      OGMRipSourceType    *type);
void           ogmrip_source_chooser_set_active      (OGMRipSourceChooser *chooser,
                                                      OGMRipSource        *source);
void           ogmrip_source_chooser_select_language (OGMRipSourceChooser *chooser,
                                                      gint                language);

G_END_DECLS

#endif /* __OGMRIP_SOURCE_CHOOSER_H__ */

