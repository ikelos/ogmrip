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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/**
 * SECTION:ogmrip-helper
 * @title: Helper
 * @include: ogmrip-source-chooser.h
 * @short_description: A list of helper functions
 */

#include "ogmrip-helper.h"
#include "ogmrip-plugin.h"
#include "ogmrip-container.h"

#include <glib/gi18n.h>

#include <stdlib.h>
#include <locale.h>

extern const gchar *ogmdvd_languages[][3];
extern const guint  ogmdvd_nlanguages;

/**
 * g_get_locale:
 * @category: A pointer to store the type of the chooser
 *
 * Returns the active source and its type.
 *
 * Returns: The active #OGMRipSource
 */
gchar *
g_get_locale (gint category)
{
  gchar *locale;

  locale = setlocale (category, NULL);
  if (locale)
    return g_strdup (locale);

  return NULL;
}

typedef struct
{
  gpointer instance;
  gulong handler;
} GConnectInfo;

static void
g_signal_instance_destroyed (GConnectInfo *info, GObject *object)
{
  g_signal_handler_disconnect (info->instance, info->handler);

  g_free (info);
}

/**
 * g_signal_connect_data_while_alive:
 * @instance: the instance to connect to
 * @detailed_signal: a string of the form "signal-name::detail"
 * @c_handler: the #GCallback to connect
 * @alive: the instance to check for
 * @destroy_data: a #GClosureNotify for data
 * @connect_flags: a combination of #GConnectFlags
 *
 * Connects a #GCallback function to a signal for a particular object automatically
 * disconnecting it when @alive is destroyed.
 *
 * Returns: the handler id
 */
gulong
g_signal_connect_data_while_alive (gpointer instance, const gchar *detailed_signal,
    GCallback c_handler, gpointer alive, GClosureNotify destroy_data, GConnectFlags connect_flags)
{
  GConnectInfo *info;

  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE (instance), 0);
  g_return_val_if_fail (detailed_signal != NULL, 0);
  g_return_val_if_fail (c_handler != NULL, 0);
  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE (alive), 0);

  info = g_new0 (GConnectInfo, 1);
  info->instance = instance;

  info->handler = g_signal_connect_data (instance, detailed_signal, c_handler, alive, destroy_data, connect_flags);

  g_object_weak_ref (alive, (GWeakNotify) g_signal_instance_destroyed, info);

  return info->handler;
}

/**
 * gtk_window_set_parent:
 * @window: A #GtkWindow
 * @parent: The parent window
 *
 * Sets a parent window for a window. This is equivalent to calling
 * gtk_window_set_transient_for(),
 * gtk_window_set_position(),
 * gtk_window_set_gravity(), and
 * gtk_window_set_destroy_with_parent() on @window.
 */
void
gtk_window_set_parent (GtkWindow *window, GtkWindow *parent)
{
  g_return_if_fail (GTK_IS_WINDOW (window));
  g_return_if_fail (GTK_IS_WINDOW (parent));
  g_return_if_fail (window != parent);

  gtk_window_set_transient_for (window, parent);
  gtk_window_set_position (window, GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_gravity (window, GDK_GRAVITY_CENTER);
  gtk_window_set_destroy_with_parent (window, TRUE);
}

/**
 * gtk_window_set_icon_from_stock:
 * @window: A #GtkWindow
 * @stock_id: the name of the stock item
 *
 * Sets the icon of @window from a stock item.
 */
void
gtk_window_set_icon_from_stock (GtkWindow *window, const gchar *stock_id)
{
  GdkPixbuf *pixbuf;

  g_return_if_fail (GTK_IS_WINDOW (window));
  g_return_if_fail (stock_id && *stock_id);
  
  pixbuf = gtk_widget_render_icon (GTK_WIDGET (window), stock_id, GTK_ICON_SIZE_DIALOG, NULL);
  gtk_window_set_icon (window, pixbuf);
  g_object_unref (pixbuf);
}

/**
 * gtk_radio_button_get_active:
 * @radio: Any #GtkRadioButton of the group
 *
 * Returns the index of the active #GtkRadioButton.
 *
 * Returns: An integer, or -1
 */
gint
gtk_radio_button_get_active (GtkRadioButton *radio)
{
  GSList *link;
  gint i;

  g_return_val_if_fail (GTK_IS_RADIO_BUTTON (radio), -1);

  link = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));

  for (i = g_slist_length (link) - 1; link; i--, link = link->next)
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (link->data)))
      return i;

  return -1;
}

