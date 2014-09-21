/* OGMRip - A media encoder for GNOME
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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
#include "ogmrip-crop-dialog.h"

#include "ogmrip-helper.h"
#include "ogmrip-settings.h"

#include <glib/gi18n.h>

#include <stdlib.h>

#define OGMRIP_UI_RES  "/org/ogmrip/ogmrip-options-dialog.ui"
#define OGMRIP_UI_ROOT "root"

#define ROUND(x) ((gint) ((x) + 0.5) != (gint) (x) ? ((gint) ((x) + 0.5)) : ((gint) (x)))

enum
{
  PROP_0,
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
  GtkWidget *profile_chooser;

  GtkWidget *crop_check;
  GtkWidget *crop_button;
  GtkWidget *crop_box;

  GtkWidget *crop_left_label;
  GtkWidget *crop_top_label;
  GtkWidget *crop_right_label;
  GtkWidget *crop_bottom_label;

  GtkWidget *scale_combo;
  GtkWidget *scale_check;
  GtkWidget *scale_box;
  GtkWidget *scale_user_box;

  GtkWidget *scale_width_spin;
  GtkWidget *scale_height_spin;

  GtkWidget *test_check;
  GtkWidget *test_button;
  GtkWidget *test_box;

  GtkWidget *edit_button;

  GtkWidget *deint_check;

  GtkWidget *autoscale_button;
  GtkWidget *autocrop_button;

  OGMRipEncoding *encoding;
  OGMRipTitle *title;
};

extern GSettings *settings;

static const guint m[] = { 0, 1, 1, 1, 3, 5, 1 };
static const guint d[] = { 0, 8, 4, 2, 4, 6, 1 };

static void
gtk_label_set_int (GtkLabel *label, gint value)
{
  gchar *text;

  text = g_strdup_printf ("%d", value);
  gtk_label_set_text (label, text);
  g_free (text);
}

static gint
gtk_label_get_int (GtkLabel *label)
{
  return atoi (gtk_label_get_text (label));
}

OGMRipProfile *
ogmrip_profile_chooser_get_active (GtkComboBox *chooser)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (!gtk_combo_box_get_active_iter (chooser, &iter))
    return NULL;

  model = gtk_combo_box_get_model (chooser);

  return ogmrip_profile_store_get_profile (OGMRIP_PROFILE_STORE (model), &iter);
}

void
ogmrip_profile_chooser_set_active (GtkComboBox *chooser, OGMRipProfile *profile)
{
  if (!profile)
    gtk_combo_box_set_active (chooser, -1);
  else
  {
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_combo_box_get_model (chooser);
    if (ogmrip_profile_store_get_iter (OGMRIP_PROFILE_STORE (model), &iter, profile))
      gtk_combo_box_set_active_iter (chooser, &iter);
    else
      gtk_combo_box_set_active (chooser, 0);
  }
}

static void
scale_to_size (guint cw, guint ch, gdouble r, guint s, guint *sw, guint *sh)
{
  *sw = cw * m[s] / d[s];
  *sw += *sw % 2;

  *sw = 16 * ROUND (*sw / 16.);
  *sh = 16 * ROUND (*sw / r / 16.);
}

static void
crop_to_size (guint rw, guint rh, guint l, guint t, guint r, guint b, guint *cw, guint *ch)
{
  *cw = rw;
  if (*cw > l + r)
    *cw -= l + r;

  *ch = rh;
  if (*ch > t + b)
    *ch -= t + b;
}

static gdouble
get_ratio (guint rw, guint rh, guint cw, guint ch, guint n, guint d)
{
  if (!rw || !rh)
    return 1.0;

  return cw / (gdouble) ch * rh / (gdouble) rw * n / (gdouble) d;
}

static void
ogmrip_options_dialog_get_crop (OGMRipOptionsDialog *dialog, guint *l, guint *t, guint *r, guint *b)
{
  *l = gtk_label_get_int (GTK_LABEL (dialog->priv->crop_left_label));
  *t = gtk_label_get_int (GTK_LABEL (dialog->priv->crop_top_label));
  *r = gtk_label_get_int (GTK_LABEL (dialog->priv->crop_right_label));
  *b = gtk_label_get_int (GTK_LABEL (dialog->priv->crop_bottom_label));
}

static gdouble
ogmrip_options_dialog_get_crop_full (OGMRipOptionsDialog *dialog, guint *x, guint *y, guint *w, guint *h)
{
  guint r, b, n, d, rw, rh;

  ogmrip_options_dialog_get_crop (dialog, x, y, &r, &b);

  ogmrip_video_stream_get_resolution (ogmrip_title_get_video_stream (dialog->priv->title), &rw, &rh);
  crop_to_size (rw, rh, *x, *y, r, b, w, h);

  ogmrip_video_stream_get_aspect_ratio (ogmrip_title_get_video_stream (dialog->priv->title), &n, &d);

  return get_ratio (rw, rh, *w, *h, n, d);
}

static void
ogmrip_options_dialog_set_crop (OGMRipOptionsDialog *dialog, guint l, guint t, guint r, guint b)
{
  gtk_label_set_int (GTK_LABEL (dialog->priv->crop_left_label), l);
  gtk_label_set_int (GTK_LABEL (dialog->priv->crop_top_label), t);
  gtk_label_set_int (GTK_LABEL (dialog->priv->crop_right_label), r);
  gtk_label_set_int (GTK_LABEL (dialog->priv->crop_bottom_label), b);
}

static void
ogmrip_options_dialog_set_scale (OGMRipOptionsDialog *dialog, guint w, guint h)
{
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->scale_width_spin), w);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->scale_height_spin), h);
}

static void
ogmrip_options_dialog_update_scale_combo (OGMRipOptionsDialog *dialog)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  gchar *str;
  guint x, y, rw, rh, cw, ch, sw, sh, i;
  gdouble f;

  f = ogmrip_options_dialog_get_crop_full (dialog, &x, &y, &cw, &ch);

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (dialog->priv->scale_width_spin), 0, cw);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (dialog->priv->scale_height_spin), 0, ch);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (dialog->priv->scale_combo));

  for (i = OGMRIP_SCALE_NONE; i < OGMRIP_SCALE_USER; i++)
  {
    if (gtk_tree_model_iter_nth_child (model, &iter, NULL, i))
    {
      if (i == OGMRIP_SCALE_NONE)
      {
        ogmrip_video_stream_get_resolution (ogmrip_title_get_video_stream (dialog->priv->title), &rw, &rh);

        str = g_strdup_printf ("%u x %u", rw, rh);
      }
      else
      {
        scale_to_size (cw, ch, f, i, &sh, &sw);
        str = g_strdup_printf ("%u x %u", sw, sh);
      }

      gtk_list_store_set (GTK_LIST_STORE (model), &iter, 1, str, -1);
      g_free (str);
    }
  }
}

static void
ogmrip_options_dialog_set_sensitivity (OGMRipOptionsDialog *dialog, OGMRipProfile *profile)
{
  OGMRipFormat format = OGMRIP_FORMAT_UNDEFINED;
  OGMRipEncodingMethod method = 0;
  OGMRipScalerType scaler = 0;
  gboolean sensitive, can_crop = FALSE;

  if (profile)
  {
    GSettings *settings;
    GType codec;
    gchar *str;

    settings = ogmrip_profile_get_child (profile, OGMRIP_PROFILE_VIDEO);

    str = g_settings_get_string (settings, OGMRIP_PROFILE_CODEC);
    codec = ogmrip_type_from_name (str);
    g_free (str);

    if (codec != G_TYPE_NONE)
      format = ogmrip_codec_format (codec);

    if (format == OGMRIP_FORMAT_COPY)
      format = OGMRIP_FORMAT_UNDEFINED;

    scaler = g_settings_get_uint (settings, OGMRIP_PROFILE_SCALER);
    can_crop = g_settings_get_boolean (settings, OGMRIP_PROFILE_CAN_CROP);

    g_object_unref (settings); 

    settings = ogmrip_profile_get_child (profile, OGMRIP_PROFILE_GENERAL);
    method = g_settings_get_uint (settings, OGMRIP_PROFILE_ENCODING_METHOD);
    g_object_unref (settings); 
  }

  sensitive = gtk_widget_is_sensitive (dialog->priv->crop_check);
  gtk_widget_set_sensitive (dialog->priv->crop_check,
      format != OGMRIP_FORMAT_UNDEFINED && can_crop);

  if (format == OGMRIP_FORMAT_UNDEFINED || !can_crop)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->crop_check), FALSE);
  else if (!sensitive)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->crop_check),
        !gtk_widget_get_visible (dialog->priv->crop_box));

  sensitive = gtk_widget_is_sensitive (dialog->priv->scale_check);
  gtk_widget_set_sensitive (dialog->priv->scale_check,
      format != OGMRIP_FORMAT_UNDEFINED && scaler != OGMRIP_SCALER_NONE);

  if (format == OGMRIP_FORMAT_UNDEFINED || scaler == OGMRIP_SCALER_NONE)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->scale_check), FALSE);
  else if (!sensitive)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->scale_check),
        !gtk_widget_get_visible (dialog->priv->scale_box));

  sensitive = gtk_widget_is_sensitive (dialog->priv->test_check);
  gtk_widget_set_sensitive (dialog->priv->test_check,
      format != OGMRIP_FORMAT_UNDEFINED && scaler != OGMRIP_SCALER_NONE && method != OGMRIP_ENCODING_QUANTIZER);

  if (format == OGMRIP_FORMAT_UNDEFINED || scaler == OGMRIP_SCALER_NONE || method == OGMRIP_ENCODING_QUANTIZER)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->test_check), FALSE);
  else if (!sensitive)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->test_check),
        !gtk_widget_get_visible (dialog->priv->test_box));

  gtk_widget_set_sensitive (dialog->priv->deint_check, format != OGMRIP_FORMAT_UNDEFINED);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), OGMRIP_RESPONSE_ENQUEUE, profile != NULL);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), OGMRIP_RESPONSE_EXTRACT, profile != NULL);
}

static GVariant *
ogmrip_options_dialog_set_profile_mapping (const GValue *value, const GVariantType *expected_type, gpointer user_data)
{
  OGMRipProfile *profile;
  gchar *name;

  profile = g_value_get_object (value);
  if (!profile)
    return g_variant_new_string (NULL);

  g_object_get (profile, "name", &name, NULL);

  return g_variant_new_take_string (name);
}

static gboolean
ogmrip_options_dialog_get_profile_mapping (GValue *value, GVariant *variant, gpointer user_data)
{
  OGMRipProfileEngine *engine;
  OGMRipProfile *profile;
  const gchar *name;

  engine = ogmrip_profile_engine_get_default ();

  name = g_variant_get_string (variant, NULL);
  profile = name ? ogmrip_profile_engine_get (engine, name) : NULL;

  g_value_set_object (value, profile);

  return TRUE;
}

static void
ogmrip_options_dialog_profile_chooser_changed (OGMRipOptionsDialog *dialog)
{
  ogmrip_options_dialog_set_sensitivity (dialog,
      ogmrip_profile_chooser_widget_get_active (OGMRIP_PROFILE_CHOOSER_WIDGET (dialog->priv->profile_chooser)));
}

static void
ogmrip_options_dialog_check_toggled (GtkToggleButton *check, GtkWidget *box)
{
  if (gtk_widget_is_sensitive (GTK_WIDGET (check)))
    gtk_widget_set_visible (box, !gtk_toggle_button_get_active (check));
}

static void
ogmrip_options_dialog_scale_combo_changed (OGMRipOptionsDialog *dialog)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (dialog->priv->scale_combo), &iter))
  {
    GtkTreeModel *model;
    guint w, h;
    gchar *str;

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (dialog->priv->scale_combo));
    gtk_tree_model_get (model, &iter, 1, &str, -1);

    if (str && sscanf (str, "%u x %u", &w, &h) == 2)
    {
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->scale_width_spin), w);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->scale_height_spin), h);
    }

    g_free (str);
  }
}

static void
ogmrip_options_dialog_edit_profiles_button_clicked (OGMRipOptionsDialog *parent)
{
  OGMRipProfile *profile;

  profile = ogmrip_profile_chooser_get_active (GTK_COMBO_BOX (parent->priv->profile_chooser));
  if (profile)
  {
    GtkWidget *dialog;

    dialog = ogmrip_profile_editor_dialog_new (profile);
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));
    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), FALSE);

    gtk_dialog_run (GTK_DIALOG (dialog));

    gtk_widget_destroy (dialog);
  }
}

static void
ogmrip_options_dialog_crop_button_clicked (OGMRipOptionsDialog *parent)
{
  GError *error = NULL;

  if (ogmrip_open_title (GTK_WINDOW (parent), parent->priv->title, &error))
  {
    GtkWidget *dialog;
    guint l, t, r, b;

    ogmrip_options_dialog_get_crop (parent, &l, &t, &r, &b);

    dialog = ogmrip_crop_dialog_new (parent->priv->title, l, t, r, b);
    ogmrip_crop_dialog_set_deinterlacer (OGMRIP_CROP_DIALOG (dialog),
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (parent->priv->deint_check)));

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      ogmrip_crop_dialog_get_crop (OGMRIP_CROP_DIALOG (dialog), &l, &t, &r, &b);

      ogmrip_options_dialog_set_crop (parent, l, t, r, b);
      ogmrip_options_dialog_update_scale_combo (parent);

      gtk_widget_set_sensitive (parent->priv->autocrop_button, TRUE);
    }
    gtk_widget_destroy (dialog);

    ogmrip_title_close (parent->priv->title);
  }

  if (error)
    g_error_free (error);
}

static void
ogmrip_progress_dialog_progress_cb (OGMJobTask *spawn, GParamSpec *pspec, OGMRipProgressDialog *dialog)
{
  ogmrip_progress_dialog_set_fraction (dialog, ogmjob_task_get_progress (spawn));
}

static void
ogmrip_progress_dialog_response_cb (OGMRipProgressDialog *dialog, gint response_id, GCancellable *cancellable)
{
  g_cancellable_cancel (cancellable);
}

static void
ogmrip_options_dialog_autocrop_button_clicked (OGMRipOptionsDialog *parent)
{
  GError *error = NULL;

  if (ogmrip_open_title (GTK_WINDOW (parent), parent->priv->title, &error))
  {
    GCancellable *cancellable;
    GtkWidget *dialog;
    OGMJobTask *task;
    gint result;

    dialog = ogmrip_progress_dialog_new (GTK_WINDOW (parent), 
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, FALSE);
    ogmrip_progress_dialog_set_title (OGMRIP_PROGRESS_DIALOG (dialog),
        ogmrip_media_get_label (ogmrip_title_get_media (parent->priv->title)));
    gtk_window_present (GTK_WINDOW (dialog));

    ogmrip_progress_dialog_set_message (OGMRIP_PROGRESS_DIALOG (dialog), _("Analyzing video stream"));
    ogmrip_progress_dialog_set_fraction (OGMRIP_PROGRESS_DIALOG (dialog), -1.0);

    task = ogmrip_analyze_new (parent->priv->title);
    g_signal_connect (task, "notify::progress",
        G_CALLBACK (ogmrip_progress_dialog_progress_cb), dialog);

    cancellable = g_cancellable_new ();
    g_signal_connect (dialog, "response", G_CALLBACK (ogmrip_progress_dialog_response_cb), cancellable);

    result = ogmjob_task_run (task, cancellable, NULL);

    g_object_unref (task);
    g_object_unref (cancellable);

    if (result)
    {
      OGMRipVideoStream *stream;
      guint x, y, w, h, rw, rh;

      stream = ogmrip_title_get_video_stream (parent->priv->title);
      ogmrip_video_stream_get_crop_size (stream, &x, &y, &w, &h);

      ogmrip_video_stream_get_resolution (ogmrip_title_get_video_stream (parent->priv->title), &rw, &rh);

      ogmrip_options_dialog_set_crop (parent, x, y, rw > x + w ? rw - w - x : 0, rh > h + y ? rh - h - y : 0);
      ogmrip_options_dialog_update_scale_combo (parent);

      gtk_widget_set_sensitive (parent->priv->autocrop_button, FALSE);
    }

    gtk_widget_destroy (dialog);

    ogmrip_title_close (parent->priv->title);
  }

  if (error)
    g_error_free (error);
}

static void
ogmrip_options_dialog_autoscale_button_clicked (OGMRipOptionsDialog *dialog)
{
  OGMRipProfile *profile;
  OGMRipEncoding *encoding;
  OGMJobSpawn *spawn;

  GType codec;
  guint x, y, w, h;
  gchar *name;

  profile = ogmrip_profile_chooser_get_active (GTK_COMBO_BOX (dialog->priv->profile_chooser));

  ogmrip_profile_get (profile, OGMRIP_PROFILE_VIDEO, OGMRIP_PROFILE_CODEC, "s", &name);
  codec = g_type_from_name (name);
  g_free (name);

  encoding = ogmrip_encoding_new (dialog->priv->title);

  spawn = g_object_new (codec, "input", ogmrip_title_get_video_stream (dialog->priv->title), NULL);
  ogmrip_encoding_set_video_codec (encoding, OGMRIP_VIDEO_CODEC (spawn), NULL);
  g_object_unref (spawn);

  ogmrip_options_dialog_get_crop_full (dialog, &x, &y, &w, &h);
  ogmrip_video_codec_set_crop_size (OGMRIP_VIDEO_CODEC (spawn), x, y, w, h);

  ogmrip_encoding_autoscale (encoding, 0.25, &w, &h);
  g_object_unref (encoding);

  ogmrip_options_dialog_set_scale (dialog, w, h);

  gtk_widget_set_sensitive (dialog->priv->autoscale_button, FALSE);
}

static void
ogmrip_options_dialog_test_button_clicked (OGMRipOptionsDialog *dialog)
{
  gtk_dialog_response (GTK_DIALOG (dialog), OGMRIP_RESPONSE_TEST);
}

static gboolean
ogmrip_options_dialog_set_edit_button_sensitivity (GBinding *binding, const GValue *source_value, GValue *target_value, gpointer data)
{
  g_value_set_boolean (target_value, g_value_get_int (source_value) != -1);

  return TRUE;
}

static gboolean
ogmrip_options_dialog_set_scale_box_sensitivity (GBinding *binding, const GValue *source_value, GValue *target_value, gpointer data)
{
  g_value_set_boolean (target_value, g_value_get_int (source_value) == OGMRIP_SCALE_USER);

  return TRUE;
}

static void g_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (OGMRipOptionsDialog, ogmrip_options_dialog, GTK_TYPE_DIALOG,
    G_ADD_PRIVATE (OGMRipOptionsDialog)
    G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, g_initable_iface_init));

static void
ogmrip_options_dialog_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipOptionsDialog *dialog = OGMRIP_OPTIONS_DIALOG (gobject);

  switch (property_id)
  {
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
    case PROP_ENCODING:
      dialog->priv->encoding = g_value_dup_object (value);
      if (dialog->priv->encoding)
        dialog->priv->title = ogmrip_encoding_get_title (dialog->priv->encoding);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_options_dialog_constructed (GObject *gobject)
{
  OGMRipOptionsDialog *dialog = OGMRIP_OPTIONS_DIALOG (gobject);
  OGMRipProfile *profile;
  GtkWidget *button;

  profile = ogmrip_encoding_get_profile (dialog->priv->encoding);

  button = gtk_dialog_get_widget_for_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_widget_set_visible (button, profile != NULL);

  button = gtk_dialog_get_widget_for_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
  gtk_widget_set_visible (button, profile == NULL);

  button = gtk_dialog_get_widget_for_response (GTK_DIALOG (dialog), OGMRIP_RESPONSE_EXTRACT);
  gtk_widget_set_visible (button, profile == NULL);

  button = gtk_dialog_get_widget_for_response (GTK_DIALOG (dialog), OGMRIP_RESPONSE_ENQUEUE);
  gtk_widget_set_visible (button, profile == NULL);

  ogmrip_options_dialog_update_scale_combo (dialog);

  G_OBJECT_CLASS (ogmrip_options_dialog_parent_class)->constructed (gobject);
}

static void
ogmrip_options_dialog_dispose (GObject *gobject)
{
  OGMRipOptionsDialog *dialog = OGMRIP_OPTIONS_DIALOG (gobject);

  g_clear_object (&dialog->priv->encoding);

  G_OBJECT_CLASS (ogmrip_options_dialog_parent_class)->dispose (gobject);
}

static void
ogmrip_options_dialog_response (GtkDialog *dialog, gint response_id)
{
  OGMRipOptionsDialog *options = OGMRIP_OPTIONS_DIALOG (dialog);
  OGMRipProfile *profile;

  if (response_id != OGMRIP_RESPONSE_EXTRACT &&
      response_id != OGMRIP_RESPONSE_ENQUEUE &&
      response_id != OGMRIP_RESPONSE_TEST    &&
      response_id != GTK_RESPONSE_CLOSE)
    return;

  if (response_id == GTK_RESPONSE_CLOSE)
    profile = ogmrip_encoding_get_profile (options->priv->encoding);
  else
  {
    profile = ogmrip_profile_chooser_get_active (GTK_COMBO_BOX (options->priv->profile_chooser));
    ogmrip_encoding_set_profile (options->priv->encoding, profile);
  }

  if (!gtk_widget_is_sensitive (options->priv->crop_check))
    ogmrip_encoding_set_autocrop (options->priv->encoding, FALSE);
  else
  {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options->priv->crop_check)))
      ogmrip_encoding_set_autocrop (options->priv->encoding, TRUE);
    else
      ogmrip_encoding_set_autocrop (options->priv->encoding, FALSE);
  }

  if (!gtk_widget_is_sensitive (options->priv->scale_check))
    ogmrip_encoding_set_autoscale (options->priv->encoding, FALSE);
  else
  {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options->priv->scale_check)))
      ogmrip_encoding_set_autoscale (options->priv->encoding, TRUE);
    else
      ogmrip_encoding_set_autoscale (options->priv->encoding, FALSE);
  }

  ogmrip_encoding_set_test (options->priv->encoding,
      gtk_widget_is_sensitive (options->priv->test_check) &&
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options->priv->test_check)));
}

static void
ogmrip_options_dialog_class_init (OGMRipOptionsDialogClass *klass)
{
  GObjectClass *gobject_class;
  GtkDialogClass *dialog_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_options_dialog_get_property;
  gobject_class->set_property = ogmrip_options_dialog_set_property;
  gobject_class->constructed = ogmrip_options_dialog_constructed;
  gobject_class->dispose = ogmrip_options_dialog_dispose;

  dialog_class = GTK_DIALOG_CLASS (klass);
  dialog_class->response = ogmrip_options_dialog_response;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ENCODING,
      g_param_spec_object ("encoding", "encoding", "encoding",
        OGMRIP_TYPE_ENCODING, G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), OGMRIP_UI_RES);

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, profile_chooser);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, crop_check);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, crop_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, crop_box);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, crop_left_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, crop_right_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, crop_top_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, crop_bottom_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, autocrop_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, scale_check);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, scale_box);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, scale_user_box);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, scale_combo);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, scale_width_spin);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, scale_height_spin);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, autoscale_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, test_check);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, test_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, test_box);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, deint_check);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipOptionsDialog, edit_button);
}

static void
ogmrip_options_dialog_init (OGMRipOptionsDialog *dialog)
{
  g_type_ensure (OGMRIP_TYPE_PROFILE_CHOOSER_WIDGET);

  gtk_widget_init_template (GTK_WIDGET (dialog));

  dialog->priv = ogmrip_options_dialog_get_instance_private (dialog);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), OGMRIP_RESPONSE_EXTRACT);

  g_object_bind_property_full (dialog->priv->profile_chooser, "active",
      dialog->priv->edit_button, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_options_dialog_set_edit_button_sensitivity, NULL, NULL, NULL);

  g_object_bind_property_full (dialog->priv->scale_combo, "active",
      dialog->priv->scale_user_box, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_options_dialog_set_scale_box_sensitivity, NULL, NULL, NULL);

  g_object_bind_property (dialog->priv->crop_check, "sensitive",
                          dialog->priv->crop_box, "sensitive",
                          G_BINDING_SYNC_CREATE); 
  g_object_bind_property (dialog->priv->scale_check, "sensitive",
                          dialog->priv->scale_box, "sensitive",
                          G_BINDING_SYNC_CREATE); 
  g_object_bind_property (dialog->priv->test_check, "sensitive",
                          dialog->priv->test_box, "sensitive",
                          G_BINDING_SYNC_CREATE); 

  g_signal_connect_swapped (dialog->priv->profile_chooser, "changed",
      G_CALLBACK (ogmrip_options_dialog_profile_chooser_changed), dialog);

  g_signal_connect_swapped (dialog->priv->edit_button, "clicked",
      G_CALLBACK (ogmrip_options_dialog_edit_profiles_button_clicked), dialog);

  g_signal_connect (dialog->priv->crop_check, "toggled",
      G_CALLBACK (ogmrip_options_dialog_check_toggled), dialog->priv->crop_box);

  g_signal_connect_swapped (dialog->priv->crop_button, "clicked", 
      G_CALLBACK (ogmrip_options_dialog_crop_button_clicked), dialog);

  g_signal_connect_swapped (dialog->priv->autocrop_button, "clicked", 
      G_CALLBACK (ogmrip_options_dialog_autocrop_button_clicked), dialog);

  g_signal_connect (dialog->priv->scale_check, "toggled",
      G_CALLBACK (ogmrip_options_dialog_check_toggled), dialog->priv->scale_box);

  g_signal_connect_swapped (dialog->priv->autoscale_button, "clicked", 
      G_CALLBACK (ogmrip_options_dialog_autoscale_button_clicked), dialog);

  g_signal_connect_swapped (dialog->priv->scale_combo, "changed",
      G_CALLBACK (ogmrip_options_dialog_scale_combo_changed), dialog);

  g_signal_connect (dialog->priv->test_check, "toggled",
      G_CALLBACK (ogmrip_options_dialog_check_toggled), dialog->priv->test_box);

  g_signal_connect_swapped (dialog->priv->test_button, "clicked",
      G_CALLBACK (ogmrip_options_dialog_test_button_clicked), dialog);

  g_settings_bind_with_mapping (settings, OGMRIP_SETTINGS_PROFILE,
                                dialog->priv->profile_chooser, "profile",
                                G_SETTINGS_BIND_DEFAULT,
                                ogmrip_options_dialog_get_profile_mapping,
                                ogmrip_options_dialog_set_profile_mapping,
                                NULL, NULL);
}

static gboolean
ogmrip_options_dialog_initable_init (GInitable *initable, GCancellable *cancellable, GError **error)
{
  if (!OGMRIP_OPTIONS_DIALOG (initable)->priv->encoding)
    return FALSE;

  return TRUE;
}

static void
g_initable_iface_init (GInitableIface *iface)
{
  iface->init = ogmrip_options_dialog_initable_init;
}

GtkWidget *
ogmrip_options_dialog_new (OGMRipEncoding *encoding)
{
  return g_initable_new (OGMRIP_TYPE_OPTIONS_DIALOG, NULL, NULL, "use-header-bar", TRUE, "encoding", encoding, NULL);
}

GtkWidget *
ogmrip_options_dialog_new_at_scale (OGMRipEncoding *encoding, guint width, guint height)
{
  GtkWidget *dialog;

  dialog = g_initable_new (OGMRIP_TYPE_OPTIONS_DIALOG, NULL, NULL, "encoding", encoding, NULL);

  if (width && height)
  {
    OGMRipOptionsDialog *options = OGMRIP_OPTIONS_DIALOG (dialog);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->priv->scale_check), FALSE);
    gtk_combo_box_set_active (GTK_COMBO_BOX (options->priv->scale_combo), OGMRIP_SCALE_USER);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (options->priv->scale_width_spin), width);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (options->priv->scale_height_spin), height);
  }

  return dialog;
}

void
ogmrip_options_dialog_get_scale_size (OGMRipOptionsDialog *dialog, guint *width, guint *height)
{
  guint active;

  g_return_if_fail (OGMRIP_IS_OPTIONS_DIALOG (dialog));
  g_return_if_fail (width != NULL && height != NULL);

  active = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->priv->scale_combo));
  if (active == OGMRIP_SCALE_NONE)
    *width = *height = 0;
  else
  {
    *width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->scale_width_spin));
    *height = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->scale_height_spin));
  }
}

void
ogmrip_options_dialog_get_crop_size (OGMRipOptionsDialog *dialog, guint *x, guint *y, guint *width, guint *height)
{
  guint l, t, r, b;

  g_return_if_fail (OGMRIP_IS_OPTIONS_DIALOG (dialog));
  g_return_if_fail (x != NULL && y != NULL && width != NULL && height != NULL);

  ogmrip_options_dialog_get_crop (dialog, &l, &t, &r, &b);
  if (!l && !t && !r && !b)
    *x = *y = *width = *height = 0;
  else
    ogmrip_options_dialog_get_crop_full (dialog, x, y, width, height);
}

gint
ogmrip_options_dialog_get_deinterlacer (OGMRipOptionsDialog *dialog)
{
  g_return_val_if_fail (OGMRIP_IS_OPTIONS_DIALOG (dialog), -1);

  if (gtk_widget_is_sensitive (dialog->priv->deint_check) &&
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->deint_check)))
    return OGMRIP_DEINT_YADIF;

  return OGMRIP_DEINT_NONE;
}

