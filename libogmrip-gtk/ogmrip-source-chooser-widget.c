/* OGMRip - A wrapper library around libdvdread
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

/**
 * SECTION:ogmrip-source-chooser-widget
 * @title: OGMRipSourceChooserWidget
 * @include: ogmrip-source-chooser-widget.h
 * @short_description: Source chooser widget that can be embedded in other widgets
 */

#include "ogmrip-source-chooser-widget.h"

#include "ogmrip-helper.h"

#include <glib/gi18n-lib.h>

#define OGMRIP_AUDIO_CHOOSER_WIDGET_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_AUDIO_CHOOSER_WIDGET, OGMRipSourceChooserWidgetPriv))

#define OGMRIP_SUBP_CHOOSER_WIDGET_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_SUBP_CHOOSER_WIDGET, OGMRipSourceChooserWidgetPriv))

enum
{
  PROP_0,
  PROP_TITLE
};

enum
{
  TEXT_COLUMN,
  TYPE_COLUMN,
  LANG_COLUMN,
  SOURCE_COLUMN,
  NUM_COLUMNS
};

enum
{
  ROW_TYPE_FILE_SEP = OGMRIP_SOURCE_FILE + 1,
  ROW_TYPE_OTHER_SEP,
  ROW_TYPE_OTHER
};

struct _OGMRipSourceChooserWidgetPriv
{
  OGMDvdTitle *title;

  GtkTreePath *prev_path;
};

/*
 * GObject funcs
 */

static void ogmrip_audio_chooser_widget_dispose         (GObject      *gobject);
static void ogmrip_audio_chooser_widget_finalize        (GObject      *gobject);
static void ogmrip_audio_chooser_widget_get_property    (GObject      *gobject,
                                                         guint        property_id,
                                                         GValue       *value,
                                                         GParamSpec   *pspec);
static void ogmrip_audio_chooser_widget_set_property    (GObject      *gobject,
                                                         guint        property_id,
                                                         const GValue *value,
                                                         GParamSpec   *pspec);
static void ogmrip_audio_chooser_widget_changed         (GtkComboBox  *combo);
static void ogmrip_subp_chooser_widget_dispose          (GObject      *gobject);
static void ogmrip_subp_chooser_widget_finalize         (GObject      *gobject);
static void ogmrip_subp_chooser_widget_get_property     (GObject      *gobject,
                                                         guint        property_id,
                                                         GValue       *value,
                                                         GParamSpec   *pspec);
static void ogmrip_subp_chooser_widget_set_property     (GObject      *gobject,
                                                         guint        property_id,
                                                         const GValue *value,
                                                         GParamSpec   *pspec);
static void ogmrip_subp_chooser_widget_changed          (GtkComboBox  *combo);

/*
 * OGMRipSourceChooser funcs
 */

static void           ogmrip_source_chooser_init                   (OGMRipSourceChooserIface *iface);
static void           ogmrip_source_chooser_widget_set_title       (OGMRipSourceChooser      *chooser,
                                                                    OGMDvdTitle              *title);
static OGMDvdTitle *  ogmrip_source_chooser_widget_get_title       (OGMRipSourceChooser      *chooser);
static OGMRipSource * ogmrip_source_chooser_widget_get_active      (OGMRipSourceChooser      *chooser,
                                                                    OGMRipSourceType         *type);
static void           ogmrip_source_chooser_widget_select_language (OGMRipSourceChooser      *chooser,
                                                                    gint                     language);

/*
 * Internal functions
 */

static void     ogmrip_audio_chooser_widget_init             (OGMRipSourceChooserWidget      *chooser);
static void     ogmrip_audio_chooser_widget_class_init       (OGMRipSourceChooserWidgetClass *klass);

static void     ogmrip_subp_chooser_widget_init              (OGMRipSourceChooserWidget      *chooser);
static void     ogmrip_subp_chooser_widget_class_init        (OGMRipSourceChooserWidgetClass *klass);

