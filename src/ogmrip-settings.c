/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-settings.h"

#include <string.h>

GSettings *settings;

GType
ogmrip_profile_get_container_type (OGMRipProfile *profile, const gchar *name)
{
  GType container;
  gchar *str;

  if (name)
    str = g_strdup (name);
  else
    ogmrip_profile_get (profile, OGMRIP_PROFILE_GENERAL, OGMRIP_PROFILE_CONTAINER, "s", &str);

  container = ogmrip_plugin_get_container_by_name (str);
  g_free (str);

  return container;
}

GType
ogmrip_profile_get_video_codec_type (OGMRipProfile *profile, const gchar *name)
{
  GType codec;
  gchar *str;

  if (name)
    str = g_strdup (name);
  else
    ogmrip_profile_get (profile, OGMRIP_PROFILE_VIDEO, OGMRIP_PROFILE_CODEC, "s", &str);

  codec = ogmrip_plugin_get_video_codec_by_name (str);
  g_free (str);

  return codec;
}

GType
ogmrip_profile_get_audio_codec_type (OGMRipProfile *profile, const gchar *name)
{
  GType codec;
  gchar *str;

  if (name)
    str = g_strdup (name);
  else
    ogmrip_profile_get (profile, OGMRIP_PROFILE_AUDIO, OGMRIP_PROFILE_CODEC, "s", &str);

  codec = ogmrip_plugin_get_audio_codec_by_name (str);
  g_free (str);

  return codec;
}

GType
ogmrip_profile_get_subp_codec_type (OGMRipProfile *profile, const gchar *name)
{
  GType codec;
  gchar *str;

  if (name)
    str = g_strdup (name);
  else
    ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_CODEC, "s", &str);

  codec = ogmrip_plugin_get_subp_codec_by_name (str);
  g_free (str);

  return codec;
}

void
ogmrip_settings_init (void)
{
  if (!settings)
  {
    gchar *path;

    settings = g_settings_new ("org.ogmrip.preferences");

    path = g_settings_get_string (settings, OGMRIP_SETTINGS_OUTPUT_DIR);
    if (!strlen (path))
      g_settings_set_string (settings, OGMRIP_SETTINGS_OUTPUT_DIR, g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS));
    g_free (path);

    path = g_settings_get_string (settings, OGMRIP_SETTINGS_TMP_DIR);
    if (!strlen (path))
      g_settings_set_string (settings, OGMRIP_SETTINGS_TMP_DIR, g_get_tmp_dir ());
    g_free (path);
  }
}

void
ogmrip_settings_uninit (void)
{
  if (settings)
  {
    g_object_unref (settings);
    settings = NULL;
  }
}

