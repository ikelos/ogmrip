/* OGMRip - A library for media ripping and encoding
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

#ifndef __OGMRIP_ANALYZE_H__
#define __OGMRIP_ANALYZE_H__

#include <ogmrip-job.h>
#include <ogmrip-media.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_ANALYZE          (ogmrip_analyze_get_type ())
#define OGMRIP_ANALYZE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_ANALYZE, OGMRipAnalyze))
#define OGMRIP_ANALYZE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_ANALYZE, OGMRipAnalyzeClass))
#define OGMRIP_IS_ANALYZE(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_ANALYZE))
#define OGMRIP_IS_ANALYZE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_ANALYZE))

typedef struct _OGMRipAnalyze      OGMRipAnalyze;
typedef struct _OGMRipAnalyzeClass OGMRipAnalyzeClass;
typedef struct _OGMRipAnalyzePriv  OGMRipAnalyzePriv;

struct _OGMRipAnalyze
{
  OGMJobSpawn parent_instance;

  OGMRipAnalyzePriv *priv;
};

struct _OGMRipAnalyzeClass
{
  OGMJobSpawnClass parent_class;
};

GType        ogmrip_analyze_get_type (void);
OGMJobTask * ogmrip_analyze_new      (OGMRipTitle *title);

G_END_DECLS

#endif /* __OGMRIP_ANALYZE_H__ */