/**
 * gtk_radio_button_set_active:
 * @radio: Any #GtkRadioButton of the group
 * @index: The index of the active item
 *
 * Sets the active item of the radio group.
 */
void
gtk_radio_button_set_active (GtkRadioButton *radio, guint index)
{
  GSList *link;
  guint i;

  g_return_if_fail (GTK_IS_RADIO_BUTTON (radio));

  link = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
  for (i = g_slist_length (link) - 1;  link; i--, link = link->next)
    if (i == index)
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (link->data), TRUE);
}

/**
 * gtk_tree_model_iter_prev:
 * @tree_model: A #GtkTreeModel
 * @iter: The #GtkTreeIter
 *
 * Sets @iter to point to the node preceding it at the current level.
 * If there is no previous @iter, %FALSE is returned and @iter is set to be invalid.
 *
 * Returns: %TRUE, if @iter has been changed to the previous node
 */
gboolean
gtk_tree_model_iter_prev (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  gboolean retval = FALSE;
  GtkTreePath *path;

  path = gtk_tree_model_get_path (tree_model, iter);
  if (path)
  {
    if (gtk_tree_path_prev (path))
      retval = gtk_tree_model_get_iter (tree_model, iter, path);
    gtk_tree_path_free (path);
  }

  return retval;
}

GtkTreeRowReference *
gtk_tree_model_get_row_reference (GtkTreeModel *model, GtkTreeIter *iter)
{
  GtkTreePath *path;
  GtkTreeRowReference *ref;

  path = gtk_tree_model_get_path (model, iter);
  ref = gtk_tree_row_reference_new (model, path);
  gtk_tree_path_free (path);

  return ref;
}

gboolean
gtk_tree_row_reference_get_iter (GtkTreeRowReference *ref, GtkTreeIter *iter)
{
  GtkTreeModel *model;
  GtkTreePath *path;
  gboolean retval;

  model = gtk_tree_row_reference_get_model (ref);

  path = gtk_tree_row_reference_get_path (ref);
  if (!path)
    return FALSE;

  retval = gtk_tree_model_get_iter (model, iter, path);

  gtk_tree_path_free (path);

  return retval;
}

/**
 * gtk_label_set_int:
 * @label: A #GtkLabel
 * @value: An integer
 *
 * Sets the value of a #GtkLabel widget.
 */
void
gtk_label_set_int (GtkLabel *label, gint value)
{
  gchar *text;

  g_return_if_fail (GTK_IS_LABEL (label));

  text = g_strdup_printf ("%d", value);
  gtk_label_set_text (label, text);
  g_free (text);
}

/**
 * gtk_label_get_int:
 * @label: A #GtkLabel
 *
 * Gets the value of the @label represented as an integer.
 *
 * Returns: The value of the @label widget
 */
gint
gtk_label_get_int (GtkLabel *label)
{
  const gchar *text;

  g_return_val_if_fail (GTK_IS_LABEL (label), G_MININT);

  text = gtk_label_get_text (label);
  
  return atoi (text);
}

/**
 * gtk_box_get_nth_child:
 * @box: A #GtkBox
 * @n: The index of the desired child
 *
 * Returns the @n'th item in @box.
 *
 * Returns: The nth #GtkWidget, or NULL
 */
GtkWidget *
gtk_box_get_nth_child (GtkBox *box, gint n)
{
  GList *children, *link;
  GtkWidget *child;

  g_return_val_if_fail (GTK_IS_BOX (box), NULL);

  children = gtk_container_get_children (GTK_CONTAINER (box));
  if (!children)
    return NULL;

  if (n < 0)
    link = g_list_last (children);
  else
    link = g_list_nth (children, n);

  child = link->data;

  g_list_free (children);

  return child;
}

