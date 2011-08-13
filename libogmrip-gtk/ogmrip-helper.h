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

#define GTK_BOX_CHILD(b) ((GtkBoxChild *) (b))

/*
 * GLib
 */

gchar * g_get_locale (gint category);

/*
 * GObject
 */

/**
 * g_signal_connect_while_alive:
 * @instance: the instance to connect to
 * @detailed_signal: a string of the form "signal-name::detail"
 * @c_handler: the #GCallback to connect
 * @alive: the instance to check for
 *
 * Connects a #GCallback function to a signal for a particular object automatically
 * disconnecting it when @alive is destroyed.
 *
 * Returns: the handler id
 */
#define g_signal_connect_while_alive(instance, detailed_signal, c_handler, alive) \
    g_signal_connect_data_while_alive ((instance), (detailed_signal), (c_handler), (alive), NULL, (GConnectFlags) 0)

/**
 * g_signal_connect_swapped_while_alive:
 * @instance: the instance to connect to
 * @detailed_signal: a string of the form "signal-name::detail"
 * @c_handler: the #GCallback to connect
 * @alive: the instance to check for
 *
 * Connects a #GCallback function to a signal for a particular object automatically
 * disconnecting it when @alive is destroyed.
 *
 * Returns: the handler id
 */
#define g_signal_connect_swapped_while_alive(instance, detailed_signal, c_handler, alive) \
    g_signal_connect_data_while_alive ((instance), (detailed_signal), (c_handler), (alive), NULL, G_CONNECT_SWAPPED)

gulong g_signal_connect_data_while_alive (gpointer       instance,
                                          const gchar    *detailed_signal,
                                          GCallback      c_handler,
                                          gpointer       alive,
                                          GClosureNotify destroy_data,
                                          GConnectFlags  connect_flags);

/*
 * Gio
 */

guint g_settings_get_uint (GSettings   *settings,
                           const gchar *key);
/*
 * Gtk+
 */

#define gtk_builder_get_widget(builder, name) \
    (GtkWidget *) gtk_builder_get_object ((builder), (name))

void        gtk_window_set_parent           (GtkWindow        *window, 
                                             GtkWindow        *parent);
void        gtk_window_set_icon_from_stock  (GtkWindow        *window,
                                             const gchar      *stock_id);
gint        gtk_radio_button_get_active     (GtkRadioButton   *radio);
void        gtk_radio_button_set_active     (GtkRadioButton   *radio, 
                                             guint            index);
gboolean    gtk_tree_model_iter_prev        (GtkTreeModel     *tree_model,
                                             GtkTreeIter      *iter);
void        gtk_label_set_int               (GtkLabel         *label,
                                             gint             value);
gint        gtk_label_get_int               (GtkLabel         *label);
void        gtk_container_clear             (GtkContainer     *container);
GtkWidget * gtk_box_get_nth_child           (GtkBox           *box,
                                             gint             n);
void        gtk_table_append                (GtkTable         *table,
                                             GtkWidget        *widget,
                                             GtkAttachOptions xoptions,
                                             GtkAttachOptions yoptions,
                                             guint            xpadding,
                                             guint            ypadding);
void        gtk_dialog_set_response_visible (GtkDialog        *dialog,
                                             gint             response_id,
                                             gboolean         setting);
void        gtk_dialog_response_accept      (GtkDialog        *dialog);

GtkTreeRowReference * gtk_tree_model_get_row_reference (GtkTreeModel *model,
                                                        GtkTreeIter  *iter);
gboolean              gtk_tree_row_reference_get_iter  (GtkTreeRowReference *ref,
                                                        GtkTreeIter         *iter);

#if GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION < 24
#define GTK_COMBO_BOX_TEXT(obj) GTK_COMBO_BOX(obj)
#define gtk_combo_box_text_new() gtk_combo_box_new_text()
#define gtk_combo_box_text_append_text(obj,txt) gtk_combo_box_append_text(obj,txt)
#define gtk_combo_box_text_remove(obj,pos) gtk_combo_box_remove_text(obj,pos)
#endif

/*
 * OGMRip
 */

void ogmrip_run_error_dialog (GtkWindow   *parent,
                              GError      *error,
                              const gchar *format,
                              ...);

/*
 * OGMDvd
 */

gboolean ogmrip_open_title (GtkWindow   *parent,
                            OGMDvdTitle *title);

/*
 * Languages
 */

GtkWidget * ogmrip_language_chooser_new             (void);
void        ogmrip_language_chooser_construct       (GtkComboBox *chooser);
guint       ogmrip_language_chooser_get_active      (GtkComboBox *chooser);
void        ogmrip_language_chooser_set_active      (GtkComboBox *chooser,
                                                     guint       language);

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

/*
 * Profiles
 */

void            ogmrip_profile_viewer_construct   (GtkTreeView   *viewer);
void            ogmrip_profile_chooser_construct  (GtkComboBox   *chooser);
void            ogmrip_profile_store_add          (GtkListStore  *store,
                                                   GtkTreeIter   *iter,
                                                   OGMRipProfile *profile);
gchar *         ogmrip_profile_store_get_name     (GtkListStore  *store,
                                                   GtkTreeIter   *iter);
void            ogmrip_profile_store_set_name     (GtkListStore  *store,
                                                   GtkTreeIter   *iter,
                                                   const gchar   *name);
OGMRipProfile * ogmrip_profile_store_get_profile  (GtkListStore  *store,
                                                   GtkTreeIter   *iter);
OGMRipProfile * ogmrip_profile_chooser_get_active (GtkComboBox   *chooser);

const gchar *   ogmrip_get_system_profiles_dir   (void);
const gchar *   ogmrip_get_user_profiles_dir     (void);

G_END_DECLS

#endif /* __OGMRIP_HELPER_H__ */

