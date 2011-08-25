/* OGMRip - A library for DVD ripping and encoding
 * Testright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
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
 * You should have received a test of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __OGMRIP_TEST_H__
#define __OGMRIP_TEST_H__

#include <ogmjob.h>
#include <ogmrip-encoding.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_TEST          (ogmrip_test_get_type ())
#define OGMRIP_TEST(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_TEST, OGMRipTest))
#define OGMRIP_TEST_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_TEST, OGMRipTestClass))
#define OGMRIP_IS_TEST(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_TEST))
#define OGMRIP_IS_TEST_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_TEST))

typedef struct _OGMRipTest      OGMRipTest;
typedef struct _OGMRipTestClass OGMRipTestClass;
typedef struct _OGMRipTestPriv  OGMRipTestPriv;

struct _OGMRipTest
{
  OGMJobSpawn parent_instance;

  OGMRipTestPriv *priv;
};

struct _OGMRipTestClass
{
  OGMJobSpawnClass parent_class;
};

GType         ogmrip_test_get_type (void);
OGMJobSpawn * ogmrip_test_new      (OGMRipEncoding *encoding);

G_END_DECLS

#endif /* __OGMRIP_TEST_H__ */

