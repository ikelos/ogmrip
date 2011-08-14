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

#ifndef __OGMRIP_PROFILE_H__
#define __OGMRIP_PROFILE_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define OGMRIP_PROFILE_PATH "/apps/ogmrip/profiles/"

#define OGMRIP_TYPE_PROFILE          (ogmrip_profile_get_type ())
#define OGMRIP_PROFILE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_PROFILE, OGMRipProfile))
#define OGMRIP_PROFILE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_PROFILE, OGMRipProfileClass))
#define OGMRIP_IS_PROFILE(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_PROFILE))
#define OGMRIP_IS_PROFILE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_PROFILE))

typedef struct _OGMRipProfile        OGMRipProfile;
typedef struct _OGMRipProfilePriv    OGMRipProfilePriv;
typedef struct _OGMRipProfileClass   OGMRipProfileClass;

struct _OGMRipProfile
{
  GSettings parent_instance;

  OGMRipProfilePriv *priv;
};

struct _OGMRipProfileClass
{
  GSettingsClass parent_class;
};

GType           ogmrip_profile_get_type      (void);
OGMRipProfile * ogmrip_profile_new           (const gchar   *name);
OGMRipProfile * ogmrip_profile_new_from_file (GFile         *file,
                                              GError        **error);
gboolean        ogmrip_profile_dump          (OGMRipProfile *profile,
                                              GFile         *file,
                                              GError        **error);
void            ogmrip_profile_get           (OGMRipProfile *profile,
                                              const gchar   *section,
                                              const gchar   *key,
                                              const gchar   *format,
                                              ...);
gboolean        ogmrip_profile_set           (OGMRipProfile *profile,
                                              const gchar   *section,
                                              const gchar   *key,
                                              const gchar   *format,
                                              ...);
GSettings *     ogmrip_profile_get_child     (OGMRipProfile *profile,
                                              const gchar   *name);

G_END_DECLS

#endif /* __OGMRIP_PROFILE_H__ */

