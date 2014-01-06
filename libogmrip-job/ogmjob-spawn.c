/* OGMJob - A library to spawn processes
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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
  gboolean result;
  gchar **argv;

  OGMJobWatch watch_out;
  gpointer data_out;

  OGMJobWatch watch_err;
  gpointer data_err;
};

enum
{
  PROP_0,
  PROP_ARGV
};

typedef struct
{
  OGMJobSpawn *spawn;
  GTask *task;
  GError *error;

  GCancellable *cancellable;
  gulong handler;

  gchar *partial_out;
  gchar *partial_err;

  guint src_out;
  guint src_err;
  guint src_pid;
} TaskAsyncData;

typedef struct
{
  gboolean complete;
  GError *error;
  gboolean retval;
} TaskSyncData;

static void
task_kill (TaskAsyncData *data)
{
  if (data->spawn->priv->pid > 0)
  {
    kill (data->spawn->priv->pid, SIGCONT);
    kill (data->spawn->priv->pid, SIGINT);
  }
}

static void
task_cancel_cb (GCancellable *cancellable, TaskAsyncData *data)
{
  task_kill (data);
}

static TaskAsyncData *
new_task (OGMJobTask *spawn, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  TaskAsyncData *data;

  data = g_new0 (TaskAsyncData, 1);

  /* No need to take a reference because g_task_new does */
  data->spawn = OGMJOB_SPAWN (spawn);

  data->task = g_task_new (spawn, cancellable, callback, user_data);

  if (cancellable)
  {
    data->cancellable = g_object_ref (cancellable);
    data->handler = g_cancellable_connect (cancellable,
        G_CALLBACK (task_cancel_cb), data, NULL);
  }

  return data;
}

static void
complete_task (TaskAsyncData *data)
{
  if (!data->spawn->priv->pid && !data->src_out && !data->src_err)
  {
    if (!data->error)
      g_cancellable_set_error_if_cancelled (data->cancellable, &data->error);

    if (data->error)
    {
      g_task_return_error (data->task, data->error);
      data->error = NULL;
    }
    else
      g_task_return_boolean (data->task, WIFEXITED (data->spawn->priv->status) && WEXITSTATUS (data->spawn->priv->status) == 0);

    if (data->cancellable)
    {
      g_cancellable_disconnect (data->cancellable, data->handler);
      g_object_unref (data->cancellable);
    }

    if (data->error)
      g_error_free (data->error);

    g_free (data->partial_out);
    g_free (data->partial_err);

    g_object_unref (data->task);

    g_free (data);
  }
}

static void
task_pid_watch (GPid pid, gint status, TaskAsyncData *data)
{
  OGMJobSpawn *spawn = data->spawn;

  g_spawn_close_pid (spawn->priv->pid);
  spawn->priv->pid = 0;
/*
  data->spawn->priv->status = -1;
  if (WIFEXITED (status))
    data->spawn->priv->status = WEXITSTATUS (status);
*/
  data->spawn->priv->status = status;
}

static void
task_stdout_notify (TaskAsyncData *data)
{
  data->src_out = 0;
  complete_task (data);
}

static void
task_stderr_notify (TaskAsyncData *data)
{
  data->src_err = 0;
  complete_task (data);
}

static gboolean
task_watch (GIOChannel *channel, TaskAsyncData *data, gchar **partial, OGMJobWatch watch_func, gpointer watch_data)
{
  GIOStatus status;

  gboolean is_partial, retval = TRUE;
  gchar *buffer, **strv, *str;
  gsize size, len;
  guint i;

  if (data->error)
    return FALSE;

  size = g_io_channel_get_buffer_size (channel);
  buffer = g_new0 (char, size + 1);

  status = g_io_channel_read_chars (channel, buffer, size, &len, &data->error);
  if (status != G_IO_STATUS_NORMAL)
  {
    g_free (buffer);
    return status == G_IO_STATUS_AGAIN;
  }

  ogmrip_log_write (buffer);
/*
#ifdef G_ENABLE_DEBUG
  g_print ("%s", buffer);
#endif
*/
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
        retval = watch_func (data->spawn, strv[i], watch_data, &data->error);
      else if (*partial)
      {
        str = g_strconcat (*partial, strv[i], NULL);
        retval = watch_func (data->spawn, str, watch_data, &data->error);
        g_free (str);

        g_free (*partial);
        *partial = NULL;
      }

      if (!retval)
        task_kill (data);
    }
  }
  g_strfreev (strv);

  return retval;
}

