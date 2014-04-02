/* OGMRipBase - A foundation library for OGMRip
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

#include "ogmrip-xml.h"

#include <glib/gi18n-lib.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlsave.h>

struct _OGMRipXML
{
  xmlDoc *doc;
  xmlNode *root;
  xmlNode *node;

  GFileInputStream *istream;
};

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

static int
xmlWriteOutputStream (GOutputStream *ostream, const gchar *buffer, int len)
{
  return g_output_stream_write (ostream, buffer, len, NULL, NULL);
}

static int
xmlCloseOutputStream (GOutputStream *ostream)
{
  if (g_output_stream_close (ostream, NULL, NULL))
    return 0;

  return -1;
}

OGMRipXML *
ogmrip_xml_new (void)
{
  OGMRipXML *xml;

  xml = g_new0 (OGMRipXML, 1);
  xml->doc = xmlNewDoc (BAD_CAST "1.0");

  return xml;
}

OGMRipXML *
ogmrip_xml_new_from_file (GFile *file, GError **error)
{
  OGMRipXML *xml;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  xmlKeepBlanksDefault (0);

  xml = g_new0 (OGMRipXML, 1);

  xml->istream = g_file_read (file, NULL, error);
  if (!xml->istream)
  {
    ogmrip_xml_free (xml);
    return NULL;
  }

  xml->doc = xmlReadIO ((xmlInputReadCallback) xmlReadInputStream,
      (xmlInputCloseCallback) xmlCloseInputStream, xml->istream, NULL, "UTF-8",
      XML_PARSE_NOENT | XML_PARSE_NOBLANKS);

  xml->root = xml->doc ? xmlDocGetRootElement (xml->doc) : NULL;
  if (!xml->root)
  {
    gchar *filename;

    filename = g_file_get_basename (file);
    g_set_error (error, OGMRIP_XML_ERROR, OGMRIP_XML_ERROR_INVALID, _("'%s' is not a valid XML file"), filename);
    g_free (filename);

    ogmrip_xml_free (xml);

    return NULL;
  }
  xml->node = xml->root;

  return xml;
}

void
ogmrip_xml_free (OGMRipXML *xml)
{
  g_return_if_fail (xml != NULL);

  if (xml->doc)
    xmlFreeDoc (xml->doc);

  if (xml->istream)
    g_object_unref (xml->istream);

  g_free (xml);
}

gboolean
ogmrip_xml_save (OGMRipXML *xml, GFile *file, GError **error)
{
  GFileOutputStream *ostream;
  xmlSaveCtxt *ctxt;
  gint len = -1;

  g_return_val_if_fail (xml != NULL, FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  ostream = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, error);
  if (!ostream)
    return FALSE;

  ctxt = xmlSaveToIO ((xmlOutputWriteCallback) xmlWriteOutputStream,
      (xmlOutputCloseCallback) xmlCloseOutputStream, ostream, "UTF-8", XML_SAVE_FORMAT);
  if (ctxt)
  {
    if (xml->doc)
      len = xmlSaveDoc (ctxt, xml->doc);
    xmlSaveFlush (ctxt);
    xmlSaveClose (ctxt);
  }

  g_object_unref (ostream);

  return len == 0;
}

void
ogmrip_xml_reset (OGMRipXML *xml)
{
  xml->node = xml->root;
}

gboolean
ogmrip_xml_previous (OGMRipXML *xml)
{
  if (!xml->node || !xml->node->prev)
    return FALSE;

  xml->node = xml->node->prev;

  return TRUE;
}

gboolean
ogmrip_xml_next (OGMRipXML *xml)
{
  if (!xml->node || !xml->node->next)
    return FALSE;

  xml->node = xml->node->next;

  return TRUE;
}

gboolean
ogmrip_xml_children (OGMRipXML *xml)
{
  if (!xml->node || !xml->node->children)
    return FALSE;

  xml->node = xml->node->children;

  return TRUE;
}

gboolean
ogmrip_xml_parent (OGMRipXML *xml)
{
  if (!xml->node || !xml->node->parent)
    return FALSE;

  xml->node = xml->node->parent;

  return TRUE;
}

const gchar *
ogmrip_xml_get_name (OGMRipXML *xml)
{
  if (!xml->node)
    return NULL;

  return (const gchar *) xml->node->name;
}

void
ogmrip_xml_append (OGMRipXML *xml, const gchar *name)
{
  xmlNode *node;

  node = xmlNewNode (NULL, BAD_CAST (name));
  if (xml->node)
    xmlAddChild (xml->node, node);
  else
  {
    xmlDocSetRootElement (xml->doc, node);
    xml->root = node;
  }

  xml->node = node;
}

void
ogmrip_xml_get_value (OGMRipXML *xml, const gchar *property, GValue *value)
{
  switch (value->g_type)
  {
    case G_TYPE_BOOLEAN:
      g_value_set_boolean (value, ogmrip_xml_get_boolean (xml, property));
      break;
    case G_TYPE_INT:
      g_value_set_int (value, ogmrip_xml_get_int (xml, property));
      break;
    case G_TYPE_UINT:
      g_value_set_uint (value, ogmrip_xml_get_uint (xml, property));
      break;
    case G_TYPE_DOUBLE:
      g_value_set_double (value, ogmrip_xml_get_double (xml, property));
      break;
    case G_TYPE_STRING:
      g_value_take_string (value, ogmrip_xml_get_string (xml, property));
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

void
ogmrip_xml_set_value (OGMRipXML *xml, const gchar *property, const GValue *value)
{
  switch (value->g_type)
  {
    case G_TYPE_BOOLEAN:
      ogmrip_xml_set_boolean (xml, property, g_value_get_boolean (value));
      break;
    case G_TYPE_INT:
      ogmrip_xml_set_int (xml, property, g_value_get_int (value));
      break;
    case G_TYPE_UINT:
      ogmrip_xml_set_uint (xml, property, g_value_get_uint (value));
      break;
    case G_TYPE_DOUBLE:
      ogmrip_xml_set_double (xml, property, g_value_get_double (value));
      break;
    case G_TYPE_STRING:
      ogmrip_xml_set_string (xml, property, g_value_get_string (value));
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

GVariant *
ogmrip_xml_get_variant (OGMRipXML *xml, const gchar *property, const gchar *format)
{
  GVariant *variant;
  gchar *str;

  str = ogmrip_xml_get_string (xml, property);
  variant = g_variant_parse ((const GVariantType *) format, str, NULL, NULL, NULL);
  g_free (str);

  return variant;
}

void
ogmrip_xml_set_variant (OGMRipXML *xml, const gchar *property, GVariant *value)
{
  gchar *str;

  str = g_variant_print (value, FALSE);
  ogmrip_xml_set_string (xml, property, str);
  g_free (str);
}

gboolean
ogmrip_xml_get_boolean (OGMRipXML *xml, const gchar *property)
{
  gchar *str;
  gboolean val;

  str = ogmrip_xml_get_string (xml, property);
  if (!str)
    return FALSE;

  val = g_str_equal (str, "true") ? TRUE : FALSE;
  g_free (str);

  return val;
}

void
ogmrip_xml_set_boolean (OGMRipXML *xml, const gchar *property, gboolean value)
{
  ogmrip_xml_set_string (xml, property, value ? "true" : "false");
}

gint
ogmrip_xml_get_int (OGMRipXML *xml, const gchar *property)
{
  gchar *str;
  gint val;

  str = ogmrip_xml_get_string (xml, property);
  if (!str)
    return 0;

  val = strtol (str, NULL, 10);
  g_free (str);

  return val;
}

void
ogmrip_xml_set_int (OGMRipXML *xml, const gchar *property, gint value)
{
  gchar *str;

  str = g_strdup_printf ("%d", value);
  ogmrip_xml_set_string (xml, property, str);
  g_free (str);
}

guint
ogmrip_xml_get_uint (OGMRipXML *xml, const gchar *property)
{
  gchar *str;
  guint val;

  str = ogmrip_xml_get_string (xml, property);
  if (!str)
    return 0;

  val = strtoul (str, NULL, 10);
  g_free (str);

  return val;
}

void
ogmrip_xml_set_uint (OGMRipXML *xml, const gchar *property, guint value)
{
  gchar *str;

  str = g_strdup_printf ("%u", value);
  ogmrip_xml_set_string (xml, property, str);
  g_free (str);
}

gdouble
ogmrip_xml_get_double (OGMRipXML *xml, const gchar *property)
{
  gchar *str;
  gdouble val;

  str = ogmrip_xml_get_string (xml, property);
  if (!str)
    return 0.0;

  val = g_ascii_strtod (str, NULL);
  g_free (str);

  return val;
}

void
ogmrip_xml_set_double (OGMRipXML *xml, const gchar *property, gdouble value)
{
  gchar *str;

  str = g_new0 (gchar, G_ASCII_DTOSTR_BUF_SIZE);
  g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, value);
  ogmrip_xml_set_string (xml, property, str);
  g_free (str);
}

gchar *
ogmrip_xml_get_string (OGMRipXML *xml, const gchar *property)
{
  if (!xml->node)
    return NULL;

  if (property)
    return (gchar *) xmlGetProp (xml->node, BAD_CAST property);

  return (gchar *) xmlNodeGetContent (xml->node);
}

void
ogmrip_xml_set_string (OGMRipXML *xml, const gchar *property, const gchar *value)
{
  if (xml->node)
  {
    if (property)
      xmlSetProp (xml->node, BAD_CAST property, BAD_CAST value);
    else
      xmlNodeSetContent (xml->node, BAD_CAST value);
  }
}

GQuark
ogmrip_xml_error_quark (void)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_static_string ("ogmrip-xml-error-quark");

  return quark;
}

