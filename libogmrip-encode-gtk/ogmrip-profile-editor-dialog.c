/* OGMRip - A library for media ripping and encoding
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

#include "ogmrip-profile-editor-dialog.h"
#include "ogmrip-type-chooser-widget.h"
#include "ogmrip-options-editable.h"

#include <glib/gi18n-lib.h>

#define OGMRIP_UI_FILE "ogmrip" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-profile-editor.ui"
#define OGMRIP_UI_ROOT "root"

#define gtk_builder_get_widget(builder, name) \
    (GtkWidget *) gtk_builder_get_object ((builder), (name))

struct _OGMRipProfileEditorDialogPriv
{
  GtkWidget *method_chooser;
  GtkWidget *container_chooser;
  GtkWidget *video_chooser;
  GtkWidget *audio_chooser;
  GtkWidget *subp_chooser;

  OGMRipProfile *profile;
};

enum
{
  PROP_0,
  PROP_PROFILE
};

static void ogmrip_profile_editor_dialog_constructed  (GObject               *gobject);
static void ogmrip_profile_editor_dialog_get_property (GObject               *gobject,
                                                       guint                 property_id,
                                                       GValue                *value,
                                                       GParamSpec            *pspec);
static void ogmrip_profile_editor_dialog_set_property (GObject               *gobject,
                                                       guint                 property_id,
                                                       const GValue          *value,
                                                       GParamSpec            *pspec);
static void ogmrip_profile_editor_dialog_dispose      (GObject               *gobject);

static GtkTreeRowReference *
gtk_tree_model_get_row_reference (GtkTreeModel *model, GtkTreeIter *iter)
{
  GtkTreePath *path;
  GtkTreeRowReference *ref;

  path = gtk_tree_model_get_path (model, iter);
  ref = gtk_tree_row_reference_new (model, path);
  gtk_tree_path_free (path);

  return ref;
}

static gboolean
gtk_tree_row_reference_get_iter (GtkTreeRowReference *ref, GtkTreeIter *iter)
{
  GtkTreeModel *model;
  GtkTreePath *path;
  gboolean retval;

  model = gtk_tree_row_reference_get_model (ref);

  path = gtk_tree_row_reference_get_path (ref);
  if (!path)
    return FALSE;

  retval = gtk_tree_model_get_iter (model, iter, path);

  gtk_tree_path_free (path);

  return retval;
}

static void
ogmrip_profile_editor_dialog_update_codecs (OGMRipProfileEditorDialog *editor)
{
  GType type;

  type = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (editor->priv->container_chooser));
  if (type != G_TYPE_NONE)
  {
    ogmrip_type_chooser_widget_set_filter (GTK_COMBO_BOX (editor->priv->video_chooser), type);
    if (gtk_combo_box_get_active (GTK_COMBO_BOX (editor->priv->video_chooser)) < 0)
      gtk_combo_box_set_active (GTK_COMBO_BOX (editor->priv->video_chooser), 0);

    ogmrip_type_chooser_widget_set_filter (GTK_COMBO_BOX (editor->priv->audio_chooser), type);
    if (gtk_combo_box_get_active (GTK_COMBO_BOX (editor->priv->audio_chooser)) < 0)
      gtk_combo_box_set_active (GTK_COMBO_BOX (editor->priv->audio_chooser), 0);

    ogmrip_type_chooser_widget_set_filter (GTK_COMBO_BOX (editor->priv->subp_chooser), type);
    if (gtk_combo_box_get_active (GTK_COMBO_BOX (editor->priv->subp_chooser)) < 0)
      gtk_combo_box_set_active (GTK_COMBO_BOX (editor->priv->subp_chooser), 0);
  }
}

static void
ogmrip_profile_editor_reset_button_clicked (OGMRipProfileEditorDialog *editor)
{
  ogmrip_profile_reset (editor->priv->profile);
}

static void
ogmrip_profile_editor_container_options_button_clicked (OGMRipProfileEditorDialog *editor)
{
  GType type;

  type = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (editor->priv->container_chooser));
  g_assert (type != G_TYPE_NONE);

  type = ogmrip_type_get_extension (type, OGMRIP_TYPE_OPTIONS_EDITABLE);
  if (type != G_TYPE_NONE)
  {
    GtkWidget *dialog;

    dialog = g_object_new (type, "profile", editor->priv->profile, NULL);
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (editor));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  }
}

static void
ogmrip_profile_editor_video_options_button_clicked (OGMRipProfileEditorDialog *editor)
{
  GType type;

  type = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (editor->priv->video_chooser));
  g_assert (type != G_TYPE_NONE);

  type = ogmrip_type_get_extension (type, OGMRIP_TYPE_OPTIONS_EDITABLE);
  if (type != G_TYPE_NONE)
  {
    GtkWidget *dialog;

    dialog = g_object_new (type, "profile", editor->priv->profile, NULL);
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (editor));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  }
}

static void
ogmrip_profile_editor_audio_options_button_clicked (OGMRipProfileEditorDialog *editor)
{
  GType type;

  type = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (editor->priv->audio_chooser));
  g_assert (type != G_TYPE_NONE);

  type = ogmrip_type_get_extension (type, OGMRIP_TYPE_OPTIONS_EDITABLE);
  if (type != G_TYPE_NONE)
  {
    GtkWidget *dialog;

    dialog = g_object_new (type, "profile", editor->priv->profile, NULL);
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (editor));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  }
}

static void
ogmrip_profile_editor_subp_options_button_clicked (OGMRipProfileEditorDialog *editor)
{
  GType type;

  type = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (editor->priv->subp_chooser));
  g_assert (type != G_TYPE_NONE);

  type = ogmrip_type_get_extension (type, OGMRIP_TYPE_OPTIONS_EDITABLE);
  if (type != G_TYPE_NONE)
  {
    GtkWidget *dialog;

    dialog = g_object_new (type, "profile", editor->priv->profile, NULL);
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (editor));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  }
}

static void
ogmrip_profile_editor_dialog_container_setting_changed (GtkWidget *chooser, const gchar *key, GSettings *settings);

static void
ogmrip_profile_editor_dialog_container_chooser_changed (GtkWidget *chooser, GSettings *settings)
{
  GType type;

  g_signal_handlers_block_by_func (settings,
      ogmrip_profile_editor_dialog_container_setting_changed, chooser);

  type = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (chooser));
  g_assert (type != G_TYPE_NONE);

  g_settings_set_string (settings, OGMRIP_PROFILE_CONTAINER, ogmrip_type_name (type));

  g_signal_handlers_unblock_by_func (settings,
      ogmrip_profile_editor_dialog_container_setting_changed, chooser);
}

static void
ogmrip_profile_editor_dialog_container_setting_changed (GtkWidget *chooser, const gchar *key, GSettings *settings)
{
  gchar *name;

  g_signal_handlers_block_by_func (chooser,
      ogmrip_profile_editor_dialog_container_chooser_changed, settings);

  name = g_settings_get_string (settings, OGMRIP_PROFILE_CONTAINER);
  ogmrip_type_chooser_widget_set_active (GTK_COMBO_BOX (chooser), ogmrip_type_from_name (name));
  g_free (name);

  g_signal_handlers_unblock_by_func (chooser,
      ogmrip_profile_editor_dialog_container_chooser_changed, settings);
}

static void
ogmrip_profile_editor_dialog_codec_setting_changed (GtkWidget *chooser, const gchar *key, GSettings *settings);

static void
ogmrip_profile_editor_dialog_codec_chooser_changed (GtkWidget *chooser, GSettings *settings)
{
  GType type;

  g_signal_handlers_block_by_func (settings,
      ogmrip_profile_editor_dialog_codec_setting_changed, chooser);

  type = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (chooser));
  g_settings_set_string (settings, OGMRIP_PROFILE_CODEC, ogmrip_type_name (type));

  g_signal_handlers_unblock_by_func (settings,
      ogmrip_profile_editor_dialog_codec_setting_changed, chooser);
}

static void
ogmrip_profile_editor_dialog_codec_setting_changed (GtkWidget *chooser, const gchar *key, GSettings *settings)
{
  gchar *name;

  g_signal_handlers_block_by_func (chooser,
      ogmrip_profile_editor_dialog_codec_chooser_changed, settings);

  name = g_settings_get_string (settings, OGMRIP_PROFILE_CODEC);
  ogmrip_type_chooser_widget_set_active (GTK_COMBO_BOX (chooser), ogmrip_type_from_name (name));
  g_free (name);

  g_signal_handlers_unblock_by_func (chooser,
      ogmrip_profile_editor_dialog_codec_chooser_changed, settings);
}

static void
ogmrip_profile_editor_dialog_set_user_entry_visibility (GtkWidget *chooser, GtkWidget *combo)
{
  GType type;

  type = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (chooser));
  if (type != G_TYPE_NONE)
  {
    GtkTreeRowReference *ref;
    GtkTreeIter iter;

    ref = g_object_get_data (G_OBJECT (combo), "row-reference");
    if (gtk_tree_row_reference_get_iter (ref, &iter))
    {
      GtkTreeModel *model; 

      model = gtk_tree_row_reference_get_model (ref);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          1, ogmrip_type_get_extension (type, GTK_TYPE_DIALOG) != G_TYPE_NONE, -1);
    }

    if (gtk_combo_box_get_active (GTK_COMBO_BOX (combo)) < 0)
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  }
}

static gboolean
ogmrip_profile_editor_set_expand_sensitivity (GValue *value, GVariant *variant, gpointer settings)
{
  g_value_set_boolean (value,
      g_settings_get_uint (settings, OGMRIP_PROFILE_MAX_WIDTH) > 0 &&
      g_settings_get_uint (settings, OGMRIP_PROFILE_MAX_HEIGHT) > 0);

  return TRUE;
}

static gboolean
ogmrip_profile_editor_set_video_options_sensitivity (GBinding *binding, const GValue *source_value, GValue *target_value, gpointer data)
{
  GtkWidget *chooser;
  GType type;
  gint format;

  chooser = (GtkWidget *) g_binding_get_source (binding);

  type = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (chooser));

  format = type != G_TYPE_NONE ? ogmrip_codec_format (type) : OGMRIP_FORMAT_UNDEFINED;
  g_value_set_boolean (target_value, format != OGMRIP_FORMAT_UNDEFINED && format != OGMRIP_FORMAT_COPY);

  return TRUE;
}

static gboolean
ogmrip_profile_editor_set_page_sensitivity (GBinding *binding, const GValue *source_value, GValue *target_value, gpointer data)
{
  g_value_set_boolean (target_value, g_value_get_int (source_value) >= 0);

  return TRUE;
}

static gboolean
ogmrip_profile_editor_set_options_button_sensitivity (GBinding *binding, const GValue *source_value, GValue *target_value, gpointer data)
{
  GtkWidget *chooser;
  GType type;

  chooser = (GtkWidget *) g_binding_get_source (binding);

  type = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (chooser));
  g_value_set_boolean (target_value,
      type != G_TYPE_NONE && ogmrip_type_get_extension (type, GTK_TYPE_DIALOG) != G_TYPE_NONE);

  return TRUE;
}

static gboolean
ogmrip_profile_editor_set_quality_options_sensitivity (GBinding *binding, const GValue *source_value, GValue *target_value, gpointer data)
{
  GtkWidget *chooser;
  GType type;
  gint format;

  chooser = (GtkWidget *) g_binding_get_source (binding);
  
  type = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (chooser));

  format = type != G_TYPE_NONE ? ogmrip_codec_format (type) : OGMRIP_FORMAT_UNDEFINED;
  g_value_set_boolean (target_value, format != OGMRIP_FORMAT_UNDEFINED && format != OGMRIP_FORMAT_COPY && format != OGMRIP_FORMAT_PCM);

  return TRUE;
}

static gboolean
ogmrip_profile_editor_set_audio_options_sensitivity (GBinding *binding, const GValue *source_value, GValue *target_value, gpointer data)
{
  GtkWidget *chooser;
  GType type;
  gint format;

  chooser = (GtkWidget *) g_binding_get_source (binding);

  type = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (chooser));

  format = type != G_TYPE_NONE ? ogmrip_codec_format (type) : OGMRIP_FORMAT_UNDEFINED;
  g_value_set_boolean (target_value, format != OGMRIP_FORMAT_UNDEFINED && format != OGMRIP_FORMAT_COPY);

  return TRUE;
}

static gboolean
ogmrip_profile_editor_set_text_options_sensitivity (GBinding *binding, const GValue *source_value, GValue *target_value, gpointer data)
{
  GtkWidget *chooser;
  GType type;

  chooser = (GtkWidget *) g_binding_get_source (binding);

  type = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (chooser));
  g_value_set_boolean (target_value, type != G_TYPE_NONE && ogmrip_codec_format (type) != OGMRIP_FORMAT_VOBSUB);

  return TRUE;
}

static gboolean
ogmrip_profile_editor_set_video_options_button_sensitivity (GBinding *binding, const GValue *source_value, GValue *target_value, gpointer data)
{
  g_value_set_boolean (target_value, g_value_get_int (source_value) == OGMRIP_VIDEO_QUALITY_USER);

  return TRUE;
}

static gboolean
ogmrip_profile_editor_set_target_sensitivity (GBinding *binding, const GValue *source_value, GValue *target_value, gpointer data)
{
  g_value_set_boolean (target_value, g_value_get_int (source_value) == OGMRIP_ENCODING_SIZE);

  return TRUE;
}

static gboolean
ogmrip_profile_editor_set_bitrate_sensitivity (GBinding *binding, const GValue *source_value, GValue *target_value, gpointer data)
{
  g_value_set_boolean (target_value, g_value_get_int (source_value) == OGMRIP_ENCODING_BITRATE);

  return TRUE;
}

static gboolean
ogmrip_profile_editor_set_quantizer_sensitivity (GBinding *binding, const GValue *source_value, GValue *target_value, gpointer data)
{
  g_value_set_boolean (target_value, g_value_get_int (source_value) == OGMRIP_ENCODING_QUANTIZER);

  return TRUE;
}

static gboolean
ogmrip_profile_editor_set_passes_sensitivity (GBinding *binding, const GValue *source_value, GValue *target_value, gpointer data)
{
  OGMRipProfileEditorDialog *dialog = data;
  GType codec;
  gint method;

  method = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->priv->method_chooser));
  codec = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (dialog->priv->video_chooser));

  g_value_set_boolean (target_value, method != OGMRIP_ENCODING_QUANTIZER &&
      codec != G_TYPE_NONE && ogmrip_codec_format (codec) != OGMRIP_FORMAT_COPY);

  return TRUE;
}

static gboolean
ogmrip_profile_editor_set_passes_spin_adjustment (GBinding *binding, const GValue *source_value, GValue *target_value, gpointer data)
{
  GtkWidget *widget;
  GType type;

  widget = (GtkWidget *) g_binding_get_source (binding);

  type = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (widget));
  if (type != G_TYPE_NONE)
  {
    GtkAdjustment *adj;
    GParamSpec *pspec;
    guint value, lower, upper;

    widget = (GtkWidget *) g_binding_get_target (binding);

    adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));

    value = gtk_adjustment_get_value (adj);
    lower = gtk_adjustment_get_lower (adj);

    pspec = ogmrip_type_property (type, "passes");
    upper = G_PARAM_SPEC_UINT (pspec)->maximum;

    g_value_set_object (target_value,
        gtk_adjustment_new (CLAMP (value, lower, upper), lower, upper,
          gtk_adjustment_get_step_increment (adj),
          gtk_adjustment_get_page_increment (adj),
          gtk_adjustment_get_page_size (adj)));
  }

  return TRUE;
}

G_DEFINE_TYPE (OGMRipProfileEditorDialog, ogmrip_profile_editor_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_profile_editor_dialog_class_init (OGMRipProfileEditorDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_profile_editor_dialog_constructed;
  gobject_class->get_property = ogmrip_profile_editor_dialog_get_property;
  gobject_class->set_property = ogmrip_profile_editor_dialog_set_property;
  gobject_class->dispose = ogmrip_profile_editor_dialog_dispose;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_PROFILE,
      g_param_spec_object ("profile", "profile", "profile", OGMRIP_TYPE_PROFILE,
        G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (OGMRipProfileEditorDialogPriv));
}

static void
ogmrip_profile_editor_dialog_init (OGMRipProfileEditorDialog *dialog)
{
  dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog,
      OGMRIP_TYPE_PROFILE_EDITOR_DIALOG, OGMRipProfileEditorDialogPriv);
}

static void
ogmrip_profile_editor_dialog_constructed (GObject *gobject)
{
  GError *error = NULL;
  OGMRipProfileEditorDialog *dialog = OGMRIP_PROFILE_EDITOR_DIALOG (gobject);

  GSettings *settings;
  GtkWidget *widget, *misc;
  GtkBuilder *builder;

  GtkTreeModel *model, *filter;
  GtkTreeRowReference *ref;
  GtkTreeIter iter;

  G_OBJECT_CLASS (ogmrip_profile_editor_dialog_parent_class)->constructed (gobject);

  if (!dialog->priv->profile)
    g_error ("No profile specified");

  misc = gtk_dialog_get_action_area (GTK_DIALOG (gobject));

  widget = gtk_button_new_with_mnemonic (_("_Reset"));
  gtk_container_add (GTK_CONTAINER (misc), widget);
  gtk_widget_show (widget);

  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_profile_editor_reset_button_clicked), dialog);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
      NULL);
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

  /*
   * Container
   */

  settings = ogmrip_profile_get_child (dialog->priv->profile, OGMRIP_PROFILE_GENERAL);

  dialog->priv->container_chooser = gtk_builder_get_widget (builder, "container-combo");
  ogmrip_type_chooser_widget_construct (GTK_COMBO_BOX (dialog->priv->container_chooser), OGMRIP_TYPE_CONTAINER);

  ogmrip_profile_editor_dialog_container_setting_changed (dialog->priv->container_chooser, OGMRIP_PROFILE_CONTAINER, settings);

  g_signal_connect (dialog->priv->container_chooser, "changed",
      G_CALLBACK (ogmrip_profile_editor_dialog_container_chooser_changed), settings);
  g_signal_connect_swapped (settings, "changed::" OGMRIP_PROFILE_CONTAINER,
      G_CALLBACK (ogmrip_profile_editor_dialog_container_setting_changed), dialog->priv->container_chooser);

  g_signal_connect_swapped (dialog->priv->container_chooser, "changed",
      G_CALLBACK (ogmrip_profile_editor_dialog_update_codecs), dialog);

  widget = gtk_builder_get_widget (builder, "container-page");
  g_object_bind_property_full (dialog->priv->container_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_page_sensitivity, NULL, NULL, NULL);

  widget = gtk_builder_get_widget (builder, "container-options-button");
  g_object_bind_property_full (dialog->priv->container_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_options_button_sensitivity, NULL, NULL, NULL);

  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_profile_editor_container_options_button_clicked), dialog);

  widget = gtk_builder_get_widget (builder, "fourcc-combo");
  g_settings_bind (settings, OGMRIP_PROFILE_FOURCC, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "ensure-sync-check");
  g_settings_bind (settings, OGMRIP_PROFILE_ENSURE_SYNC, widget, "active", G_SETTINGS_BIND_DEFAULT);

  dialog->priv->method_chooser = gtk_builder_get_widget (builder, "encoding-combo");
  g_settings_bind (settings, OGMRIP_PROFILE_ENCODING_METHOD, dialog->priv->method_chooser, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "tnumber-spin");
  g_settings_bind (settings, OGMRIP_PROFILE_TARGET_NUMBER, widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "tsize-spin");
  g_settings_bind (settings, OGMRIP_PROFILE_TARGET_SIZE, widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "target-label");
  g_object_bind_property_full (dialog->priv->method_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_target_sensitivity, NULL, NULL, NULL);

  misc = gtk_builder_get_widget (builder, "target-hbox");
  g_object_bind_property (widget, "sensitive", misc, "sensitive", G_BINDING_SYNC_CREATE);

  widget = gtk_builder_get_widget (builder, "bitrate-label");
  g_object_bind_property_full (dialog->priv->method_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_bitrate_sensitivity, NULL, NULL, NULL);

  misc = gtk_builder_get_widget (builder, "bitrate-hbox");
  g_object_bind_property (widget, "sensitive", misc, "sensitive", G_BINDING_SYNC_CREATE);

  widget = gtk_builder_get_widget (builder, "quantizer-label");
  g_object_bind_property_full (dialog->priv->method_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_quantizer_sensitivity, NULL, NULL, NULL);

  misc = gtk_builder_get_widget (builder, "quantizer-hbox");
  g_object_bind_property (widget, "sensitive", misc, "sensitive", G_BINDING_SYNC_CREATE);

  g_object_unref (settings);

  /*
   * Video
   */

  settings = ogmrip_profile_get_child (dialog->priv->profile, OGMRIP_PROFILE_VIDEO);

  dialog->priv->video_chooser = gtk_builder_get_widget (builder, "video-codec-combo");
  ogmrip_type_chooser_widget_construct (GTK_COMBO_BOX (dialog->priv->video_chooser), OGMRIP_TYPE_VIDEO_CODEC);

  ogmrip_profile_editor_dialog_codec_setting_changed (dialog->priv->video_chooser, OGMRIP_PROFILE_CODEC, settings);

  g_signal_connect (dialog->priv->video_chooser, "changed",
      G_CALLBACK (ogmrip_profile_editor_dialog_codec_chooser_changed), settings);
  g_signal_connect_swapped (settings, "changed::" OGMRIP_PROFILE_CODEC,
      G_CALLBACK (ogmrip_profile_editor_dialog_codec_setting_changed), dialog->priv->video_chooser);

  widget = gtk_builder_get_widget (builder, "video-page");
  g_object_bind_property_full (dialog->priv->video_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_page_sensitivity, NULL, NULL, NULL);

  widget = gtk_builder_get_widget (builder, "bitrate-spin");
  g_settings_bind (settings, OGMRIP_PROFILE_BITRATE, widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "quantizer-spin");
  g_settings_bind (settings, OGMRIP_PROFILE_QUANTIZER, widget, "value", G_SETTINGS_BIND_DEFAULT);

  misc = gtk_builder_get_widget (builder, "video-quality-combo");
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (misc));

  filter = gtk_tree_model_filter_new (model, NULL);
  gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (filter), 1);
  gtk_combo_box_set_model (GTK_COMBO_BOX (misc), filter);
  g_object_unref (filter);

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, _("User"), 1, FALSE, -1);

  ref = gtk_tree_model_get_row_reference (model, &iter);
  g_object_set_data_full (G_OBJECT (misc), "row-reference", ref,
      (GDestroyNotify) gtk_tree_row_reference_free);

  ogmrip_profile_editor_dialog_set_user_entry_visibility (dialog->priv->video_chooser, misc);
  g_signal_connect (dialog->priv->video_chooser, "changed",
      G_CALLBACK (ogmrip_profile_editor_dialog_set_user_entry_visibility), misc);

  g_settings_bind (settings, OGMRIP_PROFILE_QUALITY, misc, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "video-options-button");
  g_object_bind_property_full (misc, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_video_options_button_sensitivity, NULL, NULL, NULL);

  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_profile_editor_video_options_button_clicked), dialog);

  misc = gtk_builder_get_widget (builder, "passes-spin");
  g_settings_bind (settings, OGMRIP_PROFILE_PASSES, misc, "value", G_SETTINGS_BIND_DEFAULT);

  g_object_bind_property_full (dialog->priv->video_chooser, "active", misc, "adjustment", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_passes_spin_adjustment, NULL, NULL, NULL);

  widget = gtk_builder_get_widget (builder, "passes-label");
  g_object_bind_property_full (dialog->priv->method_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_passes_sensitivity, NULL, dialog, NULL);
  g_object_bind_property_full (dialog->priv->video_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_passes_sensitivity, NULL, dialog, NULL);

  misc = gtk_builder_get_widget (builder, "passes-hbox");
  g_object_bind_property (widget, "sensitive", misc, "sensitive", G_BINDING_SYNC_CREATE);

  widget = gtk_builder_get_widget (builder, "denoise-check");
  g_settings_bind (settings, OGMRIP_PROFILE_DENOISE, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "deblock-check");
  g_settings_bind (settings, OGMRIP_PROFILE_DEBLOCK, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "dering-check");
  g_settings_bind (settings, OGMRIP_PROFILE_DERING, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "turbo-check");
  g_settings_bind (settings, OGMRIP_PROFILE_TURBO, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "can-crop-check");
  g_settings_bind (settings, OGMRIP_PROFILE_CAN_CROP, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "scaler-combo");
  g_settings_bind (settings, OGMRIP_PROFILE_SCALER, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "max-width-spin");
  g_settings_bind (settings, OGMRIP_PROFILE_MAX_WIDTH, widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "max-height-spin");
  g_settings_bind (settings, OGMRIP_PROFILE_MAX_HEIGHT, widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "min-width-spin");
  g_settings_bind (settings, OGMRIP_PROFILE_MIN_WIDTH, widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "min-height-spin");
  g_settings_bind (settings, OGMRIP_PROFILE_MIN_HEIGHT, widget, "value", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "expand-check");
  g_settings_bind (settings, OGMRIP_PROFILE_EXPAND, widget, "active", G_SETTINGS_BIND_DEFAULT);

  g_settings_bind_with_mapping (settings, OGMRIP_PROFILE_MAX_WIDTH, widget, "sensitive", G_SETTINGS_BIND_GET,
      ogmrip_profile_editor_set_expand_sensitivity, NULL, settings, NULL);
  g_settings_bind_with_mapping (settings, OGMRIP_PROFILE_MAX_HEIGHT, widget, "sensitive", G_SETTINGS_BIND_GET,
      ogmrip_profile_editor_set_expand_sensitivity, NULL, settings, NULL);

  widget = gtk_builder_get_widget (builder, "video-options-label");
  g_object_bind_property_full (dialog->priv->video_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_video_options_sensitivity, NULL, NULL, NULL);

  misc = gtk_builder_get_widget (builder, "video-options-table");
  g_object_bind_property (widget, "sensitive", misc, "sensitive", G_BINDING_SYNC_CREATE);

  misc = gtk_builder_get_widget (builder, "video-options-expander");
  g_object_bind_property (widget, "sensitive", misc, "sensitive", G_BINDING_SYNC_CREATE);

  misc = gtk_builder_get_widget (builder, "encoding-method-table");
  g_object_bind_property (widget, "sensitive", misc, "sensitive", G_BINDING_SYNC_CREATE);

  misc = gtk_builder_get_widget (builder, "video-quality-label");
  g_object_bind_property (widget, "sensitive", misc, "sensitive", G_BINDING_SYNC_CREATE);

  misc = gtk_builder_get_widget (builder, "video-quality-box");
  g_object_bind_property (widget, "sensitive", misc, "sensitive", G_BINDING_SYNC_CREATE);

  g_object_unref (settings);

  /*
   * Audio
   */

  settings = ogmrip_profile_get_child (dialog->priv->profile, OGMRIP_PROFILE_AUDIO);

  dialog->priv->audio_chooser = gtk_builder_get_widget (builder, "audio-codec-combo");
  ogmrip_type_chooser_widget_construct (GTK_COMBO_BOX (dialog->priv->audio_chooser), OGMRIP_TYPE_AUDIO_CODEC);

  ogmrip_profile_editor_dialog_codec_setting_changed (dialog->priv->audio_chooser, OGMRIP_PROFILE_CODEC, settings);

  g_signal_connect (dialog->priv->audio_chooser, "changed",
      G_CALLBACK (ogmrip_profile_editor_dialog_codec_chooser_changed), settings);
  g_signal_connect_swapped (settings, "changed::" OGMRIP_PROFILE_CODEC,
      G_CALLBACK (ogmrip_profile_editor_dialog_codec_setting_changed), dialog->priv->audio_chooser);

  widget = gtk_builder_get_widget (builder, "audio-page");
  g_object_bind_property_full (dialog->priv->audio_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_page_sensitivity, NULL, NULL, NULL);

  widget = gtk_builder_get_widget (builder, "audio-options-button");
  g_object_bind_property_full (dialog->priv->audio_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_options_button_sensitivity, NULL, NULL, NULL);

  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_profile_editor_audio_options_button_clicked), dialog);

  widget = gtk_builder_get_widget (builder, "audio-quality-spin");
  g_settings_bind (settings, OGMRIP_PROFILE_QUALITY, widget, "value", G_SETTINGS_BIND_DEFAULT);

  g_object_bind_property_full (dialog->priv->audio_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_quality_options_sensitivity, NULL, NULL, NULL);

  misc = gtk_builder_get_widget (builder, "audio-quality-label");
  g_object_bind_property (widget, "sensitive", misc, "sensitive", G_BINDING_SYNC_CREATE);

  widget = gtk_builder_get_widget (builder, "channels-combo");
  g_settings_bind (settings, OGMRIP_PROFILE_CHANNELS, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "srate-combo");
  g_settings_bind (settings, OGMRIP_PROFILE_SAMPLERATE, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "normalize-check");
  g_settings_bind (settings, OGMRIP_PROFILE_NORMALIZE, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "audio-options-label");
  g_object_bind_property_full (dialog->priv->audio_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_audio_options_sensitivity, NULL, NULL, NULL);

  misc = gtk_builder_get_widget (builder, "audio-options-table");
  g_object_bind_property (widget, "sensitive", misc, "sensitive", G_BINDING_SYNC_CREATE);

  g_object_unref (settings);

  /*
   * Subp
   */

  settings = ogmrip_profile_get_child (dialog->priv->profile, OGMRIP_PROFILE_SUBP);

  dialog->priv->subp_chooser = gtk_builder_get_widget (builder, "subp-codec-combo");
  ogmrip_type_chooser_widget_construct (GTK_COMBO_BOX (dialog->priv->subp_chooser), OGMRIP_TYPE_SUBP_CODEC);

  ogmrip_profile_editor_dialog_codec_setting_changed (dialog->priv->subp_chooser, OGMRIP_PROFILE_CODEC, settings);

  g_signal_connect (dialog->priv->subp_chooser, "changed",
      G_CALLBACK (ogmrip_profile_editor_dialog_codec_chooser_changed), settings);
  g_signal_connect_swapped (settings, "changed::" OGMRIP_PROFILE_CODEC,
      G_CALLBACK (ogmrip_profile_editor_dialog_codec_setting_changed), dialog->priv->subp_chooser);

  widget = gtk_builder_get_widget (builder, "subp-page");
  g_object_bind_property_full (dialog->priv->subp_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_page_sensitivity, NULL, NULL, NULL);

  widget = gtk_builder_get_widget (builder, "subp-options-button");
  g_object_bind_property_full (dialog->priv->subp_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_options_button_sensitivity, NULL, NULL, NULL);

  g_signal_connect_swapped (widget, "clicked",
      G_CALLBACK (ogmrip_profile_editor_subp_options_button_clicked), dialog);

  widget = gtk_builder_get_widget (builder, "forced-subs-check");
  g_settings_bind (settings, OGMRIP_PROFILE_FORCED_ONLY, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "charset-combo");
  g_settings_bind (settings, OGMRIP_PROFILE_CHARACTER_SET, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "newline-combo");
  g_settings_bind (settings, OGMRIP_PROFILE_NEWLINE_STYLE, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "spell-check");
  g_settings_bind (settings, OGMRIP_PROFILE_SPELL_CHECK, widget, "active", G_SETTINGS_BIND_DEFAULT);

  widget = gtk_builder_get_widget (builder, "subp-options-label");
  g_object_bind_property_full (dialog->priv->subp_chooser, "active", widget, "sensitive", G_BINDING_SYNC_CREATE,
      ogmrip_profile_editor_set_text_options_sensitivity, NULL, NULL, NULL);

  misc = gtk_builder_get_widget (builder, "subp-options-table");
  g_object_bind_property (widget, "sensitive", misc, "sensitive", G_BINDING_SYNC_CREATE);

  g_object_unref (settings);

  g_object_unref (builder);

  ogmrip_profile_editor_dialog_update_codecs (dialog);
}

