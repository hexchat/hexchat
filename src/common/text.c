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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "xchat.h"
#include "cfgfiles.h"
#include "plugin.h"
#include "fe.h"
#include "util.h"
#include "outbound.h"
#include "xchatc.h"
#include "text.h"


struct pevt_stage1
{
	int len;
	char *data;
	struct pevt_stage1 *next;
};


#ifdef USE_HEBREW

static inline int iswhitespace(const char ch)
{
	return (ch == ' ' ||  ch == 8);
}

static inline int isheb(const char ch)
{
	return (((unsigned char)ch) >= 0xE0  &&  ((unsigned char)ch) <= 0xFA);
}

static inline int iseng(const char ch)
{
	return ((ch >= 'A'  &&  ch <= 'z')  ||  (ch >= 'a'  &&  ch <= 'z'));
}

static char *strhebpatch(char *dest, const char *src)
{
	short int mode = 0, imode;
	const char *hmark = NULL, *lmark, *nmark, *nlmark;
	char ch;

	if (!dest || !src)
		return NULL;
	
	for (;;)
	{
		if (mode == 0)
		{
			if (isheb(*src))
			{
				hmark = src;
				mode = 1;
			}
			else
				*dest++ = *src;
		}
		else if (mode == 1)
		{
			if (*src == 0  ||  iseng(*src))
			{
				lmark = src-1;
				while (!isheb(*lmark)) lmark--;
				src = lmark;
				imode = 0;
				nmark = NULL;

				while (lmark >= hmark)
				{
					ch = *lmark;

					if (imode == 0)
						switch (ch)
						{
							case '(': ch = ')'; break;
							case ')': ch = '('; break;
							case '{': ch = '}'; break;
							case '}': ch = '{'; break;
							case '[': ch = ']'; break;
							case ']': ch = '['; break;
						}

					if (imode == 0)
					{
						if (isdigit(ch))
						{
							imode = 1;
							nmark = lmark;			
						}
						else 
							*dest++ = ch;
					}
					else
  						if (imode == 1  &&  (isheb(ch) || iswhitespace(ch)))
						{
							nlmark = lmark+1;
							while (nlmark <= nmark)
								*dest++ = *nlmark++;
							imode = 0;
							lmark++;
						}
					lmark--;
				}
				
				hmark = NULL;
				mode = 0;
			}
		}
		if (!*src++)
			break;
	}
	return dest;
}

#endif /* !USE_HEBREW */

/* Make ip:port urls clikable (127.0.0.1:80 etc)
*  patch by Alex <alex@cosinus.org> & dobler <dobler@barrysworld.com>
*/

static int
q3link (char *word)
{
	char *s;
	int i;
	int d = 0;

	if ((s = strchr (word,':')) != NULL)
	{
		for (i = 0; i < (int) ((unsigned long)s - (unsigned long)word); i++)
		{
			if (word[i] == '.')
				d++;
			else if (!isdigit (word[i]))
			{
				d = 0;
				break;
			}
		}

		if (d == 3)
		{
			s++;
			d = 0;

			while (*s != 0)
			{
				if (!isdigit (*s++))
					return (0);
				d++;
			}
			if (d > 0)
				return (1);
		}
	}
	return (0);
}

/* check if a word is clickable */

int
text_word_check (char *word)
{
	session *sess = current_sess;
	char *at, *dot;
	int i, dots;
	int len = strlen (word);

	if(q3link(word))
		return WORD_URL;

	if ((word[0] == '@' || word[0] == '+') && word[1] == '#')
		return WORD_CHANNEL;

	if ((word[0] == '#' || word[0] == '&') && word[1] != '#' && word[1] != 0)
		return WORD_CHANNEL;

	if (!strncasecmp (word, "irc.", 4) && word[4] != '.')
		return WORD_URL;

	if (!strncasecmp (word, "ftp.", 4) && word[4] != '.')
		return WORD_URL;

	if (!strncasecmp (word, "www.", 4) && word[4] != '.')
		return WORD_URL;

	if (!strncasecmp (word, "irc://", 6) && word[6] != 0)
		return WORD_URL;

	if (!strncasecmp (word, "ftp://", 6) && word[6] != 0)
		return WORD_URL;

	if (!strncasecmp (word, "http://", 7) && word[7] != 0)
		return WORD_URL;

	if (!strncasecmp (word, "file://", 7) && word[7] != 0)
		return WORD_URL;

	if (!strncasecmp (word, "https://", 8) && word[8] != 0)
		return WORD_URL;

	if (!strncasecmp (word, "gopher://", 9) && word[9] != 0)
		return WORD_URL;

	if (( (word[0]=='@' || word[0]=='+') && find_name (sess, word+1)) || find_name (sess, word))
		return WORD_NICK;

	at = strchr (word, '@');	  /* check for email addy */
	dot = strrchr (word, '.');
	if (at && dot)
	{
		if ((unsigned long) at < (unsigned long) dot)
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
		if (word[i] == '.')
			dots++;
		else if (!isdigit (word[i]))
		{
			dots = 0;
			break;
		}
	}
	if (dots == 3)
		return WORD_HOST;

	if (!strncasecmp (word + len - 5, ".html", 5))
		return WORD_HOST;

	if (!strncasecmp (word + len - 4, ".org", 4))
		return WORD_HOST;

	if (!strncasecmp (word + len - 4, ".net", 4))
		return WORD_HOST;

	if (!strncasecmp (word + len - 4, ".com", 4))
		return WORD_HOST;

	if (!strncasecmp (word + len - 4, ".edu", 4))
		return WORD_HOST;

	if (len > 5)
	{
		if (word[len - 3] == '.' &&
			 isalpha (word[len - 2]) && isalpha (word[len - 1]))
			return WORD_HOST;
	}

	if (sess->type == SESS_DIALOG)
		return WORD_DIALOG;

	return 0;
}

