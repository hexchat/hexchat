/* HexChat
 * Copyright (C) 1998-2010 Peter Zelezny.
 * Copyright (C) 2009-2013 Berke Viktor.
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

#ifndef HEXCHAT_PIXMAPS_H
#define HEXCHAT_PIXMAPS_H

extern GdkPixbuf *pix_ulist_voice;
extern GdkPixbuf *pix_ulist_halfop;
extern GdkPixbuf *pix_ulist_op;
extern GdkPixbuf *pix_ulist_owner;
extern GdkPixbuf *pix_ulist_founder;
extern GdkPixbuf *pix_ulist_netop;

extern GdkPixbuf *pix_tray_normal;
extern GdkPixbuf *pix_tray_fileoffer;
extern GdkPixbuf *pix_tray_highlight;
extern GdkPixbuf *pix_tray_message;

extern GdkPixbuf *pix_tree_channel;
extern GdkPixbuf *pix_tree_dialog;
extern GdkPixbuf *pix_tree_server;
extern GdkPixbuf *pix_tree_util;

extern GdkPixbuf *pix_book;
extern GdkPixbuf *pix_hexchat;

extern GdkPixmap *pixmap_load_from_file (char *file);
extern void pixmaps_init (void);

#endif
