/* OGMRipBase - A foundation library for OGMRip
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_FS_H__
#define __OGMRIP_FS_H__

#include <glib.h>

#include <sys/stat.h>
#include <sys/types.h>

G_BEGIN_DECLS

const gchar * ogmrip_fs_get_tmp_dir      (void);
void          ogmrip_fs_set_tmp_dir      (const gchar  *dir);

gboolean      ogmrip_fs_rmdir            (const gchar  *path,
                                          gboolean     recursive,
                                          GError       **error);

gchar *       ogmrip_fs_mktemp           (const gchar  *tmpl,
                                          GError       **error);
gchar *       ogmrip_fs_mkftemp          (const gchar  *tmpl,
                                          GError       **error);

gint          ogmrip_fs_open_tmp         (const gchar  *tmpl,
                                          gchar        **name_used,
                                          GError       **error);

const gchar * ogmrip_fs_get_extension    (const gchar  *filename);
gchar *       ogmrip_fs_set_extension    (const gchar  *filename,
                                          const gchar  *extension);

G_END_DECLS

#endif /* __OGMRIP_FS_H__ */

