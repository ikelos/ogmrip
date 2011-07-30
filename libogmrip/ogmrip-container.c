/* OGMRip - A library for DVD ripping and encoding
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
 * SECTION:ogmrip-container
 * @title: OGMRipContainer
 * @short_description: Base class for containers
 * @include: ogmrip-container.h
 */

#include "ogmrip-container.h"
#include "ogmrip-plugin.h"
#include "ogmjob-exec.h"

#include <unistd.h>
#include <glib/gstdio.h>

#define DEFAULT_OVERHEAD 6

#define OGMRIP_CONTAINER_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OGMRIP_TYPE_CONTAINER, OGMRipContainerPriv))

struct _OGMRipContainerPriv
{
  gchar *label;
  gchar *output;
  gchar *fourcc;

  guint tsize;
  guint tnumber;
  guint start_delay;

  GSList *files;
};

typedef struct
{
  gchar *filename;
  gchar *name;
  OGMRipFormatType format;
  guint lang;
} OGMRipContainerFile;

enum 
{
  PROP_0,
  PROP_OUTPUT,
  PROP_LABEL,
  PROP_FOURCC,
  PROP_TSIZE,
  PROP_TNUMBER,
  PROP_OVERHEAD,
  PROP_START_DELAY
};

static void ogmrip_container_finalize     (GObject      *gobject);
static void ogmrip_container_set_property (GObject      *gobject,
                                           guint        property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec);
static void ogmrip_container_get_property (GObject      *gobject,
                                           guint        property_id,
                                           GValue       *value,
                                           GParamSpec   *pspec);

static OGMRipContainerFile *
ogmrip_container_file_new (const gchar *filename, OGMRipFormatType format, const gchar *name, guint language)
{
  OGMRipContainerFile *file;

  file = g_new0 (OGMRipContainerFile, 1);

  file->filename = g_strdup (filename);
  file->format = format;
  file->lang = language;

  if (name)
    file->name = g_strdup (name);

  return file;
}

static void
ogmrip_container_file_free (OGMRipContainerFile *file)
{
  g_free (file->filename);
  g_free (file->name);
  g_free (file);
}

G_DEFINE_ABSTRACT_TYPE (OGMRipContainer, ogmrip_container, OGMJOB_TYPE_BIN)