void
log_close (session *sess)
{
	char obuf[512];
	time_t currenttime;

	if (sess->logfd != -1)
	{
		currenttime = time (NULL);
		write (sess->logfd, obuf,
			 snprintf (obuf, sizeof (obuf) - 1, _("**** ENDING LOGGING AT %s\n"),
						  ctime (&currenttime)));
		close (sess->logfd);
		sess->logfd = -1;
	}
}

static void
log_create_filename (char *buf, char *servname, char *channame, char *netname)
{
	char fn[256];
	char *tmp, *dir, *sep;
	int pathlen=510, c=0, mbl;

	if (!rfc_casecmp (channame, servname))
	{
		channame = g_strdup ("server");
	} else
	{
		sep = tmp = strdup (channame);
		while (*tmp)
		{
			mbl = g_utf8_skip[((unsigned char *)tmp)[0]];
			if (mbl == 1)
			{
#ifndef WIN32
				*tmp = rfc_tolower (*tmp);
				if (*tmp == '/')
#else
				/* win32 can't handle filenames with the '|' character */
				if (*tmp == '|')
#endif
					*tmp = '_';
			}
			tmp += mbl;
		}

		channame = g_filename_from_utf8 (sep, -1, 0, 0, 0);
		free (sep);
	}

	snprintf (buf, 512, "%s/xchatlogs", get_xdir ());
	if (access (buf, F_OK) != 0)
#ifdef WIN32
		mkdir (buf);
#else
		mkdir (buf, S_IRUSR | S_IWUSR | S_IXUSR);
#endif
	auto_insert (fn, sizeof (fn), prefs.logmask, NULL, NULL, "", channame, "", "", netname, servname);
	g_free (channame);

	snprintf (buf, 512, "%s/xchatlogs/%s", get_xdir (), fn);

 	/* The following code handles subdirectories in logpath and creates
 	 * them, if they don't exist. Useful with logmasks like "%c/%y.log" in
 	 * ~/.xchat/xchat.conf.
 	 *                       -- patch by tvk <tvk@von-koch.com>
 	 */

	if (access (buf, F_OK) != 0)
	{
		snprintf (buf, 512, "%s/xchatlogs/", get_xdir ());
		pathlen -= strlen(buf);
 
		/* how many sub-directories do we have? */
		sep = fn;
		while (*sep++) if (*sep=='/') c++;
			
		if (c) {
 			/* create each sub-directory */
			sep = strdup(fn);
			dir = strtok(sep, "/");
		
			while (c-- && dir != NULL) {
				strncat (buf, dir, pathlen);
				strcat (buf, "/");
				pathlen -= strlen(dir);
 		
				if (access (buf, F_OK) != 0)
#ifdef WIN32	
					mkdir (buf);
#else	
					mkdir (buf, S_IRUSR | S_IWUSR | S_IXUSR);
#endif	
				dir = strtok(NULL, "/");
			}

			/* append the filename */
			strncat (buf, strrchr(fn, '/')+1, pathlen);
			free (sep);
		} else {
			strncat (buf, fn, pathlen);
		}

	}
}

static int
log_open_file (char *servname, char *channame, char *netname)
{
	char buf[512];
	int fd;
	time_t currenttime;

	if (!netname)
		netname = "NETWORK";

	log_create_filename (buf, servname, channame, netname);
#ifdef WIN32
	fd = open (buf, O_CREAT | O_APPEND | O_WRONLY, S_IREAD|S_IWRITE);
#else
	fd = open (buf, O_CREAT | O_APPEND | O_WRONLY, 0644);
#endif
	if (fd == -1)
		return -1;
	currenttime = time (NULL);
	write (fd, buf,
			 snprintf (buf, 510, _("**** BEGIN LOGGING AT %s\n"),
						  ctime (&currenttime)));

	return fd;
}

void
log_open (session *sess)
{
	static gboolean log_error = FALSE;

	log_close (sess);
	sess->logfd = log_open_file (sess->server->servername, sess->channel,
										  sess->server->networkname);

	if (!log_error && sess->logfd == -1)
	{
		char message[512];
		snprintf (message, sizeof (message),
					_("* Can't open log file(s) for writing. Check the\n" \
					  "  permissions on %s/xchatlogs"), get_xdir());
		fe_message (message, TRUE);

		log_error = TRUE;
	}
}

