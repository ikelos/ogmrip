/* OGMRipMplayer - A library around mplayer/mencoder for OGMRip
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-mplayer-version.h"

#include <stdlib.h>

static gboolean have_mplayer  = FALSE;
static gboolean have_mencoder = FALSE;
static gboolean have_dts      = FALSE;

static gint mplayer_major = 0;
static gint mplayer_minor = 0;
static gint mplayer_pre   = 0;
static gint mplayer_rc    = 0;

/**
 * ogmrip_check_mplayer:
 *
 * Checks if mplayer is installed.
 *
 * Returns: TRUE if mplayer is installed
 */
gboolean
ogmrip_check_mplayer (void)
{
  static gboolean mplayer_checked  = FALSE;

  if (!mplayer_checked)
  {
    GRegex *regex;
    GMatchInfo *match_info;
    gboolean match;
    const gchar *version;
    gchar *output;

    mplayer_checked = TRUE;

    version = g_getenv ("MPLAYER_VERSION");
    if (version)
      output = g_strdup_printf ("MPlayer %s", version);
    else
    {
      if (!g_spawn_command_line_sync ("mplayer", &output, NULL, NULL, NULL))
        return FALSE;
    }

    have_mplayer = TRUE;

    regex = g_regex_new ("MPlayer (\\d+)\\.(\\d+)((rc|pre)(\\d+))?", 0, 0, NULL);
    if (!regex)
    {
      g_free (output);
      return FALSE;
    }

    match = g_regex_match_full (regex, output, -1, 0, 0, &match_info, NULL);
    if (match)
    {
      gchar *str, **vstr;

      while (g_match_info_matches (match_info))
      {
        str = g_match_info_fetch (match_info, 0);
        vstr = g_regex_split_full (regex, str, -1, 0, 0, -1, NULL);
        g_free (str);

        if (vstr)
        {
          if (vstr[0] && vstr[1])
          {
            mplayer_major = atoi (vstr[1]);
            mplayer_minor = atoi (vstr[2]);
            if (vstr[3] && vstr[4] && vstr[5])
            {
              if (g_str_equal (vstr[4], "rc"))
                mplayer_rc = atoi (vstr[5]);
              else
                mplayer_pre = atoi (vstr[5]);
            }
          }
          g_strfreev (vstr);
        }

        g_match_info_next (match_info, NULL);
      }

      g_match_info_free (match_info);
    }

    g_regex_unref (regex);
    g_free (output);
  }

  return have_mplayer;
}

/**
 * ogmrip_check_mencoder:
 *
 * Checks if mencoder is installed.
 *
 * Returns: TRUE if mencoder is installed
 */
gboolean
ogmrip_check_mencoder (void)
{
  static gboolean mencoder_checked = FALSE;

  if (!mencoder_checked)
  {
    gchar *fullname;

    fullname = g_find_program_in_path ("mencoder");
    have_mencoder = fullname != NULL;
    g_free (fullname);

    mencoder_checked = TRUE;
  }

  return have_mencoder;
}

/**
 * ogmrip_check_mplayer_version:
 * @major: The major version number
 * @minor: The minor version number
 * @rc: The release candidate version number, or 0 if none
 * @pre: The pre-release version number, or 0 if none
 *
 * Checks if the version of mplayer is older than a given version.
 *
 * Returns: TRUE if the version is older
 */
gboolean
ogmrip_check_mplayer_version (gint major, gint minor, gint rc, gint pre)
{
  if (!ogmrip_check_mplayer ())
    return FALSE;

  if (!mplayer_major && !mplayer_minor && !mplayer_rc && !mplayer_pre)
    return TRUE;
  else
  {
    if (mplayer_major > major)
      return TRUE;
    else if (mplayer_major == major)
    {
      if (mplayer_minor > minor)
        return TRUE;
      else if (mplayer_minor == minor)
      {
        if (mplayer_rc == 0 && mplayer_pre == 0)
          return TRUE;

        if ((mplayer_rc != 0 || mplayer_pre != 0) && (rc != 0 || pre != 0))
        {
          if (mplayer_rc > rc)
            return TRUE;
          else if (mplayer_rc == rc && mplayer_pre >= pre)
            return TRUE;
        }
      }
    }
  }

  return FALSE;
}

/**
 * ogmrip_check_mplayer_dts:
 *
 * Checks if mplayer has DTS support.
 *
 * Returns: TRUE if DTS is supported
 */
gboolean
ogmrip_check_mplayer_dts (void)
{
  static gboolean dts_checked = FALSE;

  if (!dts_checked)
  {
    gchar *output, *error;
    gint retval;

    dts_checked = TRUE;

    if (!ogmrip_check_mplayer ())
      return FALSE;

    if (!g_spawn_command_line_sync ("mplayer -ac help -noconfig all", &output, &error, &retval, NULL))
      return FALSE;

    if (retval != 0)
    {
      g_free (output);
      g_free (error);

      if (!g_spawn_command_line_sync ("mplayer -ac help", &output, &error, NULL, NULL))
        return FALSE;
    }

    g_free (error);

    have_dts = g_regex_match_simple ("^(ffdts|ffdca|dts).*working.*$", output, G_REGEX_MULTILINE, 0);
    g_free (output);
  }

  return have_dts;
}

