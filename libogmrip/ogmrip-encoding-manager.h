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

#ifndef __OGMRIP_ENCODING_MANAGER_H__
#define __OGMRIP_ENCODING_MANAGER_H__

#include <ogmrip-encoding.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_ENCODING_MANAGER          (ogmrip_encoding_manager_get_type ())
#define OGMRIP_ENCODING_MANAGER(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_ENCODING_MANAGER, OGMRipEncodingManager))
#define OGMRIP_ENCODING_MANAGER_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_ENCODING_MANAGER, OGMRipEncodingManagerClass))
#define OGMRIP_IS_ENCODING_MANAGER(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_ENCODING_MANAGER))
#define OGMRIP_IS_ENCODING_MANAGER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_ENCODING_MANAGER))
#define OGMRIP_ENCODING_MANAGER_ERROR         (ogmrip_encoding_manager_error_quark ())

typedef struct _OGMRipEncodingManager      OGMRipEncodingManager;
typedef struct _OGMRipEncodingManagerPriv  OGMRipEncodingManagerPriv;
typedef struct _OGMRipEncodingManagerClass OGMRipEncodingManagerClass;

struct _OGMRipEncodingManager
{
  GObject parent_instance;

  OGMRipEncodingManagerPriv *priv;
};

struct _OGMRipEncodingManagerClass
{
  GObjectClass parent_class;
};

/**
 * OGMRipCleanupType:
 * @OGMRIP_CLEANUP_REMOVE_ALL: Remove the temporary files of all encodings
 * @OGMRIP_CLEANUP_KEEP_ALL: Keep the temporary files of all encodings
 * @OGMRIP_CLEANUP_KEEP_LAST: Keep the temporary files of the last encoding only
 *
 * How to clean up the temporary files of the encodings.
 */
typedef enum
{
  OGMRIP_CLEANUP_REMOVE_ALL,
  OGMRIP_CLEANUP_KEEP_ALL,
  OGMRIP_CLEANUP_KEEP_LAST
} OGMRipCleanupType;

/**
 * OGMRipEncodingFunc:
 * @encoding: An #OGMRipEncoding
 * @data: The user data
 *
 * Specifies the type of functions passed to ogmrip_encoding_manager_foreach().
 *
 * Returns: %FALSE to stop calling the function.
 */
typedef gboolean (* OGMRipEncodingFunc) (OGMRipEncoding *encoding,
                                         gpointer       data);

GType                   ogmrip_encoding_manager_get_type    (void);

OGMRipEncodingManager * ogmrip_encoding_manager_new         (void);

gint                    ogmrip_encoding_manager_run         (OGMRipEncodingManager *manager,
                                                             GError                **error);
void                    ogmrip_encoding_manager_cancel      (OGMRipEncodingManager *manager);

void                    ogmrip_encoding_manager_add         (OGMRipEncodingManager *manager,
                                                             OGMRipEncoding        *encoding);
void                    ogmrip_encoding_manager_remove      (OGMRipEncodingManager *manager,
                                                             OGMRipEncoding        *encoding);

gboolean                ogmrip_encoding_manager_foreach     (OGMRipEncodingManager *manager,
                                                             OGMRipEncodingFunc    func,
                                                             gpointer              data);

OGMRipEncoding *        ogmrip_encoding_manager_find        (OGMRipEncodingManager *manager,
                                                             OGMRipEncodingFunc    func,
                                                             gpointer              data);

OGMRipEncoding *        ogmrip_encoding_manager_nth         (OGMRipEncodingManager *manager,
                                                             gint                  n);

void                    ogmrip_encoding_manager_set_cleanup (OGMRipEncodingManager *manager,
                                                             OGMRipCleanupType     type);
gint                    ogmrip_encoding_manager_get_cleanup (OGMRipEncodingManager *manager);

G_END_DECLS

#endif /* __OGMRIP_ENCODING_MANAGER_H__ */

