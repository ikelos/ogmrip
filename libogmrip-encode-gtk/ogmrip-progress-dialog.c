/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-progress-dialog.h"

#include <glib/gi18n.h>

#ifdef HAVE_LIBNOTIFY_SUPPORT
#include <libnotify/notify.h>
#endif /* HAVE_LIBNOTIFY_SUPPORT */

#define OGMRIP_ICON_FILE "pixmaps" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip.png"
#define OGMRIP_UI_RES    "/org/ogmrip/ogmrip-progress-dialog.ui"
#define OGMRIP_MENU_RES  "/org/ogmrip/ogmrip-progress-menu.ui"

struct _OGMRipProgressDialogPriv
{
  GtkWidget *title_label;
  GtkWidget *message_label;
  GtkWidget *eta_label;
  GtkWidget *progress_bar;
  GtkWidget *cancel_button;
  GtkWidget *resume_button;
  GtkWidget *suspend_button;

  GtkWidget *popup_menu;
  GtkStatusIcon *status_icon;

  gboolean iconified;

  gulong start_time;
  gulong suspend_time;

#ifdef HAVE_LIBNOTIFY_SUPPORT
  NotifyNotification *notification;
#endif /* HAVE_LIBNOTIFY_SUPPORT */
};

static void ogmrip_progress_dialog_dispose (GObject *gobject);

#ifdef HAVE_LIBNOTIFY_SUPPORT
static gboolean
ogmrip_progress_dialog_get_visibility (OGMRipProgressDialog *dialog)
{
  GdkWindowState state;

  if (!gtk_widget_get_realized (GTK_WIDGET (dialog)))
    return FALSE;

  if (dialog->priv->iconified)
    return FALSE;

  state = gdk_window_get_state (gtk_widget_get_window (GTK_WIDGET (dialog)));
  if (state & (GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED))
    return FALSE;

  return TRUE;
}
#endif

static gboolean
ogmrip_progress_dialog_state_changed (OGMRipProgressDialog *dialog, GdkEventWindowState *event)
{
  dialog->priv->iconified = ((event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) != 0);

  return FALSE;
}
/*
static void
ogmrip_progress_dialog_iconify (OGMRipProgressDialog *dialog)
{
  gtk_window_iconify (gtk_window_get_transient_for (GTK_WINDOW (dialog)));
}
*/
static void
ogmrip_progress_dialog_suspend_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipProgressDialog *dialog = data;
  GTimeVal tv;

  g_get_current_time (&tv);
  dialog->priv->suspend_time = tv.tv_sec;

  gtk_widget_hide (dialog->priv->suspend_button);

  gtk_dialog_response (GTK_DIALOG (dialog), OGMRIP_RESPONSE_SUSPEND);
}

static void
ogmrip_progress_dialog_resume_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipProgressDialog *dialog = data;
  GTimeVal tv;

  g_get_current_time (&tv);
  dialog->priv->start_time += tv.tv_sec - dialog->priv->suspend_time;

  gtk_widget_show (dialog->priv->suspend_button);

  gtk_dialog_response (GTK_DIALOG (dialog), OGMRIP_RESPONSE_RESUME);
}

static void
ogmrip_progress_dialog_cancel_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  gtk_dialog_response (GTK_DIALOG (data), GTK_RESPONSE_CANCEL);
}

static void
ogmrip_progress_dialog_status_icon_popup_menu (OGMRipProgressDialog *dialog, guint button, guint activate_time)
{
  gtk_menu_popup (GTK_MENU (dialog->priv->popup_menu), NULL, NULL, gtk_status_icon_position_menu,
      dialog->priv->status_icon, button, activate_time);
}

G_DEFINE_TYPE_WITH_PRIVATE (OGMRipProgressDialog, ogmrip_progress_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_progress_dialog_class_init (OGMRipProgressDialogClass *klass)
{
  GObjectClass *gobject_class;
  
  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_progress_dialog_dispose;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), OGMRIP_UI_RES);

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProgressDialog, title_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProgressDialog, message_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProgressDialog, eta_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProgressDialog, progress_bar);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProgressDialog, cancel_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProgressDialog, suspend_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProgressDialog, resume_button);
}

