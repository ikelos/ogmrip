#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "avilib.h"
#include "aud_scan.h"

#define BPP                        24
#define TC_MAX_V_FRAME_WIDTH     2500
#define TC_MAX_V_FRAME_HEIGHT    2000
#define MBYTE                (1 << 20)

#define SIZE_RGB_FRAME ((int) TC_MAX_V_FRAME_WIDTH * TC_MAX_V_FRAME_HEIGHT * (BPP / 8))

typedef struct _media_file_t  media_file_t;
typedef struct _media_list_t  media_list_t;

struct _media_file_t
{
  const char *name;

  FILE *f;
  avi_t *avi;

  char *codec;
  int tracks, chan, bits, format, width, height, head_len, error;
  long mp3_rate, rate;
  double fps, aud_ms;
};

struct _media_list_t
{
  media_file_t *file;
  media_list_t *next;
};

void AVI_info (avi_t *avi);

static char data[SIZE_RGB_FRAME];

static int verbose = 0;

int
getopt_long (int argc, char * const *argv, const char *shortopts, const struct option *longopts, int *longind)
{
  return getopt (argc, argv, shortopts);
}

static void
usage (const char *name)
{
  fprintf (stdout, "Usage:\n");
  fprintf (stdout, "  %s [global options] -o <output> -i <input> [local options] ...\n", name);
  fprintf (stdout, "\n");
  fprintf (stdout, "Global Options:\n");
  fprintf (stdout, "  -h, --help                 Show help options\n");
  fprintf (stdout, "  -v, --verbose              Show verbose output\n");
  fprintf (stdout, "  -f, --fourcc <fourcc>      Specify the FOURCC of the output file\n");
  fprintf (stdout, "  -o, --output <filename>    Specify the name of the output file\n");
  fprintf (stdout, "  -s, --split <size>         Specify the split size in MB\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "Local Options:\n");
  fprintf (stdout, "  -i, --input    <filename>  Specify the input avi\n");
  fprintf (stdout, "  -n, --noaudio              Don't copy any audio stream from the input file\n");
  fprintf (stdout, "\n");
}

static media_file_t *
media_file_new (char *filename)
{
  media_file_t *avi;

  if (!filename)
    return NULL;

  avi = (media_file_t *) malloc (sizeof (media_file_t));
  memset (avi, 0, sizeof (media_file_t));

  avi->name = filename;

  return avi;
}

static void media_file_close (media_file_t *file);

static void
media_file_free (media_file_t *file)
{
  if (file)
  {
    media_file_close (file);
    free (file);
  }
}

static int
media_file_open_avi (media_file_t *file, int wrt)
{
  if (!file)
    return -1;

  if (wrt)
    file->avi = AVI_open_output_file (file->name);
  else
    file->avi = AVI_open_input_file (file->name, 1);
  if (!file->avi)
    return -1;

  if (!wrt)
  {
    if (verbose)
    {
      AVI_info (file->avi);
      fflush (stdout);
    }

    file->tracks = AVI_audio_tracks (file->avi);

    file->width = AVI_video_width (file->avi);
    file->height = AVI_video_height (file->avi);

    file->rate = AVI_audio_rate (file->avi);
    file->chan = AVI_audio_channels (file->avi);
    file->bits = AVI_audio_bits (file->avi);

    file->format = AVI_audio_format (file->avi);
    file->mp3_rate = AVI_audio_mp3rate (file->avi);

    file->fps =  AVI_frame_rate (file->avi);

    file->codec = AVI_video_compressor (file->avi);
  }

  return 0;
}

static int
media_file_open_mp3 (media_file_t *file)
{
  int ret = -1;

  if (!file)
    return -1;

  file->f = fopen (file->name, "r");
  if (file->f)
  {
    unsigned char *c, head[1024];

    c = head;
    if (fread (head, 1, 1024, file->f) == 1024)
    {
      while ((c - head < 1024 - 8) && (ret = tc_probe_audio_header (c, 8)) <= 0)
        c ++;

      if (ret > 0)
      {
        int len, rate, mp3_rate;
        long offset;
        
        offset = c - head;

        fseek (file->f, offset, SEEK_SET);
        len = fread (head, 1, 8, file->f);

        file->format  = tc_probe_audio_header (head, len);
        file->head_len = tc_get_audio_header (head, len, file->format, &file->chan, &rate, &mp3_rate);

        if (verbose)
          fprintf (stdout, "%s looks like a %s track\n", file->name, file->format == 0x55 ? "MP3" : "AC3");

        fseek (file->f, offset, SEEK_SET);

        if (file->format == 0x55 || file->format == 0x2000)
        {
          file->tracks = 1;
          file->mp3_rate = mp3_rate;
          file->rate = rate;
          file->bits = 16;

          ret = 0;
        }
      }
    }
  }

  return ret;
}

static void media_file_close (media_file_t *file);

static int
media_file_open (media_file_t *file, int wrt)
{
  int retval;

  if (!file)
    return -1;

  file->error = 0;
  file->aud_ms = 0.0;
  file->head_len = 0;

  media_file_close (file);

  retval = media_file_open_avi (file, wrt);
  if (retval < 0)
    retval = media_file_open_mp3 (file);

  return retval;
}

static void
media_file_close (media_file_t *file)
{
  if (file)
  {
    if (file->avi)
    {
      AVI_close (file->avi);
      file->avi = NULL;
    }

    if (file->f)
    {
      fclose (file->f);
      file->f = NULL;
    }
  }
}

static int
media_file_set_audio_track (media_file_t *file, unsigned int track)
{
  if (!file)
    return -1;

  if (file->f)
    return 0;

  if (!file->avi)
    return -1;

  return AVI_set_audio_track (file->avi, track);
}

static int
media_file_merge_avi (media_file_t *output, media_file_t *audio, double vid_ms)
{
  double aud_ms;

  if (audio->chan)
    sync_audio_video_avi2avi (vid_ms, &aud_ms, audio->avi, output->avi);

  return 0;
}

static int
media_file_merge_mp3 (media_file_t *output, media_file_t *audio, double vid_ms)
{
  if (audio->head_len > 4 && !audio->error)
  {
    unsigned char head[8];
    int len, mp3_rate;
    off_t pos;

    while (audio->aud_ms < vid_ms)
    {
      pos = ftell (audio->f);

      len = fread (head, 1, 8, audio->f);
      if (len <= 0)
      {
        fprintf (stderr, "EOF in %s; continuing ..\n", audio->name);
        audio->error = 1;
        break;
      }

      audio->head_len = tc_get_audio_header (head, len, audio->format, NULL, NULL, &mp3_rate);

      if (audio->head_len < 0)
      {
        audio->aud_ms = vid_ms;
        audio->error = 1;
      }
      else
        audio->aud_ms += audio->head_len * 8.0 / mp3_rate;

      fseek (audio->f, pos, SEEK_SET);

      len = fread (data, audio->head_len, 1, audio->f);
      if (len <= 0)
      {
        fprintf (stderr, "EOF in %s; continuing ..\n", audio->name);
        audio->error = 1;
        break;
      }

      if (AVI_write_audio (output->avi, data, audio->head_len) < 0)
      {
        AVI_print_error ("AVI write audio frame");
        return -1;
      }
    }
  }

  return 0;
}

static int
media_file_merge (media_file_t *input, media_list_t *list, const char *basename, char *fourcc, int chunk)
{
  media_list_t *link;
  media_file_t *output = NULL;

  char filename[FILENAME_MAX];

  int key, tracks;
  long i, j, frames, bytes, file = 1, retval = -1;

  unsigned long vid_chunks;
  double vid_ms, aud_ms_w[AVI_MAX_TRACKS];

  if (!input)
    return -1;

  /* initializing local variables */
  for (i = 0; i < AVI_MAX_TRACKS; i ++)
    aud_ms_w[i] = 0.0;
  vid_chunks = 0;

  /* opening audio tracks */
  for (link = list; link; link = link->next)
  {
    if (media_file_open (link->file, 0) < 0)
    {
      AVI_print_error (link->file->name);
      goto merge_cleanup;
    }
  }

  AVI_seek_start (input->avi);
  frames = AVI_video_frames (input->avi);

  for (i = 0; i < frames; i ++)
  {
    /* reading video */
    if ((bytes = AVI_read_frame (input->avi, data, &key)) < 0)
    {
      AVI_print_error ("AVI read video frame");
      goto merge_cleanup;
    }

    /* is there enough space in the output file ? */
    if (output && key && chunk > 0 && AVI_bytes_written (output->avi) + bytes > (uint64_t) (chunk * MBYTE))
    {
      media_file_free (output);
      output = NULL;
    }

    if (!output)
    {
      if (file > 1)
        fprintf (stdout, "\n");

      if (chunk > 0)
        snprintf (filename, FILENAME_MAX, "%s-%04ld.avi", basename, file ++);
      else
        snprintf (filename, FILENAME_MAX, "%s.avi", basename);

      output = media_file_new (filename);
      if (media_file_open (output, 1) < 0)
      {
        AVI_print_error ("AVI open");
        return -1;
      }

      /* configuring video from input file */
      AVI_set_video (output->avi, input->width, input->height, input->fps, fourcc ? fourcc : input->codec);

      /* configuring audio tracks from input file */
      for (tracks = 0; tracks < input->tracks; tracks ++)
      {
        media_file_set_audio_track (input, tracks);
        AVI_set_audio (output->avi, input->chan, input->rate, input->bits, input->format, input->mp3_rate);
        AVI_set_audio_vbr (output->avi, AVI_get_audio_vbr (input->avi));
      }

      /* configuring new audio tracks */
      for (link = list; link; link = link->next)
      {
        for (j = 0; j < link->file->tracks; j ++)
        {
          media_file_set_audio_track (output, tracks);
          media_file_set_audio_track (link->file, j);

          AVI_set_audio (output->avi, link->file->chan, link->file->rate, link->file->bits, link->file->format, link->file->mp3_rate);
          AVI_set_audio_vbr (output->avi, link->file->avi ? AVI_get_audio_vbr (link->file->avi) : 1);

          tracks ++;
        }
      }

      output->tracks = tracks;
    }

    /* writing video */
    if (AVI_write_frame (output->avi, data, bytes, key) < 0)
    {
      AVI_print_error ("AVI write video frame");
      goto merge_cleanup;
    }

    vid_chunks ++;
    vid_ms = vid_chunks * 1000.0 / input->fps;

    /* copying audio tracks from input file */
    for (tracks = 0; tracks < input->tracks; tracks ++)
    {
      media_file_set_audio_track (input, tracks);
      media_file_set_audio_track (output, tracks);

      if (input->chan)
        sync_audio_video_avi2avi (vid_ms, &aud_ms_w[tracks], input->avi, output->avi);
    }

    /* adding new audio tracks */
    for (link = list; link; link = link->next)
    {
      for (j = 0; j < link->file->tracks; j ++)
      {
        media_file_set_audio_track (output, tracks);
        media_file_set_audio_track (link->file, j);

        if ((link->file->avi && media_file_merge_avi (output, link->file, vid_ms) == 0) ||
            (link->file->f   && media_file_merge_mp3 (output, link->file, vid_ms) == 0))
          tracks ++;
      }
    }

    fprintf (stdout, "\r%s: %06ld-%06ld frames written (%.0f%%)", output->name, i + 1, frames, (i + 1) / (double) frames * 100);
    fflush (stdout);
  }

  fprintf (stdout, "\n");
  fflush (stdout);

  retval = 0;

merge_cleanup:

  for (link = list; link; link = link->next)
    media_file_close (link->file);

  media_file_free (output);

  return retval;
}

int
media_file_set_fourcc (media_file_t *file, const char *fourcc)
{
  size_t nitems;
  off_t vh_offset, vf_offset;
  char codec[5];

  int retval = -1;

  if (media_file_open (file, 0) < 0)
  {
    AVI_print_error (file->name);
    return -1;
  }

  vh_offset = AVI_video_codech_offset (file->avi);
  vf_offset = AVI_video_codecf_offset (file->avi);

  fseek (file->f, vh_offset, SEEK_SET);
  if (fread (codec, 1, 4, file->f) != 4)
    goto fourcc_cleanup;
  codec[4] = 0;

  fseek (file->f, vf_offset, SEEK_SET);
  if (fread (codec, 1, 4, file->f) != 4)
    goto fourcc_cleanup;
  codec[4] = 0;

  fseek (file->f, vh_offset, SEEK_SET);
  if (strncmp (fourcc, "RGB", 3) == 0)
    nitems = fwrite (codec, 1, 4, file->f);
  else
    nitems = fwrite (fourcc, 1, 4, file->f);

  if (nitems != 4)
    goto fourcc_cleanup;

  fseek (file->f, vf_offset, SEEK_SET);
  if (strncmp (fourcc, "RGB", 3) == 0)
  {
    memset (codec, 0, 4);
    nitems = fwrite (codec, 1, 4, file->f);
  }
  else
    nitems = fwrite (fourcc, 1, 4, file->f);

  if (nitems != 4)
    goto fourcc_cleanup;

  retval = 0;

fourcc_cleanup:
  media_file_close (file);

  return retval;
}

static media_list_t *
media_list_append (media_list_t *list, media_file_t *file)
{
  media_list_t *link;

  if (!file)
    return list;

  link = (media_list_t *) malloc (sizeof (media_list_t));

  link->file = file;
  link->next = NULL;

  if (!list)
    return link;

  while (list->next)
    list = list->next;

  list->next = link;

  return list;
}

static void
media_list_free (media_list_t *list)
{
  media_list_t *link;

  while (list)
  {
    link = list;
    list = list->next;

    media_file_free (link->file);
    free (link);
  }
}

static const char *shortopts = "hvno:f:i:s:";
static const struct option longopts[] =
{
  { "help",     no_argument,       NULL, 'h' },
  { "verbose",  no_argument,       NULL, 'v' },
  { "noaudio",  no_argument,       NULL, 'n' },
  { "output",   required_argument, NULL, 'o' },
  { "fourcc",   required_argument, NULL, 'f' },
  { "input",    required_argument, NULL, 'i' },
  { "split",    required_argument, NULL, 's' },
  { NULL,       0,                 NULL,  0  }
};

int
main (int argc, char *argv[])
{
  media_file_t *input = NULL, *audio = NULL;
  media_list_t *list = NULL;
  char *fourcc = NULL;

  char basename[FILENAME_MAX];

  int retval = EXIT_FAILURE;
  int optidx, optelt, noaudio = 0, chunk = 0;

  basename[0] = '\0';
  
  while ((optelt = getopt_long (argc, argv, shortopts, longopts, &optidx)) != EOF)
  {
    switch (optelt)
    {
      case 'h':
        usage (argv[0]);
        goto cleanup;
        break;
      case 'v':
        verbose = 1;
        break;
      case 'f':
        fourcc = optarg;
        break;
      case 'o':
        strncpy (basename, optarg, FILENAME_MAX);
        break;
      case 'i':
        if (input)
          media_file_free (input);
        input = media_file_new (optarg);
        break;
      case 's':
        chunk = atoi (optarg);
        break;
      case 'n':
        noaudio = 1;
        break;
      default:
        break;
    }
  }

  if (!input)
  {
    usage (argv[0]);
    goto cleanup;
  }

  if (chunk < 0)
  {
    usage (argv[0]);
    goto cleanup;
  }

  if (media_file_open (input, 0) < 0)
  {
    AVI_print_error (input->name);
    goto cleanup;
  }

  while (optind < argc)
  {
    audio = media_file_new (argv[optind ++]);
    list = media_list_append (list, audio);
  }

  if (basename[0] == '\0')
    strncpy (basename, input->name, FILENAME_MAX);

  {
    int i;

    i = strlen (basename);
    while (i && basename[i] != '.')
      i --;

    if (i)
      basename[i] = '\0';
  }

  /* forcing number of tracks to 0 to avoid copying them */
  if (noaudio)
    input->tracks = 0;

  if (media_file_merge (input, list, basename, fourcc, chunk) < 0)
    goto cleanup;

  retval = EXIT_SUCCESS;

cleanup:
  media_file_free (input);
  media_list_free (list);

  return retval;
}

