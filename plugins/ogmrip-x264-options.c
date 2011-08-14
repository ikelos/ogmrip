/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
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

#include <glib/gi18n.h>

#include "ogmrip-helper.h"
#include "ogmrip-options-plugin.h"
#include "ogmrip-plugin.h"
#include "ogmrip-x264.h"

#define OGMRIP_GLADE_FILE "ogmrip/ogmrip-x264.glade"
#define OGMRIP_GLADE_ROOT "root"

#define OGMRIP_TYPE_X264_DIALOG          (ogmrip_x264_dialog_get_type ())
#define OGMRIP_X264_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_X264_DIALOG, OGMRipX264Dialog))
#define OGMRIP_X264_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_X264_DIALOG, OGMRipX264DialogClass))
#define OGMRIP_IS_X264_DIALOG(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_X264_DIALOG))
#define OGMRIP_IS_X264_DIALOG_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_X264_DIALOG))

#define OGMRIP_X264_PROP_PROFILE      "profile"
#define OGMRIP_X264_DEFAULT_PROFILE   OGMRIP_X264_PROFILE_HIGH

typedef struct _OGMRipX264Dialog      OGMRipX264Dialog;
typedef struct _OGMRipX264DialogClass OGMRipX264DialogClass;

struct _OGMRipX264Dialog
{
  OGMRipPluginDialog parent_instance;
};

struct _OGMRipX264DialogClass
{
  OGMRipPluginDialogClass parent_class;
};

enum
{
  OGMRIP_X264_PROFILE_BASELINE,
  OGMRIP_X264_PROFILE_MAIN,
  OGMRIP_X264_PROFILE_HIGH
};

GType ogmrip_x264_get_type ();

static gboolean x264_have_8x8dct     = FALSE;
static gboolean x264_have_aud        = FALSE;
static gboolean x264_have_b_pyramid  = FALSE;
static gboolean x264_have_brdo       = FALSE;
static gboolean x264_have_lookahead  = FALSE;
static gboolean x264_have_me         = FALSE;
static gboolean x264_have_me_tesa    = FALSE;
static gboolean x264_have_mixed_refs = FALSE;
static gboolean x264_have_psy        = FALSE;
static gboolean x264_have_weight_p   = FALSE;

static gboolean
ogmrip_x264_get_me (GValue *value, GVariant *variant, gpointer user_data)
{
  g_value_set_uint (value, g_variant_get_uint32 (variant) - 1);

  return TRUE;
}