static GActionEntry entries[] =
{
  { "suspend", ogmrip_progress_dialog_suspend_activated, NULL, NULL, NULL },
  { "resume",  ogmrip_progress_dialog_resume_activated,  NULL, NULL, NULL },
  { "cancel",  ogmrip_progress_dialog_cancel_activated,  NULL, NULL, NULL },
};

static void
ogmrip_progress_dialog_init (OGMRipProgressDialog *dialog)
{
  GtkBuilder *builder;
  GSimpleActionGroup *group;
  GAction *action;
  GObject *model;

  gtk_widget_init_template (GTK_WIDGET (dialog));

  dialog->priv = ogmrip_progress_dialog_get_instance_private (dialog);
/*
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
*/
  g_signal_connect (dialog, "window-state-event",
      G_CALLBACK (ogmrip_progress_dialog_state_changed), NULL);

  g_object_bind_property (dialog->priv->suspend_button, "visible",
      dialog->priv->resume_button, "visible", G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  group = g_simple_action_group_new ();
  gtk_widget_insert_action_group (GTK_WIDGET (dialog), "progress", G_ACTION_GROUP (group));
  g_object_unref (group);

  g_action_map_add_action_entries (G_ACTION_MAP (group),
      entries, G_N_ELEMENTS (entries), dialog);

  action = g_action_map_lookup_action (G_ACTION_MAP (group), "resume");
  g_object_bind_property (dialog->priv->resume_button, "visible",
      action, "enabled", G_BINDING_SYNC_CREATE);

  action = g_action_map_lookup_action (G_ACTION_MAP (group), "suspend");
  g_object_bind_property (dialog->priv->suspend_button, "visible",
      action, "enabled", G_BINDING_SYNC_CREATE);

  gtk_actionable_set_action_name (GTK_ACTIONABLE (dialog->priv->cancel_button), "progress.cancel");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (dialog->priv->resume_button), "progress.resume");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (dialog->priv->suspend_button), "progress.suspend");

  builder = gtk_builder_new_from_resource (OGMRIP_MENU_RES);
  model = gtk_builder_get_object (builder, "menu");
  g_object_unref (builder);

  dialog->priv->popup_menu = gtk_menu_new_from_model (G_MENU_MODEL (model));
/*
  closure = g_cclosure_new_swap (G_CALLBACK (ogmrip_progress_dialog_iconify), dialog, NULL);
  gtk_accel_group_connect (accel_group, 'w', GDK_CONTROL_MASK, 0, closure);
  g_closure_unref (closure);
*/
  dialog->priv->status_icon = gtk_status_icon_new_from_file (OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_ICON_FILE);
  gtk_status_icon_set_visible(dialog->priv->status_icon, TRUE);

  g_signal_connect_swapped (dialog->priv->status_icon, "popup_menu",
      G_CALLBACK (ogmrip_progress_dialog_status_icon_popup_menu), dialog);

#ifdef HAVE_LIBNOTIFY_SUPPORT
  dialog->priv->notification = notify_notification_new ("Dummy", "Dummy", NULL);
#endif /* HAVE_LIBNOTIFY_SUPPORT */
}

static void
ogmrip_progress_dialog_dispose (GObject *gobject)
{
  OGMRipProgressDialog *dialog = OGMRIP_PROGRESS_DIALOG (gobject);

  if (dialog->priv->status_icon)
  {
    g_object_unref (dialog->priv->status_icon);
    dialog->priv->status_icon = NULL;
  }

#ifdef HAVE_LIBNOTIFY_SUPPORT
  if (dialog->priv->notification)
  {
    g_object_unref (dialog->priv->notification);
    dialog->priv->notification = NULL;
  }
#endif /* HAVE_LIBNOTIFY_SUPPORT */

  G_OBJECT_CLASS (ogmrip_progress_dialog_parent_class)->dispose (gobject);
}