int
get_stamp_str (char *fmt, time_t tim, char **ret)
{
	char dest[128];
	gsize len;

	len = strftime (dest, sizeof (dest), fmt, localtime (&tim));
	if (len)
	{
		if (prefs.utf8_locale)
			*ret = g_strdup (dest);
		else
			*ret = g_locale_to_utf8 (dest, len, 0, &len, 0);
	}

	return len;
}

static void
log_write (session *sess, char *text)
{
	char *temp;
	char *stamp;
	int len;

	if (sess->logfd != -1 && prefs.logging)
	{
		if (prefs.timestamp_logs)
		{
			len = get_stamp_str (prefs.timestamp_log_format, time (0), &stamp);
			if (len)
			{
				write (sess->logfd, stamp, len);
				g_free (stamp);
			}
		}
		temp = strip_color (text);
		write (sess->logfd, temp, strlen (temp));
		free (temp);
	}
}

char *
text_validate (char **text, int *len)
{
	char *utf;
	gsize utf_len;
	GError *error = NULL;

	/* valid utf8? */
	if (g_utf8_validate (*text, *len, 0))
		return NULL;

	/* maybe it's iso-8859-1 */
	utf = g_convert (*text, *len, "UTF-8", "ISO-8859-1", 0, &utf_len, &error);
	if (!utf)       /* should never happen; all text is iso-8859-1 valid */
	{
		if (error)
		{
			*text = g_strdup_printf ("\0034ICONV\017 %s\n", error->message);
			*len = strlen (*text);
			g_error_free (error);
		} else
		{
			*text = g_strdup ("%INVALID%");
			*len = 9;
		}
	} else
	{
		*text = utf;
		*len = utf_len;
	}

	return utf;
}

void
PrintText (session *sess, char *text)
{
	char *conv;

	if (!sess)
	{
		if (!sess_list)
			return;
		sess = (session *) sess_list->data;
	}

	/* make sure it's valid utf8 */
	if (text[0] == 0)
	{
		text = "\n";
		conv = NULL;
	} else
	{
		int len = -1;
		conv = text_validate ((char **)&text, &len);
	}

	log_write (sess, text);

#ifdef USE_HEBREW
	if (prefs.hebrew)
	{
   	const char *orgtext = NULL;
   	if (text)
   	{
			orgtext = text;
			text = strdup(text);
			strhebpatch (text, orgtext);
   	}
		fe_print_text (sess, text);
   	if (orgtext)
			free(text);
	} else
	{
#endif
		fe_print_text (sess, text);
#ifdef USE_HEBREW
	}
#endif

	if (conv)
		g_free (conv);
}

void
PrintTextf (session *sess, char *format, ...)
{
	va_list args;
	char *buf;

	va_start (args, format);
	buf = g_strdup_vprintf (format, args);
	va_end (args);

	PrintText (sess, buf);
	g_free (buf);
}

/* Print Events stuff here --AGL */

/* Consider the following a NOTES file:

   The main upshot of this is:
   * Plugins and Perl scripts (when I get round to signaling perl.c) can intercept text events and do what they like
   * The default text engine can be config'ed

   By default it should appear *exactly* the same (I'm working hard not to change the default style) but if you go into Settings->Edit Event Texts you can change the text's. The format is thus:

   The normal %Cx (color) and %B (bold) etc work

   $x is replaced with the data in var x (e.g. $1 is often the nick)

   $axxx is replace with a single byte of value xxx (in base 10)

   AGL (990507)
 */

/* These lists are thus:
   pntevts_text[] are the strings the user sees (WITH %x etc)
   pntevts[] are the data strings with \000 etc
 */

/* To add a new event:

   Think up a name (like "Join")
   Make up a pevt_name_help struct
	Add an entry to textevents.in
	Type: make textevents
 */

/* Internals:

   On startup ~/.xchat/printevents.conf is loaded if it doesn't exist the
   defaults are loaded. Any missing events are filled from defaults.
   Each event is parsed by pevt_build_string and a binary output is produced
   which looks like:

   (byte) value: 0 = {
   (int) numbers of bytes
   (char []) that number of byte to be memcpy'ed into the buffer
   }
   1 =
   (byte) number of varable to insert
   2 = end of buffer

   Each XP_TE_* signal is hard coded to call text_emit which calls
   display_event which decodes the data

   This means that this system *should be faster* than snprintf because
   it always 'knows' that format of the string (basically is preparses much
   of the work)

   --AGL
 */

char *pntevts_text[NUM_XP];
char *pntevts[NUM_XP];

#define pevt_generic_none_help NULL

static char *pevt_genmsg_help[] = {
	N_("Left message"),
	N_("Right message"),
};

static char *pevt_join_help[] = {
	N_("The nick of the joining person"),
	N_("The channel being joined"),
	N_("The host of the person"),
};

static char *pevt_chanaction_help[] = {
	N_("Nickname"),
	N_("The action"),
};

static char *pevt_chanmsg_help[] = {
	N_("Nickname"),
	N_("The text"),
	N_("Mode char"),
};

