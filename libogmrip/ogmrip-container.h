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

#ifndef __OGMRIP_CONTAINER_H__
#define __OGMRIP_CONTAINER_H__

#include <ogmjob-bin.h>

#include <ogmrip-profile.h>
#include <ogmrip-chapters.h>
#include <ogmrip-video-codec.h>
#include <ogmrip-audio-codec.h>
#include <ogmrip-subp-codec.h>
#include <ogmrip-file.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_CONTAINER           (ogmrip_container_get_type ())
#define OGMRIP_CONTAINER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_CONTAINER, OGMRipContainer))
#define OGMRIP_CONTAINER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_CONTAINER, OGMRipContainerClass))
#define OGMRIP_IS_CONTAINER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_CONTAINER))
#define OGMRIP_IS_CONTAINER_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_CONTAINER))
#define OGMRIP_CONTAINER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMRIP_TYPE_CONTAINER, OGMRipContainerClass))

typedef struct _OGMRipContainer      OGMRipContainer;
typedef struct _OGMRipContainerPriv  OGMRipContainerPriv;
typedef struct _OGMRipContainerClass OGMRipContainerClass;

/**
 * OGMRipContainerCodecFunc:
 * @container: An #OGMRipContainer
 * @codec: An #OGMRipCodec
 * @demuxer: The demuxer to be used
 * @language: The language of the stream
 * @user_data: The user data
 *
 * Specifies the type of functions passed to ogmrip_container_foreach_audio(),
 * ogmrip_container_foreach_subp(), and ogmrip_container_foreach_chapters().
 */
typedef void (* OGMRipContainerCodecFunc) (OGMRipContainer *container,
                                           OGMRipCodec     *codec,
                                           guint           demuxer,
                                           gint            language,
                                           gpointer        user_data);

/**
 * OGMRipContainerFileFunc:
 * @container: An #OGMRipContainer
 * @file: An #OGMRipFile
 * @user_data: The user data
 *
 * Specifies the type of functions passed to ogmrip_container_foreach_file().
 */
typedef void (* OGMRipContainerFileFunc)  (OGMRipContainer *container,
                                           OGMRipFile      *file,
                                           gpointer        user_data);

struct _OGMRipContainer
{
  OGMJobBin parent_instance;

  OGMRipContainerPriv *priv;
};

struct _OGMRipContainerClass
{
  OGMJobBinClass parent_class;

  /* vtable */
  gint (* get_overhead) (OGMRipContainer *container);
  void (* set_profile)  (OGMRipContainer *container,
                         OGMRipProfile   *profile);
};

GType              ogmrip_container_get_type             (void);

void               ogmrip_container_set_profile          (OGMRipContainer          *container,
                                                          OGMRipProfile            *profile);

G_CONST_RETURN
gchar *            ogmrip_container_get_output           (OGMRipContainer          *container);
G_CONST_RETURN
gchar *            ogmrip_container_get_label            (OGMRipContainer          *container);
void               ogmrip_container_set_label            (OGMRipContainer          *container,
                                                          const gchar              *label);

G_CONST_RETURN
gchar *            ogmrip_container_get_fourcc           (OGMRipContainer          *container);
void               ogmrip_container_set_fourcc           (OGMRipContainer          *container,
                                                          const gchar              *fourcc);

gint               ogmrip_container_get_start_delay      (OGMRipContainer          *container);
void               ogmrip_container_set_start_delay      (OGMRipContainer          *container,
                                                          guint                    start_delay);

gint               ogmrip_container_get_overhead         (OGMRipContainer          *container);

OGMRipVideoCodec * ogmrip_container_get_video            (OGMRipContainer          *container);
void               ogmrip_container_set_video            (OGMRipContainer          *container,
                                                          OGMRipVideoCodec         *video);

void               ogmrip_container_add_audio            (OGMRipContainer          *container,
                                                          OGMRipAudioCodec         *audio,
                                                          OGMRipAudioDemuxer       demuxer,
                                                          gint                     language);
void               ogmrip_container_remove_audio         (OGMRipContainer          *container,
                                                          OGMRipAudioCodec         *audio);
GSList *           ogmrip_container_get_audio            (OGMRipContainer          *container);
OGMRipAudioCodec * ogmrip_container_get_nth_audio        (OGMRipContainer          *container,
                                                          gint                     n);
gint               ogmrip_container_get_n_audio          (OGMRipContainer          *container);
void               ogmrip_container_foreach_audio        (OGMRipContainer          *container, 
                                                          OGMRipContainerCodecFunc func, 
                                                          gpointer                 data);

void               ogmrip_container_add_subp             (OGMRipContainer          *container,
                                                          OGMRipSubpCodec          *subp,
                                                          OGMRipSubpDemuxer        demuxer,
                                                          gint                     language);
void               ogmrip_container_remove_subp          (OGMRipContainer          *container,
                                                          OGMRipSubpCodec          *subp);
GSList *           ogmrip_container_get_subp             (OGMRipContainer          *container);
gint               ogmrip_container_get_n_subp           (OGMRipContainer          *container);
OGMRipSubpCodec *  ogmrip_container_get_nth_subp         (OGMRipContainer          *container,
                                                          gint                     n);
void               ogmrip_container_foreach_subp         (OGMRipContainer          *container, 
                                                          OGMRipContainerCodecFunc func, 
                                                          gpointer                 data);

void               ogmrip_container_add_chapters         (OGMRipContainer          *container,
                                                          OGMRipChapters           *chapters,
                                                          gint                     language);
void               ogmrip_container_remove_chapters      (OGMRipContainer          *container,
                                                          OGMRipChapters           *chapters);
GSList *           ogmrip_container_get_chapters         (OGMRipContainer          *container);
OGMRipChapters *   ogmrip_container_get_nth_chapters     (OGMRipContainer          *container,
                                                          gint                     n);
gint               ogmrip_container_get_n_chapters       (OGMRipContainer          *container);
void               ogmrip_container_foreach_chapters     (OGMRipContainer          *container, 
                                                          OGMRipContainerCodecFunc func, 
                                                          gpointer                 data);

void               ogmrip_container_add_file             (OGMRipContainer          *container,
                                                          OGMRipFile               *file);
void               ogmrip_container_remove_file          (OGMRipContainer          *container,
                                                          OGMRipFile               *file);
GSList *           ogmrip_container_get_files            (OGMRipContainer          *container);
OGMRipFile *       ogmrip_container_get_nth_file         (OGMRipContainer          *container,
                                                          gint                     n);
gint               ogmrip_container_get_n_files          (OGMRipContainer          *container);
void               ogmrip_container_foreach_file         (OGMRipContainer          *container,
                                                          OGMRipContainerFileFunc func,
                                                          gpointer                 data);

void               ogmrip_container_set_split            (OGMRipContainer          *container,
                                                          guint                    number,
                                                          guint                    size);
void               ogmrip_container_get_split            (OGMRipContainer          *container,
                                                          guint                    *number,
                                                          guint                    *size);

gint64             ogmrip_container_get_overhead_size    (OGMRipContainer          *container);
gint64             ogmrip_container_get_nonvideo_size    (OGMRipContainer          *container);

G_END_DECLS

#endif /* __OGMRIP_CONTAINER_H__ */

