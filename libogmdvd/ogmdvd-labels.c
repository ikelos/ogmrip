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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/**
 * SECTION:ogmdvd-labels
 * @title: Conversion functions
 * @include: ogmdvd-labels.h
 * @short_description: Converts enumerated types into human readable strings
 */

#include "ogmdvd-labels.h"

/**
 * SECTION:ogmdvd-enums
 * @title: Enumerations
 * @include: ogmdvd-enums.h
 * @short_description: Public enumerated types used throughout OGMDvd
 */

#include "ogmdvd-enums.h"

/**
 * SECTION:ogmdvd-types
 * @title: Types
 * @include: ogmdvd-types.h
 * @short_description: Public data structures used throughout OGMDvd
 */

#include <string.h>

const gchar *ogmdvd_languages[][3] = 
{
  { "  ", "und", "Undetermined" },
  { "??", "und", "Undetermined" },
  { "ab", "abk", "Abkhazian" },
  { "aa", "aar", "Afar" },
  { "af", "afr", "Afrikaans" },
  { "ak", "aka", "Akan" },
  { "sq", "alb", "Albanian" },
  { "am", "amh", "Amharic" },
  { "ar", "ara", "Arabic" },
  { "an", "arg", "Aragonese" },
  { "hy", "arm", "Armenian" },
  { "as", "asm", "Assamese" },
  { "av", "ava", "Avaric" },
  { "ae", "ave", "Avestan" },
  { "ay", "aym", "Aymara" },
  { "az", "aze", "Azerbaijani" },
  { "bm", "bam", "Bambara" },
  { "ba", "bak", "Bashkir" },
  { "eu", "baq", "Basque" },
  { "be", "bel", "Belarusian" },
  { "bn", "ben", "Bengali" },
  { "bh", "bih", "Bihari" },
  { "bi", "bis", "Bislama" },
  { "bs", "bos", "Bosnian" },
  { "br", "bre", "Breton" },
  { "bg", "bul", "Bulgarian" },
  { "my", "bur", "Burmese" },
  { "ca", "cat", "Catalan" },
  { "ch", "cha", "Chamorro" },
  { "ce", "che", "Chechen" },
  { "ny", "nya", "Chewa; Chichewa; Nyanja" },
  { "zh", "chi", "Chinese" },
  { "za", "zha", "Chuang; Zhuang" },
  { "cu", "chu", "Church Slavonic; Old Bulgarian; Church Slavic;" },
  { "cv", "chv", "Chuvash" },
  { "kw", "cor", "Cornish" },
  { "co", "cos", "Corsican" },
  { "cr", "cre", "Cree" },
  { "hr", "scr", "Croatian" },
  { "cs", "cze", "Czech" },
  { "da", "dan", "Danish" },
  { "de", "ger", "Deutsch" },
  { "dv", "div", "Divehi" },
  { "dz", "dzo", "Dzongkha" },
  { "en", "eng", "English" },
  { "es", "spa", "Espanol" },
  { "eo", "epo", "Esperanto" },
  { "et", "est", "Estonian" },
  { "ee", "ewe", "Ewe" },
  { "fo", "fao", "Faroese" },
  { "fj", "fij", "Fijian" },
  { "fi", "fin", "Finnish" },
  { "fr", "fre", "Francais" },
  { "fy", "fry", "Frisian" },
  { "ff", "ful", "Fulah" },
  { "gd", "gla", "Gaelic; Scottish Gaelic" },
  { "gl", "glg", "Gallegan" },
  { "lg", "lug", "Ganda" },
  { "ka", "geo", "Georgian" },
  { "ki", "kik", "Gikuyu; Kikuyu" },
  { "el", "ell", "Greek" },
  { "kl", "kal", "Greenlandic; Kalaallisut" },
  { "gn", "grn", "Guarani" },
  { "gu", "guj", "Gujarati" },
  { "ha", "hau", "Hausa" },
  { "he", "heb", "Hebrew" },
  { "hz", "her", "Herero" },
  { "hi", "hin", "Hindi" },
  { "ho", "hmo", "Hiri Motu" },
  { "hu", "hun", "Hungarian" },
  { "is", "ice", "Icelandic" },
  { "io", "ido", "Ido" },
  { "ig", "ibo", "Igbo" },
  { "id", "ind", "Indonesian" },
  { "ia", "ina", "Interlingua (International)" },
  { "ie", "ile", "Interlingue" },
  { "iu", "iku", "Inuktitut" },
  { "ik", "ipk", "Inupiaq" },
  { "ga", "gle", "Irish" },
  { "it", "ita", "Italiano" },
  { "ja", "jpn", "Japanese" },
  { "jv", "jav", "Javanese" },
  { "kn", "kan", "Kannada" },
  { "kr", "kau", "Kanuri" },
  { "ks", "kas", "Kashmiri" },
  { "kk", "kaz", "Kazakh" },
  { "km", "khm", "Khmer" },
  { "rw", "kin", "Kinyarwanda" },
  { "ky", "kir", "Kirghiz" },
  { "kv", "kom", "Komi" },
  { "kg", "kon", "Kongo" },
  { "ko", "kor", "Korean" },
  { "ku", "kur", "Kurdish" },
  { "kj", "kua", "Kwanyama, Kuanyama" },
  { "lo", "lao", "Lao" },
  { "la", "lat", "Latin" },
  { "lv", "lav", "Latvian" },
  { "li", "lim", "Limburgish; Limburger; Limburgan" },
  { "ln", "lin", "Lingala" },
  { "lt", "lit", "Lithuanian" },
  { "lu", "lub", "Luba-Katanga" },
  { "lb", "ltz", "Luxembourgish; Letzeburgesch" },
  { "mk", "mac", "Macedonian" },
  { "mg", "mlg", "Malagasy" },
  { "ml", "mal", "Malayalam" },
  { "ms", "may", "Malay" },
  { "mt", "mlt", "Maltese" },
  { "gv", "glv", "Manx" },
  { "mi", "mao", "Maori" },
  { "mr", "mar", "Marathi" },
  { "mh", "mah", "Marshallese" },
  { "mo", "mol", "Moldavian" },
  { "mn", "mon", "Mongolian" },
  { "na", "nau", "Nauru" },
  { "nv", "nav", "Navajo; Navaho" },
  { "ng", "ndo", "Ndonga" },
  { "nl", "nld", "Nederlands" },
  { "ne", "nep", "Nepali" },
  { "se", "sme", "Northern Sami" },
  { "nd", "nde", "North Ndebele" },
  { "nb", "nob", "Norwegian Bokmal; Bokmal, Norwegian" },
  { "no", "nor", "Norwegian" },
  { "nn", "nno", "Norwegian Nynorsk; Nynorsk, Norwegian" },
  { "oj", "oji", "Ojibwa" },
  { "or", "ori", "Oriya" },
  { "om", "orm", "Oromo" },
  { "os", "oss", "Ossetian; Ossetic" },
  { "pi", "pli", "Pali" },
  { "pa", "pan", "Panjabi" },
  { "fa", "per", "Persian" },
  { "pl", "pol", "Polish" },
  { "pt", "por", "Portuguese" },
  { "oc", "oci", "Provencal; Occitan (post 1500)" },
  { "ps", "pus", "Pushto" },
  { "qu", "que", "Quechua" },
  { "rm", "roh", "Raeto-Romance" },
  { "ro", "ron", "Romanian" },
  { "rn", "run", "Rundi" },
  { "ru", "rus", "Russian" },
  { "sm", "smo", "Samoan" },
  { "sg", "sag", "Sango" },
  { "sa", "san", "Sanskrit" },
  { "sc", "srd", "Sardinian" },
  { "sr", "srp", "Serbian" },
  { "sn", "sna", "Shona" },
  { "ii", "iii", "Sichuan Yi" },
  { "sd", "snd", "Sindhi" },
  { "si", "sin", "Sinhalese" },
  { "sk", "slo", "Slovak" },
  { "sl", "slv", "Slovenian" },
  { "so", "som", "Somali" },
  { "st", "sot", "Sotho, Southern" },
  { "nr", "nbl", "South Ndebele" },
  { "su", "sun", "Sundanese" },
  { "sw", "swa", "Swahili" },
  { "ss", "ssw", "Swati" },
  { "sv", "swe", "Swedish" },
  { "tl", "tgl", "Tagalog" },
  { "ty", "tah", "Tahitian" },
  { "tg", "tgk", "Tajik" },
  { "ta", "tam", "Tamil" },
  { "tt", "tat", "Tatar" },
  { "te", "tel", "Telugu" },
  { "th", "tha", "Thai" },
  { "bo", "tib", "Tibetan" },
  { "ti", "tir", "Tigrinya" },
  { "to", "ton", "Tonga (Tonga Islands)" },
  { "ts", "tso", "Tsonga" },
  { "tn", "tsn", "Tswana" },
  { "tr", "tur", "Turkish" },
  { "tk", "tuk", "Turkmen" },
  { "tw", "twi", "Twi" },
  { "ug", "uig", "Uighur" },
  { "uk", "ukr", "Ukrainian" },
  { "ur", "urd", "Urdu" },
  { "uz", "uzb", "Uzbek" },
  { "ve", "ven", "Venda" },
  { "vi", "vie", "Vietnamese" },
  { "vo", "vol", "Volapük" },
  { "wa", "wln", "Walloon" },
  { "cy", "wel", "Welsh" },
  { "wo", "wol", "Wolof" },
  { "xh", "xho", "Xhosa" },
  { "yi", "yid", "Yiddish" },
  { "yo", "yor", "Yoruba" },
  { "zu", "zul", "Zulu" },
  { NULL, NULL,  NULL },
};

