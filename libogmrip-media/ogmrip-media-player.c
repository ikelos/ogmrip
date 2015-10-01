/* OGMRipMedia - A media library for OGMRip
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

#include "ogmrip-media-player.h"
#include "ogmrip-media-object.h"
#include "ogmrip-media-title.h"
#include "ogmrip-media-stream.h"
#include "ogmrip-media-audio.h"
#include "ogmrip-media-subp.h"

#include <unistd.h>

struct _OGMRipPlayerPriv
{
  OGMRipTitle *title;

  OGMRipAudioStream *astream;
  OGMRipSubpStream *sstream;

  GSubprocess *subprocess;

  guint start_chap;
  gint end_chap;
};

enum
{
  PLAY,
  STOP,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
ogmrip_mplayer_set_input (OGMRipTitle *title, GPtrArray *argv)
{
  const gchar *uri;

  uri = ogmrip_media_get_uri (ogmrip_title_get_media (title));
  if (g_str_has_prefix (uri, "file://"))
    g_ptr_array_add (argv, g_strdup (uri + 7));
  else if (g_str_has_prefix (uri, "dvd://"))
  {
    g_ptr_array_add (argv, g_strdup ("-dvd-device"));
    g_ptr_array_add (argv, g_strdup (uri + 6));
    g_ptr_array_add (argv, g_strdup_printf ("dvd://%d", ogmrip_title_get_id (title) + 1));
  }
  else if (g_str_has_prefix (uri, "br://"))
  {
    g_ptr_array_add (argv, g_strdup ("-bluray-device"));
    g_ptr_array_add (argv, g_strdup (uri + 5));
    g_ptr_array_add (argv, g_strdup_printf ("br://%d", ogmrip_title_get_id (title) + 1));
  }
  else
    g_warning ("Unknown scheme for '%s'", uri);
}

static gchar **
ogmrip_mplayer_play_command (OGMRipPlayer *player)
{
  GPtrArray *argv;

  argv = g_ptr_array_new_full (20, g_free);
  g_ptr_array_add (argv, g_strdup ("mplayer"));

  g_ptr_array_add (argv, g_strdup ("-slave"));
  g_ptr_array_add (argv, g_strdup ("-quiet"));
  g_ptr_array_add (argv, g_strdup ("-nojoystick"));
  g_ptr_array_add (argv, g_strdup ("-nolirc"));

  g_ptr_array_add (argv, g_strdup ("-nomouseinput"));
  g_ptr_array_add (argv, g_strdup ("-noconsolecontrols"));

  g_ptr_array_add (argv, g_strdup ("-cache"));
  g_ptr_array_add (argv, g_strdup ("8192"));

  g_ptr_array_add (argv, g_strdup ("-cache-min"));
  g_ptr_array_add (argv, g_strdup ("20"));

  g_ptr_array_add (argv, g_strdup ("-cache-seek-min"));
  g_ptr_array_add (argv, g_strdup ("50"));

  g_ptr_array_add (argv, g_strdup ("-noconfig"));
  g_ptr_array_add (argv, g_strdup ("all"));

  g_ptr_array_add (argv, g_strdup ("-demuxer"));
  g_ptr_array_add (argv, g_strdup ("lavf"));

  g_ptr_array_add (argv, g_strdup ("-zoom"));

  if (player->priv->astream)
  {
    g_ptr_array_add (argv, g_strdup ("-aid"));
    g_ptr_array_add (argv, g_strdup_printf ("%d",
          ogmrip_stream_get_id (OGMRIP_STREAM (player->priv->astream))));
  }
  else
    g_ptr_array_add (argv, g_strdup ("-nosound"));

  if (!player->priv->sstream)
    g_ptr_array_add (argv, g_strdup ("-nosub"));
  else
  {
    g_ptr_array_add (argv, g_strdup ("-spuaa"));
    g_ptr_array_add (argv, g_strdup ("20"));
    g_ptr_array_add (argv, g_strdup ("-sid"));
    g_ptr_array_add (argv, g_strdup_printf ("%d",
          ogmrip_stream_get_id (OGMRIP_STREAM (player->priv->sstream))));
  }

  if (player->priv->start_chap > 0 || player->priv->end_chap >= 0)
  {
    g_ptr_array_add (argv, g_strdup ("-chapter"));
    if (player->priv->end_chap >= 0)
      g_ptr_array_add (argv, g_strdup_printf ("%d-%d", player->priv->start_chap + 1, player->priv->end_chap + 1));
    else
      g_ptr_array_add (argv, g_strdup_printf ("%d", player->priv->start_chap + 1));
  }

  ogmrip_mplayer_set_input (player->priv->title, argv);

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}

static void
ogmrip_player_ready_cb (GSubprocess *subprocess, GAsyncResult *res, OGMRipPlayer *player)
{
  g_subprocess_wait_finish (subprocess, res, NULL);
  g_clear_object (&player->priv->subprocess);

  g_signal_emit (player, signals[STOP], 0);
}

static gboolean
ogmrip_player_spawn (OGMRipPlayer *player, GError **error)
{
  gchar **argv;

#ifdef G_ENABLE_DEBUG
  gint i;
#endif

  argv = ogmrip_mplayer_play_command (player);

#ifdef G_ENABLE_DEBUG
  for (i = 0; argv[i]; i++)
    g_print ("%s ", argv[i]);
  g_print ("\n");
#endif

  player->priv->subprocess = g_subprocess_newv ((const gchar * const *) argv,
      G_SUBPROCESS_FLAGS_STDIN_PIPE | G_SUBPROCESS_FLAGS_STDOUT_SILENCE | G_SUBPROCESS_FLAGS_STDERR_SILENCE, error);

  g_strfreev (argv);

  if (!player->priv->subprocess)
    return FALSE;

  g_subprocess_wait_async (player->priv->subprocess, NULL,
      (GAsyncReadyCallback) ogmrip_player_ready_cb, player);

  return TRUE;
}

G_DEFINE_TYPE_WITH_PRIVATE (OGMRipPlayer, ogmrip_player, G_TYPE_OBJECT)

static void
ogmrip_player_dispose (GObject *gobject)
{
  OGMRipPlayer *player;

  player = OGMRIP_PLAYER (gobject);

  g_clear_object (&player->priv->title);
  g_clear_object (&player->priv->astream);
  g_clear_object (&player->priv->sstream);

  G_OBJECT_CLASS (ogmrip_player_parent_class)->dispose (gobject);
}

static void
ogmrip_player_class_init (OGMRipPlayerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = ogmrip_player_dispose;

  /**
   * OGMRipPlayer::play
   * @player: the player that received the signal
   *
   * Emitted each time a title is played
   */
  signals[PLAY] = g_signal_new ("play", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipPlayerClass, play), NULL, NULL,
      g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);

  /**
   * OGMRipPlayer::stop
   * @player: the player that received the signal
   *
   * Emitted each time a title is stopped
   */
  signals[STOP] = g_signal_new ("stop", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (OGMRipPlayerClass, stop), NULL, NULL,
      g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);
}

