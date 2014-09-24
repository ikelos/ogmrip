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

#include "ogmrip-gui.h"
#include "ogmrip-application.h"
#include "ogmrip-main-window.h"
#include "ogmrip-pref-dialog.h"

#include <glib/gi18n.h>

#ifdef G_ENABLE_DEBUG
static gboolean debug = TRUE;
#else
static gboolean debug = FALSE;
#endif

#define OGMRIP_MENU_RES "/org/ogmrip/ogmrip-menu.ui"
#define OGMRIP_ICON_RES "/org/ogmrip/ogmrip.png"

struct _OGMRipGuiPriv
{
  OGMRipEncodingManager *manager;

  gboolean is_prepared;
};

static void
ogmrip_gui_profiles_activated (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  GtkWidget *dialog;
  
  dialog = ogmrip_profile_manager_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog),
      gtk_application_get_active_window (app));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
ogmrip_gui_encodings_activated (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  OGMRipGui *gui = app;
  GtkWindow *window;
  GtkWidget *dialog;

  window = gtk_application_get_active_window (app);

  dialog = ogmrip_encoding_manager_dialog_new (gui->priv->manager);
  gtk_window_set_transient_for (GTK_WINDOW (dialog), window);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_APPLY)
    gtk_widget_destroy (dialog);
  else
  {
    GSList *list, *link;

    list = ogmrip_encoding_manager_get_list (gui->priv->manager);
    for (link = list; link; link = link->next)
      if (ogmrip_encoding_manager_get_status (gui->priv->manager, link->data) != OGMRIP_ENCODING_SUCCESS)
        ogmrip_main_window_encode (OGMRIP_MAIN_WINDOW (window), g_object_ref (link->data));
    g_slist_free (list);
  }
}

static void
ogmrip_gui_pref_activated (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  GtkWidget *dialog;

  dialog = ogmrip_pref_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog),
      gtk_application_get_active_window (app));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
ogmrip_gui_about_activated (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  GdkPixbuf *icon = NULL;

  const gchar *authors[] =
  {
    "Olivier Rolland <billl@users.sourceforge.net>",
    NULL
  };
  gchar *translator_credits = _("translator-credits");

  const gchar *documenters[] =
  {
    "Olivier Rolland <billl@users.sourceforge.net>",
    NULL
  };

  icon = gdk_pixbuf_new_from_resource (OGMRIP_ICON_RES, NULL);

  if (g_str_equal (translator_credits, "translator-credits"))
    translator_credits = NULL;

  gtk_show_about_dialog (
      gtk_application_get_active_window (app),
      "name", PACKAGE_NAME,
      "version", PACKAGE_VERSION,
      "comments", _("A Media Encoder for GNOME"),
      "copyright", "(c) 2004-2014 Olivier Rolland",
      "website", "http://ogmrip.sourceforge.net",
      "translator-credits", translator_credits,
      "documenters", documenters,
      "authors", authors,
      "logo", icon,
      NULL);

  if (icon)
    g_object_unref (icon);
}

static void
ogmrip_gui_quit_activated (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  GList *list;

  while (1)
  {
    list = gtk_application_get_windows (app);
    if (!list)
      break;
    gtk_widget_destroy (list->data);
  }
}

static GActionEntry app_entries[] =
{
  { "profiles",    ogmrip_gui_profiles_activated,  NULL, NULL, NULL },
  { "encodings",   ogmrip_gui_encodings_activated, NULL, NULL, NULL },
  { "preferences", ogmrip_gui_pref_activated,      NULL, NULL, NULL },
  { "about",       ogmrip_gui_about_activated,     NULL, NULL, NULL },
  { "quit",        ogmrip_gui_quit_activated,      NULL, NULL, NULL }
};

static void
ogmrip_gui_startup_cb (GApplication *app)
{
  GError *error = NULL;
  GtkBuilder *builder;
  GObject *menu;

  g_action_map_add_action_entries (G_ACTION_MAP (app),
      app_entries, G_N_ELEMENTS (app_entries), app);

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_resource (builder, OGMRIP_MENU_RES, &error))
    g_error ("Couldn't load builder file: %s", error->message);
  menu = gtk_builder_get_object (builder, "app-menu");
  g_object_unref (builder);

  gtk_application_set_app_menu (GTK_APPLICATION (app), G_MENU_MODEL (menu));
}

static void
ogmrip_gui_activate_cb (GApplication *app)
{
  GtkWidget *window;

  window = ogmrip_main_window_new (OGMRIP_APPLICATION (app), OGMRIP_GUI (app)->priv->manager);
  gtk_window_present (GTK_WINDOW (window));
}

