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
  GtkWidget *popup_menu;

  GtkStatusIcon *status_icon;

  gchar *title;
  gchar *label;

  gboolean iconified;

#ifdef HAVE_LIBNOTIFY_SUPPORT
  NotifyNotification *notification;
#endif /* HAVE_LIBNOTIFY_SUPPORT */

};

enum
{
  PROP_0,
  PROP_ENCODING
};

enum
{
  OGMRIP_RESPONSE_CANCEL_ALL,
  OGMRIP_RESPONSE_SUSPEND,
  OGMRIP_RESPONSE_RESUME,
  OGMRIP_RESPONSE_QUIT
};

static void ogmrip_progress_dialog_dispose      (GObject      *gobject);
static void ogmrip_progress_dialog_get_property (GObject      *gobject,
                                                 guint        property_id,
                                                 GValue       *value,
                                                 GParamSpec   *pspec);
static void ogmrip_progress_dialog_set_property (GObject      *gobject,
                                                 guint        property_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec);

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

static void
ogmrip_progress_dialog_encoding_run (OGMRipProgressDialog *dialog, OGMJobSpawn *spawn, OGMRipEncoding *encoding)
{
  OGMDvdStream *stream;
  gchar *message;

  if (spawn)
  {
    if (OGMRIP_IS_VIDEO_CODEC (spawn))
      ogmrip_progress_dialog_set_label (dialog, _("Encoding video title"));
    else if (OGMRIP_IS_CHAPTERS (spawn))
      ogmrip_progress_dialog_set_label (dialog, _("Extracting chapters information"));
    else if (OGMRIP_IS_CONTAINER (spawn))
      ogmrip_progress_dialog_set_label (dialog, _("Merging audio and video streams"));
    else if (OGMRIP_IS_COPY (spawn))
      ogmrip_progress_dialog_set_label (dialog, _("DVD backup"));
    else if (OGMRIP_IS_ANALYZE (spawn))
      ogmrip_progress_dialog_set_label (dialog, _("Analyzing video stream"));
    else if (OGMRIP_IS_AUDIO_CODEC (spawn))
    {
      stream = ogmrip_codec_get_input (OGMRIP_CODEC (spawn));
      message = g_strdup_printf (_("Extracting audio stream %d"), ogmdvd_stream_get_nr (stream) + 1);
      ogmrip_progress_dialog_set_label (dialog, message);
      g_free (message);
    }
    else if (OGMRIP_IS_SUBP_CODEC (spawn))
    {
      stream = ogmrip_codec_get_input (OGMRIP_CODEC (spawn));
      message = g_strdup_printf (_("Extracting subtitle stream %d"), ogmdvd_stream_get_nr (stream) + 1);
      ogmrip_progress_dialog_set_label (dialog, message);
      g_free (message);
    }
  }

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->stage_progress), 0);
}

static void
ogmrip_progress_dialog_encoding_progress (OGMRipProgressDialog *dialog, OGMJobSpawn *spawn, gdouble fraction, OGMRipEncoding *encoding)
{
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->stage_progress), CLAMP (fraction, 0.0, 1.0));
}

static void
ogmrip_progress_dialog_cancel_menu_activated (OGMRipProgressDialog *dialog)
{
  gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
}

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
ogmrip_progress_dialog_status_icon_popup_menu (OGMRipProgressDialog *dialog, guint button, guint activate_time)
{
  gtk_menu_popup (GTK_MENU (dialog->priv->popup_menu), NULL, NULL, gtk_status_icon_position_menu,
      dialog->priv->status_icon, button, activate_time);
}

static void
ogmrip_progress_dialog_set_encoding (OGMRipProgressDialog *dialog, OGMRipEncoding *encoding)
{
  dialog->priv->encoding = g_object_ref (encoding);

  g_signal_connect_swapped (encoding, "run",
      G_CALLBACK (ogmrip_progress_dialog_encoding_run), dialog);
  g_signal_connect_swapped (encoding, "progress",
      G_CALLBACK (ogmrip_progress_dialog_encoding_progress), dialog);
}

