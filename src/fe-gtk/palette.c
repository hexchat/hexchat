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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fe-gtk.h"

#include "../common/xchat.h"
#include "../common/util.h"
#include "../common/cfgfiles.h"


GdkColor colors[] = {
	{0, 0xcf3c, 0xcf3c, 0xcf3c}, /* 0  white */
	{0, 0x0000, 0x0000, 0x0000}, /* 1  black */
	{0, 0x0000, 0x0000, 0xcccc}, /* 2  blue */
	{0, 0x0000, 0xcccc, 0x0000}, /* 3  green */
	{0, 0xdddd, 0x0000, 0x0000}, /* 4  red */
	{0, 0xaaaa, 0x0000, 0x0000}, /* 5  light red */
	{0, 0xbbbb, 0x0000, 0xbbbb}, /* 6  purple */
	{0, 0xffff, 0xaaaa, 0x0000}, /* 7  orange */
	{0, 0xeeee, 0xdddd, 0x2222}, /* 8  yellow */
	{0, 0x3333, 0xdede, 0x5555}, /* 9  green */
	{0, 0x0000, 0xcccc, 0xcccc}, /* 10 aqua */
	{0, 0x3333, 0xeeee, 0xffff}, /* 11 light aqua */
	{0, 0x0000, 0x0000, 0xffff}, /* 12 blue */
	{0, 0xeeee, 0x2222, 0xeeee}, /* 13 light purple */
	{0, 0x7777, 0x7777, 0x7777}, /* 14 grey */
	{0, 0x9999, 0x9999, 0x9999}, /* 15 light grey */
	{0, 0xa4a4, 0xdfdf, 0xffff}, /* 16 marktext Back (blue) */
	{0, 0x0000, 0x0000, 0x0000}, /* 17 marktext Fore (black) */
	{0, 0xdf3c, 0xdf3c, 0xdf3c}, /* 18 foreground (white) */
	{0, 0x0000, 0x0000, 0x0000}, /* 19 background (black) */
	{0, 0x8c8c, 0x1010, 0x1010}, /* 20 tab New Data (dark red) */
	{0, 0x0000, 0x0000, 0xffff}, /* 21 tab Nick Mentioned (blue) */
	{0, 0xf5f5, 0x0000, 0x0000}, /* 22 tab New Message (red) */
};

void
palette_alloc (GtkWidget * widget)
{
	int i;
	static int done_alloc = FALSE;
	GdkColormap *cmap;

	if (!done_alloc)		  /* don't do it again */
	{
		done_alloc = TRUE;
		cmap = gtk_widget_get_colormap (widget);
		for (i = 22; i >= 0; i--)
			gdk_colormap_alloc_color (cmap, &colors[i], TRUE, TRUE);
	}
}

void
palette_load (void)
{
	int i, l, fh, res;
	char prefname[256];
	struct stat st;
	char *cfg;
	unsigned long red, green, blue;

   snprintf (prefname, sizeof (prefname), "%s/palette.conf", get_xdir ());
   fh = open (prefname, O_RDONLY | OFLAGS);
	if (fh != -1)
	{
		fstat (fh, &st);
		cfg = malloc (st.st_size + 1);
		if (cfg)
		{
			cfg[0] = '\0';
			l = read (fh, cfg, st.st_size);
			if (l >= 0)
				cfg[l] = '\0';

			for (i = 0; i < 23; i++)
			{
				snprintf (prefname, sizeof prefname, "color_%d_red", i);
				red = cfg_get_int (cfg, prefname);

				snprintf (prefname, sizeof prefname, "color_%d_grn", i);
				green = cfg_get_int (cfg, prefname);

				snprintf (prefname, sizeof prefname, "color_%d_blu", i);
				blue = cfg_get_int_with_result (cfg, prefname, &res);

				if (res)
				{
					colors[i].red = red;
					colors[i].green = green;
					colors[i].blue = blue;
				}
			}
			free (cfg);
		}
		close (fh);
	}
}

void
palette_save (void)
{
	int i, fh;
	char prefname[256];

   snprintf (prefname, sizeof (prefname), "%s/palette.conf", get_xdir ());
   fh = open (prefname, O_TRUNC | O_WRONLY | O_CREAT | OFLAGS, 0600);
	if (fh != -1)
	{
		for (i = 0; i < 23; i++)
		{
			snprintf (prefname, sizeof prefname, "color_%d_red", i);
			cfg_put_int (fh, colors[i].red, prefname);

			snprintf (prefname, sizeof prefname, "color_%d_grn", i);
			cfg_put_int (fh, colors[i].green, prefname);

			snprintf (prefname, sizeof prefname, "color_%d_blu", i);
			cfg_put_int (fh, colors[i].blue, prefname);
		}
		close (fh);
	}
}
