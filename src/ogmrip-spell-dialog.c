/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#ifdef HAVE_ENCHANT_SUPPORT

#include "ogmrip-spell-dialog.h"

#include "ogmrip-helper.h"

#include <enchant.h>
#include <glib/gi18n.h>

#define OGMRIP_GLADE_FILE "ogmrip" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "ogmrip-spell.glade"
#define OGMRIP_GLADE_ROOT "root"

#define OGMRIP_SPELL_DIALOG_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_SPELL_DIALOG, OGMRipSpellDialogPriv))

enum
{
  OGMRIP_SPELL_RESPONSE_NONE       = GTK_RESPONSE_NONE,
  OGMRIP_SPELL_RESPONSE_CANCEL     = GTK_RESPONSE_CANCEL,
  OGMRIP_SPELL_RESPONSE_REPLACE    = -12,
  OGMRIP_SPELL_RESPONSE_IGNORE     = -13,
  OGMRIP_SPELL_RESPONSE_IGNORE_ALL = -14,
  OGMRIP_SPELL_RESPONSE_ADD_WORD   = -15
};

struct _OGMRipSpellDialogPriv
{
  EnchantBroker *broker;
  EnchantDict *dict;
/*
  GtkWidget *dialog;
*/
  GtkTextBuffer *buffer;

  GtkWidget *word_entry;
  GtkWidget *replace_entry;

  GtkTreeSelection *select;
  GtkListStore *word_store;

  const gchar *word;
};

static void ogmrip_spell_dialog_dispose (GObject *gobject);

static void
ogmrip_spell_dialog_changed (OGMRipSpellDialog *dialog, GtkTreeSelection *select)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (select, &model, &iter))
  {
    gchar *text;

    gtk_tree_model_get (model, &iter, 0, &text, -1);
    gtk_entry_set_text (GTK_ENTRY (dialog->priv->replace_entry), text);

    g_free (text);
  }
}

static void
ogmrip_spell_dialog_replace (OGMRipSpellDialog *dialog)
{
  dialog->priv->word = gtk_entry_get_text (GTK_ENTRY (dialog->priv->replace_entry));
  gtk_dialog_response (GTK_DIALOG (dialog), OGMRIP_SPELL_RESPONSE_REPLACE);
}

static void
ogmrip_spell_dialog_ignore (OGMRipSpellDialog *dialog)
{
  dialog->priv->word = gtk_entry_get_text (GTK_ENTRY (dialog->priv->word_entry));
  gtk_dialog_response (GTK_DIALOG (dialog), OGMRIP_SPELL_RESPONSE_IGNORE);
}

static void
ogmrip_spell_dialog_ignore_all (OGMRipSpellDialog *dialog)
{
  dialog->priv->word = gtk_entry_get_text (GTK_ENTRY (dialog->priv->word_entry));
  gtk_dialog_response (GTK_DIALOG (dialog), OGMRIP_SPELL_RESPONSE_IGNORE_ALL);
}

static void
ogmrip_spell_dialog_add_word (OGMRipSpellDialog *dialog)
{
  dialog->priv->word = gtk_entry_get_text (GTK_ENTRY (dialog->priv->word_entry));
  gtk_dialog_response (GTK_DIALOG (dialog), OGMRIP_SPELL_RESPONSE_ADD_WORD);
}

static void
ogmrip_spell_dialog_row_activated (OGMRipSpellDialog *dialog, GtkTreePath *path, 
    GtkTreeViewColumn  *column)
{
  ogmrip_spell_dialog_replace (dialog);
}

void
ogmrip_spell_dialog_set_text (OGMRipSpellDialog *dialog, const gchar *text)
{
  gtk_text_buffer_set_text (dialog->priv->buffer, text, -1);
}