static char *pevt_privmsg_help[] = {
	N_("Nickname"),
	N_("The message"),
};

static char *pevt_changenick_help[] = {
	N_("Old nickname"),
	N_("New nickname"),
};

static char *pevt_newtopic_help[] = {
	N_("Nick of person who changed the topic"),
	N_("Topic"),
	N_("Channel"),
};

static char *pevt_topic_help[] = {
	N_("Channel"),
	N_("Topic"),
};

static char *pevt_kick_help[] = {
	N_("The nickname of the kicker"),
	N_("The person being kicked"),
	N_("The channel"),
	N_("The reason"),
};

static char *pevt_part_help[] = {
	N_("The nick of the person leaving"),
	N_("The host of the person"),
	N_("The channel"),
};

static char *pevt_chandate_help[] = {
	N_("The channel"),
	N_("The time"),
};

static char *pevt_topicdate_help[] = {
	N_("The channel"),
	N_("The creator"),
	N_("The time"),
};

static char *pevt_quit_help[] = {
	N_("Nick"),
	N_("Reason"),
	N_("Host"),
};

static char *pevt_pingrep_help[] = {
	N_("Who it's from"),
	N_("The time in x.x format (see below)"),
};

static char *pevt_notice_help[] = {
	N_("Who it's from"),
	N_("The message"),
};

static char *pevt_channotice_help[] = {
	N_("Who it's from"),
	N_("The Channel it's going to"),
	N_("The message"),
};

static char *pevt_dprivmsg_help[] = {
	N_("Nickname"),
	N_("The message"),
};

static char *pevt_uchangenick_help[] = {
	N_("Old nickname"),
	N_("New nickname"),
};

static char *pevt_ukick_help[] = {
	N_("The person being kicked"),
	N_("The channel"),
	N_("The nickname of the kicker"),
	N_("The reason"),
};

static char *pevt_partreason_help[] = {
	N_("The nick of the person leaving"),
	N_("The host of the person"),
	N_("The channel"),
	N_("The reason"),
};

static char *pevt_ctcpsnd_help[] = {
	N_("The sound"),
	N_("The nick of the person"),
};

static char *pevt_ctcpgen_help[] = {
	N_("The CTCP event"),
	N_("The nick of the person"),
};

static char *pevt_ctcpgenc_help[] = {
	N_("The CTCP event"),
	N_("The nick of the person"),
	N_("The Channel it's going to"),
};

static char *pevt_chansetkey_help[] = {
	N_("The nick of the person who set the key"),
	N_("The key"),
};

static char *pevt_chansetlimit_help[] = {
	N_("The nick of the person who set the limit"),
	N_("The limit"),
};

static char *pevt_chanop_help[] = {
	N_("The nick of the person who did the op'ing"),
	N_("The nick of the person who has been op'ed"),
};

static char *pevt_chanhop_help[] = {
	N_("The nick of the person who has been halfop'ed"),
	N_("The nick of the person who did the halfop'ing"),
};

static char *pevt_chanvoice_help[] = {
	N_("The nick of the person who did the voice'ing"),
	N_("The nick of the person who has been voice'ed"),
};

static char *pevt_chanban_help[] = {
	N_("The nick of the person who did the banning"),
	N_("The ban mask"),
};

static char *pevt_chanrmkey_help[] = {
	N_("The nick who removed the key"),
};

static char *pevt_chanrmlimit_help[] = {
	N_("The nick who removed the limit"),
};

static char *pevt_chandeop_help[] = {
	N_("The nick of the person of did the deop'ing"),
	N_("The nick of the person who has been deop'ed"),
};
static char *pevt_chandehop_help[] = {
	N_("The nick of the person of did the dehalfop'ing"),
	N_("The nick of the person who has been dehalfop'ed"),
};

static char *pevt_chandevoice_help[] = {
	N_("The nick of the person of did the devoice'ing"),
	N_("The nick of the person who has been devoice'ed"),
};

static char *pevt_chanunban_help[] = {
	N_("The nick of the person of did the unban'ing"),
	N_("The ban mask"),
};

static char *pevt_chanexempt_help[] = {
	N_("The nick of the person who did the exempt"),
	N_("The exempt mask"),
};

static char *pevt_chanrmexempt_help[] = {
	N_("The nick of the person removed the exempt"),
	N_("The exempt mask"),
};

static char *pevt_chaninvite_help[] = {
	N_("The nick of the person who did the invite"),
	N_("The invite mask"),
};

static char *pevt_chanrminvite_help[] = {
	N_("The nick of the person removed the invite"),
	N_("The invite mask"),
};

static char *pevt_chanmodegen_help[] = {
	N_("The nick of the person setting the mode"),
	N_("The mode's sign (+/-)"),
	N_("The mode letter"),
	N_("The channel it's being set on"),
};

static char *pevt_whois1_help[] = {
	N_("Nickname"),
	N_("Username"),
	N_("Host"),
	N_("Full name"),
};

static char *pevt_whois2_help[] = {
	N_("Nickname"),
	N_("Channel Membership/\"is an IRC operator\""),
};