static void     ogmrip_source_chooser_widget_construct       (OGMRipSourceChooserWidget      *chooser);
static void     ogmrip_source_chooser_widget_dispose         (OGMRipSourceChooserWidget      *chooser);
static void     ogmrip_source_chooser_widget_finalize        (OGMRipSourceChooserWidget      *chooser);
static void     ogmrip_source_chooser_widget_get_property    (OGMRipSourceChooser            *chooser,
                                                              guint                          property_id,
                                                              GValue                         *value,
                                                              GParamSpec                     *pspec);
static void     ogmrip_source_chooser_widget_set_property    (OGMRipSourceChooser            *chooser,
                                                              guint                          property_id,
                                                              const GValue                   *value,
                                                              GParamSpec                     *pspec);
static gboolean ogmrip_source_chooser_widget_sep_func        (GtkTreeModel                   *model,
                                                              GtkTreeIter                    *iter,
                                                              gpointer                       data);
static void     ogmrip_source_chooser_widget_clear           (OGMRipSourceChooserWidget      *chooser);
static void     ogmrip_source_chooser_widget_set_file        (OGMRipSourceChooserWidget      *chooser,
                                                              const gchar                    *filename,
                                                              gint                           language);
static gboolean ogmrip_source_chooser_widget_get_file_iter   (OGMRipSourceChooserWidget      *chooser,
                                                              GtkTreeModel                   **model,
                                                              GtkTreeIter                    *iter);
static void     ogmrip_source_chooser_widget_dialog_response (OGMRipSourceChooserWidget      *chooser,
                                                              gint                           response,
                                                              GtkWidget                      *dialog);

extern const gchar *ogmdvd_languages[][3];
extern const guint  ogmdvd_nlanguages;

static gpointer ogmrip_audio_chooser_widget_parent_class = NULL;
static gpointer ogmrip_subp_chooser_widget_parent_class = NULL;

static GtkWidget *
ogmrip_source_chooser_construct_file_chooser_dialog (gboolean audio)
{
  GtkWidget *dialog, *alignment, *hbox, *label, *combo;
  GtkFileFilter *filter;

  const gchar* const *langs;
  gchar *str, lang[2];
  guint i;

  dialog = gtk_file_chooser_dialog_new (NULL, NULL,
      GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
      GTK_RESPONSE_ACCEPT, GTK_RESPONSE_CANCEL, -1);

  g_signal_connect (dialog, "delete_event", G_CALLBACK (gtk_true), NULL);

  filter = gtk_file_filter_new ();

  if (audio)
  {
    gtk_window_set_title (GTK_WINDOW (dialog), _("Select an audio file"));

    gtk_file_filter_add_mime_type (filter, "audio/*");
    gtk_file_filter_add_mime_type (filter, "application/ogg");
  }
  else
  {
    gtk_window_set_title (GTK_WINDOW (dialog), _("Select a subps file"));

    gtk_file_filter_add_mime_type (filter, "text/*");
  }

  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

  alignment = gtk_alignment_new (1.0, 0.5, 0.0, 0.0);
  gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog), alignment);
  gtk_widget_show (alignment);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (alignment), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Language:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gtk_combo_box_text_new ();
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  g_object_set_data (G_OBJECT (dialog), "__ogmrip_source_chooser_widget_lang_combo__", combo);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  langs = g_get_language_names ();
  if (!langs[0] || strcmp (langs[0], "C") == 0 || strcmp (langs[0], "POSIX") == 0)
  {
    lang[0] = 'e';
    lang[1] = 'n';
  }
  else
  {
    lang[0] = langs[0][0];
    lang[1] = langs[0][1];
  }

  for (i = 2; i < ogmdvd_nlanguages; i++)
  {
    str = g_strdup_printf ("%s (%s)", ogmdvd_languages[i][OGMDVD_LANGUAGE_NAME],
        ogmdvd_languages[i][OGMDVD_LANGUAGE_ISO639_1]);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), str);
    g_free (str);

    if (strncmp (ogmdvd_languages[i][OGMDVD_LANGUAGE_ISO639_1], lang, 2) == 0)
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i - 2);
  }

  return dialog;
}

static void
ogmrip_audio_chooser_widget_class_intern_init (gpointer klass)
{
  ogmrip_audio_chooser_widget_parent_class = g_type_class_peek_parent (klass);
  ogmrip_audio_chooser_widget_class_init ((OGMRipSourceChooserWidgetClass*) klass);
}

