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

#ifndef HEXCHAT_PALETTE_H
#define HEXCHAT_PALETTE_H

extern GdkColor colors[];

#define COL_MARK_FG 32
#define COL_MARK_BG 33
#define COL_FG 34
#define COL_BG 35
#define COL_MARKER 36
#define COL_NEW_DATA 37
#define COL_HILIGHT 38
#define COL_NEW_MSG 39
#define COL_AWAY 40
#define COL_SPELL 41
#define MAX_COL 41

void palette_alloc (GtkWidget * widget);
void palette_load (void);
void palette_save (void);

#endif
