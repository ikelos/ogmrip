/* OGMRip - A wrapper library around libdvdread
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

#include "ogmrip-subp-chooser-widget.h"
#include "ogmrip-subp-file-chooser-dialog.h"

G_DEFINE_TYPE (OGMRipSubpChooserWidget, ogmrip_subp_chooser_widget, OGMRIP_TYPE_SOURCE_CHOOSER_WIDGET);

static void
ogmrip_subp_chooser_widget_class_init (OGMRipSubpChooserWidgetClass *klass)
{
}

static void
ogmrip_subp_chooser_widget_init (OGMRipSubpChooserWidget *chooser)
{
}

GtkWidget *
ogmrip_subp_chooser_widget_new (void)
{
  return g_object_new (OGMRIP_TYPE_SUBP_CHOOSER_WIDGET,
      "dialog", ogmrip_subp_file_chooser_dialog_new (), NULL);
}

