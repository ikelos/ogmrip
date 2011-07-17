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

#include "ogmrip-options-dialog.h"
#include "ogmrip-profiles-dialog.h"
#include "ogmrip-crop-dialog.h"

#include "ogmrip-settings.h"

#include <glib/gi18n.h>

#define OGMRIP_GLADE_FILE "ogmrip" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-options.glade"
#define OGMRIP_GLADE_ROOT "root"

#define ROUND(x) ((gint) ((x) + 0.5) != (gint) (x) ? ((gint) ((x) + 0.5)) : ((gint) (x)))

#define OGMRIP_OPTIONS_DIALOG_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_OPTIONS_DIALOG, OGMRipOptionsDialogPriv))

enum
{
  PROP_0,
  PROP_ACTION,
  PROP_ENCODING
};

enum
{
  OGMRIP_SCALE_NONE,
  OGMRIP_SCALE_XSMALL,
  OGMRIP_SCALE_SMALL,
  OGMRIP_SCALE_MEDIUM,
  OGMRIP_SCALE_LARGE,
  OGMRIP_SCALE_XLARGE,
  OGMRIP_SCALE_FULL,
  OGMRIP_SCALE_USER,
  OGMRIP_SCALE_AUTOMATIC
};

struct _OGMRipOptionsDialogPriv
{
  OGMRipOptionsDialogAction action;

  GtkWidget *crop_check;
  GtkWidget *crop_box;

  GtkWidget *crop_left_label;
  GtkWidget *crop_top_label;
  GtkWidget *crop_right_label;
  GtkWidget *crop_bottom_label;

  GtkWidget *scale_check;
  GtkWidget *scale_box;

  GtkWidget *scale_width_spin;
  GtkWidget *scale_height_spin;

  GtkWidget *test_check;
  GtkWidget *test_box;

  GtkWidget *deint_check;

  OGMRipEncoding *encoding;
};

static void ogmrip_options_dialog_constructed  (GObject      *gobject);
static void ogmrip_options_dialog_get_property (GObject      *gobject,
                                                guint        property_id,
                                                GValue       *value,
                                                GParamSpec   *pspec);
static void ogmrip_options_dialog_set_property (GObject      *gobject,
                                                guint        property_id,
                                                const GValue *value,
                                                GParamSpec   *pspec);
static void ogmrip_options_dialog_dispose      (GObject      *gobject);

static void
ogmrip_options_dialog_get_crop_info (OGMRipOptionsDialog *dialog, guint *x, guint *y, guint *w, guint *h, gdouble *r)
{
  gint left, top, right, bottom;

  left   = gtk_label_get_int (GTK_LABEL (dialog->priv->crop_left_label));
  top    = gtk_label_get_int (GTK_LABEL (dialog->priv->crop_top_label));
  right  = gtk_label_get_int (GTK_LABEL (dialog->priv->crop_right_label));
  bottom = gtk_label_get_int (GTK_LABEL (dialog->priv->crop_bottom_label));

  *x = left;
  *y = top;

  /*
   * TODO encoding is always defined
   */

  *w = 0;
  *h = 0;
  *r = 1.0;

  if (dialog->priv->encoding)
  {
    OGMDvdTitle *title;
    guint n, d;

    title = ogmrip_encoding_get_title (dialog->priv->encoding);
    ogmdvd_title_get_aspect_ratio (title, &n, &d);
    ogmdvd_title_get_size (title, w, h);

    *r = *h / (gdouble) *w * n / (gdouble) d;
  }

  *w -= left + right;
  *h -= top + bottom;

  *r *= *w / (gdouble) *h;
}

