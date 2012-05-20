/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-application.h"

#include <stdlib.h>

struct _OGMRipApplicationPriv
{
  gboolean prepared;
};

enum
{
  PROP_0,
  PROP_IS_PREPARED
};

#ifdef G_ENABLE_DEBUG
static gboolean debug = TRUE;
#else
static gboolean debug = FALSE;
#endif

G_DEFINE_TYPE (OGMRipApplication, ogmrip_application, G_TYPE_APPLICATION);

static void
ogmrip_application_get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
  {
    case PROP_IS_PREPARED:
      g_value_set_boolean (value, OGMRIP_APPLICATION (gobject)->priv->prepared);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static GOptionEntry opts[] =
{
  { "debug", 0,  0, G_OPTION_ARG_NONE, &debug, "Enable debug messages", NULL },
  { NULL,    0,  0, 0,                 NULL,   NULL,                    NULL }
};

static gboolean
ogmrip_application_local_cmdline (GApplication *application, gchar ***argv, gint *status)
{
  GError *error = NULL;
  GOptionContext *context;
  gboolean retval;
  gint argc;

  context = g_option_context_new ("[<PATH>]");
  g_option_context_add_main_entries (context, opts, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  argc = g_strv_length (*argv);
  retval = g_option_context_parse (context, &argc, argv, &error);
  g_option_context_free (context);

  if (!retval)
  {
    g_print ("option parsing failed: %s\n", error->message);
    g_error_free (error);
    *status = 1;

    return TRUE;
  }

  if (!g_application_register (application, NULL, &error))
  {
    g_critical ("%s", error->message);
    g_error_free (error);
    *status = 1;

    return TRUE;
  }

  if (argc <= 1)
    g_application_activate (application);
  else
  {
    GFile **files;
    gint i;

    files = g_new (GFile *, argc - 1);

    for (i = 0; i < argc - 1; i ++)
      files[i] = g_file_new_for_commandline_arg ((*argv)[i + 1]);

    g_application_open (application, files, argc - 1, "");

    for (i = 0; i < argc - 1; i ++)
      g_object_unref (files[i]);
    g_free (files);
  }

  if (debug)
    ogmrip_log_set_print_stdout (TRUE);

  *status = 0;

  return TRUE;
}

static void
ogmrip_application_class_init (OGMRipApplicationClass *klass)
{
  GObjectClass *gobject_class;
  GApplicationClass *application_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = ogmrip_application_get_property;

  application_class = G_APPLICATION_CLASS (klass);
  application_class->local_command_line = ogmrip_application_local_cmdline;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_IS_PREPARED,
      g_param_spec_boolean ("is-prepared", "is-prepared", "is-prepared",
        FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipApplicationPriv));
}

static void
ogmrip_application_init (OGMRipApplication *app)
{
  app->priv = G_TYPE_INSTANCE_GET_PRIVATE (app, OGMRIP_TYPE_APPLICATION, OGMRipApplicationPriv);
}

GApplication *
ogmrip_application_new (const gchar *app_id)
{
  g_return_val_if_fail (g_application_id_is_valid (app_id), NULL);

  g_type_init ();

  return g_object_new (OGMRIP_TYPE_APPLICATION,
      "application-id", app_id, "flags", G_APPLICATION_HANDLES_OPEN, NULL);
}

void
ogmrip_application_prepare (OGMRipApplication *app)
{
  g_return_if_fail (OGMRIP_IS_APPLICATION (app));

  if (!app->priv->prepared)
  {
    app->priv->prepared = TRUE;

    g_object_notify (G_OBJECT (app), "is-prepared");
  }
}

gboolean
ogmrip_application_is_prepared (OGMRipApplication *app)
{
  g_return_val_if_fail (OGMRIP_IS_APPLICATION (app), FALSE);

  return app->priv->prepared;
}

