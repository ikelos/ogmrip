/* OGMJob - A library to spawn processes
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

/**
 * SECTION:ogmjob-exec
 * @title: OGMJobExec
 * @include: ogmjob-exec.h
 * @short_description: A spawn that runs a command line
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmjob-exec.h"
#include "ogmjob-log.h"

#include <glib/gi18n-lib.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define OGMJOB_EXEC_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMJOB_TYPE_EXEC, OGMJobExecPriv))

struct _OGMJobExecPriv
{
  OGMJobWatch watch_func;
  gpointer watch_data;
  gboolean watch_out;
  gboolean watch_err;
  GMainLoop *loop;
  GError *error;
  GPid pid;

  guint src_out;
  guint src_err;
  guint src_pid;
  gint  status;
  gint  result;
  gboolean swapped;
  gdouble fraction;
  gchar **argv;
};

static void ogmjob_exec_dispose  (GObject     *gobject);
static void ogmjob_exec_finalize (GObject     *gobject);
static gint ogmjob_exec_run      (OGMJobSpawn *spawn);
static void ogmjob_exec_cancel   (OGMJobSpawn *spawn);
static void ogmjob_exec_suspend  (OGMJobSpawn *spawn);
static void ogmjob_exec_resume   (OGMJobSpawn *spawn);

G_DEFINE_TYPE (OGMJobExec, ogmjob_exec, OGMJOB_TYPE_SPAWN)

static void
ogmjob_exec_class_init (OGMJobExecClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobSpawnClass *spawn_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmjob_exec_dispose;
  gobject_class->finalize = ogmjob_exec_finalize;

  spawn_class = OGMJOB_SPAWN_CLASS (klass);
  spawn_class->run = ogmjob_exec_run;
  spawn_class->cancel = ogmjob_exec_cancel;
  spawn_class->suspend = ogmjob_exec_suspend;
  spawn_class->resume = ogmjob_exec_resume;

  g_type_class_add_private (klass, sizeof (OGMJobExecPriv));
}

static void
ogmjob_exec_init (OGMJobExec *exec)
{
  exec->priv = OGMJOB_EXEC_GET_PRIVATE (exec);
}

static void
ogmjob_exec_dispose (GObject *gobject)
{
  OGMJobExec *exec;

  exec = OGMJOB_EXEC (gobject);

  if (exec->priv->src_out)
    g_source_remove (exec->priv->src_out);
  exec->priv->src_out = 0;

  if (exec->priv->src_err)
    g_source_remove (exec->priv->src_err);
  exec->priv->src_err = 0;

  if (exec->priv->src_pid > 0)
    g_source_remove (exec->priv->src_pid);
  exec->priv->src_pid = 0;

  if (exec->priv->pid > 0)
    kill (exec->priv->pid, SIGINT);
  exec->priv->pid = 0;

  if (exec->priv->loop)
    g_main_loop_quit (exec->priv->loop);

  G_OBJECT_CLASS (ogmjob_exec_parent_class)->dispose (gobject);
}

static void
ogmjob_exec_finalize (GObject *gobject)
{
  OGMJobExec *exec;

  exec = OGMJOB_EXEC (gobject);

  if (exec->priv->error)
  {
#ifdef G_ENABLE_DEBUG
    g_warning ("%s", exec->priv->error->message);
#endif
    g_error_free (exec->priv->error);
  }

  if (exec->priv->argv)
    g_strfreev (exec->priv->argv);

  G_OBJECT_CLASS (ogmjob_exec_parent_class)->finalize (gobject);
}

static void
ogmjob_exec_pid_notify (OGMJobExec *exec)
{
  if (exec->priv->pid)
    g_spawn_close_pid (exec->priv->pid);
  exec->priv->pid = 0;

  if (exec->priv->loop)
    g_main_loop_quit (exec->priv->loop);
}

static void
ogmjob_exec_pid_watch (GPid pid, gint status, OGMJobExec *exec)
{
  if (WIFEXITED (status))
  {
    exec->priv->status = WEXITSTATUS (status);
    if (WEXITSTATUS (status) != 0 && exec->priv->result != OGMJOB_RESULT_CANCEL)
      exec->priv->result = OGMJOB_RESULT_ERROR;
  }
  else if (WIFSIGNALED (status) && WTERMSIG (status) != SIGINT)
    exec->priv->result = OGMJOB_RESULT_ERROR;

  if (exec->priv->src_out > 0)
    g_source_remove (exec->priv->src_out);

  if (exec->priv->src_err > 0)
    g_source_remove (exec->priv->src_err);
}

static void
ogmjob_exec_stdout_notify (OGMJobExec *exec)
{
  exec->priv->src_out = 0;
}

static void
ogmjob_exec_stderr_notify (OGMJobExec *exec)
{
  exec->priv->src_err = 0;
}

static gboolean
ogmjob_exec_watch (GIOChannel *channel, GIOCondition condition, OGMJobExec *exec, gboolean do_watch, gboolean do_log)
{
  GIOStatus status;
  gsize size, bytes_read;
  gchar *buffer;

  if (exec->priv->error)
    return FALSE;

  exec->priv->error = NULL;
  size = g_io_channel_get_buffer_size (channel);
  buffer = g_new0 (char, size + 1);

  status = g_io_channel_read_chars (channel, buffer, size, &bytes_read, &exec->priv->error);

  if (status != G_IO_STATUS_NORMAL)
  {
    g_free (buffer);
    return status == G_IO_STATUS_AGAIN;
  }

  if (do_log)
    ogmjob_log_write (buffer);
#ifdef G_ENABLE_DEBUG
  else
    g_print ("%s", buffer);
#endif

  if (do_watch)
  {
    gchar **vline;

    vline = g_strsplit_set (buffer, "\r\n", -1);
    g_free (buffer);

    if (vline)
    {
      guint i;

      for (i = 0; vline[i]; i++)
      {
        if (strlen (vline[i]) > 0)
        {
          if (exec->priv->watch_func)
          {
            gdouble fraction;

            fraction = exec->priv->watch_func (exec, vline[i], exec->priv->watch_data);
            fraction = MIN (fraction, 1.0);

            if (fraction > 0.0 && fraction > exec->priv->fraction + 0.001)
            {
              exec->priv->fraction = fraction;
              if (exec->priv->swapped)
                g_signal_emit_by_name (exec->priv->watch_data, "progress", fraction);
              else
                g_signal_emit_by_name (exec, "progress", fraction);
            }
          }
        }
      }
      g_strfreev (vline);
    }
  }

  return TRUE;
}

static gboolean
ogmjob_exec_watch_stdout (GIOChannel *channel, GIOCondition condition, OGMJobExec *exec)
{
  return ogmjob_exec_watch (channel, condition, exec, exec->priv->watch_out, FALSE);
}

static gboolean
ogmjob_exec_watch_stderr (GIOChannel *channel, GIOCondition condition, OGMJobExec *exec)
{
  return ogmjob_exec_watch (channel, condition, exec, exec->priv->watch_err, TRUE);
}

static gint
ogmjob_exec_run (OGMJobSpawn *spawn)
{
  OGMJobExec *exec;
  GSpawnFlags flags;
  GIOChannel *channel;
  gint fdout, fderr;
  guint i;

  exec = OGMJOB_EXEC (spawn);

  for (i = 0; exec->priv->argv[i]; i++)
    ogmjob_log_printf ("%s ", exec->priv->argv[i]);
  ogmjob_log_write ("\n");

  exec->priv->error = NULL;
  exec->priv->result = OGMJOB_RESULT_SUCCESS;
  exec->priv->status = 0;

  flags = G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD;

  if (!g_spawn_async_with_pipes (NULL, exec->priv->argv, NULL, flags, NULL, NULL, 
        &exec->priv->pid, NULL, &fdout, &fderr, &exec->priv->error))
    return OGMJOB_RESULT_ERROR;

  exec->priv->src_pid = g_child_watch_add_full (G_PRIORITY_DEFAULT_IDLE, exec->priv->pid, 
      (GChildWatchFunc) ogmjob_exec_pid_watch, exec, (GDestroyNotify) ogmjob_exec_pid_notify);

  fcntl (fdout, F_SETFL, O_NONBLOCK);
  channel = g_io_channel_unix_new (fdout);
  g_io_channel_set_close_on_unref (channel, TRUE);
  g_io_channel_set_encoding (channel, NULL, NULL);
  exec->priv->src_out = g_io_add_watch_full (channel, G_PRIORITY_DEFAULT_IDLE,
      G_IO_IN | G_IO_HUP | G_IO_ERR, (GIOFunc) ogmjob_exec_watch_stdout, exec, 
      (GDestroyNotify) ogmjob_exec_stdout_notify);
  g_io_channel_unref (channel);

  fcntl (fderr, F_SETFL, O_NONBLOCK);
  channel = g_io_channel_unix_new (fderr);
  g_io_channel_set_close_on_unref (channel, TRUE);
  g_io_channel_set_encoding (channel, NULL, NULL);
  exec->priv->src_err = g_io_add_watch_full (channel, G_PRIORITY_DEFAULT_IDLE,
      G_IO_IN | G_IO_HUP | G_IO_ERR, (GIOFunc) ogmjob_exec_watch_stderr, exec, 
      (GDestroyNotify) ogmjob_exec_stderr_notify);
  g_io_channel_unref (channel);

  if (!ogmjob_spawn_get_async (OGMJOB_SPAWN (exec)))
  {
    exec->priv->loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (exec->priv->loop);
    g_main_loop_unref (exec->priv->loop);
    exec->priv->loop = NULL;
  }

  return exec->priv->result;
}

static void
ogmjob_exec_cancel (OGMJobSpawn *spawn)
{
  OGMJobExec *exec;

  exec = OGMJOB_EXEC (spawn);

  exec->priv->result = OGMJOB_RESULT_CANCEL;

  if (exec->priv->pid > 0)
  {
    kill (exec->priv->pid, SIGCONT);
    kill (exec->priv->pid, SIGINT);
  }
}

static void
ogmjob_exec_suspend (OGMJobSpawn *spawn)
{
  OGMJobExec *exec;

  exec = OGMJOB_EXEC (spawn);

  if (exec->priv->pid > 0)
    kill (exec->priv->pid, SIGSTOP);
}

static void
ogmjob_exec_resume (OGMJobSpawn *spawn)
{
  OGMJobExec *exec;

  exec = OGMJOB_EXEC (spawn);

  if (exec->priv->pid > 0)
    kill (exec->priv->pid, SIGCONT);
}

static void
ogmjob_exec_construct (OGMJobExec *exec, const gchar *command_line)
{
  GError *error = NULL;
  gchar **argv;
  gint argc;

  g_return_if_fail (OGMJOB_IS_EXEC (exec));
  g_return_if_fail (exec->priv->argv == NULL);
  g_return_if_fail (command_line != NULL);

  if (!g_shell_parse_argv (command_line, &argc, &argv, &error))
  {
#ifdef G_ENABLE_DEBUG
    g_warning ("%s", error->message);
#endif
    g_error_free (error);
    return;
  }

  g_return_if_fail (argc && argv && argv[0]);

  exec->priv->argv = argv;
}

static void
ogmjob_exec_constructv (OGMJobExec *exec, gchar **argv)
{
  g_return_if_fail (OGMJOB_IS_EXEC (exec));
  g_return_if_fail (exec->priv->argv == NULL);
  g_return_if_fail (argv != NULL);
  g_return_if_fail (argv[0] != NULL);

  exec->priv->argv = argv;
}

/**
 * ogmjob_exec_new:
 * @command_line: A command line
 *
 * Creates a new #OGMJobExec.
 *
 * Returns: The new #OGMJobExec
 */