void
ogmrip_spell_dialog_set_word (OGMRipSpellDialog *dialog, const gchar *word, gint offset, gchar **suggs, guint n_suggs)
{
  GtkTreeIter iter;
  guint i;

  dialog->priv->word = NULL;
  gtk_entry_set_text (GTK_ENTRY (dialog->priv->word_entry), word);
  gtk_editable_delete_text (GTK_EDITABLE (dialog->priv->replace_entry), 0, -1);

  if (offset > -1)
  {
    GtkTextIter ins, bound;

    gtk_text_buffer_get_iter_at_offset (dialog->priv->buffer, &ins, offset);
    gtk_text_buffer_get_iter_at_offset (dialog->priv->buffer, &bound, offset + g_utf8_strlen (word, -1));
    gtk_text_buffer_select_range (dialog->priv->buffer, &ins, &bound);
  }

  gtk_list_store_clear (dialog->priv->word_store);

  for (i = 0; i < n_suggs; i++)
  {
    gtk_list_store_append (dialog->priv->word_store, &iter);
    gtk_list_store_set (dialog->priv->word_store, &iter, 0, suggs[i], -1);

    if (i == 0)
      gtk_tree_selection_select_iter (dialog->priv->select, &iter);
  }

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->priv->word_store), &iter))
    gtk_tree_selection_select_iter (dialog->priv->select, &iter);
  else
    gtk_entry_set_text (GTK_ENTRY (dialog->priv->replace_entry), word);
}

gchar *
ogmrip_spell_dialog_get_word (OGMRipSpellDialog *dialog)
{
  return g_strdup (dialog->priv->word);
}

G_DEFINE_TYPE (OGMRipSpellDialog, ogmrip_spell_dialog, GTK_TYPE_DIALOG);

static void
ogmrip_spell_dialog_class_init (OGMRipSpellDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = ogmrip_spell_dialog_dispose;

  g_type_class_add_private (klass, sizeof (OGMRipSpellDialogPriv));
}

static void
ogmrip_spell_dialog_init (OGMRipSpellDialog *dialog)
{
  GError *error = NULL;

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkWidget *area, *widget;
  GtkBuilder *builder;

  dialog->priv = OGMRIP_SPELL_DIALOG_GET_PRIVATE (dialog);

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, OGMRIP_DATA_DIR G_DIR_SEPARATOR_S OGMRIP_GLADE_FILE, &error))
  {
    g_warning ("Couldn't load builder file: %s", error->message);
    g_object_unref (builder);
    g_error_free (error);
    return;
  }

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      NULL);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Spell Checking"));

  area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));

  widget = gtk_builder_get_widget (builder, OGMRIP_GLADE_ROOT);
  gtk_container_add (GTK_CONTAINER (area), widget);
  gtk_widget_show (widget);

  widget = gtk_builder_get_widget (builder, "text-view");
  dialog->priv->buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
  dialog->priv->word_entry = gtk_builder_get_widget (builder, "word-entry");
  dialog->priv->replace_entry = gtk_builder_get_widget (builder, "replace-entry");

  widget = gtk_builder_get_widget (builder, "replace-button");
  g_signal_connect_swapped (widget, "clicked", G_CALLBACK (ogmrip_spell_dialog_replace), dialog);

  widget = gtk_builder_get_widget (builder, "ignore-button");
  g_signal_connect_swapped (widget, "clicked", G_CALLBACK (ogmrip_spell_dialog_ignore), dialog);

  widget = gtk_builder_get_widget (builder, "ignore-all-button");
  g_signal_connect_swapped (widget, "clicked", G_CALLBACK (ogmrip_spell_dialog_ignore_all), dialog);

  widget = gtk_builder_get_widget (builder, "add-word-button");
  g_signal_connect_swapped (widget, "clicked", G_CALLBACK (ogmrip_spell_dialog_add_word), dialog);

  widget = gtk_builder_get_widget (builder, "word-list");
  g_signal_connect_swapped (widget, "row-activated", G_CALLBACK (ogmrip_spell_dialog_row_activated), dialog);

  dialog->priv->select = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
  g_signal_connect_swapped (dialog->priv->select, "changed", G_CALLBACK (ogmrip_spell_dialog_changed), dialog);

  dialog->priv->word_store = gtk_list_store_new (1, G_TYPE_STRING);
  gtk_tree_view_set_model (GTK_TREE_VIEW (widget), GTK_TREE_MODEL (dialog->priv->word_store));
  g_object_unref (dialog->priv->word_store);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Word", renderer, "text", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  g_object_unref (builder);
}

