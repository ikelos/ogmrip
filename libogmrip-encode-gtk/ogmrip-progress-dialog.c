/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#define OGMRIP_UI_RES "/org/ogmrip/ogmrip-progress-dialog.ui"

struct _OGMRipProgressDialogPriv
{
  GtkWidget *title_label;
  GtkWidget *message_label;
  GtkWidget *eta_label;
  GtkWidget *progress_bar;
  GtkWidget *progress_label;
  GtkWidget *cancel_button;
  GtkWidget *resume_button;
  GtkWidget *suspend_button;

  gulong start_time;
  gulong suspend_time;
};

static void
ogmrip_progress_dialog_cancel_clicked (OGMRipProgressDialog *dialog)
{
  gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
}

static void
ogmrip_progress_dialog_suspend_clicked (OGMRipProgressDialog *dialog)
{
  GTimeVal tv;

  g_get_current_time (&tv);
  dialog->priv->suspend_time = tv.tv_sec;
  gtk_widget_hide (dialog->priv->suspend_button);

  gtk_dialog_response (GTK_DIALOG (dialog), OGMRIP_RESPONSE_SUSPEND);
}

static void
ogmrip_progress_dialog_resume_clicked (OGMRipProgressDialog *dialog)
{
  GTimeVal tv;

  g_get_current_time (&tv);
  dialog->priv->start_time += tv.tv_sec - dialog->priv->suspend_time;
  gtk_widget_show (dialog->priv->suspend_button);

  gtk_dialog_response (GTK_DIALOG (dialog), OGMRIP_RESPONSE_RESUME);
}

G_DEFINE_TYPE_WITH_PRIVATE (OGMRipProgressDialog, ogmrip_progress_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_progress_dialog_class_init (OGMRipProgressDialogClass *klass)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), OGMRIP_UI_RES);

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProgressDialog, title_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProgressDialog, message_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProgressDialog, eta_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProgressDialog, progress_bar);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProgressDialog, progress_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProgressDialog, cancel_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProgressDialog, suspend_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipProgressDialog, resume_button);
}

static void
ogmrip_progress_dialog_init (OGMRipProgressDialog *dialog)
{
  GtkWidget *headerbar;

  gtk_widget_init_template (GTK_WIDGET (dialog));

  dialog->priv = ogmrip_progress_dialog_get_instance_private (dialog);

  headerbar = gtk_dialog_get_header_bar (GTK_DIALOG (dialog));
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (headerbar), FALSE);

  g_object_bind_property (dialog->priv->suspend_button, "visible",
      dialog->priv->resume_button, "visible", G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  g_signal_connect_swapped (dialog->priv->cancel_button, "clicked",
      G_CALLBACK (ogmrip_progress_dialog_cancel_clicked), dialog);

  g_signal_connect_swapped (dialog->priv->suspend_button, "clicked",
      G_CALLBACK (ogmrip_progress_dialog_suspend_clicked), dialog);

  g_signal_connect_swapped (dialog->priv->resume_button, "clicked",
      G_CALLBACK (ogmrip_progress_dialog_resume_clicked), dialog);
}

GtkWidget *
ogmrip_progress_dialog_new (GtkWindow *parent, GtkDialogFlags flags, gboolean can_suspend)
{
  GtkWidget *dialog;

  dialog = g_object_new (OGMRIP_TYPE_PROGRESS_DIALOG, "use-header-bar", TRUE, NULL);

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

    str = g_strdup_printf ("(%.0f%%)", fraction * 100);
    gtk_label_set_text (GTK_LABEL (dialog->priv->progress_label), str);
    g_free (str);

    str = NULL;

    if (fraction > 0.0)
    {
      eta = (1.0 - fraction) * (tv.tv_sec - dialog->priv->start_time) / fraction;

      if (eta >= 3600)
        str = g_strdup_printf ("About %ld hour(s) %ld minute(s)", eta / 3600, (eta % 3600) / 60);
      else if (eta >= 60)
        str = g_strdup_printf ("About %ld minute(s)", eta / 60);
      else
        str = g_strdup_printf ("Less than 1 minute");
    }
  }

  gtk_label_set_text (GTK_LABEL (dialog->priv->eta_label), str ? str : "");
  g_free (str);
}

