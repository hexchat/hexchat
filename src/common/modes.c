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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "hexchat.h"
#include "hexchatc.h"
#include "modes.h"
#include "server.h"
#include "text.h"
#include "fe.h"
#include "util.h"
#include "inbound.h"
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <glib/gprintf.h>

typedef struct
{
	server *serv;
	char *op;
	char *deop;
	char *voice;
	char *devoice;
} mode_run;

static int is_prefix_char (server * serv, char c);
static void record_chan_mode (session *sess, char sign, char mode, char *arg);
static char *mode_cat (char *str, char *addition);
static void handle_single_mode (mode_run *mr, char sign, char mode, char *nick,
										  char *chan, char *arg, int quiet, int is_324,
										  const message_tags_data *tags_data);
static int mode_has_arg (server *serv, char sign, char mode);
static void mode_print_grouped (session *sess, char *nick, mode_run *mr,
										  const message_tags_data *tags_data);
static int mode_chanmode_type (server * serv, char mode);


/* word[] - list of nicks.
   wpos   - index into word[]. Where nicks really start.
   end    - index into word[]. Last entry plus one.
   sign   - a char, e.g. '+' or '-'
   mode   - a mode, e.g. 'o' or 'v'	*/
void
send_channel_modes (session *sess, char *tbuf, char *word[], int wpos,
						  int end, char sign, char mode, int modes_per_line)
{
	int usable_modes, orig_len, len, wlen, i, max;
	server *serv = sess->server;

	/* sanity check. IRC RFC says three per line. */
	if (serv->modes_per_line < 3)
		serv->modes_per_line = 3;
	if (modes_per_line < 1)
		modes_per_line = serv->modes_per_line;

	/* RFC max, minus length of "MODE %s " and "\r\n" and 1 +/- sign */
	/* 512 - 6 - 2 - 1 - strlen(chan) */
	max = 503 - strlen (sess->channel);

	while (wpos < end)
	{
		tbuf[0] = '\0';
		orig_len = len = 0;

		/* we'll need this many modechars too */
		len += modes_per_line;

		/* how many can we fit? */
		for (i = 0; i < modes_per_line; i++)
		{
			/* no more nicks left? */
			if (wpos + i >= end)
				break;
			wlen = strlen (word[wpos + i]) + 1;
			if (wlen + len > max)
				break;
			len += wlen; /* length of our whole string so far */
		}
		if (i < 1)
			return;
		usable_modes = i;	/* this is how many we'll send on this line */

		/* add the +/-modemodemodemode */
		len = orig_len;
		tbuf[len] = sign;
		len++;
		for (i = 0; i < usable_modes; i++)
		{
			tbuf[len] = mode;
			len++;
		}
		tbuf[len] = 0;	/* null terminate for the strcat() to work */

		/* add all the nicknames */
		for (i = 0; i < usable_modes; i++)
		{
			strcat (tbuf, " ");
			strcat (tbuf, word[wpos + i]);
		}
		serv->p_mode (serv, sess->channel, tbuf);

		wpos += usable_modes;
	}
}

/* does 'chan' have a valid prefix? e.g. # or & */

int
is_channel (server * serv, char *chan)
{
	if (strchr (serv->chantypes, chan[0]))
		return 1;
	return 0;
}

/* is the given char a valid nick mode char? e.g. @ or + */

static int
is_prefix_char (server * serv, char c)
{
	int pos = 0;
	char *np = serv->nick_prefixes;

	while (np[0])
	{
		if (np[0] == c)
			return pos;
		pos++;
		np++;
	}

	if (serv->bad_prefix)
	{
		if (strchr (serv->bad_nick_prefixes, c))
		/* valid prefix char, but mode unknown */
			return -2;
	}

	return -1;
}

/* returns '@' for ops etc... */

char
get_nick_prefix (server * serv, unsigned int access)
{
	int pos;
	char c;

	for (pos = 0; pos < USERACCESS_SIZE; pos++)
	{
		c = serv->nick_prefixes[pos];
		if (c == 0)
			break;
		if (access & (1 << pos))
			return c;
	}

	return 0;
}

/* returns the access bitfield for a nickname. E.g.
	@nick would return 000010 in binary
	%nick would return 000100 in binary
	+nick would return 001000 in binary */

unsigned int
nick_access (server * serv, char *nick, int *modechars)
{
	int i;
	unsigned int access = 0;
	char *orig = nick;

	while (*nick)
	{
		i = is_prefix_char (serv, *nick);
		if (i == -1)
			break;

		/* -2 == valid prefix char, but mode unknown */
		if (i != -2)
			access |= (1 << i);

		nick++;
	}

	*modechars = nick - orig;

	return access;
}

