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

#ifndef __OGMRIP_OPTIONS_H__
#define __OGMRIP_OPTIONS_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _OGMRipAudioOptions OGMRipAudioOptions;
typedef struct _OGMRipSubpOptions  OGMRipSubpOptions;

/**
 * OGMRipAudioOptions:
 * @codec: The type of the audio codec
 * @label: The label of the track
 * @quality: The quality of the track
 * @srate: The sample rate of the track
 * @channels: The number of channels of the track
 * @language: The language of the track
 * @normalize: Whether to normalize the sound
 * @defaults: Whether these are the default options
 *
 * This structure describes the options of an audio track.
 */
struct _OGMRipAudioOptions
{
  GType codec;
  gchar *label;
  gint quality;
  gint srate;
  gint channels;
  gint language;
  gboolean normalize;
  gboolean defaults;
};

/**
 * OGMRipSubpOptions:
 * @codec: The type of the subtitles codec
 * @label: The label of the stream
 * @charset: The character set of the stream
 * @newline: The newline style
 * @language: The language of the stream
 * @spell: Whether to spell check the subtitles
 * @forced_subs: Whether to encode forced subs obly
 * @defaults: Whether these are the default options
 *
 * This structure describes the options of a subtitles stream.
 */
struct _OGMRipSubpOptions
{
  GType codec;
  gchar *label;
  gint charset;
  gint newline;
  gint language;
  gboolean spell;
  gboolean forced_subs;
  gboolean defaults;
};

/**
 * OGMRipOptions:
 * @codec: The type of the codec
 * @audio_options: The audio options
 * @subp_options: The subtitles options
 *
 * This structure contains a union of the audio and subtitles options.
 */
typedef union
{
  GType codec;
  OGMRipAudioOptions audio_options;
  OGMRipSubpOptions  subp_options;
} OGMRipOptions;

void ogmrip_audio_options_init  (OGMRipAudioOptions *options);
void ogmrip_audio_options_reset (OGMRipAudioOptions *options);

void ogmrip_subp_options_init   (OGMRipSubpOptions  *options);
void ogmrip_subp_options_reset  (OGMRipSubpOptions  *options);

G_END_DECLS

#endif /* __OGMRIP_OPTIONS_H__ */
