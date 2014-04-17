/* OGMRip - A media encoder for GNOME
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_SETTINGS_H__
#define __OGMRIP_SETTINGS_H__

#include <glib.h>

G_BEGIN_DECLS

#define OGMRIP_SETTINGS_PROFILE      "profile"
#define OGMRIP_SETTINGS_OUTPUT_DIR   "output-directory"
#define OGMRIP_SETTINGS_FILENAME     "filename"
#define OGMRIP_SETTINGS_PREF_AUDIO   "prefered-audio-language"
#define OGMRIP_SETTINGS_PREF_SUBP    "prefered-subp-language"
#define OGMRIP_SETTINGS_CHAPTER_LANG "chapters-language"
#define OGMRIP_SETTINGS_TMP_DIR      "temporary-directory"
#define OGMRIP_SETTINGS_COPY_DVD     "copy-dvd"
#define OGMRIP_SETTINGS_AFTER_ENC    "after-encoding"
#define OGMRIP_SETTINGS_KEEP_TMP     "keep-temporary-files"
#define OGMRIP_SETTINGS_LOG_OUTPUT   "create-log-file"
#define OGMRIP_SETTINGS_THREADS      "threads"
#define OGMRIP_SETTINGS_AUTO_SUBP    "auto-subp"

enum
{
  OGMRIP_AFTER_ENC_REMOVE,
  OGMRIP_AFTER_ENC_KEEP,
  OGMRIP_AFTER_ENC_ASK
};

G_END_DECLS

#endif /* __OGMRIP_SETTINGS_H__ */

