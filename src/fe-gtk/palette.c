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
	/* colors for xtext */
	{0, 0xcccc, 0xcccc, 0xcccc}, /* 0  white */
	{0, 0x0000, 0x0000, 0x0000}, /* 1  black */
	{0, 0x0000, 0x0000, 0xcccc}, /* 2  blue */
	{0, 0x0000, 0x9999, 0x0000}, /* 3  green */
	{0, 0xcccc, 0x0000, 0x0000}, /* 4  red */
	{0, 0xaaaa, 0x0000, 0x0000}, /* 5  light red */
	{0, 0xaaaa, 0x0000, 0xaaaa}, /* 6  purple */
	{0, 0x9999, 0x3333, 0x0000}, /* 7  orange */
	{0, 0xffff, 0xaaaa, 0x0000}, /* 8  yellow */
	{0, 0x0000, 0xffff, 0x0000}, /* 9  green */
	{0, 0x0000, 0x5555, 0x5555}, /* 10 aqua */
	{0, 0x3333, 0x9999, 0x7f7f}, /* 11 light aqua */
	{0, 0x0000, 0x0000, 0xffff}, /* 12 blue */
	{0, 0xffff, 0x3333, 0xffff}, /* 13 light purple */
	{0, 0x7f7f, 0x7f7f, 0x7f7f}, /* 14 grey */
	{0, 0x9595, 0x9595, 0x9595}, /* 15 light grey */

	{0, 0xcccc, 0xcccc, 0xcccc}, /* 16 white */
	{0, 0x0000, 0x0000, 0x0000}, /* 17 black */
	{0, 0x0000, 0x0000, 0xcccc}, /* 18 blue */
	{0, 0x0000, 0x9999, 0x0000}, /* 19 green */
	{0, 0xcccc, 0x0000, 0x0000}, /* 20 red */
	{0, 0xaaaa, 0x0000, 0x0000}, /* 21 light red */
	{0, 0xaaaa, 0x0000, 0xaaaa}, /* 22 purple */
	{0, 0x9999, 0x3333, 0x0000}, /* 23 orange */
	{0, 0xffff, 0xaaaa, 0x0000}, /* 24 yellow */
	{0, 0x0000, 0xffff, 0x0000}, /* 25 green */
	{0, 0x0000, 0x5555, 0x5555}, /* 26 aqua */
	{0, 0x3333, 0x9999, 0x7f7f}, /* 27 light aqua */
	{0, 0x0000, 0x0000, 0xffff}, /* 28 blue */
	{0, 0xffff, 0x3333, 0xffff}, /* 29 light purple */
	{0, 0x7f7f, 0x7f7f, 0x7f7f}, /* 30 grey */
	{0, 0x9595, 0x9595, 0x9595}, /* 31 light grey */

	{0, 0x0000, 0x0000, 0x0000}, /* 32 marktext Fore (black) */
	{0, 0x9999, 0xcccc, 0xffff}, /* 33 marktext Back (blue) */
	{0, 0x0000, 0x0000, 0x0000}, /* 34 foreground (black) */
	{0, 0xf0f0, 0xf0f0, 0xf0f0}, /* 35 background (white) */
	{0, 0xcccc, 0x0000, 0x0000}, /* 36 marker line (red) */

	/* colors for GUI */
	{0, 0x9999, 0x0000, 0x0000}, /* 37 tab New Data (dark red) */
	{0, 0x0000, 0x0000, 0xffff}, /* 38 tab Nick Mentioned (blue) */
	{0, 0xffff, 0x0000, 0x0000}, /* 39 tab New Message (red) */
	{0, 0x9595, 0x9595, 0x9595}, /* 40 away user (grey) */
};
#define MAX_COL 40


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
	int i, l, fh, res;
	char prefname[256];
	struct stat st;
	char *cfg;
	unsigned long red, green, blue;

   snprintf (prefname, sizeof (prefname), "%s/colors.conf", get_xdir_fs ());
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

			for (i = 0; i < MAX_COL+1; i++)
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

   snprintf (prefname, sizeof (prefname), "%s/colors.conf", get_xdir_fs ());
   fh = open (prefname, O_TRUNC | O_WRONLY | O_CREAT | OFLAGS, 0600);
	if (fh != -1)
	{
		for (i = 0; i < MAX_COL+1; i++)
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
