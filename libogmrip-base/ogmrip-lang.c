/* OGMRipBase - A foundation library for OGMRip
 * Copyright (C) 2010-2012 Olivier Rolland <billl@users.sourceforge.net>
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

/**
 * SECTION:ogmrip-media-labels
 * @title: Conversion functions
 * @include: ogmrip-media.h
 * @short_description: Converts enumerated types into human readable strings
 */

#include "ogmrip-lang.h"

#include <string.h>

const gchar *ogmdvd_languages[][3] = 
{
  { "  ", "und", "Undetermined" },
  { "??", "und", "Undetermined" },
  { "aa", "aar", "Afar" },
  { "ab", "abk", "Abkhazian" },
  { "ae", "ave", "Avestan" },
  { "af", "afr", "Afrikaans" },
  { "ak", "aka", "Akan" },
  { "am", "amh", "Amharic" },
  { "an", "arg", "Aragonese" },
  { "ar", "ara", "Arabic" },
  { "as", "asm", "Assamese" },
  { "av", "ava", "Avaric" },
  { "ay", "aym", "Aymara" },
  { "az", "aze", "Azerbaijani" },
  { "ba", "bak", "Bashkir" },
  { "be", "bel", "Belarusian" },
  { "bg", "bul", "Bulgarian" },
  { "bh", "bih", "Bihari" },
  { "bi", "bis", "Bislama" },
  { "bm", "bam", "Bambara" },
  { "bn", "ben", "Bengali" },
  { "bo", "bob", "Tibetan" },
  { "bo", "tib", "Tibetan" },
  { "br", "bre", "Breton" },
  { "bs", "bos", "Bosnian" },
  { "ca", "cat", "Catalan" },
  { "ce", "che", "Chechen" },
  { "ch", "cha", "Chamorro" },
  { "co", "cos", "Corsican" },
  { "cr", "cre", "Cree" },
  { "cs", "ces", "Czech" },
  { "cs", "cze", "Czech" },
  { "cu", "chu", "Church Slavonic; Old Bulgarian; Church Slavic;" },
  { "cv", "chv", "Chuvash" },
  { "cy", "cym", "Welsh" },
  { "cy", "wel", "Welsh" },
  { "da", "dan", "Danish" },
  { "de", "deu", "Deutsch" },
  { "de", "ger", "Deutsch" },
  { "dv", "div", "Divehi" },
  { "dz", "dzo", "Dzongkha" },
  { "ee", "ewe", "Ewe" },
  { "el", "ell", "Greek" },
  { "el", "gre", "Greek" },
  { "en", "eng", "English" },
  { "eo", "epo", "Esperanto" },
  { "es", "spa", "Espanol" },
  { "et", "est", "Estonian" },
  { "eu", "baq", "Basque" },
  { "eu", "eus", "Basque" },
  { "fa", "fas", "Persian" },
  { "fa", "per", "Persian" },
  { "ff", "ful", "Fulah" },
  { "fi", "fin", "Finnish" },
  { "fj", "fij", "Fijian" },
  { "fo", "fao", "Faroese" },
  { "fr", "fra", "Francais" },
  { "fr", "fre", "Francais" },
  { "fy", "fry", "Frisian" },
  { "ga", "gle", "Irish" },
  { "gd", "gla", "Gaelic; Scottish Gaelic" },
  { "gl", "glg", "Gallegan" },
  { "gn", "grn", "Guarani" },
  { "gu", "guj", "Gujarati" },
  { "gv", "glv", "Manx" },
  { "ha", "hau", "Hausa" },
  { "he", "heb", "Hebrew" },
  { "hi", "hin", "Hindi" },
  { "ho", "hmo", "Hiri Motu" },
  { "hr", "hrv", "Croatian" },
  { "hr", "scr", "Croatian" },
  { "ht", "hat", "Haitian" },
  { "hu", "hun", "Hungarian" },
  { "hy", "arm", "Armenian" },
  { "hy", "hye", "Armenian" },
  { "hz", "her", "Herero" },
  { "ia", "ina", "Interlingua (International)" },
  { "id", "ind", "Indonesian" },
  { "ie", "ile", "Interlingue" },
  { "ig", "ibo", "Igbo" },
  { "ii", "iii", "Sichuan Yi" },
  { "ik", "ipk", "Inupiaq" },
  { "io", "ido", "Ido" },
  { "is", "ice", "Icelandic" },
  { "is", "isl", "Icelandic" },
  { "it", "ita", "Italiano" },
  { "iu", "iku", "Inuktitut" },
  { "ja", "jpn", "Japanese" },
  { "jv", "jav", "Javanese" },
  { "ka", "geo", "Georgian" },
  { "ka", "kat", "Georgian" },
  { "kg", "kon", "Kongo" },
  { "ki", "kik", "Gikuyu; Kikuyu" },
  { "kj", "kua", "Kwanyama, Kuanyama" },
  { "kk", "kaz", "Kazakh" },
  { "kl", "kal", "Greenlandic; Kalaallisut" },
  { "km", "khm", "Khmer" },
  { "kn", "kan", "Kannada" },
  { "ko", "kor", "Korean" },
  { "kr", "kau", "Kanuri" },
  { "ks", "kas", "Kashmiri" },
  { "ku", "kur", "Kurdish" },
  { "kv", "kom", "Komi" },
  { "kw", "cor", "Cornish" },
  { "ky", "kir", "Kirghiz" },
  { "la", "lat", "Latin" },
  { "lb", "ltz", "Luxembourgish; Letzeburgesch" },
  { "lg", "lug", "Ganda" },
  { "li", "lim", "Limburgish; Limburger; Limburgan" },
  { "ln", "lin", "Lingala" },
  { "lo", "lao", "Lao" },
  { "lt", "lit", "Lithuanian" },
  { "lu", "lub", "Luba-Katanga" },
  { "lv", "lav", "Latvian" },
  { "mg", "mlg", "Malagasy" },
  { "mh", "mah", "Marshallese" },
  { "mi", "mao", "Maori" },
  { "mi", "mri", "Maori" },
  { "mk", "mac", "Macedonian" },
  { "mk", "mkd", "Macedonian" },
  { "ml", "mal", "Malayalam" },
  { "mn", "mon", "Mongolian" },
  { "mo", "mol", "Moldavian" },
  { "mr", "mar", "Marathi" },
  { "ms", "may", "Malay" },
  { "ms", "msa", "Malay" },
  { "mt", "mlt", "Maltese" },
  { "my", "bur", "Burmese" },
  { "my", "mya", "Burmese" },
  { "na", "nau", "Nauru" },
  { "nb", "nob", "Norwegian Bokmal; Bokmal, Norwegian" },
  { "nd", "nde", "North Ndebele" },
  { "ne", "nep", "Nepali" },
  { "ng", "ndo", "Ndonga" },
  { "nl", "dut", "Nederlands" },
  { "nl", "nld", "Nederlands" },
  { "nn", "nno", "Norwegian Nynorsk; Nynorsk, Norwegian" },
  { "no", "nor", "Norwegian" },
  { "nr", "nbl", "South Ndebele" },
  { "nv", "nav", "Navajo; Navaho" },
  { "ny", "nya", "Chewa; Chichewa; Nyanja" },
  { "oc", "oci", "Provencal; Occitan (post 1500)" },
  { "oj", "oji", "Ojibwa" },
  { "om", "orm", "Oromo" },
  { "or", "ori", "Oriya" },
  { "os", "oss", "Ossetian; Ossetic" },
  { "pa", "pan", "Panjabi" },
  { "pi", "pli", "Pali" },
  { "pl", "pol", "Polish" },
  { "ps", "pus", "Pushto" },
  { "pt", "por", "Portuguese" },
  { "qu", "que", "Quechua" },
  { "rm", "roh", "Raeto-Romance" },
  { "rn", "run", "Rundi" },
  { "ro", "ron", "Romanian" },
  { "ro", "rum", "Romanian" },
  { "ru", "rus", "Russian" },
  { "rw", "kin", "Kinyarwanda" },
  { "sa", "san", "Sanskrit" },
  { "sc", "srd", "Sardinian" },
  { "sd", "snd", "Sindhi" },
  { "se", "sme", "Northern Sami" },
  { "sg", "sag", "Sango" },
  { "si", "sin", "Sinhalese" },
  { "sk", "slk", "Slovak" },
  { "sk", "slo", "Slovak" },
  { "sl", "slv", "Slovenian" },
  { "sm", "smo", "Samoan" },
  { "sn", "sna", "Shona" },
  { "so", "som", "Somali" },
  { "sq", "alb", "Albanian" },
  { "sq", "sqi", "Albanian" },
  { "sr", "scc", "Serbian" },
  { "sr", "srp", "Serbian" },
  { "ss", "ssw", "Swati" },
  { "st", "sot", "Sotho, Southern" },
  { "su", "sun", "Sundanese" },
  { "sv", "swe", "Swedish" },
  { "sw", "swa", "Swahili" },
  { "ta", "tam", "Tamil" },
  { "te", "tel", "Telugu" },
  { "tg", "tgk", "Tajik" },
  { "th", "tha", "Thai" },
  { "ti", "tir", "Tigrinya" },
  { "tk", "tuk", "Turkmen" },
  { "tl", "tgl", "Tagalog" },
  { "tn", "tsn", "Tswana" },
  { "to", "ton", "Tonga (Tonga Islands)" },
  { "tr", "tur", "Turkish" },
  { "ts", "tso", "Tsonga" },
  { "tt", "tat", "Tatar" },
  { "tw", "twi", "Twi" },
  { "ty", "tah", "Tahitian" },
  { "ug", "uig", "Uighur" },
  { "uk", "ukr", "Ukrainian" },
  { "ur", "urd", "Urdu" },
  { "uz", "uzb", "Uzbek" },
  { "ve", "ven", "Venda" },
  { "vi", "vie", "Vietnamese" },
  { "vo", "vol", "Volapük" },
  { "wa", "wln", "Walloon" },
  { "wo", "wol", "Wolof" },
  { "xh", "xho", "Xhosa" },
  { "yi", "yid", "Yiddish" },
  { "yo", "yor", "Yoruba" },
  { "za", "zha", "Chuang; Zhuang" },
  { "zh", "chi", "Chinese" },
  { "zh", "zho", "Chinese" },
  { "zu", "zul", "Zulu" },
  { NULL, NULL,  NULL },
};