static gboolean
task_watch_stdout (GIOChannel *channel, GIOCondition condition, TaskAsyncData *data)
{
  return task_watch (channel, data, &data->partial_out, data->spawn->priv->watch_out, data->spawn->priv->data_out);
}

static gboolean
task_watch_stderr (GIOChannel *channel, GIOCondition condition, TaskAsyncData *data)
{
  return task_watch (channel, data, &data->partial_err, data->spawn->priv->watch_err, data->spawn->priv->data_err);
}

static void
task_ready_cb (OGMJobTask *task, GAsyncResult *res, TaskSyncData *data)
{
  data->retval = ogmjob_task_run_finish (task, res, &data->error);

  data->complete = TRUE;
}

G_DEFINE_TYPE (OGMJobSpawn, ogmjob_spawn, OGMJOB_TYPE_TASK);

static void
ogmjob_spawn_run_async (OGMJobTask *task, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  TaskAsyncData *data;
  gint fdout, fderr;
  guint i;

  data = new_task (task, cancellable, callback, user_data);

  for (i = 0; data->spawn->priv->argv[i]; i++)
    ogmrip_log_printf ("%s ", data->spawn->priv->argv[i]);
  ogmrip_log_write ("\n");

  if (!g_spawn_async_with_pipes (NULL, data->spawn->priv->argv, NULL,
        G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, 
        &data->spawn->priv->pid, NULL, &fdout, &fderr, &data->error))
  {
    g_task_return_error (data->task, data->error);
    complete_task (data);
  }
  else
  {
    GIOChannel *channel;

    data->src_pid = g_child_watch_add_full (G_PRIORITY_DEFAULT_IDLE, data->spawn->priv->pid, 
        (GChildWatchFunc) task_pid_watch, data, (GDestroyNotify) complete_task);

    fcntl (fdout, F_SETFL, O_NONBLOCK);
    channel = g_io_channel_unix_new (fdout);
    g_io_channel_set_close_on_unref (channel, TRUE);
    g_io_channel_set_encoding (channel, NULL, NULL);
    data->src_out = g_io_add_watch_full (channel, G_PRIORITY_DEFAULT_IDLE,
        G_IO_IN | G_IO_HUP | G_IO_ERR, (GIOFunc) task_watch_stdout, data, 
        (GDestroyNotify) task_stdout_notify);
    g_io_channel_unref (channel);

    fcntl (fderr, F_SETFL, O_NONBLOCK);
    channel = g_io_channel_unix_new (fderr);
    g_io_channel_set_close_on_unref (channel, TRUE);
    g_io_channel_set_encoding (channel, NULL, NULL);
    data->src_err = g_io_add_watch_full (channel, G_PRIORITY_DEFAULT_IDLE,
        G_IO_IN | G_IO_HUP | G_IO_ERR, (GIOFunc) task_watch_stderr, data, 
        (GDestroyNotify) task_stderr_notify);
    g_io_channel_unref (channel);
  }
}

static gboolean
ogmjob_spawn_run (OGMJobTask *task, GCancellable *cancellable, GError **error)
{
  TaskSyncData data = { FALSE, NULL, FALSE };

  g_object_ref (task);

  ogmjob_spawn_run_async (task, cancellable, (GAsyncReadyCallback) task_ready_cb, &data);

  while (!data.complete)
    g_main_context_iteration (NULL, TRUE);

  if (data.error)
    g_propagate_error (error, data.error);

  g_object_unref (task);

  return data.retval;
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

  g_type_class_add_private (klass, sizeof (OGMJobSpawnPriv));
}

static void
ogmjob_spawn_init (OGMJobSpawn *spawn)
{
  spawn->priv = G_TYPE_INSTANCE_GET_PRIVATE (spawn, OGMJOB_TYPE_SPAWN, OGMJobSpawnPriv);
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
ogmjob_spawn_set_watch_stdout (OGMJobSpawn *spawn, OGMJobWatch watch_func, gpointer watch_data)
{
  g_return_if_fail (OGMJOB_IS_SPAWN (spawn));

  spawn->priv->watch_out = watch_func;
  spawn->priv->data_out = watch_data;
}

void
ogmjob_spawn_set_watch_stderr (OGMJobSpawn *spawn, OGMJobWatch watch_func, gpointer watch_data)
{
  g_return_if_fail (OGMJOB_IS_SPAWN (spawn));

  spawn->priv->watch_err = watch_func;
  spawn->priv->data_err = watch_data;
}

gint
ogmjob_spawn_get_status (OGMJobSpawn *spawn)
{
  g_return_val_if_fail (OGMJOB_IS_SPAWN (spawn), 0);

  return spawn->priv->status;
}