/**
 * gtk_dialog_set_response_visible:
 * @dialog: a #GtkDialog
 * @response_id: a response ID
 * @setting: %TRUE for visible
 *
 * Sets the <literal>visible</literal> property of 
 * each widget in the dialog's action area with the given @response_id.
 * A convenient way to show/hide dialog buttons.
 */
void
gtk_dialog_set_response_visible (GtkDialog *dialog, gint response_id, gboolean setting)
{
  GList *children, *child;
  GtkWidget *area, *widget;
  gint rid;

  g_return_if_fail (GTK_IS_DIALOG (dialog));

  area = gtk_dialog_get_action_area (dialog);

  children = gtk_container_get_children (GTK_CONTAINER (area));
  for (child = children; child; child = child->next)
  {
    widget = child->data;

    rid = gtk_dialog_get_response_for_widget (dialog, widget);
    if (rid == response_id)
      g_object_set (widget, "visible", setting, NULL);
  }

  g_list_free (children);
}

/**
 * gtk_dialog_response_accept:
 * @dialog: a #GtkDialog
 *
 * Emits the "response" signal with #GTK_RESPONSE_ACCEPT.
 */
void
gtk_dialog_response_accept (GtkDialog *dialog)
{
  gtk_dialog_response (dialog, GTK_RESPONSE_ACCEPT);
}

/**
 * ogmrip_statusbar_push:
 * @statusbar: A #GtkStatusbar
 * @text: The message to add to the statusbar
 *
 * Pushes a new message onto a statusbar's stack using the default
 * context identifier.
 */
void
ogmrip_statusbar_push (GtkStatusbar *statusbar, const gchar *text)
{
  guint id;

  g_return_if_fail (GTK_IS_STATUSBAR (statusbar));
  g_return_if_fail (text != NULL);

  id = gtk_statusbar_get_context_id (statusbar, "__menu_hint__");
  gtk_statusbar_push (statusbar, id, text);
}

/**
 * ogmrip_statusbar_pop:
 * @statusbar: A #GtkStatusbar
 *
 * Removes the message at the top of a GtkStatusBar's stack using the
 * default context identifier.
 */
void
ogmrip_statusbar_pop (GtkStatusbar *statusbar)
{
  guint id;

  g_return_if_fail (GTK_IS_STATUSBAR (statusbar));

  id = gtk_statusbar_get_context_id (statusbar, "__menu_hint__");
  gtk_statusbar_pop (statusbar, id);
}

/**
 * ogmrip_message_dialog_newv:
 * @parent: A #GtkWindow
 * @type: A #GtkMessageType
 * @format: printf()-style format string, or NULL
 * @args: Arguments for @format
 *
 * Creates a new message dialog, which is a simple dialog with an icon
 * indicating the dialog type (error, warning, etc.) and some text the user may
 * want to see.
 *
 * Returns: A new #GtkMessageDialog
 */
GtkWidget *
ogmrip_message_dialog_newv (GtkWindow *parent, GtkMessageType type, const gchar *format, va_list args)
{
  GtkWidget *dialog = NULL;
  GtkButtonsType buttons = GTK_BUTTONS_NONE;
  const gchar *stock_id = NULL;
  gchar *message;

  g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), NULL);

  switch (type)
  {
    case GTK_MESSAGE_ERROR:
      buttons = GTK_BUTTONS_CLOSE;
      stock_id = GTK_STOCK_DIALOG_ERROR;
      break;
    case GTK_MESSAGE_QUESTION:
      buttons = GTK_BUTTONS_YES_NO;
      stock_id = GTK_STOCK_DIALOG_QUESTION;
      break;
    case GTK_MESSAGE_INFO:
      buttons = GTK_BUTTONS_CLOSE;
      stock_id = GTK_STOCK_DIALOG_INFO;
      break;
    case GTK_MESSAGE_WARNING:
      buttons = GTK_BUTTONS_CLOSE;
      stock_id = GTK_STOCK_DIALOG_WARNING;
      break;
    default:
      break;
  }

  dialog = gtk_message_dialog_new (parent,
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, type, buttons, NULL);

  if (!dialog)
    return NULL;

  message = g_strdup_vprintf (format, args);
  gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog), message);
  g_free (message);

  if (stock_id)
    gtk_window_set_icon_from_stock (GTK_WINDOW (dialog), stock_id);

  gtk_window_set_gravity (GTK_WINDOW (dialog), GDK_GRAVITY_CENTER);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);

  return dialog;
}