GType
ogmrip_audio_chooser_widget_get_type (void)
{
  static GType audio_chooser_widget_type = 0;

  if (!audio_chooser_widget_type)
  {
    const GInterfaceInfo g_implement_interface_info =
    {
      (GInterfaceInitFunc) ogmrip_source_chooser_init,
      NULL,
      NULL
    };

    audio_chooser_widget_type = g_type_register_static_simple (GTK_TYPE_COMBO_BOX,
        "OGMRipAudioChooserWidget",
        sizeof (OGMRipSourceChooserWidgetClass),
        (GClassInitFunc) ogmrip_audio_chooser_widget_class_intern_init,
        sizeof (OGMRipSourceChooserWidget),
        (GInstanceInitFunc)ogmrip_audio_chooser_widget_init,
        (GTypeFlags) 0);

    g_type_add_interface_static (audio_chooser_widget_type,
        OGMRIP_TYPE_SOURCE_CHOOSER, &g_implement_interface_info);
  }

  return audio_chooser_widget_type;
}

static void
ogmrip_audio_chooser_widget_class_init (OGMRipSourceChooserWidgetClass *klass)
{
  GObjectClass *object_class;
  GtkComboBoxClass *combo_box_class;

  object_class = (GObjectClass *) klass;
  object_class->dispose = ogmrip_audio_chooser_widget_dispose;
  object_class->finalize = ogmrip_audio_chooser_widget_finalize;
  object_class->get_property = ogmrip_audio_chooser_widget_get_property;
  object_class->set_property = ogmrip_audio_chooser_widget_set_property;

  combo_box_class = (GtkComboBoxClass *) klass;
  combo_box_class->changed = ogmrip_audio_chooser_widget_changed;

  g_object_class_override_property (object_class, PROP_TITLE, "title");

  g_type_class_add_private (klass, sizeof (OGMRipSourceChooserWidgetPriv));
}

static void
ogmrip_audio_chooser_widget_init (OGMRipSourceChooserWidget *chooser)
{
  chooser->priv = OGMRIP_AUDIO_CHOOSER_WIDGET_GET_PRIVATE (chooser);

  ogmrip_source_chooser_widget_construct (chooser);
}

static void
ogmrip_audio_chooser_widget_dispose (GObject *gobject)
{
  ogmrip_source_chooser_widget_dispose (OGMRIP_AUDIO_CHOOSER_WIDGET (gobject));

  (*G_OBJECT_CLASS (ogmrip_audio_chooser_widget_parent_class)->dispose) (gobject);
}

static void
ogmrip_audio_chooser_widget_finalize (GObject *gobject)
{
  ogmrip_source_chooser_widget_finalize (OGMRIP_AUDIO_CHOOSER_WIDGET (gobject));

  (*G_OBJECT_CLASS (ogmrip_audio_chooser_widget_parent_class)->finalize) (gobject);
}

static void
ogmrip_audio_chooser_widget_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  ogmrip_source_chooser_widget_get_property (OGMRIP_SOURCE_CHOOSER (gobject), property_id, value, pspec);
}

static void
ogmrip_audio_chooser_widget_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  ogmrip_source_chooser_widget_set_property (OGMRIP_SOURCE_CHOOSER (gobject), property_id, value, pspec);
}

static void
ogmrip_audio_chooser_widget_changed (GtkComboBox *combo)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (combo, &iter))
  {
    OGMRipSourceChooserWidget *chooser;
    GtkTreeModel *model;
    gint type;

    chooser = OGMRIP_AUDIO_CHOOSER_WIDGET (combo);

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
    gtk_tree_model_get (model, &iter, TYPE_COLUMN, &type, -1);

    if (type == ROW_TYPE_OTHER)
    {
      GtkWidget *dialog, *toplevel;
      gint response;

      dialog = ogmrip_source_chooser_construct_file_chooser_dialog (TRUE);

      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (chooser));
      if (gtk_widget_is_toplevel (toplevel) && GTK_IS_WINDOW (toplevel))
        gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (toplevel));

      response = gtk_dialog_run (GTK_DIALOG (dialog));
      ogmrip_source_chooser_widget_dialog_response (chooser, response, dialog);
      gtk_widget_destroy (dialog);
    }
    else
    {
      if (chooser->priv->prev_path)
        gtk_tree_path_free (chooser->priv->prev_path);
      chooser->priv->prev_path = gtk_tree_model_get_path (model, &iter);
    }
  }
}