const guint ogmdvd_nlanguages = G_N_ELEMENTS (ogmdvd_languages) - 1;

/**
 * ogmrip_language_to_name:
 * @code: The language code
 *
 * Returns a human readable language.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_language_to_name (gint code)
{
  const gchar *lang;
  guint i;

  lang = ogmrip_language_to_iso639_1 (code);

  for (i = 0; ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_1]; i++)
    if (g_str_equal (ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_1], lang))
      return ogmdvd_languages[i][OGMRIP_LANGUAGE_NAME];

  return NULL;
}

gint
ogmrip_language_from_name (const gchar *name)
{
  guint i;

  for (i = 0; ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_1]; i++)
    if (g_str_equal (ogmdvd_languages[i][OGMRIP_LANGUAGE_NAME], name))
      return ogmrip_language_from_iso639_1 (ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_1]);

  return ogmrip_language_from_iso639_1 (ogmdvd_languages[1][OGMRIP_LANGUAGE_ISO639_1]);
}

/**
 * ogmrip_language_to_iso639_1:
 * @code: The language code
 *
 * Returns an ISO 639-1 language.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_language_to_iso639_1 (gint code)
{
  static gchar lang[3];

  if (code < 1)
  {
    lang[0] = '?';
    lang[1] = '?';
    lang[2] = 0;
  }
  else
  {
    lang[0] = code >> 8;
    lang[1] = code & 0xff;
    lang[2] = 0;
  }

  return lang;
}

gint
ogmrip_language_from_iso639_1 (const gchar *iso639_1)
{
  gint code = iso639_1[0];

  code = (code << 8) | iso639_1[1];

  return code;
}

/**
 * ogmrip_language_get_iso639_2:
 * @code: The language code
 *
 * Returns an ISO 639-2 language.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_language_to_iso639_2 (gint code)
{
  const gchar *lang;
  guint i;

  lang = ogmrip_language_to_iso639_1 (code);

  for (i = 0; ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_1]; i++)
    if (g_str_equal (ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_1], lang))
      return ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_2];

  return NULL;
}

gint
ogmrip_language_from_iso639_2 (const gchar *iso639_2)
{
  guint i;

  for (i = 0; ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_1]; i++)
    if (g_str_equal (ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_2], iso639_2))
      return ogmrip_language_from_iso639_1 (ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_1]);

  return ogmrip_language_from_iso639_1 (ogmdvd_languages[1][OGMRIP_LANGUAGE_ISO639_1]);
}

