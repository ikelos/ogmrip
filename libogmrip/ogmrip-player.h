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

#ifndef __OGMRIP_PLAYER_H__
#define __OGMRIP_PLAYER_H__

#include <glib-object.h>

#include <ogmdvd.h>
#include <ogmrip-file.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_PLAYER          (ogmrip_player_get_type ())
#define OGMRIP_PLAYER(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_PLAYER, OGMRipPlayer))
#define OGMRIP_PLAYER_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_PLAYER, OGMRipPlayerClass))
#define OGMRIP_IS_PLAYER(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_PLAYER))
#define OGMRIP_IS_PLAYER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_PLAYER))

typedef struct _OGMRipPlayer      OGMRipPlayer;
typedef struct _OGMRipPlayerPriv  OGMRipPlayerPriv;
typedef struct _OGMRipPlayerClass OGMRipPlayerClass;

struct _OGMRipPlayer
{
  GObject parent_instance;

  OGMRipPlayerPriv *priv;
};

struct _OGMRipPlayerClass
{
  GObjectClass parent_class;

  void (* play) (OGMRipPlayer *player);
  void (* stop) (OGMRipPlayer *player);
};

GType          ogmrip_player_get_type         (void);
OGMRipPlayer * ogmrip_player_new              (void);

void           ogmrip_player_set_title        (OGMRipPlayer      *player,
                                               OGMDvdTitle       *title);
void           ogmrip_player_set_audio_stream (OGMRipPlayer      *player,
                                               OGMDvdAudioStream *stream);
void           ogmrip_player_set_audio_file   (OGMRipPlayer      *player,
                                               OGMRipFile        *file);
void           ogmrip_player_set_subp_stream  (OGMRipPlayer      *player,
                                               OGMDvdSubpStream  *stream);
void           ogmrip_player_set_subp_file    (OGMRipPlayer      *player,
                                               OGMRipFile        *file);
void           ogmrip_player_set_chapters     (OGMRipPlayer      *player,
                                               guint             start,
                                               gint              end);

gboolean       ogmrip_player_play             (OGMRipPlayer      *player,
                                               GError            **error);
void           ogmrip_player_stop             (OGMRipPlayer      *player);

G_END_DECLS

#endif /* __OGMRIP_PLAYER_H__ */