static char *pevt_whois3_help[] = {
	N_("Nickname"),
	N_("Server Information"),
};

static char *pevt_whois4_help[] = {
	N_("Nickname"),
	N_("Idle time"),
};

static char *pevt_whois4t_help[] = {
	N_("Nickname"),
	N_("Idle time"),
	N_("Signon time"),
};

static char *pevt_whois5_help[] = {
	N_("Nickname"),
	N_("Away reason"),
};

static char *pevt_whois6_help[] = {
	N_("Nickname"),
};

static char *pevt_whoisid_help[] = {
	N_("Nickname"),
	N_("Message"),
};

static char *pevt_whoisauth_help[] = {
	N_("Nickname"),
	N_("Message"),
	N_("Account"),
};

static char *pevt_whoisrealhost_help[] = {
	N_("Nickname"),
	N_("Real user@host"),
	N_("Real IP"),
	N_("Message"),
};

static char *pevt_generic_channel_help[] = {
	N_("Channel Name"),
};

static char *pevt_servertext_help[] = {
	N_("Text"),
	N_("Server Name")
};

static char *pevt_invited_help[] = {
	N_("Channel Name"),
	N_("Nick of person who invited you"),
	N_("Server Name"),
};

static char *pevt_usersonchan_help[] = {
	N_("Channel Name"),
	N_("Users"),
};

static char *pevt_nickclash_help[] = {
	N_("Nickname in use"),
	N_("Nick being tried"),
};

static char *pevt_connfail_help[] = {
	N_("Error String"),
};

static char *pevt_connect_help[] = {
	N_("Host"),
	N_("IP"),
	N_("Port"),
};

static char *pevt_sconnect_help[] = {
	N_("PID"),
};

static char *pevt_generic_nick_help[] = {
	N_("Nickname"),
};

static char *pevt_chanmodes_help[] = {
	N_("Channel name"),
	N_("Modes string"),
};

static char *pevt_rawmodes_help[] = {
	N_("Nickname"),
	N_("Modes string"),
};

static char *pevt_kill_help[] = {
	N_("Nickname"),
	N_("Reason"),
};

static char *pevt_dccchaterr_help[] = {
	N_("Nickname"),
	N_("IP address"),
	N_("Port"),
	N_("Error name"),
};

static char *pevt_dccstall_help[] = {
	N_("DCC Type"),
	N_("Filename"),
	N_("Nickname"),
};

static char *pevt_generic_file_help[] = {
	N_("Filename"),
};

static char *pevt_dccrecverr_help[] = {
	N_("Filename"),
	N_("Destination filename"),
	N_("Nickname"),
	N_("Error name"),
};

static char *pevt_dccrecvcomp_help[] = {
	N_("Filename"),
	N_("Destination filename"),
	N_("Nickname"),
	N_("CPS"),
};

static char *pevt_dccconfail_help[] = {
	N_("DCC Type"),
	N_("Nickname"),
	N_("Error string"),
};

static char *pevt_dccchatcon_help[] = {
	N_("Nickname"),
	N_("IP address"),
};

static char *pevt_dcccon_help[] = {
	N_("Nickname"),
	N_("IP address"),
	N_("Filename"),
};

static char *pevt_dccsendfail_help[] = {
	N_("Filename"),
	N_("Nickname"),
	N_("Error name"),
};

static char *pevt_dccsendcomp_help[] = {
	N_("Filename"),
	N_("Nickname"),
	N_("CPS"),
};

static char *pevt_dccoffer_help[] = {
	N_("Filename"),
	N_("Nickname"),
	N_("Pathname"),
};

static char *pevt_dccfileabort_help[] = {
	N_("Nickname"),
	N_("Filename")
};

static char *pevt_dccchatabort_help[] = {
	N_("Nickname"),
};

static char *pevt_dccresumeoffer_help[] = {
	N_("Nickname"),
	N_("Filename"),
	N_("Position"),
};

static char *pevt_dccsendoffer_help[] = {
	N_("Nickname"),
	N_("Filename"),
	N_("Size"),
	N_("IP address"),
};

static char *pevt_dccgenericoffer_help[] = {
	N_("DCC String"),
	N_("Nickname"),
};

static char *pevt_notifynumber_help[] = {
	N_("Number of notify items"),
};

static char *pevt_serverlookup_help[] = {
	N_("Servername"),
};

static char *pevt_servererror_help[] = {
	N_("Text"),
};

static char *pevt_foundip_help[] = {
	N_("IP"),
};

static char *pevt_dccrename_help[] = {
	N_("Old Filename"),
	N_("New Filename"),
};

static char *pevt_ctcpsend_help[] = {
	N_("Receiver"),
	N_("Message"),
};

static char *pevt_ignoreaddremove_help[] = {
	N_("Hostmask"),
};

static char *pevt_resolvinguser_help[] = {
	N_("Nickname"),
	N_("Hostname"),
};

static char *pevt_malformed_help[] = {
	N_("Nickname"),
	N_("The Packet"),
};