/**
 * ogmrip_message_dialog_new:
 * @parent: A #GtkWindow
 * @type: A #GtkMessageType
 * @format: printf()-style format string, or NULL
 * @...: Arguments for @format
 *
 * Creates a new message dialog, which is a simple dialog with an icon
 * indicating the dialog type (error, warning, etc.) and some text the user may
 * want to see.
 *
 * Returns: A new #GtkMessageDialog
 */
GtkWidget *
ogmrip_message_dialog_new (GtkWindow *parent, GtkMessageType type, const gchar *format, ...)
{
  GtkWidget *dialog;
  va_list args;

  g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), NULL);

  va_start (args, format);
  dialog = ogmrip_message_dialog_newv (parent, type, format, args);
  va_end (args);

  return dialog;
}

/**
 * ogmrip_message_dialog:
 * @parent: A #GtkWindow
 * @type: A #GtkMessageType
 * @format: printf()-style format string, or NULL
 * @...: Arguments for @format
 *
 * Creates and displays a new message dialog, which is a simple dialog with an
 * icon indicating the dialog type (error, warning, etc.) and some text the user
 * may want to see.
 *
 * Returns: The response ID
 */
gint
ogmrip_message_dialog (GtkWindow *parent, GtkMessageType type, const gchar *format, ...)
{
  GtkWidget *dialog;
  va_list args;

  g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), GTK_RESPONSE_NONE);

  va_start (args, format);
  dialog = ogmrip_message_dialog_newv (parent, type, format, args);
  va_end (args);

  if (dialog)
  {
    gint response;

    response = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    return response;
  }

  return GTK_RESPONSE_NONE;
}

enum
{
  COL_CODEC_DESCRIPTION,
  COL_CODEC_NAME,
  COL_CODEC_TYPE,
  COL_CODEC_VISIBLE,
  N_CODEC_COLUMNS
};

static void
ogmrip_codec_chooser_construct (GtkComboBox *chooser)
{
  GtkListStore *store;
  GtkTreeModel *filter;
  GtkCellRenderer *cell;

  g_return_if_fail (GTK_IS_COMBO_BOX (chooser));

  store = gtk_list_store_new (N_CODEC_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_GTYPE, G_TYPE_BOOLEAN);
  filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (store), NULL);
  g_object_unref (store);

  gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (filter), COL_CODEC_VISIBLE);
  gtk_combo_box_set_model (chooser, filter);
  g_object_unref (filter);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser), cell, "text", COL_CODEC_DESCRIPTION, NULL);
}

static void
ogmrip_combo_box_append_item (GType type, const gchar *name, const gchar *description, GtkTreeModel *model)
{
  GtkTreeIter iter;

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      COL_CODEC_DESCRIPTION, gettext (description), COL_CODEC_NAME, name, COL_CODEC_TYPE, type, COL_CODEC_VISIBLE, TRUE, -1);
}

/**
 * ogmrip_container_chooser_construct:
 * @chooser: A #GtkComboBox
 *
 * Configures @chooser to store containers.
 */
