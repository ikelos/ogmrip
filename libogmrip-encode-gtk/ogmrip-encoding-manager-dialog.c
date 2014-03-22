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

#define OGMRIP_UI_RES "/org/ogmrip/ogmrip-encoding-manager-dialog.ui"

enum
{
  PROP_0,
  PROP_MANAGER
};

struct _OGMRipEncodingManagerDialogPriv
{
  OGMRipEncodingManager *manager;

  GtkWidget *swin;
  GtkWidget *toolbar;
  GtkWidget *list_box;
  GtkWidget *import_button;
  GtkWidget *export_button;
  GtkWidget *remove_button;
  GtkWidget *remove_all_button;
  GtkWidget *select_button;

  GAction *select_action;
  GAction *export_action;
  GAction *remove_action;
  GAction *remove_all_action;

  guint nencodings;
  guint nselected;
};

static GtkWidget *
ogmrip_encoding_manager_dialog_get_row (OGMRipEncodingManagerDialog *dialog, OGMRipEncoding *encoding)
{
  GList *list, *link;

  list = gtk_container_get_children (GTK_CONTAINER (dialog->priv->list_box));
  for (link = list; link; link = link->next)
  {
    OGMRipEncoding *current_encoding;

    current_encoding = g_object_get_data (link->data, "encoding");
    if (current_encoding == encoding)
      return link->data;
  }
  g_list_free (list);

  return NULL;
}

static void
ogmrip_encoding_manager_dialog_encoding_run_cb (OGMRipEncoding *encoding, OGMJobTask *spawn, GtkWidget *progress)
{
  OGMRipStream *stream;
  gchar *message;

  gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (progress), TRUE);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), 0.0);

  if (spawn)
  {
    if (OGMRIP_IS_VIDEO_CODEC (spawn))
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), _("Encoding video title"));
    else if (OGMRIP_IS_CHAPTERS (spawn))
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), _("Extracting chapters information"));
    else if (OGMRIP_IS_CONTAINER (spawn))
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), _("Merging audio and video streams"));
    else if (OGMRIP_IS_COPY (spawn))
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), _("Copying media"));
    else if (OGMRIP_IS_ANALYZE (spawn))
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), _("Analyzing video stream"));
    else if (OGMRIP_IS_TEST (spawn))
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), _("Running the compressibility test"));
    else if (OGMRIP_IS_AUDIO_CODEC (spawn))
    {
      stream = ogmrip_codec_get_input (OGMRIP_CODEC (spawn));
      message = g_strdup_printf (_("Extracting audio stream %d"), ogmrip_stream_get_id (stream) + 1);
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), message);
      g_free (message);
    }
    else if (OGMRIP_IS_SUBP_CODEC (spawn))
    {
      stream = ogmrip_codec_get_input (OGMRIP_CODEC (spawn));
      message = g_strdup_printf (_("Extracting subtitle stream %d"), ogmrip_stream_get_id (stream) + 1);
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), message);
      g_free (message);
    }
  }
}

static void
ogmrip_encoding_manager_dialog_encoding_progress_cb (OGMRipEncoding *encoding, OGMJobTask *spawn, gdouble fraction, GtkWidget *progress)
{
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), fraction);
}

static void
ogmrip_encoding_manager_dialog_encoding_complete_cb (OGMRipEncoding *encoding, OGMJobTask *spawn, OGMRipEncodingStatus status, GtkWidget *progress)
{
  if (!spawn)
  {
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), 100.0);

    switch (status)
    {
      case OGMRIP_ENCODING_SUCCESS:
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), _("Encoding completed successfully"));
        break;
      case OGMRIP_ENCODING_CANCELLED:
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), _("Encoding cancelled by user"));
        break;
      case OGMRIP_ENCODING_FAILURE:
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), _("Encoding failed"));
        break;
      default:
        gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (progress), FALSE);
        break;
    }
  }
}