GtkWidget *
ogmrip_progress_dialog_new (GtkWindow *parent, GtkDialogFlags flags, gboolean can_suspend)
{
  GtkWidget *dialog;

  dialog = g_object_new (OGMRIP_TYPE_PROGRESS_DIALOG, NULL);

  if (parent)
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));

  if (flags & GTK_DIALOG_MODAL)
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  if (flags & GTK_DIALOG_DESTROY_WITH_PARENT)
    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  gtk_widget_set_visible (OGMRIP_PROGRESS_DIALOG (dialog)->priv->suspend_button, can_suspend);

  return dialog;
}

void
ogmrip_progress_dialog_set_title (OGMRipProgressDialog *dialog, const gchar *title)
{
  g_return_if_fail (OGMRIP_IS_PROGRESS_DIALOG (dialog));

  if (!title)
    gtk_label_set_text (GTK_LABEL (dialog->priv->title_label), "");
  else
  {
    gchar *str;

    str = g_strdup_printf ("<big><b>%s</b></big>", title);
    gtk_label_set_markup (GTK_LABEL (dialog->priv->title_label), str);
    g_free (str);
  }

#ifdef HAVE_LIBNOTIFY_SUPPORT
  notify_notification_update (dialog->priv->notification, title, "Dummy",
      OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_ICON_FILE);
#endif /* HAVE_LIBNOTIFY_SUPPORT */
}

void
ogmrip_progress_dialog_set_message (OGMRipProgressDialog *dialog, const gchar *message)
{
  g_return_if_fail (OGMRIP_IS_PROGRESS_DIALOG (dialog));

  if (!message)
    gtk_label_set_text (GTK_LABEL (dialog->priv->message_label), "");
  else
  {
    gchar *str;

    str = g_strdup_printf ("<i>%s</i>", message);
    gtk_label_set_markup (GTK_LABEL (dialog->priv->message_label), str);
    g_free (str);
  }

#ifdef HAVE_LIBNOTIFY_SUPPORT
  if (dialog->priv->notification && !ogmrip_progress_dialog_get_visibility (dialog))
  {
    g_object_set (dialog->priv->notification, "body", message, NULL);
    notify_notification_show (dialog->priv->notification, NULL);
  }
#endif
}

void
ogmrip_progress_dialog_set_fraction (OGMRipProgressDialog *dialog, gdouble fraction)
{
  GTimeVal tv;
  gulong eta;
  gchar *str = NULL;

  g_return_if_fail (OGMRIP_IS_PROGRESS_DIALOG (dialog));

  g_get_current_time (&tv);
  if (fraction < 0.0)
    dialog->priv->start_time = tv.tv_sec;
  else
  {
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->progress_bar), fraction);

    if (fraction > 0.0)
    {
      eta = (1.0 - fraction) * (tv.tv_sec - dialog->priv->start_time) / fraction;

      if (eta >= 3600)
        str = g_strdup_printf ("%ld hour(s) %ld minute(s)", eta / 3600, (eta % 3600) / 60);
      else if (eta >= 60)
        str = g_strdup_printf ("%ld minute(s)", eta / 60);
      else
        str = g_strdup_printf ("%ld second(s)", eta);
    }
  }

  gtk_label_set_text (GTK_LABEL (dialog->priv->eta_label), str ? str : "");
  g_free (str);

  if (dialog->priv->status_icon)
  {
    str = g_strdup_printf (_("%s: %02.0lf%% done"),
        gtk_label_get_text (GTK_LABEL (dialog->priv->title_label)), fraction * 100);
    gtk_status_icon_set_tooltip_text (dialog->priv->status_icon, str);
    g_free (str);
  }
/*
  parent = gtk_window_get_transient_for (GTK_WINDOW (dialog));
  if (parent)
  {
    gchar *title;

    title = g_strdup_printf ("OGMRip: %s: %.0f%%", dialog->priv->label, CLAMP (fraction, 0.0, 1.0) * 100);
    gtk_window_set_title (parent, title);
    g_free (title);
  }
*/
}

