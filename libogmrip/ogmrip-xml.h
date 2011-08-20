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

#ifndef __OGMRIP_XML_H__
#define __OGMRIP_XML_H__

#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _OGMRipXML     OGMRipXML;

OGMRipXML *     ogmrip_xml_new           (void);
OGMRipXML *     ogmrip_xml_new_from_file (GFile              *file,
                                          GError             **error);
void            ogmrip_xml_free          (OGMRipXML          *xml);
gboolean        ogmrip_xml_save          (OGMRipXML          *xml,
                                          GFile              *file,
                                          GError             **error);
void            ogmrip_xml_reset         (OGMRipXML          *xml);
gboolean        ogmrip_xml_previous      (OGMRipXML          *xml);
gboolean        ogmrip_xml_next          (OGMRipXML          *xml);
gboolean        ogmrip_xml_children      (OGMRipXML          *xml);
gboolean        ogmrip_xml_parent        (OGMRipXML          *xml);
void            ogmrip_xml_append        (OGMRipXML          *xml,
                                          const gchar        *name);
const gchar *   ogmrip_xml_get_name      (OGMRipXML          *xml);
void            ogmrip_xml_get_value     (OGMRipXML          *xml,
                                          const gchar        *property,
                                          GValue             *value);
void            ogmrip_xml_set_value     (OGMRipXML          *xml,
                                          const gchar        *property,
                                          const GValue       *value);
GVariant *      ogmrip_xml_get_variant   (OGMRipXML          *xml,
                                          const gchar        *property,
                                          const GVariantType *type);
void            ogmrip_xml_set_variant   (OGMRipXML          *xml,
                                          const gchar        *property,
                                          GVariant           *value);
gboolean        ogmrip_xml_get_boolean   (OGMRipXML          *xml,
                                          const gchar        *property);
void            ogmrip_xml_set_boolean   (OGMRipXML          *xml,
                                          const gchar        *property,
                                          gboolean           value);
gint            ogmrip_xml_get_int       (OGMRipXML          *xml,
                                          const gchar        *property);
void            ogmrip_xml_set_int       (OGMRipXML          *xml,
                                          const gchar        *property,
                                          gint               value);
guint           ogmrip_xml_get_uint      (OGMRipXML          *xml,
                                          const gchar        *property);
void            ogmrip_xml_set_uint      (OGMRipXML          *xml,
                                          const gchar        *property,
                                          guint              value);
gdouble         ogmrip_xml_get_double    (OGMRipXML          *xml,
                                          const gchar        *property);
void            ogmrip_xml_set_double    (OGMRipXML          *xml,
                                          const gchar        *property,
                                          gdouble            value);
gchar *         ogmrip_xml_get_string    (OGMRipXML          *xml,
                                          const gchar        *property);
void            ogmrip_xml_set_string    (OGMRipXML          *xml,
                                          const gchar        *property,
                                          const gchar        *value);

G_END_DECLS

#endif /* __OGMRIP_XML_H__ */