static void
ogmrip_encoding_manager_dialog_check_toggled_cb (OGMRipEncodingManagerDialog *dialog, GtkWidget *check)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check)))
    dialog->priv->nselected ++;
  else
    dialog->priv->nselected --;

  g_simple_action_set_enabled (G_SIMPLE_ACTION (dialog->priv->export_action), dialog->priv->nselected > 0);
  g_simple_action_set_enabled (G_SIMPLE_ACTION (dialog->priv->remove_all_action), dialog->priv->nselected > 0);
}

static void
ogmrip_encoding_manager_dialog_add_encoding (OGMRipEncodingManagerDialog *dialog, OGMRipEncoding *encoding)
{
  OGMRipProfile *profile;
  GtkWidget *row, *grid, *label, *progress, *check;
  gchar *str;

  row = gtk_list_box_row_new ();
  gtk_container_add (GTK_CONTAINER (dialog->priv->list_box), row);
  gtk_widget_show (row);

  g_object_set_data_full (G_OBJECT (row), "encoding", g_object_ref (encoding), g_object_unref);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_homogeneous (GTK_GRID (grid), FALSE);
  gtk_container_add (GTK_CONTAINER (row), grid);
  gtk_widget_show (grid);

  profile = ogmrip_encoding_get_profile (encoding);

  label = gtk_label_new (ogmrip_container_get_label (ogmrip_encoding_get_container (encoding)));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_widget_show (label);

  str = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME);
  label = gtk_label_new (str);
  g_free (str);

  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_widget_show (label);

  progress = gtk_progress_bar_new ();
  gtk_widget_set_hexpand (progress, TRUE);
  gtk_widget_set_vexpand (progress, TRUE);
  gtk_grid_attach (GTK_GRID (grid), progress, 1, 0, 1, 2);
  gtk_widget_show (progress);

  g_object_set_data (G_OBJECT (row), "progress", progress);

  check = gtk_check_button_new ();
  gtk_grid_attach (GTK_GRID (grid), check, 2, 0, 1, 2);

  g_object_set_data (G_OBJECT (row), "check", check);

  g_object_bind_property (dialog->priv->select_button, "active",
      check, "visible", G_BINDING_SYNC_CREATE);

  g_signal_connect_swapped (check, "toggled",
      G_CALLBACK (ogmrip_encoding_manager_dialog_check_toggled_cb), dialog);

  g_signal_connect (encoding, "run",
      G_CALLBACK (ogmrip_encoding_manager_dialog_encoding_run_cb), progress);
  g_signal_connect (encoding, "progress",
      G_CALLBACK (ogmrip_encoding_manager_dialog_encoding_progress_cb), progress);
  g_signal_connect (encoding, "complete",
      G_CALLBACK (ogmrip_encoding_manager_dialog_encoding_complete_cb), progress);

  dialog->priv->nencodings ++;

  g_simple_action_set_enabled (G_SIMPLE_ACTION (dialog->priv->select_action), TRUE);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_APPLY, TRUE);
}

static void
ogmrip_encoding_manager_dialog_remove_encoding (OGMRipEncodingManagerDialog *dialog, OGMRipEncoding *encoding)
{
  GtkWidget *row;

  row = ogmrip_encoding_manager_dialog_get_row (dialog, encoding);
  if (row)
  {
    GtkWidget *progress;

    progress = g_object_get_data (G_OBJECT (row), "progress");

    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_encoding_manager_dialog_encoding_run_cb, progress);
    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_encoding_manager_dialog_encoding_progress_cb, progress);
    g_signal_handlers_disconnect_by_func (encoding,
        ogmrip_encoding_manager_dialog_encoding_complete_cb, progress);

    gtk_container_remove (GTK_CONTAINER (dialog->priv->list_box), row);

    dialog->priv->nencodings --;

    g_simple_action_set_enabled (G_SIMPLE_ACTION (dialog->priv->select_action), dialog->priv->nencodings > 0);

    gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_APPLY, dialog->priv->nencodings > 0);
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
}

