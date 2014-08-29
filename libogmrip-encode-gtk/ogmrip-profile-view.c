/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2010-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-profile-view.h"

#include <glib/gi18n-lib.h>

struct _OGMRipProfileViewPriv
{
  OGMRipProfileStore *store;
};

static void ogmrip_profile_view_dispose (GObject *gobject);

G_DEFINE_TYPE_WITH_PRIVATE (OGMRipProfileView, ogmrip_profile_view, GTK_TYPE_TREE_VIEW);

static void
ogmrip_profile_view_class_init (OGMRipProfileViewClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) klass;
  gobject_class->dispose = ogmrip_profile_view_dispose;
}

static void
ogmrip_profile_view_init (OGMRipProfileView *view)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  view->priv = ogmrip_profile_view_get_instance_private (view);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

  view->priv->store = ogmrip_profile_store_new (NULL, TRUE);
  gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (view->priv->store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Profile"),
      renderer, "markup", OGMRIP_PROFILE_STORE_INFO_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
}

static void
ogmrip_profile_view_dispose (GObject *gobject)
{
  OGMRipProfileView *view = OGMRIP_PROFILE_VIEW (gobject);

  if (view->priv->store)
  {
    g_object_unref (view->priv->store);
    view->priv->store = NULL;
  }

  (*G_OBJECT_CLASS (ogmrip_profile_view_parent_class)->dispose) (gobject);
}

GtkWidget *
ogmrip_profile_view_new (void)
{
  return g_object_new (OGMRIP_TYPE_PROFILE_VIEW, NULL);
}