static void
ogmrip_subp_chooser_widget_class_intern_init (gpointer klass)
{
  ogmrip_subp_chooser_widget_parent_class = g_type_class_peek_parent (klass);
  ogmrip_subp_chooser_widget_class_init ((OGMRipSourceChooserWidgetClass*) klass);
}

GType
ogmrip_subp_chooser_widget_get_type (void)
{
  static GType subp_chooser_widget_type = 0;

  if (!subp_chooser_widget_type)
  {
    const GInterfaceInfo g_implement_interface_info =
    {
      (GInterfaceInitFunc) ogmrip_source_chooser_init,
      NULL,
      NULL
    };

    subp_chooser_widget_type = g_type_register_static_simple (GTK_TYPE_COMBO_BOX,
        "OGMRipSubpChooserWidget",
        sizeof (OGMRipSourceChooserWidgetClass),
        (GClassInitFunc) ogmrip_subp_chooser_widget_class_intern_init,
        sizeof (OGMRipSourceChooserWidget),
        (GInstanceInitFunc)ogmrip_subp_chooser_widget_init,
        (GTypeFlags) 0);

    g_type_add_interface_static (subp_chooser_widget_type,
        OGMRIP_TYPE_SOURCE_CHOOSER, &g_implement_interface_info);
  }

  return subp_chooser_widget_type;
}

static void
ogmrip_subp_chooser_widget_class_init (OGMRipSourceChooserWidgetClass *klass)
{
  GObjectClass *object_class;
  GtkComboBoxClass *combo_box_class;

  object_class = (GObjectClass *) klass;
  object_class->dispose = ogmrip_subp_chooser_widget_dispose;
  object_class->finalize = ogmrip_subp_chooser_widget_finalize;
  object_class->get_property = ogmrip_subp_chooser_widget_get_property;
  object_class->set_property = ogmrip_subp_chooser_widget_set_property;

  combo_box_class = (GtkComboBoxClass *) klass;
  combo_box_class->changed = ogmrip_subp_chooser_widget_changed;

  g_object_class_override_property (object_class, PROP_TITLE, "title");

  g_type_class_add_private (klass, sizeof (OGMRipSourceChooserWidgetPriv));
}

static void
ogmrip_subp_chooser_widget_init (OGMRipSourceChooserWidget *chooser)
{
  chooser->priv = OGMRIP_SUBP_CHOOSER_WIDGET_GET_PRIVATE (chooser);

  ogmrip_source_chooser_widget_construct (chooser);
}

static void
ogmrip_subp_chooser_widget_dispose (GObject *gobject)
{
  ogmrip_source_chooser_widget_dispose (OGMRIP_SUBP_CHOOSER_WIDGET (gobject));

  (*G_OBJECT_CLASS (ogmrip_subp_chooser_widget_parent_class)->dispose) (gobject);
}

static void
ogmrip_subp_chooser_widget_finalize (GObject *gobject)
{
  ogmrip_source_chooser_widget_finalize (OGMRIP_SUBP_CHOOSER_WIDGET (gobject));

  (*G_OBJECT_CLASS (ogmrip_subp_chooser_widget_parent_class)->finalize) (gobject);
}

static void
ogmrip_subp_chooser_widget_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  ogmrip_source_chooser_widget_get_property (OGMRIP_SOURCE_CHOOSER (gobject), property_id, value, pspec);
}

static void
ogmrip_subp_chooser_widget_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  ogmrip_source_chooser_widget_set_property (OGMRIP_SOURCE_CHOOSER (gobject), property_id, value, pspec);
}

