/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#include <ogmrip-encode-gtk.h>
#include <ogmrip-module.h>

#include "ogmrip-xvid.h"

#include <glib/gi18n.h>

#define OGMRIP_GLADE_FILE "ogmrip" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-xvid.glade"
#define OGMRIP_GLADE_ROOT "root"

#define gtk_builder_get_widget(builder, name) \
    (GtkWidget *) gtk_builder_get_object ((builder), (name))

#define OGMRIP_TYPE_XVID_DIALOG          (ogmrip_xvid_dialog_get_type ())
#define OGMRIP_XVID_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_XVID_DIALOG, OGMRipXvidDialog))
#define OGMRIP_XVID_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_XVID_DIALOG, OGMRipXvidDialogClass))
#define OGMRIP_IS_XVID_DIALOG(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_XVID_DIALOG))
#define OGMRIP_IS_XVID_DIALOG_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_XVID_DIALOG))

typedef struct _OGMRipXvidDialog      OGMRipXvidDialog;
typedef struct _OGMRipXvidDialogClass OGMRipXvidDialogClass;

struct _OGMRipXvidDialog
{
  GtkDialog parent_instance;

  OGMRipProfile *profile;

  GtkWidget *bquant_offset_spin;
  GtkWidget *bquant_ratio_spin;
  GtkWidget *bvhq_check;
  GtkWidget *cartoon_check;
  GtkWidget *chroma_me_check;
  GtkWidget *chroma_opt_check;
  GtkWidget *closed_gop_check;
  GtkWidget *frame_drop_ratio_spin;
  GtkWidget *gmc_check;
  GtkWidget *grayscale_check;
  GtkWidget *interlacing_check;
  GtkWidget *lumi_mask_check;
  GtkWidget *max_bframes_spin;
  GtkWidget *max_bquant_spin;
  GtkWidget *max_iquant_spin;
  GtkWidget *max_keyint_spin;
  GtkWidget *max_pquant_spin;
  GtkWidget *me_quality_combo;
  GtkWidget *min_bquant_spin;
  GtkWidget *min_iquant_spin;
  GtkWidget *min_pquant_spin;
  GtkWidget *packed_check;
  GtkWidget *par_combo;
  GtkWidget *par_height_spin;
  GtkWidget *par_width_spin;
  GtkWidget *profile_combo;
  GtkWidget *qpel_check;
  GtkWidget *quant_type_combo;
  GtkWidget *trellis_check;
  GtkWidget *vhq_combo;
};

struct _OGMRipXvidDialogClass
{
  GtkDialogClass parent_class;
};

enum
{
  PROP_0,
  PROP_PROFILE
};

static void ogmrip_options_editable_init (OGMRipOptionsEditableInterface *iface);

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

G_DEFINE_DYNAMIC_TYPE_EXTENDED (OGMRipXvidDialog, ogmrip_xvid_dialog, GTK_TYPE_DIALOG, 0,
    G_IMPLEMENT_INTERFACE_DYNAMIC (OGMRIP_TYPE_OPTIONS_EDITABLE, ogmrip_options_editable_init));

static void
ogmrip_xvid_dialog_set_profile (OGMRipXvidDialog *dialog, OGMRipProfile *profile)
{
  GSettings *settings;

  if (dialog->profile)
    g_object_unref (dialog->profile);
  dialog->profile = g_object_ref (profile);

  settings = ogmrip_profile_get_child (profile, "xvid");
  g_assert (settings != NULL);

  g_settings_bind (settings, OGMRIP_XVID_PROP_BQUANT_OFFSET,
      dialog->bquant_offset_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_BQUANT_RATIO,
      dialog->bquant_ratio_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_BVHQ,
      dialog->bvhq_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_CARTOON,
      dialog->cartoon_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_CHROMA_ME,
      dialog->chroma_me_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_CHROMA_OPT,
      dialog->chroma_opt_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_CLOSED_GOP,
      dialog->closed_gop_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_FRAME_DROP_RATIO,
      dialog->frame_drop_ratio_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_GMC,
      dialog->gmc_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_GRAYSCALE,
      dialog->grayscale_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_INTERLACING,
      dialog->interlacing_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_LUMI_MASK,
      dialog->lumi_mask_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_MAX_BFRAMES,
      dialog->max_bframes_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_MAX_BQUANT,
      dialog->max_bquant_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_MAX_IQUANT,
      dialog->max_iquant_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_MAX_KEYINT,
      dialog->max_keyint_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_MAX_PQUANT,
      dialog->max_pquant_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_ME_QUALITY,
      dialog->me_quality_combo, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_MIN_BQUANT,
      dialog->min_bquant_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_MIN_IQUANT,
      dialog->min_iquant_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_MIN_PQUANT,
      dialog->min_pquant_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_PACKED,
      dialog->packed_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_PAR,
      dialog->par_combo, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_PAR_HEIGHT,
      dialog->par_height_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_PAR_WIDTH,
      dialog->par_width_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_PROFILE,
      dialog->profile_combo, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_QPEL,
      dialog->qpel_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_QUANT_TYPE,
      dialog->quant_type_combo, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_TRELLIS,
      dialog->trellis_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_XVID_PROP_VHQ,
      dialog->vhq_combo, "active", G_SETTINGS_BIND_DEFAULT);

  g_object_unref (settings);
}