static char *pevt_pingtimeout_help[] = {
	N_("Seconds"),
};

static char *pevt_uinvite_help[] = {
	N_("Nick of person who have been invited"),
	N_("Channel Name"),
	N_("Server Name"),
};

static char *pevt_banlist_help[] = {
	N_("Channel"),
	N_("Banmask"),
	N_("Who set the ban"),
	N_("Ban time"),
};

static char *pevt_discon_help[] = {
	N_("Error"),
};

#include "textevents.h"

static void
pevent_load_defaults ()
{
	int i, len;

	for (i = 0; i < NUM_XP; i++)
	{
		len = strlen (_(te[i].def));
		len++;
		if (pntevts_text[i])
			free (pntevts_text[i]);
		pntevts_text[i] = malloc (len);
		memcpy (pntevts_text[i], _(te[i].def), len);
	}
}

void
pevent_make_pntevts ()
{
	int i, m, len;
	char out[1024];

	for (i = 0; i < NUM_XP; i++)
	{
		if (pntevts[i] != NULL)
			free (pntevts[i]);
		if (pevt_build_string (pntevts_text[i], &(pntevts[i]), &m) != 0)
		{
			snprintf (out, sizeof (out),
						 _("Error parsing event %s.\nLoading default"), te[i].name);
			fe_message (out, FALSE);
			free (pntevts_text[i]);
			len = strlen (te[i].def) + 1;
			pntevts_text[i] = malloc (len);
			memcpy (pntevts_text[i], te[i].def, len);
			if (pevt_build_string (pntevts_text[i], &(pntevts[i]), &m) != 0)
			{
				fprintf (stderr,
							"XChat CRITICAL *** default event text failed to build!\n");
				abort ();
			}
		}
		check_special_chars (pntevts[i], TRUE);
	}
}

/* Loading happens at 2 levels:
   1) File is read into blocks
   2) Pe block is parsed and loaded

   --AGL */

/* Better hope you pass good args.. --AGL */

static void
pevent_trigger_load (int *i_penum, char **i_text, char **i_snd)
{
	int penum = *i_penum, len;
	char *text = *i_text, *snd = *i_snd;

	if (penum != -1 && text != NULL)
	{
		len = strlen (text) + 1;
		if (pntevts_text[penum])
			free (pntevts_text[penum]);
		pntevts_text[penum] = malloc (len);
		memcpy (pntevts_text[penum], text, len);
		if (te[penum].sound)
			free (te[penum].sound);
		if (snd)
		{
			len = strlen (snd) + 1;
			te[penum].sound = malloc (len);
			memcpy (te[penum].sound, snd, len);
		} else
			te[penum].sound = NULL;
	}

	if (text)
		free (text);
	if (snd)
		free (snd);
	*i_text = NULL;
	*i_snd = NULL;
	*i_penum = 0;
}

static int
pevent_find (char *name, int *i_i)
{
	int i = *i_i, j;

	j = i + 1;
	while (1)
	{
		if (j == NUM_XP)
			j = 0;
		if (strcmp (te[j].name, name) == 0)
		{
			*i_i = j;
			return j;
		}
		if (j == i)
			return -1;
		j++;
	}
}

int
pevent_load (char *filename)
{
	/* AGL, I've changed this file and pevent_save, could you please take a look at
	 *      the changes and possibly modify them to suit you
	 *      //David H
	 */
	char *buf, *ibuf;
	int fd, i = 0, pnt = 0;
	struct stat st;
	char *text = NULL, *snd = NULL;
	int penum = 0;
	char *ofs;

	buf = malloc (1000);
	if (filename == NULL)
		snprintf (buf, 1000, "%s/pevents.conf", get_xdir ());
   else
      safe_strcpy (buf, filename, 1000);

	fd = open (buf, O_RDONLY | OFLAGS);
	free (buf);
	if (fd == -1)
		return 1;
	if (fstat (fd, &st) != 0)
		return 1;
	ibuf = malloc (st.st_size);
	read (fd, ibuf, st.st_size);
	close (fd);

	while (buf_get_line (ibuf, &buf, &pnt, st.st_size))
	{
		if (buf[0] == '#')
			continue;
		if (strlen (buf) == 0)
			continue;

		ofs = strchr (buf, '=');
		if (!ofs)
			continue;
		*ofs = 0;
		ofs++;
		/*if (*ofs == 0)
			continue;*/

		if (strcmp (buf, "event_name") == 0)
		{
			if (penum >= 0)
				pevent_trigger_load (&penum, &text, &snd);
			penum = pevent_find (ofs, &i);
			continue;
		} else if (strcmp (buf, "event_text") == 0)
		{
			if (text)
				free (text);
			text = strdup (ofs);
			continue;
		} else if (strcmp (buf, "event_sound") == 0)
		{
			if (snd)
				free (snd);
			snd = strdup (ofs);
			continue;
		}

		continue;
	}

	pevent_trigger_load (&penum, &text, &snd);
	free (ibuf);
	return 0;
}