static void
ogmrip_subp_chooser_widget_changed (GtkComboBox *combo)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (combo, &iter))
  {
    OGMRipSourceChooserWidget *chooser;
    GtkTreeModel *model;
    gint type;

    chooser = OGMRIP_SUBP_CHOOSER_WIDGET (combo);

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
    gtk_tree_model_get (model, &iter, TYPE_COLUMN, &type, -1);

    if (type == ROW_TYPE_OTHER)
    {
      GtkWidget *dialog, *toplevel;
      gint response;

      dialog = ogmrip_source_chooser_construct_file_chooser_dialog (FALSE);

      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (chooser));
      if (gtk_widget_is_toplevel (toplevel) && GTK_IS_WINDOW (toplevel))
        gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (toplevel));

      response = gtk_dialog_run (GTK_DIALOG (dialog));
      ogmrip_source_chooser_widget_dialog_response (chooser, response, dialog);
      gtk_widget_destroy (dialog);
    }
    else
    {
      if (chooser->priv->prev_path)
        gtk_tree_path_free (chooser->priv->prev_path);
      chooser->priv->prev_path = gtk_tree_model_get_path (model, &iter);
    }
  }
}

static void
ogmrip_source_chooser_init (OGMRipSourceChooserIface *iface)
{
  iface->get_active = ogmrip_source_chooser_widget_get_active;
  iface->select_language = ogmrip_source_chooser_widget_select_language;
}

static void
ogmrip_source_chooser_widget_construct (OGMRipSourceChooserWidget *chooser)
{
  GtkCellRenderer *cell;
  GtkListStore *store;

  store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_POINTER);
  gtk_combo_box_set_model (GTK_COMBO_BOX (chooser), GTK_TREE_MODEL (store));
  g_object_unref (store);

  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (chooser),
      ogmrip_source_chooser_widget_sep_func, NULL, NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser), cell, "text", TEXT_COLUMN, NULL);
}

static void
ogmrip_source_chooser_widget_dispose (OGMRipSourceChooserWidget *chooser)
{
  ogmrip_source_chooser_widget_clear (chooser);

  if (chooser->priv->title)
    ogmdvd_title_unref (chooser->priv->title);
  chooser->priv->title = NULL;
}

static void
ogmrip_source_chooser_widget_finalize (OGMRipSourceChooserWidget *chooser)
{
  if (chooser->priv->prev_path)
    gtk_tree_path_free (chooser->priv->prev_path);
  chooser->priv->prev_path = NULL;
}

static void
ogmrip_source_chooser_widget_get_property (OGMRipSourceChooser *chooser, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id) 
  {
    case PROP_TITLE:
      g_value_set_pointer (value, ogmrip_source_chooser_widget_get_title (chooser));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (chooser, property_id, pspec);
      break;
  }
}

static void
ogmrip_source_chooser_widget_set_property (OGMRipSourceChooser *chooser, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id) 
  {
    case PROP_TITLE:
      ogmrip_source_chooser_widget_set_title (chooser, g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (chooser, property_id, pspec);
      break;
  }
}

static gboolean
ogmrip_source_chooser_widget_sep_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  gint type = OGMRIP_SOURCE_INVALID;

  gtk_tree_model_get (model, iter, TYPE_COLUMN, &type, -1);

  return (type == ROW_TYPE_FILE_SEP || type == ROW_TYPE_OTHER_SEP);
}

