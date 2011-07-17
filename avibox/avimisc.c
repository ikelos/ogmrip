/*
 *  avimisc.c
 *
 *  Copyright (C) Thomas Oestreich - June 2001
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

#include "avilib.h"

#include <stdio.h>
#include <stdlib.h>

#if !defined(COMP_MSC)
#include <unistd.h>
#endif

#include <sys/stat.h>
#include <string.h>
/*
#include "libtc/xio.h"
*/
void AVI_info(avi_t *avifile);

void AVI_info(avi_t *avifile)
{
    if (avifile == NULL) {
        fprintf(stderr, "[avilib] bad avi reference\n");
    } else {
        long frames = AVI_video_frames(avifile);
        int width = AVI_video_width(avifile);
        int height = AVI_video_height(avifile);
        double fps = AVI_frame_rate(avifile);
        const char *codec = AVI_video_compressor(avifile);
        int tracks = AVI_audio_tracks(avifile);
        int tmp = AVI_get_audio_track(avifile);
        int j = 0;

        printf("V: %6.3f fps, codec=%s, frames=%ld,"
               " width=%d, height=%d\n",
               fps, ((strlen(codec)==0)? "RGB": codec), frames,
               width, height);

        for (j = 0; j < tracks; j++) {
            long rate, mp3rate, chunks, tot_bytes;
            int format, chan, bits;

            AVI_set_audio_track(avifile, j);
            rate = AVI_audio_rate(avifile);
            format = AVI_audio_format(avifile);
            chan = AVI_audio_channels(avifile);
            bits = AVI_audio_bits(avifile);
            mp3rate = AVI_audio_mp3rate(avifile);

            chunks = AVI_audio_chunks(avifile);
            tot_bytes = AVI_audio_bytes(avifile);

            if (chan > 0) {
                printf("A: %ld Hz, format=0x%02x, bits=%d,"
                       " channels=%d, bitrate=%ld kbps,\n",
                       rate, format, bits,
                       chan, mp3rate);
                printf("   %ld chunks, %ld bytes, %s\n",
                       chunks, tot_bytes,
                       (AVI_get_audio_vbr(avifile)?"VBR":"CBR"));
            } else {
                printf("[avilib] A: no audio track found\n");
            }
        }
        AVI_set_audio_track(avifile, tmp); //reset
    }
}

/*************************************************************************/

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