static void
ogmrip_gui_open_cb (GApplication *app, GFile **files, gint n_files, const gchar *hint)
{
  GtkWidget *window;
  gchar *filename;
  gint i;

  for (i = 0; i < n_files; i ++)
  {
    window = ogmrip_main_window_new (OGMRIP_APPLICATION (app), OGMRIP_GUI (app)->priv->manager);

    filename = g_file_get_path (files[i]);
    if (!g_file_test (filename, G_FILE_TEST_EXISTS) ||
        !ogmrip_main_window_load_path (OGMRIP_MAIN_WINDOW (window), filename))
      gtk_widget_destroy (window);
    else
      gtk_window_present (GTK_WINDOW (window));
    g_free (filename);
  }
}

static void
ogmrip_gui_prepare_cb (OGMRipApplication *app)
{
  OGMRIP_GUI (app)->priv->is_prepared = TRUE;
}

static gboolean
ogmrip_gui_get_is_prepared (OGMRipApplication *app)
{
  return OGMRIP_GUI (app)->priv->is_prepared;
}

static void
ogmrip_application_iface_init (OGMRipApplicationInterface *iface)
{
  iface->get_is_prepared = ogmrip_gui_get_is_prepared;
}

G_DEFINE_TYPE_WITH_CODE (OGMRipGui, ogmrip_gui, GTK_TYPE_APPLICATION,
    G_ADD_PRIVATE (OGMRipGui)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_APPLICATION, ogmrip_application_iface_init));

static GOptionEntry opts[] =
{
  { "debug", 0,  0, G_OPTION_ARG_NONE, &debug, "Enable debug messages", NULL },
  { NULL,    0,  0, 0,                 NULL,   NULL,                    NULL }
};

static gboolean
ogmrip_gui_local_cmdline (GApplication *application, gchar ***argv, gint *status)
{
  GError *error = NULL;
  GOptionContext *context;
  gboolean retval;
  gint argc;

  context = g_option_context_new ("[<PATH>]");
  g_option_context_add_main_entries (context, opts, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  argc = g_strv_length (*argv);
  retval = g_option_context_parse (context, &argc, argv, &error);
  g_option_context_free (context);

  if (!retval)
  {
    g_print ("option parsing failed: %s\n", error->message);
    g_error_free (error);
    *status = 1;

    return TRUE;
  }

  if (!g_application_register (application, NULL, &error))
  {
    g_critical ("%s", error->message);
    g_error_free (error);
    *status = 1;

    return TRUE;
  }

  if (argc <= 1)
    g_application_activate (application);
  else
  {
    GFile **files;
    gint i;

    files = g_new (GFile *, argc - 1);

    for (i = 0; i < argc - 1; i ++)
      files[i] = g_file_new_for_commandline_arg ((*argv)[i + 1]);

    g_application_open (application, files, argc - 1, "");

    for (i = 0; i < argc - 1; i ++)
      g_object_unref (files[i]);
    g_free (files);
  }

  if (debug)
    ogmrip_log_set_print_stdout (TRUE);

  *status = 0;

  return TRUE;
}

static void
ogmrip_gui_dispose (GObject *gobject)
{
  OGMRipGui *gui = OGMRIP_GUI (gobject);

  g_clear_object (&gui->priv->manager);

  G_OBJECT_CLASS (ogmrip_gui_parent_class)->dispose (gobject);
}

static void
ogmrip_gui_class_init (OGMRipGuiClass *klass)
{
  GObjectClass *gobject_class;
  GApplicationClass *application_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_gui_dispose;

  application_class = G_APPLICATION_CLASS (klass);
  application_class->local_command_line = ogmrip_gui_local_cmdline;
}

static void
ogmrip_gui_init (OGMRipGui *gui)
{
  gui->priv = ogmrip_gui_get_instance_private (gui);

  gui->priv->manager = ogmrip_encoding_manager_new ();
}

GApplication *
ogmrip_gui_new (const gchar *app_id)
{
  GApplication *app;

  g_return_val_if_fail (g_application_id_is_valid (app_id), NULL);

  app = g_object_new (OGMRIP_TYPE_GUI,
      "application-id", app_id,
      "flags", G_APPLICATION_HANDLES_OPEN,
      NULL);

  g_signal_connect (app, "startup",
      G_CALLBACK (ogmrip_gui_startup_cb), NULL);
  g_signal_connect (app, "activate",
      G_CALLBACK (ogmrip_gui_activate_cb), NULL);
  g_signal_connect (app, "open",
      G_CALLBACK (ogmrip_gui_open_cb), NULL);
  g_signal_connect (app, "prepare",
      G_CALLBACK (ogmrip_gui_prepare_cb), NULL);

  return app;
}

