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
  OGMRipEncoding *encoding;

  GtkWidget *title_label;
  GtkWidget *stage_label;
  GtkWidget *eta_label;

  GtkWidget *stage_progress;

  GtkWidget *suspend_button;
  GtkWidget *resume_button;
  GtkWidget *cancel_button;
  GtkWidget *quit_check;

  gulong start_time;
  gulong suspend_time;

  gchar *title;
  gchar *label;

  gboolean notify;
  gboolean can_quit;

#ifdef HAVE_LIBNOTIFY_SUPPORT
  NotifyNotification *notification;
  GtkStatusIcon *status_icon;
  gboolean iconified;
#endif /* HAVE_LIBNOTIFY_SUPPORT */

};

static void ogmrip_progress_dialog_dispose  (GObject   *gobject);

#ifdef HAVE_LIBNOTIFY_SUPPORT
/*
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
*/
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
ogmrip_progress_dialog_cancel_menu_activated (OGMRipProgressDialog *dialog)
{
  gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
}

static void
ogmrip_progress_dialog_status_icon_popup_menu (OGMRipProgressDialog *dialog, guint button, guint activate_time)
{
  GtkWidget *menu, *menuitem, *image;

  menu = gtk_menu_new ();

  menuitem = gtk_image_menu_item_new_with_label (_("Suspend"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  if (gtk_widget_get_visible (dialog->priv->suspend_button))
    gtk_widget_show (menuitem);

  g_signal_connect_swapped (menuitem, "activate",
      G_CALLBACK (gtk_button_clicked), dialog->priv->suspend_button);

  image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
  gtk_widget_show (image);

  menuitem = gtk_image_menu_item_new_with_label (_("Resume"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  if (gtk_widget_get_visible (dialog->priv->resume_button))
    gtk_widget_show (menuitem);

  g_signal_connect_swapped (menuitem, "activate",
      G_CALLBACK (gtk_button_clicked), dialog->priv->resume_button);

  image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
  gtk_widget_show (image);

  menuitem = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_CANCEL, NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  g_signal_connect_swapped (menuitem, "activate",
      G_CALLBACK (ogmrip_progress_dialog_cancel_menu_activated), dialog);

  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, gtk_status_icon_position_menu,
      dialog->priv->status_icon, button, activate_time);
}

#endif /* HAVE_LIBNOTIFY_SUPPORT */
/*
static void
ogmrip_progress_dialog_set_label (OGMRipProgressDialog *dialog, const gchar *label)
{
  gchar *str;

  if (dialog->priv->label)
    g_free (dialog->priv->label);
  dialog->priv->label = g_strdup (label);

  str = g_strdup_printf ("<i>%s</i>", label);
  gtk_label_set_markup (GTK_LABEL (dialog->priv->stage_label), str);
  g_free (str);

#ifdef HAVE_LIBNOTIFY_SUPPORT
  if (dialog->priv->notification && !ogmrip_progress_dialog_get_visibility (dialog))
  {
    g_object_set (dialog->priv->notification, "body", label, NULL);
    notify_notification_show (dialog->priv->notification, NULL);
  }
#endif
}
*/
#define SAMPLE_LENGTH  10.0
/*
static void
ogmrip_progress_dialog_run (OGMRipProgressDialog *dialog, OGMJobSpawn *spawn, OGMRipTaskType type)
{
  OGMDvdStream *stream;
  GTimeVal tv;

  gchar *message;

  g_get_current_time (&tv);
  dialog->priv->start_time = tv.tv_sec;

  switch (type)
  {
    case OGMRIP_TASK_ANALYZE:
      ogmrip_progress_dialog_set_label (dialog, _("Analyzing video stream"));
      break;
    case OGMRIP_TASK_CHAPTERS:
      ogmrip_progress_dialog_set_label (dialog, _("Extracting chapters information"));
      break;
    case OGMRIP_TASK_VIDEO:
      ogmrip_progress_dialog_set_label (dialog, _("Encoding video title"));
      break;
    case OGMRIP_TASK_AUDIO:
      stream = ogmrip_codec_get_input (OGMRIP_CODEC (spawn));
      message = g_strdup_printf (_("Extracting audio stream %d"), ogmdvd_stream_get_nr (stream) + 1);
      ogmrip_progress_dialog_set_label (dialog, message);
      g_free (message);
      break;
    case OGMRIP_TASK_SUBP:
      stream = ogmrip_codec_get_input (OGMRIP_CODEC (spawn));
      message = g_strdup_printf (_("Extracting subtitle stream %d"), ogmdvd_stream_get_nr (stream) + 1);
      ogmrip_progress_dialog_set_label (dialog, message);
      g_free (message);
      break;
    case OGMRIP_TASK_MERGE:
      ogmrip_progress_dialog_set_label (dialog, _("Merging audio and video streams"));
      break;
    case OGMRIP_TASK_BACKUP:
      ogmrip_progress_dialog_set_label (dialog, _("DVD backup"));
      break;
    case OGMRIP_TASK_TEST:
      ogmrip_progress_dialog_set_label (dialog, _("Compressibility Test"));
      break;
    case OGMRIP_TASK_CROP:
      ogmrip_progress_dialog_set_label (dialog, _("Cropping"));
      break;
  }

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->stage_progress), 0);
}

static void
ogmrip_progress_dialog_progress (OGMRipProgressDialog *dialog, gdouble fraction)
{
  GtkWindow *parent;
  GTimeVal tv;
  gchar *str;
  gulong eta;

  if (!dialog->priv->start_time)
  {
    g_get_current_time (&tv);
    dialog->priv->start_time = tv.tv_sec;
  }

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->stage_progress), CLAMP (fraction, 0.0, 1.0));

  g_get_current_time (&tv);
  eta = (1.0 - fraction) * (tv.tv_sec - dialog->priv->start_time) / fraction;

  if (eta >= 3600)
    str = g_strdup_printf ("%ld hour(s) %ld minute(s)", eta / 3600, (eta % 3600) / 60);
  else
    str = g_strdup_printf ("%ld minute(s)", eta / 60);

  gtk_label_set_text (GTK_LABEL (dialog->priv->eta_label), str);
  g_free (str);

#ifdef HAVE_LIBNOTIFY_SUPPORT
  if (dialog->priv->status_icon)
  {
    str = g_strdup_printf (_("%s: %02.0lf%% done"), dialog->priv->label, fraction * 100);
    gtk_status_icon_set_tooltip_text (dialog->priv->status_icon, str);
    g_free (str);
  }
#endif

  parent = gtk_window_get_transient_for (GTK_WINDOW (dialog));
  if (parent)
  {
    gchar *title;

    title = g_strdup_printf ("OGMRip: %s: %.0f%%", dialog->priv->label, CLAMP (fraction, 0.0, 1.0) * 100);
    gtk_window_set_title (parent, title);
    g_free (title);
  }
}

static void
ogmrip_progress_dialog_suspend (OGMRipProgressDialog *dialog)
{
  GTimeVal tv;

  g_get_current_time (&tv);
  dialog->priv->suspend_time = tv.tv_sec;
}

static void
ogmrip_progress_dialog_resume (OGMRipProgressDialog *dialog)
{
  GTimeVal tv;

  g_get_current_time (&tv);
  dialog->priv->start_time += tv.tv_sec - dialog->priv->suspend_time;
}

static void
ogmrip_progress_dialog_task_event (OGMRipProgressDialog *dialog, OGMRipEncodingTask *task, OGMRipEncoding *encoding)
{
  switch (task->event)
  {
    case OGMRIP_TASK_RUN:
      ogmrip_progress_dialog_run (dialog, task->spawn, task->type);
      break;
    case OGMRIP_TASK_PROGRESS:
      ogmrip_progress_dialog_progress (dialog, task->detail.fraction);
      break;
    case OGMRIP_TASK_SUSPEND:
      ogmrip_progress_dialog_suspend (dialog);
      break;
    case OGMRIP_TASK_RESUME:
      ogmrip_progress_dialog_resume (dialog);
      break;
    default:
      break;
  }
}
*/
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

  GtkWidget *area, *root, *image;
  GtkSizeGroup *group;
  GtkBuilder *builder;

#ifdef HAVE_LIBNOTIFY_SUPPORT
  GtkAccelGroup *accel_group;
  GClosure *closure;
#endif /* HAVE_LIBNOTIFY_SUPPORT */

  dialog->priv = OGMRIP_PROGRESS_DIALOG_GET_PRIVATE (dialog);

  dialog->priv->can_quit = TRUE;

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_GLADE_FILE, &error))
  {
    g_warning ("Couldn't load builder file: %s", error->message);
    g_object_unref (builder);
    g_error_free (error);
    return;
  }

  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));

  gtk_button_box_set_layout (GTK_BUTTON_BOX (area), GTK_BUTTONBOX_EDGE);

  dialog->priv->resume_button = gtk_button_new_with_mnemonic (_("Resume"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), dialog->priv->resume_button, OGMRIP_RESPONSE_RESUME);
  gtk_size_group_add_widget (group, dialog->priv->resume_button);

  image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (dialog->priv->resume_button), image);

  dialog->priv->suspend_button = gtk_button_new_with_mnemonic (_("Suspend"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), dialog->priv->suspend_button, OGMRIP_RESPONSE_SUSPEND);
  gtk_size_group_add_widget (group, dialog->priv->suspend_button);
  gtk_widget_show (dialog->priv->suspend_button);

  image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (dialog->priv->suspend_button), image);

  dialog->priv->cancel_button = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

  gtk_window_set_title (GTK_WINDOW (dialog), _("Progress"));
  gtk_window_set_default_size (GTK_WINDOW (dialog), 450, -1);
  gtk_window_set_icon_from_stock (GTK_WINDOW (dialog), GTK_STOCK_EXECUTE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  root = gtk_builder_get_widget (builder, OGMRIP_GLADE_ROOT);
  gtk_container_add (GTK_CONTAINER (area), root);
  gtk_widget_show (root);

#ifdef HAVE_LIBNOTIFY_SUPPORT
  dialog->priv->status_icon = gtk_status_icon_new_from_file (OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_ICON_FILE);

  g_signal_connect_swapped (dialog->priv->status_icon, "popup_menu",
      G_CALLBACK (ogmrip_progress_dialog_status_icon_popup_menu), dialog);

  dialog->priv->notification = notify_notification_new ("Dummy", "Dummy", NULL);
  g_signal_connect (dialog, "window-state-event",
      G_CALLBACK (ogmrip_progress_dialog_state_changed), NULL);

  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (dialog), accel_group);

  closure = g_cclosure_new_swap (G_CALLBACK (ogmrip_progress_dialog_iconify), dialog, NULL);
  gtk_accel_group_connect (accel_group, 'w', GDK_CONTROL_MASK, 0, closure);
  g_closure_unref (closure);