/* returns the access number for a particular mode. e.g.
	mode 'a' returns 0
	mode 'o' returns 1
	mode 'h' returns 2
	mode 'v' returns 3
	Also puts the nick-prefix-char in 'prefix' */

int
mode_access (server * serv, char mode, char *prefix)
{
	int pos = 0;

	while (serv->nick_modes[pos])
	{
		if (serv->nick_modes[pos] == mode)
		{
			*prefix = serv->nick_prefixes[pos];
			return pos;
		}
		pos++;
	}

	*prefix = 0;

	return -1;
}

static void
record_chan_mode (session *sess, char sign, char mode, char *arg)
{
	/* Somebody needed to acutally update sess->current_modes, needed to
		play nice with bouncers, and less mode calls. Also keeps modes up
		to date for scripts */
	server *serv = sess->server;
	GString *current = g_string_new(sess->current_modes);
	gint mode_pos = -1;
	gchar *current_char = current->str;
	gint modes_length;
	gint argument_num = 0;
	gint argument_offset = 0;
	gint argument_length = 0;
	int i = 0;
	gchar *arguments_start;

	/* find out if the mode currently exists */
	arguments_start = g_strstr_len(current->str	, -1, " ");
	if (arguments_start) {
		modes_length = arguments_start - current->str;
	}
	else {
		modes_length = current->len;
		/* set this to the end of the modes */
		arguments_start = current->str + current->len;
	}

	while (mode_pos == -1 && i < modes_length)
	{
		if (*current_char == mode)
		{
			mode_pos = i;
		}
		else
		{
			i++;
			current_char++;
		}
	}

	/* if the mode currently exists and has an arg, need to know where
	 * (including leading space) */
	if (mode_pos != -1 && mode_has_arg(serv, '+', mode))
	{
		current_char = current->str;

		i = 0;
		while (i <= mode_pos)
		{
			if (mode_has_arg(serv, '+', *current_char))
				argument_num++;
			current_char++;
			i++;
		}

		/* check through arguments for where to start */
		current_char = arguments_start;
		i = 0;
		while (i < argument_num && *current_char != '\0')
		{
			if (*current_char == ' ')
				i++;
			if (i != argument_num)
				current_char++;
		}
		argument_offset = current_char - current->str;

		/* how long the existing argument is for this key
		 * important for malloc and strncpy */
		if (i == argument_num)
		{
			argument_length++;
			current_char++;
			while (*current_char != '\0' && *current_char != ' ')
			{
				argument_length++;
				current_char++;
			}
		}
	}

	/* two cases, adding and removing a mode, handled differently */
	if (sign == '+')
	{
		if (mode_pos != -1)
		{
			/* if it already exists, only need to do something (change)
			 * if there should be a param */
			if (mode_has_arg(serv, sign, mode))
			{
				/* leave the old space there */
				current = g_string_erase(current, argument_offset+1, argument_length-1);
				current = g_string_insert(current, argument_offset+1, arg);

				free(sess->current_modes);
				sess->current_modes = g_string_free(current, FALSE);
			}
		}
		/* mode wasn't there before */
		else
		{
			/* insert the new mode character */
			current = g_string_insert_c(current, modes_length, mode);

			/* add the argument, with space if there is one */
			if (mode_has_arg(serv, sign, mode))
			{
				current = g_string_append_c(current, ' ');
				current = g_string_append(current, arg);
			}

			free(sess->current_modes);
			sess->current_modes = g_string_free(current, FALSE);
		}
	}
	else if (sign == '-' && mode_pos != -1)
	{
		/* remove the argument first if it has one*/
		if (mode_has_arg(serv, '+', mode))
			current = g_string_erase(current, argument_offset, argument_length);

		/* remove the mode character */
		current = g_string_erase(current, mode_pos, 1);

		free(sess->current_modes);
		sess->current_modes = g_string_free(current, FALSE);
	}
}

static char *
mode_cat (char *str, char *addition)
{
	int len;

	if (str)
	{
		len = strlen (str) + strlen (addition) + 2;
		str = realloc (str, len);
		strcat (str, " ");
		strcat (str, addition);
	} else
	{
		str = strdup (addition);
	}

	return str;
}

/* handle one mode, e.g.
   handle_single_mode (mr,'+','b',"elite","#warez","banneduser",) */