static GVariant *
ogmrip_x264_set_me (const GValue *value, const GVariantType *type, gpointer user_data)
{
  return g_variant_new_uint32 (g_value_get_uint (value) + 1);
}
/*
static void
ogmrip_x264_dialog_profile_changed (OGMRipX264Dialog *dialog)
{
  gint profile;

  profile = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->profile_combo));

  if (profile != OGMRIP_X264_PROFILE_HIGH)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->x88dct_check), FALSE);
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->cqm_combo), 0);
  }

  gtk_widget_set_sensitive (dialog->x88dct_check, x264_have_8x8dct && profile == OGMRIP_X264_PROFILE_HIGH);
  gtk_widget_set_sensitive (dialog->cqm_combo, profile == OGMRIP_X264_PROFILE_HIGH);

  if (profile == OGMRIP_X264_PROFILE_BASELINE)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->cabac_check), FALSE);
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->weight_p_combo), 0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->bframes_spin), 0);
  }

  gtk_widget_set_sensitive (dialog->cabac_check, profile != OGMRIP_X264_PROFILE_BASELINE);
  gtk_widget_set_sensitive (dialog->weight_p_combo, x264_have_weight_p && profile != OGMRIP_X264_PROFILE_BASELINE);
  gtk_widget_set_sensitive (dialog->bframes_spin, profile != OGMRIP_X264_PROFILE_BASELINE);
}

static void
ogmrip_x264_dialog_bframes_changed (OGMRipX264Dialog *dialog)
{
  gint bframes;

  bframes = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->bframes_spin));

  gtk_widget_set_sensitive (dialog->b_pyramid_check, bframes > 1);
  gtk_widget_set_sensitive (dialog->b_pyramid_combo, bframes > 1);
  gtk_widget_set_sensitive (dialog->weight_b_check, bframes > 1);

  if (bframes <= 1)
  {
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->b_pyramid_combo), 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->b_pyramid_check), FALSE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->weight_b_check), FALSE);
  }
}

static void
ogmrip_x264_dialog_subq_changed (OGMRipX264Dialog *dialog)
{
  gint subq;

  subq = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->subq_spin));

  gtk_widget_set_sensitive (dialog->brdo_check, x264_have_brdo && subq > 5);
  gtk_widget_set_sensitive (dialog->psy_rd_spin, x264_have_psy && subq > 5);

  if (subq <= 5)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->brdo_check), FALSE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->psy_rd_spin), 0.0);
  }
}

static void
ogmrip_x264_dialog_frameref_changed (OGMRipX264Dialog *dialog)
{
  gint frameref;

  frameref = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->frameref_spin));

  gtk_widget_set_sensitive (dialog->mixed_refs_check, x264_have_mixed_refs && frameref > 1);
  if (frameref <= 1)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->mixed_refs_check), FALSE);
}
*/
static gboolean
ogmrip_x264_dialog_set_vbv_bufsize_sensitivity (GBinding *binding,
    const GValue *source_value, GValue *target_value, gpointer data)
{
  g_value_set_boolean (target_value, g_value_get_int (source_value) > 0);

  return TRUE;
}

G_DEFINE_TYPE (OGMRipX264Dialog, ogmrip_x264_dialog, OGMRIP_TYPE_PLUGIN_DIALOG)