static void
ogmrip_options_dialog_profile_combo_changed (OGMRipOptionsDialog *dialog, GtkComboBox *chooser)
{
  OGMRipProfile *profile;

  GType video_codec = G_TYPE_NONE;
  OGMRipEncodingMethod method = 0;
  OGMRipScalerType scaler = 0;
  gboolean sensitive, can_crop = FALSE;

  profile = ogmrip_profile_chooser_get_active (chooser);
  if (profile)
  {
    GSettings *settings;
    gchar *codec;

    settings = ogmrip_profile_get_child (profile, OGMRIP_PROFILE_VIDEO);

    codec = g_settings_get_string (settings, OGMRIP_PROFILE_CODEC);
    video_codec = ogmrip_plugin_get_video_codec_by_name (codec);
    g_free (codec);

    g_settings_get (settings, OGMRIP_PROFILE_SCALER, "u", &scaler);
    can_crop = g_settings_get_boolean (settings, OGMRIP_PROFILE_CAN_CROP);

    settings = ogmrip_profile_get_child (profile, OGMRIP_PROFILE_GENERAL);
    g_settings_get (settings, OGMRIP_PROFILE_ENCODING_METHOD, "u", &method);
  }

  sensitive = gtk_widget_get_sensitive (dialog->priv->crop_check);
  gtk_widget_set_sensitive (dialog->priv->crop_check,
      video_codec != G_TYPE_NONE && can_crop);

  if (video_codec == G_TYPE_NONE || !can_crop)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->crop_check), FALSE);
  else if (!sensitive)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->crop_check),
        !gtk_widget_get_visible (dialog->priv->crop_box));

  sensitive = gtk_widget_get_sensitive (dialog->priv->scale_check);
  gtk_widget_set_sensitive (dialog->priv->scale_check,
      video_codec != G_TYPE_NONE && scaler != OGMRIP_SCALER_NONE);

  if (video_codec == G_TYPE_NONE || scaler == OGMRIP_SCALER_NONE)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->scale_check), FALSE);
  else if (!sensitive)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->scale_check),
        !gtk_widget_get_visible (dialog->priv->scale_box));

  sensitive = gtk_widget_get_sensitive (dialog->priv->test_check);
  gtk_widget_set_sensitive (dialog->priv->test_check,
      video_codec != G_TYPE_NONE && scaler != OGMRIP_SCALER_NONE && method != OGMRIP_ENCODING_QUANTIZER);

  if (video_codec == G_TYPE_NONE || scaler == OGMRIP_SCALER_NONE || method == OGMRIP_ENCODING_QUANTIZER)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->test_check), FALSE);
  else if (!sensitive)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->test_check),
        !gtk_widget_get_visible (dialog->priv->test_box));

  gtk_widget_set_sensitive (dialog->priv->deint_check, video_codec != G_TYPE_NONE);
}

static void
ogmrip_options_dialog_check_toggled (GtkToggleButton *check, GtkWidget *box)
{
  if (gtk_widget_get_sensitive (GTK_WIDGET (check)))
    gtk_widget_set_visible (box, !gtk_toggle_button_get_active (check));
}

static void
ogmrip_options_dialog_crop_changed (OGMRipOptionsDialog *dialog)
{
  guint x, y, w, h, i;
//  gboolean active;
  gdouble r;

  ogmrip_options_dialog_get_crop_info (dialog, &x, &y, &w, &h, &r);

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (dialog->priv->scale_width_spin), 0, w);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (dialog->priv->scale_height_spin), 0, h);

//  active = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->priv->scale_combo));

  for (i = 1; i < OGMRIP_SCALE_USER; i++)
  {
/*
    if (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (dialog->priv->scale_store), &iter, NULL, i))
    {
      scale_width = crop_width;
      scale_height = crop_height;

      ogmrip_options_dialog_scale (i, aspect, &scale_width, &scale_height);

      snprintf (text, 16, "%u x %u", scale_width, scale_height);
      gtk_list_store_set (dialog->priv->scale_store, &iter, 1, text, -1);

      if (active == i)
        ogmrip_options_dialog_set_scale_internal (dialog, -1, scale_width, scale_height);
    }
*/
  }
}

static void
ogmrip_options_dialog_edit_profiles_button_clicked (OGMRipOptionsDialog *dialog)
{
}

static void
ogmrip_options_dialog_crop_button_clicked (OGMRipOptionsDialog *options)
{
}

static void
ogmrip_options_dialog_autocrop_button_clicked (OGMRipOptionsDialog *options)
{
}

static void
ogmrip_options_dialog_autoscale_button_clicked (OGMRipOptionsDialog *dialog)
{
}

static void
ogmrip_options_dialog_test_button_clicked (OGMRipOptionsDialog *dialog)
{
}

