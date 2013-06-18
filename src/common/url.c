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
static int do_an_re (const char *word, int *start, int *end, int *type);
static GRegex *re_url (void);
static GRegex *re_host (void);
static GRegex *re_host6 (void);
static GRegex *re_email (void);
static GRegex *re_nick (void);
static GRegex *re_channel (void);
static GRegex *re_path (void);


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

static int laststart = 0;
static int lastend = 0;
static int lasttype = 0;

static int
strchrs (char c, char *s)
{
	while (*s)
		if (c == *s++)
			return TRUE;
	return FALSE;
}

#define NICKPRE "~+!@%%&"
int
url_check_word (const char *word)
{
	laststart = lastend = lasttype = 0;
	if (do_an_re (word, &laststart, &lastend, &lasttype))
	{
		switch (lasttype)
		{
			char *str;

			case WORD_NICK:
				if (strchrs (word[laststart], NICKPRE))
					laststart++;
				str = g_strndup (&word[laststart], lastend - laststart);
				if (!userlist_find (current_sess, str))
					lasttype = 0;
				g_free (str);
				return lasttype;
			case WORD_EMAIL:
				if (!isalnum (word[laststart]))
					laststart++;
				/* Fall through */
			case WORD_URL:
			case WORD_HOST:
			case WORD_HOST6:
			case WORD_CHANNEL:
			case WORD_PATH:
				return lasttype;
			default:
				return 0;	/* Should not occur */
		}
	}
	else
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
	GRegex *re(void);
	GMatchInfo *gmi;
	char *po = buf;
	int i;

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

	g_regex_match(re_url(), po, 0, &gmi);
	while (g_match_info_matches(gmi))
	{
		int start, end;

		g_match_info_fetch_pos(gmi, 0, &start, &end);
		while (end > start && (po[end - 1] == '\r' || po[end - 1] == '\n'))
			end--;
		if (g_strstr_len (po + start, end - start, "://"))
			url_add(po + start, end - start);
		g_match_info_next(gmi, NULL);
	}
	g_match_info_free(gmi);
}

int
url_last (int *lstart, int *lend)
{
	*lstart = laststart;
	*lend = lastend;
	return lasttype;
}

static int
do_an_re(const char *word, int *start, int *end, int *type)
{
	typedef struct func_s {
		GRegex *(*fn)(void);
		int type;
	} func_t;
	func_t funcs[] =
	{
		{ re_url, WORD_URL },
		{ re_email, WORD_EMAIL },
		{ re_channel, WORD_CHANNEL },
		{ re_host6, WORD_HOST6 },
		{ re_host, WORD_HOST },
		{ re_path, WORD_PATH },
		{ re_nick, WORD_NICK }
	};

	GMatchInfo *gmi;
	int k;

	for (k = 0; k < sizeof funcs / sizeof (func_t); k++)
	{
		g_regex_match (funcs[k].fn(), word, 0, &gmi);
		if (!g_match_info_matches (gmi))
		{
			g_match_info_free (gmi);
			continue;
		}
		while (g_match_info_matches (gmi))
		{
			g_match_info_fetch_pos (gmi, 0, start, end);
			g_match_info_next (gmi, NULL);
		}
		g_match_info_free (gmi);
		*type = funcs[k].type;
		return TRUE;
	}

	return FALSE;
}

/*	Miscellaneous description --- */
#define DOMAIN "[a-z0-9][-a-z0-9]*(\\.[-a-z0-9]+)*\\."
#define TLD "[a-z][-a-z0-9]*[a-z]"
#define IPADDR "[0-9]{1,3}(\\.[0-9]{1,3}){3}"
#define IPV6GROUP "([0-9a-f]{0,4})"
#define IPV6ADDR "((" IPV6GROUP "(:" IPV6GROUP "){7})"	\
	         "|(" IPV6GROUP "(:" IPV6GROUP ")*:(:" IPV6GROUP ")+))" /* with :: compression */
#define HOST "(" DOMAIN TLD "|" IPADDR "|" IPV6ADDR ")"
/* In urls the IPv6 must be enclosed in square brackets */
#define HOST_URL "(" DOMAIN TLD "|" IPADDR "|" "\\[" IPV6ADDR "\\]" ")"
#define PORT "(:[1-9][0-9]{0,4})"
#define OPT_PORT "(" PORT ")?"