const guint ogmdvd_nlanguages = G_N_ELEMENTS (ogmdvd_languages) - 1;

/**
 * ogmdvd_get_video_format_label:
 * @format: The video format
 *
 * Returns a human readable video format.
 *
 * Returns: A constant string, or NULL
 */
G_CONST_RETURN gchar *
ogmdvd_get_video_format_label (gint format)
{
  static const gchar *video_format[] = 
  {
    "NTSC",
    "PAL",
    "Error",
    "Error"
  };

  g_return_val_if_fail (format >= 0 && format <= 3, NULL);

  return video_format[format];
}

/**
 * ogmdvd_get_display_aspect_label:
 * @aspect: The display aspect
 *
 * Returns a human readable display aspect.
 *
 * Returns: A constant string, or NULL
 */
G_CONST_RETURN gchar *
ogmdvd_get_display_aspect_label (gint aspect)
{
  static gchar *display_aspect[] =
  {
    "4/3", 
    "16/9", 
    "?:?", 
    "16/9"
  };

  g_return_val_if_fail (aspect >= 0 && aspect <= 3, NULL);

  return display_aspect[aspect];
}

/**
 * ogmdvd_get_audio_format_label:
 * @format: The audio format
 *
 * Returns a human readable audio format.
 *
 * Returns: A constant string, or NULL
 */
