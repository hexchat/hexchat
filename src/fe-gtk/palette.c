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
#include "palette.h"

#include "../common/xchat.h"
#include "../common/util.h"
#include "../common/cfgfiles.h"


GdkColor colors[] = {
	/* colors for xtext */
	{0, 0xcccc, 0xcccc, 0xcccc}, /* 16 white */
	{0, 0x0000, 0x0000, 0x0000}, /* 17 black */
	{0, 0x35c2, 0x35c2, 0xb332}, /* 18 blue */
	{0, 0x2a3d, 0x8ccc, 0x2a3d}, /* 19 green */
	{0, 0xc3c3, 0x3b3b, 0x3b3b}, /* 20 red */
	{0, 0xc7c7, 0x3232, 0x3232}, /* 21 light red */
	{0, 0x8000, 0x2666, 0x7fff}, /* 22 purple */
	{0, 0x6666, 0x3636, 0x1f1f}, /* 23 orange */
	{0, 0xd999, 0xa6d3, 0x4147}, /* 24 yellow */
	{0, 0x3d70, 0xcccc, 0x3d70}, /* 25 green */
	{0, 0x199a, 0x5555, 0x5555}, /* 26 aqua */
	{0, 0x2eef, 0x8ccc, 0x74df}, /* 27 light aqua */
	{0, 0x451e, 0x451e, 0xe666}, /* 28 blue */
	{0, 0xb0b0, 0x3737, 0xb0b0}, /* 29 light purple */
	{0, 0x4c4c, 0x4c4c, 0x4c4c}, /* 30 grey */
	{0, 0x9595, 0x9595, 0x9595}, /* 31 light grey */

	{0, 0xcccc, 0xcccc, 0xcccc}, /* 16 white */
	{0, 0x0000, 0x0000, 0x0000}, /* 17 black */
	{0, 0x35c2, 0x35c2, 0xb332}, /* 18 blue */
	{0, 0x2a3d, 0x8ccc, 0x2a3d}, /* 19 green */
	{0, 0xc3c3, 0x3b3b, 0x3b3b}, /* 20 red */
	{0, 0xc7c7, 0x3232, 0x3232}, /* 21 light red */
	{0, 0x8000, 0x2666, 0x7fff}, /* 22 purple */
	{0, 0x6666, 0x3636, 0x1f1f}, /* 23 orange */
	{0, 0xd999, 0xa6d3, 0x4147}, /* 24 yellow */
	{0, 0x3d70, 0xcccc, 0x3d70}, /* 25 green */
	{0, 0x199a, 0x5555, 0x5555}, /* 26 aqua */
	{0, 0x2eef, 0x8ccc, 0x74df}, /* 27 light aqua */
	{0, 0x451e, 0x451e, 0xe666}, /* 28 blue */
	{0, 0xb0b0, 0x3737, 0xb0b0}, /* 29 light purple */
	{0, 0x4c4c, 0x4c4c, 0x4c4c}, /* 30 grey */
	{0, 0x9595, 0x9595, 0x9595}, /* 31 light grey */

	{0, 0xffff, 0xffff, 0xffff}, /* 32 marktext Fore (white) */
	{0, 0x3535, 0x6e6e, 0xc1c1}, /* 33 marktext Back (blue) */
	{0, 0x0000, 0x0000, 0x0000}, /* 34 foreground (black) */
	{0, 0xf0f0, 0xf0f0, 0xf0f0}, /* 35 background (white) */
	{0, 0xcccc, 0x1010, 0x1010}, /* 36 marker line (red) */

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

/* maps XChat 2.0.x colors to current */
static const int remap[] =
{
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	33,	/* 16:marktextback */
	32,	/* 17:marktextfore */
	34,	/* 18: fg */
	35,	/* 19: bg */
	37,	/* 20: newdata */
	38,	/* 21: blue */
	39,	/* 22: newmsg */
	40		/* 23: away */
};

void
palette_load (void)
{
	int i, j, l, fh, res;
	char prefname[256];
	struct stat st;
	char *cfg;
	int red, green, blue;
	int upgrade = FALSE;

	snprintf (prefname, sizeof (prefname), "%s/colors.conf", get_xdir_fs ());
	fh = open (prefname, O_RDONLY | OFLAGS);
	if (fh == -1)
	{
		snprintf (prefname, sizeof (prefname), "%s/palette.conf", get_xdir_fs ());
	   fh = open (prefname, O_RDONLY | OFLAGS);
		upgrade = TRUE;
	}

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

			if (!upgrade)
			{
				/* mIRC colors 0-31 are here */
				for (i = 0; i < 32; i++)
				{
					snprintf (prefname, sizeof prefname, "color_%d", i);
					cfg_get_color (cfg, prefname, &red, &green, &blue);
					colors[i].red = red;
					colors[i].green = green;
					colors[i].blue = blue;
				}

				/* our special colors are mapped at 256+ */
				for (i = 256, j = 32; j < MAX_COL+1; i++, j++)
				{
					snprintf (prefname, sizeof prefname, "color_%d", i);
					cfg_get_color (cfg, prefname, &red, &green, &blue);
					colors[j].red = red;
					colors[j].green = green;
					colors[j].blue = blue;
				}

			} else
			{
				/* loading 2.0.x palette.conf */
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
						colors[remap[i]].red = red;
						colors[remap[i]].green = green;
						colors[remap[i]].blue = blue;
					}
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
	int i, j, fh;
	char prefname[256];

	snprintf (prefname, sizeof (prefname), "%s/colors.conf", get_xdir_fs ());
	fh = open (prefname, O_TRUNC | O_WRONLY | O_CREAT | OFLAGS, 0600);
	if (fh != -1)
	{
		/* mIRC colors 0-31 are here */
		for (i = 0; i < 32; i++)
		{
			snprintf (prefname, sizeof prefname, "color_%d", i);
			cfg_put_color (fh, colors[i].red, colors[i].green, colors[i].blue, prefname);
		}

		/* our special colors are mapped at 256+ */
		for (i = 256, j = 32; j < MAX_COL+1; i++, j++)
		{
			snprintf (prefname, sizeof prefname, "color_%d", i);
			cfg_put_color (fh, colors[j].red, colors[j].green, colors[j].blue, prefname);
		}

		close (fh);
	}
}