static void
ogmrip_profile_editor_dialog_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_PROFILE:
      g_value_set_object (value, OGMRIP_PROFILE_EDITOR_DIALOG (gobject)->priv->profile);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static gchar *
get_profile_title (const gchar *name)
{
  gchar *str1, *str2;
  gint i;

  for (i = 0; name[i] != '\0'; i++)
    if (name[i] == '\n' || name[i] == '\r')
      break;

  str1 = g_strndup (name, i);

  if (!pango_parse_markup (str1, -1, 0, NULL, &str2, NULL, NULL))
    str2 = g_strdup (name);

  g_free (str1);

  str1 = g_strdup_printf (_("Editing profile \"%s\""), str2);
  g_free (str2);

  return str1;
}

static void
ogmrip_profile_editor_set_profile (OGMRipProfileEditorDialog *dialog, OGMRipProfile *profile)
{
  gchar *name, *title;

  dialog->priv->profile = g_object_ref (profile);

  name = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME);
  title = get_profile_title (name);
  g_free (name);

  gtk_window_set_title (GTK_WINDOW (dialog), title);
  g_free (title);
}

static void
ogmrip_profile_editor_dialog_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_PROFILE:
      ogmrip_profile_editor_set_profile (OGMRIP_PROFILE_EDITOR_DIALOG (gobject), g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_profile_editor_dialog_dispose (GObject *gobject)
{
  OGMRipProfileEditorDialog *dialog;

  dialog = OGMRIP_PROFILE_EDITOR_DIALOG (gobject);

  if (dialog->priv->profile)
  {
    g_object_unref (dialog->priv->profile);
    dialog->priv->profile = NULL;
  }

  (*G_OBJECT_CLASS (ogmrip_profile_editor_dialog_parent_class)->dispose) (gobject);
}

GtkWidget *
ogmrip_profile_editor_dialog_new (OGMRipProfile *profile)
{
  return g_object_new (OGMRIP_TYPE_PROFILE_EDITOR_DIALOG, "profile", profile, NULL);
}

