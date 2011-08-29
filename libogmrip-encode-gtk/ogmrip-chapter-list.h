/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __OGMRIP_CHAPTER_LIST_H__
#define __OGMRIP_CHAPTER_LIST_H__

#include <ogmrip-dvd-gtk.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_CHAPTER_LIST            (ogmrip_chapter_list_get_type ())
#define OGMRIP_CHAPTER_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_CHAPTER_LIST, OGMRipChapterList))
#define OGMRIP_CHAPTER_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_CHAPTER_LIST, OGMRipChapterListClass))
#define OGMRIP_IS_CHAPTER_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_CHAPTER_LIST))
#define OGMRIP_IS_CHAPTER_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_CHAPTER_LIST))

typedef struct _OGMRipChapterList      OGMRipChapterList;
typedef struct _OGMRipChapterListClass OGMRipChapterListClass;

struct _OGMRipChapterList
{
  OGMDvdChapterList widget;
};

struct _OGMRipChapterListClass
{
  OGMDvdChapterListClass parent_class;

  void (* selection_changed) (OGMRipChapterList *list);
};

GType       ogmrip_chapter_list_get_type     (void);
GtkWidget * ogmrip_chapter_list_new          (void);

void        ogmrip_chapter_list_select_all   (OGMRipChapterList *list);
void        ogmrip_chapter_list_deselect_all (OGMRipChapterList *list);

gboolean    ogmrip_chapter_list_get_selected (OGMRipChapterList *list,
                                              guint             *start_chapter,
                                              gint              *end_chapter);

G_END_DECLS

#endif /* __OGMRIP_CHAPTER_LIST_H__ */