G_DEFINE_TYPE (OGMRipOptionsDialog, ogmrip_options_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_options_dialog_class_init (OGMRipOptionsDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = ogmrip_options_dialog_constructed;
  gobject_class->get_property = ogmrip_options_dialog_get_property;
  gobject_class->set_property = ogmrip_options_dialog_set_property;
  gobject_class->dispose = ogmrip_options_dialog_dispose;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ACTION,
      g_param_spec_uint ("action", "action", "action",
        OGMRIP_OPTIONS_DIALOG_CREATE, OGMRIP_OPTIONS_DIALOG_EDIT, OGMRIP_OPTIONS_DIALOG_CREATE,
        G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ENCODING,
      g_param_spec_object ("encoding", "encoding", "encoding",
        OGMRIP_TYPE_ENCODING, G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (OGMRipOptionsDialogPriv));
}

static void
ogmrip_options_dialog_init (OGMRipOptionsDialog *dialog)
{
  dialog->priv = OGMRIP_OPTIONS_DIALOG_GET_PRIVATE (dialog);

  dialog->priv->action = OGMRIP_OPTIONS_DIALOG_CREATE;
}

static void
ogmrip_options_dialog_constructed (GObject *gobject)
{
  GError *error = NULL;

  OGMRipOptionsDialog *dialog = OGMRIP_OPTIONS_DIALOG (gobject);

  GtkBuilder *builder;
  GtkWidget *misc, *widget, *combo;
  guint i;

  GtkTreeIter iter;
  GtkListStore *store;
  GtkCellRenderer *renderer;

  gchar *size[] = { N_("None"), N_("Extra Small"), N_("Small"), N_("Medium"), 
    N_("Large"), N_("Extra Large"), N_("Full"), N_("User Defined") };
/*
  if (!dialog->priv->encoding)
  {
    g_critical ("No encoding has been specified");
    return;
  }
*/
  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_GLADE_FILE, &error))
  {
    g_critical ("Couldn't load builder file: %s", error->message);
    return;
  }

  gtk_window_set_title (GTK_WINDOW (dialog), _("Options"));
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), OGMRIP_RESPONSE_EXTRACT);
  gtk_window_set_icon_from_stock (GTK_WINDOW (dialog), GTK_STOCK_PROPERTIES);

  if (dialog->priv->action == OGMRIP_OPTIONS_DIALOG_EDIT)
    gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
  else
  {
    GtkWidget *image;

    gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT);

    widget = gtk_button_new_with_mnemonic (_("En_queue"));
    gtk_dialog_add_action_widget (GTK_DIALOG (dialog), widget, OGMRIP_RESPONSE_ENQUEUE);
    gtk_widget_set_can_default (widget, TRUE);
    gtk_widget_show (widget);

    image = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image (GTK_BUTTON (widget), image);

    widget = gtk_button_new_with_mnemonic (_("E_xtract"));
    gtk_dialog_add_action_widget (GTK_DIALOG (dialog), widget, OGMRIP_RESPONSE_EXTRACT);
    gtk_widget_set_can_default (widget, TRUE);
    gtk_widget_show (widget);

    image = gtk_image_new_from_stock (GTK_STOCK_CONVERT, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image (GTK_BUTTON (widget), image);
  }

  misc = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  widget = gtk_builder_get_widget (builder, OGMRIP_GLADE_ROOT);
  gtk_container_add (GTK_CONTAINER (misc), widget);
  gtk_widget_show (widget);

  combo = gtk_builder_get_widget (builder, "profile-combo");
  ogmrip_profile_chooser_construct (GTK_COMBO_BOX (combo));
  g_signal_connect_swapped (combo, "changed",
      G_CALLBACK (ogmrip_options_dialog_profile_combo_changed), dialog);
