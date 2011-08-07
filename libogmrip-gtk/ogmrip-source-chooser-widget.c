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

/**
 * SECTION:ogmrip-source-chooser-widget
 * @title: OGMRipSourceChooserWidget
 * @include: ogmrip-source-chooser-widget.h
 * @short_description: Source chooser widget that can be embedded in other widgets
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-source-chooser-widget.h"
#include "ogmrip-file-chooser-dialog.h"

#include "ogmrip-helper.h"

#include <glib/gi18n-lib.h>

#define OGMRIP_SOURCE_CHOOSER_WIDGET_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_SOURCE_CHOOSER_WIDGET, OGMRipSourceChooserWidgetPriv))

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_DIALOG
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
  GtkWidget *dialog;
  GtkWidget *options;

  GtkTreePath *prev_path;
};

static void ogmrip_source_chooser_init (OGMRipSourceChooserInterface   *iface);
static void ogmrip_source_chooser_widget_constructed  (GObject      *gobject);
static void ogmrip_source_chooser_widget_dispose      (GObject      *gobject);
static void ogmrip_source_chooser_widget_finalize     (GObject      *gobject);
static void ogmrip_source_chooser_widget_get_property (GObject      *gobject,
                                                       guint        property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);
static void ogmrip_source_chooser_widget_set_property (GObject      *gobject,
                                                       guint        property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
static void ogmrip_source_chooser_widget_destroy      (GtkObject    *object);
static void ogmrip_source_chooser_widget_changed      (GtkComboBox  *combo);

static gboolean
ogmrip_source_chooser_widget_sep_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  gint type = OGMRIP_SOURCE_INVALID;

  gtk_tree_model_get (model, iter, TYPE_COLUMN, &type, -1);

  return (type == ROW_TYPE_FILE_SEP || type == ROW_TYPE_OTHER_SEP);
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

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      TEXT_COLUMN, _("No stream"), TYPE_COLUMN, OGMRIP_SOURCE_NONE, LANG_COLUMN, -1, SOURCE_COLUMN, NULL, -1);

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      TEXT_COLUMN, NULL, TYPE_COLUMN, ROW_TYPE_OTHER_SEP, LANG_COLUMN, -1, SOURCE_COLUMN, NULL, -1);

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
      TEXT_COLUMN, _("Other..."), TYPE_COLUMN, ROW_TYPE_OTHER, LANG_COLUMN, -1, SOURCE_COLUMN, NULL, -1);

}

static void
ogmrip_source_chooser_widget_set_title (OGMRipSourceChooserWidget *chooser, OGMDvdTitle *title)
{
  if (chooser->priv->title != title)
  {
    if (title)
      ogmdvd_title_ref (title);
    if (chooser->priv->title)
      ogmdvd_title_unref (chooser->priv->title);
    chooser->priv->title = title;

    ogmrip_source_chooser_widget_clear (chooser);
  }
}

static gboolean
ogmrip_source_chooser_widget_get_stream_iter (OGMRipSourceChooserWidget *chooser, GtkTreeIter *iter)
{
  GtkTreeModel *model;
  GtkTreeIter sibling;
  gint type;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
  if (!gtk_tree_model_get_iter_first (model, &sibling))
    return FALSE;

  do
  {
    gtk_tree_model_get (model, &sibling, TYPE_COLUMN, &type, -1);
    if (type != OGMRIP_SOURCE_STREAM && type != OGMRIP_SOURCE_NONE)
      break;
  }
  while (gtk_tree_model_iter_next (model, &sibling));

  gtk_list_store_insert_before (GTK_LIST_STORE (model), iter, &sibling);

  return TRUE;
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
ogmrip_source_chooser_widget_set_file (OGMRipSourceChooserWidget *chooser, OGMRipFile *file)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (ogmrip_source_chooser_widget_get_file_iter (chooser, &model, &iter))
  {
    OGMRipFile *old_file;
    gchar *filename, *basename;

    gtk_tree_model_get (model, &iter, SOURCE_COLUMN, &old_file, -1);
    if (old_file)
      ogmrip_file_unref (old_file);

    filename = ogmrip_file_get_filename (file);
    basename = g_path_get_basename (filename);
    g_free (filename);

    gtk_list_store_set (GTK_LIST_STORE (model), &iter, TEXT_COLUMN, basename,
        TYPE_COLUMN, OGMRIP_SOURCE_FILE, LANG_COLUMN, ogmrip_file_get_language (file), SOURCE_COLUMN, file, -1);
    g_free (basename);
  }

  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (chooser), &iter);
}

static void
ogmrip_source_chooser_widget_changed (GtkComboBox *combo)
{
  OGMRipSourceChooserWidget *chooser = OGMRIP_SOURCE_CHOOSER_WIDGET (combo);
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (chooser), &iter))
  {
    GtkTreeModel *model;
    gint type;

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
    gtk_tree_model_get (model, &iter, TYPE_COLUMN, &type, -1);

    if (type == ROW_TYPE_OTHER && chooser->priv->dialog)
    {
      GtkWidget *toplevel;
      gboolean select_prev = TRUE;

      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (chooser));
      if (gtk_widget_is_toplevel (toplevel) && GTK_IS_WINDOW (toplevel))
        gtk_window_set_transient_for (GTK_WINDOW (chooser->priv->dialog), GTK_WINDOW (toplevel));

      if (gtk_dialog_run (GTK_DIALOG (chooser->priv->dialog)) == GTK_RESPONSE_ACCEPT)
      {
        GError *error = NULL;
        OGMRipFile *file;

        file = ogmrip_file_chooser_dialog_get_file (OGMRIP_FILE_CHOOSER_DIALOG (chooser->priv->dialog), &error);
        select_prev = file != NULL;

        if (file)
          ogmrip_source_chooser_widget_set_file (chooser, file);
        else
        {
          GtkWidget *toplevel;

          toplevel = gtk_widget_get_toplevel (GTK_WIDGET (chooser));
          ogmrip_message_dialog (GTK_WINDOW (toplevel), GTK_MESSAGE_ERROR, "%s",
              error ? error->message : _("Unknown error while opening file"));

          if (error)
            g_error_free (error);
        }
      }

      if (select_prev)
      {
        GtkTreeModel *model;
        GtkTreeIter iter;

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));
        if (chooser->priv->prev_path && gtk_tree_model_get_iter (model, &iter, chooser->priv->prev_path))
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (chooser), &iter);
        else
          gtk_combo_box_set_active (GTK_COMBO_BOX (chooser), -1);
      }
    }
    else
    {
      if (chooser->priv->prev_path)
        gtk_tree_path_free (chooser->priv->prev_path);
      chooser->priv->prev_path = gtk_tree_model_get_path (model, &iter);
    }
  }
}

G_DEFINE_TYPE_WITH_CODE (OGMRipSourceChooserWidget, ogmrip_source_chooser_widget, GTK_TYPE_COMBO_BOX,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_SOURCE_CHOOSER, ogmrip_source_chooser_init))

static void
ogmrip_source_chooser_widget_class_init (OGMRipSourceChooserWidgetClass *klass)
{
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;
  GtkComboBoxClass *combo_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_source_chooser_widget_constructed;
  gobject_class->dispose = ogmrip_source_chooser_widget_dispose;
  gobject_class->finalize = ogmrip_source_chooser_widget_finalize;
  gobject_class->get_property = ogmrip_source_chooser_widget_get_property;
  gobject_class->set_property = ogmrip_source_chooser_widget_set_property;

  object_class = GTK_OBJECT_CLASS (klass);
  object_class->destroy = ogmrip_source_chooser_widget_destroy;

  combo_class = GTK_COMBO_BOX_CLASS (klass);
  combo_class->changed = ogmrip_source_chooser_widget_changed;

  g_object_class_override_property (gobject_class, PROP_TITLE, "title");

  g_object_class_install_property (gobject_class, PROP_DIALOG,
      g_param_spec_object ("dialog", "dialog", "dialog",
        OGMRIP_TYPE_FILE_CHOOSER_DIALOG, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipSourceChooserWidgetPriv));
}

static void
ogmrip_source_chooser_widget_init (OGMRipSourceChooserWidget *chooser)
{
  GtkCellRenderer *cell;
  GtkListStore *store;

  chooser->priv = OGMRIP_SOURCE_CHOOSER_WIDGET_GET_PRIVATE (chooser);

  store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_POINTER);
  gtk_combo_box_set_model (GTK_COMBO_BOX (chooser), GTK_TREE_MODEL (store));
  g_object_unref (store);

  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (chooser),
      ogmrip_source_chooser_widget_sep_func, NULL, NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser), cell, "text", TEXT_COLUMN, NULL);

  ogmrip_source_chooser_widget_clear (chooser);
}

static void
ogmrip_source_chooser_widget_constructed (GObject *gobject)
{
  OGMRipSourceChooserWidget *chooser = OGMRIP_SOURCE_CHOOSER_WIDGET (gobject);

  if (chooser->priv->dialog)
    g_signal_connect (chooser->priv->dialog, "delete-event",
        G_CALLBACK (gtk_widget_hide_on_delete), NULL);

  G_OBJECT_CLASS (ogmrip_source_chooser_widget_parent_class)->constructed (gobject);
}

static void
ogmrip_source_chooser_widget_dispose (GObject *gobject)
{
  OGMRipSourceChooserWidget *chooser = OGMRIP_SOURCE_CHOOSER_WIDGET (gobject);

  ogmrip_source_chooser_widget_clear (chooser);

  if (chooser->priv->title)
    ogmdvd_title_unref (chooser->priv->title);
  chooser->priv->title = NULL;

  G_OBJECT_CLASS (ogmrip_source_chooser_widget_parent_class)->dispose (gobject);
}

static void
ogmrip_source_chooser_widget_finalize (GObject *gobject)
{
  OGMRipSourceChooserWidget *chooser = OGMRIP_SOURCE_CHOOSER_WIDGET (gobject);

  if (chooser->priv->prev_path)
    gtk_tree_path_free (chooser->priv->prev_path);
  chooser->priv->prev_path = NULL;

  G_OBJECT_CLASS (ogmrip_source_chooser_widget_parent_class)->finalize (gobject);
}

static void
ogmrip_source_chooser_widget_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipSourceChooserWidget *chooser = OGMRIP_SOURCE_CHOOSER_WIDGET (gobject);

  switch (property_id) 
  {
    case PROP_TITLE:
      g_value_set_pointer (value, chooser->priv->title);
      break;
    case PROP_DIALOG:
      g_value_set_object (value, chooser->priv->dialog);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_source_chooser_widget_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipSourceChooserWidget *chooser = OGMRIP_SOURCE_CHOOSER_WIDGET (gobject);

  switch (property_id) 
  {
    case PROP_TITLE:
      ogmrip_source_chooser_widget_set_title (chooser, g_value_get_pointer (value));
      break;
    case PROP_DIALOG:
      chooser->priv->dialog = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_source_chooser_widget_destroy (GtkObject *object)
{
  OGMRipSourceChooserWidget *chooser = OGMRIP_SOURCE_CHOOSER_WIDGET (object);

  if (chooser->priv->dialog)
  {
    gtk_widget_destroy (chooser->priv->dialog);
    chooser->priv->dialog = NULL;
  }

  GTK_OBJECT_CLASS (ogmrip_source_chooser_widget_parent_class)->destroy (object);
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

static void
ogmrip_source_chooser_init (OGMRipSourceChooserInterface *iface)
{
  iface->get_active = ogmrip_source_chooser_widget_get_active;
  iface->select_language = ogmrip_source_chooser_widget_select_language;
}

GtkWidget *
ogmrip_source_chooser_widget_new (void)
{
  return g_object_new (OGMRIP_TYPE_SOURCE_CHOOSER_WIDGET, NULL);
}

GtkWidget *
ogmrip_source_chooser_widget_new_with_dialog (OGMRipFileChooserDialog *dialog)
{
  return g_object_new (OGMRIP_TYPE_SOURCE_CHOOSER_WIDGET, "dialog", dialog, NULL);
}

void
ogmrip_source_chooser_widget_add_audio_stream (OGMRipSourceChooserWidget *chooser, OGMDvdAudioStream *stream)
{
  GtkTreeIter iter;
  GtkTreeModel *model;

  g_return_if_fail (OGMRIP_IS_SOURCE_CHOOSER_WIDGET (chooser));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));

  if (!stream)
  {
    gtk_tree_model_get_iter_first (model, &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
        TEXT_COLUMN, _("No audio"), TYPE_COLUMN, OGMRIP_SOURCE_NONE, LANG_COLUMN, -1, SOURCE_COLUMN, NULL, -1);
  }
  else
  {
    gint aid, channels, format, lang, content, bitrate;
    gchar *str;

    ogmdvd_stream_ref (OGMDVD_STREAM (stream));

    aid = ogmdvd_stream_get_nr (OGMDVD_STREAM (stream));
    bitrate = ogmdvd_audio_stream_get_bitrate (stream);
    channels = ogmdvd_audio_stream_get_channels (stream);
    content = ogmdvd_audio_stream_get_content (stream);
    format = ogmdvd_audio_stream_get_format (stream);
    lang = ogmdvd_audio_stream_get_language (stream);

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

    ogmrip_source_chooser_widget_get_stream_iter (chooser, &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
        TEXT_COLUMN, str, TYPE_COLUMN, OGMRIP_SOURCE_STREAM, LANG_COLUMN, lang, SOURCE_COLUMN, stream, -1);

    g_free (str);
  }
}

void
ogmrip_source_chooser_widget_add_subp_stream (OGMRipSourceChooserWidget *chooser, OGMDvdSubpStream *stream)
{
  GtkTreeIter iter;
  GtkTreeModel *model;

  g_return_if_fail (OGMRIP_IS_SOURCE_CHOOSER_WIDGET (chooser));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser));

  if (!stream)
  {
    gtk_tree_model_get_iter_first (model, &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
        TEXT_COLUMN, _("No subp"), TYPE_COLUMN, OGMRIP_SOURCE_NONE, LANG_COLUMN, -1, SOURCE_COLUMN, NULL, -1);
  }
  else
  {
    gint sid, lang, content;
    gchar *str;

    ogmdvd_stream_ref (OGMDVD_STREAM (stream));

    sid = ogmdvd_stream_get_nr (OGMDVD_STREAM (stream));
    lang = ogmdvd_subp_stream_get_language (stream);
    content = ogmdvd_subp_stream_get_content (stream);

    if (content > 0)
      str = g_strdup_printf ("%s %02d: %s (%s)", _("Subtitle"), sid + 1, 
          ogmdvd_get_subp_content_label (content), ogmdvd_get_language_label (lang));
    else
      str = g_strdup_printf ("%s %02d (%s)", _("Subtitle"), sid + 1, 
          ogmdvd_get_language_label (lang));

    ogmrip_source_chooser_widget_get_stream_iter (chooser, &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
        TEXT_COLUMN, str, TYPE_COLUMN, OGMRIP_SOURCE_STREAM, LANG_COLUMN, lang, SOURCE_COLUMN, stream, -1);

    g_free (str);
  }
}

