/* OGMRip - A media encoder for GNOME
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-application.h"
#include "ogmrip-cli.h"
#include "ogmrip-settings.h"

#ifdef HAVE_GTK_SUPPORT
#include "ogmrip-gui.h"
#endif

#include <ogmrip-encode.h>
#include <ogmrip-dvd.h>
#include <ogmrip-module.h>

#ifdef HAVE_BLURAY_SUPPORT
#include <ogmrip-bluray.h>
#endif

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <locale.h>

#ifdef HAVE_LIBNOTIFY_SUPPORT
#include <libnotify/notify.h>
#endif /* HAVE_LIBNOTIFY_SUPPORT */

GSettings *settings = NULL;

static void
ogmrip_tmp_dir_changed_cb (GSettings *settings, const gchar *key)
{
  gchar *path;

  path = g_settings_get_string (settings, key);
  ogmrip_fs_set_tmp_dir (path);
  g_free (path);
}

#ifdef HAVE_GTK_SUPPORT
static gboolean
ogmrip_profile_engine_update_cb (OGMRipProfileEngine *engine, OGMRipProfile *profile)
{
  GtkWidget *dialog;
  gchar *name;
  gint response;

  name = g_settings_get_string (G_SETTINGS (profile), OGMRIP_PROFILE_NAME);

  dialog = gtk_message_dialog_new_with_markup (NULL, 0,
      GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "<b>%s</b>\n\n%s",
      "A new version of the following profile is available.", name);
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
      "Do you want to use it ?");

  g_free (name);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  return response == GTK_RESPONSE_YES;
}
#endif

static gboolean
ogmrip_startup_finish (OGMRipApplication *app)
{
  ogmrip_application_prepare (app);

  return TRUE;
}

static gboolean
ogmrip_startup_thread (GIOSchedulerJob *job, GCancellable *cancellable, GApplication *app)
{
  OGMRipModuleEngine *module_engine;
  OGMRipProfileEngine *profile_engine;
  gchar *path;

  ogmrip_dvd_register_media ();
#ifdef HAVE_BLURAY_SUPPORT
  ogmrip_bluray_register_media ();
#endif
  ogmrip_file_register_media ();

  ogmrip_hardsub_register_codec ();
  // ogmrip_novideo_register_codec ();

  module_engine = ogmrip_module_engine_get_default ();
  g_object_set_data_full (G_OBJECT (app), "module-engine", module_engine, g_object_unref);

  path = g_build_filename (OGMRIP_LIB_DIR, "ogmrip", "plugins", NULL);
  ogmrip_module_engine_add_path (module_engine, path, NULL);
  g_free (path);

  path = g_build_filename (g_get_user_data_dir (), "ogmrip", "plugins", NULL);
  ogmrip_module_engine_add_path (module_engine, path, NULL);
  g_free (path);

  path = g_build_filename (OGMRIP_LIB_DIR, "ogmrip", "extensions", NULL);
  ogmrip_module_engine_add_path (module_engine, path, NULL);
  g_free (path);

  path = g_build_filename (g_get_user_data_dir (), "ogmrip", "extensions", NULL);
  ogmrip_module_engine_add_path (module_engine, path, NULL);
  g_free (path);

  profile_engine = ogmrip_profile_engine_get_default ();
  g_object_set_data_full (G_OBJECT (app), "profile-engine", profile_engine, g_object_unref);

#ifdef HAVE_GTK_SUPPORT
  g_signal_connect (profile_engine, "update",
      G_CALLBACK (ogmrip_profile_engine_update_cb), NULL);
#endif

  path = g_build_filename (OGMRIP_DATA_DIR, "ogmrip", "profiles", NULL);
  ogmrip_profile_engine_add_path (profile_engine, path);
  g_free (path);

  path = g_build_filename (g_get_user_data_dir (), "ogmrip", "profiles", NULL);
  ogmrip_profile_engine_add_path (profile_engine, path);
  g_free (path);

  g_io_scheduler_job_send_to_mainloop (job, (GSourceFunc) ogmrip_startup_finish, app, NULL);

  return FALSE;
}

static guint
ogmrip_get_locale (void)
{
  static guint code = 0;

  if (!code)
  {
    gchar *locale;

    locale = setlocale (LC_ALL, NULL);
    if (locale && strlen (locale) > 2)
    {
      code = locale[0];
      code = (code << 8) | locale[1];

      if (!ogmrip_language_to_name (code))
        code = 0;
    }
  }

  return code;
}

/*
 * When the application starts
 */
static void
ogmrip_gui_startup_cb (GApplication *app)
{
  gchar *path;

  settings = g_settings_new ("org.ogmrip.preferences");
  g_object_set_data_full (G_OBJECT (app), "settings", settings, g_object_unref);

  if (!g_settings_get_uint (settings, OGMRIP_SETTINGS_PREF_AUDIO))
    g_settings_set_uint (settings, OGMRIP_SETTINGS_PREF_AUDIO, ogmrip_get_locale ());

  if (!g_settings_get_uint (settings, OGMRIP_SETTINGS_CHAPTER_LANG))
    g_settings_set_uint (settings, OGMRIP_SETTINGS_CHAPTER_LANG, ogmrip_get_locale ());

  path = g_settings_get_string (settings, OGMRIP_SETTINGS_OUTPUT_DIR);
  if (!strlen (path) || !g_file_test (path, G_FILE_TEST_IS_DIR))
    g_settings_set_string (settings, OGMRIP_SETTINGS_OUTPUT_DIR, g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS));
  g_free (path);

  path = g_settings_get_string (settings, OGMRIP_SETTINGS_TMP_DIR);
  if (strlen (path) > 0 && g_file_test (path, G_FILE_TEST_IS_DIR))
    ogmrip_fs_set_tmp_dir (path);
  else
    g_settings_set_string (settings, OGMRIP_SETTINGS_TMP_DIR, ogmrip_fs_get_tmp_dir ());
  g_free (path);

  g_signal_connect (settings, "changed::" OGMRIP_SETTINGS_TMP_DIR,
      G_CALLBACK (ogmrip_tmp_dir_changed_cb), NULL);

#ifdef HAVE_LIBNOTIFY_SUPPORT
  notify_init (PACKAGE_NAME);
#endif /* HAVE_LIBNOTIFY_SUPPORT */

  g_io_scheduler_push_job ((GIOSchedulerJobFunc) ogmrip_startup_thread,
      app, NULL, G_PRIORITY_HIGH, NULL);
}

int
main (int argc, char *argv[])
{
  GApplication *app;
  gint status;

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */

#ifdef HAVE_GTK_SUPPORT
  gtk_init (&argc, &argv);
#endif

  g_set_application_name (_("OGMRip"));

#ifdef HAVE_GTK_SUPPORT
  app = ogmrip_gui_new ("org.gnome.ogmrip");
#else
  app = ogmrip_cli_new ("org.gnome.ogmrip");
#endif

  g_signal_connect (G_OBJECT (app), "startup",
      G_CALLBACK (ogmrip_gui_startup_cb), NULL);

  status = g_application_run (app, argc, argv);

  g_object_unref (app);

  return status;
}