G_CONST_RETURN gchar *
ogmdvd_get_audio_format_label (gint format)
{
  static gchar *audio_format[] =
  {
    "AC3",
    "Unknown",
    "MPEG1",
    "MPEG2EXT",
    "LPCM",
    "SDDS",
    "DTS",
    "Error"
  };

  g_return_val_if_fail (format >= 0 && format <= 7, NULL);

  return audio_format[format];
}

/**
 * ogmdvd_get_audio_channels_label:
 * @channels: The number of channels
 *
 * Returns a human readable number of channels.
 *
 * Returns: A constant string, or NULL
 */
G_CONST_RETURN gchar *
ogmdvd_get_audio_channels_label (gint channels)
{
  static gchar *audio_channels[] =
  {
    "Mono",
    "Stereo",
    "Unknown",
    "Surround",
    "Unknown",
    "5.1",
    "6.1",
    "7.1"
  };

  g_return_val_if_fail (channels >= 0 && channels <= 7, NULL);

  return audio_channels[channels];
}

/**
 * ogmdvd_get_audio_quantization_label:
 * @quantization: The quantization
 *
 * Returns a human readable quantization.
 *
 * Returns: A constant string, or NULL
 */
G_CONST_RETURN gchar *
ogmdvd_get_audio_quantization_label (gint quantization)
{
  static gchar *audio_quantization[] = 
  {
    "16bit", 
    "20bit", 
    "24bit", 
    "drc"
  };

  g_return_val_if_fail (quantization >= 0 && quantization <= 3, NULL);

  return audio_quantization[quantization];
}

