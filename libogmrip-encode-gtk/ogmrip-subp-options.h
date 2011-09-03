/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_SUBP_OPTIONS_H__
#define __OGMRIP_SUBP_OPTIONS_H__

#include <ogmrip-encode.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_SUBP_OPTIONS            (ogmrip_subp_options_get_type ())
#define OGMRIP_SUBP_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_SUBP_OPTIONS, OGMRipSubpOptions))
#define OGMRIP_IS_SUBP_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_SUBP_OPTIONS))
#define OGMRIP_SUBP_OPTIONS_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMRIP_TYPE_SUBP_OPTIONS, OGMRipSubpOptionsInterface))

typedef struct _OGMRipSubpOptions          OGMRipSubpOptions;
typedef struct _OGMRipSubpOptionsInterface OGMRipSubpOptionsInterface;

struct _OGMRipSubpOptionsInterface
{
  GTypeInterface base_iface;
};

GType         ogmrip_subp_options_get_type        (void);
OGMRipCharset ogmrip_subp_options_get_charset     (OGMRipSubpOptions *options);
void          ogmrip_subp_options_set_charset     (OGMRipSubpOptions *options,
                                                   OGMRipCharset     charset);
GType         ogmrip_subp_options_get_codec       (OGMRipSubpOptions *dialog);
void          ogmrip_subp_options_set_codec       (OGMRipSubpOptions *dialog,
                                                   GType             type);
gboolean      ogmrip_subp_options_get_forced_only (OGMRipSubpOptions *options);
void          ogmrip_subp_options_set_forced_only (OGMRipSubpOptions *options,
                                                   gboolean          forced_only);
OGMRipNewline ogmrip_subp_options_get_newline     (OGMRipSubpOptions *options);
void          ogmrip_subp_options_set_newline     (OGMRipSubpOptions *options,
                                                   OGMRipNewline     newline);
gboolean      ogmrip_subp_options_get_spell_check (OGMRipSubpOptions *dialog);
void          ogmrip_subp_options_set_spell_check (OGMRipSubpOptions *dialog,
                                                   gboolean          spell_check);

G_END_DECLS

#endif /* __OGMRIP_SUBP_OPTIONS_H__ */

