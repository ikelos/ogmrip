/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-media-labels.h"

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

const gchar *
ogmrip_format_get_label (OGMRipFormat format)
{
  static const gchar *format_type[] = 
  {
    "Undefined",
    "MPEG1",
    "MPEG2",
    "MPEG4",
    "h264",
    "Theora",
    "Dirac",
    "PCM",
    "MP3",
    "AC3",
    "DTS",
    "AAC",
    "Vorbis",
    "MicroDVD",
    "SubRip",
    "SRT",
    "SAMI",
    "VPLAYER",
    "RT",
    "SSA",
    "PJS",
    "MPSub",
    "AQT",
    "SRT_2_0",
    "SubRip_0_9",
    "JacoSub",
    "MPL_2",
    "VOBSUB",
    "COPY",
    "LPCM",
    "BPCM",
    "MP12",
    "MJPEG",
    "Flac",
    "VP8",
    "Chapters"
  };

  return format_type[format + 1];
}

/**
 * ogmrip_standard_get_label:
 * @format: The standard
 *
 * Returns a human readable standard.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_standard_get_label (OGMRipStandard standard)
{
  static const gchar *video_format[] = 
  {
    "Undefined",
    "NTSC",
    "PAL"
  };

  return video_format[standard + 1];
}

/**
 * ogmrip_aspect_get_label:
 * @aspect: The aspect
 *
 * Returns a human readable aspect.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_aspect_get_label (OGMRipAspect aspect)
{
  static const gchar *display_aspect[] =
  {
    "Undefined",
    "4/3", 
    "16/9",
  };

  return display_aspect[aspect + 1];
}

/**
 * ogmrip_channels_get_label:
 * @channels: The number of channels
 *
 * Returns a human readable number of channels.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_channels_get_label (OGMRipChannels channels)
{
  static const gchar *audio_channels[] =
  {
    "Undefined",
    "Mono",
    "Stereo",
    "Surround",
    "5.1",
    "6.1",
    "7.1"
  };

  return audio_channels[channels + 1];
}

/**
 * ogmrip_quantization_get_label:
 * @quantization: The quantization
 *
 * Returns a human readable quantization.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_quantization_get_label (OGMRipQuantization quantization)
{
  static const gchar *audio_quantization[] = 
  {
    "Undefined",
    "16 bits", 
    "20 bits", 
    "24 bits", 
    "DRC"
  };

  return audio_quantization[quantization + 1];
}

/**
 * ogmrip_audio_content_get_label:
 * @content: The audio content
 *
 * Returns a human readable audio content.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_audio_content_get_label (OGMRipAudioContent content)
{
  static const gchar *audio_content[] = 
  {
    "Undefined", 
    "Normal", 
    "Impaired", 
    "Comments 1", 
    "Comments 2"
  };

  return audio_content[content + 1];
}

/**
 * ogmrip_subp_content_get_label:
 * @content: The subtitles content
 *
 * Returns a human readable subtitles content.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_subp_content_get_label (OGMRipSubpContent content)
{
  static const gchar *subp_content[] = 
  {
    "Undefined", 
    "Normal", 
    "Large", 
    "Children", 
    "Reserved", 
    "Normal CC", 
    "Large CC", 
    "Children CC",
    "Reserved", 
    "Forced", 
    "Reserved", 
    "Reserved", 
    "Reserved", 
    "Director", 
    "Large Director", 
    "Children Director"
  };

  return subp_content[content + 1];
}

/**
 * ogmrip_language_get_label:
 * @code: The language code
 *
 * Returns a human readable language.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_language_get_label (gint code)
{
  const gchar *lang;
  guint i;

  lang = ogmrip_language_get_iso639_1 (code);

  for (i = 0; ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_1]; i++)
    if (strcmp (ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_1], lang) == 0)
      return ogmdvd_languages[i][OGMRIP_LANGUAGE_NAME];

  return NULL;
}

/**
 * ogmrip_language_get_iso639_1:
 * @code: The language code
 *
 * Returns an ISO 639-1 language.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_language_get_iso639_1 (gint code)
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
 * ogmrip_language_get_iso639_2:
 * @code: The language code
 *
 * Returns an ISO 639-2 language.
 *
 * Returns: A constant string, or NULL
 */
const gchar *
ogmrip_language_get_iso639_2 (gint code)
{
  const gchar *lang;
  guint i;

  lang = ogmrip_language_get_iso639_1 (code);

  for (i = 0; ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_1]; i++)
    if (strcmp (ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_1], lang) == 0)
      return ogmdvd_languages[i][OGMRIP_LANGUAGE_ISO639_2];

  return NULL;
}

