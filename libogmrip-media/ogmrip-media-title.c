/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-media-title.h"
#include "ogmrip-media-object.h"

G_DEFINE_INTERFACE (OGMRipTitle, ogmrip_title, G_TYPE_OBJECT)

static void
ogmrip_title_default_init (OGMRipTitleInterface *iface)
{
}

gboolean 
ogmrip_title_open (OGMRipTitle *title, GError **error)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->open)
    return TRUE;

  return iface->open (title, error);
}

void 
ogmrip_title_close (OGMRipTitle *title)
{
  OGMRipTitleInterface *iface;

  g_return_if_fail (OGMRIP_IS_TITLE (title));

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (iface->close)
    iface->close (title);
}

gboolean 
ogmrip_title_is_open (OGMRipTitle *title)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), FALSE);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->is_open)
    return TRUE;

  return iface->is_open (title);
}

OGMRipMedia * 
ogmrip_title_get_media (OGMRipTitle *title)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), NULL);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_media)
    return NULL;

  return iface->get_media (title);
}

gint 
ogmrip_title_get_nr (OGMRipTitle *title)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), -1);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_nr)
    return 0;

  return iface->get_nr (title);
}

gint 
ogmrip_title_get_ts_nr (OGMRipTitle *title)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), -1);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_ts_nr)
    return ogmrip_title_get_nr (title);

  return iface->get_ts_nr (title);
}

gboolean
ogmrip_title_get_progressive (OGMRipTitle *title)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), -1);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_progressive)
    return FALSE;

  return iface->get_progressive (title);
}

gboolean
ogmrip_title_get_telecine (OGMRipTitle *title)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), -1);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_telecine)
    return FALSE;

  return iface->get_telecine (title);
}

gboolean
ogmrip_title_get_interlaced (OGMRipTitle *title)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), -1);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_interlaced)
    return FALSE;

  return iface->get_interlaced (title);
}

gint64
ogmrip_title_get_size (OGMRipTitle *title)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), -1);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_size)
    return 0;

  return iface->get_size (title);
}

gdouble 
ogmrip_title_get_length (OGMRipTitle *title, OGMRipTime *length)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), -1.0);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_length)
    return 0.0;

  return iface->get_length (title, length);
}

gdouble 
ogmrip_title_get_chapters_length (OGMRipTitle *title, guint start, gint end, OGMRipTime *length)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), -1.0);
  g_return_val_if_fail ((start <= end && end < ogmrip_title_get_n_chapters (title)) || end < 0, -1.0);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_chapters_length)
    return 0.0;

  return iface->get_chapters_length (title, start, end, length);
}

const guint * 
ogmrip_title_get_palette (OGMRipTitle *title)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), NULL);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_palette)
    return NULL;

  return iface->get_palette (title);
}

gint 
ogmrip_title_get_n_angles (OGMRipTitle *title)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), -1);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_n_angles)
    return 1;

  return iface->get_n_angles (title);
}

gint 
ogmrip_title_get_n_chapters (OGMRipTitle *title)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), -1);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_n_chapters)
    return 1;

  return iface->get_n_chapters (title);
}

OGMRipVideoStream *
ogmrip_title_get_video_stream (OGMRipTitle *title)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), NULL);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_video_stream)
    return NULL;

  return iface->get_video_stream (title);
}

gint 
ogmrip_title_get_n_audio_streams (OGMRipTitle *title)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), -1);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_n_audio_streams)
    return 0;

  return iface->get_n_audio_streams (title);
}

OGMRipAudioStream * 
ogmrip_title_get_nth_audio_stream (OGMRipTitle *title, guint nr)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), NULL);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_nth_audio_stream)
    return NULL;

  return iface->get_nth_audio_stream (title, nr);
}

GList * 
ogmrip_title_get_audio_streams (OGMRipTitle *title)
{
  GList *list = NULL;
  OGMRipAudioStream *stream;
  gint i, n;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), NULL);

  n = ogmrip_title_get_n_audio_streams (title);
  for (i = 0; i < n; i ++)
  {
    stream = ogmrip_title_get_nth_audio_stream (title, i);
    list = g_list_prepend (list, stream);
  }

  return g_list_reverse (list);
}

gint 
ogmrip_title_get_n_subp_streams (OGMRipTitle *title)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), -1);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_n_subp_streams)
    return 0;

  return iface->get_n_subp_streams (title);
}

OGMRipSubpStream * 
ogmrip_title_get_nth_subp_stream (OGMRipTitle *title, guint nr)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), NULL);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->get_nth_subp_stream)
    return NULL;

  return iface->get_nth_subp_stream (title, nr);
}

GList * 
ogmrip_title_get_subp_streams (OGMRipTitle *title)
{
  GList *list = NULL;
  OGMRipSubpStream *stream;
  gint i, n;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), NULL);

  n = ogmrip_title_get_n_subp_streams (title);
  for (i = 0; i < n; i ++)
  {
    stream = ogmrip_title_get_nth_subp_stream (title, i);
    list = g_list_prepend (list, stream);
  }

  return g_list_reverse (list);
}

gboolean
ogmrip_title_analyze (OGMRipTitle *title, GCancellable *cancellable,
    OGMRipTitleCallback callback, gpointer user_data, GError **error)
{
  OGMRipTitleInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_TITLE (title), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  iface = OGMRIP_TITLE_GET_IFACE (title);

  if (!iface->analyze)
    return TRUE;

  return iface->analyze (title, cancellable, callback, user_data, error);
}