static void
pevent_check_all_loaded ()
{
	int i, len;

	for (i = 0; i < NUM_XP; i++)
	{
		if (pntevts_text[i] == NULL)
		{
			/*printf ("%s\n", te[i].name);
			snprintf(out, sizeof(out), "The data for event %s failed to load. Reverting to defaults.\nThis may be because a new version of XChat is loading an old config file.\n\nCheck all print event texts are correct", evtnames[i]);
			   gtkutil_simpledialog(out); */
			len = strlen (te[i].def) + 1;
			pntevts_text[i] = malloc (len);
			memcpy (pntevts_text[i], te[i].def, len);
		}
	}
}

void
load_text_events ()
{
	memset (&pntevts_text, 0, sizeof (char *) * (NUM_XP));
	memset (&pntevts, 0, sizeof (char *) * (NUM_XP));

	if (pevent_load (NULL))
		pevent_load_defaults ();
	pevent_check_all_loaded ();
	pevent_make_pntevts ();
}

static void
display_event (char *i, session *sess, int numargs, char **args)
{
	int len, oi, ii;
	char *ar, o[4096], d, a, done_all = FALSE;

	oi = ii = len = d = a = 0;

	if (i == NULL)
		return;

	while (done_all == FALSE)
	{
		d = i[ii++];
		switch (d)
		{
		case 0:
			memcpy (&len, &(i[ii]), sizeof (int));
			ii += sizeof (int);
			if (oi + len > sizeof (o))
			{
				printf ("Overflow in display_event (%s)\n", i);
				return;
			}
			memcpy (&(o[oi]), &(i[ii]), len);
			oi += len;
			ii += len;
			break;
		case 1:
			a = i[ii++];
			if (a > numargs)
			{
				fprintf (stderr,
							"XChat DEBUG: display_event: arg > numargs (%d %d %s)\n",
							a, numargs, i);
				break;
			}
			ar = args[(int) a + 1];
			if (ar == NULL)
			{
				printf ("arg[%d] is NULL in print event\n", a + 1);
			} else
			{
				len = strlen (ar);
				memcpy (&o[oi], ar, len);
				oi += len;
			}
			break;
		case 2:
			o[oi++] = '\n';
			o[oi++] = 0;
			done_all = TRUE;
			continue;
		case 3:
/*			if (sess->type == SESS_DIALOG)
			{
				if (prefs.dialog_indent_nicks)
					o[oi++] = '\t';
				else
					o[oi++] = ' ';
			} else
			{*/
				if (prefs.indent_nicks)
					o[oi++] = '\t';
				else
					o[oi++] = ' ';
			/*}*/
			break;
		}
	}
	o[oi] = 0;
	if (*o == '\n')
		return;
	PrintText (sess, o);
}

int
pevt_build_string (const char *input, char **output, int *max_arg)
{
	struct pevt_stage1 *s = NULL, *base = NULL, *last = NULL, *next;
	int clen;
	char o[4096], d, *obuf, *i;
	int oi, ii, max = -1, len, x;

	len = strlen (input);
	i = malloc (len + 1);
	memcpy (i, input, len + 1);
	check_special_chars (i, TRUE);

	len = strlen (i);

	clen = oi = ii = 0;

	for (;;)
	{
		if (ii == len)
			break;
		d = i[ii++];
		if (d != '$')
		{
			o[oi++] = d;
			continue;
		}
		if (i[ii] == '$')
		{
			o[oi++] = '$';
			continue;
		}
		if (oi > 0)
		{
			s = (struct pevt_stage1 *) malloc (sizeof (struct pevt_stage1));
			if (base == NULL)
				base = s;
			if (last != NULL)
				last->next = s;
			last = s;
			s->next = NULL;
			s->data = malloc (oi + sizeof (int) + 1);
			s->len = oi + sizeof (int) + 1;
			clen += oi + sizeof (int) + 1;
			s->data[0] = 0;
			memcpy (&(s->data[1]), &oi, sizeof (int));
			memcpy (&(s->data[1 + sizeof (int)]), o, oi);
			oi = 0;
		}
		if (ii == len)
		{
			fe_message ("String ends with a $", FALSE);
			return 1;
		}
		d = i[ii++];
		if (d == 'a')
		{								  /* Hex value */
			x = 0;
			if (ii == len)
				goto a_len_error;
			d = i[ii++];
			d -= '0';
			x = d * 100;
			if (ii == len)
				goto a_len_error;
			d = i[ii++];
			d -= '0';
			x += d * 10;
			if (ii == len)
				goto a_len_error;
			d = i[ii++];
			d -= '0';
			x += d;
			if (x > 255)
				goto a_range_error;
			o[oi++] = x;
			continue;

		 a_len_error:
			fe_message ("String ends in $a", FALSE);
			return 1;
		 a_range_error:
			fe_message ("$a value is greater then 255", FALSE);
			return 1;
		}
		if (d == 't')
		{
			/* Tab - if tabnicks is set then write '\t' else ' ' */
			s = (struct pevt_stage1 *) malloc (sizeof (struct pevt_stage1));
			if (base == NULL)
				base = s;
			if (last != NULL)
				last->next = s;
			last = s;
			s->next = NULL;
			s->data = malloc (1);
			s->len = 1;
			clen += 1;
			s->data[0] = 3;

			continue;
		}
		if (d < '1' || d > '9')
		{
			snprintf (o, sizeof (o), "Error, invalid argument $%c\n", d);
			fe_message (o, FALSE);
			return 1;
		}
		d -= '0';
		if (max < d)
			max = d;
		s = (struct pevt_stage1 *) malloc (sizeof (struct pevt_stage1));
		if (base == NULL)
			base = s;
		if (last != NULL)
			last->next = s;
		last = s;
		s->next = NULL;
		s->data = malloc (2);
		s->len = 2;
		clen += 2;
		s->data[0] = 1;
		s->data[1] = d - 1;
	}
	if (oi > 0)
	{
		s = (struct pevt_stage1 *) malloc (sizeof (struct pevt_stage1));
		if (base == NULL)
			base = s;
		if (last != NULL)
			last->next = s;
		last = s;
		s->next = NULL;
		s->data = malloc (oi + sizeof (int) + 1);
		s->len = oi + sizeof (int) + 1;
		clen += oi + sizeof (int) + 1;
		s->data[0] = 0;
		memcpy (&(s->data[1]), &oi, sizeof (int));
		memcpy (&(s->data[1 + sizeof (int)]), o, oi);
		oi = 0;
	}
	s = (struct pevt_stage1 *) malloc (sizeof (struct pevt_stage1));
	if (base == NULL)
		base = s;
	if (last != NULL)
		last->next = s;
	last = s;
	s->next = NULL;
	s->data = malloc (1);
	s->len = 1;
	clen += 1;
	s->data[0] = 2;

	oi = 0;
	s = base;
	obuf = malloc (clen);
	while (s)
	{
		next = s->next;
		memcpy (&obuf[oi], s->data, s->len);
		oi += s->len;
		free (s->data);
		free (s);
		s = next;
	}

	free (i);

	if (max_arg)
		*max_arg = max;
	if (output)
		*output = obuf;

	return 0;
}