#endif /* HAVE_LIBNOTIFY_SUPPORT */

  dialog->priv->title_label = gtk_builder_get_widget (builder, "title-label");
  dialog->priv->stage_label = gtk_builder_get_widget (builder, "stage-label");
  dialog->priv->stage_progress = gtk_builder_get_widget (builder, "stage-progress");
  dialog->priv->eta_label = gtk_builder_get_widget (builder, "eta-label");
  dialog->priv->quit_check = gtk_builder_get_widget (builder, "quit-check");

  g_object_unref (builder);
}

static void
ogmrip_progress_dialog_dispose (GObject *gobject)
{
  OGMRipProgressDialog *dialog = OGMRIP_PROGRESS_DIALOG (gobject);

  if (dialog->priv->encoding)
  {
/*
    g_signal_handlers_disconnect_by_func (dialog->priv->encoding,
        ogmrip_progress_dialog_task_event, dialog);
*/
    g_object_unref (dialog->priv->encoding);
    dialog->priv->encoding = NULL;
  }

  if (dialog->priv->title)
    g_free (dialog->priv->title);
  dialog->priv->title = NULL;

  if (dialog->priv->label)
    g_free (dialog->priv->label);
  dialog->priv->label = NULL;

#ifdef HAVE_LIBNOTIFY_SUPPORT
  if (dialog->priv->notification)
    g_object_unref (dialog->priv->notification);
  dialog->priv->notification = NULL;

  if (dialog->priv->status_icon)
    g_object_unref (dialog->priv->status_icon);
  dialog->priv->status_icon = NULL;
#endif /* HAVE_LIBNOTIFY_SUPPORT */

  G_OBJECT_CLASS (ogmrip_progress_dialog_parent_class)->dispose (gobject);
}