static OGMRipEncoding *
ogmrip_encoding_manager_dialog_get_active (OGMRipEncodingManagerDialog *dialog)
{
  GtkListBoxRow *row;

  row = gtk_list_box_get_selected_row (GTK_LIST_BOX (dialog->priv->list_box));
  if (!row)
    return NULL;

  return g_object_get_data (G_OBJECT (row), "encoding");
}

static void
ogmrip_encoding_manager_dialog_remove_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipEncodingManagerDialog *dialog = data;
  OGMRipEncoding *encoding;

  encoding = ogmrip_encoding_manager_dialog_get_active (dialog);
  if (encoding)
    ogmrip_encoding_manager_remove (dialog->priv->manager, encoding);
}

static void
ogmrip_encoding_manager_dialog_remove_all_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipEncodingManagerDialog *dialog = data;
  GList *list, *link;

  list = gtk_container_get_children (GTK_CONTAINER (dialog->priv->list_box));
  for (link = list; link; link = link->next)
  {
    GtkWidget *check;

    check = g_object_get_data (link->data, "check");
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check)))
      ogmrip_encoding_manager_dialog_remove_encoding (dialog, g_object_get_data (link->data, "encoding"));
  }
  g_list_free (list);
}

static void
ogmrip_encoding_manager_dialog_import_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipEncodingManagerDialog *parent = data;
  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new (_("Import Encoding"),
      GTK_WINDOW (parent), GTK_FILE_CHOOSER_ACTION_OPEN,
      _("_Cancel"), GTK_RESPONSE_CANCEL,
      _("_Import"), GTK_RESPONSE_OK,
      NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
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
ogmrip_encoding_manager_dialog_export_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  OGMRipEncodingManagerDialog *parent = data;
  GList *list, *link;

  list = gtk_container_get_children (GTK_CONTAINER (parent->priv->list_box));
  if (list)
  {
    GtkWidget *dialog;

    dialog = gtk_file_chooser_dialog_new (_("Export Encoding"),
        GTK_WINDOW (parent), GTK_FILE_CHOOSER_ACTION_SAVE,
        _("_Cancel"), GTK_RESPONSE_CANCEL,
        _("_Export"), GTK_RESPONSE_OK,
        NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      GError *error = NULL;
      gboolean status = TRUE;

      OGMRipXML *xml;
      GFile *file;

      xml =  ogmrip_xml_new ();

      if (list->next)
        ogmrip_xml_append (xml, "encodings");

      for (link = list; link; link = link->next)
      {
        OGMRipEncoding *encoding;

        encoding = g_object_get_data (link->data, "encoding");
        if (encoding)
        {
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
  }
  g_list_free (list);
}

static void
ogmrip_encoding_manager_dialog_encoding_selected_cb (GtkListBox *list_box, GtkListBoxRow *row, OGMRipEncodingManagerDialog *dialog)
{
  g_simple_action_set_enabled (G_SIMPLE_ACTION (dialog->priv->remove_action), row != NULL);
}

G_DEFINE_TYPE_WITH_PRIVATE (OGMRipEncodingManagerDialog, ogmrip_encoding_manager_dialog, GTK_TYPE_DIALOG)

static void
ogmrip_encoding_manager_dialog_dispose (GObject *gobject)
{
  OGMRipEncodingManagerDialog *dialog;

  dialog = OGMRIP_ENCODING_MANAGER_DIALOG (gobject);

  if (dialog->priv->manager)
  {
    g_signal_handlers_disconnect_by_func (dialog->priv->manager,
        ogmrip_encoding_manager_dialog_add_encoding, dialog);
    g_signal_handlers_disconnect_by_func (dialog->priv->manager,
        ogmrip_encoding_manager_dialog_remove_encoding, dialog);

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

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), OGMRIP_UI_RES);

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipEncodingManagerDialog, swin);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipEncodingManagerDialog, toolbar);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipEncodingManagerDialog, import_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipEncodingManagerDialog, export_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipEncodingManagerDialog, remove_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipEncodingManagerDialog, remove_all_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipEncodingManagerDialog, select_button);
}

