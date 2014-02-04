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

#include "ogmrip-encoding-manager-dialog.h"
#include "ogmrip-profile-keys.h"
#include "ogmrip-error-dialog.h"

#include <glib/gi18n-lib.h>

#define OGMRIP_UI_RES  "/org/ogmrip/ogmrip-encoding-manager-dialog.ui"
#define OGMRIP_UI_ROOT "root"

#define gtk_builder_get_widget(builder, name) \
    (GtkWidget *) gtk_builder_get_object ((builder), (name))

enum
{
  PROP_0,
  PROP_MANAGER
};

enum
{
  COL_NAME,
  COL_TITLE,
  COL_PROFILE,
  COL_ENCODING,
  COL_STEP,
  COL_PROGRESS,
  COL_COLOR,
  COL_LAST
};

struct _OGMRipEncodingManagerDialogPriv
{
  OGMRipEncodingManager *manager;

  GtkListStore *store;
  GtkTreeSelection *selection;
};

static void ogmrip_encoding_manager_dialog_dispose      (GObject      *gobject);
static void ogmrip_encoding_manager_dialog_get_property (GObject      *gobject,
                                                         guint        property_id,
                                                         GValue       *value,
                                                         GParamSpec   *pspec);
static void ogmrip_encoding_manager_dialog_set_property (GObject      *gobject,
                                                         guint        property_id,
                                                         const GValue *value,
                                                         GParamSpec   *pspec);

static gboolean
gtk_widget_button_press_cb (GtkWidget *widget, GdkEventButton *event, GtkWidget *menu)
{
  if (event->button != 3 || event->type != GDK_BUTTON_PRESS)
    return FALSE;

  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, event->time);

  return TRUE;
}

static gboolean
gtk_widget_popup_menu_cb (GtkWidget *widget, GtkWidget *menu)
{
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());

  return TRUE;
}

static void
gtk_widget_set_popup_menu (GtkWidget *widget, GtkMenu *menu)
{
  gtk_menu_attach_to_widget (menu, widget, NULL);

  g_signal_connect (widget, "button-press-event",
      G_CALLBACK (gtk_widget_button_press_cb), menu);
  g_signal_connect (widget, "popup-menu",
      G_CALLBACK (gtk_widget_popup_menu_cb), menu);
}

static gboolean
ogmrip_encoding_manager_dialog_get_iter (OGMRipEncodingManagerDialog *dialog, OGMRipEncoding *encoding, GtkTreeIter *iter)
{
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->priv->store), iter))
  {
    OGMRipEncoding *current_encoding;

    do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->store), iter, COL_ENCODING, &current_encoding, -1);
      g_object_unref (current_encoding);

      if (current_encoding == encoding)
        return TRUE;
    }
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->priv->store), iter));
  }

  return FALSE;
}

static void
ogmrip_encoding_manager_dialog_encoding_run_cb (OGMRipEncodingManagerDialog *dialog, OGMJobTask *spawn, OGMRipEncoding *encoding)
{
  GtkTreeIter iter;

  if (ogmrip_encoding_manager_dialog_get_iter (dialog, encoding, &iter))
  {
    OGMRipStream *stream;
    gchar *message;

    gtk_list_store_set (dialog->priv->store, &iter, COL_STEP, NULL, COL_PROGRESS, 0, COL_COLOR, "rgb(0,0,0)", -1);

    if (spawn)
    {
      if (OGMRIP_IS_VIDEO_CODEC (spawn))
        gtk_list_store_set (dialog->priv->store, &iter, COL_STEP, _("Encoding video title"), -1);
      else if (OGMRIP_IS_CHAPTERS (spawn))
        gtk_list_store_set (dialog->priv->store, &iter, COL_STEP, _("Extracting chapters information"), -1);
      else if (OGMRIP_IS_CONTAINER (spawn))
        gtk_list_store_set (dialog->priv->store, &iter, COL_STEP, _("Merging audio and video streams"), -1);
      else if (OGMRIP_IS_COPY (spawn))
        gtk_list_store_set (dialog->priv->store, &iter, COL_STEP, _("Copying media"), -1);
      else if (OGMRIP_IS_ANALYZE (spawn))
        gtk_list_store_set (dialog->priv->store, &iter, COL_STEP, _("Analyzing video stream"), -1);
      else if (OGMRIP_IS_TEST (spawn))
        gtk_list_store_set (dialog->priv->store, &iter, COL_STEP, _("Running the compressibility test"), -1);
      else if (OGMRIP_IS_AUDIO_CODEC (spawn))
      {
        stream = ogmrip_codec_get_input (OGMRIP_CODEC (spawn));
        message = g_strdup_printf (_("Extracting audio stream %d"), ogmrip_stream_get_id (stream) + 1);
        gtk_list_store_set (dialog->priv->store, &iter, COL_STEP, message, -1);
        g_free (message);
      }
      else if (OGMRIP_IS_SUBP_CODEC (spawn))
      {
        stream = ogmrip_codec_get_input (OGMRIP_CODEC (spawn));
        message = g_strdup_printf (_("Extracting subtitle stream %d"), ogmrip_stream_get_id (stream) + 1);
        gtk_list_store_set (dialog->priv->store, &iter, COL_STEP, message, -1);
        g_free (message);
      }
    }
  }
}

