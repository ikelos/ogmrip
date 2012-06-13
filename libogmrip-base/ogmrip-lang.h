/* OGMRipBase - A foundation library for OGMRip
 * Copyright (C) 2010-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_LANG_H__
#define __OGMRIP_LANG_H__

#include <glib.h>

G_BEGIN_DECLS

enum
{
  OGMRIP_LANGUAGE_ISO639_1,
  OGMRIP_LANGUAGE_ISO639_2,
  OGMRIP_LANGUAGE_NAME
};

const gchar * ogmrip_language_to_name       (gint        code);
gint          ogmrip_language_from_name     (const gchar *name);
const gchar * ogmrip_language_to_iso639_1   (gint        code);
gint          ogmrip_language_from_iso639_1 (const gchar *iso639_1);
const gchar * ogmrip_language_to_iso639_2   (gint        code);
gint          ogmrip_language_from_iso639_2 (const gchar *iso639_2);

G_END_DECLS

#endif /* __OGMRIP_LANG_H__ */

