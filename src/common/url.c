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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xchat.h"
#include "cfgfiles.h"
#include "fe.h"
#include "tree.h"
#include "url.h"
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

void *url_tree = NULL;


static int
url_free (char *url, void *data)
{
	free (url);
	return TRUE;
}

void
url_clear (void)
{
	tree_foreach (url_tree, (tree_traverse_func *)url_free, NULL);
	tree_destroy (url_tree);
	url_tree = NULL;
}

static int
url_save_cb (char *url, FILE *fd)
{
	fprintf (fd, "%s\n", url);
	return TRUE;
}

void
url_save (const char *fname, const char *mode)
{
	FILE *fd;

	fd = fopen (fname, mode);
	if (fd == NULL)
		return;

	tree_foreach (url_tree, (tree_traverse_func *)url_save_cb, fd);
	fclose (fd);
}

void
url_autosave (void)
{
	char *buf;

	buf = malloc (512);
	snprintf (buf, 512, "%s/url.save", get_xdir ());
	url_save (buf, "a");
	free (buf);
}

static int
url_find (char *urltext)
{
	int pos;

	if (tree_find (url_tree, urltext, (tree_cmp_func *)strcasecmp, NULL, &pos))
		return 1;
	return 0;
}

static void
url_add (char *urltext)
{
	char *data = strdup (urltext);
	if (!data)
		return;

	if (data[strlen (data) - 1] == '.')	/* chop trailing dot */
		data[strlen (data) - 1] = 0;

	if (url_find (data))
	{
		free (data);
		return;
	}

	if (!url_tree)
		url_tree = tree_new ((tree_cmp_func *)strcasecmp, NULL);

	tree_insert (url_tree, data);
	fe_url_add (data);
}

void
url_check (char *buf)
{
	char *sp, *po = buf + 1;
	unsigned char t;

	if (buf[0] == ':' && buf[1] != 0)
		po++;

	while (po[0])
	{
		if (strncasecmp (po, "http:", 5) == 0 ||
			 strncasecmp (po, "www.", 4) == 0 ||
			 strncasecmp (po, "ftp.", 4) == 0 ||
			 strncasecmp (po, "ftp:", 4) == 0 ||
			 strncasecmp (po, "irc://", 6) == 0 ||
			 strncasecmp (po, "irc.", 4) == 0)
			break;
		po++;
	}

	if (po[0])
	{
		sp = strchr (po, ' ');
		if (sp)
		{
			t = sp[0];
			sp[0] = 0;
			url_add (po);
			sp[0] = t;
		} else
			url_add (po);
	}
}
