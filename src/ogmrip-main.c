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

GSettings *settings = NULL;

static void
ogmrip_tmp_dir_changed_cb (GSettings *gsettings, const gchar *key)
{
  gchar *path;

  path = g_settings_get_string (gsettings, key);
  ogmrip_fs_set_tmp_dir (path);
  g_free (path);
}

static gboolean
ogmrip_startup_finish (OGMRipApplication *app)
{
  ogmrip_application_prepare (app);

  return FALSE;
}

static void
ogmrip_startup_thread (GTask *task, GApplication *app, gpointer data, GCancellable *cancellable)
{
  OGMRipModuleEngine *module_engine;
  OGMRipProfileEngine *profile_engine;
  OGMRipProfile *default_profile;
  gchar *str;

  ogmrip_dvd_register_media ();
#ifdef HAVE_BLURAY_SUPPORT
  ogmrip_bluray_register_media ();
#endif
  ogmrip_file_register_media ();

  ogmrip_hardsub_register_codec ();
  // ogmrip_novideo_register_codec ();

  module_engine = ogmrip_module_engine_get_default ();
  g_object_set_data_full (G_OBJECT (app), "module-engine", module_engine, g_object_unref);

  str = g_build_filename (OGMRIP_LIB_DIR, "ogmrip", "plugins", NULL);
  ogmrip_module_engine_add_path (module_engine, str, NULL);
  g_free (str);

  str = g_build_filename (g_get_user_data_dir (), "ogmrip", "plugins", NULL);
  ogmrip_module_engine_add_path (module_engine, str, NULL);
  g_free (str);

  str = g_build_filename (OGMRIP_LIB_DIR, "ogmrip", "extensions", NULL);
  ogmrip_module_engine_add_path (module_engine, str, NULL);
  g_free (str);

  str = g_build_filename (g_get_user_data_dir (), "ogmrip", "extensions", NULL);
  ogmrip_module_engine_add_path (module_engine, str, NULL);
  g_free (str);

  profile_engine = ogmrip_profile_engine_get_default ();
  g_object_set_data_full (G_OBJECT (app), "profile-engine", profile_engine, g_object_unref);

  str = g_build_filename (OGMRIP_DATA_DIR, "ogmrip", "profiles", NULL);
  ogmrip_profile_engine_add_path (profile_engine, str);
  g_free (str);

  str = g_build_filename (g_get_user_data_dir (), "ogmrip", "profiles", NULL);
  ogmrip_profile_engine_add_path (profile_engine, str);
  g_free (str);

  str = g_settings_get_string (settings, OGMRIP_SETTINGS_PROFILE);
  default_profile = strlen (str) > 0 ? ogmrip_profile_engine_get (profile_engine, str) : NULL;
  g_free (str);

  if (default_profile)
    g_object_unref (default_profile);
  else
  {
    GSList *list;

    list = ogmrip_profile_engine_get_list (profile_engine);
    if (list)
    {
      gchar *name;

      g_object_get (list->data, "name", &name, NULL);
      g_settings_set_string (settings, OGMRIP_SETTINGS_PROFILE, name);
      g_free (name);
    }
    g_slist_foreach (list, (GFunc) g_object_unref, NULL);
    g_slist_free (list);
  }

  g_main_context_invoke (NULL, (GSourceFunc) ogmrip_startup_finish, app);

  g_task_return_pointer (task, NULL, NULL);

  g_object_unref (task);
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
  GTask *task;
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

  task = g_task_new (app, NULL, NULL, NULL);
  g_task_run_in_thread (task, (GTaskThreadFunc) ogmrip_startup_thread);
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

  setlocale (LC_ALL, "");

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