static void
play_wave (const char *file)
{
	char buf[512];
	char wavfile[512];

	memset (buf, 0, sizeof (buf));
	memset (wavfile, 0, sizeof (wavfile));
	if (file[0] != '/')
	{
		snprintf (wavfile, sizeof (wavfile), "%s/%s", prefs.sounddir, file);
	} else
	{
		strncpy (wavfile, file, sizeof (wavfile));
	}
	if (access (wavfile, R_OK) == 0)
	{
		snprintf (buf, sizeof (buf), "%s %s", prefs.soundcmd, wavfile);
		buf[sizeof (buf) - 1] = '\0';
		xchat_exec (buf);
	} else
	{
		snprintf (buf, sizeof (buf), "Cannot read %s", wavfile);
		fe_message (buf, FALSE);
	}
}

/* called by EMIT_SIGNAL macro */

void
text_emit (int index, session *sess, char *a, char *b, char *c, char *d)
{
	char *word[PDIWORDS];
	int i;

	if (!a)
		a = "\000";
	if (!b)
		b = "\000";
	if (!c)
		c = "\000";
	if (!d)
		d = "\000";

	word[0] = te[index].name;
	word[1] = a;
	word[2] = b;
	word[3] = c;
	word[4] = d;
	for (i = 5; i < PDIWORDS; i++)
		word[i] = "\000";

	if (plugin_emit_print (sess, word))
		return;

	if (te[index].sound)
		play_wave (te[index].sound);

	display_event (pntevts[index], sess, te[index].num_args, word);
}

int
text_emit_by_name (char *name, session *sess, char *a, char *b, char *c, char *d)
{
	int i = 0;

	i = pevent_find (name, &i);
	if (i >= 0)
	{
		text_emit (i, sess, a, b, c, d);
		return 1;
	}

	return 0;
}

void
pevent_save (char *fn)
{
	int fd, i;
	char buf[1024];

	if (!fn)
		snprintf (buf, sizeof (buf), "%s/pevents.conf", get_xdir ());
	else
		safe_strcpy (buf, fn, sizeof (buf));
	fd = open (buf, O_CREAT | O_TRUNC | O_WRONLY | OFLAGS, 0x180);
	if (fd == -1)
	{
		/*
		   fe_message ("Error opening config file\n", FALSE); 
		   If we get here when X-Chat is closing the fe-message causes a nice & hard crash
		   so we have to use perror which doesn't rely on GTK
		 */

		perror ("Error opening config file\n");
		return;
	}

	for (i = 0; i < NUM_XP; i++)
	{
		write (fd, buf, snprintf (buf, sizeof (buf),
										  "event_name=%s\n", te[i].name));
		write (fd, buf, snprintf (buf, sizeof (buf),
										  "event_text=%s\n", pntevts_text[i]));
		if (te[i].sound && te[i].sound[0])
			write (fd, buf, snprintf (buf, sizeof (buf),
											  "event_sound=%s\n", te[i].sound));
		write (fd, "\n", 1);
	}

	close (fd);
}
