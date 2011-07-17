/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
 *
 * This library is free software; you can redisectionibute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is disectionibuted in the hope that it will be useful,
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

#include <glib/gi18n.h>

#include "ogmrip-helper.h"
#include "ogmrip-options-plugin.h"
#include "ogmrip-plugin.h"
#include "ogmrip-xvid.h"

#define OGMRIP_GLADE_FILE "ogmrip/ogmrip-xvid.glade"
#define OGMRIP_GLADE_ROOT "root"

#define OGMRIP_TYPE_XVID_DIALOG          (ogmrip_xvid_dialog_get_type ())
#define OGMRIP_XVID_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_XVID_DIALOG, OGMRipXvidDialog))
#define OGMRIP_XVID_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_XVID_DIALOG, OGMRipXvidDialogClass))
#define OGMRIP_IS_XVID_DIALOG(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_XVID_DIALOG))
#define OGMRIP_IS_XVID_DIALOG_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_XVID_DIALOG))

typedef struct _OGMRipXvidDialog      OGMRipXvidDialog;
typedef struct _OGMRipXvidDialogClass OGMRipXvidDialogClass;

struct _OGMRipXvidDialog
{
  OGMRipPluginDialog parent_instance;
};

struct _OGMRipXvidDialogClass
{
  OGMRipPluginDialogClass parent_class;
};

static gboolean
ogmrip_xvid_dialog_set_frame_drop_ratio_sensitivity (GBinding *binding,
    const GValue *source_value, GValue *target_value, gpointer data)
{
  g_value_set_boolean (target_value, g_value_get_int (source_value) == 0);

  return TRUE;
}

static gboolean
ogmrip_xvid_dialog_set_par_sensitivity (GBinding *binding,
    const GValue *source_value, GValue *target_value, gpointer data)
{
  g_value_set_boolean (target_value, g_value_get_int (source_value) == 6);

  return TRUE;
}

G_DEFINE_TYPE (OGMRipXvidDialog, ogmrip_xvid_dialog, OGMRIP_TYPE_PLUGIN_DIALOG)

