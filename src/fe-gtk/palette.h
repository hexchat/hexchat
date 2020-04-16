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

#define THEME_MAX_MIRC_COLS 32
#define THEME_MAX_COLOR (THEME_MAX_MIRC_COLS + 9)

#define COL_START_SYS 99 /* beginning index of system colors */
#define COL_MARK_FG (COL_START_SYS)
#define COL_MARK_BG (COL_START_SYS + 1)
#define COL_FG (COL_START_SYS + 2)
#define COL_BG (COL_START_SYS + 3)
#define COL_MARKER (COL_START_SYS + 4)
#define COL_NEW_DATA (COL_START_SYS + 5)
#define COL_HILIGHT (COL_START_SYS + 6)
#define COL_NEW_MSG (COL_START_SYS + 7)
#define COL_AWAY (COL_START_SYS + 8)
#define COL_SPELL (COL_START_SYS + 9)
#define MAX_COL COL_SPELL

void palette_alloc (GtkWidget * widget);
void palette_load (void);
void palette_save (void);

#endif