static void
ogmrip_x264_dialog_constructed (GObject *gobject)
{
  GError *error = NULL;

  OGMRipProfile *profile;
  GSettings *settings;

  GtkWidget *misc, *widget;
  GtkBuilder *builder;

  G_OBJECT_CLASS (ogmrip_x264_dialog_parent_class)->constructed (gobject);

  profile = ogmrip_plugin_dialog_get_profile (OGMRIP_PLUGIN_DIALOG (gobject));
  if (!profile)
  {
    g_critical ("No profile has been specified");
    return;
  }

  settings = ogmrip_profile_get_child (profile, "lavc");
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

  if (x264_have_b_pyramid)
    g_settings_bind (settings, OGMRIP_X264_PROP_B_PYRAMID,
        widget, "active", G_SETTINGS_BIND_DEFAULT);
  else
    g_settings_bind (settings, OGMRIP_X264_PROP_B_PYRAMID,
        widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "profile-combo");
  g_settings_bind (settings, OGMRIP_X264_PROP_PROFILE,
      widget, "active", G_SETTINGS_BIND_DEFAULT);
/*
  g_signal_connect_swapped (widget, "changed",
      G_CALLBACK (ogmrip_x264_dialog_profile_changed), dialog);
*/
  widget = gtk_builder_get_widget (builder, "bframes-spin");
  g_settings_bind (settings, OGMRIP_X264_PROP_MAX_BFRAMES,
      widget, "value", G_SETTINGS_BIND_DEFAULT);
/*
  g_signal_connect_swapped (widget, "value-changed",
      G_CALLBACK (ogmrip_x264_dialog_bframes_changed), dialog);
*/
  widget = gtk_builder_get_widget (builder, "cabac-check");
  g_settings_bind (settings, OGMRIP_X264_PROP_CABAC,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "cqm-combo");
  g_settings_bind (settings, OGMRIP_X264_PROP_CQM,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "subq-spin");
  g_settings_bind (settings, OGMRIP_X264_PROP_SUBQ,
      widget, "value", G_SETTINGS_BIND_DEFAULT);
/*
  g_signal_connect_swapped (widget, "value-changed",
      G_CALLBACK (ogmrip_x264_dialog_subq_changed), dialog);
*/
  widget = gtk_builder_get_widget (builder, "global_header-check");
  g_settings_bind (settings, OGMRIP_X264_PROP_GLOBAL_HEADER,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "weight_b-check");
  g_settings_bind (settings, OGMRIP_X264_PROP_WEIGHT_B,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "partitions-check");
  g_settings_bind (settings, OGMRIP_X264_PROP_PARTITIONS,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "weight_p-combo");
  gtk_widget_set_sensitive (widget, x264_have_weight_p);
  g_settings_bind (settings, OGMRIP_X264_PROP_WEIGHT_P,
      widget, "active", G_SETTINGS_BIND_DEFAULT);
  
  widget = gtk_builder_get_widget (builder, "b_pyramid-check");
  g_object_set (widget, "visible", !x264_have_b_pyramid, NULL);

  widget = gtk_builder_get_widget (builder, "b_pyramid-combo");
  g_object_set (widget, "visible", x264_have_b_pyramid, NULL);

  widget = gtk_builder_get_widget (builder, "b_pyramid-label");
  g_object_set (widget, "visible", x264_have_b_pyramid, NULL);

  widget = gtk_builder_get_widget (builder, "frameref-spin");
  g_settings_bind (settings, OGMRIP_X264_PROP_FRAMEREF,
      widget, "value", G_SETTINGS_BIND_DEFAULT);
/*
  g_signal_connect_swapped (widget, "value-changed",
      G_CALLBACK (ogmrip_x264_dialog_frameref_changed), dialog);
*/
  widget = gtk_builder_get_widget (builder, "me-combo");
  gtk_widget_set_sensitive (widget, x264_have_me);

  if (x264_have_me_tesa)
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widget),
        _("Transformed Exhaustive search (tesa - even slower)"));

  g_settings_bind_with_mapping (settings, OGMRIP_X264_PROP_ME,
      widget, "active", G_SETTINGS_BIND_DEFAULT,
      ogmrip_x264_get_me, ogmrip_x264_set_me, NULL, NULL);

  widget = gtk_builder_get_widget (builder, "merange-spin");
  gtk_widget_set_sensitive (widget, x264_have_me);
  g_settings_bind (settings, OGMRIP_X264_PROP_MERANGE,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "dct8x8-check");
  gtk_widget_set_sensitive (widget, x264_have_8x8dct);
  g_settings_bind (settings, OGMRIP_X264_PROP_8X8DCT,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "mixed_refs-check");
  gtk_widget_set_sensitive (widget, x264_have_mixed_refs);
  g_settings_bind (settings, OGMRIP_X264_PROP_MIXED_REFS,
      widget, "active", G_SETTINGS_BIND_DEFAULT);
  
  widget = gtk_builder_get_widget (builder, "brdo-check");
  gtk_widget_set_sensitive (widget, x264_have_brdo);
  g_settings_bind (settings, OGMRIP_X264_PROP_BRDO,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "vbv_maxrate-spin");
  g_settings_bind (settings, OGMRIP_X264_PROP_VBV_MAXRATE,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  misc = gtk_builder_get_widget (builder, "vbv_bufsize-spin");
  g_settings_bind (settings, OGMRIP_X264_PROP_VBV_BUFSIZE,
      misc, "value", G_SETTINGS_BIND_DEFAULT);

  g_object_bind_property_full (widget, "value", misc, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_x264_dialog_set_vbv_bufsize_sensitivity, NULL, NULL, NULL);

  widget = gtk_builder_get_widget (builder, "level_idc-spin");
  g_settings_bind (settings, OGMRIP_X264_PROP_LEVEL_IDC,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "direct-combo");
  g_settings_bind (settings, OGMRIP_X264_PROP_DIRECT,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "b_adapt-spin");
  g_settings_bind (settings, OGMRIP_X264_PROP_B_ADAPT,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "keyint-spin");
  g_settings_bind (settings, OGMRIP_X264_PROP_KEYINT,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "psy_rd-spin");
  gtk_widget_set_sensitive (widget, x264_have_psy);
  g_settings_bind (settings, OGMRIP_X264_PROP_PSY_RD,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "psy_trellis-spin");
  gtk_widget_set_sensitive (widget, x264_have_psy);
  g_settings_bind (settings, OGMRIP_X264_PROP_PSY_TRELLIS,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "aud-check");
  gtk_widget_set_sensitive (widget, x264_have_aud);
  g_settings_bind (settings, OGMRIP_X264_PROP_AUD,
      widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "rc_lookahead-spin");
  gtk_widget_set_sensitive (widget, x264_have_lookahead);
  g_settings_bind (settings, OGMRIP_X264_PROP_RC_LOOKAHEAD,
      widget, "value", G_SETTINGS_BIND_DEFAULT);

  g_object_unref (builder);
}