void
ogmrip_container_chooser_construct (GtkComboBox *chooser)
{
  GtkTreeModel *filter, *model;

  g_return_if_fail (GTK_IS_COMBO_BOX (chooser));

  ogmrip_codec_chooser_construct (chooser);

  filter = gtk_combo_box_get_model (chooser);
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));
  gtk_list_store_clear (GTK_LIST_STORE (model));

  ogmrip_plugin_foreach_container ((OGMRipPluginFunc) ogmrip_combo_box_append_item, model);

  gtk_widget_set_sensitive (GTK_WIDGET (chooser), ogmrip_plugin_get_n_containers () > 0);
}

/**
 * ogmrip_video_codec_chooser_construct:
 * @chooser: A #GtkComboBox
 *
 * Configures @chooser to store video codecs.
 */
void
ogmrip_video_codec_chooser_construct (GtkComboBox *chooser)
{
  GtkTreeModel *filter, *model;

  g_return_if_fail (GTK_IS_COMBO_BOX (chooser));

  ogmrip_codec_chooser_construct (chooser);

  filter = gtk_combo_box_get_model (chooser);
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));
  gtk_list_store_clear (GTK_LIST_STORE (model));

  ogmrip_plugin_foreach_video_codec ((OGMRipPluginFunc) ogmrip_combo_box_append_item, model);

  gtk_widget_set_sensitive (GTK_WIDGET (chooser), ogmrip_plugin_get_n_video_codecs () > 0);
}

void
ogmrip_video_codec_chooser_filter (GtkComboBox *chooser, GType container)
{
  GtkTreeModel *filter, *model;
  GtkTreeIter iter;
  GType type;

  filter = gtk_combo_box_get_model (chooser);
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));

  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    do
    {
      gtk_tree_model_get (model, &iter, COL_CODEC_TYPE, &type, -1);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          COL_CODEC_VISIBLE, ogmrip_plugin_can_contain_video (container, type), -1);
    }
    while (gtk_tree_model_iter_next (model, &iter));
  }
}

/**
 * ogmrip_audio_codec_chooser_construct:
 * @chooser: A #GtkComboBox
 *
 * Configures @chooser to store audio codecs.
 */
void
ogmrip_audio_codec_chooser_construct (GtkComboBox *chooser)
{
  GtkTreeModel *filter, *model;

  g_return_if_fail (GTK_IS_COMBO_BOX (chooser));

  ogmrip_codec_chooser_construct (chooser);

  filter = gtk_combo_box_get_model (chooser);
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));
  gtk_list_store_clear (GTK_LIST_STORE (model));

  ogmrip_plugin_foreach_audio_codec ((OGMRipPluginFunc) ogmrip_combo_box_append_item, model);

  gtk_widget_set_sensitive (GTK_WIDGET (chooser), ogmrip_plugin_get_n_audio_codecs () > 0);
}

void
ogmrip_audio_codec_chooser_filter (GtkComboBox *chooser, GType container)
{
  GtkTreeModel *filter, *model;
  GtkTreeIter iter;
  GType type;

  filter = gtk_combo_box_get_model (chooser);
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));

  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    do
    {
      gtk_tree_model_get (model, &iter, COL_CODEC_TYPE, &type, -1);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          COL_CODEC_VISIBLE, ogmrip_plugin_can_contain_audio (container, type), -1);
    }
    while (gtk_tree_model_iter_next (model, &iter));
  }
}

/**
 * ogmrip_subp_codec_chooser_construct
 * @chooser: A #GtkComboBox
 *
 * Configures @chooser to store subp codecs.
 */
void
ogmrip_subp_codec_chooser_construct (GtkComboBox *chooser)
{
  GtkTreeModel *filter, *model;

  g_return_if_fail (GTK_IS_COMBO_BOX (chooser));

  ogmrip_codec_chooser_construct (chooser);

  filter = gtk_combo_box_get_model (chooser);
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));
  gtk_list_store_clear (GTK_LIST_STORE (model));

  ogmrip_plugin_foreach_subp_codec ((OGMRipPluginFunc) ogmrip_combo_box_append_item, model);

  gtk_widget_set_sensitive (GTK_WIDGET (chooser), ogmrip_plugin_get_n_subp_codecs () > 0);
}

