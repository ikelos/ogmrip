/* OGMRipLavcOptions - An LAVC options plugin for OGMRip
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

#include <ogmrip-mplayer.h>
#include <ogmrip-encode-gtk.h>
#include <ogmrip-module.h>

#include <glib/gi18n.h>

#define OGMRIP_UI_RES  "/org/ogmrip/ogmrip-lavc-options-dialog.ui"
#define OGMRIP_UI_ROOT "root"

#define OGMRIP_TYPE_LAVC_DIALOG          (ogmrip_lavc_dialog_get_type ())
#define OGMRIP_LAVC_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_LAVC_DIALOG, OGMRipLavcDialog))
#define OGMRIP_LAVC_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_LAVC_DIALOG, OGMRipLavcDialogClass))
#define OGMRIP_IS_LAVC_DIALOG(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_LAVC_DIALOG))
#define OGMRIP_IS_LAVC_DIALOG_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_LAVC_DIALOG))

typedef struct _OGMRipLavcDialog      OGMRipLavcDialog;
typedef struct _OGMRipLavcDialogClass OGMRipLavcDialogClass;

struct _OGMRipLavcDialog
{
  GtkDialog parent_instance;

  OGMRipProfile *profile;

  GtkWidget *buf_size_spin;
  GtkWidget *cmp_spin;
  GtkWidget *dc_spin;
  GtkWidget *dia_spin;
  GtkWidget *grayscale_check;
  GtkWidget *keyint_spin;
  GtkWidget *last_pred_spin;
  GtkWidget *max_bframes_spin;
  GtkWidget *max_rate_spin;
  GtkWidget *mbd_combo;
  GtkWidget *min_rate_spin;
  GtkWidget *mv0_check;
  GtkWidget *precmp_spin;
  GtkWidget *predia_spin;
  GtkWidget *preme_combo;
  GtkWidget *qns_combo;
  GtkWidget *qpel_check;
  GtkWidget *strict_combo;
  GtkWidget *subcmp_spin;
  GtkWidget *trellis_check;
  GtkWidget *v4mv_check;
  GtkWidget *vb_strategy_combo;
  GtkWidget *vqcomp_spin;
};

struct _OGMRipLavcDialogClass
{
  GtkDialogClass parent_class;
};

enum
{
  PROP_0,
  PROP_PROFILE
};

static void ogmrip_options_editable_init (OGMRipOptionsEditableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (OGMRipLavcDialog, ogmrip_lavc_dialog, GTK_TYPE_DIALOG,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_OPTIONS_EDITABLE, ogmrip_options_editable_init));

static void
ogmrip_lavc_dialog_set_profile (OGMRipLavcDialog *dialog, OGMRipProfile *profile)
{
  GSettings *settings;
  gchar *str;

  if (dialog->profile)
    g_object_unref (dialog->profile);
  dialog->profile = g_object_ref (profile);

  ogmrip_profile_get (profile, OGMRIP_PROFILE_VIDEO, OGMRIP_PROFILE_CODEC, "s", &str, NULL);
  settings = ogmrip_profile_get_child (profile, str);
  g_free (str);

  g_assert (settings != NULL);

  g_settings_bind (settings, OGMRIP_LAVC_PROP_BUF_SIZE,
      dialog->buf_size_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_CMP,
      dialog->cmp_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_DC,
      dialog->dc_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_DIA,
      dialog->dia_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_GRAYSCALE,
      dialog->grayscale_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_KEYINT,
      dialog->keyint_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_LAST_PRED,
      dialog->last_pred_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_MAX_BFRAMES,
      dialog->max_bframes_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_MAX_RATE,
      dialog->max_rate_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_MBD,
      dialog->mbd_combo, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_MIN_RATE,
      dialog->min_rate_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_MV0,
      dialog->mv0_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_PRECMP,
      dialog->precmp_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_PREDIA,
      dialog->predia_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_PREME,
      dialog->preme_combo, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_QNS,
      dialog->qns_combo, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_QPEL,
      dialog->qpel_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_STRICT,
      dialog->strict_combo, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_SUBCMP,
      dialog->subcmp_spin, "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_TRELLIS,
      dialog->trellis_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_V4MV,
      dialog->v4mv_check, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_VB_STRATEGY,
      dialog->vb_strategy_combo, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, OGMRIP_LAVC_PROP_VQCOMP,
      dialog->vqcomp_spin, "value", G_SETTINGS_BIND_DEFAULT);

  g_object_unref (settings);
}

static void
ogmrip_lavc_dialog_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
  {
    case PROP_PROFILE:
      g_value_set_object (value, OGMRIP_LAVC_DIALOG (gobject)->profile);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_lavc_dialog_set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
  {
    case PROP_PROFILE:
      ogmrip_lavc_dialog_set_profile (OGMRIP_LAVC_DIALOG (gobject), g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_lavc_dialog_class_init (OGMRipLavcDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_lavc_dialog_get_property;
  gobject_class->set_property = ogmrip_lavc_dialog_set_property;

  g_object_class_override_property (gobject_class, PROP_PROFILE, "profile");

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), OGMRIP_UI_RES);

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, buf_size_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, cmp_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, dc_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, dia_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, grayscale_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, keyint_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, last_pred_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, max_bframes_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, max_rate_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, mbd_combo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, min_rate_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, mv0_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, precmp_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, predia_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, preme_combo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, qns_combo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, qpel_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, strict_combo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, subcmp_spin);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, trellis_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, v4mv_check);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, vb_strategy_combo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), OGMRipLavcDialog, vqcomp_spin);
}

static void
ogmrip_lavc_dialog_init (OGMRipLavcDialog *dialog)
{
}

static void
ogmrip_options_editable_init (OGMRipOptionsEditableInterface *iface)
{
}

void
ogmrip_module_load (OGMRipModule *module)
{
  ogmrip_type_add_extension (OGMRIP_TYPE_LAVC, OGMRIP_TYPE_LAVC_DIALOG);
}

