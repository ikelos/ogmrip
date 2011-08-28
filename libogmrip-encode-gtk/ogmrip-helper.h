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

#ifndef __OGMRIP_HELPER_H__
#define __OGMRIP_HELPER_H__

#include <ogmdvd.h>
#include <ogmrip.h>

#include <gtk/gtk.h>
#include <sys/types.h>

G_BEGIN_DECLS

#define gtk_builder_get_widget(builder, name) \
    (GtkWidget *) gtk_builder_get_object ((builder), (name))

#if !GTK_CHECK_VERSION(2,24,0)
#define GTK_COMBO_BOX_TEXT(obj) GTK_COMBO_BOX(obj)
#define gtk_combo_box_text_new() gtk_combo_box_new_text()
#define gtk_combo_box_text_append_text(obj,txt) gtk_combo_box_append_text(obj,txt)
#define gtk_combo_box_text_remove(obj,pos) gtk_combo_box_remove_text(obj,pos)
#endif

guint    g_settings_get_uint     (GSettings    *settings,
                                  const gchar  *key);
void     gtk_container_clear     (GtkContainer *container);
gboolean ogmrip_open_title       (GtkWindow    *parent,
                                  OGMDvdTitle  *title);

/*
 * Containers and Codecs
 */

void        ogmrip_container_chooser_construct   (GtkComboBox *chooser);
void        ogmrip_video_codec_chooser_construct (GtkComboBox *chooser);
void        ogmrip_video_codec_chooser_filter    (GtkComboBox *chooser,
                                                  GType       container);
void        ogmrip_audio_codec_chooser_construct (GtkComboBox *chooser);
void        ogmrip_audio_codec_chooser_filter    (GtkComboBox *chooser,
                                                  GType       container);
void        ogmrip_subp_codec_chooser_construct  (GtkComboBox *chooser);
void        ogmrip_subp_codec_chooser_filter     (GtkComboBox *chooser,
                                                  GType       container);
gchar *     ogmrip_codec_chooser_get_active      (GtkComboBox *chooser);
GType       ogmrip_codec_chooser_get_active_type (GtkComboBox *chooser);
void        ogmrip_codec_chooser_set_active      (GtkComboBox *chooser,
                                                  const gchar *codec);

G_END_DECLS

#endif /* __OGMRIP_HELPER_H__ */

