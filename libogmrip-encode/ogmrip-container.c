/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
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
 * SECTION:ogmrip-container
 * @title: OGMRipContainer
 * @short_description: Base class for containers
 * @include: ogmrip-container.h
 */

#include "ogmrip-container.h"

#include <unistd.h>
#include <glib/gstdio.h>

#define DEFAULT_OVERHEAD 6

#define OGMRIP_CONTAINER_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_CONTAINER, OGMRipContainerPriv))

struct _OGMRipContainerPriv
{
  gchar *label;
  gchar *fourcc;

  guint tsize;
  guint tnumber;

  GFile *output;
  GList *files;
};

enum 
{
  PROP_0,
  PROP_OUTPUT,
  PROP_LABEL,
  PROP_FOURCC,
  PROP_TSIZE,
  PROP_TNUMBER,
  PROP_OVERHEAD
};

static void ogmrip_container_dispose      (GObject      *gobject);
static void ogmrip_container_finalize     (GObject      *gobject);
static void ogmrip_container_set_property (GObject      *gobject,
                                           guint        property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec);
static void ogmrip_container_get_property (GObject      *gobject,
                                           guint        property_id,
                                           GValue       *value,
                                           GParamSpec   *pspec);

G_DEFINE_ABSTRACT_TYPE (OGMRipContainer, ogmrip_container, OGMJOB_TYPE_BIN)