GRegex *
make_re (char *grist)
{
	GRegex *ret;
	GError *err = NULL;

	ret = g_regex_new (grist, G_REGEX_CASELESS | G_REGEX_OPTIMIZE, 0, &err);
	g_free (grist);
	return ret;
}

/*	HOST description --- */
/* (see miscellaneous above) */
static GRegex *
re_host (void)
{
	static GRegex *host_ret;
	char *grist;

	if (host_ret) return host_ret;

	grist = g_strdup_printf (
		"("
			"(" HOST_URL PORT ")|(" HOST ")"
		")"
	);
	host_ret = make_re (grist);
	return host_ret;
}

static GRegex *
re_host6 (void)
{
	static GRegex *host6_ret;
	char *grist;

	if (host6_ret) return host6_ret;

	grist = g_strdup_printf (
		"("
			"(" IPV6ADDR ")|(" "\\[" IPV6ADDR "\\]" PORT ")"
		")"
	);

	host6_ret = make_re (grist);

	return host6_ret;
}

/*	URL description --- */
#define SCHEME "(%s)"
#define LPAR "\\("
#define RPAR "\\)"
#define NOPARENS "[^() \t]*"
#define PATH								\
	"("								\
	   "(" LPAR NOPARENS RPAR ")"					\
	   "|"								\
	   "(" NOPARENS ")"						\
	")*"	/* Zero or more occurrences of either of these */	\
	"(?<![.,?!\\]])"	/* Not allowed to end with these */
#define USERINFO "([-a-z0-9._~%]+(:[-a-z0-9._~%]*)?@)"

/* Flags used to describe URIs (RFC 3986)
 *
 * Bellow is an example of what the flags match.
 *
 * URI_AUTHORITY - http://example.org:80/foo/bar
 *                      ^^^^^^^^^^^^^^^^
 * URI_USERINFO/URI_OPT_USERINFO - http://user@example.org:80/foo/bar
 *                                        ^^^^^
 * URI_PATH - http://example.org:80/foo/bar
 *                                 ^^^^^^^^
 */
#define URI_AUTHORITY     (1 << 0)
#define URI_OPT_USERINFO  (1 << 1)
#define URI_USERINFO      (1 << 2)
#define URI_PATH          (1 << 3)

struct
{
	const char *scheme;    /* scheme name. e.g. http */
	const char *path_sep;  /* string that begins the path */
	int flags;             /* see above (flag macros) */
} uri[] = {
	{ "irc",       "/", URI_PATH },
	{ "ircs",      "/", URI_PATH },
	{ "rtsp",      "/", URI_AUTHORITY | URI_PATH },
	{ "feed",      "/", URI_AUTHORITY | URI_PATH },
	{ "teamspeak", "?", URI_AUTHORITY | URI_PATH },
	{ "ftp",       "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "sftp",      "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "ftps",      "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "http",      "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "https",     "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "cvs",       "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "svn",       "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "git",       "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "bzr",       "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "rsync",     "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "mumble",    "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "ventrilo",  "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "xmpp",      "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "h323",      ";", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "imap",      "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "pop",       "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "nfs",       "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "smb",       "/", URI_AUTHORITY | URI_OPT_USERINFO | URI_PATH },
	{ "ssh",       "",  URI_AUTHORITY | URI_OPT_USERINFO },
	{ "sip",       "",  URI_AUTHORITY | URI_USERINFO },
	{ "sips",      "",  URI_AUTHORITY | URI_USERINFO },
	{ "magnet",    "?", URI_PATH },
	{ "mailto",    "",  URI_PATH },
	{ "bitcoin",   "",  URI_PATH },
	{ "gtalk",     "",  URI_PATH },
	{ "steam",     "",  URI_PATH },
	{ "file",      "/", URI_PATH },
	{ "callto",    "",  URI_PATH },
	{ "skype",     "",  URI_PATH },
	{ "geo",       "",  URI_PATH },
	{ NULL,        "",  0}
};

