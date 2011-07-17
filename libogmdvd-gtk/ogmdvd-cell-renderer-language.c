/* OGMDvd - A wrapper library around libdvdread
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

/**
 * SECTION: ogmdvd-cell-renderer-language
 * @title: OGMDvdCellRendererLanguage
 * @include: ogmdvd-cell-renderer-language.h
 * @short_description: Renders languages in a cell
 */

#include "ogmdvd-cell-renderer-language.h"

#include "ogmdvd-enums.h"

#define OGMDVD_CELL_RENDERER_LANGUAGE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), OGMDVD_TYPE_CELL_RENDERER_LANGUAGE, OGMDvdCellRendererLanguagePriv))

struct _OGMDvdCellRendererLanguagePriv
{
  guint language;
  gchar *default_text;
};

static void ogmdvd_cell_renderer_language_finalize      (GObject                    *object);
static void ogmdvd_cell_renderer_language_get_property  (GObject                    *object,
                                                         guint                       param_id,
                                                         GValue                     *value,
                                                         GParamSpec                 *pspec);

static void ogmdvd_cell_renderer_language_set_property  (GObject                    *object,
                                                         guint                       param_id,
                                                         const GValue               *value,
                                                         GParamSpec                 *pspec);

enum
{
  PROP_0,
  PROP_LANGUAGE,
  PROP_DEFAULT
};

extern const gchar *ogmdvd_languages[][3];
extern const guint ogmdvd_nlanguages;

G_DEFINE_TYPE (OGMDvdCellRendererLanguage, ogmdvd_cell_renderer_language, GTK_TYPE_CELL_RENDERER_TEXT)

static void
ogmdvd_cell_renderer_language_init (OGMDvdCellRendererLanguage *cell)
{
  const gchar *lang;

  cell->priv = OGMDVD_CELL_RENDERER_LANGUAGE_GET_PRIVATE (cell);

  lang = ogmdvd_languages[0][OGMDVD_LANGUAGE_ISO639_1];
  cell->priv->language = (lang[0] << 8) | lang[1];
}

static void
ogmdvd_cell_renderer_language_class_init (OGMDvdCellRendererLanguageClass *klass)
{
  GObjectClass *object_class;

  g_type_class_add_private (klass, sizeof (OGMDvdCellRendererLanguagePriv));

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = ogmdvd_cell_renderer_language_finalize;
  object_class->get_property = ogmdvd_cell_renderer_language_get_property;
  object_class->set_property = ogmdvd_cell_renderer_language_set_property;

  g_object_class_install_property (object_class, PROP_LANGUAGE,
      g_param_spec_uint ("language", "Language", "The language code", 0, G_MAXUINT, 0, G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_DEFAULT,
      g_param_spec_string ("default", "Default", "The default text", NULL, G_PARAM_READWRITE));
}

static void
ogmdvd_cell_renderer_language_finalize (GObject *object)
{
  OGMDvdCellRendererLanguage  *cell = OGMDVD_CELL_RENDERER_LANGUAGE (object);

  if (cell->priv->default_text)
  {
    g_free (cell->priv->default_text);
    cell->priv->default_text = NULL;
  }

  (* G_OBJECT_CLASS (ogmdvd_cell_renderer_language_parent_class)->finalize) (object);
}

static void
ogmdvd_cell_renderer_language_get_property (GObject *object, guint param_id, GValue *value, GParamSpec *psec)
{
  OGMDvdCellRendererLanguage  *cell = OGMDVD_CELL_RENDERER_LANGUAGE (object);

  switch (param_id)
  {
    case PROP_LANGUAGE:
      g_value_set_uint (value, cell->priv->language);
      break;
    case PROP_DEFAULT:
      g_value_set_string (value, cell->priv->default_text);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, psec);
      break;
  }
}

static void
ogmdvd_cell_renderer_language_set_property (GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
{
  OGMDvdCellRendererLanguage *cell = OGMDVD_CELL_RENDERER_LANGUAGE (object);
  guint index, code, val;
  const gchar *lang;

  switch (param_id)
  {
    case PROP_LANGUAGE:
      val = g_value_get_uint (value);
      if (!val && cell->priv->default_text)
      {
        cell->priv->language = 0;
        g_object_set (object, "text", cell->priv->default_text, NULL);
      }
      else
      {
        for (index = 2; cell->priv->language != val && index < ogmdvd_nlanguages; index ++)
        {
          lang = ogmdvd_languages[index][OGMDVD_LANGUAGE_ISO639_1];
          code = (lang[0] << 8) | lang[1];

          if (code == val)
          {
            cell->priv->language = val;
            g_object_set (object, "text", ogmdvd_languages[index][OGMDVD_LANGUAGE_NAME], NULL);
          }
        }
      }
      break;
    case PROP_DEFAULT:
      if (cell->priv->default_text)
        g_free (cell->priv->default_text);
      cell->priv->default_text = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
  }
}

/**
 * ogmdvd_cell_renderer_language_new:
 *
 * Creates a new #OGMDvdCellRendererLanguage.
 *
 * Returns: The new #OGMDvdCellRendererLanguage
 */
GtkCellRenderer *
ogmdvd_cell_renderer_language_new (void)
{
  return g_object_new (OGMDVD_TYPE_CELL_RENDERER_LANGUAGE, NULL);
}

