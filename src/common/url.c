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
#include "util.h"
#include "url.h"

GSList *url_list = 0;

void
url_clear (void)
{
	while (url_list)
	{
		free (url_list->data);
		url_list = g_slist_remove (url_list, url_list->data);
	}
}

void
url_save (const char *fname, const char *mode)
{
	FILE *fd;
	GSList *list;

	fd = fopen (fname, mode);
	if (fd == NULL)
		return;

	list = url_list;

	while (list)
	{
		fprintf (fd, "%s\n", (char *) list->data);
		list = list->next;
	}

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
	GSList *list = url_list;
	while (list)
	{
		if (!strcasecmp (urltext, (char *) list->data))
			return 1;
		list = list->next;
	}
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

	url_list = g_slist_prepend (url_list, data);
	fe_url_add (data);
}

void
url_check (char *buf)
{
	char t, *po, *urltext = nocasestrstr (buf, "http:");
	if (!urltext)
		urltext = nocasestrstr (buf, "www.");
	if (!urltext)
		urltext = nocasestrstr (buf, "ftp.");
	if (!urltext)
		urltext = nocasestrstr (buf, "ftp:");
	if (!urltext)
		urltext = nocasestrstr (buf, "irc://");
	if (!urltext)
		urltext = nocasestrstr (buf, "irc.");
	if (urltext)
	{
		po = strchr (urltext, ' ');
		if (po)
		{
			t = *po;
			*po = 0;
			url_add (urltext);
			*po = t;
		} else
			url_add (urltext);
	}
}
