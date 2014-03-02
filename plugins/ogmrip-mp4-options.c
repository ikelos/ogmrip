/* OGMRipMp4Options - An MP4 options plugin for OGMRip
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

#include <ogmrip-encode-gtk.h>
#include <ogmrip-module.h>

#include "ogmrip-mp4.h"

#include <glib/gi18n.h>

#define OGMRIP_UI_RES  "/org/ogmrip/ogmrip-mp4-options-dialog.ui"
#define OGMRIP_UI_ROOT "root"

#define OGMRIP_TYPE_MP4_DIALOG          (ogmrip_mp4_dialog_get_type ())
#define OGMRIP_MP4_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MP4_DIALOG, OGMRipMp4Dialog))
#define OGMRIP_MP4_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_MP4_DIALOG, OGMRipMp4DialogClass))
#define OGMRIP_IS_MP4_DIALOG(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_MP4_DIALOG))
#define OGMRIP_IS_MP4_DIALOG_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_MP4_DIALOG))

typedef struct _OGMRipMp4Dialog      OGMRipMp4Dialog;
typedef struct _OGMRipMp4DialogClass OGMRipMp4DialogClass;

struct _OGMRipMp4Dialog
{
  GtkDialog parent_instance;

  OGMRipProfile *profile;

  GtkWidget *format_combo;
  GtkWidget *hint_check;
};

struct _OGMRipMp4DialogClass
{
  GtkDialogClass parent_class;
};

enum
{
  PROP_0,
  PROP_PROFILE
};

static void ogmrip_options_editable_init (OGMRipOptionsEditableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (OGMRipMp4Dialog, ogmrip_mp4_dialog, GTK_TYPE_DIALOG,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_OPTIONS_EDITABLE, ogmrip_options_editable_init));

static void
ogmrip_mp4_dialog_set_profile (OGMRipMp4Dialog *dialog, OGMRipProfile *profile)
{
  if (dialog->profile)
  {
    g_object_unref (dialog->profile);
    dialog->profile = NULL;
  }

  if (profile)
  {
    GSettings *settings;

    dialog->profile = g_object_ref (profile);

    settings = ogmrip_profile_get_child (profile, "mp4");
    g_assert (settings != NULL);

    g_settings_bind (settings, OGMRIP_MP4_PROP_FORMAT,
        dialog->format_combo, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_MP4_PROP_HINT,
        dialog->hint_check, "active", G_SETTINGS_BIND_DEFAULT);
    g_object_unref (settings);
  }
}

static void
ogmrip_mp4_dialog_dispose (GObject *gobject)
{
  OGMRipMp4Dialog *dialog = OGMRIP_MP4_DIALOG (gobject);

  if (dialog->profile)
  {
    g_object_unref (dialog->profile);
    dialog->profile = NULL;
  }

  G_OBJECT_CLASS (ogmrip_mp4_dialog_parent_class)->dispose (gobject);
}

static void
ogmrip_mp4_dialog_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
  {
    case PROP_PROFILE:
      g_value_set_object (value, OGMRIP_MP4_DIALOG (gobject)->profile);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_mp4_dialog_set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
  {
    case PROP_PROFILE:
      ogmrip_mp4_dialog_set_profile (OGMRIP_MP4_DIALOG (gobject), g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_mp4_dialog_class_init (OGMRipMp4DialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_mp4_dialog_dispose;
  gobject_class->get_property = ogmrip_mp4_dialog_get_property;
  gobject_class->set_property = ogmrip_mp4_dialog_set_property;

  g_object_class_override_property (gobject_class, PROP_PROFILE, "profile");

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), OGMRIP_UI_RES);

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipMp4Dialog, format_combo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipMp4Dialog, hint_check);
}

static void
ogmrip_mp4_dialog_init (OGMRipMp4Dialog *dialog)
{
  GtkWidget *misc, *widget;

  gtk_widget_init_template (GTK_WIDGET (dialog));

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
}

static void
ogmrip_options_editable_init (OGMRipOptionsEditableInterface *iface)
{
}

void
ogmrip_module_load (OGMRipModule *module)
{
  GType gtype;

  gtype = ogmrip_type_from_name ("mp4");
  if (gtype == G_TYPE_NONE)
    return;

  ogmrip_type_add_extension (gtype, OGMRIP_TYPE_MP4_DIALOG);
}

