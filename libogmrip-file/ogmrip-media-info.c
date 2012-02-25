/* OGMRipFile - A file library for OGMRip
 * Copyright (C) 2010-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-media-info.h"

#include <ogmrip-media.h>

#if defined(UNICODE) || defined (_UNICODE)
#include <string.h>
#include <wchar.h>
#endif

#define MEDIAINFO_GLIBC
#include <MediaInfoDLL/MediaInfoDLL.h>

struct _OGMRipMediaInfoPriv
{
  const gchar *filename;
  gboolean is_open;
  gpointer handle;
  gchar *value;
};

static OGMRipMediaInfo *default_media_info = NULL;

static void ogmrip_media_info_constructed  (GObject *gobject);
static void ogmrip_media_info_finalize     (GObject *gobject);

#if defined(UNICODE) || defined (_UNICODE)
static gchar *
g_locale_from_wstring (const wchar_t *wstr)
{
  mbstate_t ps;
  size_t len;
  gchar *str;

  memset (&ps, 0, sizeof (ps));
  len = wcsrtombs (NULL, &wstr, 0, &ps);

  if (len == (size_t) -1)
    return NULL;

  str = g_new0 (char, len + 1);
  wcsrtombs (str, &wstr, len + 1, &ps);

  return str;
}

static wchar_t *
g_locale_to_wstring (const gchar *str)
{
  mbstate_t ps;
  size_t len;
  wchar_t *wstr;

  memset (&ps, 0, sizeof (ps));
  len = mbsrtowcs (NULL, &str, 0, &ps);

  if (len == (size_t) -1)
    return NULL;

  wstr = g_new0 (wchar_t, len + 1);
  mbsrtowcs (wstr, &str, len + 1, &ps);

  return wstr;
}
#endif

G_DEFINE_TYPE (OGMRipMediaInfo, ogmrip_media_info, G_TYPE_OBJECT)

static void
ogmrip_media_info_class_init (OGMRipMediaInfoClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = ogmrip_media_info_constructed;
  gobject_class->finalize = ogmrip_media_info_finalize;

  g_type_class_add_private (klass, sizeof (OGMRipMediaInfoPriv));
}

static void
ogmrip_media_info_init (OGMRipMediaInfo *info)
{
  info->priv = G_TYPE_INSTANCE_GET_PRIVATE (info, OGMRIP_TYPE_MEDIA_INFO, OGMRipMediaInfoPriv);
}

static void
ogmrip_media_info_constructed (GObject *gobject)
{
  OGMRipMediaInfo *info = OGMRIP_MEDIA_INFO (gobject);

  MediaInfoDLL_Load ();

  info->priv->handle = MediaInfo_New ();
  if (!info->priv->handle)
  {
  }

  G_OBJECT_CLASS (ogmrip_media_info_parent_class)->constructed (gobject);
}

static void
ogmrip_media_info_finalize (GObject *gobject)
{
  OGMRipMediaInfo *info = OGMRIP_MEDIA_INFO (gobject);

  g_free (info->priv->value);

  if (info->priv->handle)
  {
    if (info->priv->is_open)
      MediaInfo_Close (info->priv->handle);
    info->priv->handle = NULL;
  }

  MediaInfoDLL_UnLoad ();

  G_OBJECT_CLASS (ogmrip_media_info_parent_class)->finalize (gobject);
}

OGMRipMediaInfo *
ogmrip_media_info_get_default (void)
{
  if (!default_media_info)
    default_media_info = g_object_new (OGMRIP_TYPE_MEDIA_INFO, NULL);

  return default_media_info;
}

gboolean
ogmrip_media_info_open (OGMRipMediaInfo *info, const gchar *filename)
{
  g_return_val_if_fail (OGMRIP_IS_MEDIA_INFO (info), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

  ogmrip_media_info_close (info);

  if (!g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    return FALSE;

#if defined(UNICODE) || defined (_UNICODE)
  wchar_t *wstr;

  wstr = g_locale_to_wstring (filename);
  info->priv->is_open = MediaInfo_Open (info->priv->handle, wstr);
  g_free (wstr);
#else
  info->priv->is_open = MediaInfo_Open (info->priv->handle, filename);
#endif

  return info->priv->is_open;
}

void
ogmrip_media_info_close (OGMRipMediaInfo *info)
{
  g_return_if_fail (OGMRIP_IS_MEDIA_INFO (info));

  if (info->priv->is_open)
  {
    MediaInfo_Close (info->priv->handle);
    info->priv->is_open = FALSE;
  }
}

const gchar *
ogmrip_media_info_get (OGMRipMediaInfo *info, OGMRipCategoryType category, guint stream, const gchar *name)
{
  const gchar *value;

  g_return_val_if_fail (OGMRIP_IS_MEDIA_INFO (info), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  if (!info->priv->handle)
    return NULL;

#if defined(UNICODE) || defined (_UNICODE)
  wchar_t *wstr1;
  const wchar_t *wstr2;
  char *str;

  wstr1 = g_locale_to_wstring (name);
  wstr2 = MediaInfo_Get (info->priv->handle, category, stream, wstr1, MediaInfo_Info_Text, MediaInfo_Info_Name);
  g_free (wstr1);

  if (info->priv->value)
    g_free (info->priv->value);

  str = g_locale_from_wstring (wstr2);
  value = info->priv->value = g_locale_to_utf8 (str, -1, NULL, NULL, NULL);
  g_free (str);
#else
  value = MediaInfo_Get (info->priv->handle, category, stream, name, MediaInfo_Info_Text, MediaInfo_Info_Name);
#endif

  return *value ? value : NULL;
}

