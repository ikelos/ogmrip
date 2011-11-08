/* OGMJob - A library to spawn processes
 * Copyright (C) 2004-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMJOB_PIPELINE_H__
#define __OGMJOB_PIPELINE_H__

#include <ogmjob-container.h>

G_BEGIN_DECLS

#define OGMJOB_TYPE_PIPELINE           (ogmjob_pipeline_get_type ())
#define OGMJOB_PIPELINE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMJOB_TYPE_PIPELINE, OGMJobPipeline))
#define OGMJOB_PIPELINE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), OGMJOB_TYPE_PIPELINE, OGMJobPipelineClass))
#define OGMJOB_IS_PIPELINE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMJOB_TYPE_PIPELINE))
#define OGMJOB_IS_PIPELINE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMJOB_TYPE_PIPELINE))
#define OGMJOB_PIPELINE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMJOB_TYPE_PIPELINE, OGMJobPipelineClass))

typedef struct _OGMJobPipeline      OGMJobPipeline;
typedef struct _OGMJobPipelinePriv  OGMJobPipelinePriv;
typedef struct _OGMJobPipelineClass OGMJobPipelineClass;

struct _OGMJobPipeline
{
  OGMJobContainer parent_instance;

  OGMJobPipelinePriv *priv;
};

struct _OGMJobPipelineClass
{
  OGMJobContainerClass parent_class;
};

GType        ogmjob_pipeline_get_type (void);
OGMJobTask * ogmjob_pipeline_new      (void);

G_END_DECLS

#endif /* __OGMJOB_PIPELINE_H__ */