void
ogmrip_subp_codec_chooser_filter (GtkComboBox *chooser, GType container)
{
  GtkTreeModel *filter, *model;
  GtkTreeIter iter;
  GType type;

  filter = gtk_combo_box_get_model (chooser);
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));

  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    do
    {
      gtk_tree_model_get (model, &iter, COL_CODEC_TYPE, &type, -1);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          COL_CODEC_VISIBLE, ogmrip_plugin_can_contain_subp (container, type), -1);
    }
    while (gtk_tree_model_iter_next (model, &iter));
  }
}

/**
 * ogmrip_codec_chooser_set_active:
 * @chooser: A #GtkComboBox
 * @name: The name of the codec
 *
 * Selects the codec with the given @name.
 */
void
ogmrip_codec_chooser_set_active (GtkComboBox *chooser, const char *name)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  model = gtk_combo_box_get_model (chooser);
  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    if (name)
    {
      gchar *str;

      do
      {
        gtk_tree_model_get (model, &iter, COL_CODEC_NAME, &str, -1);
        if (g_str_equal (str, name))
        {
          gtk_combo_box_set_active_iter (chooser, &iter);
          g_free (str);
          break;
        }
        g_free (str);
      }
      while (gtk_tree_model_iter_next (model, &iter));
    }

    if (gtk_combo_box_get_active (chooser) < 0)
      gtk_combo_box_set_active (chooser, 0);
  }
}

/**
 * ogmrip_codec_chooser_get_active:
 * @chooser: A #GtkComboBox
 *
 * Returns the selected codec.
 *
 * Returns: the name of the codec, or NULL
 */
gchar *
ogmrip_codec_chooser_get_active (GtkComboBox *chooser)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *name;

  g_return_val_if_fail (GTK_IS_COMBO_BOX (chooser), NULL);

  if (!gtk_combo_box_get_active_iter (chooser, &iter))
    return NULL;

  model = gtk_combo_box_get_model (chooser);
  gtk_tree_model_get (model, &iter, COL_CODEC_NAME, &name, -1);

  return name;
}

GType
ogmrip_codec_chooser_get_active_type (GtkComboBox *chooser)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GType type;

  g_return_val_if_fail (GTK_IS_COMBO_BOX (chooser), G_TYPE_NONE);

  if (!gtk_combo_box_get_active_iter (chooser, &iter))
    return G_TYPE_NONE;

  model = gtk_combo_box_get_model (chooser);
  gtk_tree_model_get (model, &iter, COL_CODEC_TYPE, &type, -1);

  return type;
}

static gboolean
ogmrip_drive_eject_idle (OGMDvdDrive *drive)
{
  ogmdvd_drive_eject (drive);

  return FALSE;
}

/**
 * ogmrip_load_dvd_dialog_new:
 * @parent: Transient parent of the dialog, or NULL
 * @disc: An #OGMDvdDisc
 * @name: The name of the DVD
 * @cancellable: Whether the dialog is cancellable
 *
 * Creates a dialog waiting for the given DVD to be inserted.
 *
 * Returns: a newly created dialog
 */
GtkWidget *
ogmrip_load_dvd_dialog_new (GtkWindow *parent, OGMDvdDisc *disc, const gchar *name, gboolean cancellable)
{
  GtkWidget *dialog;
  OGMDvdMonitor *monitor;
  OGMDvdDrive *drive;

  g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), NULL);
  g_return_val_if_fail (disc != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  monitor = ogmdvd_monitor_get_default ();
  drive = ogmdvd_monitor_get_drive (monitor, ogmdvd_disc_get_device (disc));
  g_object_unref (monitor);

  if (!drive)
    return NULL;

  dialog = gtk_message_dialog_new_with_markup (parent,
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_INFO, cancellable ? GTK_BUTTONS_CANCEL : GTK_BUTTONS_NONE,
      "<b>%s</b>\n\n%s", name, _("Please insert the DVD required to encode this title."));
  // gtk_label_set_selectable (GTK_LABEL (GTK_MESSAGE_DIALOG (dialog)->label), FALSE);
  gtk_window_set_icon_from_stock (GTK_WINDOW (dialog), GTK_STOCK_DIALOG_INFO);

  g_signal_connect_swapped_while_alive (drive, "medium-added", G_CALLBACK (gtk_dialog_response_accept), dialog);

  g_signal_connect_swapped (dialog, "destroy", G_CALLBACK (g_object_unref), drive);

  g_idle_add ((GSourceFunc) ogmrip_drive_eject_idle, drive);

  return dialog;
}

