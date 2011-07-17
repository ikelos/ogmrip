/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-profile.h"
#include "ogmrip-profile-keys.h"
#include "ogmrip-marshal.h"

#include <glib/gi18n.h>

#include <libxml/parser.h>

#define OGMRIP_PROFILE_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_PROFILE, OGMRipProfilePriv))

typedef struct
{
  GSettings *settings;
  gchar *name;
} OGMRipProfileSection;

struct _OGMRipProfilePriv
{
  gchar *name;
  GSList *sections;
};

enum 
{
  PROP_0,
  PROP_NAME
};

static void ogmrip_profile_constructed  (GObject    *gobject);
static void ogmrip_profile_dispose      (GObject    *gobject);
static void ogmrip_profile_get_property (GObject    *gobject,
                                         guint      property_id,
                                         GValue     *value,
                                         GParamSpec *pspec);


static gboolean
ogmrip_profile_has_schema (const gchar *schema)
{
  const gchar * const *schemas;
  guint i;
/*
  schemas = g_settings_list_relocatable_schemas ();
*/
  schemas = g_settings_list_schemas ();

  for (i = 0; schemas[i]; i ++)
    if (g_str_equal (schemas[i], schema))
      return TRUE;

  return FALSE;
}

static OGMRipProfileSection *
ogmrip_profile_section_new (OGMRipProfile *profile, const gchar *name)
{
  OGMRipProfileSection *section;

  section = g_new0 (OGMRipProfileSection, 1);
  section->name = g_strdup (name);

  section->settings = g_settings_get_child (G_SETTINGS (profile), name);

  return section;
}

static OGMRipProfileSection *
ogmrip_profile_section_new_for_codec (OGMRipProfile *profile, const gchar *path, const gchar *name)
{
  OGMRipProfileSection *section;
  gchar *schema, *new_path;

  schema = g_strconcat ("org.ogmrip.", name, NULL);
  if (!ogmrip_profile_has_schema (schema))
  {
    g_free (schema);
    return NULL;
  }

  section = g_new0 (OGMRipProfileSection, 1);
  section->name = g_strdup (name);

  new_path = g_strconcat (path, name, "/", NULL);
  section->settings = g_settings_new_with_path (schema, new_path);
  g_free (new_path);

  return section;
}

static void
ogmrip_profile_section_free (OGMRipProfileSection *section)
{
  g_object_unref (section->settings);
  g_free (section->name);
  g_free (section);
}

G_DEFINE_TYPE (OGMRipProfile, ogmrip_profile, G_TYPE_SETTINGS)

static void
ogmrip_profile_class_init (OGMRipProfileClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_profile_constructed;
  gobject_class->dispose = ogmrip_profile_dispose;
  gobject_class->get_property = ogmrip_profile_get_property;

  g_object_class_install_property (gobject_class, PROP_NAME,
      g_param_spec_string ("name", "name", "name",
        NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipProfilePriv));
}

static void
ogmrip_profile_init (OGMRipProfile *profile)
{
  profile->priv = OGMRIP_PROFILE_GET_PRIVATE (profile);
}

static void
ogmrip_profile_constructed (GObject *gobject)
{
  OGMRipProfile *profile = OGMRIP_PROFILE (gobject);
  OGMRipProfileSection *section;
  gchar *path, *name;

  G_OBJECT_CLASS (ogmrip_profile_parent_class)->constructed (gobject);

  g_object_get (profile, "path", &path, NULL);
  profile->priv->name = g_path_get_basename (path);
  g_free (path);

  section = ogmrip_profile_section_new (profile, "subp");
  profile->priv->sections = g_slist_prepend (profile->priv->sections, section);

  name = g_settings_get_string (section->settings, OGMRIP_PROFILE_CODEC);
  section = ogmrip_profile_section_new_for_codec (profile, path, name);
  g_free (name);

  if (section)
    profile->priv->sections = g_slist_prepend (profile->priv->sections, section);

  section = ogmrip_profile_section_new (profile, "audio");
  profile->priv->sections = g_slist_prepend (profile->priv->sections, section);

  name = g_settings_get_string (section->settings, OGMRIP_PROFILE_CODEC);
  section = ogmrip_profile_section_new_for_codec (profile, path, name);
  g_free (name);

  if (section)
    profile->priv->sections = g_slist_prepend (profile->priv->sections, section);

  section = ogmrip_profile_section_new (profile, "video");
  profile->priv->sections = g_slist_prepend (profile->priv->sections, section);

  name = g_settings_get_string (section->settings, OGMRIP_PROFILE_CODEC);
  section = ogmrip_profile_section_new_for_codec (profile, path, name);
  g_free (name);

  if (section)
    profile->priv->sections = g_slist_prepend (profile->priv->sections, section);

  section = ogmrip_profile_section_new (profile, "general");
  profile->priv->sections = g_slist_prepend (profile->priv->sections, section);

  name = g_settings_get_string (section->settings, OGMRIP_PROFILE_CONTAINER);
  section = ogmrip_profile_section_new_for_codec (profile, path, name);
  g_free (name);

  if (section)
    profile->priv->sections = g_slist_prepend (profile->priv->sections, section);
}