static void
handle_single_mode (mode_run *mr, char sign, char mode, char *nick,
						  char *chan, char *arg, int quiet, int is_324,
						  const message_tags_data *tags_data)
{
	session *sess;
	server *serv = mr->serv;
	char outbuf[4];
	char *cm = serv->chanmodes;
	gboolean supportsq = FALSE;

	outbuf[0] = sign;
	outbuf[1] = 0;
	outbuf[2] = mode;
	outbuf[3] = 0;

	sess = find_channel (serv, chan);
	if (!sess || !is_channel (serv, chan))
	{
		/* got modes for a chan we're not in! probably nickmode +isw etc */
		sess = serv->front_session;
		goto genmode;
	}

	/* is this a nick mode? */
	if (strchr (serv->nick_modes, mode))
	{
		/* update the user in the userlist */
		userlist_update_mode (sess, /*nickname */ arg, mode, sign);
	} else
	{
		if (!is_324 && !sess->ignore_mode && mode_chanmode_type(serv, mode) >= 1)
			record_chan_mode (sess, sign, mode, arg);
	}

	/* Is q a chanmode on this server? */
	if (cm)
		while (*cm)
		{
			if (*cm == ',')
				break;
			if (*cm == 'q')
				supportsq = TRUE;
			cm++;
		}

	switch (sign)
	{
	case '+':
		switch (mode)
		{
		case 'k':
			safe_strcpy (sess->channelkey, arg, sizeof (sess->channelkey));
			fe_update_channel_key (sess);
			fe_update_mode_buttons (sess, mode, sign);
			if (!quiet)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANSETKEY, sess, nick, arg, NULL,
											  NULL, 0, tags_data->timestamp);
			return;
		case 'l':
			sess->limit = atoi (arg);
			fe_update_channel_limit (sess);
			fe_update_mode_buttons (sess, mode, sign);
			if (!quiet)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANSETLIMIT, sess, nick, arg, NULL,
											  NULL, 0, tags_data->timestamp);
			return;
		case 'o':
			if (!quiet)
				mr->op = mode_cat (mr->op, arg);
			return;
		case 'h':
			if (!quiet)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANHOP, sess, nick, arg, NULL, NULL,
											  0, tags_data->timestamp);
			return;
		case 'v':
			if (!quiet)
				mr->voice = mode_cat (mr->voice, arg);
			return;
		case 'b':
			if (!quiet)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANBAN, sess, nick, arg, NULL, NULL,
											  0, tags_data->timestamp);
			return;
		case 'e':
			if (!quiet)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANEXEMPT, sess, nick, arg, NULL,
											  NULL, 0, tags_data->timestamp);
			return;
		case 'I':
			if (!quiet)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANINVITE, sess, nick, arg, NULL, NULL,
											  0, tags_data->timestamp);
			return;
		case 'q':
			if (!supportsq)
				break; /* +q is owner on this server */
			if (!quiet)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANQUIET, sess, nick, arg, NULL, NULL, 0,
								 tags_data->timestamp);
			return;
		}
		break;
	case '-':
		switch (mode)
		{
		case 'k':
			sess->channelkey[0] = 0;
			fe_update_channel_key (sess);
			fe_update_mode_buttons (sess, mode, sign);
			if (!quiet)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANRMKEY, sess, nick, NULL, NULL,
											  NULL, 0, tags_data->timestamp);
			return;
		case 'l':
			sess->limit = 0;
			fe_update_channel_limit (sess);
			fe_update_mode_buttons (sess, mode, sign);
			if (!quiet)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANRMLIMIT, sess, nick, NULL, NULL,
											  NULL, 0, tags_data->timestamp);
			return;
		case 'o':
			if (!quiet)
				mr->deop = mode_cat (mr->deop, arg);
			return;
		case 'h':
			if (!quiet)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANDEHOP, sess, nick, arg, NULL,
											  NULL, 0, tags_data->timestamp);
			return;
		case 'v':
			if (!quiet)
				mr->devoice = mode_cat (mr->devoice, arg);
			return;
		case 'b':
			if (!quiet)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANUNBAN, sess, nick, arg, NULL, NULL,
											  0, tags_data->timestamp);
			return;
		case 'e':
			if (!quiet)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANRMEXEMPT, sess, nick, arg, NULL,
											  NULL, 0, tags_data->timestamp);
			return;
		case 'I':
			if (!quiet)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANRMINVITE, sess, nick, arg, NULL,
											  NULL, 0, tags_data->timestamp);
			return;
		case 'q':
			if (!supportsq)
				break; /* -q is owner on this server */
			if (!quiet)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANUNQUIET, sess, nick, arg, NULL,
											  NULL, 0, tags_data->timestamp);
			return;
		}
	}

	fe_update_mode_buttons (sess, mode, sign);

 genmode:
	/* Received umode +e. If we're waiting to send JOIN then send now! */
	if (mode == 'e' && sign == '+' && !serv->p_cmp (chan, serv->nick))
		inbound_identified (serv);

	if (!quiet)
	{
		if (*arg)
		{
			char *buf = malloc (strlen (chan) + strlen (arg) + 2);
			sprintf (buf, "%s %s", chan, arg);
			EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANMODEGEN, sess, nick, outbuf,
										  outbuf + 2, buf, 0, tags_data->timestamp);
			free (buf);
		} else
			EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANMODEGEN, sess, nick, outbuf,
										  outbuf + 2, chan, 0, tags_data->timestamp);
	}
}

