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
#include "gtkutil.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixdata.h>

#include "../pixmaps/inline_pngs.h"

GdkPixbuf *pix_about;
GdkPixbuf *pix_xchat;
GdkPixbuf *pix_book;

GdkPixbuf *pix_purple;
GdkPixbuf *pix_red;
GdkPixbuf *pix_op;
GdkPixbuf *pix_hop;
GdkPixbuf *pix_voice;


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
		gtkutil_simpledialog (buf);
	}

	return pix;
}

void
pixmaps_init (void)
{
	pix_about = gdk_pixbuf_new_from_inline (-1, aboutpng, FALSE, 0);
	pix_book = gdk_pixbuf_new_from_inline (-1, bookpng, FALSE, 0);
	pix_xchat = gdk_pixbuf_new_from_inline (-1, xchatpng, FALSE, 0);

	pix_purple = gdk_pixbuf_new_from_file (XCHATSHAREDIR"/purple.png", 0);
	if (!pix_purple)
		pix_purple = gdk_pixbuf_new_from_inline (-1, purplepng, FALSE, 0);

	pix_red = gdk_pixbuf_new_from_file (XCHATSHAREDIR"/red.png", 0);
	if (!pix_red)
		pix_red = gdk_pixbuf_new_from_inline (-1, redpng, FALSE, 0);

	pix_op = gdk_pixbuf_new_from_file (XCHATSHAREDIR"/op.png", 0);
	if (!pix_op)
		pix_op = gdk_pixbuf_new_from_inline (-1, oppng, FALSE, 0);

	pix_hop = gdk_pixbuf_new_from_file (XCHATSHAREDIR"/hop.png", 0);
	if (!pix_hop)
		pix_hop = gdk_pixbuf_new_from_inline (-1, hoppng, FALSE, 0);

	pix_voice = gdk_pixbuf_new_from_file (XCHATSHAREDIR"/voice.png", 0);
	if (!pix_voice)
		pix_voice = gdk_pixbuf_new_from_inline (-1, voicepng, FALSE, 0);
}
