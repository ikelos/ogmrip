/* OGMRipX264Options - An X264 options plugin for OGMRip
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

#include "ogmrip-x264.h"

#include <glib/gi18n.h>

#define OGMRIP_UI_FILE "ogmrip" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-x264-options-dialog.ui"
#define OGMRIP_UI_ROOT "root"

#define gtk_builder_get_widget(builder, name) \
    (GtkWidget *) gtk_builder_get_object ((builder), (name))

#define OGMRIP_TYPE_X264_DIALOG          (ogmrip_x264_dialog_get_type ())
#define OGMRIP_X264_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_X264_DIALOG, OGMRipX264Dialog))
#define OGMRIP_X264_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_X264_DIALOG, OGMRipX264DialogClass))
#define OGMRIP_IS_X264_DIALOG(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_X264_DIALOG))
#define OGMRIP_IS_X264_DIALOG_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_X264_DIALOG))

typedef struct _OGMRipX264Dialog      OGMRipX264Dialog;
typedef struct _OGMRipX264DialogClass OGMRipX264DialogClass;

struct _OGMRipX264Dialog
{
  GtkDialog parent_instance;

  OGMRipProfile *profile;

  GtkWidget *aq_mode_combo;
  GtkWidget *aud_check;
  GtkWidget *b_adapt_spin;
  GtkWidget *bframes_spin;
  GtkWidget *b_pyramid_check;
  GtkWidget *b_pyramid_combo;
  GtkWidget *b_pyramid_label;
  GtkWidget *brdo_check;
  GtkWidget *cabac_check;
  GtkWidget *cqm_combo;
  GtkWidget *dct8x8_check;
  GtkWidget *dct_decimate_check;
  GtkWidget *direct_combo;
  GtkWidget *fast_pskip_check;
  GtkWidget *force_cfr_check;
  GtkWidget *frameref_spin;
  GtkWidget *global_header_check;
  GtkWidget *keyint_spin;
  GtkWidget *level_idc_spin;
  GtkWidget *me_combo;
  GtkWidget *merange_spin;
  GtkWidget *mixed_refs_check;
  GtkWidget *profile_combo;
  GtkWidget *psy_rd_spin;
  GtkWidget *psy_trellis_spin;
  GtkWidget *rc_lookahead_spin;
  GtkWidget *subq_spin;
  GtkWidget *trellis_combo;
  GtkWidget *vbv_bufsize_spin;
  GtkWidget *vbv_maxrate_spin;
  GtkWidget *weight_b_check;
  GtkWidget *weight_p_combo;
  GtkWidget *partitions_check[5];
};

struct _OGMRipX264DialogClass
{
  GtkDialogClass parent_class;
};

enum
{
  PROP_0,
  PROP_PROFILE
};

enum
{
  BASELINE,
  MAIN,
  HIGH
};

enum
{
  B8X8,
  I8X8,
  P8X8,
  I4X4,
  P4X4,
  LAST
};

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

static void ogmrip_options_editable_init (OGMRipOptionsEditableInterface *iface);

static const gchar *partitions[] =
{
  "b8x8",
  "i8x8",
  "p8x8",
  "i4x4",
  "p4x4"
};

static gboolean
ogmrip_x264_get_b8x8 (GValue *value, GVariant *variant, gpointer user_data)
{
  const gchar **strv;
  guint i;

  strv = g_variant_get_strv (variant, NULL);
  for (i = 0; strv[i]; i ++)
    if (g_str_equal (strv[i], "b8x8"))
      break;

  g_value_set_boolean (value, strv[i] != NULL);

  return TRUE;
}

static gboolean
ogmrip_x264_get_i8x8 (GValue *value, GVariant *variant, gpointer user_data)
{
  const gchar **strv;
  guint i;

  strv = g_variant_get_strv (variant, NULL);
  for (i = 0; strv[i]; i ++)
    if (g_str_equal (strv[i], "i8x8"))
      break;

  g_value_set_boolean (value, strv[i] != NULL);

  return TRUE;
}

static gboolean
ogmrip_x264_get_p8x8 (GValue *value, GVariant *variant, gpointer user_data)
{
  const gchar **strv;
  guint i;

  strv = g_variant_get_strv (variant, NULL);
  for (i = 0; strv[i]; i ++)
    if (g_str_equal (strv[i], "p8x8"))
      break;

  g_value_set_boolean (value, strv[i] != NULL);

  return TRUE;
}

static gboolean
ogmrip_x264_get_i4x4 (GValue *value, GVariant *variant, gpointer user_data)
{
  const gchar **strv;
  guint i;

  strv = g_variant_get_strv (variant, NULL);
  for (i = 0; strv[i]; i ++)
    if (g_str_equal (strv[i], "i4x4"))
      break;

  g_value_set_boolean (value, strv[i] != NULL);

  return TRUE;
}

static gboolean
ogmrip_x264_get_p4x4 (GValue *value, GVariant *variant, gpointer user_data)
{
  const gchar **strv;
  guint i;

  strv = g_variant_get_strv (variant, NULL);
  for (i = 0; strv[i]; i ++)
    if (g_str_equal (strv[i], "p4x4"))
      break;

  g_value_set_boolean (value, strv[i] != NULL);

  return TRUE;
}

static GVariant *
ogmrip_x264_set_partitions (const GValue *value, const GVariantType *type, gpointer user_data)
{
  GtkWidget **checks = user_data;
  const gchar *strv[LAST];
  guint i, j;

  for (i = 0, j = 0; i < LAST; i ++)
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checks[i])))
      strv[j ++] = partitions[i];

  return g_variant_new_strv (strv, j);
}

static gboolean
ogmrip_x264_get_me (GValue *value, GVariant *variant, gpointer user_data)
{
  g_value_set_int (value, g_variant_get_uint32 (variant) - 1);

  return TRUE;
}

static GVariant *
ogmrip_x264_set_me (const GValue *value, const GVariantType *type, gpointer user_data)
{
  return g_variant_new_uint32 (g_value_get_int (value) + 1);
}

static void
ogmrip_x264_dialog_profile_changed (OGMRipX264Dialog *dialog)
{
  gint profile;

  profile = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->profile_combo));

  if (profile != HIGH)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->dct8x8_check), FALSE);
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->cqm_combo), 0);
  }

  gtk_widget_set_sensitive (dialog->dct8x8_check, x264_have_8x8dct && profile == HIGH);
  gtk_widget_set_sensitive (dialog->cqm_combo, profile == HIGH);

  if (profile == BASELINE)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->cabac_check), FALSE);
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->weight_p_combo), 0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->bframes_spin), 0);
  }

  gtk_widget_set_sensitive (dialog->cabac_check, profile != BASELINE);
  gtk_widget_set_sensitive (dialog->weight_p_combo, x264_have_weight_p && profile != BASELINE);
  gtk_widget_set_sensitive (dialog->bframes_spin, profile != BASELINE);
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

static gboolean
ogmrip_x264_dialog_set_vbv_bufsize_sensitivity (GBinding *binding,
    const GValue *source_value, GValue *target_value, gpointer data)
{
  gint value;

  value = g_value_get_double (source_value);
  g_value_set_boolean (target_value, value > 0);

  return TRUE;
}

G_DEFINE_TYPE_EXTENDED (OGMRipX264Dialog, ogmrip_x264_dialog, GTK_TYPE_DIALOG, 0,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_OPTIONS_EDITABLE, ogmrip_options_editable_init));

static void
ogmrip_x264_dialog_set_profile (OGMRipX264Dialog *dialog, OGMRipProfile *profile)
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

    settings = ogmrip_profile_get_child (profile, "x264");
    g_assert (settings != NULL);

    g_settings_bind (settings, OGMRIP_X264_PROP_PROFILE,
        dialog->profile_combo, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_B_FRAMES,
        dialog->bframes_spin, "value", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_CABAC,
        dialog->cabac_check, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_CQM,
        dialog->cqm_combo, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_SUBQ,
        dialog->subq_spin, "value", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_WEIGHT_B,
        dialog->weight_b_check, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_DCT_DECIMATE,
        dialog->dct_decimate_check, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_FAST_PSKIP,
        dialog->fast_pskip_check, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_FORCE_CFR,
        dialog->force_cfr_check, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_GLOBAL_HEADER,
        dialog->global_header_check, "active", G_SETTINGS_BIND_DEFAULT);

    g_settings_bind_with_mapping (settings, OGMRIP_X264_PROP_PARTITIONS,
        dialog->partitions_check[B8X8], "active", G_SETTINGS_BIND_DEFAULT,
        ogmrip_x264_get_b8x8, ogmrip_x264_set_partitions, dialog->partitions_check, NULL);
    g_settings_bind_with_mapping (settings, OGMRIP_X264_PROP_PARTITIONS,
        dialog->partitions_check[I8X8], "active", G_SETTINGS_BIND_DEFAULT,
        ogmrip_x264_get_i8x8, ogmrip_x264_set_partitions, dialog->partitions_check, NULL);
    g_settings_bind_with_mapping (settings, OGMRIP_X264_PROP_PARTITIONS,
        dialog->partitions_check[P8X8], "active", G_SETTINGS_BIND_DEFAULT,
        ogmrip_x264_get_p8x8, ogmrip_x264_set_partitions, dialog->partitions_check, NULL);
    g_settings_bind_with_mapping (settings, OGMRIP_X264_PROP_PARTITIONS,
        dialog->partitions_check[I4X4], "active", G_SETTINGS_BIND_DEFAULT,
        ogmrip_x264_get_i4x4, ogmrip_x264_set_partitions, dialog->partitions_check, NULL);
    g_settings_bind_with_mapping (settings, OGMRIP_X264_PROP_PARTITIONS,
        dialog->partitions_check[P4X4], "active", G_SETTINGS_BIND_DEFAULT,
        ogmrip_x264_get_p4x4, ogmrip_x264_set_partitions, dialog->partitions_check, NULL);

    g_settings_bind (settings, OGMRIP_X264_PROP_WEIGHT_P,
        dialog->weight_p_combo, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_FRAMEREF,
        dialog->frameref_spin, "value", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind_with_mapping (settings, OGMRIP_X264_PROP_ME,
        dialog->me_combo, "active", G_SETTINGS_BIND_DEFAULT,
        ogmrip_x264_get_me, ogmrip_x264_set_me, NULL, NULL);
    g_settings_bind (settings, OGMRIP_X264_PROP_MERANGE,
        dialog->merange_spin, "value", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_8X8DCT,
        dialog->dct8x8_check, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_MIXED_REFS,
        dialog->mixed_refs_check, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_BRDO,
        dialog->brdo_check, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_VBV_MAXRATE,
        dialog->vbv_maxrate_spin, "value", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_VBV_BUFSIZE,
        dialog->vbv_bufsize_spin, "value", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_LEVEL_IDC,
        dialog->level_idc_spin, "value", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_DIRECT,
        dialog->direct_combo, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_B_ADAPT,
        dialog->b_adapt_spin, "value", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_KEYINT,
        dialog->keyint_spin, "value", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_PSY_RD,
        dialog->psy_rd_spin, "value", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_PSY_TRELLIS,
        dialog->psy_trellis_spin, "value", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_AUD,
        dialog->aud_check, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_RC_LOOKAHEAD,
        dialog->rc_lookahead_spin, "value", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_TRELLIS,
        dialog->trellis_combo, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (settings, OGMRIP_X264_PROP_AQ_MODE,
        dialog->aq_mode_combo, "active", G_SETTINGS_BIND_DEFAULT);

    if (x264_have_b_pyramid)
      g_settings_bind (settings, OGMRIP_X264_PROP_B_PYRAMID,
          dialog->b_pyramid_combo, "active", G_SETTINGS_BIND_DEFAULT);
    else
      g_settings_bind (settings, OGMRIP_X264_PROP_B_PYRAMID,
          dialog->b_pyramid_check, "active", G_SETTINGS_BIND_DEFAULT);

    g_object_unref (settings);
  }
}

static void
ogmrip_x264_dialog_dispose (GObject *gobject)
{
  OGMRipX264Dialog *dialog = OGMRIP_X264_DIALOG (gobject);

  if (dialog->profile)
  {
    g_object_unref (dialog->profile);
    dialog->profile = NULL;
  }

  G_OBJECT_CLASS (ogmrip_x264_dialog_parent_class)->dispose (gobject);
}

static void
ogmrip_x264_dialog_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
  {
    case PROP_PROFILE:
      g_value_set_object (value, OGMRIP_X264_DIALOG (gobject)->profile);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_x264_dialog_set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
  {
    case PROP_PROFILE:
      ogmrip_x264_dialog_set_profile (OGMRIP_X264_DIALOG (gobject), g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_x264_dialog_class_init (OGMRipX264DialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_x264_dialog_dispose;
  gobject_class->get_property = ogmrip_x264_dialog_get_property;
  gobject_class->set_property = ogmrip_x264_dialog_set_property;

  g_object_class_override_property (gobject_class, PROP_PROFILE, "profile");
}

static void
ogmrip_x264_dialog_init (OGMRipX264Dialog *dialog)
{
  GError *error = NULL;

  GtkBuilder *builder;
  GtkWidget *misc, *widget;

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
      _("_Close"), GTK_RESPONSE_CLOSE,
      NULL);
  gtk_window_set_title (GTK_WINDOW (dialog), _("X264 Options"));
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_UI_FILE, &error))
    g_error ("Couldn't load builder file: %s", error->message);

  misc = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  widget = gtk_builder_get_widget (builder, OGMRIP_UI_ROOT);
  gtk_container_add (GTK_CONTAINER (misc), widget);
  gtk_widget_show (widget);

  dialog->profile_combo = gtk_builder_get_widget (builder, "profile-combo");
  g_signal_connect_swapped (dialog->profile_combo, "changed",
      G_CALLBACK (ogmrip_x264_dialog_profile_changed), dialog);

  dialog->bframes_spin = gtk_builder_get_widget (builder, "bframes-spin");
  g_signal_connect_swapped (dialog->bframes_spin, "value-changed",
      G_CALLBACK (ogmrip_x264_dialog_bframes_changed), dialog);

  dialog->cabac_check = gtk_builder_get_widget (builder, "cabac-check");
  dialog->cqm_combo = gtk_builder_get_widget (builder, "cqm-combo");

  dialog->subq_spin = gtk_builder_get_widget (builder, "subq-spin");
  g_signal_connect_swapped (dialog->subq_spin, "value-changed",
      G_CALLBACK (ogmrip_x264_dialog_subq_changed), dialog);

  dialog->weight_b_check = gtk_builder_get_widget (builder, "weight_b-check");
  dialog->partitions_check[B8X8] = gtk_builder_get_widget (builder, "b8x8-check");
  dialog->partitions_check[I8X8] = gtk_builder_get_widget (builder, "i8x8-check");
  dialog->partitions_check[P8X8] = gtk_builder_get_widget (builder, "p8x8-check");
  dialog->partitions_check[I4X4] = gtk_builder_get_widget (builder, "i4x4-check");
  dialog->partitions_check[P4X4] = gtk_builder_get_widget (builder, "p4x4-check");

  dialog->weight_p_combo = gtk_builder_get_widget (builder, "weight_p-combo");
  gtk_widget_set_sensitive (dialog->weight_p_combo, x264_have_weight_p);
  
  dialog->b_pyramid_check = gtk_builder_get_widget (builder, "b_pyramid-check");
  gtk_widget_set_visible (dialog->b_pyramid_check, !x264_have_b_pyramid);

  dialog->b_pyramid_combo = gtk_builder_get_widget (builder, "b_pyramid-combo");
  gtk_widget_set_visible (dialog->b_pyramid_combo, x264_have_b_pyramid);

  dialog->b_pyramid_label = gtk_builder_get_widget (builder, "b_pyramid-label");
  gtk_widget_set_visible (dialog->b_pyramid_label, x264_have_b_pyramid);

  dialog->frameref_spin = gtk_builder_get_widget (builder, "frameref-spin");
  g_signal_connect_swapped (dialog->frameref_spin, "value-changed",
      G_CALLBACK (ogmrip_x264_dialog_frameref_changed), dialog);

  dialog->me_combo = gtk_builder_get_widget (builder, "me-combo");
  gtk_widget_set_sensitive (dialog->me_combo, x264_have_me);

  if (x264_have_me_tesa)
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (dialog->me_combo),
        _("Transformed Exhaustive search (tesa - even slower)"));

  dialog->merange_spin = gtk_builder_get_widget (builder, "merange-spin");
  gtk_widget_set_sensitive (dialog->merange_spin, x264_have_me);

  dialog->dct8x8_check = gtk_builder_get_widget (builder, "dct8x8-check");
  gtk_widget_set_sensitive (dialog->dct8x8_check, x264_have_8x8dct);

  dialog->mixed_refs_check = gtk_builder_get_widget (builder, "mixed_refs-check");
  gtk_widget_set_sensitive (dialog->mixed_refs_check, x264_have_mixed_refs);
  
  dialog->brdo_check = gtk_builder_get_widget (builder, "brdo-check");
  gtk_widget_set_sensitive (dialog->brdo_check, x264_have_brdo);

  dialog->vbv_maxrate_spin = gtk_builder_get_widget (builder, "vbv_maxrate-spin");
  dialog->vbv_bufsize_spin = gtk_builder_get_widget (builder, "vbv_bufsize-spin");

  g_object_bind_property_full (dialog->vbv_maxrate_spin, "value",
      dialog->vbv_bufsize_spin, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_x264_dialog_set_vbv_bufsize_sensitivity, NULL, NULL, NULL);

  dialog->level_idc_spin = gtk_builder_get_widget (builder, "level_idc-spin");
  dialog->direct_combo = gtk_builder_get_widget (builder, "direct-combo");
  dialog->b_adapt_spin = gtk_builder_get_widget (builder, "b_adapt-spin");
  dialog->keyint_spin = gtk_builder_get_widget (builder, "keyint-spin");
  dialog->aq_mode_combo = gtk_builder_get_widget (builder, "aq_mode-combo");
  dialog->dct_decimate_check = gtk_builder_get_widget (builder, "dct_decimate-check");
  dialog->fast_pskip_check = gtk_builder_get_widget (builder, "fast_pskip-check");
  dialog->force_cfr_check = gtk_builder_get_widget (builder, "force_cfr-check");
  dialog->global_header_check = gtk_builder_get_widget (builder, "global_header-check");

  dialog->psy_rd_spin = gtk_builder_get_widget (builder, "psy_rd-spin");
  gtk_widget_set_sensitive (dialog->psy_rd_spin, x264_have_psy);

  dialog->psy_trellis_spin = gtk_builder_get_widget (builder, "psy_trellis-spin");
  gtk_widget_set_sensitive (dialog->psy_trellis_spin, x264_have_psy);

  dialog->aud_check = gtk_builder_get_widget (builder, "aud-check");
  gtk_widget_set_sensitive (dialog->aud_check, x264_have_aud);

  dialog->rc_lookahead_spin = gtk_builder_get_widget (builder, "rc_lookahead-spin");
  gtk_widget_set_sensitive (dialog->rc_lookahead_spin, x264_have_lookahead);

  dialog->trellis_combo = gtk_builder_get_widget (builder, "trellis-combo");

  g_object_unref (builder);
}

static void
ogmrip_options_editable_init (OGMRipOptionsEditableInterface *iface)
{
}

void
ogmrip_module_load (OGMRipModule *module)
{
  OGMRipModuleEngine *engine;
  GType gtype;
  gboolean *symbol;

  engine = ogmrip_module_engine_get_default ();

  gtype = ogmrip_type_from_name ("x264");
  if (gtype == G_TYPE_NONE)
    return;

  ogmrip_type_add_extension (gtype, OGMRIP_TYPE_X264_DIALOG);

  module = ogmrip_module_engine_get (engine, "ogmrip-x264");
  if (!module)
    return;

  if (ogmrip_module_get_symbol (module, "x264_have_8x8dct", (gpointer *) &symbol))
    x264_have_8x8dct = *symbol;

  if (ogmrip_module_get_symbol (module, "x264_have_brdo", (gpointer *) &symbol))
    x264_have_brdo = *symbol;

  if (ogmrip_module_get_symbol (module, "x264_have_psy", (gpointer *) &symbol))
    x264_have_psy = *symbol;

  if (ogmrip_module_get_symbol (module, "x264_have_aud", (gpointer *) &symbol))
    x264_have_aud = *symbol;

  if (ogmrip_module_get_symbol (module, "x264_have_lookahead", (gpointer *) &symbol))
    x264_have_lookahead = *symbol;

  if (ogmrip_module_get_symbol (module, "x264_have_me", (gpointer *) &symbol))
    x264_have_me = *symbol;

  if (x264_have_me && ogmrip_module_get_symbol (module, "x264_have_me_tesa", (gpointer *) &symbol))
    x264_have_me_tesa = *symbol;

  if (ogmrip_module_get_symbol (module, "x264_have_mixed_refs", (gpointer *) &symbol))
    x264_have_mixed_refs = *symbol;

  if (ogmrip_module_get_symbol (module, "x264_have_b_pyramid", (gpointer *) &symbol))
    x264_have_b_pyramid = *symbol;

  if (ogmrip_module_get_symbol (module, "x264_have_weight_p", (gpointer *) &symbol))
    x264_have_weight_p = *symbol;
}

