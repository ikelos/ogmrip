/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * SECTION:ogmrip-options
 * @title: OGMRipOptions
 * @short_description: Structures to manipulate audio and subtitles options
 * @include: ogmrip-options.h
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-options.h"

#include <string.h>

/**
 * ogmrip_audio_options_init:
 * @options: An #OGMRipAudioOptions
 *
 * Initializes @options with the default audio options.
 */
void
ogmrip_audio_options_init (OGMRipAudioOptions *options)
{
  g_return_if_fail (options != NULL);

  memset (options, 0, sizeof (OGMRipAudioOptions));

  options->codec = G_TYPE_NONE;
  options->quality = 3;
  options->channels = 1;
  options->normalize = TRUE;
  options->defaults = TRUE;
}

/**
 * ogmrip_audio_options_reset:
 * @options: An #OGMRipAudioOptions
 *
 * Clears the current options in @options and resets them to the default audio options.
 */
void
ogmrip_audio_options_reset (OGMRipAudioOptions *options)
{
  g_return_if_fail (options != NULL);

  if (options->label)
    g_free (options->label);

  ogmrip_audio_options_init (options);
}

/**
 * ogmrip_subp_options_init:
 * @options: An #OGMRipSubpOptions
 *
 * Initializes @options with the default subp options.
 */
void
ogmrip_subp_options_init (OGMRipSubpOptions *options)
{
  g_return_if_fail (options != NULL);

  memset (options, 0, sizeof (OGMRipSubpOptions));

  options->codec = G_TYPE_NONE;
  options->defaults = TRUE;
}

/**
 * ogmrip_subp_options_reset:
 * @options: An #OGMRipSubpOptions
 *
 * Clears the current options in @options and resets them to the default subp options.
 */
void
ogmrip_subp_options_reset (OGMRipSubpOptions *options)
{
  g_return_if_fail (options != NULL);

  if (options->label)
    g_free (options->label);

  ogmrip_subp_options_init (options);
}