static void
ogmrip_encoding_manager_dialog_encoding_progress_cb (OGMRipEncodingManagerDialog *dialog, OGMJobTask *spawn, gdouble fraction, OGMRipEncoding *encoding)
{
  GtkTreeIter iter;

  if (ogmrip_encoding_manager_dialog_get_iter (dialog, encoding, &iter))
  {
    gint percent = fraction * 100;

    gtk_list_store_set (dialog->priv->store, &iter, COL_PROGRESS, CLAMP (percent, 0, 100), -1);
  }
}

static void
ogmrip_encoding_manager_dialog_encoding_complete_cb (OGMRipEncodingManagerDialog *dialog, OGMJobTask *spawn, OGMRipEncodingStatus status, OGMRipEncoding *encoding)
{
  if (!spawn)
  {
    GtkTreeIter iter;

    if (ogmrip_encoding_manager_dialog_get_iter (dialog, encoding, &iter))
    {
      gtk_list_store_set (dialog->priv->store, &iter, COL_PROGRESS, 100, COL_COLOR, "rgb(211,211,211)", -1);

      switch (status)
      {
        case OGMRIP_ENCODING_SUCCESS:
          gtk_list_store_set (dialog->priv->store, &iter, COL_STEP, _("Encoding completed successfully"), -1);
          break;
        case OGMRIP_ENCODING_CANCELLED:
          gtk_list_store_set (dialog->priv->store, &iter, COL_STEP, _("Encoding cancelled by user"), -1);
          break;
        case OGMRIP_ENCODING_FAILURE:
          gtk_list_store_set (dialog->priv->store, &iter, COL_STEP, _("Encoding failed"), -1);
          break;
        default:
          gtk_list_store_set (dialog->priv->store, &iter, COL_STEP, NULL, -1);
          break;
      }
    }
  }
}

static void
ogmrip_encoding_manager_dialog_add_encoding (OGMRipEncodingManagerDialog *dialog, OGMRipEncoding *encoding)
{
  OGMRipProfile *profile;
  GtkTreeIter iter;
  gchar *str;

  profile = ogmrip_encoding_get_profile (encoding);
  str = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME);

  gtk_list_store_append (dialog->priv->store, &iter);
  gtk_list_store_set (dialog->priv->store, &iter,
      COL_NAME, ogmrip_container_get_label (ogmrip_encoding_get_container (encoding)),
      COL_TITLE, ogmrip_title_get_id (ogmrip_encoding_get_title (encoding)) + 1,
      COL_PROFILE, str, COL_ENCODING, encoding, -1);

  g_signal_connect_swapped (encoding, "run",
      G_CALLBACK (ogmrip_encoding_manager_dialog_encoding_run_cb), dialog);
  g_signal_connect_swapped (encoding, "progress",
      G_CALLBACK (ogmrip_encoding_manager_dialog_encoding_progress_cb), dialog);
  g_signal_connect_swapped (encoding, "complete",
      G_CALLBACK (ogmrip_encoding_manager_dialog_encoding_complete_cb), dialog);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT, TRUE);

  g_free (str);
}

