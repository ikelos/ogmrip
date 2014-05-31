/* OGMRipX264Options - An X264 options plugin for OGMRip
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

#include <ogmrip-encode-gtk.h>
#include <ogmrip-module.h>

#include "ogmrip-x264.h"

#include <glib/gi18n.h>

#define OGMRIP_UI_RES  "/org/ogmrip/ogmrip-x264-options-dialog.ui"
#define OGMRIP_UI_ROOT "root"

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
  GtkWidget *b8x8_check;
  GtkWidget *i8x8_check;
  GtkWidget *p8x8_check;
  GtkWidget *i4x4_check;
  GtkWidget *p4x4_check;
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
  OGMRipX264Dialog *dialog = user_data;
  const gchar *strv[LAST];
  guint i = 0;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->b8x8_check)))
    strv[i ++] = "b8x8";

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->i8x8_check)))
    strv[i ++] = "i8x8";

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->p8x8_check)))
    strv[i ++] = "p8x8";

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->i4x4_check)))
    strv[i ++] = "i4x4";

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->p4x4_check)))
    strv[i ++] = "p4x4";

  return g_variant_new_strv (strv, i);
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

G_DEFINE_TYPE_WITH_CODE (OGMRipX264Dialog, ogmrip_x264_dialog, GTK_TYPE_DIALOG,
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
        dialog->b8x8_check, "active", G_SETTINGS_BIND_DEFAULT,
        ogmrip_x264_get_b8x8, ogmrip_x264_set_partitions, dialog, NULL);
    g_settings_bind_with_mapping (settings, OGMRIP_X264_PROP_PARTITIONS,
        dialog->i8x8_check, "active", G_SETTINGS_BIND_DEFAULT,
        ogmrip_x264_get_i8x8, ogmrip_x264_set_partitions, dialog, NULL);
    g_settings_bind_with_mapping (settings, OGMRIP_X264_PROP_PARTITIONS,
        dialog->p8x8_check, "active", G_SETTINGS_BIND_DEFAULT,
        ogmrip_x264_get_p8x8, ogmrip_x264_set_partitions, dialog, NULL);
    g_settings_bind_with_mapping (settings, OGMRIP_X264_PROP_PARTITIONS,
        dialog->i4x4_check, "active", G_SETTINGS_BIND_DEFAULT,
        ogmrip_x264_get_i4x4, ogmrip_x264_set_partitions, dialog, NULL);
    g_settings_bind_with_mapping (settings, OGMRIP_X264_PROP_PARTITIONS,
        dialog->p4x4_check, "active", G_SETTINGS_BIND_DEFAULT,
        ogmrip_x264_get_p4x4, ogmrip_x264_set_partitions, dialog, NULL);

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

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), OGMRIP_UI_RES);

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, aq_mode_combo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, aud_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, b_adapt_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, bframes_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, b_pyramid_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, b_pyramid_combo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, b_pyramid_label);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, brdo_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, cabac_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, cqm_combo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, dct8x8_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, dct_decimate_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, direct_combo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, fast_pskip_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, force_cfr_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, frameref_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, global_header_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, keyint_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, level_idc_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, me_combo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, merange_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, mixed_refs_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, profile_combo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, psy_rd_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, psy_trellis_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, rc_lookahead_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, subq_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, trellis_combo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, vbv_bufsize_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, vbv_maxrate_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, weight_b_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, weight_p_combo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, b8x8_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, i8x8_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, p8x8_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, i4x4_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipX264Dialog, p4x4_check);
}

static void
ogmrip_x264_dialog_init (OGMRipX264Dialog *dialog)
{
  gtk_widget_init_template (GTK_WIDGET (dialog));

  gtk_widget_set_sensitive (dialog->weight_p_combo, x264_have_weight_p);
  gtk_widget_set_sensitive (dialog->me_combo, x264_have_me);
  gtk_widget_set_sensitive (dialog->merange_spin, x264_have_me);
  gtk_widget_set_sensitive (dialog->dct8x8_check, x264_have_8x8dct);
  gtk_widget_set_sensitive (dialog->mixed_refs_check, x264_have_mixed_refs);
  gtk_widget_set_sensitive (dialog->brdo_check, x264_have_brdo);
  gtk_widget_set_sensitive (dialog->psy_rd_spin, x264_have_psy);
  gtk_widget_set_sensitive (dialog->psy_trellis_spin, x264_have_psy);
  gtk_widget_set_sensitive (dialog->aud_check, x264_have_aud);
  gtk_widget_set_sensitive (dialog->rc_lookahead_spin, x264_have_lookahead);
  
  gtk_widget_set_visible (dialog->b_pyramid_check, !x264_have_b_pyramid);
  gtk_widget_set_visible (dialog->b_pyramid_combo, x264_have_b_pyramid);
  gtk_widget_set_visible (dialog->b_pyramid_label, x264_have_b_pyramid);

  if (x264_have_me_tesa)
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (dialog->me_combo),
        _("Transformed Exhaustive search (tesa - even slower)"));

  g_object_bind_property_full (dialog->vbv_maxrate_spin, "value",
      dialog->vbv_bufsize_spin, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_x264_dialog_set_vbv_bufsize_sensitivity, NULL, NULL, NULL);

  g_signal_connect_swapped (dialog->frameref_spin, "value-changed",
      G_CALLBACK (ogmrip_x264_dialog_frameref_changed), dialog);

  g_signal_connect_swapped (dialog->profile_combo, "changed",
      G_CALLBACK (ogmrip_x264_dialog_profile_changed), dialog);

  g_signal_connect_swapped (dialog->bframes_spin, "value-changed",
      G_CALLBACK (ogmrip_x264_dialog_bframes_changed), dialog);

  g_signal_connect_swapped (dialog->subq_spin, "value-changed",
      G_CALLBACK (ogmrip_x264_dialog_subq_changed), dialog);
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

