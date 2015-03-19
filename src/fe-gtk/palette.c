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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "fe-gtk.h"
#include "palette.h"

#include "../common/hexchat.h"
#include "../common/util.h"
#include "../common/cfgfiles.h"
#include "../common/typedef.h"


GdkColor colors[] = {
	/* colors for xtext */
	{0, 0xd3d3, 0xd7d7, 0xcfcf}, /* 0 white */
	{0, 0x2e2e, 0x3434, 0x3636}, /* 1 black */
	{0, 0x3434, 0x6565, 0xa4a4}, /* 2 blue */
	{0, 0x4e4e, 0x9a9a, 0x0606}, /* 3 green */
	{0, 0xcccc, 0x0000, 0x0000}, /* 4 red */
	{0, 0x8f8f, 0x3939, 0x0202}, /* 5 light red */
	{0, 0x5c5c, 0x3535, 0x6666}, /* 6 purple */
	{0, 0xcece, 0x5c5c, 0x0000}, /* 7 orange */
	{0, 0xc4c4, 0xa0a0, 0x0000}, /* 8 yellow */
	{0, 0x7373, 0xd2d2, 0x1616}, /* 9 green */
	{0, 0x1111, 0xa8a8, 0x7979}, /* 10 aqua */
	{0, 0x5858, 0xa1a1, 0x9d9d}, /* 11 light aqua */
	{0, 0x5757, 0x7979, 0x9e9e}, /* 12 blue */
	{0, 0xa0d0, 0x42d4, 0x6562}, /* 13 light purple */
	{0, 0x5555, 0x5757, 0x5353}, /* 14 grey */
	{0, 0x8888, 0x8a8a, 0x8585}, /* 15 light grey */

	{0, 0xd3d3, 0xd7d7, 0xcfcf}, /* 16 white */
	{0, 0x2e2e, 0x3434, 0x3636}, /* 17 black */
	{0, 0x3434, 0x6565, 0xa4a4}, /* 18 blue */
	{0, 0x4e4e, 0x9a9a, 0x0606}, /* 19 green */
	{0, 0xcccc, 0x0000, 0x0000}, /* 20 red */
	{0, 0x8f8f, 0x3939, 0x0202}, /* 21 light red */
	{0, 0x5c5c, 0x3535, 0x6666}, /* 22 purple */
	{0, 0xcece, 0x5c5c, 0x0000}, /* 23 orange */
	{0, 0xc4c4, 0xa0a0, 0x0000}, /* 24 yellow */
	{0, 0x7373, 0xd2d2, 0x1616}, /* 25 green */
	{0, 0x1111, 0xa8a8, 0x7979}, /* 26 aqua */
	{0, 0x5858, 0xa1a1, 0x9d9d}, /* 27 light aqua */
	{0, 0x5757, 0x7979, 0x9e9e}, /* 28 blue */
	{0, 0xa0d0, 0x42d4, 0x6562}, /* 29 light purple */
	{0, 0x5555, 0x5757, 0x5353}, /* 30 grey */
	{0, 0x8888, 0x8a8a, 0x8585}, /* 31 light grey */

	{0, 0xd3d3, 0xd7d7, 0xcfcf}, /* 32 marktext Fore (white) */
	{0, 0x2020, 0x4a4a, 0x8787}, /* 33 marktext Back (blue) */
	{0, 0x2512, 0x29e8, 0x2b85}, /* 34 foreground (black) */
	{0, 0xfae0, 0xfae0, 0xf8c4}, /* 35 background (white) */
	{0, 0x8f8f, 0x3939, 0x0202}, /* 36 marker line (red) */

	/* colors for GUI */
	{0, 0x3434, 0x6565, 0xa4a4}, /* 37 tab New Data (dark red) */
	{0, 0x4e4e, 0x9a9a, 0x0606}, /* 38 tab Nick Mentioned (blue) */
	{0, 0xcece, 0x5c5c, 0x0000}, /* 39 tab New Message (red) */
	{0, 0x8888, 0x8a8a, 0x8585}, /* 40 away user (grey) */
	{0, 0xa4a4, 0x0000, 0x0000}, /* 41 spell checker color (red) */
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
		for (i = MAX_COL; i >= 0; i--)
			gdk_colormap_alloc_color (cmap, &colors[i], FALSE, TRUE);
	}
}

void
palette_load (void)
{
	int i, j, fh;
	char prefname[256];
	struct stat st;
	char *cfg;
	guint16 red, green, blue;

	fh = hexchat_open_file ("colors.conf", O_RDONLY, 0, 0);
	if (fh != -1)
	{
		fstat (fh, &st);
		cfg = g_malloc0 (st.st_size + 1);
		read (fh, cfg, st.st_size);

		/* mIRC colors 0-31 are here */
		for (i = 0; i < 32; i++)
		{
			g_snprintf (prefname, sizeof prefname, "color_%d", i);
			cfg_get_color (cfg, prefname, &red, &green, &blue);
			colors[i].red = red;
			colors[i].green = green;
			colors[i].blue = blue;
		}

		/* our special colors are mapped at 256+ */
		for (i = 256, j = 32; j < MAX_COL+1; i++, j++)
		{
			g_snprintf (prefname, sizeof prefname, "color_%d", i);
			cfg_get_color (cfg, prefname, &red, &green, &blue);
			colors[j].red = red;
			colors[j].green = green;
			colors[j].blue = blue;
		}
		g_free (cfg);
		close (fh);
	}
}

void
palette_save (void)
{
	int i, j, fh;
	char prefname[256];

	fh = hexchat_open_file ("colors.conf", O_TRUNC | O_WRONLY | O_CREAT, 0600, XOF_DOMODE);
	if (fh != -1)
	{
		/* mIRC colors 0-31 are here */
		for (i = 0; i < 32; i++)
		{
			g_snprintf (prefname, sizeof prefname, "color_%d", i);
			cfg_put_color (fh, colors[i].red, colors[i].green, colors[i].blue, prefname);
		}

		/* our special colors are mapped at 256+ */
		for (i = 256, j = 32; j < MAX_COL+1; i++, j++)
		{
			g_snprintf (prefname, sizeof prefname, "color_%d", i);
			cfg_put_color (fh, colors[j].red, colors[j].green, colors[j].blue, prefname);
		}

		close (fh);
	}
}