/**
 * ogmrip_get_system_profiles_dir:
 *
 * Return the system directory containing profiles.
 *
 * Returns: a directory, or NULL
 */
const gchar *
ogmrip_get_system_profiles_dir (void)
{
  static gchar *dir = NULL;

  if (!dir)
    dir = g_build_filename (OGMRIP_DATA_DIR, "ogmrip", "profiles", NULL);

  return dir;
}

/**
 * ogmrip_get_user_profiles_dir:
 *
 * Return the user directory containing profiles.
 *
 * Returns: a directory, or NULL
 */
const gchar *
ogmrip_get_user_profiles_dir (void)
{
  static gchar *dir = NULL;

  if (!dir)
    dir = g_build_filename (g_get_home_dir (), ".ogmrip", "profiles", NULL);

  return dir;
}

/**
 * ogmrip_language_chooser_construct:
 * @chooser: a #GtkComboBox
 *
 * Configures a @chooser to store languages.
 */
void
ogmrip_language_chooser_construct (GtkComboBox *chooser)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  const gchar *lang;
  guint i;

  model = gtk_combo_box_get_model (chooser);

  for (i = 2; i < ogmdvd_nlanguages; i ++)
  {
    lang = ogmdvd_languages[i][OGMDVD_LANGUAGE_ISO639_1];

    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
        0, ogmdvd_languages[i][OGMDVD_LANGUAGE_NAME],
        1, (lang[0] << 8) | lang[1], -1);
  }
}

guint
ogmrip_language_chooser_get_active (GtkComboBox *chooser)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  guint lang;

  model = gtk_combo_box_get_model (chooser);
  if (!model)
    return 0;

  if (!gtk_combo_box_get_active_iter (chooser, &iter))
    return 0;

  gtk_tree_model_get (model, &iter, 1, &lang, -1);

  return lang;
}

void
ogmrip_language_chooser_set_active (GtkComboBox *chooser, guint language)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  model = gtk_combo_box_get_model (chooser);
  if (model && gtk_tree_model_get_iter_first (model, &iter))
  {
    guint lang;

    do
    {
      gtk_tree_model_get (model, &iter, 1, &lang, -1);
      if (lang == language)
      {
        gtk_combo_box_set_active_iter (chooser, &iter);
        break;
      }
    }
    while (gtk_tree_model_iter_next (model, &iter));
  }
}

enum
{
  COL_PROFILE_NAME,
  COL_PROFILE_OBJECT,
  N_PROFILE_COLUMNS
};

static gint
ogmrip_profile_chooser_compare (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data)
{
  gchar *aname, *bname;
  gint ret;

  gtk_tree_model_get (model, a, COL_PROFILE_NAME, &aname, -1);
  gtk_tree_model_get (model, b, COL_PROFILE_NAME, &bname, -1);

  ret = g_utf8_collate (aname, bname);

  g_free (aname);
  g_free (bname);

  return ret;
}

void
ogmrip_profile_viewer_construct (GtkTreeView *viewer)
{
  GtkListStore *store;
  GtkCellRenderer *cell;
  GtkTreeViewColumn *column;
  GtkTreeIter iter;

  OGMRipProfileEngine *engine;
  GSList *list, *link;

  store = gtk_list_store_new (N_PROFILE_COLUMNS, G_TYPE_STRING, OGMRIP_TYPE_PROFILE);
  gtk_tree_view_set_model (viewer, GTK_TREE_MODEL (store));
  g_object_unref (store);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (viewer, column);

  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell, "markup", COL_PROFILE_NAME, NULL);

  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store), COL_PROFILE_NAME,
      ogmrip_profile_chooser_compare, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
      COL_PROFILE_NAME, GTK_SORT_ASCENDING);

  engine = ogmrip_profile_engine_get_default ();

  list = ogmrip_profile_engine_get_list (engine);
  for (link = list; link; link = link->next)
    ogmrip_profile_store_add (store, &iter, link->data);
  g_slist_free (list);
}