static void
ogmrip_x264_dialog_class_init (OGMRipX264DialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_x264_dialog_constructed;
}

static void
ogmrip_x264_dialog_init (OGMRipX264Dialog *dialog)
{
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
      NULL);
  gtk_window_set_title (GTK_WINDOW (dialog), _("X264 Options"));
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_window_set_icon_from_stock (GTK_WINDOW (dialog), GTK_STOCK_PREFERENCES);
}

static OGMRipVideoOptionsPlugin x264_options_plugin =
{
  NULL,
  G_TYPE_NONE,
  G_TYPE_NONE
};

OGMRipVideoOptionsPlugin *
ogmrip_init_options_plugin (void)
{
  GModule *module;

  x264_options_plugin.type = ogmrip_plugin_get_video_codec_by_name ("x264");
  if (x264_options_plugin.type == G_TYPE_NONE)
    return NULL;

  module = ogmrip_plugin_get_video_codec_module (x264_options_plugin.type);
  if (module)
  {
    gboolean *symbol;

    if (g_module_symbol (module, "x264_have_8x8dct", (gpointer *) &symbol))
      x264_have_8x8dct = *symbol;

    if (g_module_symbol (module, "x264_have_brdo", (gpointer *) &symbol))
      x264_have_brdo = *symbol;

    if (g_module_symbol (module, "x264_have_psy", (gpointer *) &symbol))
      x264_have_psy = *symbol;

    if (g_module_symbol (module, "x264_have_aud", (gpointer *) &symbol))
      x264_have_aud = *symbol;

    if (g_module_symbol (module, "x264_have_lookahead", (gpointer *) &symbol))
      x264_have_lookahead = *symbol;

    if (g_module_symbol (module, "x264_have_me", (gpointer *) &symbol))
      x264_have_me = *symbol;

    if (x264_have_me && g_module_symbol (module, "x264_have_me_tesa", (gpointer *) &symbol))
      x264_have_me_tesa = *symbol;

    if (g_module_symbol (module, "x264_have_mixed_refs", (gpointer *) &symbol))
      x264_have_mixed_refs = *symbol;

    if (g_module_symbol (module, "x264_have_b_pyramid", (gpointer *) &symbol))
      x264_have_b_pyramid = *symbol;

    if (g_module_symbol (module, "x264_have_weight_p", (gpointer *) &symbol))
      x264_have_weight_p = *symbol;
  }
/*
  settings = ogmrip_settings_get_default ();
  if (settings)
  {
    ogmrip_settings_install_key (settings,
        g_param_spec_uint (OGMRIP_X264_SECTION "/" OGMRIP_X264_PROP_PROFILE, "Profile property", "Set profile",
          OGMRIP_X264_PROFILE_BASELINE, OGMRIP_X264_PROFILE_HIGH, OGMRIP_X264_DEFAULT_PROFILE, G_PARAM_READWRITE));
  }
*/
  x264_options_plugin.dialog = OGMRIP_TYPE_X264_DIALOG;

  return &x264_options_plugin;
}

