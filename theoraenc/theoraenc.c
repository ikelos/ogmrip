/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _REENTRANT
#define _REENTRANT
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <math.h>

#include <theora/theora.h>

const char *optstring = "ho:b:q:a:f:";
struct option options[] = 
{
  { "help",      no_argument,       NULL, 'h' },
  { "output",    required_argument, NULL, 'o' },
  { "bitrate",   required_argument, NULL, 'b' },
  { "quality",   required_argument, NULL, 'q' },
  { "aspect",    required_argument, NULL, 'a' },
  { "framerate", required_argument, NULL, 'f' },
  { NULL,        0,                 NULL,  0  }
};

FILE *video = NULL;

int video_x = 0;
int video_y = 0;
int frame_x = 0;
int frame_y = 0;
int frame_x_offset = 0;
int frame_y_offset = 0;
int video_hzn = -1;
int video_hzd = -1;
int video_an = -1;
int video_ad = -1;

int video_r = -1;
int video_q = 16;

static void
usage (const char *name)
{
  fprintf (stdout, "Usage:\n");
  fprintf (stdout, "  %s [OPTION...] <VIDEO FILE>\n", name);
  fprintf (stdout, "\n");
  fprintf (stdout, "Help Options:\n");
  fprintf (stdout, "  -h, --help        Show help options\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "Application Options:\n");
  fprintf (stdout, "  -o, --output=<filename>     Use filename for output (default: stdout)\n");
  fprintf (stdout, "  -b, --bitrate=<bitrate>     Bitrate target for Theora video\n");
  fprintf (stdout, "  -q, --quality=<quality>     Theora quality selector from 0 to 10 (0 yields\n");
  fprintf (stdout, "                              smallest files but lowest video quality. 10 yields\n");
  fprintf (stdout, "                              highest fidelity but large files)\n");
  fprintf (stdout, "  -a, --aspect=<aspect>       Set the aspect ratio of the input file\n");
  fprintf (stdout, "  -f, --framerate=<rate>      Set the framerate of the input file\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "%s only accepts YUV4MPEG2 uncompressed video.\n", name);
  fprintf (stdout, "\n");
}

static int
id_file (char *f)
{
  FILE *test;
  char buffer[80];
  int ret, tmp_video_hzn, tmp_video_hzd, tmp_video_an, tmp_video_ad;

  /*
   * open it, look for magic 
   */

  if (!strcmp (f, "-"))
  {
    /*
     * stdin 
     */
    test = stdin;
  }
  else
  {
    test = fopen (f, "rb");
    if (!test)
    {
      fprintf (stderr, "Unable to open file %s.\n", f);
      return -1;
    }
  }

  ret = fread (buffer, 1, 4, test);
  if (ret < 4)
  {
    fprintf (stderr, "EOF determining file type of file %s.\n", f);
    return -1;
  }

  if (!memcmp (buffer, "YUV4", 4))
  {
    /*
     * possible YUV2MPEG2 format file 
     */
    /*
     * read until newline, or 80 cols, whichever happens first 
     */
    int i;
    for (i = 0; i < 79; i++)
    {
      ret = fread (buffer + i, 1, 1, test);
      if (ret < 1)
        goto yuv_err;
      if (buffer[i] == '\n')
        break;
    }
    if (i == 79)
      fprintf (stderr, "Error parsing %s header; not a YUV2MPEG2 file?\n", f);

    buffer[i] = '\0';
    if (!memcmp (buffer, "MPEG", 4))
    {
      char interlace;

      if (video)
      {
        /*
         * umm, we already have one 
         */
        fprintf (stderr, "Multiple video files specified on command line.\n");
        return -1;
      }

      if (buffer[4] != '2')
        fprintf (stderr, "Incorrect YUV input file version; YUV4MPEG2 required.\n");

      ret = sscanf (buffer, "MPEG2 W%d H%d F%d:%d I%c A%d:%d", 
          &frame_x, &frame_y, &tmp_video_hzn, &tmp_video_hzd, 
          &interlace, &tmp_video_an, &tmp_video_ad);
      if (ret < 7)
      {
        fprintf (stderr, "Error parsing YUV4MPEG2 header in file %s.\n", f);
        return -1;
      }

      /*
       * update fps and aspect ratio globals if not specified in the command line 
       */
      if (video_hzn == -1)
        video_hzn = tmp_video_hzn;
      if (video_hzd == -1)
        video_hzd = tmp_video_hzd;
      if (video_an == -1)
        video_an = tmp_video_an;
      if (video_ad == -1)
        video_ad = tmp_video_ad;

      if (interlace != 'p')
      {
        fprintf (stderr, "Input video is interlaced; Theora handles only progressive scan\n");
        return -1;
      }

      video = test;

      fprintf (stderr, "File %s is %dx%d %.02f fps YUV12 video.\n", 
          f, frame_x, frame_y, (double) video_hzn / video_hzd);

      return 0;
    }
  }

  fprintf (stderr, "Input file %s is not a YUV4MPEG2 file.\n", f);
  return -1;

yuv_err:
  fprintf (stderr, "EOF parsing YUV4MPEG2 file %s.\n", f);
  return -1;
}

int spinner = 0;
char *spinascii = "|/-\\";

static void
spinnit (void)
{
  spinner++;
  if (spinner == 4)
    spinner = 0;
  fprintf (stderr, "\r%c", spinascii[spinner]);
}

static int
fetch_and_process_video (FILE *video, ogg_page *videopage, ogg_stream_state *to, theora_state *td, int videoflag)
{
  /*
   * You'll go to Hell for using static variables 
   */
  static int state = -1;
  static unsigned char *yuvframe[2];
  unsigned char *line;
  yuv_buffer yuv;
  ogg_packet op;
  int i, e;

  if (state == -1)
  {
    /*
     * initialize the double frame buffer 
     */
    yuvframe[0] = malloc (video_x * video_y * 3 / 2);
    yuvframe[1] = malloc (video_x * video_y * 3 / 2);

    /*
     * clear initial frame as it may be larger than actual video data 
     */
    /*
     * fill Y plane with 0x10 and UV planes with 0X80, for black data 
     */
    memset (yuvframe[0], 0x10, video_x * video_y);
    memset (yuvframe[0] + video_x * video_y, 0x80, video_x * video_y / 2);
    memset (yuvframe[1], 0x10, video_x * video_y);
    memset (yuvframe[1] + video_x * video_y, 0x80, video_x * video_y / 2);

    state = 0;
  }

  /*
   * is there a video page flushed?  If not, work until there is. 
   */
  while (!videoflag)
  {
    spinnit ();

    if (ogg_stream_pageout (to, videopage) > 0)
      return 1;
    if (ogg_stream_eos (to))
      return 0;

    {
      /*
       * read and process more video 
       */
      /*
       * video strategy reads one frame ahead so we know when we're
       * at end of stream and can mark last video frame as such
       * (vorbis audio has to flush one frame past last video frame
       * due to overlap and thus doesn't need this extra work 
       */

      /*
       * have two frame buffers full (if possible) before
       * proceeding.  after first pass and until eos, one will
       * always be full when we get here 
       */

      for (i = state; i < 2; i++)
      {
        char c, frame[6];
        int ret = fread (frame, 1, 6, video);

        /*
         * match and skip the frame header 
         */
        if (ret < 6)
          break;
        if (memcmp (frame, "FRAME", 5))
        {
          fprintf (stderr, "Loss of framing in YUV input data\n");
          exit (EXIT_FAILURE);
        }
        if (frame[5] != '\n')
        {
          int j;
          for (j = 0; j < 79; j++)
            if (fread (&c, 1, 1, video) && c == '\n')
              break;
          if (j == 79)
          {
            fprintf (stderr, "Error parsing YUV frame header\n");
            exit (EXIT_FAILURE);
          }
        }

        /*
         * read the Y plane into our frame buffer with centering 
         */
        line = yuvframe[i] + video_x * frame_y_offset + frame_x_offset;
        for (e = 0; e < frame_y; e++)
        {
          ret = fread (line, 1, frame_x, video);
          if (ret != frame_x)
            break;
          line += video_x;
        }
        /*
         * now get U plane
         */
        line = yuvframe[i] + (video_x * video_y) + (video_x / 2) * (frame_y_offset / 2) + frame_x_offset / 2;
        for (e = 0; e < frame_y / 2; e++)
        {
          ret = fread (line, 1, frame_x / 2, video);
          if (ret != frame_x / 2)
            break;
          line += video_x / 2;
        }
        /*
         * and the V plane
         */
        line = yuvframe[i] + (video_x * video_y * 5 / 4) + (video_x / 2) * (frame_y_offset / 2) + frame_x_offset / 2;
        for (e = 0; e < frame_y / 2; e++)
        {
          ret = fread (line, 1, frame_x / 2, video);
          if (ret != frame_x / 2)
            break;
          line += video_x / 2;
        }
        state++;
      }

      if (state < 1)
      {
        /*
         * can't get here unless YUV4MPEG stream has no video 
         */
        fprintf (stderr, "Video input contains no frames.\n");
        exit (EXIT_FAILURE);
      }

      /*
       * Theora is a one-frame-in,one-frame-out system; submit a frame
       * for compression and pull out the packet 
       */

      {
        yuv.y_width = video_x;
        yuv.y_height = video_y;
        yuv.y_stride = video_x;

        yuv.uv_width = video_x / 2;
        yuv.uv_height = video_y / 2;
        yuv.uv_stride = video_x / 2;

        yuv.y = yuvframe[0];
        yuv.u = yuvframe[0] + video_x * video_y;
        yuv.v = yuvframe[0] + video_x * video_y * 5 / 4;
      }

      theora_encode_YUVin (td, &yuv);

      /*
       * if there's only one frame, it's the last in the stream 
       */
      if (state < 2)
        theora_encode_packetout (td, 1, &op);
      else
        theora_encode_packetout (td, 0, &op);

      ogg_stream_packetin (to, &op);

      {
        unsigned char *temp = yuvframe[0];
        yuvframe[0] = yuvframe[1];
        yuvframe[1] = temp;
        state--;
      }

    }
  }

  return videoflag;
}

int
main (int argc, char *argv[])
{
  int c, long_option_index;

  ogg_stream_state to;          /* take physical pages, weld into a logical
                                 * stream of packets */
  ogg_stream_state vo;          /* take physical pages, weld into a logical
                                 * stream of packets */
  ogg_page og;                  /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet op;                /* one raw packet of data for decode */

  theora_state td;
  theora_info ti;
  theora_comment tc;

  int videoflag = 0;
  int vkbps = 0;

  ogg_int64_t video_bytesout = 0;
  double timebase;

  FILE *outfile = stdout;

  while ((c = getopt_long (argc, argv, optstring, options, &long_option_index)) != EOF)
  {
    switch (c)
    {
      case 'o':
        outfile = fopen (optarg, "wb");
        if (outfile == NULL)
        {
          fprintf (stderr, "Unable to open output file '%s'\n", optarg);
          return EXIT_FAILURE;
        }
        break;;

      case 'q':
        video_q = rint (atof (optarg) * 6.3);
        if (video_q < 0 || video_q > 63)
        {
          fprintf (stderr, "Illegal video quality (choose 0 through 10)\n");
          return EXIT_FAILURE;
        }
        video_r = 0;
        break;

      case 'b':
        video_r = rint (atof (optarg) * 1000);
        if (video_r < 45000 || video_r > 2000000)
        {
          fprintf (stderr, "Illegal video bitrate (choose 45kbps through 2000kbps)\n");
          return EXIT_FAILURE;
        }
        video_q = 0;
        break;

      case 'a':
        if (sscanf (optarg, "%d/%d", &video_an, &video_ad) != 2)
        {
          fprintf (stderr, "Cannot parse aspect ratio\n");
          return (EXIT_FAILURE);
        }
        break;

      case 'f':
        if (sscanf (optarg, "%d/%d", &video_hzn, &video_hzd) != 2)
        {
          fprintf (stderr, "Cannot parse framerate\n");
          return EXIT_FAILURE;
        }
        break;

      case 'h':
        usage (argv[0]);
        return EXIT_SUCCESS;

      default:
        return EXIT_FAILURE;
    }
  }

  while (optind < argc)
  {
    /*
     * assume that anything following the options must be a filename 
     */
    if (id_file (argv[optind]) < 0)
      return EXIT_FAILURE;
    optind++;
  }

  /*
   * yayness.  Set up Ogg output stream 
   */
  srand (time (NULL));
  {
    /*
     * need two inequal serial numbers 
     */
    int serial1, serial2;
    serial1 = rand ();
    serial2 = rand ();
    if (serial1 == serial2)
      serial2++;
    ogg_stream_init (&to, serial1);
    ogg_stream_init (&vo, serial2);
  }

  /*
   * Set up Theora encoder 
   */
  if (!video)
  {
    usage (argv[0]);

    return EXIT_FAILURE;
  }
  /*
   * Theora has a divisible-by-sixteen restriction for the encoded video size 
   */
  /*
   * scale the frame size up to the nearest /16 and calculate offsets 
   */
  video_x = ((frame_x + 15) >> 4) << 4;
  video_y = ((frame_y + 15) >> 4) << 4;
  /*
   * We force the offset to be even.
   * This ensures that the chroma samples align properly with the luma
   * samples. 
   */
  frame_x_offset = ((video_x - frame_x) / 2) & ~1;
  frame_y_offset = ((video_y - frame_y) / 2) & ~1;

  theora_info_init (&ti);
  ti.width = video_x;
  ti.height = video_y;
  ti.frame_width = frame_x;
  ti.frame_height = frame_y;
  ti.offset_x = frame_x_offset;
  ti.offset_y = frame_y_offset;
  ti.fps_numerator = video_hzn;
  ti.fps_denominator = video_hzd;
  ti.aspect_numerator = video_an;
  ti.aspect_denominator = video_ad;
  ti.colorspace = OC_CS_UNSPECIFIED;
  ti.pixelformat = OC_PF_420;
  ti.target_bitrate = video_r;
  ti.quality = video_q;

  ti.dropframes_p = 0;
  ti.quick_p = 1;
  ti.keyframe_auto_p = 1;
  ti.keyframe_frequency = 64;
  ti.keyframe_frequency_force = 64;
  ti.keyframe_data_target_bitrate = video_r * 1.5;
  ti.keyframe_auto_threshold = 80;
  ti.keyframe_mindistance = 8;
  ti.noise_sensitivity = 1;

  theora_encode_init (&td, &ti);
  theora_info_clear (&ti);

  /*
   * write the bitstream header packets with proper page interleave 
   */

  /*
   * first packet will get its own page automatically 
   */
  theora_encode_header (&td, &op);
  ogg_stream_packetin (&to, &op);
  if (ogg_stream_pageout (&to, &og) != 1)
  {
    fprintf (stderr, "Internal Ogg library error.\n");
    return EXIT_FAILURE;
  }
  if (fwrite (og.header, 1, og.header_len, outfile) != og.header_len)
  {
    fprintf (stderr, "Cannot write Ogg header\n");
    return EXIT_FAILURE;
  }
  if (fwrite (og.body, 1, og.body_len, outfile) != og.body_len)
  {
    fprintf (stderr, "Cannot write Ogg body\n");
    return EXIT_FAILURE;
  }

  /*
   * create the remaining theora headers 
   */
  theora_comment_init (&tc);
  theora_encode_comment (&tc, &op);
  ogg_stream_packetin (&to, &op);
  theora_encode_tables (&td, &op);
  ogg_stream_packetin (&to, &op);

  /*
   * Flush the rest of our headers. This ensures
   * the actual data in each stream will start
   * on a new page, as per spec. 
   */
  while (1)
  {
    int result = ogg_stream_flush (&to, &og);
    if (result < 0)
    {
      /*
       * can't get here 
       */
      fprintf (stderr, "Internal Ogg library error.\n");
      return EXIT_FAILURE;
    }
    if (result == 0)
      break;
    if (fwrite (og.header, 1, og.header_len, outfile) != og.header_len)
    {
      fprintf (stderr, "Cannot write Ogg header\n");
      return EXIT_FAILURE;
    }
    if (fwrite (og.body, 1, og.body_len, outfile) != og.body_len)
    {
      fprintf (stderr, "Cannot write Ogg body\n");
      return EXIT_FAILURE;
    }
  }

  /*
   * setup complete.  Raw processing loop 
   */
  fprintf (stderr, "Compressing....\n");
  while (1)
  {
    ogg_page videopage;

    /*
     * is there a video page flushed?  If not, fetch one if possible 
     */
    videoflag = fetch_and_process_video (video, &videopage, &to, &td, videoflag);

    /*
     * no pages of either?  Must be end of stream. 
     */
    if (!videoflag)
      break;

    /*
     * which is earlier; the end of the audio page or the end of the
     * video page? Flush the earlier to stream 
     */
    {
      double videotime = theora_granule_time (&td, ogg_page_granulepos (&videopage));

      /*
       * flush a video page 
       */
      video_bytesout += fwrite (videopage.header, 1, videopage.header_len, outfile);
      video_bytesout += fwrite (videopage.body, 1, videopage.body_len, outfile);
      videoflag = 0;
      timebase = videotime;

      {
        int hundredths = timebase * 100 - (long) timebase * 100;
        int seconds = (long) timebase % 60;
        int minutes = ((long) timebase / 60) % 60;
        int hours = (long) timebase / 3600;

        vkbps = rint (video_bytesout * 8. / timebase * .001);

        fprintf (stderr, "\r      %d:%02d:%02d.%02d video: %dkbps                 ", 
            hours, minutes, seconds, hundredths, vkbps);
      }
    }

  }

  /*
   * clear out state 
   */

  if (video)
  {
    ogg_stream_clear (&to);
    theora_clear (&td);
  }

  if (outfile && outfile != stdout)
    fclose (outfile);

  fprintf (stderr, "\r   \ndone.\n\n");

  return EXIT_SUCCESS;
}