static void
ogmrip_encoding_manager_dialog_remove_encoding (OGMRipEncodingManagerDialog *dialog, OGMRipEncoding *encoding)
{
  GtkTreeIter iter;

  if (ogmrip_encoding_manager_dialog_get_iter (dialog, encoding, &iter))
  {
    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_encoding_manager_dialog_encoding_run_cb, dialog);
    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_encoding_manager_dialog_encoding_progress_cb, dialog);
    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_encoding_manager_dialog_encoding_complete_cb, dialog);

    gtk_list_store_remove (dialog->priv->store, &iter);
  }

  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT,
      gtk_tree_model_iter_n_children (GTK_TREE_MODEL (dialog->priv->store), NULL) > 0);
}

static void
ogmrip_encoding_manager_dialog_move_encoding (OGMRipEncodingManagerDialog *dialog, OGMRipEncoding *encoding, OGMRipDirection direction)
{
  GtkTreeIter iter;

  if (ogmrip_encoding_manager_dialog_get_iter (dialog, encoding, &iter))
  {
    GtkTreeIter position = iter;

    switch (direction)
    {
      case OGMRIP_DIRECTION_TOP:
        gtk_list_store_move_after (dialog->priv->store, &iter, NULL);
        break;
      case OGMRIP_DIRECTION_UP:
        if (gtk_tree_model_iter_previous (GTK_TREE_MODEL (dialog->priv->store), &position))
          gtk_list_store_move_before (dialog->priv->store, &iter, &position);
        break;
      case OGMRIP_DIRECTION_DOWN:
        if (gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->priv->store), &position))
          gtk_list_store_move_after (dialog->priv->store, &iter, &position);
        break;
      case OGMRIP_DIRECTION_BOTTOM:
        gtk_list_store_move_before (dialog->priv->store, &iter, NULL);
        break;
    }
  }
}

static void
ogmrip_encoding_manager_dialog_set_manager (OGMRipEncodingManagerDialog *dialog, OGMRipEncodingManager *manager)
{
  GSList *list, *link;

  dialog->priv->manager = g_object_ref (manager);

  list = ogmrip_encoding_manager_get_list (manager);
  for (link = list; link; link = link->next)
    ogmrip_encoding_manager_dialog_add_encoding (dialog, link->data);
  g_slist_free (list);

  g_signal_connect_swapped (manager, "add",
      G_CALLBACK (ogmrip_encoding_manager_dialog_add_encoding), dialog);
  g_signal_connect_swapped (manager, "remove",
      G_CALLBACK (ogmrip_encoding_manager_dialog_remove_encoding), dialog);
  g_signal_connect_swapped (manager, "move",
      G_CALLBACK (ogmrip_encoding_manager_dialog_move_encoding), dialog);
}

static void
ogmrip_encoding_manager_dialog_set_move_to_top_sensitivity (GtkTreeSelection *selection, GtkAction *action)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  gtk_action_set_sensitive (action,
      gtk_tree_selection_get_selected (selection, &model, &iter) &&
      gtk_tree_model_iter_previous (model, &iter));
}

static void
ogmrip_encoding_manager_dialog_set_move_down_sensitivity (GtkTreeSelection *selection, GtkAction *action)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  gtk_action_set_sensitive (action,
      gtk_tree_selection_get_selected (selection, &model, &iter) &&
      gtk_tree_model_iter_next (model, &iter));
}

static void
ogmrip_encoding_manager_dialog_set_remove_all_sensitivity (GtkTreeSelection *selection, GtkAction *action)
{
  GtkTreeModel *model;

  gtk_tree_selection_get_selected (selection, &model, NULL);
  gtk_action_set_sensitive (action, gtk_tree_model_iter_n_children (model, NULL) > 0);
}

static void
ogmrip_encoding_manager_dialog_set_action_sensitivity (GtkTreeSelection *selection, GtkAction *action)
{
  gtk_action_set_sensitive (action, gtk_tree_selection_get_selected (selection, NULL, NULL));
}

static OGMRipEncoding *
ogmrip_encoding_manager_dialog_get_active (OGMRipEncodingManagerDialog *dialog)
{
  OGMRipEncoding *encoding;
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (!gtk_tree_selection_get_selected (dialog->priv->selection, &model, &iter))
    return NULL;

  gtk_tree_model_get (model, &iter, COL_ENCODING, &encoding, -1);
  g_object_unref (encoding);

  return encoding;
}

