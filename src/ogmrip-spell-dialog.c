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

#ifdef HAVE_ENCHANT_SUPPORT

#include "ogmrip-spell-dialog.h"

#include "ogmrip-helper.h"

#include <enchant.h>
#include <glib/gi18n.h>

#define OGMRIP_UI_RES  "/org/ogmrip/ogmrip-spell-dialog.ui"
#define OGMRIP_UI_ROOT "root"

enum
{
  OGMRIP_SPELL_RESPONSE_REPLACE    = 0,
  OGMRIP_SPELL_RESPONSE_IGNORE     = 1,
  OGMRIP_SPELL_RESPONSE_IGNORE_ALL = 2,
  OGMRIP_SPELL_RESPONSE_ADD_WORD   = 4
};

struct _OGMRipSpellDialogPriv
{
  EnchantBroker *broker;
  EnchantDict *dict;

  GtkTextBuffer *text_buffer;

  GtkWidget *word_entry;
  GtkWidget *replace_entry;
  GtkWidget *replace_button;
  GtkWidget *ignore_button;
  GtkWidget *ignore_all_button;
  GtkWidget *add_word_button;
  GtkWidget *word_list;

  GtkTreeSelection *word_selection;
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
  gtk_text_buffer_set_text (dialog->priv->text_buffer, text, -1);
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

    gtk_text_buffer_get_iter_at_offset (dialog->priv->text_buffer, &ins, offset);
    gtk_text_buffer_get_iter_at_offset (dialog->priv->text_buffer, &bound, offset + g_utf8_strlen (word, -1));
    gtk_text_buffer_select_range (dialog->priv->text_buffer, &ins, &bound);
  }

  gtk_list_store_clear (dialog->priv->word_store);

  for (i = 0; i < n_suggs; i++)
  {
    gtk_list_store_append (dialog->priv->word_store, &iter);
    gtk_list_store_set (dialog->priv->word_store, &iter, 0, suggs[i], -1);

    if (i == 0)
      gtk_tree_selection_select_iter (dialog->priv->word_selection, &iter);
  }

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->priv->word_store), &iter))
    gtk_tree_selection_select_iter (dialog->priv->word_selection, &iter);
  else
    gtk_entry_set_text (GTK_ENTRY (dialog->priv->replace_entry), word);
}

gchar *
ogmrip_spell_dialog_get_word (OGMRipSpellDialog *dialog)
{
  return g_strdup (dialog->priv->word);
}

G_DEFINE_TYPE_WITH_PRIVATE (OGMRipSpellDialog, ogmrip_spell_dialog, GTK_TYPE_DIALOG);

static void
ogmrip_spell_dialog_class_init (OGMRipSpellDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = ogmrip_spell_dialog_dispose;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass), OGMRIP_UI_RES);

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipSpellDialog, text_buffer);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipSpellDialog, word_entry);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipSpellDialog, replace_entry);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipSpellDialog, replace_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipSpellDialog, ignore_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipSpellDialog, ignore_all_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipSpellDialog, add_word_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipSpellDialog, word_list);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipSpellDialog, word_store);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), OGMRipSpellDialog, word_selection);
}

static void
ogmrip_spell_dialog_init (OGMRipSpellDialog *dialog)
{
  gtk_widget_init_template (GTK_WIDGET (dialog));

  dialog->priv = ogmrip_spell_dialog_get_instance_private (dialog);

  g_signal_connect_swapped (dialog->priv->replace_button, "clicked",
                            G_CALLBACK (ogmrip_spell_dialog_replace), dialog);
  g_signal_connect_swapped (dialog->priv->ignore_button, "clicked",
                            G_CALLBACK (ogmrip_spell_dialog_ignore), dialog);
  g_signal_connect_swapped (dialog->priv->ignore_all_button, "clicked",
                            G_CALLBACK (ogmrip_spell_dialog_ignore_all), dialog);
  g_signal_connect_swapped (dialog->priv->add_word_button, "clicked",
                            G_CALLBACK (ogmrip_spell_dialog_add_word), dialog);
  g_signal_connect_swapped (dialog->priv->word_list, "row-activated",
                            G_CALLBACK (ogmrip_spell_dialog_row_activated), dialog);
  g_signal_connect_swapped (dialog->priv->word_selection, "changed",
                            G_CALLBACK (ogmrip_spell_dialog_changed), dialog);
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
      case GTK_RESPONSE_NONE:
      case GTK_RESPONSE_CANCEL:
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