static void
ogmrip_xvid_dialog_constructed (GObject *gobject)
{
  GError *error = NULL;

  OGMRipProfile *profile;
  GSettings *settings;

  GtkWidget *misc, *widget;
  GtkBuilder *builder;

  G_OBJECT_CLASS (ogmrip_xvid_dialog_parent_class)->constructed (gobject);

  profile = ogmrip_plugin_dialog_get_profile (OGMRIP_PLUGIN_DIALOG (gobject));
  if (!profile)
  {
    g_critical ("No profile has been specified");
    return;
  }

  settings = ogmrip_profile_get_child (profile, "xvid");
  g_assert (settings != NULL);

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_GLADE_FILE, &error))
  {
    g_critical ("Couldn't load builder file: %s", error->message);
    return;
  }

  misc = gtk_dialog_get_content_area (GTK_DIALOG (gobject));

  widget = gtk_builder_get_widget (builder, OGMRIP_GLADE_ROOT);
  gtk_container_add (GTK_CONTAINER (misc), widget);
  gtk_widget_show (widget);

  widget = gtk_builder_get_widget (builder, "bquant_offset-spin");
  g_settings_bind (settings, OGMRIP_XVID_PROP_BQUANT_OFFSET,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "bquant_ratio-spin");
  g_settings_bind (settings, OGMRIP_XVID_PROP_BQUANT_RATIO,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "bvhq-check");
  g_settings_bind (settings, OGMRIP_XVID_PROP_BVHQ,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "chroma_me-check");
  g_settings_bind (settings, OGMRIP_XVID_PROP_CHROMA_ME,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "chroma_opt-check");
  g_settings_bind (settings, OGMRIP_XVID_PROP_CHROMA_OPT,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "closed_gop-check");
  g_settings_bind (settings, OGMRIP_XVID_PROP_CLOSED_GOP,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "max_bframes-spin");
  g_settings_bind (settings, OGMRIP_XVID_PROP_BFRAMES,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  g_object_bind_property (misc, "active", widget, "sensitive",
      G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  misc = gtk_builder_get_widget (builder, "frame_drop_ratio-spin");
  g_settings_bind (settings, OGMRIP_XVID_PROP_FRAME_DROP_RATIO,
      misc, "value", G_SETTINGS_BIND_DEFAULT);

  g_object_bind_property_full (widget, "value", misc, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_xvid_dialog_set_frame_drop_ratio_sensitivity, NULL, NULL, NULL);

  widget = gtk_builder_get_widget (builder, "gmc-check");
  g_settings_bind (settings, OGMRIP_XVID_PROP_GMC,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "interlacing-check");
  g_settings_bind (settings, OGMRIP_XVID_PROP_INTERLACING,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "max_bquant-spin");
  g_settings_bind (settings, OGMRIP_XVID_PROP_MAX_BQUANT,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "max_iquant-spin");
  g_settings_bind (settings, OGMRIP_XVID_PROP_MAX_IQUANT,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "max_pquant-spin");
  g_settings_bind (settings, OGMRIP_XVID_PROP_MAX_PQUANT,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "me_quality-combo");
  g_settings_bind (settings, OGMRIP_XVID_PROP_ME_QUALITY,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "min_bquant-spin");
  g_settings_bind (settings, OGMRIP_XVID_PROP_MIN_BQUANT,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "min_iquant-spin");
  g_settings_bind (settings, OGMRIP_XVID_PROP_MIN_IQUANT,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "min_pquant-spin");
  g_settings_bind (settings, OGMRIP_XVID_PROP_MIN_PQUANT,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "max_keyint-spin");
  g_settings_bind (settings, OGMRIP_XVID_PROP_MAX_KEYINT,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "packed-check");
  g_settings_bind (settings, OGMRIP_XVID_PROP_PACKED,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "profile-combo");
  g_settings_bind (settings, OGMRIP_XVID_PROP_PROFILE,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "quant_type-combo");
  g_settings_bind (settings, OGMRIP_XVID_PROP_QUANT_TYPE,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "vhq-combo");
  g_settings_bind (settings, OGMRIP_XVID_PROP_VHQ,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "par_width-spin");
  g_settings_bind (settings, OGMRIP_XVID_PROP_PAR_WIDTH,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  misc = gtk_builder_get_widget (builder, "par_height-spin");
  g_settings_bind (settings, OGMRIP_XVID_PROP_PAR_HEIGHT,
      misc, "value", G_SETTINGS_BIND_DEFAULT);

  g_object_bind_property (widget, "sensitive", misc, "sensitive", G_BINDING_SYNC_CREATE);

  misc = gtk_builder_get_widget (builder, "par-combo");
  g_settings_bind (settings, OGMRIP_XVID_PROP_PAR,
      misc, "active", G_SETTINGS_BIND_DEFAULT);

  g_object_bind_property_full (misc, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_xvid_dialog_set_par_sensitivity, NULL, NULL, NULL);

  g_object_unref (builder);
}

static void
ogmrip_xvid_dialog_class_init (OGMRipXvidDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_xvid_dialog_constructed;
}

static void
ogmrip_xvid_dialog_init (OGMRipXvidDialog *dialog)
{
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
      NULL);
  gtk_window_set_title (GTK_WINDOW (dialog), _("XviD Options"));
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_window_set_icon_from_stock (GTK_WINDOW (dialog), GTK_STOCK_PREFERENCES);
}

static OGMRipVideoOptionsPlugin xvid_options_plugin =
{
  NULL,
  G_TYPE_NONE,
  G_TYPE_NONE
};

OGMRipVideoOptionsPlugin *
ogmrip_init_options_plugin (void)
{
  xvid_options_plugin.type = ogmrip_plugin_get_video_codec_by_name ("xvid");
  if (xvid_options_plugin.type == G_TYPE_NONE)
    return NULL;

  xvid_options_plugin.dialog = OGMRIP_TYPE_XVID_DIALOG;

  return &xvid_options_plugin;
}

