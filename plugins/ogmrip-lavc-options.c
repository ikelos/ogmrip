/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-lavc-mpeg4.h"

#include "ogmrip-lavc.h"
#include "ogmrip-helper.h"
#include "ogmrip-options-plugin.h"

#include <glib/gi18n.h>

#define OGMRIP_GLADE_FILE "ogmrip/ogmrip-lavc.glade"
#define OGMRIP_GLADE_ROOT "root"

#define OGMRIP_TYPE_LAVC_DIALOG          (ogmrip_lavc_dialog_get_type ())
#define OGMRIP_LAVC_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_LAVC_DIALOG, OGMRipLavcDialog))
#define OGMRIP_LAVC_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_LAVC_DIALOG, OGMRipLavcDialogClass))
#define OGMRIP_IS_LAVC_DIALOG(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_LAVC_DIALOG))
#define OGMRIP_IS_LAVC_DIALOG_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_LAVC_DIALOG))

typedef struct _OGMRipLavcDialog      OGMRipLavcDialog;
typedef struct _OGMRipLavcDialogClass OGMRipLavcDialogClass;

struct _OGMRipLavcDialog
{
  OGMRipPluginDialog parent_instance;
};

struct _OGMRipLavcDialogClass
{
  OGMRipPluginDialogClass parent_class;
};

G_DEFINE_TYPE (OGMRipLavcDialog, ogmrip_lavc_dialog, OGMRIP_TYPE_PLUGIN_DIALOG)

static void
ogmrip_lavc_dialog_constructed (GObject *gobject)
{
  GError *error = NULL;

  OGMRipProfile *profile;
  GSettings *settings;

  GtkWidget *misc, *widget;
  GtkBuilder *builder;

  G_OBJECT_CLASS (ogmrip_lavc_dialog_parent_class)->constructed (gobject);

  profile = ogmrip_plugin_dialog_get_profile (OGMRIP_PLUGIN_DIALOG (gobject));
  if (!profile)
  {
    g_critical ("No profile has been specified");
    return;
  }

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_GLADE_FILE, &error))
  {
    g_critical ("Couldn't load builder file: %s", error->message);
    return;
  }

  settings = ogmrip_profile_get_child (profile, "lavc");
  g_assert (settings != NULL);

  misc = gtk_dialog_get_content_area (GTK_DIALOG (gobject));

  widget = gtk_builder_get_widget (builder, OGMRIP_GLADE_ROOT);
  gtk_container_add (GTK_CONTAINER (misc), widget);
  gtk_widget_show (widget);

  widget = gtk_builder_get_widget (builder, "mv0-check");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_MV0,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "v4mv-check");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_V4MV,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "mbd-combo");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_MBD,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "strict-combo");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_STRICT,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "vb_strategy-combo");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_VB_STRATEGY,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "qns-combo");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_QNS,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "keyint-spin");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_KEYINT,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "last_pred-spin");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_LAST_PRED,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "vqcomp-spin");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_VQCOMP,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "dc-spin");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_DC,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "preme-combo");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_PREME,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "cmp-spin");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_CMP,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "precmp-spin");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_PRECMP,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "subcmp-spin");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_SUBCMP,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "dia-spin");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_DIA,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "predia-spin");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_PREDIA,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "buf_size-spin");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_BUF_SIZE,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "min_rate-spin");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_MIN_RATE,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "max_rate-spin");
  g_settings_bind (settings, OGMRIP_LAVC_PROP_MAX_RATE,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  g_object_unref (settings);
  g_object_unref (builder);
}

static void
ogmrip_lavc_dialog_class_init (OGMRipLavcDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_lavc_dialog_constructed;
}

static void
ogmrip_lavc_dialog_init (OGMRipLavcDialog *dialog)
{
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
      NULL);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Lavc Options"));
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
}

static OGMRipVideoOptionsPlugin lavc_options_plugin =
{
  NULL,
  G_TYPE_NONE,
  G_TYPE_NONE
};

OGMRipVideoOptionsPlugin *
ogmrip_init_options_plugin (void)
{
  lavc_options_plugin.type = OGMRIP_TYPE_LAVC;
  lavc_options_plugin.dialog = OGMRIP_TYPE_LAVC_DIALOG;

  return &lavc_options_plugin;
}