/* does this mode have an arg? like +b +l +o */

static int
mode_has_arg (server * serv, char sign, char mode)
{
	int type;

	/* if it's a nickmode, it must have an arg */
	if (strchr (serv->nick_modes, mode))
		return 1;

	type = mode_chanmode_type (serv, mode);
	switch (type)
	{
	case 0:					  /* type A */
	case 1:					  /* type B */
		return 1;
	case 2:					  /* type C */
		if (sign == '+')
			return 1;
	case 3:					  /* type D */
		return 0;
	default:
		return 0;
	}

}

/* what type of chanmode is it? -1 for not in chanmode */
static int
mode_chanmode_type (server * serv, char mode)
{
	/* see what numeric 005 CHANMODES=xxx said */
	char *cm = serv->chanmodes;
	int type = 0;
	int found = 0;

	while (*cm && !found)
	{
		if (*cm == ',')
		{
			type++;
		} else if (*cm == mode)
		{
			found = 1;
		}
		cm++;
	}
	if (found)
		return type;
	/* not found? -1 */
	else
		return -1;
}

static void
mode_print_grouped (session *sess, char *nick, mode_run *mr,
						  const message_tags_data *tags_data)
{
	/* print all the grouped Op/Deops */
	if (mr->op)
	{
		EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANOP, sess, nick, mr->op, NULL, NULL, 0,
									  tags_data->timestamp);
		free (mr->op);
		mr->op = NULL;
	}

	if (mr->deop)
	{
		EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANDEOP, sess, nick, mr->deop, NULL, NULL,
									  0, tags_data->timestamp);
		free (mr->deop);
		mr->deop = NULL;
	}

	if (mr->voice)
	{
		EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANVOICE, sess, nick, mr->voice, NULL, NULL,
									  0, tags_data->timestamp);
		free (mr->voice);
		mr->voice = NULL;
	}

	if (mr->devoice)
	{
		EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANDEVOICE, sess, nick, mr->devoice, NULL,
									  NULL, 0, tags_data->timestamp);
		free (mr->devoice);
		mr->devoice = NULL;
	}
}


/* handle a MODE or numeric 324 from server */

void
handle_mode (server * serv, char *word[], char *word_eol[],
				 char *nick, int numeric_324, const message_tags_data *tags_data)
{
	session *sess;
	char *chan;
	char *modes;
	char *argstr;
	char sign;
	int len;
	int arg;
	int i, num_args;
	int num_modes;
	int offset = 3;
	int all_modes_have_args = FALSE;
	int using_front_tab = FALSE;
	mode_run mr;

	mr.serv = serv;
	mr.op = mr.deop = mr.voice = mr.devoice = NULL;

	/* numeric 324 has everything 1 word later (as opposed to MODE) */
	if (numeric_324)
		offset++;

	chan = word[offset];
	modes = word[offset + 1];
	if (*modes == ':')
		modes++;

	if (*modes == 0)
		return;	/* beyondirc's blank modes */

	sess = find_channel (serv, chan);
	if (!sess)
	{
		sess = serv->front_session;
		using_front_tab = TRUE;
	}
	/* remove trailing space */
	len = strlen (word_eol[offset]) - 1;
	if (word_eol[offset][len] == ' ')
		word_eol[offset][len] = 0;

	if (prefs.hex_irc_raw_modes && !numeric_324)
		EMIT_SIGNAL_TIMESTAMP (XP_TE_RAWMODES, sess, nick, word_eol[offset], 0, 0, 0,
									  tags_data->timestamp);

	if (numeric_324 && !using_front_tab)
	{
		if (sess->current_modes)
			free (sess->current_modes);
		sess->current_modes = strdup (word_eol[offset+1]);
	}

	sign = *modes;
	modes++;
	arg = 1;

	/* count the number of arguments (e.g. after the -o+v) */
	num_args = 0;
	i = 1;
	while ((i + offset + 1) < PDIWORDS)
	{
		i++;
		if (!(*word[i + offset]))
			break;
		num_args++;
	}

	/* count the number of modes (without the -/+ chars */
	num_modes = 0;
	i = 0;
	while (i < strlen (modes))
	{
		if (modes[i] != '+' && modes[i] != '-')
			num_modes++;
		i++;
	}

	if (num_args == num_modes)
		all_modes_have_args = TRUE;

	while (*modes)
	{
		switch (*modes)
		{
		case '-':
		case '+':
			/* print all the grouped Op/Deops */
			mode_print_grouped (sess, nick, &mr, tags_data);
			sign = *modes;
			break;
		default:
			argstr = "";
			if ((all_modes_have_args || mode_has_arg (serv, sign, *modes)) && arg < (num_args+1))
			{
				arg++;
				argstr = word[arg + offset];
			}
			handle_single_mode (&mr, sign, *modes, nick, chan,
									  argstr, numeric_324 || prefs.hex_irc_raw_modes,
									  numeric_324, tags_data);
		}

		modes++;
	}

	/* update the title at the end, now that the mode update is internal now */
	if (!using_front_tab)
		fe_set_title (sess);

	/* print all the grouped Op/Deops */
	mode_print_grouped (sess, nick, &mr, tags_data);
}

