/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-audio-options-dialog.h"
#include "ogmrip-helper.h"

#include <ogmrip-media-gtk.h>

#include <glib/gi18n-lib.h>

#define OGMRIP_GLADE_FILE "ogmrip" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-profile-editor.glade"
#define OGMRIP_GLADE_ROOT "audio-page"

#define OGMRIP_AUDIO_OPTIONS_DIALOG_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_AUDIO_OPTIONS_DIALOG, OGMRipAudioOptionsDialogPriv))

struct _OGMRipAudioOptionsDialogPriv
{
  GtkWidget *codec_combo;
  GtkWidget *default_check;
  GtkWidget *quality_spin;
  GtkWidget *srate_combo;
  GtkWidget *channels_combo;
  GtkWidget *normalize_check;
  GtkWidget *lang_chooser;
  GtkWidget *label_entry;
};

enum
{
  PROP_0,
  PROP_CHANNELS,
  PROP_CODEC,
  PROP_LABEL,
  PROP_LANGUAGE,
  PROP_NORMALIZE,
  PROP_QUALITY,
  PROP_SAMPLE_RATE
};

static void ogmrip_audio_options_init (OGMRipAudioOptionsInterface *iface);
static void ogmrip_audio_options_dialog_get_property  (GObject     *gobject,
                                                      guint        prop_id,
                                                      GValue       *value,
                                                      GParamSpec   *pspec);
static void ogmrip_audio_options_dialog_set_property (GObject      *gobject,
                                                      guint        prop_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec);

static void
ogmrip_audio_options_dialog_codec_changed (GtkWidget *combo, GtkWidget *table)
{
  GType codec;
  gchar *name;

  name = ogmrip_codec_chooser_get_active (GTK_COMBO_BOX (combo));
  codec = ogmrip_plugin_get_audio_codec_by_name (name);
  g_free (name);

  if (codec != G_TYPE_NONE)
    gtk_widget_set_sensitive (table, ogmrip_plugin_get_audio_codec_format (codec) != OGMRIP_FORMAT_COPY);
}

static gint
ogmrip_audio_options_dialog_get_channels (OGMRipAudioOptionsDialog *dialog)
{
  return gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->priv->channels_combo));
}

static void
ogmrip_audio_options_dialog_set_channels (OGMRipAudioOptionsDialog *dialog, OGMRipChannels channels)
{
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->channels_combo), channels);
}

static GType
ogmrip_audio_options_dialog_get_codec (OGMRipAudioOptionsDialog *dialog)
{
  return ogmrip_codec_chooser_get_active_type (GTK_COMBO_BOX (dialog->priv->codec_combo));
}

static void
ogmrip_audio_options_dialog_set_codec (OGMRipAudioOptionsDialog *dialog, GType codec)
{
  const gchar *name;

  name = ogmrip_plugin_get_audio_codec_name (codec);
  ogmrip_codec_chooser_set_active (GTK_COMBO_BOX (dialog->priv->codec_combo), name);
}

static gboolean
ogmrip_audio_options_dialog_get_normalize (OGMRipAudioOptionsDialog *dialog)
{
  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->normalize_check));
}

static void
ogmrip_audio_options_dialog_set_normalize (OGMRipAudioOptionsDialog *dialog, gboolean normalize)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->normalize_check), normalize);
}

static gint
ogmrip_audio_options_dialog_get_quality (OGMRipAudioOptionsDialog *dialog)
{
  return gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->quality_spin));
}

static void
ogmrip_audio_options_dialog_set_quality (OGMRipAudioOptionsDialog *dialog, guint quality)
{
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->quality_spin), quality);
}

static gint
ogmrip_audio_options_dialog_get_sample_rate (OGMRipAudioOptionsDialog *dialog)
{
  static const gint sample_rate[] =
  { 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000 };
  gint i;

  i = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->priv->srate_combo));

  return sample_rate[i];
}

static void
ogmrip_audio_options_dialog_set_sample_rate (OGMRipAudioOptionsDialog *dialog, guint srate)
{
  if (srate >= 48000)
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->srate_combo), 0);
  else if (srate >= 44100)
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->srate_combo), 1);
  else if (srate >= 32000)
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->srate_combo), 2);
  else if (srate >= 24000)
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->srate_combo), 3);
  else if (srate >= 22050)
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->srate_combo), 4);
  else if (srate >= 16000)
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->srate_combo), 5);
  else if (srate >= 12000)
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->srate_combo), 6);
  else if (srate >= 11025)
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->srate_combo), 7);
  else
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->srate_combo), 8);
}

