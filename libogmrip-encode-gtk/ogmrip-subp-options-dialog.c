/* OGMRip - A library for media ripping and encoding
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

#include "ogmrip-subp-options-dialog.h"
#include "ogmrip-language-chooser-widget.h"
#include "ogmrip-type-chooser-widget.h"

#include <glib/gi18n-lib.h>

#define OGMRIP_UI_RES  "/org/ogmrip/ogmrip-profile-editor-dialog.ui"
#define OGMRIP_UI_ROOT "subp-page"

#define gtk_builder_get_widget(builder, name) \
    (GtkWidget *) gtk_builder_get_object ((builder), (name))

struct _OGMRipSubpOptionsDialogPriv
{
  GtkWidget *codec_combo;
  GtkWidget *default_check;
  GtkWidget *charset_combo;
  GtkWidget *newline_combo;
  GtkWidget *spell_check;
  GtkWidget *forced_subs_check;
  GtkWidget *lang_chooser;
  GtkWidget *label_entry;
};

enum
{
  PROP_0,
  PROP_CHARSET,
  PROP_CODEC,
  PROP_FORCED_ONLY,
  PROP_LABEL,
  PROP_LANGUAGE,
  PROP_NEWLINE,
  PROP_SPELL_CHECK
};

static void ogmrip_subp_options_init (OGMRipSubpOptionsInterface *iface);
static void ogmrip_subp_options_dialog_get_property (GObject     *gobject,
                                                     guint        prop_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);
static void ogmrip_subp_options_dialog_set_property (GObject      *gobject,
                                                     guint        prop_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);

static void
ogmrip_subp_options_dialog_codec_changed (GtkWidget *combo, GtkWidget *table)
{
  GType codec;

  codec = ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (combo));
  if (codec != G_TYPE_NONE)
    gtk_widget_set_sensitive (table, ogmrip_codec_format (codec) != OGMRIP_FORMAT_VOBSUB);
}

static OGMRipCharset
ogmrip_subp_options_dialog_get_charset (OGMRipSubpOptionsDialog *dialog)
{
  return gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->priv->charset_combo));
}

static void
ogmrip_subp_options_dialog_set_charset (OGMRipSubpOptionsDialog *dialog, OGMRipCharset charset)
{
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->charset_combo), charset);
}

static GType
ogmrip_subp_options_dialog_get_codec (OGMRipSubpOptionsDialog *dialog)
{
  return ogmrip_type_chooser_widget_get_active (GTK_COMBO_BOX (dialog->priv->codec_combo));
}

static void
ogmrip_subp_options_dialog_set_codec (OGMRipSubpOptionsDialog *dialog, GType codec)
{
  ogmrip_type_chooser_widget_set_active (GTK_COMBO_BOX (dialog->priv->codec_combo), codec);
}

static gboolean
ogmrip_subp_options_dialog_get_forced_only (OGMRipSubpOptionsDialog *dialog)
{
  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->forced_subs_check));
}

static void
ogmrip_subp_options_dialog_set_forced_only (OGMRipSubpOptionsDialog *dialog, gboolean forced_only)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->forced_subs_check), forced_only);
}

static OGMRipNewline
ogmrip_subp_options_dialog_get_newline (OGMRipSubpOptionsDialog *dialog)
{
  return gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->priv->newline_combo));
}

static void
ogmrip_subp_options_dialog_set_newline (OGMRipSubpOptionsDialog *dialog, OGMRipNewline newline)
{
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->newline_combo), newline);
}

static gboolean
ogmrip_subp_options_dialog_get_spell_check (OGMRipSubpOptionsDialog *dialog)
{
  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->spell_check));
}

static void
ogmrip_subp_options_dialog_set_spell_check (OGMRipSubpOptionsDialog *dialog, gboolean spell_check)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->spell_check), spell_check);
}

G_DEFINE_TYPE_WITH_CODE (OGMRipSubpOptionsDialog, ogmrip_subp_options_dialog, GTK_TYPE_DIALOG,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_SUBP_OPTIONS, ogmrip_subp_options_init))

static void
ogmrip_subp_options_dialog_class_init (OGMRipSubpOptionsDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_subp_options_dialog_get_property;
  gobject_class->set_property = ogmrip_subp_options_dialog_set_property;

  g_object_class_override_property (gobject_class, PROP_CHARSET, "charset");
  g_object_class_override_property (gobject_class, PROP_CODEC, "codec");
  g_object_class_override_property (gobject_class, PROP_FORCED_ONLY, "forced-only");
  g_object_class_override_property (gobject_class, PROP_NEWLINE, "newline");
  g_object_class_override_property (gobject_class, PROP_SPELL_CHECK, "spell-check");

  g_object_class_install_property (gobject_class, PROP_LABEL,
      g_param_spec_string ("label", "Label property", "Set label",
        NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LANGUAGE,
      g_param_spec_uint ("language", "Language property", "Set language",
        0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipSubpOptionsDialogPriv));
}

static void
ogmrip_subp_options_init (OGMRipSubpOptionsInterface *iface)
{
}

static void
ogmrip_subp_options_dialog_init (OGMRipSubpOptionsDialog *dialog)
{
  GError *error = NULL;

  GtkWidget *area, *grid1, *grid2, *label;
  GtkBuilder *builder;

  GtkTreeModel *model;
  GtkTreeIter iter;

  dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog,
      OGMRIP_TYPE_SUBP_OPTIONS_DIALOG, OGMRipSubpOptionsDialogPriv);

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_resource (builder, OGMRIP_UI_RES, &error))
  {
    g_warning ("Couldn't load builder file: %s", error->message);
    g_object_unref (builder);
    g_error_free (error);
    return;
  }

  gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Close"), GTK_RESPONSE_CLOSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Subtitles Options"));

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  grid1 = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid1), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid1), 6);
  gtk_container_add (GTK_CONTAINER (area), grid1);
  gtk_widget_show (grid1);

  label = gtk_label_new (_("<b>Track</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid1), label, 0, 0, 2, 1);
  gtk_widget_show (label);

  label = gtk_label_new_with_mnemonic (_("_Name:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid1), label, 0, 1, 1, 1);
  gtk_widget_set_margin_start (label, 12);
  gtk_widget_show (label);

  dialog->priv->label_entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (dialog->priv->label_entry), TRUE);
  gtk_grid_attach (GTK_GRID (grid1), dialog->priv->label_entry, 1, 1, 1, 1);
  gtk_widget_show (dialog->priv->label_entry);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->priv->label_entry);

  label = gtk_label_new_with_mnemonic (_("_Language:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid1), label, 0, 2, 1, 1);
  gtk_widget_set_margin_start (label, 12);
  gtk_widget_show (label);

  dialog->priv->lang_chooser = ogmrip_language_chooser_widget_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->priv->lang_chooser);
  gtk_grid_attach (GTK_GRID (grid1), dialog->priv->lang_chooser, 1, 2, 1, 1);
  gtk_widget_show (dialog->priv->lang_chooser);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (dialog->priv->lang_chooser));
  gtk_list_store_prepend (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      OGMRIP_LANGUAGE_STORE_NAME_COLUMN, _("None"),
      OGMRIP_LANGUAGE_STORE_CODE_COLUMN, 0, -1);

  dialog->priv->default_check = gtk_check_button_new_with_mnemonic (_("Use _profile settings"));
  gtk_grid_attach (GTK_GRID (grid1), dialog->priv->default_check, 0, 3, 2, 1);
  gtk_widget_set_margin_start (dialog->priv->default_check, 12);
  gtk_widget_show (dialog->priv->default_check);

  grid2 = gtk_builder_get_widget (builder, OGMRIP_UI_ROOT);
  gtk_container_set_border_width (GTK_CONTAINER (grid2), 0);
  gtk_widget_reparent (grid2, grid1);
  gtk_widget_show (grid2);

  gtk_container_child_set (GTK_CONTAINER (grid1), grid2, "left-attach", 0, "top-attach", 4, "width", 2, NULL);

  g_object_bind_property (dialog->priv->default_check, "active", grid2, "visible", G_BINDING_INVERT_BOOLEAN);

  dialog->priv->codec_combo = gtk_builder_get_widget (builder, "subp-codec-combo");
  ogmrip_type_chooser_widget_construct (GTK_COMBO_BOX (dialog->priv->codec_combo), OGMRIP_TYPE_SUBP_CODEC);

  dialog->priv->charset_combo = gtk_builder_get_widget (builder, "charset-combo");
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->charset_combo), 0);

  dialog->priv->newline_combo = gtk_builder_get_widget (builder, "newline-combo");
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->newline_combo), 0);

  dialog->priv->spell_check = gtk_builder_get_widget (builder, "spell-check");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->spell_check), FALSE);

  dialog->priv->forced_subs_check = gtk_builder_get_widget (builder, "forced-subs-check");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->forced_subs_check), TRUE);

  grid2 = gtk_builder_get_widget (builder, "subp-options-table");
  g_signal_connect (dialog->priv->codec_combo, "changed",
      G_CALLBACK (ogmrip_subp_options_dialog_codec_changed), grid2);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->default_check), TRUE);
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->codec_combo), 0);

  g_object_unref (builder);
}

static void
ogmrip_subp_options_dialog_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  OGMRipSubpOptionsDialog *dialog = OGMRIP_SUBP_OPTIONS_DIALOG (gobject);

  switch (prop_id) 
  {
    case PROP_CHARSET:
      g_value_set_uint (value, ogmrip_subp_options_dialog_get_charset (dialog));
      break;
    case PROP_CODEC:
      g_value_set_gtype (value, ogmrip_subp_options_dialog_get_codec (dialog));
      break;
    case PROP_FORCED_ONLY:
      g_value_set_boolean (value, ogmrip_subp_options_dialog_get_forced_only (dialog));
      break;
    case PROP_LABEL:
      g_value_set_string (value, ogmrip_subp_options_dialog_get_label (dialog));
      break;
    case PROP_LANGUAGE:
      g_value_set_uint (value, ogmrip_subp_options_dialog_get_language (dialog));
      break;
    case PROP_NEWLINE:
      g_value_set_uint (value, ogmrip_subp_options_dialog_get_newline (dialog));
      break;
    case PROP_SPELL_CHECK:
      g_value_set_boolean (value, ogmrip_subp_options_dialog_get_spell_check (dialog));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmrip_subp_options_dialog_set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipSubpOptionsDialog *dialog = OGMRIP_SUBP_OPTIONS_DIALOG (gobject);

  switch (prop_id) 
  {
    case PROP_CHARSET:
      ogmrip_subp_options_dialog_set_charset (dialog, g_value_get_uint (value));
      break;
    case PROP_CODEC:
      ogmrip_subp_options_dialog_set_codec (dialog, g_value_get_gtype (value));
      break;
    case PROP_FORCED_ONLY:
      ogmrip_subp_options_dialog_set_forced_only (dialog, g_value_get_boolean (value));
      break;
    case PROP_LABEL:
      ogmrip_subp_options_dialog_set_label (dialog, g_value_get_string (value));
      break;
    case PROP_LANGUAGE:
      ogmrip_subp_options_dialog_set_language (dialog, g_value_get_uint (value));
      break;
    case PROP_NEWLINE:
      ogmrip_subp_options_dialog_set_newline (dialog, g_value_get_uint (value));
      break;
    case PROP_SPELL_CHECK:
      ogmrip_subp_options_dialog_set_spell_check (dialog, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

GtkWidget *
ogmrip_subp_options_dialog_new (void)
{
  return g_object_new (OGMRIP_TYPE_SUBP_OPTIONS_DIALOG, NULL);
}

gboolean
ogmrip_subp_options_dialog_get_use_defaults (OGMRipSubpOptionsDialog *dialog)
{
  g_return_val_if_fail (OGMRIP_IS_SUBP_OPTIONS_DIALOG (dialog), FALSE);

  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->default_check));
}

void
ogmrip_subp_options_dialog_set_use_defaults (OGMRipSubpOptionsDialog *dialog, gboolean use_defaults)
{
  g_return_if_fail (OGMRIP_IS_SUBP_OPTIONS_DIALOG (dialog));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->default_check), use_defaults);
}

const gchar *
ogmrip_subp_options_dialog_get_label (OGMRipSubpOptionsDialog *dialog)
{
  return gtk_entry_get_text (GTK_ENTRY (dialog->priv->label_entry));
}

void
ogmrip_subp_options_dialog_set_label (OGMRipSubpOptionsDialog *dialog, const gchar *label)
{
  gtk_entry_set_text (GTK_ENTRY (dialog->priv->label_entry), label ? label : "");
}

gint
ogmrip_subp_options_dialog_get_language (OGMRipSubpOptionsDialog *dialog)
{
  return ogmrip_language_chooser_widget_get_active (OGMRIP_LANGUAGE_CHOOSER_WIDGET (dialog->priv->lang_chooser));
}

void
ogmrip_subp_options_dialog_set_language (OGMRipSubpOptionsDialog *dialog, gint lang)
{
  ogmrip_language_chooser_widget_set_active (OGMRIP_LANGUAGE_CHOOSER_WIDGET (dialog->priv->lang_chooser), lang);
}