static void
ogmrip_profile_dispose (GObject *gobject)
{
  OGMRipProfile *profile = OGMRIP_PROFILE (gobject);

  if (profile->priv->sections)
  {
    g_slist_foreach (profile->priv->sections,
        (GFunc) ogmrip_profile_section_free, NULL);
    g_slist_free (profile->priv->sections);
    profile->priv->sections = NULL;
  }

  G_OBJECT_CLASS (ogmrip_profile_parent_class)->dispose (gobject);
}

static void
ogmrip_profile_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipProfile *profile = OGMRIP_PROFILE (gobject);

  switch (property_id) 
  {
    case PROP_NAME:
      g_value_set_string (value, profile->priv->name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

OGMRipProfile *
ogmrip_profile_new (const gchar *name)
{
  OGMRipProfile *profile;
  gchar *path;

  g_return_val_if_fail (name != NULL, NULL);

  path = g_strconcat (OGMRIP_PROFILE_PATH, name, "/", NULL);
  profile = g_object_new (OGMRIP_TYPE_PROFILE, "schema", "org.ogmrip.profile", "path", path, NULL);
  g_free (path);

  return profile;
}

static gint
xmlReadInputStream (GInputStream *istream, gchar *buffer, gint len)
{
  return g_input_stream_read (istream, buffer, len, NULL, NULL);
}

static gint
xmlCloseInputStream (GInputStream *istream)
{
  if (!g_input_stream_close (istream, NULL, NULL))
    return -1;

  return 0;
}

static void
ogmrip_profile_parse_key (GSettings *settings, xmlNode *node)
{
  GError *error = NULL;
  GVariant *variant1, *variant2;
  const GVariantType *type;
  gchar *name, *value;

  name = (gchar *) xmlGetProp (node, BAD_CAST "name");

  variant1 = g_settings_get_value (settings, name);
  type = g_variant_get_type (variant1);

  value = (gchar *) xmlNodeGetContent (node);
  variant2 = g_variant_parse (type, value, NULL, NULL, &error);
  g_free (value);

  if (variant2)
    g_settings_set_value (settings, name, variant2);
  else
  {
    g_error ("%s", error->message);
    g_error_free (error);
  }

  g_free (name);
  g_variant_unref (variant1);
}

static void
ogmrip_profile_parse_section (OGMRipProfile *profile, xmlNode *node)
{
  GSettings *settings;
  gchar *name;

  name = (gchar *) xmlGetProp (node, BAD_CAST "name");
  settings = ogmrip_profile_get_child (profile, name);
  g_free (name);

  for (node = node->children; node; node = node->next)
    ogmrip_profile_parse_key (settings, node);
}

OGMRipProfile *
ogmrip_profile_new_from_file (GFile *file, GError **error)
{
  OGMRipProfile *profile = NULL;
  GFileInputStream *istream;

  xmlDoc *doc;
  xmlNode *root, *node;
  gchar *name;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  xmlKeepBlanksDefault (0);

  istream = g_file_read (file, NULL, error);
  if (!istream)
    return NULL;

  doc = xmlReadIO ((xmlInputReadCallback) xmlReadInputStream,
      (xmlInputCloseCallback) xmlCloseInputStream, istream, NULL, NULL, 0);

  if (!doc)
  {
    gchar *filename;

    filename = g_file_get_basename (file);
    g_set_error (error, 0, 0, _("'%s' does not contain a valid profile"), filename);
    g_free (filename);

    g_object_unref (istream);

    return FALSE;
  }

  root = xmlDocGetRootElement (doc);
  if (!root || !xmlStrEqual (root->name, BAD_CAST "profile"))
  {
    gchar *filename;

    filename = g_file_get_basename (file);
    g_set_error (error, 0, 0, _("'%s' does not contain a valid profile"), filename);
    g_free (filename);

    goto cleanup;
  }

  name = (gchar *) xmlGetProp (root, BAD_CAST "name");
  g_assert (name != NULL);

  profile = ogmrip_profile_new (name);

  for (node = root->children; node; node = node->next)
  {
    if (xmlStrEqual (node->name, BAD_CAST "section"))
      ogmrip_profile_parse_section (profile, node);
    else if (xmlStrEqual (node->name, BAD_CAST "key"))
      ogmrip_profile_parse_key (G_SETTINGS (profile), node);
  }

cleanup:
  xmlFreeDoc (doc);
  xmlCleanupParser ();

  g_object_unref (istream);

  return profile;
}

static void
ogmrip_profile_dump_key (GSettings *settings, xmlNode *root, const gchar *key)
{
  xmlNode *node;
  GVariant *variant;
  gchar *str;

  node = xmlNewNode (NULL, BAD_CAST "key");
  xmlAddChild (root, node);

  xmlSetProp (node, BAD_CAST "name", BAD_CAST key);

  variant = g_settings_get_value (settings, key);

  str = g_variant_print (variant, FALSE);
  xmlNodeSetContent (node, BAD_CAST str);
  g_free (str);

  g_variant_unref (variant);
}

static void
ogmrip_profile_dump_section (GSettings *settings, xmlNode *root)
{
  gchar **keys;
  guint i;

  keys = g_settings_list_keys (settings);
  for (i = 0; keys[i]; i ++)
  {
    ogmrip_profile_dump_key (settings, root, keys[i]);
    g_free (keys[i]);
  }
  g_free (keys);
}

gboolean
ogmrip_profile_dump (OGMRipProfile *profile, GFile *file, GError **error)
{
  xmlNode *root, *node;
  GSettings *settings;
  gchar **sections;
  guint i;

  g_return_val_if_fail (OGMRIP_IS_PROFILE (profile), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  root = xmlNewNode (NULL, BAD_CAST "profile");

  xmlSetProp (root, BAD_CAST "name", BAD_CAST profile->priv->name);

  ogmrip_profile_dump_section (G_SETTINGS (profile), root);

  sections = g_settings_list_children (G_SETTINGS (profile));
  for (i = 0; sections[i]; i ++)
  {
    settings = g_settings_get_child (G_SETTINGS (profile), sections[i]);

    node = xmlNewNode (NULL, BAD_CAST "section");
    xmlAddChild (root, node);

    xmlSetProp (node, BAD_CAST "name", BAD_CAST sections[i]);
    g_free (sections[i]);

    ogmrip_profile_dump_section (settings, node);
  }
  g_free (sections);

  return FALSE;
}

void
ogmrip_profile_get (OGMRipProfile *profile, const gchar *section, const gchar *key, const gchar *format, ...)
{
  GSettings *settings;
  GVariant *value;
  va_list ap;

  g_return_if_fail (OGMRIP_IS_PROFILE (profile));

  settings = ogmrip_profile_get_child (profile, section);
  g_assert (settings != NULL);

  value = g_settings_get_value (settings, key);

  va_start (ap, format);
  g_variant_get_va (value, format, NULL, &ap);
  va_end (ap);

  g_variant_unref (value);
}

gboolean
ogmrip_profile_set (OGMRipProfile *profile, const gchar *section, const gchar *key, const gchar *format, ...)
{
  GSettings *settings;
  GVariant *value;
  va_list ap;

  g_return_val_if_fail (OGMRIP_IS_PROFILE (profile), FALSE);

  va_start (ap, format);
  value = g_variant_new_va (format, NULL, &ap);
  va_end (ap);

  settings = ogmrip_profile_get_child (profile, section);
  g_assert (settings != NULL);

  return g_settings_set_value (settings, key, value);
}

GSettings *
ogmrip_profile_get_child (OGMRipProfile *profile, const gchar *name)
{
  OGMRipProfileSection *section;
  GSList *link;
/*
  gchar *path, *new_path;
*/
  g_return_val_if_fail (OGMRIP_IS_PROFILE (profile), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (link = profile->priv->sections; link; link = link->next)
  {
    section = link->data;
    if (g_str_equal (section->name, name))
      return section->settings;
  }
/*
  g_return_val_if_fail (schema != NULL, NULL);

  section = g_new0 (OGMRipProfileSection, 1);
  section->name = g_strdup (name);

  g_object_get (profile, "path", &path, NULL);
  new_path = g_build_filename (path, name, NULL);
  g_free (path);

  section->settings = g_settings_new_with_path (schema, new_path);
  g_free (new_path);

  profile->priv->sections = g_slist_append (profile->priv->sections, section);

  return section->settings;
*/
  return NULL;
}