static void
ogmrip_source_chooser_widget_add_audio_streams (OGMRipSourceChooserWidget *chooser, GtkTreeModel *model, OGMDvdTitle *title)
{
  GtkTreeIter iter;
  OGMDvdAudioStream *astream;
  gint aid, naid, channels, format, lang, content, bitrate;
  gchar *str;

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      TEXT_COLUMN, _("No audio"), TYPE_COLUMN, OGMRIP_SOURCE_NONE, LANG_COLUMN, -1, SOURCE_COLUMN, NULL, -1);

  naid = ogmdvd_title_get_n_audio_streams (title);
  for (aid = 0; aid < naid; aid++)
  {
    astream = ogmdvd_title_get_nth_audio_stream (title, aid);
    if (astream)
    {
      ogmdvd_stream_ref (OGMDVD_STREAM (astream));

      bitrate = ogmdvd_audio_stream_get_bitrate (astream);
      channels = ogmdvd_audio_stream_get_channels (astream);
      content = ogmdvd_audio_stream_get_content (astream);
      format = ogmdvd_audio_stream_get_format (astream);
      lang = ogmdvd_audio_stream_get_language (astream);

      if (content > 0)
      {
        if (bitrate > 0)
          str = g_strdup_printf ("%s %02d: %s (%s, %s, %s, %d kbps)", _("Track"), aid + 1, 
              ogmdvd_get_audio_content_label (content), ogmdvd_get_language_label (lang), 
              ogmdvd_get_audio_format_label (format), ogmdvd_get_audio_channels_label (channels),
              bitrate / 1000);
        else
          str = g_strdup_printf ("%s %02d: %s (%s, %s, %s)", _("Track"), aid + 1, 
              ogmdvd_get_audio_content_label (content), ogmdvd_get_language_label (lang), 
              ogmdvd_get_audio_format_label (format), ogmdvd_get_audio_channels_label (channels));
      }
      else
      {
        if (bitrate > 0)
          str = g_strdup_printf ("%s %02d (%s, %s, %s, %d kbps)", _("Track"), aid + 1, 
              ogmdvd_get_language_label (lang), ogmdvd_get_audio_format_label (format), 
              ogmdvd_get_audio_channels_label (channels), bitrate / 1000);
        else
          str = g_strdup_printf ("%s %02d (%s, %s, %s)", _("Track"), aid + 1, 
              ogmdvd_get_language_label (lang), ogmdvd_get_audio_format_label (format), 
              ogmdvd_get_audio_channels_label (channels));
      }

      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          TEXT_COLUMN, str, TYPE_COLUMN, OGMRIP_SOURCE_STREAM, LANG_COLUMN, lang, SOURCE_COLUMN, astream, -1);

      g_free (str);
    }
  }
}

static void
ogmrip_source_chooser_widget_add_subp_streams (OGMRipSourceChooserWidget *chooser, GtkTreeModel *model, OGMDvdTitle *title)
{
  GtkTreeIter iter;
  OGMDvdSubpStream *sstream;
  gint nsid, sid, lang, content;
  gchar *str;

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      TEXT_COLUMN, _("No subp"), TYPE_COLUMN, OGMRIP_SOURCE_NONE, LANG_COLUMN, -1, SOURCE_COLUMN, NULL, -1);

  nsid = ogmdvd_title_get_n_subp_streams (title);
  for (sid = 0; sid < nsid; sid++)
  {
    sstream = ogmdvd_title_get_nth_subp_stream (title, sid);
    if (sstream)
    {
      ogmdvd_stream_ref (OGMDVD_STREAM (sstream));

      lang = ogmdvd_subp_stream_get_language (sstream);
      content = ogmdvd_subp_stream_get_content (sstream);

      if (content > 0)
        str = g_strdup_printf ("%s %02d: %s (%s)", _("Subtitle"), sid + 1, 
            ogmdvd_get_subp_content_label (content), ogmdvd_get_language_label (lang));
      else
        str = g_strdup_printf ("%s %02d (%s)", _("Subtitle"), sid + 1, 
            ogmdvd_get_language_label (lang));

      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          TEXT_COLUMN, str, TYPE_COLUMN, OGMRIP_SOURCE_STREAM, LANG_COLUMN, lang, SOURCE_COLUMN, sstream, -1);

      g_free (str);
    }
  }
}

static void
ogmrip_source_chooser_widget_set_title (OGMRipSourceChooser *chooser, OGMDvdTitle *title)
{
  OGMRipSourceChooserWidget *source_chooser;

  if (OGMRIP_IS_AUDIO_CHOOSER_WIDGET (chooser))
    source_chooser = OGMRIP_AUDIO_CHOOSER_WIDGET (chooser);
  else
    source_chooser = OGMRIP_SUBP_CHOOSER_WIDGET (chooser);

  if (source_chooser->priv->title != title)
  {
    if (title)
      ogmdvd_title_ref (title);
    if (source_chooser->priv->title)
      ogmdvd_title_unref (source_chooser->priv->title);
    source_chooser->priv->title = title;

    ogmrip_source_chooser_widget_clear (source_chooser);

    if (title)
    {
      GtkTreeModel *model;
      GtkTreeIter iter;

      model = gtk_combo_box_get_model (GTK_COMBO_BOX (source_chooser));

      if (OGMRIP_IS_AUDIO_CHOOSER_WIDGET (source_chooser))
        ogmrip_source_chooser_widget_add_audio_streams (source_chooser, model, title);
      else
        ogmrip_source_chooser_widget_add_subp_streams (source_chooser, model, title);

      if (gtk_tree_model_iter_n_children (model, NULL) > 0 )
      {
        gtk_list_store_append (GTK_LIST_STORE (model), &iter);
        gtk_list_store_set (GTK_LIST_STORE (model), &iter,
            TEXT_COLUMN, NULL, TYPE_COLUMN, ROW_TYPE_OTHER_SEP, LANG_COLUMN, -1, SOURCE_COLUMN, NULL, -1);
      }

      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          TEXT_COLUMN, _("Other..."), TYPE_COLUMN, ROW_TYPE_OTHER, LANG_COLUMN, -1, SOURCE_COLUMN, NULL, -1);
    }
  }
}