GtkWidget *
ogmrip_progress_dialog_new (void)
{
  return g_object_new (OGMRIP_TYPE_PROGRESS_DIALOG, NULL);
}
/*
static void
ogmrip_progress_dialog_set_title (OGMRipProgressDialog *dialog, const gchar *title)
{
  gchar *str;

  g_return_if_fail (OGMRIP_IS_PROGRESS_DIALOG (dialog));
  g_return_if_fail (title != NULL);

  if (dialog->priv->title)
    g_free (dialog->priv->title);
  dialog->priv->title = g_markup_escape_text (title, -1);

  str = g_strdup_printf ("<big><b>%s</b></big>", title);
  gtk_label_set_markup (GTK_LABEL (dialog->priv->title_label), str);
  g_free (str);

#ifdef HAVE_LIBNOTIFY_SUPPORT
  notify_notification_update (dialog->priv->notification, title, "Dummy",
      OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_ICON_FILE);
#endif
}
*/
void
ogmrip_progress_dialog_set_encoding (OGMRipProgressDialog *dialog, OGMRipEncoding *encoding)
{
  g_return_if_fail (OGMRIP_IS_PROGRESS_DIALOG (dialog));
  g_return_if_fail (encoding == NULL || OGMRIP_IS_ENCODING (encoding));

  if (encoding != dialog->priv->encoding)
  {
    g_object_ref (encoding);

    if (dialog->priv->encoding)
    {
/*
      g_signal_handlers_disconnect_by_func (dialog->priv->encoding,
          ogmrip_progress_dialog_task_event, dialog);
*/
      g_object_unref (dialog->priv->encoding);
    }

    dialog->priv->encoding = encoding;
/*
    g_signal_connect_swapped (encoding, "task",
        G_CALLBACK (ogmrip_progress_dialog_task_event), dialog);
*/
    // ogmrip_progress_dialog_set_title (dialog, ogmrip_encoding_get_label (encoding));
  }
}

OGMRipEncoding *
ogmrip_progress_dialog_get_encoding (OGMRipProgressDialog *dialog)
{
  return dialog->priv->encoding;
}

void
ogmrip_progress_dialog_can_quit (OGMRipProgressDialog *dialog, gboolean can_quit)
{
  if (can_quit != dialog->priv->can_quit)
  {
    dialog->priv->can_quit = can_quit;
    g_object_set (G_OBJECT (dialog->priv->quit_check),
        "visible", can_quit, "sensitive", can_quit, NULL);
  }
}

gboolean
ogmrip_progress_dialog_get_quit (OGMRipProgressDialog *dialog)
{
  return dialog->priv->can_quit &&
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->quit_check));
}

