/* OGMRip - A library for DVD ripping and encoding
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

#ifndef __OGMRIP_FS_H__
#define __OGMRIP_FS_H__

#include <glib.h>

#include <sys/stat.h>
#include <sys/types.h>

G_BEGIN_DECLS

#if ! GLIB_CHECK_VERSION(2,8,0)
#define g_chdir chdir
#endif

G_CONST_RETURN
gchar *  ogmrip_fs_get_tmp_dir      (void);
void     ogmrip_fs_set_tmp_dir      (const gchar  *dir);

gboolean ogmrip_fs_mkdir            (const gchar  *path,
                                     mode_t       mode,
                                     GError       **error);
gboolean ogmrip_fs_rmdir            (const gchar  *path,
                                     gboolean     recursive,
                                     GError       **error);

gchar *  ogmrip_fs_mktemp           (const gchar  *tmpl,
                                     GError       **error);
gchar *  ogmrip_fs_mkftemp          (const gchar  *tmpl,
                                     GError       **error);
gchar *  ogmrip_fs_mkdtemp          (const gchar  *tmpl,
                                     GError       **error);
gchar *  ogmrip_fs_lntemp           (const gchar  *oldpath,
                                     const gchar  *newtmpl,
                                     gboolean     symln,
                                     GError       **error);

gint     ogmrip_fs_open_tmp         (const gchar  *tmpl,
                                     gchar        **name_used,
                                     GError       **error);

gint64   ogmrip_fs_get_left_space   (const gchar  *filename,
                                     GError       **error);
gchar *  ogmrip_fs_get_mount_point  (const gchar  *filename,
                                     GError       **error);


void     ogmrip_fs_unref            (gchar        *filename,
                                     gboolean     do_unlink);

gboolean ogmrip_fs_rename           (const gchar  *old_name,
                                     const gchar  *new_name,
                                     GError       **error);

gchar *  ogmrip_fs_get_full_path    (const gchar  *filename);

G_CONST_RETURN
gchar *  ogmrip_fs_get_extension    (const gchar  *filename);
gchar *  ogmrip_fs_set_extension    (const gchar  *filename,
                                     const gchar  *extension);

G_END_DECLS

#endif /* __OGMRIP_FS_H__ */