static void
ogmrip_source_chooser_widget_clear (OGMRipSourceChooserWidget *chooser)
{ 
  OGMRipSource *source;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint type;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    do
    {
      gtk_tree_model_get (model, &iter, TYPE_COLUMN, &type, SOURCE_COLUMN, &source, -1);

      if (type == OGMRIP_SOURCE_FILE)
        ogmrip_file_unref (OGMRIP_FILE (source)); 
      else if (type == OGMRIP_SOURCE_STREAM)
        ogmdvd_stream_unref (OGMDVD_STREAM (source));
    }
    while (gtk_list_store_remove (GTK_LIST_STORE (model), &iter));
  }
}

static gboolean
ogmrip_source_chooser_widget_get_file_iter (OGMRipSourceChooserWidget *chooser, GtkTreeModel **model, GtkTreeIter *iter)
{
  gint type, pos = 0;

  *model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
  if (!gtk_tree_model_get_iter_first (*model, iter))
    return FALSE;

  do
  {
    gtk_tree_model_get (*model, iter, TYPE_COLUMN, &type, -1);
    if (type != OGMRIP_SOURCE_STREAM && type != OGMRIP_SOURCE_NONE)
      break;
    pos ++;
  }
  while (gtk_tree_model_iter_next (*model, iter));

  if (type != ROW_TYPE_FILE_SEP)
  {
    gtk_list_store_insert (GTK_LIST_STORE (*model), iter, pos);
    gtk_list_store_set (GTK_LIST_STORE (*model), iter,
        TEXT_COLUMN, NULL, TYPE_COLUMN, ROW_TYPE_FILE_SEP, LANG_COLUMN, -1, SOURCE_COLUMN, NULL, -1);
    pos ++;
  }
  else
  {
    gtk_tree_model_iter_next (*model, iter);
    gtk_tree_model_get (*model, iter, TYPE_COLUMN, &type, -1);
  }

  if (type != OGMRIP_SOURCE_FILE)
    gtk_list_store_insert (GTK_LIST_STORE (*model), iter, pos);

  return TRUE;
}

static void
ogmrip_source_chooser_widget_dialog_response (OGMRipSourceChooserWidget *chooser, gint response, GtkWidget *dialog)
{
  if (response == GTK_RESPONSE_ACCEPT)
  {
    GtkWidget *combo;
    const gchar *str;
    gchar *filename;
    gint lang;

    combo = g_object_get_data (G_OBJECT (dialog), "__ogmrip_source_chooser_widget_lang_combo__");
    lang = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

    lang = CLAMP (lang, 0, ogmdvd_nlanguages - 3) + 2;
    str = ogmdvd_languages[lang][OGMDVD_LANGUAGE_ISO639_1];
    lang = (str[0] << 8) | str[1];

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    ogmrip_source_chooser_widget_set_file (chooser, filename, lang);
    g_free (filename);
  }
  else
  {
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
    if (gtk_tree_model_get_iter (model, &iter, chooser->priv->prev_path))
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (chooser), &iter);
  }

  gtk_widget_set_sensitive (GTK_WIDGET (chooser), TRUE);
  gtk_widget_hide (dialog);
}