static void
ogmrip_player_init (OGMRipPlayer *player)
{
  player->priv = ogmrip_player_get_instance_private (player);

  player->priv->start_chap = 0;
  player->priv->end_chap = -1;
}

/**
 * ogmrip_player_new:
 *
 * Creates a new #OGMRipPlayer
 *
 * Returns: the new #OGMRipPlayer
 */
OGMRipPlayer *
ogmrip_player_new (void)
{
  return g_object_new (OGMRIP_TYPE_PLAYER, NULL);
}

/**
 * ogmrip_player_set_title:
 * @player: an #OGMRipPlayer
 * @title: an #OGMRipTitle
 *
 * Sets the DVD title to play
 */
void
ogmrip_player_set_title (OGMRipPlayer *player, OGMRipTitle *title)
{
  g_return_if_fail (OGMRIP_IS_PLAYER (player));

  if (title)
    g_object_ref (title);

  if (player->priv->title)
    g_object_unref (player->priv->title);

  player->priv->title = title;
}

/**
 * ogmrip_player_set_audio_stream:
 * @player: an #OGMRipPlayer
 * @stream: an #OGMRipAudioStream
 *
 * Sets the audio stream to play
 */
void
ogmrip_player_set_audio_stream (OGMRipPlayer *player, OGMRipAudioStream *stream)
{
  g_return_if_fail (OGMRIP_IS_PLAYER (player));
  g_return_if_fail (OGMRIP_IS_AUDIO_STREAM (stream));

  if (stream)
    g_object_ref (stream);

  if (player->priv->astream)
    g_object_unref (player->priv->astream);
  player->priv->astream = stream;
}

/**
 * ogmrip_player_set_subp_stream:
 * @player: an #OGMRipPlayer
 * @stream: an #OGMRipSubpStream
 *
 * Sets the subtitle stream to play
 */
void
ogmrip_player_set_subp_stream (OGMRipPlayer *player, OGMRipSubpStream *stream)
{
  g_return_if_fail (OGMRIP_IS_PLAYER (player));
  g_return_if_fail (OGMRIP_IS_SUBP_STREAM (stream));

  if (stream)
    g_object_ref (stream);

  if (player->priv->sstream)
    g_object_unref (player->priv->sstream);
  player->priv->sstream = stream;
}

/**
 * ogmrip_player_set_chapters:
 * @player: an #OGMRipPlayer
 * @start: the chapter to start playing at
 * @end: the chapter to stop playing at, or -1
 *
 * Sets the chapters to play
 */
void
ogmrip_player_set_chapters (OGMRipPlayer *player, guint start, gint end)
{
  g_return_if_fail (OGMRIP_IS_PLAYER (player));
  g_return_if_fail (end < 0 || start <= (guint) end);

  player->priv->start_chap = start;
  player->priv->end_chap = end;
}

/**
 * ogmrip_player_play:
 * @player: an #OGMRipPlayer
 * @error: return location for error
 *
 * Plays the selected title, streams and chapters
 *
 * Returns: %TRUE on success, %FALSE if an error was set
 */
gboolean
ogmrip_player_play (OGMRipPlayer *player, GError **error)
{
  gboolean retval;

  g_return_val_if_fail (OGMRIP_IS_PLAYER (player), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (!player->priv->title)
    return FALSE;

  retval = ogmrip_player_spawn (player, error);
  if (retval)
    g_signal_emit (player, signals[PLAY], 0);

  return retval;
}

/**
 * ogmrip_player_stop:
 * @player: an #OGMRipPlayer
 *
 * Stops playing the title
 */
void
ogmrip_player_stop (OGMRipPlayer *player)
{
  g_return_if_fail (OGMRIP_IS_PLAYER (player));

  if (player->priv->subprocess)
  {
    GOutputStream *stream;

    stream = g_subprocess_get_stdin_pipe (player->priv->subprocess);
    if (!g_output_stream_write_all (stream, "stop\n", 5, NULL, NULL, NULL))
      g_warning ("Couldn't write to file descriptor");
  }
}