/**
 * ogmdvd_get_audio_content_label:
 * @content: The audio content
 *
 * Returns a human readable audio content.
 *
 * Returns: A constant string, or NULL
 */
G_CONST_RETURN gchar *
ogmdvd_get_audio_content_label (gint content)
{
  static gchar *audio_content[] = 
  {
    "Undefined", 
    "Normal", 
    "Impaired", 
    "Comments 1", 
    "Comments 2"
  };

  g_return_val_if_fail (content >= 0 && content <= 4, NULL);

  return audio_content[content];
}

/**
 * ogmdvd_get_subp_content_label:
 * @content: The subtitles content
 *
 * Returns a human readable subtitles content.
 *
 * Returns: A constant string, or NULL
 */
G_CONST_RETURN gchar *
ogmdvd_get_subp_content_label (gint content)
{
  static gchar *subp_content[] = 
  {
    "Undefined", 
    "Normal", 
    "Large", 
    "Children", 
    "Reserved", 
    "Normal_CC", 
    "Large_CC", 
    "Children_CC",
    "Reserved", 
    "Forced", 
    "Reserved", 
    "Reserved", 
    "Reserved", 
    "Director", 
    "Large_Director", 
    "Children_Director"
  };

  g_return_val_if_fail (content >= 0 && content <= 15, NULL);

  return subp_content[content];
}

/**
 * ogmdvd_get_language_label:
 * @code: The language code
 *
 * Returns a human readable language.
 *
 * Returns: A constant string, or NULL
 */
G_CONST_RETURN gchar *
ogmdvd_get_language_label (gint code)
{
  const gchar *lang;
  guint i;

  lang = ogmdvd_get_language_iso639_1 (code);

  for (i = 0; ogmdvd_languages[i][OGMDVD_LANGUAGE_ISO639_1]; i++)
    if (strcmp (ogmdvd_languages[i][OGMDVD_LANGUAGE_ISO639_1], lang) == 0)
      return ogmdvd_languages[i][OGMDVD_LANGUAGE_NAME];

  return NULL;
}

/**
 * ogmdvd_get_language_iso639_1:
 * @code: The language code
 *
 * Returns an ISO 639-1 language.
 *
 * Returns: A constant string, or NULL
 */
G_CONST_RETURN gchar *
ogmdvd_get_language_iso639_1 (gint code)
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

/**
 * ogmdvd_get_language_iso639_2:
 * @code: The language code
 *
 * Returns an ISO 639-2 language.
 *
 * Returns: A constant string, or NULL
 */
G_CONST_RETURN gchar *
ogmdvd_get_language_iso639_2 (gint code)
{
  const gchar *lang;
  guint i;

  lang = ogmdvd_get_language_iso639_1 (code);

  for (i = 0; ogmdvd_languages[i][OGMDVD_LANGUAGE_ISO639_1]; i++)
    if (strcmp (ogmdvd_languages[i][OGMDVD_LANGUAGE_ISO639_1], lang) == 0)
      return ogmdvd_languages[i][OGMDVD_LANGUAGE_ISO639_2];

  return NULL;
}