static void
ogmrip_container_class_init (OGMRipContainerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = ogmrip_container_finalize;
  gobject_class->set_property = ogmrip_container_set_property;
  gobject_class->get_property = ogmrip_container_get_property;

  g_object_class_install_property (gobject_class, PROP_OUTPUT, 
        g_param_spec_string ("output", "Output property", "Set output file", 
           NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LABEL, 
        g_param_spec_string ("label", "Label property", "Set label", 
           NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FOURCC, 
        g_param_spec_string ("fourcc", "FourCC property", "Set fourcc", 
           NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TSIZE, 
        g_param_spec_uint ("target-size", "Target size property", "Set target size", 
           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TNUMBER, 
        g_param_spec_uint ("target-number", "Target number property", "Set target number", 
           0, G_MAXUINT, 1, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_OVERHEAD, 
        g_param_spec_uint ("overhead", "Overhead property", "Get overhead", 
           0, G_MAXUINT, 6, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_START_DELAY, 
        g_param_spec_uint ("start-delay", "Start delay property", "Set start delay", 
           0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

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
ogmrip_container_finalize (GObject *gobject)
{
  OGMRipContainer *container = OGMRIP_CONTAINER (gobject);

  g_slist_foreach (container->priv->files, (GFunc) ogmrip_container_file_free, NULL);
  g_slist_free (container->priv->files);

  g_free (container->priv->label);
  g_free (container->priv->output);
  g_free (container->priv->fourcc);

  G_OBJECT_CLASS (ogmrip_container_parent_class)->finalize (gobject);
}

static void
ogmrip_container_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipContainer *container;

  container = OGMRIP_CONTAINER (gobject);

  switch (property_id) 
  {
    case PROP_OUTPUT:
      ogmrip_container_set_output (container, g_value_get_string (value));
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
    case PROP_START_DELAY: 
      container->priv->start_delay = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_container_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipContainer *container;

  container = OGMRIP_CONTAINER (gobject);

  switch (property_id) 
  {
    case PROP_OUTPUT:
      g_value_set_string (value, container->priv->output);
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
      g_value_set_uint (value, 6);
      break;
    case PROP_START_DELAY:
      g_value_set_uint (value, container->priv->start_delay);
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
 * Gets the name of the output file.
 *
 * Returns: The filename, or NULL
 */
const gchar *
ogmrip_container_get_output (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  return container->priv->output;
}

/**
 * ogmrip_container_set_output:
 * @container: an #OGMRipContainer
 * @output: the name of the output file
 *
 * Sets the name of the output file.
 */
void
ogmrip_container_set_output (OGMRipContainer *container, const gchar *output)
{
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (output && *output);

  g_free (container->priv->output);
  container->priv->output = g_strdup (output);

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
 * ogmrip_container_get_start_delay:
 * @container: An #OGMRipContainer
 *
 * Gets the start delay of the audio tracks.
 *
 * Returns: The start delay, or -1
 */
gint
ogmrip_container_get_start_delay (OGMRipContainer *container)
{
  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), -1);

  return container->priv->start_delay;
}

/**
 * ogmrip_container_set_start_delay:
 * @container: An #OGMRipContainer
 * @start_delay: the start delay
 *
 * Sets the start delay of the audio tracks
 */
void
ogmrip_container_set_start_delay (OGMRipContainer *container, guint start_delay)
{
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));

  container->priv->start_delay = start_delay;

  g_object_notify (G_OBJECT (container), "start-delay");
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
  gint overhead;

  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), -1);

  g_object_get (container, "overhead", &overhead, NULL);

  return overhead;
}

/**
 * ogmrip_container_add_file:
 * @container: An #OGMRipContainer
 * @file: A filename
 *
 * Adds a file to the rip.
 */
void
ogmrip_container_add_file (OGMRipContainer *container,
    const gchar *filename, OGMRipFormatType format, const gchar *name, guint language)
{
  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (filename != NULL);

  container->priv->files = g_slist_append (container->priv->files,
      ogmrip_container_file_new (filename, format, name, language));
}

/**
 * ogmrip_container_remove_file:
 * @container: An #OGMRipContainer
 * @filename: A filename
 *
 * Removes the file from the rip.
 */
void
ogmrip_container_remove_file (OGMRipContainer *container, const gchar *filename)
{
  GSList *link;
  OGMRipContainerFile *file;

  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (filename != NULL);

  for (link = container->priv->files; link; link = link->next)
  {
    file = link->data;

    if (g_str_equal (file->filename, filename))
      break;
  }

  if (link)
  {
    container->priv->files = g_slist_remove_link (container->priv->files, link);
    ogmrip_container_file_free (link->data);
    g_slist_free (link);
  }
}

/**
 * ogmrip_container_get_files:
 * @container: An #OGMRipContainer
 *
 * Gets a list of the files of the rip.
 *
 * Returns: A #GSList, or NULL
 */
GSList *
ogmrip_container_get_files (OGMRipContainer *container)
{
  return g_slist_copy (container->priv->files);
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
  void *file;

  g_return_val_if_fail (OGMRIP_IS_CONTAINER (container), NULL);

  if (n < 0)
    file = g_slist_last (container->priv->files);
  else
    file = g_slist_nth (container->priv->files, n);

  return file;
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

  return g_slist_length (container->priv->files);
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
  GSList *link, *next;
  OGMRipContainerFile *file;

  g_return_if_fail (OGMRIP_IS_CONTAINER (container));
  g_return_if_fail (func != NULL);
  
  link = container->priv->files;
  while (link)
  {
    next = link->next;

    file = link->data;
    (* func) (container, file->filename, file->format, file->name, file->lang, data);

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

