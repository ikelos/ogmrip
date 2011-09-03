/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-profiles.h"
#include "ogmrip-settings.h"

#include <glib/gi18n.h>

gboolean
ogmrip_profiles_import (const gchar *filename, GError **error)
{
/*
  gchar *section;

  if (!ogmrip_settings_import (settings, filename, &section, error))
    return FALSE;

  if (!ogmrip_profiles_check_profile (section, error))
  {
    ogmrip_settings_remove_section (settings, section);
    return FALSE;
  }
*/
  return TRUE;
}

gboolean
ogmrip_profiles_import_all (const gchar *dirname, GError **error)
{
  GError *tmp_error = NULL;
  const gchar *filename;
  gchar *fullname;
  GDir *dir;

  dir = g_dir_open (dirname, 0, &tmp_error);
  if (!dir)
  {
    g_propagate_error (error, tmp_error);
    return FALSE;
  }

  while ((filename = g_dir_read_name (dir)))
  {
    fullname = g_build_filename (dirname, filename, NULL);
/*
    ogmrip_profiles_import (fullname, NULL);
*/
    g_free (fullname);
  }

  g_dir_close (dir);

  return TRUE;
}

gboolean
ogmrip_profiles_reload (const gchar *dirname, const gchar *profile, GError **error)
{
  GError *tmp_error = NULL;
  const gchar *filename;
  gchar *fullname;
  GDir *dir;

  g_return_val_if_fail (dirname != NULL, FALSE);
  g_return_val_if_fail (profile != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  dir = g_dir_open (dirname, 0, &tmp_error);
  if (!dir)
  {
    g_propagate_error (error, tmp_error);
    return FALSE;
  }

  while ((filename = g_dir_read_name (dir)))
  {
    fullname = g_build_filename (dirname, filename, NULL);
    ogmrip_profiles_import (fullname, NULL);
    g_free (fullname);
  }

  g_dir_close (dir);

  return TRUE;
}

gboolean
ogmrip_profiles_check_profile (OGMRipProfile *profile, GError **error)
{
  GType type;
  gchar *value;
  gboolean retval;

  ogmrip_profile_get (profile, OGMRIP_PROFILE_GENERAL, OGMRIP_PROFILE_CONTAINER, "s", &value);

  type = ogmrip_profile_get_container_type (profile, value);
  retval = type != G_TYPE_NONE;

  if (type == G_TYPE_NONE)
    g_set_error (error, 0, 0, _("The container '%s' is not available"), value);

  if (value)
    g_free (value);

  if (retval)
  {
    ogmrip_profile_get (profile, OGMRIP_PROFILE_VIDEO, OGMRIP_PROFILE_CODEC, "s", &value);

    if (g_str_equal (value, "novideo"))
      retval = TRUE;
    else
    {
      type = ogmrip_profile_get_video_codec_type (profile, value);
      retval = type != G_TYPE_NONE;

      if (type == G_TYPE_NONE)
        g_set_error (error, 0, 0, _("The video codec '%s' is not available"), value);
    }

    if (value)
      g_free (value);
  }

  if (retval)
  {
    ogmrip_profile_get (profile, OGMRIP_PROFILE_AUDIO, OGMRIP_PROFILE_CODEC, "s", &value);

    type = ogmrip_profile_get_audio_codec_type (profile, value);
    retval = type != G_TYPE_NONE;

    if (type == G_TYPE_NONE)
      g_set_error (error, 0, 0, _("The audio codec '%s' is not available"), value);

    if (value)
      g_free (value);
  }

  if (retval)
  {
    ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_CODEC, "s", &value);

    type = ogmrip_profile_get_subp_codec_type (profile, value);
    retval = type != G_TYPE_NONE;

    if (type == G_TYPE_NONE)
      g_set_error (error, 0, 0, _("The subtitles codec '%s' is not available"), value);

    if (value)
      g_free (value);
  }

  return retval;
}
/*
static gboolean
ogmrip_profiles_parse (xmlNode *node, GList **list)
{
  xmlChar *base, *new_version;
  gchar *old_version = NULL;

  if (!g_str_equal (node->name, "profile"))
    return FALSE;

  base = xmlGetProp (node, (xmlChar *) "base");
  if (!base)
    return FALSE;

  new_version = xmlGetProp (node, (xmlChar *) "version");

  ogmrip_settings_get (settings, (gchar *) base, "version", &old_version, NULL);

  if (ogmrip_settings_has_section (settings, (gchar *) base) &&
      ogmrip_settings_compare_versions ((gchar *) new_version, old_version) > 0)
    *list = g_list_prepend (*list, g_strdup_printf ("%s@%s", base, new_version ? new_version : (xmlChar *) "1.0"));

  xmlFree (base);

  if (new_version)
    xmlFree (new_version);

  if (old_version)
    g_free (old_version);

  return FALSE;
}
*/
GList *
ogmrip_profiles_check_updates (GList *list, const gchar *dirname, GError **error)
{
  GError *tmp_error = NULL;
  const gchar *filename;
  gchar *fullname;
  GList *tmp_list;
  GDir *dir;

  dir = g_dir_open (dirname, 0, &tmp_error);
  if (!dir)
  {
    g_propagate_error (error, tmp_error);
    return list;
  }

  tmp_list = list;

  while ((filename = g_dir_read_name (dir)))
  {
    fullname = g_build_filename (dirname, filename, NULL);
/*
    ogmrip_settings_parse (settings, fullname,
        (OGMRipParseFunc) ogmrip_profiles_parse, &tmp_list, NULL);
*/
    g_free (fullname);
  }

  g_dir_close (dir);

  return tmp_list;
}