static void 
ogmrip_spell_dialog_dispose (GObject *gobject)
{
  OGMRipSpellDialog *dialog = OGMRIP_SPELL_DIALOG (gobject);

  if (dialog->priv->broker)
  {
    if (dialog->priv->dict);
      enchant_broker_free_dict (dialog->priv->broker, dialog->priv->dict);
      dialog->priv->dict = NULL;

    enchant_broker_free (dialog->priv->broker);
    dialog->priv->broker = NULL;
  }

  G_OBJECT_CLASS (ogmrip_spell_dialog_parent_class)->dispose (gobject);
}

GtkWidget *
ogmrip_spell_dialog_new (const gchar *language)
{
  OGMRipSpellDialog *dialog;
  EnchantBroker *broker;
  EnchantDict *dict;

  broker = enchant_broker_init ();
  dict = enchant_broker_request_dict (broker, language);
  if (!dict)
  {
    enchant_broker_free (broker);
    return NULL;
  }

  dialog = g_object_new (OGMRIP_TYPE_SPELL_DIALOG, NULL);
  dialog->priv->broker = broker;
  dialog->priv->dict = dict;

  return GTK_WIDGET (dialog);
}

gboolean
ogmrip_spell_dialog_check_word (OGMRipSpellDialog *dialog, const gchar *word, gint offset, gchar **corrected)
{
  gboolean status = TRUE;
  size_t len;

  g_return_val_if_fail (OGMRIP_IS_SPELL_DIALOG (dialog), FALSE);
  g_return_val_if_fail (word != NULL, FALSE);
  g_return_val_if_fail (corrected != NULL, FALSE);

  *corrected = NULL;
  len = strlen (word);

  if (len && enchant_dict_check (dialog->priv->dict, word, len))
  {
    gchar **suggs;
    size_t n_suggs;

    suggs = enchant_dict_suggest (dialog->priv->dict, word, len, &n_suggs);
    ogmrip_spell_dialog_set_word (dialog, word, offset, suggs, n_suggs);
    switch (gtk_dialog_run (GTK_DIALOG (dialog)))
    {
      case OGMRIP_SPELL_RESPONSE_NONE:
      case OGMRIP_SPELL_RESPONSE_CANCEL:
        status = FALSE;
        break;
      case OGMRIP_SPELL_RESPONSE_REPLACE:
        *corrected = ogmrip_spell_dialog_get_word (dialog);
        break;
      case OGMRIP_SPELL_RESPONSE_IGNORE_ALL:
        enchant_dict_add_to_session (dialog->priv->dict, word, len);
        break;
      case OGMRIP_SPELL_RESPONSE_ADD_WORD:
        enchant_dict_add_to_personal (dialog->priv->dict, word, len);
        break;
      default:
        break;
    }

    if (suggs && n_suggs)
      enchant_dict_free_suggestions (dialog->priv->dict, suggs);
  }

  return status;
}

gboolean
ogmrip_spell_dialog_check_text (OGMRipSpellDialog *dialog, const gchar *text, gchar **corrected)
{
  GString *string;
  gchar *start, *end, *word, *cw;
  gunichar ch, underscore;
  guint offset;

  g_return_val_if_fail (OGMRIP_IS_SPELL_DIALOG (dialog), FALSE);
  g_return_val_if_fail (text != NULL, FALSE);
  g_return_val_if_fail (corrected != NULL, FALSE);

  ogmrip_spell_dialog_set_text (dialog, text);

  offset = 0;
  string = g_string_new (NULL);
  start = end = (gchar *) text;
  underscore = g_utf8_get_char ("_");

  while (*start)
  {
    ch = g_utf8_get_char (end);
    if (!g_unichar_isalpha (ch) && ch != underscore)
    {
      if (*start)
      {
        word = g_strndup (start, end - start);
        if (ogmrip_spell_dialog_check_word (dialog, word, offset, &cw))
        {
          g_string_append (string, cw ? cw : word);
          g_free (cw);
        }
        else
        {
          g_string_free (string, TRUE);
          return FALSE;
        }
        g_free (word);
      }

      offset += g_utf8_strlen (start, end - start);

      while (*end && !g_unichar_isalpha (g_utf8_get_char (end)))
      {
        g_string_append_unichar (string, g_utf8_get_char (end));
        end = g_utf8_next_char (end);
        offset ++;
      }
      start = end;
    }
    else
      end = g_utf8_next_char (end);
  }

  *corrected = g_string_free (string, FALSE);

  return TRUE;
}

#endif