void
ogmrip_profile_chooser_construct (GtkComboBox *chooser)
{
  GtkListStore *store;
  GtkCellRenderer *cell;
  GtkTreeIter iter;

  OGMRipProfileEngine *engine;
  GSList *list, *link;

  g_return_if_fail (GTK_IS_COMBO_BOX (chooser));

  store = gtk_list_store_new (N_PROFILE_COLUMNS, G_TYPE_STRING, OGMRIP_TYPE_PROFILE);
  gtk_combo_box_set_model (chooser, GTK_TREE_MODEL (store));
  g_object_unref (store);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser), cell, "markup", COL_PROFILE_NAME, NULL);

  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store), COL_PROFILE_NAME,
      ogmrip_profile_chooser_compare, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
      COL_PROFILE_NAME, GTK_SORT_ASCENDING);

  engine = ogmrip_profile_engine_get_default ();

  list = ogmrip_profile_engine_get_list (engine);
  for (link = list; link; link = link->next)
    ogmrip_profile_store_add (store, &iter, link->data);
  g_slist_free (list);
}

static void
ogmrip_profile_store_name_setting_changed (GtkTreeModel *model, gchar *key, OGMRipProfile *profile)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    OGMRipProfile *current;
    gchar *name;

    do
    {
      current = ogmrip_profile_store_get_profile (GTK_LIST_STORE (model), &iter);
      if (current == profile)
      {
        name = g_settings_get_string (G_SETTINGS (profile), key);
        ogmrip_profile_store_set_name (GTK_LIST_STORE (model), &iter, name);
        g_free (name);
      }
    }
    while (gtk_tree_model_iter_next (model, &iter));
  }
}

void
ogmrip_profile_store_add (GtkListStore *store, GtkTreeIter *iter, OGMRipProfile *profile)
{
  gchar *name;

  gtk_list_store_append (store, iter);

  name = g_settings_get_string (G_SETTINGS (profile), "name");
  gtk_list_store_set (store, iter, COL_PROFILE_NAME, name, COL_PROFILE_OBJECT, profile, -1);
  g_free (name);

  g_signal_connect_swapped (profile, "changed::" OGMRIP_PROFILE_NAME,
      G_CALLBACK (ogmrip_profile_store_name_setting_changed), store);
}

gchar *
ogmrip_profile_store_get_name (GtkListStore *store, GtkTreeIter *iter)
{
  gchar *name;

  gtk_tree_model_get (GTK_TREE_MODEL (store), iter, COL_PROFILE_NAME, &name, -1);

  return name;
}

void
ogmrip_profile_store_set_name (GtkListStore *store, GtkTreeIter *iter, const gchar *name)
{
  gtk_list_store_set (store, iter, COL_PROFILE_NAME, name, -1);
}

OGMRipProfile *
ogmrip_profile_store_get_profile (GtkListStore *store, GtkTreeIter *iter)
{
  OGMRipProfile *profile;

  gtk_tree_model_get (GTK_TREE_MODEL (store), iter, COL_PROFILE_OBJECT, &profile, -1);
  if (profile)
    g_object_unref (profile);

  return profile;
}

OGMRipProfile *
ogmrip_profile_chooser_get_active (GtkComboBox *chooser)
{
  OGMRipProfile *profile;
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (!gtk_combo_box_get_active_iter (chooser, &iter))
    return NULL;

  model = gtk_combo_box_get_model (chooser);
  gtk_tree_model_get (model, &iter, COL_PROFILE_OBJECT, &profile, -1);

  return profile;
}

