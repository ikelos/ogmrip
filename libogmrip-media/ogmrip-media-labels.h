/* OGMRipMedia - A media library for OGMRip
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

#ifndef __OGMRIP_MEDIA_LABELS_H__
#define __OGMRIP_MEDIA_LABELS_H__

#include <ogmrip-media-enums.h>

G_BEGIN_DECLS

const gchar * ogmrip_format_get_label        (OGMRipFormat       format);
const gchar * ogmrip_standard_get_label      (OGMRipStandard     standard);
const gchar * ogmrip_aspect_get_label        (OGMRipAspect       aspect);
const gchar * ogmrip_channels_get_label      (OGMRipChannels     channels);
const gchar * ogmrip_quantization_get_label  (OGMRipQuantization quantization);
const gchar * ogmrip_audio_content_get_label (OGMRipAudioContent content);
const gchar * ogmrip_subp_content_get_label  (OGMRipSubpContent  content);
const gchar * ogmrip_language_get_label      (gint code);
const gchar * ogmrip_language_get_iso639_1   (gint code);
const gchar * ogmrip_language_get_iso639_2   (gint code);

G_END_DECLS

#endif /* __OGMRIP_MEDIA_LABELS_H__ */