static void
ogmrip_encoding_manager_dialog_remove_all_activated (OGMRipEncodingManagerDialog *dialog)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->priv->store), &iter))
  {
    OGMRipEncoding *encoding;
    gboolean valid;

    g_signal_handlers_block_by_func (dialog->priv->manager,
        ogmrip_encoding_manager_dialog_remove_encoding, dialog);

    do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->store), &iter, COL_ENCODING, &encoding, -1);
      g_object_unref (encoding);

      if (ogmrip_encoding_manager_get_status (dialog->priv->manager, encoding) != OGMRIP_ENCODING_SUCCESS)
        valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->priv->store), &iter);
      else
      {
        valid = gtk_list_store_remove (dialog->priv->store, &iter);
        ogmrip_encoding_manager_remove (dialog->priv->manager, encoding);
      }
    }
    while (valid);

    g_signal_handlers_unblock_by_func (dialog->priv->manager,
        ogmrip_encoding_manager_dialog_remove_encoding, dialog);
  }
}

static void
ogmrip_encoding_manager_dialog_remove_activated (OGMRipEncodingManagerDialog *dialog)
{
  OGMRipEncoding *encoding;

  encoding = ogmrip_encoding_manager_dialog_get_active (dialog);
  if (encoding)
    ogmrip_encoding_manager_remove (dialog->priv->manager, encoding);
}

static void
ogmrip_encoding_manager_dialog_import_activated (OGMRipEncodingManagerDialog *parent)
{
  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new (_("Import Encoding"),
      GTK_WINDOW (parent), GTK_FILE_CHOOSER_ACTION_OPEN,
      _("_Cancel"), GTK_RESPONSE_REJECT,
      _("_Save"), GTK_RESPONSE_ACCEPT,
      NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    GError *error = NULL;
    gboolean status = TRUE;

    OGMRipXML *xml;
    GFile *file;

    file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
    xml = ogmrip_xml_new_from_file (file, &error);
    g_object_unref (file);

    if (!xml)
      status = FALSE;
    else
    {
      OGMRipEncoding *encoding;

      if (!g_str_equal (ogmrip_xml_get_name (xml), "encodings") || ogmrip_xml_children (xml))
      {
        do
        {
          encoding = ogmrip_encoding_new_from_xml (xml, &error);
          if (!encoding)
            status = FALSE;
          else
          {
            ogmrip_encoding_manager_add (parent->priv->manager, encoding);
            g_object_unref (encoding);
          }
        }
        while (status && ogmrip_xml_next (xml));
      }

      ogmrip_xml_free (xml);
    }

    if (!status)
    {
      ogmrip_run_error_dialog (GTK_WINDOW (dialog), error, _("Could not import the encoding"));
      g_clear_error (&error);
    }
  }

  gtk_widget_destroy (dialog);
}

static void
ogmrip_encoding_manager_dialog_export_activated (OGMRipEncodingManagerDialog *parent)
{
  GList *rows, *row;

  rows = gtk_tree_selection_get_selected_rows (parent->priv->selection, NULL);
  if (rows)
  {
    GtkWidget *dialog;

    dialog = gtk_file_chooser_dialog_new (_("Export Encoding"),
        GTK_WINDOW (parent), GTK_FILE_CHOOSER_ACTION_SAVE,
        _("_Cancel"), GTK_RESPONSE_REJECT,
        _("_Save"), GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      GError *error = NULL;
      gboolean status = TRUE;

      OGMRipXML *xml;
      GFile *file;

      xml =  ogmrip_xml_new ();

      if (rows->next)
        ogmrip_xml_append (xml, "encodings");

      for (row = rows; row; row = row->next)
      {
        GtkTreeIter iter;

        if (gtk_tree_model_get_iter (GTK_TREE_MODEL (parent->priv->store), &iter, row->data))
        {
          OGMRipEncoding *encoding;

          gtk_tree_model_get (GTK_TREE_MODEL (parent->priv->store), &iter, COL_ENCODING, &encoding, -1);
          status = ogmrip_encoding_export_to_xml (encoding, xml, &error);
          if (!status)
            break;
        }
      }

      if (status)
      {
        file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
        status = ogmrip_xml_save (xml, file, &error);
        g_object_unref (file);
      }

      ogmrip_xml_free (xml);

      if (!status)
      {
        ogmrip_run_error_dialog (GTK_WINDOW (dialog), error, _("Could not export the encoding"));
        g_clear_error (&error);
      }
    }
    gtk_widget_destroy (dialog);

    g_list_free_full (rows, (GDestroyNotify) gtk_tree_path_free);
  }
}