static GActionEntry entries[] =
{
  { "import",     ogmrip_encoding_manager_dialog_import_activated,     NULL, NULL, NULL },
  { "export",     ogmrip_encoding_manager_dialog_export_activated,     NULL, NULL, NULL },
  { "remove",     ogmrip_encoding_manager_dialog_remove_activated,     NULL, NULL, NULL },
  { "remove-all", ogmrip_encoding_manager_dialog_remove_all_activated, NULL, NULL, NULL },
  { "select",     NULL,                                                NULL, NULL, NULL }
};

static void
ogmrip_encoding_manager_dialog_init (OGMRipEncodingManagerDialog *dialog)
{
  GtkStyleContext *context;
  GSimpleActionGroup *group;

  gtk_widget_init_template (GTK_WIDGET (dialog));

  dialog->priv = ogmrip_encoding_manager_dialog_get_instance_private (dialog);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_APPLY);

  context = gtk_widget_get_style_context (dialog->priv->swin);
  gtk_style_context_set_junction_sides (context, GTK_JUNCTION_BOTTOM);

  context = gtk_widget_get_style_context (dialog->priv->toolbar);
  gtk_style_context_set_junction_sides (context, GTK_JUNCTION_TOP);
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_INLINE_TOOLBAR);

  dialog->priv->list_box = gtk_list_box_new ();
  gtk_container_add (GTK_CONTAINER (dialog->priv->swin), dialog->priv->list_box);
  gtk_widget_show (dialog->priv->list_box);

  g_signal_connect (dialog->priv->list_box, "row-selected",
      G_CALLBACK (ogmrip_encoding_manager_dialog_encoding_selected_cb), dialog);

  group = g_simple_action_group_new ();
  gtk_widget_insert_action_group (GTK_WIDGET (dialog), "encoding", G_ACTION_GROUP (group));
  g_object_unref (group);

  g_action_map_add_action_entries (G_ACTION_MAP (group),
      entries, G_N_ELEMENTS (entries), dialog);

  dialog->priv->select_action = g_action_map_lookup_action (G_ACTION_MAP (group), "select");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (dialog->priv->select_action), FALSE);

  dialog->priv->export_action = g_action_map_lookup_action (G_ACTION_MAP (group), "export");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (dialog->priv->export_action), FALSE);

  dialog->priv->remove_action = g_action_map_lookup_action (G_ACTION_MAP (group), "remove");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (dialog->priv->remove_action), FALSE);

  dialog->priv->remove_all_action = g_action_map_lookup_action (G_ACTION_MAP (group), "remove-all");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (dialog->priv->remove_all_action), FALSE);

  g_object_bind_property (dialog->priv->select_button, "active",
      dialog->priv->import_button, "visible", G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
  g_object_bind_property (dialog->priv->select_button, "active",
      dialog->priv->remove_button, "visible", G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
  g_object_bind_property (dialog->priv->select_button, "active",
      dialog->priv->export_button, "visible", G_BINDING_SYNC_CREATE);
  g_object_bind_property (dialog->priv->select_button, "active",
      dialog->priv->remove_all_button, "visible", G_BINDING_SYNC_CREATE);

  gtk_actionable_set_action_name (GTK_ACTIONABLE (dialog->priv->import_button), "encoding.import");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (dialog->priv->export_button), "encoding.export");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (dialog->priv->remove_button), "encoding.remove");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (dialog->priv->select_button), "encoding.select");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (dialog->priv->remove_all_button), "encoding.remove-all");
}

GtkWidget *
ogmrip_encoding_manager_dialog_new (OGMRipEncodingManager *manager)
{
  return g_object_new (OGMRIP_TYPE_ENCODING_MANAGER_DIALOG, "manager", manager, NULL);
}