G_DEFINE_TYPE_WITH_CODE (OGMRipAudioOptionsDialog, ogmrip_audio_options_dialog, GTK_TYPE_DIALOG,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_AUDIO_OPTIONS, ogmrip_audio_options_init))

static void
ogmrip_audio_options_dialog_class_init (OGMRipAudioOptionsDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_audio_options_dialog_get_property;
  gobject_class->set_property = ogmrip_audio_options_dialog_set_property;

  g_object_class_override_property (gobject_class, PROP_CHANNELS, "channels");
  g_object_class_override_property (gobject_class, PROP_CODEC, "codec");
  g_object_class_override_property (gobject_class, PROP_NORMALIZE, "normalize");
  g_object_class_override_property (gobject_class, PROP_QUALITY, "quality");
  g_object_class_override_property (gobject_class, PROP_SAMPLE_RATE, "sample-rate");

  g_object_class_install_property (gobject_class, PROP_LABEL,
      g_param_spec_string ("label", "Label property", "Set label",
        NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LANGUAGE,
      g_param_spec_uint ("language", "Language property", "Set language",
        0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipAudioOptionsDialogPriv));
}

static void
ogmrip_audio_options_init (OGMRipAudioOptionsInterface *iface)
{
}

static void
ogmrip_audio_options_dialog_init (OGMRipAudioOptionsDialog *dialog)
{
  GError *error = NULL;

  GtkWidget *root, *area, *vbox, *vbox1, *vbox2, *alignment, *table, *label;
  GtkBuilder *builder;

  GtkTreeModel *model;
  GtkTreeIter iter;

  dialog->priv = OGMRIP_AUDIO_OPTIONS_DIALOG_GET_PRIVATE (dialog);

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_GLADE_FILE, &error))
  {
    g_warning ("Couldn't load builder file: %s", error->message);
    g_object_unref (builder);
    g_error_free (error);
    return;
  }

  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Audio Track Options"));

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (area), vbox);
  gtk_widget_show (vbox);

  vbox1 = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 6);
  gtk_box_pack_start (GTK_BOX (vbox), vbox1, FALSE, FALSE, 0);
  gtk_widget_show (vbox1);

  vbox2 = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, FALSE, 0);
  gtk_widget_show (vbox2);

  label = gtk_label_new (_("<b>Track</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), alignment, FALSE, FALSE, 0);
  gtk_widget_show (alignment);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (alignment), table);
  gtk_widget_show (table);

  label = gtk_label_new_with_mnemonic (_("_Name:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);

  dialog->priv->label_entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (dialog->priv->label_entry), TRUE);
  gtk_table_attach (GTK_TABLE (table), dialog->priv->label_entry, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (dialog->priv->label_entry);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->priv->label_entry);

  label = gtk_label_new_with_mnemonic (_("_Language:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);

  dialog->priv->lang_chooser = ogmrip_language_chooser_widget_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->priv->lang_chooser);
  gtk_table_attach (GTK_TABLE (table), dialog->priv->lang_chooser, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (dialog->priv->lang_chooser);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (dialog->priv->lang_chooser));
  gtk_list_store_prepend (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      OGMRIP_LANGUAGE_STORE_NAME_COLUMN, _("None"),
      OGMRIP_LANGUAGE_STORE_CODE_COLUMN, 0, -1);

  dialog->priv->default_check = gtk_check_button_new_with_mnemonic (_("Use _profile settings"));
  gtk_table_attach (GTK_TABLE (table), dialog->priv->default_check, 0, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (dialog->priv->default_check);

  root = gtk_builder_get_widget (builder, OGMRIP_GLADE_ROOT);
  gtk_widget_reparent (root, vbox);
  gtk_widget_show (root);

  gtk_box_set_child_packing (GTK_BOX (vbox), root, TRUE, TRUE, 0, GTK_PACK_START);

  g_object_bind_property (dialog->priv->default_check, "active", root, "visible", G_BINDING_INVERT_BOOLEAN);

  dialog->priv->codec_combo = gtk_builder_get_widget (builder, "audio-codec-combo");
  ogmrip_audio_codec_chooser_construct (GTK_COMBO_BOX (dialog->priv->codec_combo));

  dialog->priv->quality_spin = gtk_builder_get_widget (builder, "audio-quality-spin");
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->quality_spin), 3);

  dialog->priv->srate_combo = gtk_builder_get_widget (builder, "srate-combo");
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->srate_combo), 0);

  dialog->priv->channels_combo = gtk_builder_get_widget (builder, "channels-combo");
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->channels_combo), 1);

  dialog->priv->normalize_check = gtk_builder_get_widget (builder, "normalize-check");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->normalize_check), TRUE);

  table = gtk_builder_get_widget (builder, "audio-options-table");
  g_signal_connect (dialog->priv->codec_combo, "changed",
      G_CALLBACK (ogmrip_audio_options_dialog_codec_changed), table);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->default_check), TRUE);
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->codec_combo), 0);

  g_object_unref (builder);
}

