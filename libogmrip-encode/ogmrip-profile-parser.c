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

#include "ogmrip-profile-parser.h"
#include "ogmrip-profile-keys.h"

static void
ogmrip_profile_parse_key (GSettings *settings, OGMRipXML *xml)
{
  GVariant *variant1, *variant2;
  const GVariantType *type;
  gchar *name;

  name = ogmrip_xml_get_string (xml, "name");

  variant1 = g_settings_get_value (settings, name);
  type = g_variant_get_type (variant1);

  variant2 = ogmrip_xml_get_variant (xml, NULL, (const gchar *) type);
  if (variant2)
    g_settings_set_value (settings, name, variant2);

  g_free (name);
  g_variant_unref (variant1);
}

static void
ogmrip_profile_parse_section (OGMRipProfile *profile, OGMRipXML *xml)
{
  GSettings *settings;
  gchar *name;

  name = ogmrip_xml_get_string (xml, "name");

  settings = ogmrip_profile_get_child (profile, name);
  if (!settings)
    g_warning ("No child for %s", name);
  else
  {
    if (ogmrip_xml_children (xml))
    {
      do
      {
        ogmrip_profile_parse_key (settings, xml);
      }
      while (ogmrip_xml_next (xml));

      ogmrip_xml_parent (xml);
    }

    g_object_unref (settings);
  }

  g_free (name);
}

gboolean
ogmrip_profile_parse (OGMRipProfile *profile, OGMRipXML *xml, GError **error)
{
  GVariant *variant;

  g_return_val_if_fail (OGMRIP_IS_PROFILE (profile), FALSE);
  g_return_val_if_fail (xml != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  variant = ogmrip_xml_get_variant (xml, "version", "(uu)");
  if (variant)
    g_settings_set_value (G_SETTINGS (profile), OGMRIP_PROFILE_VERSION, variant);

  if (ogmrip_xml_children (xml))
  {
    const gchar *name;

    do
    {
      name = ogmrip_xml_get_name (xml);
      if (g_str_equal (name, "section"))
        ogmrip_profile_parse_section (profile, xml);
      else if (g_str_equal (name, "key"))
        ogmrip_profile_parse_key (G_SETTINGS (profile), xml);
    }
    while (ogmrip_xml_next (xml));

    ogmrip_xml_parent (xml);
  }

  return TRUE;
}

static void
ogmrip_profile_dump_key (GSettings *settings, OGMRipXML *xml, const gchar *key)
{
  GVariant *variant;

  ogmrip_xml_append (xml, "key");

  ogmrip_xml_set_string (xml, "name", key);

  variant = g_settings_get_value (settings, key);
  ogmrip_xml_set_variant (xml, NULL, variant);
  g_variant_unref (variant);

  ogmrip_xml_parent (xml);
}

static void
ogmrip_profile_dump_section (OGMRipProfile *profile, OGMRipXML *xml, const gchar *name)
{
  GSettings *settings;

  settings = ogmrip_profile_get_child (profile, name);
  if (settings)
  {
    gchar **keys;
    guint i;

    ogmrip_xml_append (xml, "section");

    ogmrip_xml_set_string (xml, "name", name);

    keys = g_settings_list_keys (settings);
    for (i = 0; keys[i]; i ++)
    {
      ogmrip_profile_dump_key (settings, xml, keys[i]);
      g_free (keys[i]);
    }
    g_free (keys);

    ogmrip_xml_parent (xml);

    g_object_unref (settings);
  }
}

gboolean
ogmrip_profile_dump (OGMRipProfile *profile, OGMRipXML *xml, GError **error)
{
  GVariant *variant;
  gchar *str;

  g_return_val_if_fail (OGMRIP_IS_PROFILE (profile), FALSE);
  g_return_val_if_fail (xml != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  ogmrip_xml_append (xml, "profile");

  g_object_get (profile, "name", &str, NULL);
  ogmrip_xml_set_string (xml, "name", str);
  g_free (str);

  variant = g_settings_get_value (G_SETTINGS (profile), OGMRIP_PROFILE_VERSION);
  ogmrip_xml_set_variant (xml, "version", variant);
  g_variant_unref (variant);

  ogmrip_profile_dump_key (G_SETTINGS (profile), xml, OGMRIP_PROFILE_NAME);

  ogmrip_profile_dump_section (profile, xml, OGMRIP_PROFILE_GENERAL);

  ogmrip_profile_get (profile, OGMRIP_PROFILE_GENERAL, OGMRIP_PROFILE_CONTAINER, "s", &str);
  ogmrip_profile_dump_section (profile, xml, str);
  g_free (str);

  ogmrip_profile_dump_section (profile, xml, OGMRIP_PROFILE_VIDEO);

  ogmrip_profile_get (profile, OGMRIP_PROFILE_VIDEO, OGMRIP_PROFILE_CODEC, "s", &str);
  ogmrip_profile_dump_section (profile, xml, str);
  g_free (str);

  ogmrip_profile_dump_section (profile, xml, OGMRIP_PROFILE_AUDIO);

  ogmrip_profile_get (profile, OGMRIP_PROFILE_AUDIO, OGMRIP_PROFILE_CODEC, "s", &str);
  ogmrip_profile_dump_section (profile, xml, str);
  g_free (str);

  ogmrip_profile_dump_section (profile, xml, OGMRIP_PROFILE_SUBP);

  ogmrip_profile_get (profile, OGMRIP_PROFILE_SUBP, OGMRIP_PROFILE_CODEC, "s", &str);
  ogmrip_profile_dump_section (profile, xml, str);
  g_free (str);

  return TRUE;
}

