/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-subp-options.h"
#include "ogmrip-subp-codec.h"

G_DEFINE_INTERFACE (OGMRipSubpOptions, ogmrip_subp_options, G_TYPE_OBJECT);

static void
ogmrip_subp_options_default_init (OGMRipSubpOptionsInterface *iface)
{
  g_object_interface_install_property (iface,
      g_param_spec_uint ("charset", "Character set property", "Set character set",
        OGMRIP_CHARSET_UTF8, OGMRIP_CHARSET_ASCII, OGMRIP_CHARSET_UTF8,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
      g_param_spec_gtype ("codec", "Codec property", "Set codec",
        OGMRIP_TYPE_SUBP_CODEC, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
      g_param_spec_uint ("newline", "Newline style property", "Set newline style",
        OGMRIP_NEWLINE_LF, OGMRIP_NEWLINE_CR, OGMRIP_NEWLINE_LF,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
      g_param_spec_boolean ("spell-check", "Spell check property", "Set spell check",
        FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

OGMRipCharset
ogmrip_subp_options_get_charset (OGMRipSubpOptions *options)
{
  OGMRipCharset charset;

  g_return_val_if_fail (OGMRIP_IS_SUBP_OPTIONS (options), 0);

  g_object_get (options, "charset", &charset, NULL);

  return charset;
}

void
ogmrip_subp_options_set_charset (OGMRipSubpOptions *options, OGMRipCharset charset)
{
  g_return_if_fail (OGMRIP_IS_SUBP_OPTIONS (options));
  g_return_if_fail (charset > OGMRIP_CHARSET_UNDEFINED);

  g_object_set (options, "charset", charset, NULL);
}

GType
ogmrip_subp_options_get_codec (OGMRipSubpOptions *options)
{
  GType codec;

  g_return_val_if_fail (OGMRIP_IS_SUBP_OPTIONS (options), FALSE);

  g_object_get (options, "codec", &codec, NULL);

  return codec;
}

void
ogmrip_subp_options_set_codec (OGMRipSubpOptions *options, GType codec)
{
  g_return_if_fail (OGMRIP_IS_SUBP_OPTIONS (options));
  g_return_if_fail (g_type_is_a (codec, OGMRIP_TYPE_SUBP_CODEC));

  g_object_set (options, "codec", codec, NULL);
}

OGMRipNewline
ogmrip_subp_options_get_newline (OGMRipSubpOptions *options)
{
  gboolean newline;

  g_return_val_if_fail (OGMRIP_IS_SUBP_OPTIONS (options), 0);

  g_object_get (options, "newline", &newline, NULL);

  return newline;
}

void
ogmrip_subp_options_set_newline (OGMRipSubpOptions *options, OGMRipNewline newline)
{
  g_return_if_fail (OGMRIP_IS_SUBP_OPTIONS (options));
  g_return_if_fail (newline > OGMRIP_NEWLINE_UNDEFINED);

  g_object_set (options, "newline", newline, NULL);
}

gboolean
ogmrip_subp_options_get_spell_check (OGMRipSubpOptions *options)
{
  gboolean spell_check;

  g_return_val_if_fail (OGMRIP_IS_SUBP_OPTIONS (options), FALSE);

  g_object_get (options, "spell-check", &spell_check, NULL);

  return spell_check;
}

void
ogmrip_subp_options_set_spell_check (OGMRipSubpOptions *options, gboolean spell_check)
{
  g_return_if_fail (OGMRIP_IS_SUBP_OPTIONS (options));

  g_object_set (options, "spell-check", spell_check, NULL);
}

