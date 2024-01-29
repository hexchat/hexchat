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
static gboolean regex_match (const GRegex *re, const char *word,
							 int *start, int *end);
static const GRegex *re_url (void);
static const GRegex *re_url_no_scheme (void);
static const GRegex *re_host (void);
static const GRegex *re_host6 (void);
static const GRegex *re_email (void);
static const GRegex *re_nick (void);
static const GRegex *re_channel (void);
static const GRegex *re_path (void);
static gboolean match_nick (const char *word, int *start, int *end);
static gboolean match_channel (const char *word, int *start, int *end);
static gboolean match_email (const char *word, int *start, int *end);
static gboolean match_url (const char *word, int *start, int *end);
static gboolean match_host (const char *word, int *start, int *end);
static gboolean match_host6 (const char *word, int *start, int *end);
static gboolean match_path (const char *word, int *start, int *end);

static int
url_free (char *url, void *data)
{
	g_free (url);
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

	data = g_strndup (urltext, len);

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
		g_free (data);
		return;
	}

	if (!url_tree)
	{
		url_tree = tree_new ((tree_cmp_func *)strcasecmp, NULL);
		url_btree = g_tree_new ((GCompareFunc)strcasecmp);
	}

	if (url_find (data))
	{
		g_free (data);
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
			g_free (pos);
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

#define NICKPRE "~+!@%&"
#define CHANPRE "#&!+"

int
url_check_word (const char *word)
{
	struct {
		gboolean (*match) (const char *word, int *start, int *end);
		int type;
	} m[] = {
	   { match_url,     WORD_URL },
	   { match_email,   WORD_EMAIL },
	   { match_channel, WORD_CHANNEL },
	   { match_host6,   WORD_HOST6 },
	   { match_host,    WORD_HOST },
	   { match_path,    WORD_PATH },
	   { match_nick,    WORD_NICK },
	   { NULL,          0}
	};
	int i;

	laststart = lastend = lasttype = 0;

	for (i = 0; m[i].match; i++)
		if (m[i].match (word, &laststart, &lastend))
		{
			lasttype = m[i].type;
			return lasttype;
		}

	return 0;
}

static gboolean
match_nick (const char *word, int *start, int *end)
{
	const server *serv = current_sess->server;
	const char *nick_prefixes = serv ? serv->nick_prefixes : NICKPRE;
	char *str;

	if (!regex_match (re_nick (), word, start, end))
		return FALSE;

	/* ignore matches with prefixes that the server doesn't use */
	if (strchr (NICKPRE, word[*start])
		&& !strchr (nick_prefixes, word[*start]))
		return FALSE;
	
	/* nick prefix is not part of the matched word */
	if (strchr (nick_prefixes, word[*start]))
		(*start)++;

	str = g_strndup (&word[*start], *end - *start);

	if (!userlist_find (current_sess, str))
	{
		g_free (str);
		return FALSE;
	}

	g_free (str);

	return TRUE;
}

static gboolean
match_channel (const char *word, int *start, int *end)
{
	const server *serv = current_sess->server;
	const char *chan_prefixes = serv ? serv->chantypes : CHANPRE;
	const char *nick_prefixes = serv ? serv->nick_prefixes : NICKPRE;

	if (!regex_match (re_channel (), word, start, end))
		return FALSE;

	/* Check for +#channel (for example whois output) */
	if (strchr (nick_prefixes, word[*start]) != NULL
		 && strchr (chan_prefixes, word[*start + 1]) != NULL)
	{
		(*start)++;
		return TRUE;
	}
	/* Or just #channel */
	else if (strchr (chan_prefixes, word[*start]) != NULL)
		return TRUE;
	
	return FALSE;
}

static gboolean
match_email (const char *word, int *start, int *end)
{
	return regex_match (re_email (), word, start, end);
}

static gboolean
match_url (const char *word, int *start, int *end)
{
	if (regex_match (re_url (), word, start, end))
		return TRUE;

	return regex_match (re_url_no_scheme (), word, start, end);
}

static gboolean
match_host (const char *word, int *start, int *end)
{
	return regex_match (re_host (), word, start, end);
}

static gboolean
match_host6 (const char *word, int *start, int *end)
{
	return regex_match (re_host6 (), word, start, end);
}

static gboolean
match_path (const char *word, int *start, int *end)
{
	return regex_match (re_path (), word, start, end);
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
url_check_line (char *buf)
{
	GRegex *re(void);
	GMatchInfo *gmi;
	char *po = buf;
	size_t i;

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

static gboolean
regex_match (const GRegex *re, const char *word, int *start, int *end)
{
	GMatchInfo *gmi;

	g_regex_match (re, word, 0, &gmi);
	
	if (!g_match_info_matches (gmi))
	{
		g_match_info_free (gmi);
		return FALSE;
	}
	
	while (g_match_info_matches (gmi))
	{
		g_match_info_fetch_pos (gmi, 0, start, end);
		g_match_info_next (gmi, NULL);
	}
	
	g_match_info_free (gmi);
	
	return TRUE;
}

/*	Miscellaneous description --- */
#define DOMAIN "[_\\pL\\pN\\pS][-_\\pL\\pN\\pS]*(\\.[-_\\pL\\pN\\pS]+)*"
#define TLD "\\.[\\pL][-\\pL\\pN]*[\\pL]"
#define IPADDR "[0-9]{1,3}(\\.[0-9]{1,3}){3}"
#define IPV6GROUP "([0-9a-f]{0,4})"
#define IPV6ADDR "((" IPV6GROUP "(:" IPV6GROUP "){7})"	\
	         "|(" IPV6GROUP "(:" IPV6GROUP ")*:(:" IPV6GROUP ")+))" /* with :: compression */
#define HOST "(" DOMAIN TLD "|" IPADDR "|" IPV6ADDR ")"
/* In urls the IPv6 must be enclosed in square brackets */
#define HOST_URL "(" DOMAIN TLD "|" IPADDR "|" "\\[" IPV6ADDR "\\]" ")"
#define HOST_URL_OPT_TLD "(" DOMAIN "|" HOST_URL ")"
#define PORT "(:[1-9][0-9]{0,4})"
#define OPT_PORT "(" PORT ")?"

static GRegex *
make_re (const char *grist)
{
	GRegex *ret;
	GError *err = NULL;

	ret = g_regex_new (grist, G_REGEX_CASELESS | G_REGEX_OPTIMIZE, 0, &err);

	return ret;
}

/*	HOST description --- */
/* (see miscellaneous above) */
static const GRegex *
re_host (void)
{
	static GRegex *host_ret;

	if (host_ret) return host_ret;

	host_ret = make_re ("(" "(" HOST_URL PORT ")|(" HOST ")" ")");
	
	return host_ret;
}

static const GRegex *
re_host6 (void)
{
	static GRegex *host6_ret;

	if (host6_ret) return host6_ret;

	host6_ret = make_re ("(" "(" IPV6ADDR ")|(" "\\[" IPV6ADDR "\\]" PORT ")" ")");

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
	{ "gopher",    "/", URI_AUTHORITY | URI_PATH },
	{ "gemini",    "/", URI_AUTHORITY | URI_PATH },
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
	{ "spotify",   "",  URI_PATH },
	{ "lastfm",    "/", URI_PATH },
	{ "xfire",     "",  URI_PATH },
	{ "ts3server", "",  URI_PATH },
	{ NULL,        "",  0}
};

static const GRegex *
re_url_no_scheme (void)
{
	static GRegex *url_ret = NULL;

	if (url_ret) return url_ret;

	url_ret = make_re ("(" HOST_URL OPT_PORT "/" "(" PATH ")?" ")");

	return url_ret;
}

static const GRegex *
re_url (void)
{
	static GRegex *url_ret = NULL;
	GString *grist_gstr;
	char *grist;
	int i;

	if (url_ret) return url_ret;

	grist_gstr = g_string_new (NULL);

	for (i = 0; uri[i].scheme; i++)
	{
		if (i)
			g_string_append (grist_gstr, "|");

		g_string_append (grist_gstr, "(");
		g_string_append_printf (grist_gstr, "%s:", uri[i].scheme);

		if (uri[i].flags & URI_AUTHORITY)
			g_string_append (grist_gstr, "//");

		if (uri[i].flags & URI_USERINFO)
			g_string_append (grist_gstr, USERINFO);
		else if (uri[i].flags & URI_OPT_USERINFO)
			g_string_append (grist_gstr, USERINFO "?");

		if (uri[i].flags & URI_AUTHORITY)
			g_string_append (grist_gstr, HOST_URL_OPT_TLD OPT_PORT);
		
		if (uri[i].flags & URI_PATH)
		{
			char *sep_escaped = g_regex_escape_string (uri[i].path_sep, strlen(uri[i].path_sep));

			g_string_append_printf (grist_gstr, "(" "%s" PATH ")?", sep_escaped);

			g_free (sep_escaped);
		}

		g_string_append (grist_gstr, ")");
	}

	grist = g_string_free (grist_gstr, FALSE);

	url_ret = make_re (grist);
	g_free (grist);

	return url_ret;
}

/*	EMAIL description --- */
#define EMAIL "[a-z0-9][._%+-a-z0-9]+@" "(" HOST_URL ")"

static const GRegex *
re_email (void)
{
	static GRegex *email_ret;

	if (email_ret) return email_ret;

	email_ret = make_re ("(" EMAIL ")");

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
#define NICK0 "^[" NICKPRE "]?[" NICKLET NICKDIG NICKSPE "]"
#endif
#define NICK1 "[" NICKHYP NICKLET NICKDIG NICKSPE "]*"
#define NICK	NICK0 NICK1

static const GRegex *
re_nick (void)
{
	static GRegex *nick_ret;

	if (nick_ret) return nick_ret;

	nick_ret = make_re ("(" NICK ")");

	return nick_ret;
}

/*	CHANNEL description --- */
#define CHANNEL "[" CHANPRE "][^ \t\a,]+(?:,[" CHANPRE "][^ \t\a,]+)*"

static const GRegex *
re_channel (void)
{
	static GRegex *channel_ret;

	if (channel_ret) return channel_ret;

	channel_ret = make_re ("(" CHANNEL ")");

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

static const GRegex *
re_path (void)
{
	static GRegex *path_ret;

	if (path_ret) return path_ret;

	path_ret = make_re ("(" FS_PATH ")");

	return path_ret;
}