static void
ogmrip_encoding_manager_dialog_move_to_top_activated (OGMRipEncodingManagerDialog *dialog)
{
  OGMRipEncoding *encoding;

  encoding = ogmrip_encoding_manager_dialog_get_active (dialog);
  if (encoding)
    ogmrip_encoding_manager_move (dialog->priv->manager, encoding, OGMRIP_DIRECTION_TOP);
}

static void
ogmrip_encoding_manager_dialog_move_up_activated (OGMRipEncodingManagerDialog *dialog)
{
  OGMRipEncoding *encoding;

  encoding = ogmrip_encoding_manager_dialog_get_active (dialog);
  if (encoding)
    ogmrip_encoding_manager_move (dialog->priv->manager, encoding, OGMRIP_DIRECTION_UP);
}

static void
ogmrip_encoding_manager_dialog_move_down_activated (OGMRipEncodingManagerDialog *dialog)
{
  OGMRipEncoding *encoding;

  encoding = ogmrip_encoding_manager_dialog_get_active (dialog);
  if (encoding)
    ogmrip_encoding_manager_move (dialog->priv->manager, encoding, OGMRIP_DIRECTION_DOWN);
}

static void
ogmrip_encoding_manager_dialog_move_to_bottom_activated (OGMRipEncodingManagerDialog *dialog)
{
  OGMRipEncoding *encoding;

  encoding = ogmrip_encoding_manager_dialog_get_active (dialog);
  if (encoding)
    ogmrip_encoding_manager_move (dialog->priv->manager, encoding, OGMRIP_DIRECTION_BOTTOM);
}

