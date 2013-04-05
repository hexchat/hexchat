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

#ifndef HEXCHAT_MMX_CMOD_H
#define HEXCHAT_MMX_CMOD_H

void shade_ximage_15_mmx(void *data, int bpl, int w, int h, int rm, int gm, int bm);
void shade_ximage_16_mmx(void *data, int bpl, int w, int h, int rm, int gm, int bm);
void shade_ximage_32_mmx(void *data, int bpl, int w, int h, int rm, int gm, int bm);
int have_mmx (void);

#endif
