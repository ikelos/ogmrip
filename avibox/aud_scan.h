/*
 *  aud_scan.h
 *
 *  Scans the audio track
 *
 *  Copyright (C) Tilmann Bitterberg - June 2003
 *
 *  This file is part of transcode, a video stream processing tool
 *
 *  transcode is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  transcode is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

int tc_get_mp3_header(unsigned char* hbuf, int* chans, int* srate, int *bitrate);
#define tc_decode_mp3_header(hbuf)  tc_get_mp3_header(hbuf, NULL, NULL, NULL)
int tc_get_ac3_header(unsigned char* _buf, int len, int* chans, int* srate, int *bitrate);

// main entrance
int tc_get_audio_header(unsigned char* buf, int buflen, int format, int* chans, int* srate, int *bitrate);
int tc_probe_audio_header(unsigned char* buf, int buflen);

int tc_format_ms_supported(int format);
void tc_format_mute(unsigned char *buf, int buflen, int format);

// ------------------------
// You must set the requested audio before entering this function
// the AVI file out must be filled with correct values.
// ------------------------

int sync_audio_video_avi2avi (double vid_ms, double *aud_ms, avi_t *in, avi_t *out);
int sync_audio_video_avi2avi_ro (double vid_ms, double *aud_ms, avi_t *in);