static void
ogmrip_source_chooser_widget_set_file (OGMRipSourceChooserWidget *chooser, const gchar *filename, gint language)
{
  GError *error = NULL;

  OGMRipFile *file;
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (OGMRIP_IS_AUDIO_CHOOSER_WIDGET (chooser))
    file = ogmrip_audio_file_new (filename, &error);
  else
    file = ogmrip_subp_file_new (filename, &error);

  if (file)
  {
    ogmrip_file_set_language (file, language);

    if (ogmrip_source_chooser_widget_get_file_iter (chooser, &model, &iter))
    {
      OGMRipFile *old_file;
      gchar *old_filename = NULL;

      gtk_tree_model_get (model, &iter, SOURCE_COLUMN, &old_file, -1);
      if (old_file)
        old_filename = ogmrip_file_get_filename (old_file);

      if (!old_filename || strcmp (filename, old_filename) != 0)
      {
        gchar *basename;

        if (old_file)
          ogmrip_file_unref (old_file);

        basename = g_path_get_basename (filename);
        gtk_list_store_set (GTK_LIST_STORE (model), &iter, TEXT_COLUMN, basename,
            TYPE_COLUMN, OGMRIP_SOURCE_FILE, LANG_COLUMN, language, SOURCE_COLUMN, file, -1);
        g_free (basename);

        file = NULL;
      }
    }

    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (chooser), &iter);
  }
  else
  {
    GtkWidget *toplevel;

    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (chooser));
    ogmrip_message_dialog (GTK_WINDOW (toplevel), GTK_MESSAGE_ERROR, "%s",
        error ? error->message : _("Unknown error while opening file"));

    if (chooser->priv->prev_path)
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
      if (gtk_tree_model_get_iter (model, &iter, chooser->priv->prev_path))
        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (chooser), &iter);
    }
  }

  if (file)
    ogmrip_file_unref (file);
}

static OGMDvdTitle *
ogmrip_source_chooser_widget_get_title (OGMRipSourceChooser *chooser)
{
  if (OGMRIP_IS_AUDIO_CHOOSER_WIDGET (chooser))
    return OGMRIP_AUDIO_CHOOSER_WIDGET (chooser)->priv->title;
  else
    return OGMRIP_SUBP_CHOOSER_WIDGET (chooser)->priv->title;
}

static OGMRipSource *
ogmrip_source_chooser_widget_get_active (OGMRipSourceChooser *chooser, OGMRipSourceType *type)
{
  OGMRipSource *source;
  GtkTreeModel *model;
  GtkTreeIter iter;

  gint row_type = 0;

  if (type)
    *type = OGMRIP_SOURCE_INVALID;

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (chooser), &iter))
    return NULL;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
  gtk_tree_model_get (model, &iter, TYPE_COLUMN, &row_type, SOURCE_COLUMN, &source, -1);

  if (row_type != OGMRIP_SOURCE_FILE && row_type != OGMRIP_SOURCE_STREAM)
    return NULL;

  if (type)
    *type = row_type;

  return source;
}

static void
ogmrip_source_chooser_widget_select_language (OGMRipSourceChooser *chooser, gint language)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  gboolean found = FALSE;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    gint lang;

    do
    {
      gtk_tree_model_get (model, &iter, LANG_COLUMN, &lang, -1);
      if (language == lang)
        found = TRUE;
    }
    while (!found && gtk_tree_model_iter_next (model, &iter));
  }

  if (found)
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (chooser), &iter);
}

/**
 * ogmrip_audio_chooser_widget_new:
 *
 * Creates a new #OGMRipSourceChooserWidget for audio streams.
 *
 * Returns: The new #OGMRipSourceChooserWidget
 */
GtkWidget *
ogmrip_audio_chooser_widget_new (void)
{
  return g_object_new (OGMRIP_TYPE_AUDIO_CHOOSER_WIDGET, NULL);
}

/**
 * ogmrip_subp_chooser_widget_new:
 *
 * Creates a new #OGMRipSourceChooserWidget for subps streams.
 *
 * Returns: The new #OGMRipSourceChooserWidget
 */
GtkWidget *
ogmrip_subp_chooser_widget_new (void)
{
  return g_object_new (OGMRIP_TYPE_SUBP_CHOOSER_WIDGET, NULL);
}

