/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#define GTK_DISABLE_DEPRECATED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/fe.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <gtk/gtkstock.h>

#include "../pixmaps/inline_pngs.h"

GdkPixbuf *pix_xchat;
GdkPixbuf *pix_book;

GdkPixbuf *pix_purple;
GdkPixbuf *pix_red;
GdkPixbuf *pix_op;
GdkPixbuf *pix_hop;
GdkPixbuf *pix_voice;

GdkPixbuf *pix_channel;
GdkPixbuf *pix_dialog;
GdkPixbuf *pix_server;
GdkPixbuf *pix_util;


static GdkPixmap *
pixmap_load_from_file_real (char *file)
{
	GdkPixbuf *img;
	GdkPixmap *pixmap;

	img = gdk_pixbuf_new_from_file (file, 0);
	if (!img)
		return NULL;
	gdk_pixbuf_render_pixmap_and_mask (img, &pixmap, NULL, 128);
	gdk_pixbuf_unref (img);

	return pixmap;
}

GdkPixmap *
pixmap_load_from_file (char *filename)
{
	char buf[256];
	GdkPixmap *pix;

	if (filename[0] == '\0')
		return NULL;

	pix = pixmap_load_from_file_real (filename);
	if (pix == NULL)
	{
		strcpy (buf, "Cannot open:\n\n");
		strcpy (buf + 14, filename);
		fe_message (buf, FE_MSG_ERROR);
	}

	return pix;
}

#define LOADPIX(vv,pp,ff) \
	vv = gdk_pixbuf_new_from_file (XCHATSHAREDIR"/"ff, 0); \
	if (!vv) \
		vv = gdk_pixbuf_new_from_inline (-1, pp, FALSE, 0);

#define LOADPIX_DISKONLY(vv,ff) \
	vv = gdk_pixbuf_new_from_file (XCHATSHAREDIR"/"ff, 0);

void
pixmaps_init (void)
{
	pix_book = gdk_pixbuf_new_from_inline (-1, bookpng, FALSE, 0);
	pix_xchat = gdk_pixbuf_new_from_inline (-1, xchatpng, FALSE, 0);

	/* userlist icons, with inlined defaults */
	LOADPIX (pix_hop, hoppng, "hop.png");
	LOADPIX (pix_purple, purplepng, "purple.png");
	LOADPIX (pix_red, redpng, "red.png");
	LOADPIX (pix_op, oppng, "op.png");
	LOADPIX (pix_voice, voicepng, "voice.png");

	/* treeview icons, no defaults, load from disk only */
	LOADPIX_DISKONLY (pix_channel,	"channel.png");
	LOADPIX_DISKONLY (pix_dialog,		"dialog.png");
	LOADPIX_DISKONLY (pix_server,		"server.png");
	LOADPIX_DISKONLY (pix_util,		"util.png");
}

