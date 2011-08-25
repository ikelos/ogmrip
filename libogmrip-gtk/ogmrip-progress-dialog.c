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

#include "ogmrip-progress-dialog.h"
#include "ogmrip-helper.h"

#include <glib/gi18n.h>

#ifdef HAVE_LIBNOTIFY_SUPPORT
#include <libnotify/notify.h>
#endif /* HAVE_LIBNOTIFY_SUPPORT */

#define OGMRIP_PROGRESS_DIALOG_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_PROGRESS_DIALOG, OGMRipProgressDialogPriv))

#define OGMRIP_ICON_FILE  "pixmaps" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip.png"
#define OGMRIP_GLADE_FILE "ogmrip"  G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-progress.glade"
#define OGMRIP_GLADE_ROOT "root"

struct _OGMRipProgressDialogPriv
{
  GtkWidget *title_label;
  GtkWidget *message_label;
  GtkWidget *eta_label;
  GtkWidget *progress;
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

static const GtkActionEntry action_entries[] =
{
  { "Suspend", GTK_STOCK_MEDIA_PAUSE, N_("Suspend"), NULL, NULL, NULL },
  { "Resume",  GTK_STOCK_MEDIA_PLAY,  N_("Resume"),  NULL, NULL, NULL },
  { "Cancel",  GTK_STOCK_CANCEL,      NULL,          NULL, NULL, NULL },
};

static const char *ui_description =
"<ui>"
"  <popup name='Popup'>"
"    <menuitem action='Suspend'/>"
"    <menuitem action='Resume'/>"
"    <menuitem action='Cancel'/>"
"  </popup>"
"</ui>";

#ifdef HAVE_LIBNOTIFY_SUPPORT
static gboolean
ogmrip_progress_dialog_get_visibility (OGMRipProgressDialog *dialog)
{
  GdkWindowState state;

#if GTK_CHECK_VERSION(2,19,0)
  if (!gtk_widget_get_realized (GTK_WIDGET (dialog)))
#else
  if (!GTK_WIDGET_REALIZED (dialog))
#endif
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

static void
ogmrip_progress_dialog_iconify (OGMRipProgressDialog *dialog)
{
  gtk_window_iconify (gtk_window_get_transient_for (GTK_WINDOW (dialog)));
}

static void
ogmrip_progress_dialog_suspend (OGMRipProgressDialog *dialog)
{
  GTimeVal tv;

  g_get_current_time (&tv);
  dialog->priv->suspend_time = tv.tv_sec;

  gtk_dialog_response (GTK_DIALOG (dialog), OGMRIP_RESPONSE_SUSPEND);
}

static void
ogmrip_progress_dialog_resume (OGMRipProgressDialog *dialog)
{
  GTimeVal tv;

  g_get_current_time (&tv);
  dialog->priv->start_time += tv.tv_sec - dialog->priv->suspend_time;

  gtk_dialog_response (GTK_DIALOG (dialog), OGMRIP_RESPONSE_RESUME);
}

static void
ogmrip_progress_dialog_cancel (OGMRipProgressDialog *dialog)
{
  gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
}

static void
ogmrip_progress_dialog_status_icon_popup_menu (OGMRipProgressDialog *dialog, guint button, guint activate_time)
{
  gtk_menu_popup (GTK_MENU (dialog->priv->popup_menu), NULL, NULL, gtk_status_icon_position_menu,
      dialog->priv->status_icon, button, activate_time);
}

G_DEFINE_TYPE (OGMRipProgressDialog, ogmrip_progress_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_progress_dialog_class_init (OGMRipProgressDialogClass *klass)
{
  GObjectClass *gobject_class;
  
  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_progress_dialog_dispose;

  g_type_class_add_private (klass, sizeof (OGMRipProgressDialogPriv));
}

static void
ogmrip_progress_dialog_init (OGMRipProgressDialog *dialog)
{
  GError *error = NULL;

  GtkWidget *area, *root, *button, *image;
  GtkBuilder *builder;

  GtkAction *action;
  GtkActionGroup *action_group;
  GtkAccelGroup *accel_group;
  GtkUIManager *ui_manager;
  GClosure *closure;

  dialog->priv = OGMRIP_PROGRESS_DIALOG_GET_PRIVATE (dialog);

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_GLADE_FILE, &error))
  {
    g_warning ("Couldn't load builder file: %s", error->message);
    g_object_unref (builder);
    g_error_free (error);
    return;
  }

  g_signal_connect (dialog, "window-state-event",
      G_CALLBACK (ogmrip_progress_dialog_state_changed), NULL);

  area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));
  gtk_button_box_set_layout (GTK_BUTTON_BOX (area), GTK_BUTTONBOX_EDGE);

  gtk_window_set_title (GTK_WINDOW (dialog), _("Progress"));
  gtk_window_set_default_size (GTK_WINDOW (dialog), 450, -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  root = gtk_builder_get_widget (builder, OGMRIP_GLADE_ROOT);
  gtk_container_add (GTK_CONTAINER (area), root);
  gtk_widget_show (root);

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group,
      action_entries, G_N_ELEMENTS (action_entries), NULL);

  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL);

  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (dialog), accel_group);

  closure = g_cclosure_new_swap (G_CALLBACK (ogmrip_progress_dialog_iconify), dialog, NULL);
  gtk_accel_group_connect (accel_group, 'w', GDK_CONTROL_MASK, 0, closure);
  g_closure_unref (closure);

  dialog->priv->popup_menu = gtk_ui_manager_get_widget (ui_manager, "/Popup");

  button = gtk_button_new_with_mnemonic (_("Resume"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, OGMRIP_RESPONSE_RESUME);

  image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);

  action = gtk_action_group_get_action (action_group, "Resume");
  g_object_bind_property (button, "visible", action, "visible", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (action, "activate", G_CALLBACK (ogmrip_progress_dialog_resume), dialog);

  button = gtk_button_new_with_mnemonic (_("Suspend"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, OGMRIP_RESPONSE_SUSPEND);

  g_signal_connect_swapped (action, "activate", G_CALLBACK (ogmrip_progress_dialog_suspend), dialog);

  image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);

  action = gtk_action_group_get_action (action_group, "Suspend");
  g_object_bind_property (button, "visible", action, "visible", G_BINDING_SYNC_CREATE);

  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

  action = gtk_action_group_get_action (action_group, "Cancel");

  g_signal_connect_swapped (action, "activate", G_CALLBACK (ogmrip_progress_dialog_cancel), dialog);

  dialog->priv->status_icon = gtk_status_icon_new_from_file (OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_ICON_FILE);
  g_signal_connect_swapped (dialog->priv->status_icon, "popup_menu",
      G_CALLBACK (ogmrip_progress_dialog_status_icon_popup_menu), dialog);

#ifdef HAVE_LIBNOTIFY_SUPPORT
  dialog->priv->notification = notify_notification_new ("Dummy", "Dummy", NULL);
#endif /* HAVE_LIBNOTIFY_SUPPORT */

  dialog->priv->title_label = gtk_builder_get_widget (builder, "title-label");
  dialog->priv->message_label = gtk_builder_get_widget (builder, "message-label");
  dialog->priv->eta_label = gtk_builder_get_widget (builder, "eta-label");

  dialog->priv->progress = gtk_builder_get_widget (builder, "progressbar");

  g_object_unref (builder);
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
  GtkWidget *dialog, *button;

  dialog = g_object_new (OGMRIP_TYPE_PROGRESS_DIALOG, NULL);

  if (parent)
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));

  if (flags & GTK_DIALOG_MODAL)
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  if (flags & GTK_DIALOG_DESTROY_WITH_PARENT)
    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  button = gtk_dialog_get_widget_for_response (GTK_DIALOG (dialog), OGMRIP_RESPONSE_SUSPEND);
  gtk_widget_set_visible (button, can_suspend);

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
  gchar *str;

  g_return_if_fail (OGMRIP_IS_PROGRESS_DIALOG (dialog));

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->progress), fraction);

  if (fraction <= 0.0)
  {
    g_get_current_time (&tv);
    dialog->priv->start_time = tv.tv_sec;
  }

  g_get_current_time (&tv);
  eta = (1.0 - fraction) * (tv.tv_sec - dialog->priv->start_time) / fraction;

  if (eta >= 3600)
    str = g_strdup_printf ("%ld hour(s) %ld minute(s)", eta / 3600, (eta % 3600) / 60);
  else
    str = g_strdup_printf ("%ld minute(s)", eta / 60);

  gtk_label_set_text (GTK_LABEL (dialog->priv->eta_label), str);
  g_free (str);

  if (dialog->priv->status_icon)
  {
/*
    str = g_strdup_printf (_("%s: %02.0lf%% done"), dialog->priv->label, fraction * 100);
    gtk_status_icon_set_tooltip_text (dialog->priv->status_icon, str);
    g_free (str);
*/
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