static void
ogmrip_audio_options_dialog_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  OGMRipAudioOptionsDialog *dialog = OGMRIP_AUDIO_OPTIONS_DIALOG (gobject);

  switch (prop_id) 
  {
    case PROP_CHANNELS:
      g_value_set_uint (value, ogmrip_audio_options_dialog_get_channels (dialog));
      break;
    case PROP_CODEC:
      g_value_set_gtype (value, ogmrip_audio_options_dialog_get_codec (dialog));
      break;
    case PROP_LABEL:
      g_value_set_string (value, ogmrip_audio_options_dialog_get_label (dialog));
      break;
    case PROP_LANGUAGE:
      g_value_set_uint (value, ogmrip_audio_options_dialog_get_language (dialog));
      break;
    case PROP_NORMALIZE:
      g_value_set_boolean (value, ogmrip_audio_options_dialog_get_normalize (dialog));
      break;
    case PROP_QUALITY:
      g_value_set_uint (value, ogmrip_audio_options_dialog_get_quality (dialog));
      break;
    case PROP_SAMPLE_RATE:
      g_value_set_uint (value, ogmrip_audio_options_dialog_get_sample_rate (dialog));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_audio_options_dialog_set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipAudioOptionsDialog *dialog = OGMRIP_AUDIO_OPTIONS_DIALOG (gobject);

  switch (prop_id) 
  {
    case PROP_CHANNELS:
      ogmrip_audio_options_dialog_set_channels (dialog, g_value_get_uint (value));
      break;
    case PROP_CODEC:
      ogmrip_audio_options_dialog_set_codec (dialog, g_value_get_gtype (value));
      break;
    case PROP_LABEL:
      ogmrip_audio_options_dialog_set_label (dialog, g_value_get_string (value));
      break;
    case PROP_LANGUAGE:
      ogmrip_audio_options_dialog_set_language (dialog, g_value_get_uint (value));
      break;
    case PROP_NORMALIZE:
      ogmrip_audio_options_dialog_set_normalize (dialog, g_value_get_boolean (value));
      break;
    case PROP_QUALITY:
      ogmrip_audio_options_dialog_set_quality (dialog, g_value_get_uint (value));
      break;
    case PROP_SAMPLE_RATE:
      ogmrip_audio_options_dialog_set_sample_rate (dialog, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

GtkWidget *
ogmrip_audio_options_dialog_new (void)
{
  return g_object_new (OGMRIP_TYPE_AUDIO_OPTIONS_DIALOG, NULL);
}

const  gchar *
ogmrip_audio_options_dialog_get_label (OGMRipAudioOptionsDialog *dialog)
{
  return gtk_entry_get_text (GTK_ENTRY (dialog->priv->label_entry));
}

void
ogmrip_audio_options_dialog_set_label (OGMRipAudioOptionsDialog *dialog, const gchar *label)
{
  gtk_entry_set_text (GTK_ENTRY (dialog->priv->label_entry), label ? label : "");
}

gint
ogmrip_audio_options_dialog_get_language (OGMRipAudioOptionsDialog *dialog)
{
  return ogmrip_language_chooser_widget_get_active (OGMRIP_LANGUAGE_CHOOSER_WIDGET (dialog->priv->lang_chooser));
}

void
ogmrip_audio_options_dialog_set_language (OGMRipAudioOptionsDialog *dialog, guint lang)
{
  ogmrip_language_chooser_widget_set_active (OGMRIP_LANGUAGE_CHOOSER_WIDGET (dialog->priv->lang_chooser), lang);
}

gboolean
ogmrip_audio_options_dialog_get_use_defaults (OGMRipAudioOptionsDialog *dialog)
{
  g_return_val_if_fail (OGMRIP_IS_AUDIO_OPTIONS_DIALOG (dialog), FALSE);

  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->default_check));
}

void
ogmrip_audio_options_dialog_set_use_defaults (OGMRipAudioOptionsDialog *dialog, gboolean use_defaults)
{
  g_return_if_fail (OGMRIP_IS_AUDIO_OPTIONS_DIALOG (dialog));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->default_check), use_defaults);
}