static void
ogmrip_container_class_init (OGMRipContainerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ogmrip_container_dispose;
  gobject_class->finalize = ogmrip_container_finalize;
  gobject_class->set_property = ogmrip_container_set_property;
  gobject_class->get_property = ogmrip_container_get_property;

  g_object_class_install_property (gobject_class, PROP_OUTPUT, 
        g_param_spec_object ("output", "Output property", "Set output file", 
           G_TYPE_FILE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LABEL, 
        g_param_spec_string ("label", "Label property", "Set label", 
           NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FOURCC, 
        g_param_spec_string ("fourcc", "FourCC property", "Set fourcc", 
           NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TSIZE, 
        g_param_spec_uint ("target-size", "Target size property", "Set target size", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TNUMBER, 
        g_param_spec_uint ("target-number", "Target number property", "Set target number", 
           0, G_MAXUINT, 1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_OVERHEAD, 
        g_param_spec_uint ("overhead", "Overhead property", "Get overhead", 
           0, G_MAXUINT, DEFAULT_OVERHEAD, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (OGMRipContainerPriv));
}

static void
ogmrip_container_init (OGMRipContainer *container)
{
  container->priv = OGMRIP_CONTAINER_GET_PRIVATE (container);
  container->priv->tnumber = 1;
  container->priv->tsize = 0;
}

static void
ogmrip_container_dispose (GObject *gobject)
{
  OGMRipContainer *container = OGMRIP_CONTAINER (gobject);

  if (container->priv->output)
  {
    g_object_unref (container->priv->output);
    container->priv->output = NULL;
  }

  if (container->priv->files)
  {
    g_list_foreach (container->priv->files, (GFunc) g_object_unref, NULL);
    g_list_free (container->priv->files);
    container->priv->files = NULL;
  }

  G_OBJECT_CLASS (ogmrip_container_parent_class)->dispose (gobject);
}

static void
ogmrip_container_finalize (GObject *gobject)
{
  OGMRipContainer *container = OGMRIP_CONTAINER (gobject);

  g_free (container->priv->label);
  g_free (container->priv->output);
  g_free (container->priv->fourcc);

  G_OBJECT_CLASS (ogmrip_container_parent_class)->finalize (gobject);
}

static void
ogmrip_container_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipContainer *container = OGMRIP_CONTAINER (gobject);

  switch (property_id) 
  {
    case PROP_OUTPUT:
      ogmrip_container_set_output (container, g_value_get_object (value));
      break;
    case PROP_LABEL: 
      ogmrip_container_set_label (container, g_value_get_string (value));
      break;
    case PROP_FOURCC: 
      ogmrip_container_set_fourcc (container, g_value_get_string (value));
      break;
    case PROP_TSIZE: 
      container->priv->tsize = g_value_get_uint (value);
      break;
    case PROP_TNUMBER: 
      container->priv->tnumber = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_container_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipContainer *container = OGMRIP_CONTAINER (gobject);

  switch (property_id) 
  {
    case PROP_OUTPUT:
      g_value_set_object (value, container->priv->output);
      break;
    case PROP_LABEL:
      g_value_set_string (value, container->priv->label);
      break;
    case PROP_FOURCC:
      g_value_set_string (value, container->priv->fourcc);
      break;
    case PROP_TSIZE:
      g_value_set_uint (value, container->priv->tsize);
      break;
    case PROP_TNUMBER:
      g_value_set_uint (value, container->priv->tnumber);
      break;
    case PROP_OVERHEAD:
      g_value_set_uint (value, ogmrip_container_get_overhead (container));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

/**
 * ogmrip_container_get_output:
 * @container: An #OGMRipContainer
 *
 * Gets the output file.
 *
 * Returns: A #GFile, or NULL
 */
GFile *
ogmrip_container_get_output (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  return container->priv->output;
}

/**
 * ogmrip_container_set_output:
 * @container: an #OGMRipContainer
 * @output: a #GFile
 *
 * Sets the output file.
 */
void
ogmrip_container_set_output (OGMRipContainer *container, GFile *output)
{
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (G_IS_FILE (output));

  g_object_ref (output);
  if (container->priv->output)
    g_object_unref (container->priv->output);
  container->priv->output = output;

  g_object_notify (G_OBJECT (container), "output");
}

/**
 * ogmrip_container_get_label:
 * @container: An #OGMRipContainer
 *
 * Gets the label of the rip.
 *
 * Returns: The label, or NULL
 */
const gchar *
ogmrip_container_get_label (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  return container->priv->label;
}

/**
 * ogmrip_container_set_label:
 * @container: An #OGMRipContainer
 * @label: the label
 *
 * Sets the label of the rip.
 */
void
ogmrip_container_set_label (OGMRipContainer *container, const gchar *label)
{
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));

  g_free (container->priv->label);
  container->priv->label = label ? g_strdup (label) : NULL;

  g_object_notify (G_OBJECT (container), "label");
}

/**
 * ogmrip_container_get_fourcc:
 * @container: An #OGMRipContainer
 *
 * Gets the FourCC of the rip.
 *
 * Returns: The FourCC, or NULL
 */
const gchar *
ogmrip_container_get_fourcc (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  return container->priv->fourcc;
}

/**
 * ogmrip_container_set_fourcc:
 * @container: An #OGMRipContainer
 * @fourcc: the FourCC
 *
 * Sets the FourCC of the rip.
 */
void
ogmrip_container_set_fourcc (OGMRipContainer *container, const gchar *fourcc)
{
  gchar *str;

  g_return_if_fail (OGMRIP_IS_CONTAINER (container));

  if (container->priv->fourcc)
    g_free (container->priv->fourcc);
  container->priv->fourcc = NULL;

  if (fourcc)
  {
    str = g_utf8_strup (fourcc, -1);
    container->priv->fourcc = g_strndup (str, 4);
    g_free (str);
  }

  g_object_notify (G_OBJECT (container), "fourcc");
}

/**
 * ogmrip_container_get_overhead:
 * @container: An #OGMRipContainer
 *
 * Gets the overhead of the container.
 *
 * Returns: The overhead, or -1
 */
gint
ogmrip_container_get_overhead (OGMRipContainer *container)
{
  OGMRipContainerClass *klass;

  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), -1);

  klass = OGMRIP_CONTAINER_GET_CLASS (container);
  if (klass->get_overhead)
    return klass->get_overhead (container);

  return DEFAULT_OVERHEAD;
}

/**
 * ogmrip_container_add_file:
 * @container: An #OGMRipContainer
 * @file: An #OGMRipFile
 *
 * Adds @file to the rip.
 */
void
ogmrip_container_add_file (OGMRipContainer *container, OGMRipFile *file)
{
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (OGMRIP_IS_FILE (file));

  container->priv->files = g_list_append (container->priv->files, g_object_ref (file));
}

/**
 * ogmrip_container_remove_file:
 * @container: An #OGMRipContainer
 * @file: An #OGMRipFile
 *
 * Removes @file from the rip.
 */
void
ogmrip_container_remove_file (OGMRipContainer *container, OGMRipFile *file)
{
  GList *link;

  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (OGMRIP_IS_FILE (file));

  for (link = container->priv->files; link; link = link->next)
    if (link->data == file)
      break;

  if (link)
  {
    container->priv->files = g_list_remove_link (container->priv->files, link);
    g_object_unref (link->data);
    g_list_free (link);
  }
}

/**
 * ogmrip_container_get_files:
 * @container: An #OGMRipContainer
 *
 * Gets a list of the files of the rip.
 *
 * Returns: A #GList, or NULL
 */
GList *
ogmrip_container_get_files (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  return g_list_copy (container->priv->files);
}

/**
 * ogmrip_container_get_nth_file:
 * @container: an #OGMRipContainer
 * @n: The index of the file
 *
 * Gets the file at the given position.
 *
 * Returns: An #OGMRipFile, or NULL
 */
OGMRipFile *
ogmrip_container_get_nth_file (OGMRipContainer *container, gint n)
{
  GList *file;

  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  if (n < 0)
    file = g_list_last (container->priv->files);
  else
    file = g_list_nth (container->priv->files, n);

  return file->data;
}

/**
 * ogmrip_container_get_n_files:
 * @container: an #OGMRipContainer
 *
 * Gets the number of files.
 *
 * Returns: the number of files
 */
gint
ogmrip_container_get_n_files (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), -1);

  return g_list_length (container->priv->files);
}

/**
 * ogmrip_container_foreach_file:
 * @container: An #OGMRipContainer
 * @func: The function to call with each file
 * @data: User data to pass to the function
 *
 * Calls a function for each file
 */
void
ogmrip_container_foreach_file (OGMRipContainer *container, OGMRipContainerFunc func, gpointer data)
{
  GList *link, *next;

  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (func != NULL);
  
  link = container->priv->files;
  while (link)
  {
    next = link->next;
    (* func) (container, link->data, data);
    link = next;
  }
}

/**
 * ogmrip_container_set_split:
 * @container: An #OGMRipContainer
 * @number:  The number of file
 * @size: The size of each file
 *
 * Sets the number of output files and the maximum size of each one.
 */
void
ogmrip_container_set_split (OGMRipContainer *container, guint number, guint size)
{
   g_return_if_fail (OGMRIP_IS_CONTAINER (container));

   if (number > 0)
     container->priv->tnumber = number;

   if (size > 0)
     container->priv->tsize = size;
}

/**
 * ogmrip_container_get_split:
 * @container: An #OGMRipContainer
 * @number:  A pointer to store the number of file
 * @size: A pointer to store the size of each file
 *
 * Gets the number of output files and the maximum size of each one.
 */
void
ogmrip_container_get_split (OGMRipContainer *container, guint *number, guint *size)
{
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));

  if (number)
    *number = container->priv->tnumber;

  if (size)
    *size = container->priv->tsize;
}

/**
 * ogmrip_container_get_sync:
 * @container: an #OGMRipContainer
 *
 * Gets the delay of the audio streams in ms.
 *
 * Returns: The delay
 */
glong
ogmrip_container_get_sync (OGMRipContainer *container)
{
  GList *link;
  guint num, denom;
  gint start_delay;

  for (link = container->priv->files; link; link = link->next)
    if (OGMRIP_IS_VIDEO_STREAM (link->data))
      break;

  if (!link)
    return 0;

  ogmrip_video_stream_get_framerate (link->data, &num, &denom);
  start_delay = ogmrip_video_stream_get_start_delay (link->data);

  return (start_delay * denom * 1000) / (gdouble) num;
}

