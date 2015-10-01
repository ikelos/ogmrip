/* OGMJob - A library to spawn processes
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

/**
 * SECTION:ogmjob-spawn
 * @title: OGMJobSpawn
 * @include: ogmjob-spawn.h
 * @short_description: A task that runs a command line
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmjob-spawn.h"

#include <ogmrip-base.h>

#include <glib/gi18n-lib.h>

#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

struct _OGMJobSpawnPriv
{
  GPid pid;
  gint status;
  gchar **argv;

  OGMJobWatch watch_out;
  GDestroyNotify notify_out;
  gpointer data_out;

  OGMJobWatch watch_err;
  GDestroyNotify notify_err;
  gpointer data_err;
};

enum
{
  PROP_0,
  PROP_ARGV
};

typedef struct
{
  GError *error;

  GCancellable *cancellable;
  gulong handler;

  gchar *partial_out;
  gchar *partial_err;

  guint src_out;
  guint src_err;
  guint src_pid;
} TaskData;

static void
free_data (TaskData *data)
{
  if (data->cancellable)
  {
    g_cancellable_disconnect (data->cancellable, data->handler);
    g_object_unref (data->cancellable);
  }

  if (data->error)
    g_error_free (data->error);

  g_free (data->partial_out);
  g_free (data->partial_err);

  g_slice_free (TaskData, data);
}

static void
task_complete (GTask *task)
{
  OGMJobSpawn *spawn = g_task_get_source_object (task);
  TaskData *data = g_task_get_task_data (task);

  if (!spawn->priv->pid && !data->src_out && !data->src_err)
  {
    if (!data->error)
      g_cancellable_set_error_if_cancelled (data->cancellable, &data->error);

    if (data->error)
    {
      g_task_return_error (task, data->error);
      data->error = NULL;
    }
    else
      g_task_return_boolean (task, WIFEXITED (spawn->priv->status));
  }

  g_object_unref (task);
}

static void
task_pid_watch (GPid pid, gint status, GTask *task)
{
  OGMJobSpawn *spawn = g_task_get_source_object (task);

  g_spawn_close_pid (spawn->priv->pid);
  spawn->priv->pid = 0;

  spawn->priv->status = status;
}

static void
task_stdout_notify (GTask *task)
{
  OGMJobSpawn *spawn = g_task_get_source_object (task);
  TaskData *data = g_task_get_task_data (task);

  if (spawn->priv->notify_out)
    (* spawn->priv->notify_out) (spawn->priv->data_out);

  data->src_out = 0;

  task_complete (task);
}

static void
task_stderr_notify (GTask *task)
{
  OGMJobSpawn *spawn = g_task_get_source_object (task);
  TaskData *data = g_task_get_task_data (task);

  if (spawn->priv->notify_err)
    (* spawn->priv->notify_err) (spawn->priv->data_err);

  data->src_err = 0;

  task_complete (task);
}

static gboolean
task_watch (GTask *task, GIOChannel *channel, gchar **partial, OGMJobWatch watch_func, gpointer watch_data, GError **error)
{
  OGMJobSpawn *spawn = g_task_get_source_object (task);
  GIOStatus status;

  gboolean is_partial, retval = TRUE;
  gchar *buffer, **strv, *str;
  gsize size, len;
  guint i;

  if (*error)
    return FALSE;

  size = g_io_channel_get_buffer_size (channel);
  buffer = g_new0 (char, size + 1);

  status = g_io_channel_read_chars (channel, buffer, size, &len, error);
  if (status != G_IO_STATUS_NORMAL)
  {
    g_free (buffer);
    return status == G_IO_STATUS_AGAIN;
  }

  if (!watch_func)
  {
    g_free (buffer);
    return TRUE;
  }

  is_partial = buffer[len - 1] != '\n' && buffer[len - 1] != '\r';

  strv = g_strsplit_set (buffer, "\n\r", -1);
  g_free (buffer);

  for (i = 0; retval && strv && strv[i]; i ++)
  {
    if (is_partial && !strv[i + 1] && strv[i][0] != '\0')
      *partial = g_strdup (strv[i]);
    else
    {
      if (!(*partial) && strv[i][0] != '\0')
      {
        ogmrip_log_printf ("%s\n", strv[i]);
        retval = watch_func (spawn, strv[i], watch_data, error);
      }
      else if (*partial)
      {
        str = g_strconcat (*partial, strv[i], NULL);
        ogmrip_log_printf ("%s\n", str);
        retval = watch_func (spawn, str, watch_data, error);
        g_free (str);

        g_free (*partial);
        *partial = NULL;
      }

      if (!retval)
        ogmjob_spawn_kill (spawn);
    }
  }
  g_strfreev (strv);

  return retval;
}

static gboolean
task_watch_stdout (GIOChannel *channel, GIOCondition condition, GTask *task)
{
  OGMJobSpawn *spawn = g_task_get_source_object (task);
  TaskData *data = g_task_get_task_data (task);

  return task_watch (task, channel, &data->partial_out, spawn->priv->watch_out, spawn->priv->data_out, &data->error);
}

static gboolean
task_watch_stderr (GIOChannel *channel, GIOCondition condition, GTask *task)
{
  OGMJobSpawn *spawn = g_task_get_source_object (task);
  TaskData *data = g_task_get_task_data (task);

  return task_watch (task, channel, &data->partial_err, spawn->priv->watch_err, spawn->priv->data_err, &data->error);
}

static guint
create_pipe_watch (gint fd, GIOFunc func, gpointer user_data, GDestroyNotify notify)
{
  GIOChannel *channel;
  GSource *source;
  guint id;

  fcntl (fd, F_SETFL, O_NONBLOCK);
  channel = g_io_channel_unix_new (fd);
  g_io_channel_set_close_on_unref (channel, TRUE);
  g_io_channel_set_encoding (channel, NULL, NULL);

  source = g_io_create_watch (channel, G_IO_IN | G_IO_HUP | G_IO_ERR);
  g_io_channel_unref (channel);

  g_source_set_priority (source, G_PRIORITY_DEFAULT_IDLE);
  g_source_set_callback (source, (GSourceFunc) func, user_data, notify);
  id = g_source_attach (source, NULL);
  g_source_unref (source);

  return id;
}

static guint
create_child_watch (GPid pid, GChildWatchFunc func, gpointer user_data, GDestroyNotify notify)
{
  GSource *source;
  guint id;

  source = g_child_watch_source_new (pid);
  g_source_set_priority (source, G_PRIORITY_DEFAULT_IDLE);
  g_source_set_callback (source, (GSourceFunc) func, user_data, notify);
  id = g_source_attach (source, NULL);
  g_source_unref (source);

  return id;
}

static void
ogmjob_spawn_ready_cb (OGMJobSpawn *spawn, GAsyncResult *res, GAsyncResult **result)
{
  *result = g_object_ref (res);
}

static void
ogmjob_spawn_cancel_cb (GCancellable *cancellable, OGMJobSpawn *spawn)
{
  ogmjob_spawn_kill (spawn);
}

static void
ogmjob_spawn_run_internal (OGMJobSpawn *spawn, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  GTask *task;
  TaskData *data;
  gint fdout, fderr;
  guint i;

  g_return_if_fail (spawn->priv->pid == 0);

  data = g_slice_new0 (TaskData);

  task = g_task_new (spawn, cancellable, callback, user_data);
  g_task_set_task_data (task, data, (GDestroyNotify) free_data);

  if (cancellable)
  {
    data->cancellable = g_object_ref (cancellable);
    data->handler = g_cancellable_connect (cancellable,
        G_CALLBACK (ogmjob_spawn_cancel_cb), spawn, NULL);
  }

  for (i = 0; spawn->priv->argv[i]; i++)
    ogmrip_log_printf ("%s ", spawn->priv->argv[i]);
  ogmrip_log_write ("\n");

  if (!g_spawn_async_with_pipes (NULL, spawn->priv->argv, NULL,
        G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, 
        &spawn->priv->pid, NULL, &fdout, &fderr, &data->error))
  {
    g_task_return_error (task, data->error);
    task_complete (task);
  }
  else
  {
    data->src_pid = create_child_watch (spawn->priv->pid,
        (GChildWatchFunc) task_pid_watch, g_object_ref (task), (GDestroyNotify) task_complete);

    data->src_out = create_pipe_watch (fdout,
        (GIOFunc) task_watch_stdout, g_object_ref (task), (GDestroyNotify) task_stdout_notify);
    data->src_err = create_pipe_watch (fderr,
        (GIOFunc) task_watch_stderr, g_object_ref (task), (GDestroyNotify) task_stderr_notify);
  }

  g_object_unref (task);
}

G_DEFINE_TYPE_WITH_PRIVATE (OGMJobSpawn, ogmjob_spawn, OGMJOB_TYPE_TASK);

static void
ogmjob_spawn_run_async (OGMJobTask *task, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  ogmjob_spawn_run_internal (OGMJOB_SPAWN (task), cancellable, callback, user_data);
}

static gboolean
ogmjob_spawn_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  GAsyncResult *result = NULL;
  gboolean retval;

  ogmjob_spawn_run_internal (OGMJOB_SPAWN (task), cancellable, (GAsyncReadyCallback) ogmjob_spawn_ready_cb, &result);

  while (!result)
    g_main_context_iteration (NULL, TRUE);

  retval = ogmjob_task_run_finish (task, result, error);

  g_object_unref (result);

  return retval;
}

static void
ogmjob_spawn_suspend (OGMJobTask *task)
{
  OGMJobSpawn *spawn = OGMJOB_SPAWN (task);

  if (spawn->priv->pid > 0)
    kill (spawn->priv->pid, SIGSTOP);
}

static void
ogmjob_spawn_resume (OGMJobTask *task)
{
  OGMJobSpawn *spawn = OGMJOB_SPAWN (task);

  if (spawn->priv->pid > 0)
    kill (spawn->priv->pid, SIGCONT);
}

static void
ogmjob_spawn_finalize (GObject *gobject)
{
  OGMJobSpawn *spawn = OGMJOB_SPAWN (gobject);

  if (spawn->priv->argv)
    g_strfreev (spawn->priv->argv);

  G_OBJECT_CLASS (ogmjob_spawn_parent_class)->finalize (gobject);
}

static void
ogmjob_spawn_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
  {
    case PROP_ARGV:
      g_value_set_boxed (value, OGMJOB_SPAWN (gobject)->priv->argv);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmjob_spawn_set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
  {
    case PROP_ARGV:
      OGMJOB_SPAWN (gobject)->priv->argv = g_value_dup_boxed (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
ogmjob_spawn_class_init (OGMJobSpawnClass *klass)
{
  GObjectClass *gobject_class;
  OGMJobTaskClass *task_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = ogmjob_spawn_finalize;
  gobject_class->get_property = ogmjob_spawn_get_property;
  gobject_class->set_property = ogmjob_spawn_set_property;

  task_class = OGMJOB_TASK_CLASS (klass);
  task_class->run = ogmjob_spawn_run;
  task_class->run_async = ogmjob_spawn_run_async;
  task_class->suspend = ogmjob_spawn_suspend;
  task_class->resume = ogmjob_spawn_resume;

  g_object_class_install_property (gobject_class, PROP_ARGV,
      g_param_spec_boxed ("argv", "argv", "argv", G_TYPE_STRV,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
}

static void
ogmjob_spawn_init (OGMJobSpawn *spawn)
{
  spawn->priv = ogmjob_spawn_get_instance_private (spawn);
}

OGMJobTask *
ogmjob_spawn_new (const gchar *command_line)
{
  OGMJobTask *spawn;
  gchar **argv;
  gint argc;

  if (!g_shell_parse_argv (command_line, &argc, &argv, NULL))
    return NULL;

  spawn = g_object_new (OGMJOB_TYPE_SPAWN, "argv", argv, NULL);
  g_strfreev (argv);

  return spawn;
}

OGMJobTask *
ogmjob_spawn_newv (gchar **argv)
{
  return g_object_new (OGMJOB_TYPE_SPAWN, "argv", argv, NULL);
}

void
ogmjob_spawn_kill (OGMJobSpawn *spawn)
{
  g_return_if_fail (OGMJOB_IS_SPAWN (spawn));

  if (spawn->priv->pid > 0)
  {
    kill (spawn->priv->pid, SIGCONT);
    kill (spawn->priv->pid, SIGINT);
  }
}

void
ogmjob_spawn_set_watch (OGMJobSpawn *spawn, OGMJobStream stream,
    OGMJobWatch func, gpointer data, GDestroyNotify notify)
{
  g_return_if_fail (OGMJOB_IS_SPAWN (spawn));
  g_return_val_if_fail (spawn->priv->pid == 0, 0);

  switch (stream)
  {
    case OGMJOB_STREAM_OUTPUT:
      spawn->priv->watch_out = func;
      spawn->priv->data_out = data;
      spawn->priv->notify_out = notify;
      break;
    case OGMJOB_STREAM_ERROR:
      spawn->priv->watch_err = func;
      spawn->priv->data_err = data;
      spawn->priv->notify_err = notify;
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

/*
 * Check if the subprocess terminated in response to a signal
 * WIFSIGNALED (spawn->priv->status)
 *
 * Get the signal number that caused the subprocess to terminate
 * WTERMSIG (spawn->priv->status)
 */

gint
ogmjob_spawn_get_status (OGMJobSpawn *spawn)
{
  g_return_val_if_fail (OGMJOB_IS_SPAWN (spawn), 0);
  g_return_val_if_fail (spawn->priv->pid == 0, 0);

#ifdef G_OS_UNIX
  g_return_val_if_fail (WIFEXITED (spawn->priv->status), 0);

  return WEXITSTATUS (spawn->priv->status);
#else
  return spawn->priv->status;
#endif
}