/*
  if (dialog->priv->action == OGMRIP_OPTIONS_DIALOG_CREATE)
    ogmrip_settings_bind_custom (settings, OGMRIP_GCONF_GENERAL, OGMRIP_GCONF_PROFILE,
        G_OBJECT (OGMRIP_OPTIONS_DIALOG (dialog)->priv->profile_combo), "active",
        ogmrip_options_dialog_profile_get_value,
        ogmrip_options_dialog_profile_set_value,
        NULL);
*/
  widget = gtk_builder_get_widget (builder, "edit-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_options_dialog_edit_profiles_button_clicked), dialog);

  dialog->priv->crop_check = gtk_builder_get_widget (builder, "crop-check");
  dialog->priv->crop_box = gtk_builder_get_widget (builder,"crop-box");

  g_object_bind_property (dialog->priv->crop_check, "sensitive",
      dialog->priv->crop_box, "sensitive", G_BINDING_SYNC_CREATE); 
  g_signal_connect (dialog->priv->crop_check, "toggled",
      G_CALLBACK (ogmrip_options_dialog_check_toggled), dialog->priv->crop_box);

  widget = gtk_builder_get_widget (builder, "crop-left-label");
  widget = gtk_builder_get_widget (builder, "crop-right-label");
  widget = gtk_builder_get_widget (builder, "crop-top-label");
  widget = gtk_builder_get_widget (builder, "crop-bottom-label");

  widget = gtk_builder_get_widget (builder, "crop-button");
  g_signal_connect_swapped (widget, "clicked", 
      G_CALLBACK (ogmrip_options_dialog_crop_button_clicked), dialog);

  widget = gtk_builder_get_widget (builder, "autocrop-button");
  g_signal_connect_swapped (widget, "clicked", 
      G_CALLBACK (ogmrip_options_dialog_autocrop_button_clicked), dialog);

  dialog->priv->scale_check = gtk_builder_get_widget (builder, "scale-check");
  dialog->priv->scale_box = gtk_builder_get_widget (builder,"scale-box");

  g_object_bind_property (dialog->priv->scale_check, "sensitive",
      dialog->priv->scale_box, "sensitive", G_BINDING_SYNC_CREATE); 
  g_signal_connect (dialog->priv->scale_check, "toggled",
      G_CALLBACK (ogmrip_options_dialog_check_toggled), dialog->priv->scale_box);

  widget = gtk_builder_get_widget (builder, "autoscale-button");
  g_signal_connect_swapped (widget, "clicked", 
      G_CALLBACK (ogmrip_options_dialog_autoscale_button_clicked), dialog);

  widget = gtk_builder_get_widget (builder, "scale-user-hbox");
  dialog->priv->scale_width_spin = gtk_builder_get_widget (builder, "scale-width-spin");
  dialog->priv->scale_height_spin = gtk_builder_get_widget (builder, "scale-height-spin");

  store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

  widget = gtk_builder_get_widget (builder, "scale-combo");

  for (i = 0; i < G_N_ELEMENTS (size); i++)
  {
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, _(size[i]), -1);
  }

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), renderer, "text", 0, NULL);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "xalign", 1.0, "xpad", 4, NULL);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), renderer, "text", 1, NULL);

  gtk_combo_box_set_model (GTK_COMBO_BOX (widget), GTK_TREE_MODEL (store));

  dialog->priv->test_check = gtk_builder_get_widget (builder, "test-check");
  dialog->priv->test_box = gtk_builder_get_widget (builder,"test-box");

  g_object_bind_property (dialog->priv->test_check, "sensitive",
      dialog->priv->test_box, "sensitive", G_BINDING_SYNC_CREATE); 
  g_signal_connect (dialog->priv->test_check, "toggled",
      G_CALLBACK (ogmrip_options_dialog_check_toggled), dialog->priv->test_box);

  /*
   * TODO bind test to encoding
   */

  widget = gtk_builder_get_widget (builder, "test-button");
  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_options_dialog_test_button_clicked), dialog);

  dialog->priv->deint_check = gtk_builder_get_widget (builder, "deint-check");
  /*
   * TODO bind deinterlacer to encoding
   */

  g_object_unref (builder);

  ogmrip_options_dialog_profile_combo_changed (dialog, GTK_COMBO_BOX (combo));

  ogmrip_options_dialog_crop_changed (dialog);
}

static void
ogmrip_options_dialog_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipOptionsDialog *dialog = OGMRIP_OPTIONS_DIALOG (gobject);

  switch (property_id)
  {
    case PROP_ACTION:
      g_value_set_uint (value, dialog->priv->action);
      break;
    case PROP_ENCODING:
      g_value_set_object (value, dialog->priv->encoding);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_options_dialog_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipOptionsDialog *dialog = OGMRIP_OPTIONS_DIALOG (gobject);

  switch (property_id)
  {
    case PROP_ACTION:
      dialog->priv->action = g_value_get_uint (value);
      break;
    case PROP_ENCODING:
      dialog->priv->encoding = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_options_dialog_dispose (GObject *gobject)
{
  OGMRipOptionsDialog *dialog = OGMRIP_OPTIONS_DIALOG (gobject);

  if (dialog->priv->encoding)
  {
    g_object_unref (dialog->priv->encoding);
    dialog->priv->encoding = NULL;
  }

  G_OBJECT_CLASS (ogmrip_options_dialog_parent_class)->dispose (gobject);
}

GtkWidget *
ogmrip_options_dialog_new (OGMRipEncoding *encoding, OGMRipOptionsDialogAction action)
{
  return g_object_new (OGMRIP_TYPE_OPTIONS_DIALOG, "encoding", encoding, "action", action, NULL);
}