static void
ogmrip_xvid_dialog_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
  {
    case PROP_PROFILE:
      g_value_set_object (value, OGMRIP_XVID_DIALOG (gobject)->profile);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_xvid_dialog_set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
  {
    case PROP_PROFILE:
      ogmrip_xvid_dialog_set_profile (OGMRIP_XVID_DIALOG (gobject), g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_xvid_dialog_class_init (OGMRipXvidDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_xvid_dialog_get_property;
  gobject_class->set_property = ogmrip_xvid_dialog_set_property;

  g_object_class_override_property (gobject_class, PROP_PROFILE, "profile");
}

static void
ogmrip_xvid_dialog_class_finalize (OGMRipXvidDialogClass *klass)
{
}

static void
ogmrip_xvid_dialog_init (OGMRipXvidDialog *dialog)
{
  GError *error = NULL;

  GtkBuilder *builder;
  GtkWidget *misc, *widget;

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
      NULL);
  gtk_window_set_title (GTK_WINDOW (dialog), _("XviD Options"));
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_GLADE_FILE, &error))
    g_error ("Couldn't load builder file: %s", error->message);

  misc = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  widget = gtk_builder_get_widget (builder, OGMRIP_GLADE_ROOT);
  gtk_container_add (GTK_CONTAINER (misc), widget);
  gtk_widget_show (widget);

  dialog->bquant_offset_spin = gtk_builder_get_widget (builder, "bquant_offset-spin");
  dialog->bquant_ratio_spin = gtk_builder_get_widget (builder, "bquant_ratio-spin");
  dialog->bvhq_check = gtk_builder_get_widget (builder, "bvhq-check");
  dialog->cartoon_check = gtk_builder_get_widget (builder, "cartoon-check");
  dialog->chroma_me_check = gtk_builder_get_widget (builder, "chroma_me-check");
  dialog->chroma_opt_check = gtk_builder_get_widget (builder, "chroma_opt-check");
  dialog->closed_gop_check = gtk_builder_get_widget (builder, "closed_gop-check");
  dialog->frame_drop_ratio_spin = gtk_builder_get_widget (builder, "frame_drop_ratio-spin");
  dialog->gmc_check = gtk_builder_get_widget (builder, "gmc-check");
  dialog->grayscale_check = gtk_builder_get_widget (builder, "grayscale-check");
  dialog->interlacing_check = gtk_builder_get_widget (builder, "interlacing-check");
  dialog->lumi_mask_check = gtk_builder_get_widget (builder, "lumi_mask-check");
  dialog->max_bframes_spin = gtk_builder_get_widget (builder, "max_bframes-spin");
  dialog->max_bquant_spin = gtk_builder_get_widget (builder, "max_bquant-spin");
  dialog->max_iquant_spin = gtk_builder_get_widget (builder, "max_iquant-spin");
  dialog->max_keyint_spin = gtk_builder_get_widget (builder, "max_keyint-spin");
  dialog->max_pquant_spin = gtk_builder_get_widget (builder, "max_pquant-spin");
  dialog->me_quality_combo = gtk_builder_get_widget (builder, "me_quality-combo");
  dialog->min_bquant_spin = gtk_builder_get_widget (builder, "min_bquant-spin");
  dialog->min_iquant_spin = gtk_builder_get_widget (builder, "min_iquant-spin");
  dialog->min_pquant_spin = gtk_builder_get_widget (builder, "min_pquant-spin");
  dialog->packed_check = gtk_builder_get_widget (builder, "packed-check");
  dialog->par_combo = gtk_builder_get_widget (builder, "par-combo");
  dialog->par_height_spin = gtk_builder_get_widget (builder, "par_height-spin");
  dialog->par_width_spin = gtk_builder_get_widget (builder, "par_width-spin");
  dialog->profile_combo = gtk_builder_get_widget (builder, "profile-combo");
  dialog->qpel_check = gtk_builder_get_widget (builder, "qpel-check");
  dialog->quant_type_combo = gtk_builder_get_widget (builder, "quant_type-combo");
  dialog->trellis_check = gtk_builder_get_widget (builder, "trellis-check");
  dialog->vhq_combo = gtk_builder_get_widget (builder, "vhq-combo");

  g_object_bind_property_full (dialog->max_bframes_spin, "value",
      dialog->frame_drop_ratio_spin, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_xvid_dialog_set_frame_drop_ratio_sensitivity, NULL, NULL, NULL);

  g_object_bind_property (dialog->par_width_spin, "sensitive",
      dialog->par_height_spin, "sensitive", G_BINDING_SYNC_CREATE);

  g_object_bind_property_full (dialog->par_combo, "active",
      dialog->par_width_spin, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_xvid_dialog_set_par_sensitivity, NULL, NULL, NULL);

  g_object_unref (builder);
}

static void
ogmrip_options_editable_init (OGMRipOptionsEditableInterface *iface)
{
}

void
ogmrip_module_load (OGMRipModule *module)
{
  GType gtype;

  gtype = ogmrip_type_from_name ("xvid");
  if (gtype == G_TYPE_NONE)
    return;

  ogmrip_xvid_dialog_register_type (G_TYPE_MODULE (module));
  ogmrip_type_add_dynamic_extension (module,
      gtype, OGMRIP_TYPE_XVID_DIALOG);
}

