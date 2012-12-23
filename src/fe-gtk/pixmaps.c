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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fe-gtk.h"
#include "../common/cfgfiles.h"
#include "../common/hexchat.h"
#include "../common/fe.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <gtk/gtkstock.h>

#include "../pixmaps/inline_pngs.h"

GdkPixbuf *pix_ulist_voice;
GdkPixbuf *pix_ulist_halfop;
GdkPixbuf *pix_ulist_op;
GdkPixbuf *pix_ulist_owner;
GdkPixbuf *pix_ulist_founder;
GdkPixbuf *pix_ulist_netop;

GdkPixbuf *pix_tray_fileoffer;
GdkPixbuf *pix_tray_highlight;
GdkPixbuf *pix_tray_message;

GdkPixbuf *pix_tree_channel;
GdkPixbuf *pix_tree_dialog;
GdkPixbuf *pix_tree_server;
GdkPixbuf *pix_tree_util;

GdkPixbuf *pix_book;
GdkPixbuf *pix_hexchat;

static GdkPixmap *
pixmap_load_from_file_real (char *file)
{
	GdkPixbuf *img;
	GdkPixmap *pixmap;

	img = gdk_pixbuf_new_from_file (file, 0);
	if (!img)
		return NULL;
	gdk_pixbuf_render_pixmap_and_mask (img, &pixmap, NULL, 128);
	g_object_unref (img);

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
		strncpy (buf + 14, filename, sizeof (buf) - 14);
		buf[sizeof (buf) - 1] = 0;
		fe_message (buf, FE_MSG_ERROR);
	}

	return pix;
}

#if 0
#define LOADPIX(vv,pp,ff) \
	vv = gdk_pixbuf_new_from_file (HEXCHATSHAREDIR"/hexchat/"ff, 0); \
	if (!vv) \
		vv = gdk_pixbuf_new_from_inline (-1, pp, FALSE, 0);

#define LOADPIX_DISKONLY(vv,ff) \
	vv = gdk_pixbuf_new_from_file (HEXCHATSHAREDIR"/hexchat/"ff, 0);

#define EXT ".png"
#endif

/* load custom icons from <config>/icons, don't mess in system folders */
static GdkPixbuf *
load_pixmap (const char *filename, const char *name, int has_inline)
{
	gchar *path;
	GdkPixbuf *pixbuf;

	path = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "icons" G_DIR_SEPARATOR_S "%s.png", get_xdir (), filename);
	pixbuf = gdk_pixbuf_new_from_file (path, 0);
	g_free (path);

	if (has_inline && !pixbuf && name)
	{
		pixbuf = gdk_pixbuf_new_from_inline (-1, name, FALSE, 0);
	}

	return pixbuf;
}

void
pixmaps_init (void)
{
	/* userlist icons, with inlined defaults */
	pix_ulist_voice = load_pixmap ("ulist_voice", png_ulist_voice, 1);
	pix_ulist_halfop = load_pixmap ("ulist_halfop", png_ulist_halfop, 1);
	pix_ulist_op = load_pixmap ("ulist_op", png_ulist_op, 1);
	pix_ulist_owner = load_pixmap ("ulist_owner", png_ulist_owner, 1);
	pix_ulist_founder = load_pixmap ("ulist_founder", png_ulist_founder, 1);
	pix_ulist_netop = load_pixmap ("ulist_netop", png_ulist_netop, 1);

	/* tray icons, with inlined defaults */
	pix_tray_fileoffer = load_pixmap ("tray_fileoffer", png_tray_fileoffer, 1);
	pix_tray_highlight = load_pixmap ("tray_highlight", png_tray_highlight, 1);
	pix_tray_message = load_pixmap ("tray_message", png_tray_message, 1);

#if 0
	/* treeview icons, no defaults, load from disk only */
	pix_tree_channel = load_pixmap ("tree_channel", NULL, 0);
	pix_tree_dialog = load_pixmap ("tree_dialog", NULL, 0);
	pix_tree_server = load_pixmap ("tree_server", NULL, 0);
	pix_tree_util = load_pixmap ("tree_util", NULL, 0);
#endif
	/* provide inline defaults for these coz they are nice! */
	pix_tree_channel = load_pixmap ("tree_channel", png_tree_channel, 1);
	pix_tree_dialog = load_pixmap ("tree_dialog", png_tree_dialog, 1);
	pix_tree_server = load_pixmap ("tree_server", png_tree_server, 1);
	pix_tree_util = load_pixmap ("tree_util", png_tree_util, 1);

	/* non-replaceable book pixmap */
	pix_book = gdk_pixbuf_new_from_inline (-1, png_book, FALSE, 0);
	/* used in About window, tray icon and WindowManager icon. */
	pix_hexchat = load_pixmap ("hexchat", png_hexchat, 1);
}
