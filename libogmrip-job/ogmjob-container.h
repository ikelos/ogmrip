/* OGMJob - A library to spawn processes
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

#ifndef __OGMJOB_CONTAINER_H__
#define __OGMJOB_CONTAINER_H__

#include <ogmjob-task.h>

G_BEGIN_DECLS

#define OGMJOB_TYPE_CONTAINER           (ogmjob_container_get_type ())
#define OGMJOB_CONTAINER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMJOB_TYPE_CONTAINER, OGMJobContainer))
#define OGMJOB_CONTAINER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), OGMJOB_TYPE_CONTAINER, OGMJobContainerClass))
#define OGMJOB_IS_CONTAINER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMJOB_TYPE_CONTAINER))
#define OGMJOB_IS_CONTAINER_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMJOB_TYPE_CONTAINER))

/**
 * OGMJobCallback:
 * @task: An #OGMJobTask
 * @data: The user data
 *
 * Specifies the type of functions passed to ogmjob_container_foreach().
 */
typedef void (* OGMJobCallback) (OGMJobTask *task,
                                 gpointer   data);

typedef struct _OGMJobContainer      OGMJobContainer;
typedef struct _OGMJobContainerClass OGMJobContainerClass;

struct _OGMJobContainer
{
  OGMJobTask parent_instance;
};

struct _OGMJobContainerClass
{
  OGMJobTaskClass parent_class;

  /* signals */
  void (* add)    (OGMJobContainer *container,
                   OGMJobTask      *task);
  void (* remove) (OGMJobContainer *container,
                   OGMJobTask      *task);
  /* vtable */
  void (* forall) (OGMJobContainer *container,
                   OGMJobCallback  callback,
                   gpointer        data);
};

GType   ogmjob_container_get_type (void);

void    ogmjob_container_add      (OGMJobContainer *container,
                                   OGMJobTask      *task);
void    ogmjob_container_remove   (OGMJobContainer *container,
                                   OGMJobTask      *task);
void    ogmjob_container_foreach  (OGMJobContainer *container,
                                   OGMJobCallback  callback,
                                   gpointer        data);

G_END_DECLS

#endif /* __OGMJOB_CONTAINER_H__ */

