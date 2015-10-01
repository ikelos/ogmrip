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

#ifndef __OGMRIP_CLI_H__
#define __OGMRIP_CLI_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_CLI            (ogmrip_cli_get_type ())
#define OGMRIP_CLI(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_CLI, OGMRipCli))
#define OGMRIP_CLI_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_CLI, OGMRipCliClass))
#define OGMRIP_IS_CLI(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_CLI))
#define OGMRIP_IS_CLI_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_CLI))

typedef struct _OGMRipCli      OGMRipCli;
typedef struct _OGMRipCliClass OGMRipCliClass;
typedef struct _OGMRipCliPriv  OGMRipCliPrivate;

struct _OGMRipCli
{
  GApplication parent_instance;

  OGMRipCliPrivate *priv;
};

struct _OGMRipCliClass
{
  GApplicationClass parent_class;
};

GType          ogmrip_cli_get_type (void);
GApplication * ogmrip_cli_new      (const gchar *app_id);
void           ogmrip_cli_cancel   (OGMRipCli   *cli);

G_END_DECLS

#endif /* __OGMRIP_CLI_H__ */

