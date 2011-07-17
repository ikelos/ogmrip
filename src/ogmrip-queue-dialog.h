/* OGMRip - A DVD Encoder for GNOME
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __OGMRIP_QUEUE_DIALOG_H__
#define __OGMRIP_QUEUE_DIALOG_H__

#include <ogmrip-gtk.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_QUEUE_DIALOG            (ogmrip_queue_dialog_get_type ())
#define OGMRIP_QUEUE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_QUEUE_DIALOG, OGMRipQueueDialog))
#define OGMRIP_QUEUE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_QUEUE_DIALOG, OGMRipQueueDialogClass))
#define OGMRIP_IS_QUEUE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_QUEUE_DIALOG))
#define OGMRIP_IS_QUEUE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_QUEUE_DIALOG))

typedef struct _OGMRipQueueDialog      OGMRipQueueDialog;
typedef struct _OGMRipQueueDialogClass OGMRipQueueDialogClass;
typedef struct _OGMRipQueueDialogPriv  OGMRipQueueDialogPriv;

struct _OGMRipQueueDialog
{
  GtkDialog parent_instance;

  OGMRipQueueDialogPriv *priv;
};

struct _OGMRipQueueDialogClass
{
  GtkDialogClass parent_class;

  void (* add_encoding)    (OGMRipQueueDialog *dialog,
                            OGMRipEncoding    *encoding);
  void (* remove_encoding) (OGMRipQueueDialog *dialog,
                            OGMRipEncoding    *encoding);
  void (* import_encoding) (OGMRipQueueDialog *dialog,
                            OGMRipEncoding    *encoding);
  void (* export_encoding) (OGMRipQueueDialog *dialog,
                            OGMRipEncoding    *encoding);
};

GType         ogmrip_queue_dialog_get_type           (void);
GtkWidget *   ogmrip_queue_dialog_new                (void);

void          ogmrip_queue_dialog_add_encoding       (OGMRipQueueDialog   *dialog,
                                                      OGMRipEncoding      *encoding);
void          ogmrip_queue_dialog_remove_encoding    (OGMRipQueueDialog   *dialog,
                                                      OGMRipEncoding      *encoding);
gboolean      ogmrip_queue_dialog_foreach_encoding   (OGMRipQueueDialog   *dialog,
                                                      OGMRipEncodingFunc  func,
                                                      gpointer            data);

G_END_DECLS

#endif /* __OGMRIP_QUEUE_DIALOG_H__ */