G_DEFINE_TYPE (OGMRipProgressDialog, ogmrip_progress_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_progress_dialog_class_init (OGMRipProgressDialogClass *klass)
{
  GObjectClass *gobject_class;
  
  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_progress_dialog_dispose;
  gobject_class->get_property = ogmrip_progress_dialog_get_property;
  gobject_class->set_property = ogmrip_progress_dialog_set_property;

  g_object_class_install_property (gobject_class, PROP_ENCODING,
      g_param_spec_object ("encoding", "Encoding property", "Set encoding",
        OGMRIP_TYPE_ENCODING, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipProgressDialogPriv));
}

static void
ogmrip_progress_dialog_init (OGMRipProgressDialog *dialog)
{
  GError *error = NULL;

  GtkWidget *area, *root, *button, *image;
  GtkSizeGroup *group;
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

  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));
  gtk_button_box_set_layout (GTK_BUTTON_BOX (area), GTK_BUTTONBOX_EDGE);

  gtk_window_set_title (GTK_WINDOW (dialog), _("Progress"));
  gtk_window_set_default_size (GTK_WINDOW (dialog), 450, -1);
  gtk_window_set_icon_from_stock (GTK_WINDOW (dialog), GTK_STOCK_EXECUTE);
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
  gtk_size_group_add_widget (group, button);

  image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);

  action = gtk_action_group_get_action (action_group, "Resume");
  g_object_bind_property (button, "visible", action, "visible", G_BINDING_SYNC_CREATE);

  button = gtk_button_new_with_mnemonic (_("Suspend"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, OGMRIP_RESPONSE_SUSPEND);
  gtk_size_group_add_widget (group, button);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);

  action = gtk_action_group_get_action (action_group, "Suspend");
  g_object_bind_property (button, "visible", action, "visible", G_BINDING_SYNC_CREATE);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
  g_signal_connect_swapped (button, "clicked", 
      G_CALLBACK (ogmrip_progress_dialog_cancel_menu_activated), dialog);

  dialog->priv->status_icon = gtk_status_icon_new_from_file (OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_ICON_FILE);
  g_signal_connect_swapped (dialog->priv->status_icon, "popup_menu",
      G_CALLBACK (ogmrip_progress_dialog_status_icon_popup_menu), dialog);

#ifdef HAVE_LIBNOTIFY_SUPPORT
  dialog->priv->notification = notify_notification_new ("Dummy", "Dummy", NULL);
#endif /* HAVE_LIBNOTIFY_SUPPORT */

  dialog->priv->title_label = gtk_builder_get_widget (builder, "title-label");
  dialog->priv->stage_label = gtk_builder_get_widget (builder, "stage-label");
  dialog->priv->stage_progress = gtk_builder_get_widget (builder, "stage-progress");
  dialog->priv->eta_label = gtk_builder_get_widget (builder, "eta-label");

  g_object_unref (builder);
}

static void
ogmrip_progress_dialog_dispose (GObject *gobject)
{
  OGMRipProgressDialog *dialog = OGMRIP_PROGRESS_DIALOG (gobject);

  if (dialog->priv->encoding)
  {
    g_signal_handlers_disconnect_by_func (dialog->priv->encoding,
        ogmrip_progress_dialog_encoding_run, dialog);
    g_signal_handlers_disconnect_by_func (dialog->priv->encoding,
        ogmrip_progress_dialog_encoding_progress, dialog);

    g_object_unref (dialog->priv->encoding);
    dialog->priv->encoding = NULL;
  }

  if (dialog->priv->title)
  {
    g_free (dialog->priv->title);
    dialog->priv->title = NULL;
  }

  if (dialog->priv->label)
  {
    g_free (dialog->priv->label);
    dialog->priv->label = NULL;
  }

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

static void
ogmrip_progress_dialog_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipProgressDialog *dialog = OGMRIP_PROGRESS_DIALOG (gobject);

  switch (property_id)
  {
    case PROP_ENCODING:
      g_value_set_object (value, dialog->priv->encoding);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_progress_dialog_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipProgressDialog *dialog = OGMRIP_PROGRESS_DIALOG (gobject);

  switch (property_id)
  {
    case PROP_ENCODING:
      ogmrip_progress_dialog_set_encoding (dialog, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

GtkWidget *
ogmrip_progress_dialog_new (OGMRipEncoding *encoding)
{
  g_return_val_if_fail (OGMRIP_IS_ENCODING (encoding), NULL);

  return g_object_new (OGMRIP_TYPE_PROGRESS_DIALOG, "encoding", encoding, NULL);
}

OGMRipEncoding *
ogmrip_progress_dialog_get_encoding (OGMRipProgressDialog *dialog)
{
  return dialog->priv->encoding;
}