static GRegex *
re_url (void)
{
	static GRegex *url_ret = NULL;
	GString *grist_gstr;
	char *grist;
	int i;

	if (url_ret) return url_ret;

	grist_gstr = g_string_new (NULL);

	/* Add regex "host/path", representing a "schemeless" url */
	g_string_append (grist_gstr, "(" HOST_URL OPT_PORT "/" "(" PATH ")?" ")");

	for (i = 0; uri[i].scheme; i++)
	{
		g_string_append (grist_gstr, "|(");
		g_string_append_printf (grist_gstr, "%s:", uri[i].scheme);

		if (uri[i].flags & URI_AUTHORITY)
			g_string_append (grist_gstr, "//");

		if (uri[i].flags & URI_USERINFO)
			g_string_append (grist_gstr, USERINFO);
		else if (uri[i].flags & URI_OPT_USERINFO)
			g_string_append (grist_gstr, USERINFO "?");

		if (uri[i].flags & URI_AUTHORITY)
			g_string_append (grist_gstr, HOST_URL OPT_PORT);
		
		if (uri[i].flags & URI_PATH)
		{
			char *sep_escaped;
			
			sep_escaped = g_regex_escape_string (uri[i].path_sep, 
							     strlen(uri[i].path_sep));

			g_string_append_printf(grist_gstr, "(" "%s" PATH ")?",
					       sep_escaped);

			g_free(sep_escaped);
		}

		g_string_append(grist_gstr, ")");
	}

	grist = g_string_free (grist_gstr, FALSE);

	url_ret = make_re (grist);

	return url_ret;
}

/*	EMAIL description --- */
#define EMAIL "[a-z][-_a-z0-9]+@" "(" HOST_URL ")"

static GRegex *
re_email (void)
{
	static GRegex *email_ret;
	char *grist;

	if (email_ret) return email_ret;

	grist = g_strdup_printf (
		"("
			EMAIL
		")"
	);
	email_ret = make_re (grist);
	return email_ret;
}

/*	NICK description --- */
/* For NICKPRE see before url_check_word() */
#define NICKHYP	"-"
#define NICKLET "a-z"
#define NICKDIG "0-9"
/*	Note for NICKSPE:  \\\\ boils down to a single \ */
#define NICKSPE	"\\[\\]\\\\`_^{|}"
#if 0
#define NICK0 "[" NICKPRE "]?[" NICKLET NICKSPE "]"
#else
/* Allow violation of rfc 2812 by allowing digit as first char */
/* Rationale is that do_an_re() above will anyway look up what */
/* we find, and that WORD_NICK is the last item in the array */
/* that do_an_re() runs through. */
#define NICK0 "[" NICKPRE "]?[" NICKLET NICKDIG NICKSPE "]"
#endif
#define NICK1 "[" NICKHYP NICKLET NICKDIG NICKSPE "]*"
#define NICK	NICK0 NICK1

static GRegex *
re_nick (void)
{
	static GRegex *nick_ret;
	char *grist;

	if (nick_ret) return nick_ret;

	grist = g_strdup_printf (
		"("
			NICK
		")"
	);
	nick_ret = make_re (grist);
	return nick_ret;
}

/*	CHANNEL description --- */
#define CHANNEL "#[^ \t\a,:]+"

static GRegex *
re_channel (void)
{
	static GRegex *channel_ret;
	char *grist;

	if (channel_ret) return channel_ret;

	grist = g_strdup_printf (
		"("
			CHANNEL
		")"
	);
	channel_ret = make_re (grist);
	return channel_ret;
}

/*	PATH description --- */
#ifdef WIN32
/* Windows path can be .\ ..\ or C: D: etc */
#define FS_PATH "^(\\.{1,2}\\\\|[a-z]:).*"
#else
/* Linux path can be / or ./ or ../ etc */
#define FS_PATH "^(/|\\./|\\.\\./).*"
#endif

static GRegex *
re_path (void)
{
	static GRegex *path_ret;
	char *grist;

	if (path_ret) return path_ret;

	grist = g_strdup_printf (
		"("
			FS_PATH
		")"
	);
	path_ret = make_re (grist);
	return path_ret;
}
