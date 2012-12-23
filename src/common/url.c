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
#include <string.h>
#include <ctype.h>
#include "hexchat.h"
#include "hexchatc.h"
#include "cfgfiles.h"
#include "fe.h"
#include "tree.h"
#include "url.h"
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

void *url_tree = NULL;
GTree *url_btree = NULL;


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
	g_tree_destroy (url_btree);
	url_btree = NULL;
}

static int
url_save_cb (char *url, FILE *fd)
{
	fprintf (fd, "%s\n", url);
	return TRUE;
}

void
url_save_tree (const char *fname, const char *mode, gboolean fullpath)
{
	FILE *fd;

	if (fullpath)
		fd = hexchat_fopen_file (fname, mode, XOF_FULLPATH);
	else
		fd = hexchat_fopen_file (fname, mode, 0);
	if (fd == NULL)
		return;

	tree_foreach (url_tree, (tree_traverse_func *)url_save_cb, fd);
	fclose (fd);
}

static void
url_save_node (char* url)
{
	FILE *fd;

	/* open <config>/url.log in append mode */
	fd = hexchat_fopen_file ("url.log", "a", 0);
	if (fd == NULL)
	{
		return;
	}

	fprintf (fd, "%s\n", url);
	fclose (fd);	
}

static int
url_find (char *urltext)
{
	return (g_tree_lookup_extended (url_btree, urltext, NULL, NULL));
}

static void
url_add (char *urltext, int len)
{
	char *data;
	int size;

	/* we don't need any URLs if we have neither URL grabbing nor URL logging enabled */
	if (!prefs.hex_url_grabber && !prefs.hex_url_logging)
	{
		return;
	}

	data = malloc (len + 1);
	if (!data)
	{
		return;
	}
	memcpy (data, urltext, len);
	data[len] = 0;

	if (data[len - 1] == '.')	/* chop trailing dot */
	{
		len--;
		data[len] = 0;
	}
	/* chop trailing ) but only if there's no counterpart */
	if (data[len - 1] == ')' && strchr (data, '(') == NULL)
	{
		data[len - 1] = 0;
	}

	if (prefs.hex_url_logging)
	{
		url_save_node (data);
	}

	/* the URL is saved already, only continue if we need the URL grabber too */
	if (!prefs.hex_url_grabber)
	{
		free (data);
		return;
	}

	if (!url_tree)
	{
		url_tree = tree_new ((tree_cmp_func *)strcasecmp, NULL);
		url_btree = g_tree_new ((GCompareFunc)strcasecmp);
	}

	if (url_find (data))
	{
		free (data);
		return;
	}

	size = tree_size (url_tree);
	/* 0 is unlimited */
	if (prefs.hex_url_grabber_limit > 0 && size >= prefs.hex_url_grabber_limit)
	{
		/* the loop is necessary to handle having the limit lowered while
		   HexChat is running */
		size -= prefs.hex_url_grabber_limit;
		for(; size > 0; size--)
		{
			char *pos;

			pos = tree_remove_at_pos (url_tree, 0);
			g_tree_remove (url_btree, pos);
			free (pos);
		}
	}

	tree_append (url_tree, data);
	g_tree_insert (url_btree, data, GINT_TO_POINTER (tree_size (url_tree) - 1));
	fe_url_add (data);
}

/* check if a word is clickable. This is called on mouse motion events, so
   keep it FAST! This new version was found to be almost 3x faster than
   2.4.4 release. */

int
url_check_word (const char *word, int len)
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
		{ D("ut2004://") },
	},
	suffix[] = {
		{ D(".org") },
		{ D(".net") },
		{ D(".com") },
		{ D(".edu") },
		{ D(".html") },
		{ D(".info") },
		{ D(".name") },
		/* Some extra common suffixes.
		foo.blah/baz.php etc should work now, rather than
		needing  http:// at the beginning. */
		{ D(".php") },
		{ D(".htm") },
		{ D(".aero") },
		{ D(".asia") },
		{ D(".biz") },
		{ D(".cat") },
		{ D(".coop") },
		{ D(".int") },
		{ D(".jobs") },
		{ D(".mobi") },
		{ D(".museum") },
		{ D(".pro") },
		{ D(".tel") },
		{ D(".travel") },
		{ D(".xxx") },
		{ D(".asp") },
		{ D(".aspx") },
		{ D(".shtml") },
		{ D(".xml") },
	};
#undef D
	const char *at, *dot;
	int i, dots;

	/* this is pretty much the same as in logmask_is_fullpath() except with length checks and .\ for portable mode */
#ifdef WIN32
	if ((len > 1 && word[0] == '\\') ||
		(len > 2 && word[0] == '.' && word[1] == '\\') ||
		(len > 2 && (((word[0] >= 'A' && word[0] <= 'Z') || (word[0] >= 'a' && word[0] <= 'z')) && word[1] == ':')))
#else
	if (len > 1 && word[0] == '/')
#endif
	{
		return WORD_PATH;
	}

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

			/* This is pretty much g_ascii_strncasecmp(). */
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
		if (word[i] == '.' && i > 0)
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

				/* This is pretty much g_ascii_strncasecmp(). */
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

/* List of IRC commands for which contents (and thus possible URLs)
 * are visible to the user.  NOTE:  Trailing blank required in each. */
static char *commands[] = {
	"NOTICE ",
	"PRIVMSG ",
	"TOPIC ",
	"332 ",		/* RPL_TOPIC */
	"372 "		/* RPL_MOTD */
};

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

void
url_check_line (char *buf, int len)
{
	char *po = buf;
	char *start;
	int i, wlen;

	/* Skip over message prefix */
	if (*po == ':')
	{
		po = strchr (po, ' ');
		if (!po)
			return;
		po++;
	}
	/* Allow only commands from the above list */
	for (i = 0; i < ARRAY_SIZE (commands); i++)
	{
		char *cmd = commands[i];
		int len = strlen (cmd);

		if (strncmp (cmd, po, len) == 0)
		{
			po += len;
			break;
		}
	}
	if (i == ARRAY_SIZE (commands))
		return;

	/* Skip past the channel name or user nick */
	po = strchr (po, ' ');
	if (!po)
		return;
	po++;

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
		case '\r':

			wlen = po - start;
			if (wlen > 2)
			{
				/* HACK! :( */
				/* This is to work around not being able to detect URLs that are at
				   the start of messages. */
				if (start[0] == ':')
				{
					start++;
					wlen--;
				}
				if (start[0] == '+' || start[0] == '-')
				{
					start++;
					wlen--;
				}

				if (wlen > 2 && url_check_word (start, wlen) == WORD_URL)
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