OGMJobSpawn *
ogmjob_exec_new (const gchar *command_line)
{
  OGMJobExec *exec;

  exec = g_object_new (OGMJOB_TYPE_EXEC, NULL);
  ogmjob_exec_construct (exec, command_line);

  return OGMJOB_SPAWN (exec);
}

/**
 * ogmjob_exec_newv:
 * @argv: An argument vector
 *
 * Creates a new #OGMJobExec.
 *
 * Returns: The new #OGMJobExec
 */
OGMJobSpawn *
ogmjob_exec_newv (gchar **argv)
{
  OGMJobExec *exec;

  exec = g_object_new (OGMJOB_TYPE_EXEC, NULL);
  ogmjob_exec_constructv (exec, argv);

  return OGMJOB_SPAWN (exec);
}

/**
 * ogmjob_exec_add_watch_full:
 * @exec: An #OGMJobExec
 * @watch_func: An #OGMJobWatch
 * @watch_data: User data
 * @watch_out: Whether to watch stdin
 * @watch_err: Whether to watch stdout
 * @swapped: Whether to swap #exec and @watch_data when calling @watch_func
 *
 * Invokes @watch_func each time data is available on stdin or stdout.
 */
void
ogmjob_exec_add_watch_full (OGMJobExec *exec, OGMJobWatch watch_func, gpointer watch_data, 
    gboolean watch_out, gboolean watch_err, gboolean swapped)
{
  g_return_if_fail (OGMJOB_IS_EXEC (exec));

  if (swapped)
    g_return_if_fail (OGMJOB_IS_SPAWN (watch_data));

  exec->priv->watch_func = watch_func;
  exec->priv->watch_data = watch_data;
  exec->priv->swapped = swapped;
#ifdef G_ENABLE_DEBUG
  exec->priv->watch_out = TRUE;
  exec->priv->watch_err = TRUE;
#else
  exec->priv->watch_out = watch_out;
  exec->priv->watch_err = watch_err;
#endif
}

/**
 * ogmjob_exec_add_watch:
 * @exec: An #OGMJobExec
 * @watch_func: An #OGMJobWatch
 * @watch_data: User data
 *
 * Invokes @watch_func each time data is available on stdin or stdout.
 */
void
ogmjob_exec_add_watch (OGMJobExec *exec, OGMJobWatch watch_func, gpointer watch_data)
{
  g_return_if_fail (OGMJOB_IS_EXEC (exec));

  ogmjob_exec_add_watch_full (exec, watch_func, watch_data, TRUE, TRUE, FALSE);
}

/**
 * ogmjob_exec_get_status:
 * @exec: An #OGMJobExec
 *
 * Returns the execution status.
 *
 * Returns: The execution status
 */
gint
ogmjob_exec_get_status (OGMJobExec *exec)
{
  g_return_val_if_fail (OGMJOB_IS_EXEC (exec), 0);

  return exec->priv->status;
}