/* handle the 005 numeric */

void
inbound_005 (server * serv, char *word[], const message_tags_data *tags_data)
{
	int w;
	char *pre;

	w = 4;							  /* start at the 4th word */
	while (w < PDIWORDS && *word[w])
	{
		if (strncmp (word[w], "MODES=", 6) == 0)
		{
			serv->modes_per_line = atoi (word[w] + 6);
		} else if (strncmp (word[w], "CHANTYPES=", 10) == 0)
		{
			free (serv->chantypes);
			serv->chantypes = strdup (word[w] + 10);
		} else if (strncmp (word[w], "CHANMODES=", 10) == 0)
		{
			free (serv->chanmodes);
			serv->chanmodes = strdup (word[w] + 10);
		} else if (strncmp (word[w], "PREFIX=", 7) == 0)
		{
			pre = strchr (word[w] + 7, ')');
			if (pre)
			{
				pre[0] = 0;			  /* NULL out the ')' */
				free (serv->nick_prefixes);
				free (serv->nick_modes);
				serv->nick_prefixes = strdup (pre + 1);
				serv->nick_modes = strdup (word[w] + 8);
			} else
			{
				/* bad! some ircds don't give us the modes. */
				/* in this case, we use it only to strip /NAMES */
				serv->bad_prefix = TRUE;
				if (serv->bad_nick_prefixes)
					free (serv->bad_nick_prefixes);
				serv->bad_nick_prefixes = strdup (word[w] + 7);
			}
		} else if (strncmp (word[w], "WATCH=", 6) == 0)
		{
			serv->supports_watch = TRUE;
		} else if (strncmp (word[w], "MONITOR=", 8) == 0)
		{
			serv->supports_monitor = TRUE;
		} else if (strncmp (word[w], "NETWORK=", 8) == 0)
		{
/*			if (serv->networkname)
				free (serv->networkname);
			serv->networkname = strdup (word[w] + 8);*/

			if (serv->server_session->type == SESS_SERVER)
			{
				safe_strcpy (serv->server_session->channel, word[w] + 8, CHANLEN);
				fe_set_channel (serv->server_session);
			}

		} else if (strncmp (word[w], "CASEMAPPING=", 12) == 0)
		{
			if (strcmp (word[w] + 12, "ascii") == 0)	/* bahamut */
				serv->p_cmp = (void *)g_ascii_strcasecmp;
		} else if (strncmp (word[w], "CHARSET=", 8) == 0)
		{
			if (g_ascii_strncasecmp (word[w] + 8, "UTF-8", 5) == 0)
			{
				server_set_encoding (serv, "UTF-8");
			}
		} else if (strcmp (word[w], "NAMESX") == 0)
		{
									/* 12345678901234567 */
			tcp_send_len (serv, "PROTOCTL NAMESX\r\n", 17);
		} else if (strcmp (word[w], "WHOX") == 0)
		{
			serv->have_whox = TRUE;
		} else if (strcmp (word[w], "EXCEPTS") == 0)
		{
#ifndef WIN32
			serv->have_except = TRUE;
#endif
		} else if (strcmp (word[w], "INVEX") == 0)
		{
			/* supports mode letter +I, default channel invite */
			serv->have_invite = TRUE;
		} else if (strncmp (word[w], "ELIST=", 6) == 0)
		{
			/* supports LIST >< min/max user counts? */
			if (strchr (word[w] + 6, 'U') || strchr (word[w] + 6, 'u'))
				serv->use_listargs = TRUE;
		}

		w++;
	}
}
