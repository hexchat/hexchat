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
#include <ctype.h>
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
	snprintf (buf, 512, "%s/url.save", get_xdir_fs ());
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
url_add (char *urltext, int len)
{
	char *data = malloc (len + 1);
	if (!data)
		return;
	memcpy (data, urltext, len);
	data[len] = 0;

	if (data[len - 1] == '.')	/* chop trailing dot */
	{
		len--;
		data[len] = 0;
	}
	if (data[len - 1] == ')')	/* chop trailing ) */
		data[len - 1] = 0;

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

/* check if a word is clickable. This is called on mouse motion events, so
   keep it FAST! This new version was found to be almost 3x faster than
   2.4.4 release. */

int
url_check_word (char *word, int len)
{
#define D(x) (x), ((sizeof (x)) - 1)
	static const struct {
		const char *s;
		int len;
	}
	prefix[] = {
		{ D("irc.") },
		{ D("ftp.") },
		{ D("www.") },
		{ D("irc://") },
		{ D("ftp://") },
		{ D("http://") },
		{ D("https://") },
		{ D("file://") },
		{ D("rtsp://") },
		{ D("gopher://") },
	},
	suffix[] = {
		{ D(".org") },
		{ D(".net") },
		{ D(".com") },
		{ D(".edu") },
		{ D(".html") },
		{ D(".info") },
		{ D(".name") },
	};
#undef D
	const char *at, *dot;
	int i, dots;

	if (len > 1 && word[1] == '#' && strchr("@+^%*#", word[0]))
		return WORD_CHANNEL;

	if ((word[0] == '#' || word[0] == '&') && word[1] != '#' && word[1] != 0)
		return WORD_CHANNEL;

	for (i = 0; i < G_N_ELEMENTS(prefix); i++)
	{
		int l;

		l = prefix[i].len;
		if (len > l)
		{
			int j;

			/* This is pretty much strncasecmp(). */
			for (j = 0; j < l; j++)
			{
				unsigned char c = word[j];
				if (tolower(c) != prefix[i].s[j])
					break;
			}
			if (j == l)
				return WORD_URL;
		}
	}

	at = strchr (word, '@');	  /* check for email addy */
	dot = strrchr (word, '.');
	if (at && dot)
	{
		if (at < dot)
		{
			if (strchr (word, '*'))
				return WORD_HOST;
			else
				return WORD_EMAIL;
		}
	}
 
	/* check if it's an IP number */
	dots = 0;
	for (i = 0; i < len; i++)
	{
		if (word[i] == '.' && i > 1)
			dots++;	/* allow 127.0.0.1:80 */
		else if (!isdigit ((unsigned char) word[i]) && word[i] != ':')
		{
			dots = 0;
			break;
		}
	}
	if (dots == 3)
		return WORD_HOST;

	if (len > 5)
	{
		for (i = 0; i < G_N_ELEMENTS(suffix); i++)
		{
			int l;

			l = suffix[i].len;
			if (len > l)
			{
				const unsigned char *p = &word[len - l];
				int j;

				/* This is pretty much strncasecmp(). */
				for (j = 0; j < l; j++)
				{
					if (tolower(p[j]) != suffix[i].s[j])
						break;
				}
				if (j == l)
					return WORD_HOST;
			}
		}

		if (word[len - 3] == '.' &&
			 isalpha ((unsigned char) word[len - 2]) &&
				isalpha ((unsigned char) word[len - 1]))
			return WORD_HOST;
	}

	return 0;
}

void
url_check_line (char *buf, int len)
{
	char *po = buf;
	char *start;
	int wlen;

	if (buf[0] == ':' && buf[1] != 0)
		po++;

	start = po;

	/* check each "word" (space separated) */
	while (1)
	{
		switch (po[0])
		{
		case 0:
		case ' ':
			wlen = po - start;
			if (wlen > 2)
			{
				if (url_check_word (start, wlen) == WORD_URL)
				{
					url_add (start, wlen);
				}
			}
			if (po[0] == 0)
				return;
			po++;
			start = po;
			break;

		default:
			po++;
		}
	}
}
