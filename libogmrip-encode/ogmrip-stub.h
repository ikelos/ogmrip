/* OGMRip - A library for media ripping and encoding
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

#ifndef __OGMRIP_STUB_H__
#define __OGMRIP_STUB_H__

#include <ogmrip-codec.h>
#include <ogmrip-file.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_STUB          (ogmrip_stub_get_type ())
#define OGMRIP_STUB(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_STUB, OGMRipStub))
#define OGMRIP_STUB_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_STUB, OGMRipStubClass))
#define OGMRIP_IS_STUB(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_STUB))
#define OGMRIP_IS_STUB_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_STUB))

typedef struct _OGMRipStub      OGMRipStub;
typedef struct _OGMRipStubClass OGMRipStubClass;
typedef struct _OGMRipStubPriv  OGMRipStubPrivate;

struct _OGMRipStub
{
  OGMRipFile parent_instance;

  OGMRipStubPrivate *priv;
};

struct _OGMRipStubClass
{
  OGMRipVideoFileClass parent_class;
};

GType ogmrip_stub_get_type (void) G_GNUC_CONST;

#define OGMRIP_TYPE_VIDEO_STUB          (ogmrip_video_stub_get_type ())
#define OGMRIP_VIDEO_STUB(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_VIDEO_STUB, OGMRipVideoStub))
#define OGMRIP_VIDEO_STUB_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_VIDEO_STUB, OGMRipVideoStubClass))
#define OGMRIP_IS_VIDEO_STUB(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_VIDEO_STUB))
#define OGMRIP_IS_VIDEO_STUB_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_VIDEO_STUB))

typedef struct _OGMRipVideoStub      OGMRipVideoStub;
typedef struct _OGMRipVideoStubClass OGMRipVideoStubClass;

struct _OGMRipVideoStub
{
  OGMRipStub parent_instance;
};

struct _OGMRipVideoStubClass
{
  OGMRipStubClass parent_class;
};

GType ogmrip_video_stub_get_type (void) G_GNUC_CONST;

#define OGMRIP_TYPE_AUDIO_STUB          (ogmrip_audio_stub_get_type ())
#define OGMRIP_AUDIO_STUB(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_AUDIO_STUB, OGMRipAudioStub))
#define OGMRIP_AUDIO_STUB_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_AUDIO_STUB, OGMRipAudioStubClass))
#define OGMRIP_IS_AUDIO_STUB(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_AUDIO_STUB))
#define OGMRIP_IS_AUDIO_STUB_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_AUDIO_STUB))

typedef struct _OGMRipAudioStub      OGMRipAudioStub;
typedef struct _OGMRipAudioStubClass OGMRipAudioStubClass;

struct _OGMRipAudioStub
{
  OGMRipStub parent_instance;
};

struct _OGMRipAudioStubClass
{
  OGMRipStubClass parent_class;
};

GType ogmrip_audio_stub_get_type (void) G_GNUC_CONST;

#define OGMRIP_TYPE_SUBP_STUB          (ogmrip_subp_stub_get_type ())
#define OGMRIP_SUBP_STUB(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_SUBP_STUB, OGMRipSubpStub))
#define OGMRIP_SUBP_STUB_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_SUBP_STUB, OGMRipSubpStubClass))
#define OGMRIP_IS_SUBP_STUB(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_SUBP_STUB))
#define OGMRIP_IS_SUBP_STUB_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_SUBP_STUB))

typedef struct _OGMRipSubpStub      OGMRipSubpStub;
typedef struct _OGMRipSubpStubClass OGMRipSubpStubClass;

struct _OGMRipSubpStub
{
  OGMRipStub parent_instance;
};

struct _OGMRipSubpStubClass
{
  OGMRipStubClass parent_class;
};

GType ogmrip_subp_stub_get_type (void) G_GNUC_CONST;

#define OGMRIP_TYPE_CHAPTERS_STUB          (ogmrip_chapters_stub_get_type ())
#define OGMRIP_CHAPTERS_STUB(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_CHAPTERS_STUB, OGMRipChaptersStub))
#define OGMRIP_CHAPTERS_STUB_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_CHAPTERS_STUB, OGMRipChaptersStubClass))
#define OGMRIP_IS_CHAPTERS_STUB(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_CHAPTERS_STUB))
#define OGMRIP_IS_CHAPTERS_STUB_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_CHAPTERS_STUB))

typedef struct _OGMRipChaptersStub      OGMRipChaptersStub;
typedef struct _OGMRipChaptersStubClass OGMRipChaptersStubClass;

struct _OGMRipChaptersStub
{
  OGMRipStub parent_instance;
};

struct _OGMRipChaptersStubClass
{
  OGMRipStubClass parent_class;
};

GType ogmrip_chapters_stub_get_type (void) G_GNUC_CONST;

OGMRipFile * ogmrip_stub_new (OGMRipCodec *codec,
                              const gchar *uri);

G_END_DECLS

#endif /* __OGMRIP_STUB_H__ */