G_DEFINE_TYPE (OGMRipEncodingManagerDialog, ogmrip_encoding_manager_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_encoding_manager_dialog_class_init (OGMRipEncodingManagerDialogClass *klass)
{
  GObjectClass *gobject_class;
  
  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_encoding_manager_dialog_dispose;
  gobject_class->get_property = ogmrip_encoding_manager_dialog_get_property;
  gobject_class->set_property = ogmrip_encoding_manager_dialog_set_property;

  g_object_class_install_property (gobject_class, PROP_MANAGER,
      g_param_spec_object ("manager", "Encoding manager property", "Set encoding manager",
        OGMRIP_TYPE_ENCODING_MANAGER, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipEncodingManagerDialogPriv));
}

static void
ogmrip_encoding_manager_dialog_init (OGMRipEncodingManagerDialog *dialog)
{
  GError *error = NULL;

  GObject *action;
  GtkWidget *area, *widget, *menu;
  GtkBuilder *builder;

  GtkStyleContext *context;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog,
      OGMRIP_TYPE_ENCODING_MANAGER_DIALOG, OGMRipEncodingManagerDialogPriv);

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_resource (builder, OGMRIP_UI_RES, &error))
  {
    g_warning ("Couldn't load builder file: %s", error->message);
    g_object_unref (builder);
    g_error_free (error);
    return;
  }

  gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Close"), GTK_RESPONSE_CLOSE);
  gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Execute"), GTK_RESPONSE_ACCEPT);

  gtk_window_set_title (GTK_WINDOW (dialog), _("Encoding Manager"));
  gtk_window_set_default_size (GTK_WINDOW (dialog), 500, 300);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT, FALSE);

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  widget = gtk_builder_get_widget (builder, OGMRIP_UI_ROOT);
  gtk_container_add (GTK_CONTAINER (area), widget);
  gtk_widget_show (widget);

  widget = gtk_builder_get_widget (builder, "swin");
  context = gtk_widget_get_style_context (widget);
  gtk_style_context_set_junction_sides (context, GTK_JUNCTION_BOTTOM);

  widget = gtk_builder_get_widget (builder, "toolbar");
  context = gtk_widget_get_style_context (widget);
  gtk_style_context_set_junction_sides (context, GTK_JUNCTION_TOP);
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_INLINE_TOOLBAR);

  widget = gtk_builder_get_widget (builder, "treeview");

  menu = gtk_builder_get_widget (builder, "popup");
  gtk_widget_set_popup_menu (widget, GTK_MENU (menu));

  dialog->priv->store = gtk_list_store_new (COL_LAST, G_TYPE_STRING, G_TYPE_INT,
      G_TYPE_STRING, OGMRIP_TYPE_ENCODING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
  gtk_tree_view_set_model (GTK_TREE_VIEW (widget), GTK_TREE_MODEL (dialog->priv->store));

  dialog->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Name"), renderer, "text", COL_NAME, "foreground", COL_COLOR, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Title"), renderer, "text", COL_TITLE, "foreground", COL_COLOR, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Profile"), renderer, "markup", COL_PROFILE, "foreground", COL_COLOR, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  renderer = gtk_cell_renderer_progress_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Progress"), renderer, "text", COL_STEP, "value", COL_PROGRESS, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  action = gtk_builder_get_object (builder, "remove-all-action");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_encoding_manager_dialog_remove_all_activated), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_encoding_manager_dialog_set_remove_all_sensitivity), action);

  action = gtk_builder_get_object (builder, "remove-action");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_encoding_manager_dialog_remove_activated), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_encoding_manager_dialog_set_action_sensitivity), action);

  action = gtk_builder_get_object (builder, "import-action");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_encoding_manager_dialog_import_activated), dialog);

  action = gtk_builder_get_object (builder, "export-action");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_encoding_manager_dialog_export_activated), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_encoding_manager_dialog_set_action_sensitivity), action);

  action = gtk_builder_get_object (builder, "move-to-top-action");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_encoding_manager_dialog_move_to_top_activated), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_encoding_manager_dialog_set_move_to_top_sensitivity), action);

  action = gtk_builder_get_object (builder, "move-to-bottom-action");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_encoding_manager_dialog_move_to_bottom_activated), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_encoding_manager_dialog_set_move_to_top_sensitivity), action);

  action = gtk_builder_get_object (builder, "move-up-action");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_encoding_manager_dialog_move_up_activated), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_encoding_manager_dialog_set_move_down_sensitivity), action);

  action = gtk_builder_get_object (builder, "move-down-action");
  g_signal_connect_swapped (action, "activate",
      G_CALLBACK (ogmrip_encoding_manager_dialog_move_down_activated), dialog);
  g_signal_connect (dialog->priv->selection, "changed",
      G_CALLBACK (ogmrip_encoding_manager_dialog_set_move_down_sensitivity), action);

  g_object_unref (builder);
}

static void
ogmrip_encoding_manager_dialog_dispose (GObject *gobject)
{
  OGMRipEncodingManagerDialog *dialog;

  dialog = OGMRIP_ENCODING_MANAGER_DIALOG (gobject);

  if (dialog->priv->store)
  {
    g_object_unref (dialog->priv->store);
    dialog->priv->store = NULL;
  }

  if (dialog->priv->manager)
  {
    g_signal_handlers_disconnect_by_func (dialog->priv->manager,
        ogmrip_encoding_manager_dialog_add_encoding, dialog);
    g_signal_handlers_disconnect_by_func (dialog->priv->manager,
        ogmrip_encoding_manager_dialog_remove_encoding, dialog);
    g_signal_handlers_disconnect_by_func (dialog->priv->manager,
        ogmrip_encoding_manager_dialog_move_encoding, dialog);
    g_object_unref (dialog->priv->manager);
    dialog->priv->manager = NULL;
  }

  G_OBJECT_CLASS (ogmrip_encoding_manager_dialog_parent_class)->dispose (gobject);
}

static void
ogmrip_encoding_manager_dialog_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_MANAGER:
      g_value_set_object (value, OGMRIP_ENCODING_MANAGER_DIALOG (gobject)->priv->manager);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_encoding_manager_dialog_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_MANAGER:
      ogmrip_encoding_manager_dialog_set_manager (OGMRIP_ENCODING_MANAGER_DIALOG (gobject), g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

GtkWidget *
ogmrip_encoding_manager_dialog_new (OGMRipEncodingManager *manager)
{
  return g_object_new (OGMRIP_TYPE_ENCODING_MANAGER_DIALOG, "manager", manager, NULL);
}

