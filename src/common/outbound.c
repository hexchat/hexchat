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

#define _GNU_SOURCE	/* for memrchr */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#define WANTSOCKET
#define WANTARPA
#include "inet.h"

#ifndef WIN32
#include <sys/wait.h>
#endif

#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "xchat.h"
#include "plugin.h"
#include "ignore.h"
#include "util.h"
#include "fe.h"
#include "cfgfiles.h"			  /* xchat_fopen_file() */
#include "network.h"				/* net_ip() */
#include "modes.h"
#include "notify.h"
#include "inbound.h"
#include "text.h"
#include "xchatc.h"
#include "servlist.h"
#include "server.h"
#include "tree.h"
#include "outbound.h"


#ifdef USE_DEBUG
extern int current_mem_usage;
#endif
#define TBUFSIZE 4096

static void help (session *sess, char *tbuf, char *helpcmd, int quiet);
static int cmd_server (session *sess, char *tbuf, char *word[], char *word_eol[]);
static void handle_say (session *sess, char *text, int check_spch);


static void
notj_msg (struct session *sess)
{
	PrintText (sess, _("No channel joined. Try /join #<channel>\n"));
}

void
notc_msg (struct session *sess)
{
	PrintText (sess, _("Not connected. Try /server <host> [<port>]\n"));
}

static char *
random_line (char *file_name)
{
	FILE *fh;
	char buf[512];
	int lines, ran;

	if (!file_name[0])
		goto nofile;

	fh = xchat_fopen_file (file_name, "r", 0);
	if (!fh)
	{
	 nofile:
		/* reason is not a file, an actual reason! */
		return strdup (file_name);
	}

	/* count number of lines in file */
	lines = 0;
	while (fgets (buf, sizeof (buf), fh))
		lines++;

	if (lines < 1)
		goto nofile;

	/* go down a random number */
	rewind (fh);
	ran = RAND_INT (lines);
	do
	{
		fgets (buf, sizeof (buf), fh);
		lines--;
	}
	while (lines > ran);
	fclose (fh);
	buf[strlen (buf) - 1] = 0;	  /* remove the trailing '\n' */
	return strdup (buf);
}

void
server_sendpart (server * serv, char *channel, char *reason)
{
	if (!reason)
	{
		reason = random_line (prefs.partreason);
		serv->p_part (serv, channel, reason);
		free (reason);
	} else
	{
		/* reason set by /quit, /close argument */
		serv->p_part (serv, channel, reason);
	}
}

void
server_sendquit (session * sess)
{
	char *rea, *colrea;

	if (!sess->quitreason)
	{
		colrea = strdup (prefs.quitreason);
		check_special_chars (colrea, FALSE);
		rea = random_line (colrea);
		free (colrea);
		sess->server->p_quit (sess->server, rea);
		free (rea);
	} else
	{
		/* reason set by /quit, /close argument */
		sess->server->p_quit (sess->server, sess->quitreason);
	}
}

void
process_data_init (char *buf, char *cmd, char *word[],
						 char *word_eol[], gboolean handle_quotes,
						 gboolean allow_escape_quotes)
{
	int wordcount = 2;
	int space = FALSE;
	int quote = FALSE;
	int j = 0;
	int len;

	word[0] = "\000\000";
	word_eol[0] = "\000\000";
	word[1] = (char *)buf;
	word_eol[1] = (char *)cmd;

	while (1)
	{
		switch (*cmd)
		{
		case 0:
		 jump:
			buf[j] = 0;
			for (j = wordcount; j < PDIWORDS; j++)
			{
				word[j] = "\000\000";
				word_eol[j] = "\000\000";
			}
			return;
		case '\042':
			if (!handle_quotes)
				goto def;
			/* two quotes turn into 1 */
			if (allow_escape_quotes && cmd[1] == '\042')
			{
				cmd++;
				goto def;
			}
			if (quote)
			{
				quote = FALSE;
				space = FALSE;
			} else
				quote = TRUE;
			cmd++;
			break;
		case ' ':
			if (!quote)
			{
				if (!space)
				{
					buf[j] = 0;
					j++;

					word[wordcount] = &buf[j];
					word_eol[wordcount] = cmd + 1;
					wordcount++;

					if (wordcount == PDIWORDS - 1)
						goto jump;

					space = TRUE;
				}
				cmd++;
				break;
			}
		default:
def:
			space = FALSE;
			len = g_utf8_skip[((unsigned char *)cmd)[0]];
			if (len == 1)
			{
				buf[j] = *cmd;
				j++;
				cmd++;
			} else
			{
				/* skip past a multi-byte utf8 char */
				memcpy (buf + j, cmd, len);
				j += len;
				cmd += len;
			}
		}
	}
}

static int
cmd_addbutton (struct session *sess, char *tbuf, char *word[],
					char *word_eol[])
{
	if (*word[2] && *word_eol[3])
	{
		if (sess->type == SESS_DIALOG)
		{
			list_addentry (&dlgbutton_list, word_eol[3], word[2]);
			fe_dlgbuttons_update (sess);
		} else
		{
			list_addentry (&button_list, word_eol[3], word[2]);
			fe_buttons_update (sess);
		}
		return TRUE;
	}
	return FALSE;
}

static int
cmd_allchannels (session *sess, char *tbuf, char *word[], char *word_eol[])
{
	GSList *list = sess_list;

	if (!*word_eol[2])
		return FALSE;

	while (list)
	{
		sess = list->data;
		if (sess->type == SESS_CHANNEL && sess->channel[0] && sess->server->connected)
		{
			handle_command (sess, word_eol[2], FALSE);
		}
		list = list->next;
	}

	return TRUE;
}

static int
cmd_allchannelslocal (session *sess, char *tbuf, char *word[], char *word_eol[])
{
	GSList *list = sess_list;
	server *serv = sess->server;

	if (!*word_eol[2])
		return FALSE;

	while (list)
	{
		sess = list->data;
		if (sess->type == SESS_CHANNEL && sess->channel[0] &&
			 sess->server->connected && sess->server == serv)
		{
			handle_command (sess, word_eol[2], FALSE);
		}
		list = list->next;
	}

	return TRUE;
}

static int
cmd_allservers (struct session *sess, char *tbuf, char *word[],
					 char *word_eol[])
{
	GSList *list;
	server *serv;

	if (!*word_eol[2])
		return FALSE;

	list = serv_list;
	while (list)
	{
		serv = list->data;
		if (serv->connected)
			handle_command (serv->front_session, word_eol[2], FALSE);
		list = list->next;
	}

	return TRUE;
}

static int
cmd_away (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	GSList *list;
	char *reason = word_eol[2];
	int back = FALSE;
	unsigned int gone;

	if (!(*reason) && sess->server->is_away)
	{
		/* mark back */
		sess->server->p_set_back (sess->server);
		back = TRUE;
	} else
	{
		if (!(*reason))
		{
			if (sess->server->reconnect_away)
				reason = sess->server->last_away_reason;
			else
				/* must manage memory pointed to by random_line() */
				reason = random_line (prefs.awayreason);
		}
		sess->server->p_set_away (sess->server, reason);
	}

	if (prefs.show_away_message)
	{
		if (back)
		{
			gone = time (NULL) - sess->server->away_time;
			sprintf (tbuf, "me is back (gone %.2d:%.2d:%.2d)", gone / 3600,
						(gone / 60) % 60, gone % 60);
		} else
		{
			snprintf (tbuf, TBUFSIZE, "me is away: %s", reason);
		}

		list = sess_list;
		while (list)
		{
			/* am I the right server and not a dialog box */
			if (((struct session *) list->data)->server == sess->server
				 && ((struct session *) list->data)->type == SESS_CHANNEL
				 && ((struct session *) list->data)->channel[0])
			{
				handle_command ((session *) list->data, tbuf, TRUE);
			}
			list = list->next;
		}
	}

	if (sess->server->last_away_reason != reason)
	{
		free (sess->server->last_away_reason);
		if (reason == word_eol[2])
			sess->server->last_away_reason = strdup (reason);
		else
			sess->server->last_away_reason = reason;
	}

	return TRUE;
}

static void
ban (session * sess, char *tbuf, char *mask, char *bantypestr, int deop)
{
	int bantype;
	struct User *user;
	char *at, *dot, *lastdot;
	char username[64], fullhost[128], domain[128], *mode, *p2;
	server *serv = sess->server;

	user = userlist_find (sess, mask);
	if (user && user->hostname)  /* it's a nickname, let's find a proper ban mask */
	{
		if (deop)
		{
			mode = "-o+b ";
			p2 = user->nick;
		} else
		{
			mode = "+b";
			p2 = "";
		}

		mask = user->hostname;

		at = strchr (mask, '@');	/* FIXME: utf8 */
		if (!at)
			return;					  /* can't happen? */
		*at = 0;

		if (mask[0] == '~' || mask[0] == '+' ||
		    mask[0] == '=' || mask[0] == '^' || mask[0] == '-')
		{
			/* the ident is prefixed with something, we replace that sign with an * */
			safe_strcpy (username+1, mask+1, sizeof (username)-1);
			username[0] = '*';
		} else if (at - mask < USERNAMELEN)
		{
			/* we just add an * in the begining of the ident */
			safe_strcpy (username+1, mask, sizeof (username)-1);
			username[0] = '*';
		} else
		{
			/* ident might be too long, we just ban what it gives and add an * in the end */
			safe_strcpy (username, mask, sizeof (username));
		}
		*at = '@';
		safe_strcpy (fullhost, at + 1, sizeof (fullhost));

		dot = strchr (fullhost, '.');
		if (dot)
		{
			safe_strcpy (domain, dot, sizeof (domain));
		} else
		{
			safe_strcpy (domain, fullhost, sizeof (domain));
		}

		if (*bantypestr)
			bantype = atoi (bantypestr);
		else
			bantype = prefs.bantype;

		tbuf[0] = 0;
		if (inet_addr (fullhost) != -1)	/* "fullhost" is really a IP number */
		{
			lastdot = strrchr (fullhost, '.');
			if (!lastdot)
				return;				  /* can't happen? */

			*lastdot = 0;
			strcpy (domain, fullhost);
			*lastdot = '.';

			switch (bantype)
			{
			case 0:
				snprintf (tbuf, TBUFSIZE, "%s%s *!*@%s.*", mode, p2, domain);
				break;

			case 1:
				snprintf (tbuf, TBUFSIZE, "%s%s *!*@%s", mode, p2, fullhost);
				break;

			case 2:
				snprintf (tbuf, TBUFSIZE, "%s%s *!%s@%s.*", mode, p2, username, domain);
				break;

			case 3:
				snprintf (tbuf, TBUFSIZE, "%s%s *!%s@%s", mode, p2, username, fullhost);
				break;
			}
		} else
		{
			switch (bantype)
			{
			case 0:
				snprintf (tbuf, TBUFSIZE, "%s%s *!*@*%s", mode, p2, domain);
				break;

			case 1:
				snprintf (tbuf, TBUFSIZE, "%s%s *!*@%s", mode, p2, fullhost);
				break;

			case 2:
				snprintf (tbuf, TBUFSIZE, "%s%s *!%s@*%s", mode, p2, username, domain);
				break;

			case 3:
				snprintf (tbuf, TBUFSIZE, "%s%s *!%s@%s", mode, p2, username, fullhost);
				break;
			}
		}

	} else
	{
		snprintf (tbuf, TBUFSIZE, "+b %s", mask);
	}
	serv->p_mode (serv, sess->channel, tbuf);
}

static int
cmd_ban (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *mask = word[2];

	if (*mask)
	{
		ban (sess, tbuf, mask, word[3], 0);
	} else
	{
		sess->server->p_mode (sess->server, sess->channel, "+b");	/* banlist */
	}

	return TRUE;
}

static int
cmd_unban (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	/* Allow more than one mask in /unban -- tvk */
	int i = 2;

	while (1)
	{
		if (!*word[i])
		{
			if (i == 2)
				return FALSE;
			send_channel_modes (sess, tbuf, word, 2, i, '-', 'b', 0);
			return TRUE;
		}
		i++;
	}
}

static int
cmd_chanopt (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int state;
	if (!strcasecmp (word[3], "ON"))
	{
		state = TRUE;
	} else if (!strcasecmp (word[3], "OFF"))
	{
		state = FALSE;
	} else
	{
		if (!strcasecmp (word[2], "CONFMODE"))
			state = sess->hide_join_part;
		else if (!strcasecmp (word[2], "COLORPASTE"))
			state = sess->color_paste;
		else if (!strcasecmp (word[2], "BEEP"))
			state = sess->beep;
		else
			return FALSE;

		PrintTextf (sess, "%s is %s\n", word[2], state ? "ON" : "OFF");
		return TRUE;
	}

	if (!strcasecmp (word[2], "CONFMODE"))
	{
		sess->hide_join_part = state;
	} else if (!strcasecmp (word[2], "COLORPASTE"))
	{
		fe_set_color_paste (sess, state);
	} else if (!strcasecmp (word[2], "BEEP"))
	{
		sess->beep = state;
	}

	return TRUE;
}

static int
cmd_charset (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	server *serv = sess->server;
	const char *locale = NULL;
	int offset = 0;

	if (strcmp (word[2], "-quiet") == 0)
		offset++;

	if (!word[2 + offset][0])
	{
		g_get_charset (&locale);
		PrintTextf (sess, "Current charset: %s\n",
						serv->encoding ? serv->encoding : locale);
		return TRUE;
	}

	if (servlist_check_encoding (word[2 + offset]))
	{
		server_set_encoding (serv, word[2 + offset]);
		if (offset < 1)
			PrintTextf (sess, "Charset changed to: %s\n", word[2 + offset]);
	} else
	{
		PrintTextf (sess, "\0034Unknown charset:\017 %s\n", word[2 + offset]);
	}

	return TRUE;
}

static int
cmd_clear (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	GSList *list = sess_list;
	char *reason = word_eol[2];

	if (reason[0] == 0)
	{
		fe_text_clear (sess);
		return TRUE;
	}

	if (strcasecmp (reason, "HISTORY") == 0)
	{
		history_free (&sess->history);
		return TRUE;
	}

	if (strncasecmp (reason, "all", 3) == 0)
	{
		while (list)
		{
			sess = list->data;
			if (!sess->nick_said)
				fe_text_clear (list->data);
			list = list->next;
		}
		return TRUE;
	}

	return FALSE;
}

static int
cmd_close (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	GSList *list;

	if (strcmp (word[2], "-m") == 0)
	{
		list = sess_list;
		while (list)
		{
			sess = list->data;
			list = list->next;
			if (sess->type == SESS_DIALOG)
				fe_close_window (sess);
		}
	} else
	{
		if (*word_eol[2])
			sess->quitreason = word_eol[2];
		fe_close_window (sess);
	}

	return TRUE;
}

static int
cmd_ctcp (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int mbl;
	char *to = word[2];
	if (*to)
	{
		char *msg = word_eol[3];
		if (*msg)
		{
			unsigned char *cmd = (unsigned char *)msg;

			/* make the first word upper case (as per RFC) */
			while (1)
			{
				if (*cmd == ' ' || *cmd == 0)
					break;
				mbl = g_utf8_skip[*cmd];
				if (mbl == 1)
					*cmd = toupper (*cmd);
				cmd += mbl;
			}

			sess->server->p_ctcp (sess->server, to, msg);

			EMIT_SIGNAL (XP_TE_CTCPSEND, sess, to, msg, NULL, NULL, 0);

			return TRUE;
		}
	}
	return FALSE;
}

static int
cmd_country (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *code = word[2];
	if (*code)
	{
		/* search? */
		if (strcmp (code, "-s") == 0)
		{
			country_search (word[3], sess, (void *)PrintTextf);
			return TRUE;
		}

		/* search, but forgot the -s */
		if (strchr (code, '*'))
		{
			country_search (code, sess, (void *)PrintTextf);
			return TRUE;
		}

		sprintf (tbuf, "%s = %s\n", code, country (code));
		PrintText (sess, tbuf);
		return TRUE;
	}
	return FALSE;
}

static int
cmd_cycle (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *key = sess->channelkey;
	char *chan = sess->channel;
	if (*chan && sess->type == SESS_CHANNEL)
	{
		sess->server->p_cycle (sess->server, chan, key);
		return TRUE;
	}
	return FALSE;
}

static int
cmd_dcc (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int goodtype;
	struct DCC *dcc = 0;
	char *type = word[2];
	if (*type)
	{
		if (!strcasecmp (type, "HELP"))
			return FALSE;
		if (!strcasecmp (type, "CLOSE"))
		{
			if (*word[3] && *word[4])
			{
				goodtype = 0;
				if (!strcasecmp (word[3], "SEND"))
				{
					dcc = find_dcc (word[4], word[5], TYPE_SEND);
					dcc_abort (sess, dcc);
					goodtype = TRUE;
				}
				if (!strcasecmp (word[3], "GET"))
				{
					dcc = find_dcc (word[4], word[5], TYPE_RECV);
					dcc_abort (sess, dcc);
					goodtype = TRUE;
				}
				if (!strcasecmp (word[3], "CHAT"))
				{
					dcc = find_dcc (word[4], "", TYPE_CHATRECV);
					if (!dcc)
						dcc = find_dcc (word[4], "", TYPE_CHATSEND);
					dcc_abort (sess, dcc);
					goodtype = TRUE;
				}

				if (!goodtype)
					return FALSE;

				if (!dcc)
					EMIT_SIGNAL (XP_TE_NODCC, sess, NULL, NULL, NULL, NULL, 0);

				return TRUE;

			}
			return FALSE;
		}
		if ((!strcasecmp (type, "CHAT")) || (!strcasecmp (type, "PCHAT")))
		{
			char *nick = word[3];
			int passive = (!strcasecmp(type, "PCHAT")) ? 1 : 0;
			if (*nick)
				dcc_chat (sess, nick, passive);
			return TRUE;
		}
		if (!strcasecmp (type, "LIST"))
		{
			dcc_show_list (sess);
			return TRUE;
		}
		if (!strcasecmp (type, "GET"))
		{
			char *nick = word[3];
			char *file = word[4];
			if (!*file)
			{
				if (*nick)
					dcc_get_nick (sess, nick);
			} else
			{
				dcc = find_dcc (nick, file, TYPE_RECV);
				if (dcc)
					dcc_get (dcc);
				else
					EMIT_SIGNAL (XP_TE_NODCC, sess, NULL, NULL, NULL, NULL, 0);
			}
			return TRUE;
		}
		if ((!strcasecmp (type, "SEND")) || (!strcasecmp (type, "PSEND")))
		{
			int i = 3, maxcps;
			char *nick, *file;
			int passive = (!strcasecmp(type, "PSEND")) ? 1 : 0;

			nick = word[i];
			if (!*nick)
				return FALSE;

			maxcps = prefs.dcc_max_send_cps;
			if (!strncasecmp(nick, "-maxcps=", 8))
			{
				maxcps = atoi(nick + 8);
				i++;
				nick = word[i];
				if (!*nick)
					return FALSE;
			}

			i++;

			file = word[i];
			if (!*file)
			{
				fe_dcc_send_filereq (sess, nick, maxcps, passive);
				return TRUE;
			}

			do
			{
				dcc_send (sess, nick, file, maxcps, passive);
				i++;
				file = word[i];
			}
			while (*file);

			return TRUE;
		}

		return FALSE;
	}

	dcc_show_list (sess);
	return TRUE;
}

static int
cmd_debug (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	struct session *s;
	struct server *v;
	GSList *list = sess_list;

	PrintText (sess, "Session   T Channel    WaitChan  WillChan  Server\n");
	while (list)
	{
		s = (struct session *) list->data;
		sprintf (tbuf, "%p %1x %-10.10s %-10.10s %-10.10s %p\n",
					s, s->type, s->channel, s->waitchannel,
					s->willjoinchannel, s->server);
		PrintText (sess, tbuf);
		list = list->next;
	}

	list = serv_list;
	PrintText (sess, "Server    Sock  Name\n");
	while (list)
	{
		v = (struct server *) list->data;
		sprintf (tbuf, "%p %-5d %s\n",
					v, v->sok, v->servername);
		PrintText (sess, tbuf);
		list = list->next;
	}

	sprintf (tbuf,
				"\nfront_session: %p\n"
				"current_tab: %p\n\n",
				sess->server->front_session, current_tab);
	PrintText (sess, tbuf);
#ifdef USE_DEBUG
	sprintf (tbuf, "current mem: %d\n\n", current_mem_usage);
	PrintText (sess, tbuf);
#endif  /* !MEMORY_DEBUG */

	return TRUE;
}

static int
cmd_delbutton (struct session *sess, char *tbuf, char *word[],
					char *word_eol[])
{
	if (*word[2])
	{
		if (sess->type == SESS_DIALOG)
		{
			if (list_delentry (&dlgbutton_list, word[2]))
				fe_dlgbuttons_update (sess);
		} else
		{
			if (list_delentry (&button_list, word[2]))
				fe_buttons_update (sess);
		}
		return TRUE;
	}
	return FALSE;
}

static int
cmd_dehop (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 2;

	while (1)
	{
		if (!*word[i])
		{
			if (i == 2)
				return FALSE;
			send_channel_modes (sess, tbuf, word, 2, i, '-', 'h', 0);
			return TRUE;
		}
		i++;
	}
}

static int
cmd_deop (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 2;

	while (1)
	{
		if (!*word[i])
		{
			if (i == 2)
				return FALSE;
			send_channel_modes (sess, tbuf, word, 2, i, '-', 'o', 0);
			return TRUE;
		}
		i++;
	}
}

typedef struct
{
	char **nicks;
	int i;
	session *sess;
	char *reason;
	char *tbuf;
} multidata;

static int
mdehop_cb (struct User *user, multidata *data)
{
	if (user->hop && !user->me)
	{
		data->nicks[data->i] = user->nick;
		data->i++;
	}
	return TRUE;
}

static int
cmd_mdehop (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char **nicks = malloc (sizeof (char *) * sess->hops);
	multidata data;

	data.nicks = nicks;
	data.i = 0;
	tree_foreach (sess->usertree, (tree_traverse_func *)mdehop_cb, &data);
	send_channel_modes (sess, tbuf, nicks, 0, data.i, '-', 'h', 0);
	free (nicks);

	return TRUE;
}

static int
mdeop_cb (struct User *user, multidata *data)
{
	if (user->op && !user->me)
	{
		data->nicks[data->i] = user->nick;
		data->i++;
	}
	return TRUE;
}

static int
cmd_mdeop (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char **nicks = malloc (sizeof (char *) * sess->ops);
	multidata data;

	data.nicks = nicks;
	data.i = 0;
	tree_foreach (sess->usertree, (tree_traverse_func *)mdeop_cb, &data);
	send_channel_modes (sess, tbuf, nicks, 0, data.i, '-', 'o', 0);
	free (nicks);

	return TRUE;
}

GSList *menu_list = NULL;

static void
menu_free (menu_entry *me)
{
	free (me->path);
	if (me->label)
		free (me->label);
	if (me->cmd)
		free (me->cmd);
	if (me->ucmd)
		free (me->ucmd);
	free (me);
}

/* strings equal? but ignore underscores */

int
menu_streq (const char *s1, const char *s2, int def)
{
	/* for separators */
	if (s1 == NULL && s2 == NULL)
		return 0;
	if (s1 == NULL || s2 == NULL)
		return 1;
	while (*s1)
	{
		if (*s1 == '_')
			s1++;
		if (*s2 == '_')
			s2++;
		if (*s1 != *s2)
			return 1;
		s1++;
		s2++;
	}
	if (!*s2)
		return 0;
	return def;
}

static menu_entry *
menu_entry_find (char *path, char *label)
{
	GSList *list;
	menu_entry *me;

	list = menu_list;
	while (list)
	{
		me = list->data;
		if (!strcmp (path, me->path))
		{
			if (me->label && label && !strcmp (label, me->label))
				return me;
		}
		list = list->next;
	}
	return NULL;
}

static void
menu_del_children (char *path, char *label)
{
	GSList *list, *next;
	menu_entry *me;
	char buf[512];

	if (!label)
		label = "";
	if (path[0])
		snprintf (buf, sizeof (buf), "%s/%s", path, label);
	else
		snprintf (buf, sizeof (buf), "%s", label);

	list = menu_list;
	while (list)
	{
		me = list->data;
		next = list->next;
		if (!menu_streq (buf, me->path, 0))
		{
			menu_list = g_slist_remove (menu_list, me);
			menu_free (me);
		}
		list = next;
	}
}

static int
menu_del (char *path, char *label)
{
	GSList *list;
	menu_entry *me;

	list = menu_list;
	while (list)
	{
		me = list->data;
		if (!menu_streq (me->label, label, 1) && !menu_streq (me->path, path, 1))
		{
			menu_list = g_slist_remove (menu_list, me);
			fe_menu_del (me);
			menu_free (me);
			/* delete this item's children, if any */
			menu_del_children (path, label);
			return 1;
		}
		list = list->next;
	}

	return 0;
}

static void
menu_add (char *path, char *label, char *cmd, char *ucmd, int pos, int state, int markup, int enable, int mod, int key)
{
	menu_entry *me;

	/* already exists? */
	me = menu_entry_find (path, label);
	if (me)
	{
		/* update only */
		me->state = state;
		me->enable = enable;
		fe_menu_update (me);
		return;
	}

	me = malloc (sizeof (menu_entry));
	me->pos = pos;
	me->state = state;
	me->markup = markup;
	me->enable = enable;
	me->modifier = mod;
	me->key = key;
	me->path = strdup (path);
	me->label = NULL;
	me->cmd = NULL;
	me->ucmd = NULL;

	if (label)
		me->label = strdup (label);
	if (cmd)
		me->cmd = strdup (cmd);
	if (ucmd)
		me->ucmd = strdup (ucmd);

	menu_list = g_slist_append (menu_list, me);
	label = fe_menu_add (me);
	if (label)
	{
		/* FE has given us a stripped label */
		free (me->label);
		me->label = strdup (label);
		g_free (label); /* this is from pango */
	}
}

static int
cmd_menu (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int idx = 2;
	int len;
	int pos = -1;
	int state;
	int toggle = FALSE;
	int enable = TRUE;
	int markup = FALSE;
	int key = 0;
	int mod = 0;
	char *label;

	if (!word[2][0] || !word[3][0])
		return FALSE;

	/* -eX enabled or not? */
	if (word[idx][0] == '-' && word[idx][1] == 'e')
	{
		enable = atoi (word[idx] + 2);
		idx++;
	}

	/* -k<mod>,<key> key binding */
	if (word[idx][0] == '-' && word[idx][1] == 'k')
	{
		char *comma = strchr (word[idx], ',');
		if (!comma)
			return FALSE;
		mod = atoi (word[idx] + 2);
		key = atoi (comma + 1);
		idx++;
	}

	/* -m to specify PangoMarkup language */
	if (word[idx][0] == '-' && word[idx][1] == 'm')
	{
		markup = TRUE;
		idx++;
	}

	/* -pX to specify menu position */
	if (word[idx][0] == '-' && word[idx][1] == 'p')
	{
		pos = atoi (word[idx] + 2);
		idx++;
	}

	/* -tX to specify toggle item with default state */
	if (word[idx][0] == '-' && word[idx][1] == 't')
	{
		state = atoi (word[idx] + 2);
		idx++;
		toggle = TRUE;
	}

	/* the path */
	path_part (word[idx+1], tbuf, 512);
	len = strlen (tbuf);
	if (len)
		tbuf[len - 1] = 0;

	/* the name of the item */
	label = file_part (word[idx + 1]);
	if (label[0] == '-' && label[1] == 0)
		label = NULL;	/* separator */

	if (markup)
	{
		char *p;	/* to force pango closing tags through */
		for (p = label; *p; p++)
			if (*p == 3)
				*p = '/';
	}

	if (!strcasecmp (word[idx], "ADD"))
	{
		if (toggle)
		{
			menu_add (tbuf, label, word[idx + 2], word[idx + 3], pos, state, markup, enable, mod, key);
		} else
		{
			if (word[idx + 2][0])
				menu_add (tbuf, label, word[idx + 2], NULL, pos, 0, markup, enable, mod, key);
			else
				menu_add (tbuf, label, NULL, NULL, pos, 0, markup, enable, mod, key);
		}
		return TRUE;
	}

	if (!strcasecmp (word[idx], "DEL"))
	{
		menu_del (tbuf, label);
		return TRUE;
	}

	return FALSE;
}

static int
mkick_cb (struct User *user, multidata *data)
{
	if (!user->op && !user->me)
		data->sess->server->p_kick (data->sess->server, data->sess->channel, user->nick, data->reason);
	return TRUE;
}

static int
mkickops_cb (struct User *user, multidata *data)
{
	if (user->op && !user->me)
		data->sess->server->p_kick (data->sess->server, data->sess->channel, user->nick, data->reason);
	return TRUE;
}

static int
cmd_mkick (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	multidata data;

	data.sess = sess;
	data.reason = word_eol[2];
	tree_foreach (sess->usertree, (tree_traverse_func *)mkickops_cb, &data);
	tree_foreach (sess->usertree, (tree_traverse_func *)mkick_cb, &data);

	return TRUE;
}

static int
cmd_devoice (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 2;

	while (1)
	{
		if (!*word[i])
		{
			if (i == 2)
				return FALSE;
			send_channel_modes (sess, tbuf, word, 2, i, '-', 'v', 0);
			return TRUE;
		}
		i++;
	}
}

static int
cmd_discon (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	sess->server->disconnect (sess, TRUE, -1);
	return TRUE;
}

static int
cmd_dns (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
#ifdef WIN32
	PrintText (sess, "DNS is not implemented in Windows.\n");
	return TRUE;
#else
	char *nick = word[2];
	struct User *user;

	if (*nick)
	{
		if (strchr (nick, '.') == NULL)
		{
			user = userlist_find (sess, nick);
			if (user && user->hostname)
			{
				do_dns (sess, user->nick, user->hostname);
			} else
			{
				sess->server->p_get_ip (sess->server, nick);
				sess->server->doing_dns = TRUE;
			}
		} else
		{
			snprintf (tbuf, TBUFSIZE, "exec -d %s %s", prefs.dnsprogram, nick);
			handle_command (sess, tbuf, FALSE);
		}
		return TRUE;
	}
	return FALSE;
#endif
}

static int
cmd_echo (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	PrintText (sess, word_eol[2]);
	return TRUE;
}

#ifndef WIN32

static void
exec_check_process (struct session *sess)
{
	int val;

	if (sess->running_exec == NULL)
		return;
	val = waitpid (sess->running_exec->childpid, NULL, WNOHANG);
	if (val == -1 || val > 0)
	{
		close (sess->running_exec->myfd);
		fe_input_remove (sess->running_exec->iotag);
		free (sess->running_exec);
		sess->running_exec = NULL;
	}
}

#ifndef __EMX__
static int
cmd_execs (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int r;

	exec_check_process (sess);
	if (sess->running_exec == NULL)
	{
		EMIT_SIGNAL (XP_TE_NOCHILD, sess, NULL, NULL, NULL, NULL, 0);
		return FALSE;
	}
	r = kill (sess->running_exec->childpid, SIGSTOP);
	if (r == -1)
		PrintText (sess, "Error in kill(2)\n");

	return TRUE;
}

static int
cmd_execc (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int r;

	exec_check_process (sess);
	if (sess->running_exec == NULL)
	{
		EMIT_SIGNAL (XP_TE_NOCHILD, sess, NULL, NULL, NULL, NULL, 0);
		return FALSE;
	}
	r = kill (sess->running_exec->childpid, SIGCONT);
	if (r == -1)
		PrintText (sess, "Error in kill(2)\n");

	return TRUE;
}

static int
cmd_execk (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int r;

	exec_check_process (sess);
	if (sess->running_exec == NULL)
	{
		EMIT_SIGNAL (XP_TE_NOCHILD, sess, NULL, NULL, NULL, NULL, 0);
		return FALSE;
	}
	if (strcmp (word[2], "-9") == 0)
		r = kill (sess->running_exec->childpid, SIGKILL);
	else
		r = kill (sess->running_exec->childpid, SIGTERM);
	if (r == -1)
		PrintText (sess, "Error in kill(2)\n");

	return TRUE;
}

/* OS/2 Can't have the /EXECW command because it uses pipe(2) not socketpair
   and thus it is simplex --AGL */
static int
cmd_execw (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int len;
	char *temp;
	exec_check_process (sess);
	if (sess->running_exec == NULL)
	{
		EMIT_SIGNAL (XP_TE_NOCHILD, sess, NULL, NULL, NULL, NULL, 0);
		return FALSE;
	}
	len = strlen(word_eol[2]);
	temp = malloc(len + 2);
	sprintf(temp, "%s\n", word_eol[2]);
	PrintText(sess, temp);
	write(sess->running_exec->myfd, temp, len + 1);
	free(temp);

	return TRUE;
}
#endif /* !__EMX__ */

/* convert ANSI escape color codes to mIRC codes */

static short escconv[] =
/* 0 1 2 3 4 5  6 7  0 1 2 3 4  5  6  7 */
{  1,4,3,5,2,10,6,1, 1,7,9,8,12,11,13,1 };

static void
exec_handle_colors (char *buf, int len)
{
	char numb[16];
	char *nbuf;
	int i = 0, j = 0, k = 0, firstn = 0, col, colf = 0, colb = 0;
	int esc = FALSE, backc = FALSE, bold = FALSE;

	/* any escape codes in this text? */
	if (strchr (buf, 27) == 0)
		return;

	nbuf = malloc (len + 1);

	while (i < len)
	{
		switch (buf[i])
		{
		case '\r':
			break;
		case 27:
			esc = TRUE;
			break;
		case ';':
			if (!esc)
				goto norm;
			backc = TRUE;
			numb[k] = 0;
			firstn = atoi (numb);
			k = 0;
			break;
		case '[':
			if (!esc)
				goto norm;
			break;
		default:
			if (esc)
			{
				if (buf[i] >= 'A' && buf[i] <= 'z')
				{
					if (buf[i] == 'm')
					{
						/* ^[[0m */
						if (k == 0 || (numb[0] == '0' && k == 1))
						{
							nbuf[j] = '\017';
							j++;
							bold = FALSE;
							goto cont;
						}

						numb[k] = 0;
						col = atoi (numb);
						backc = FALSE;

						if (firstn == 1)
							bold = TRUE;

						if (firstn >= 30 && firstn <= 37)
							colf = firstn - 30;

						if (col >= 40)
						{
							colb = col - 40;
							backc = TRUE;
						}

						if (col >= 30 && col <= 37)
							colf = col - 30;

						if (bold)
							colf += 8;

						if (backc)
						{
							colb = escconv[colb % 14];
							colf = escconv[colf % 14];
							j += sprintf (&nbuf[j], "\003%d,%02d", colf, colb);
						} else
						{
							colf = escconv[colf % 14];
							j += sprintf (&nbuf[j], "\003%02d", colf);
						}
					}
cont:				esc = FALSE;
					backc = FALSE;
					k = 0;
				} else
				{
					if (isdigit ((unsigned char) buf[i]) && k < (sizeof (numb) - 1))
					{
						numb[k] = buf[i];
						k++;
					}
				}
			} else
			{
norm:			nbuf[j] = buf[i];
				j++;
			}
		}
		i++;
	}

	nbuf[j] = 0;
	memcpy (buf, nbuf, j + 1);
	free (nbuf);
}

#ifndef HAVE_MEMRCHR
static void *
memrchr (const void *block, int c, size_t size)
{
	unsigned char *p;

	for (p = (unsigned char *)block + size; p != block; p--)
		if (*p == c)
			return p;
	return 0;
}
#endif

static gboolean
exec_data (GIOChannel *source, GIOCondition condition, struct nbexec *s)
{
	char *buf, *readpos, *rest;
	int rd, len;
	int sok = s->myfd;

	len = s->buffill;
	if (len) {
		/* append new data to buffered incomplete line */
		buf = malloc(len + 2050);
		memcpy(buf, s->linebuf, len);
		readpos = buf + len;
		free(s->linebuf);
		s->linebuf = NULL;
	}
	else
		readpos = buf = malloc(2050);

	rd = read (sok, readpos, 2048);
	if (rd < 1)
	{
		/* The process has died */
		kill(s->childpid, SIGKILL);
		if (len) {
			buf[len] = '\0';
			exec_handle_colors(buf, len);
			if (s->tochannel)
			{
				/* must turn off auto-completion temporarily */
				unsigned int old = prefs.nickcompletion;
				prefs.nickcompletion = 0;
				handle_multiline (s->sess, buf, FALSE, TRUE);
				prefs.nickcompletion = old;
			}
			else
				PrintText (s->sess, buf);
		}
		free(buf);
		waitpid (s->childpid, NULL, 0);
		s->sess->running_exec = NULL;
		fe_input_remove (s->iotag);
		close (sok);
		free (s);
		return TRUE;
	}
	len += rd;
	buf[len] = '\0';

	rest = memrchr(buf, '\n', len);
	if (rest)
		rest++;
	else
		rest = buf;
	if (*rest) {
		s->buffill = len - (rest - buf); /* = strlen(rest) */
		s->linebuf = malloc(s->buffill);
		memcpy(s->linebuf, rest, s->buffill);
		*rest = '\0';
		len -= s->buffill; /* possibly 0 */
	}
	else
		s->buffill = 0;

	if (len) {
		exec_handle_colors (buf, len);
		if (s->tochannel)
			handle_multiline (s->sess, buf, FALSE, TRUE);
		else
			PrintText (s->sess, buf);
	}

	free(buf);
	return TRUE;
}

static int
cmd_exec (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int tochannel = FALSE;
	char *cmd = word_eol[2];
	int fds[2], pid = 0;
	struct nbexec *s;
	int shell = TRUE;
	int fd;

	if (*cmd)
	{
		exec_check_process (sess);
		if (sess->running_exec != NULL)
		{
			EMIT_SIGNAL (XP_TE_ALREADYPROCESS, sess, NULL, NULL, NULL, NULL, 0);
			return TRUE;
		}

		if (!strcmp (word[2], "-d"))
		{
			if (!*word[3])
				return FALSE;
			cmd = word_eol[3];
			shell = FALSE;
		}
		else if (!strcmp (word[2], "-o"))
		{
			if (!*word[3])
				return FALSE;
			cmd = word_eol[3];
			tochannel = TRUE;
		}

		if (shell)
		{
			if (access ("/bin/sh", X_OK) != 0)
			{
				fe_message (_("I need /bin/sh to run!\n"), FE_MSG_ERROR);
				return TRUE;
			}
		}

#ifdef __EMX__						  /* if os/2 */
		if (pipe (fds) < 0)
		{
			PrintText (sess, "Pipe create error\n");
			return FALSE;
		}
		setmode (fds[0], O_BINARY);
		setmode (fds[1], O_BINARY);
#else
		if (socketpair (PF_UNIX, SOCK_STREAM, 0, fds) == -1)
		{
			PrintText (sess, "socketpair(2) failed\n");
			return FALSE;
		}
#endif
		s = (struct nbexec *) malloc (sizeof (struct nbexec));
		memset(s, 0, sizeof(*s));
		s->myfd = fds[0];
		s->tochannel = tochannel;
		s->sess = sess;

		pid = fork ();
		if (pid == 0)
		{
			/* This is the child's context */
			close (0);
			close (1);
			close (2);
			/* Close parent's end of pipe */
			close(s->myfd);
			/* Copy the child end of the pipe to stdout and stderr */
			dup2 (fds[1], 1);
			dup2 (fds[1], 2);
			/* Also copy it to stdin so we can write to it */
			dup2 (fds[1], 0);
			/* Now close all open file descriptors except stdin, stdout and stderr */
			for (fd = 3; fd < 1024; fd++) close(fd);
			/* Now we call /bin/sh to run our cmd ; made it more friendly -DC1 */
			if (shell)
			{
				execl ("/bin/sh", "sh", "-c", cmd, NULL);
			} else
			{
				char **argv;
				int argc;

				my_poptParseArgvString (cmd, &argc, &argv);
				execvp (argv[0], argv);
			}
			/* not reached unless error */
			/*printf("exec error\n");*/
			fflush (stdout);
			fflush (stdin);
			_exit (0);
		}
		if (pid == -1)
		{
			/* Parent context, fork() failed */

			PrintText (sess, "Error in fork(2)\n");
			close(fds[0]);
			close(fds[1]);
		} else
		{
			/* Parent path */
			close(fds[1]);
			s->childpid = pid;
			s->iotag = fe_input_add (s->myfd, FIA_READ|FIA_EX, exec_data, s);
			sess->running_exec = s;
			return TRUE;
		}
	}
	return FALSE;
}

#endif

static int
cmd_flushq (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	sprintf (tbuf, "Flushing server send queue, %d bytes.\n", sess->server->sendq_len);
	PrintText (sess, tbuf);
	sess->server->flush_queue (sess->server);
	return TRUE;
}

static int
cmd_quit (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word_eol[2])
		sess->quitreason = word_eol[2];
	sess->server->disconnect (sess, TRUE, -1);
	sess->quitreason = NULL;
	return 2;
}

static int
cmd_gate (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *server_name = word[2];
	server *serv = sess->server;
	if (*server_name)
	{
		char *port = word[3];
#ifdef USE_OPENSSL
		serv->use_ssl = FALSE;
#endif
		server_fill_her_up (serv);
		if (*port)
			serv->connect (serv, server_name, atoi (port), TRUE);
		else
			serv->connect (serv, server_name, 23, TRUE);
		return TRUE;
	}
	return FALSE;
}

typedef struct
{
	char *cmd;
	session *sess;
} getvalinfo;

static void
get_int_cb (int cancel, int val, getvalinfo *info)
{
	char buf[512];

	if (!cancel)
	{
		snprintf (buf, sizeof (buf), "%s %d", info->cmd, val);
		if (is_session (info->sess))
			handle_command (info->sess, buf, FALSE);
	}

	free (info->cmd);
	free (info);
}

static int
cmd_getint (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	getvalinfo *info;

	if (!word[4][0])
		return FALSE;

	info = malloc (sizeof (*info));
	info->cmd = strdup (word[3]);
	info->sess = sess;

	fe_get_int (word[4], atoi (word[2]), get_int_cb, info);

	return TRUE;
}

static void
get_file_cb (char *cmd, char *file)
{
	char buf[1024 + 128];

	/* execute the command once per file, then once more with
      no args */
	if (file)
	{
		snprintf (buf, sizeof (buf), "%s %s", cmd, file);
		handle_command (current_sess, buf, FALSE);
	}
	else
	{
		handle_command (current_sess, cmd, FALSE);
		free (cmd);
	}
}

static int
cmd_getfile (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int idx = 2;
	int flags = 0;

	if (!word[3][0])
		return FALSE;

	if (!strcmp (word[2], "-folder"))
	{
		flags |= FRF_CHOOSEFOLDER;
		idx++;
	}

	if (!strcmp (word[idx], "-multi"))
	{
		flags |= FRF_MULTIPLE;
		idx++;
	}

	if (!strcmp (word[idx], "-save"))
	{
		flags |= FRF_WRITE;
		idx++;
	}

	fe_get_file (word[idx+1], word[idx+2], (void *)get_file_cb, strdup (word[idx]), flags);

	return TRUE;
}

static void
get_str_cb (int cancel, char *val, getvalinfo *info)
{
	char buf[512];

	if (!cancel)
	{
		snprintf (buf, sizeof (buf), "%s %s", info->cmd, val);
		if (is_session (info->sess))
			handle_command (info->sess, buf, FALSE);
	}

	free (info->cmd);
	free (info);
}

static int
cmd_getstr (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	getvalinfo *info;

	if (!word[4][0])
		return FALSE;

	info = malloc (sizeof (*info));
	info->cmd = strdup (word[3]);
	info->sess = sess;

	fe_get_str (word[4], word[2], get_str_cb, info);

	return TRUE;
}

static int
cmd_ghost (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (word[3][0])
	{
		sess->server->p_ns_ghost (sess->server, word[2], word[3]);
		return TRUE;
	}

	return FALSE; 
}

static int
cmd_gui (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	switch (str_ihash (word[2]))
	{
	case 0xac1eee45: fe_ctrl_gui (sess, 7, 2); break; /* ATTACH */
	case 0x05a72f63: fe_ctrl_gui (sess, 4, atoi (word[3])); break; /* COLOR */
	case 0xb06a1793: fe_ctrl_gui (sess, 7, 1); break; /* DETACH */
	case 0x05cfeff0: fe_ctrl_gui (sess, 3, 0); break; /* FLASH */
	case 0x05d154d8: fe_ctrl_gui (sess, 2, 0); break; /* FOCUS */
	case 0x0030dd42: fe_ctrl_gui (sess, 0, 0); break; /* HIDE */
	case 0x61addbe3: fe_ctrl_gui (sess, 5, 0); break; /* ICONIFY */
	case 0xc0851aaa: fe_message (word[3], FE_MSG_INFO|FE_MSG_MARKUP); break; /* MSGBOX */
	case 0x0035dafd: fe_ctrl_gui (sess, 1, 0); break; /* SHOW */
	case 0x0033155f: /* MENU */
		if (!strcasecmp (word[3], "TOGGLE"))
			fe_ctrl_gui (sess, 6, 0);
		else
			return FALSE;
		break;
	default:
		return FALSE;
	}

	return TRUE;
}

typedef struct
{
	int longfmt;
	int i, t;
	char *buf;
} help_list;

static void
show_help_line (session *sess, help_list *hl, char *name, char *usage)
{
	int j, len, max;
	char *p;

	if (name[0] == '.')	/* hidden command? */
		return;

	if (hl->longfmt)	/* long format for /HELP -l */
	{
		if (!usage || usage[0] == 0)
			PrintTextf (sess, "   \0034%s\003 :\n", name);
		else
			PrintTextf (sess, "   \0034%s\003 : %s\n", name, _(usage));
		return;
	}

	/* append the name into buffer, but convert to uppercase */
	len = strlen (hl->buf);
	p = name;
	while (*p)
	{
		hl->buf[len] = toupper ((unsigned char) *p);
		len++;
		p++;
	}
	hl->buf[len] = 0;

	hl->t++;
	if (hl->t == 5)
	{
		hl->t = 0;
		strcat (hl->buf, "\n");
		PrintText (sess, hl->buf);
		hl->buf[0] = ' ';
		hl->buf[1] = ' ';
		hl->buf[2] = 0;
	} else
	{
		/* append some spaces after the command name */
		max = strlen (name);
		if (max < 10)
		{
			max = 10 - max;
			for (j = 0; j < max; j++)
			{
				hl->buf[len] = ' ';
				len++;
				hl->buf[len] = 0;
			}
		}
	}
}

static int
cmd_help (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 0, longfmt = 0;
	char *helpcmd = "";
	GSList *list;

	if (tbuf)
		helpcmd = word[2];
	if (*helpcmd && strcmp (helpcmd, "-l") == 0)
		longfmt = 1;

	if (*helpcmd && !longfmt)
	{
		help (sess, tbuf, helpcmd, FALSE);
	} else
	{
		struct popup *pop;
		char *buf = malloc (4096);
		help_list hl;

		hl.longfmt = longfmt;
		hl.buf = buf;

		PrintTextf (sess, "\n%s\n\n", _("Commands Available:"));
		buf[0] = ' ';
		buf[1] = ' ';
		buf[2] = 0;
		hl.t = 0;
		hl.i = 0;
		while (xc_cmds[i].name)
		{
			show_help_line (sess, &hl, xc_cmds[i].name, xc_cmds[i].help);
			i++;
		}
		strcat (buf, "\n");
		PrintText (sess, buf);

		PrintTextf (sess, "\n%s\n\n", _("User defined commands:"));
		buf[0] = ' ';
		buf[1] = ' ';
		buf[2] = 0;
		hl.t = 0;
		hl.i = 0;
		list = command_list;
		while (list)
		{
			pop = list->data;
			show_help_line (sess, &hl, pop->name, pop->cmd);
			list = list->next;
		}
		strcat (buf, "\n");
		PrintText (sess, buf);

		PrintTextf (sess, "\n%s\n\n", _("Plugin defined commands:"));
		buf[0] = ' ';
		buf[1] = ' ';
		buf[2] = 0;
		hl.t = 0;
		hl.i = 0;
		plugin_command_foreach (sess, &hl, (void *)show_help_line);
		strcat (buf, "\n");
		PrintText (sess, buf);
		free (buf);

		PrintTextf (sess, "\n%s\n\n", _("Type /HELP <command> for more information, or /HELP -l"));
	}
	return TRUE;
}

static int
cmd_id (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (word[2][0])
	{
		sess->server->p_ns_identify (sess->server, word[2]);
		return TRUE;
	}

	return FALSE;
}

static int
cmd_ignore (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i;
	int type = 0;
	int quiet = 0;

	if (!*word[2])
	{
		ignore_showlist (sess);
		return TRUE;
	}
	if (!*word[3])
		return FALSE;

	i = 3;
	while (1)
	{
		if (!*word[i])
		{
			if (type == 0)
				return FALSE;
			i = ignore_add (word[2], type);
			if (quiet)
				return TRUE;
			switch (i)
			{
			case 1:
				EMIT_SIGNAL (XP_TE_IGNOREADD, sess, word[2], NULL, NULL, NULL, 0);
				break;
			case 2:	/* old ignore changed */
				EMIT_SIGNAL (XP_TE_IGNORECHANGE, sess, word[2], NULL, NULL,
								 NULL, 0);
			}
			return TRUE;
		}
		if (!strcasecmp (word[i], "UNIGNORE"))
			type |= IG_UNIG;
		else if (!strcasecmp (word[i], "ALL"))
			type |= IG_PRIV | IG_NOTI | IG_CHAN | IG_CTCP | IG_INVI | IG_DCC;
		else if (!strcasecmp (word[i], "PRIV"))
			type |= IG_PRIV;
		else if (!strcasecmp (word[i], "NOTI"))
			type |= IG_NOTI;
		else if (!strcasecmp (word[i], "CHAN"))
			type |= IG_CHAN;
		else if (!strcasecmp (word[i], "CTCP"))
			type |= IG_CTCP;
		else if (!strcasecmp (word[i], "INVI"))
			type |= IG_INVI;
		else if (!strcasecmp (word[i], "QUIET"))
			quiet = 1;
		else if (!strcasecmp (word[i], "NOSAVE"))
			type |= IG_NOSAVE;
		else if (!strcasecmp (word[i], "DCC"))
			type |= IG_DCC;
		else
		{
			sprintf (tbuf, _("Unknown arg '%s' ignored."), word[i]);
			PrintText (sess, tbuf);
		}
		i++;
	}
}

static int
cmd_invite (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (!*word[2])
		return FALSE;
	if (*word[3])
		sess->server->p_invite (sess->server, word[3], word[2]);
	else
		sess->server->p_invite (sess->server, sess->channel, word[2]);
	return TRUE;
}

static int
cmd_join (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *chan = word[2];
	if (*chan)
	{
		char *po, *pass = word[3];
		sess->server->p_join (sess->server, chan, pass);
		if (sess->channel[0] == 0 && sess->waitchannel[0])
		{
			po = strchr (chan, ',');
			if (po)
				*po = 0;
			safe_strcpy (sess->waitchannel, chan, CHANLEN);
		}
		return TRUE;
	}
	return FALSE;
}

static int
cmd_kick (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *nick = word[2];
	char *reason = word_eol[3];
	if (*nick)
	{
		sess->server->p_kick (sess->server, sess->channel, nick, reason);
		return TRUE;
	}
	return FALSE;
}

static int
cmd_kickban (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *nick = word[2];
	char *reason = word_eol[3];
	struct User *user;

	if (*nick)
	{
		/* if the reason is a 1 digit number, treat it as a bantype */

		user = userlist_find (sess, nick);

		if (isdigit ((unsigned char) reason[0]) && reason[1] == 0)
		{
			ban (sess, tbuf, nick, reason, (user && user->op));
			reason[0] = 0;
		} else
			ban (sess, tbuf, nick, "", (user && user->op));

		sess->server->p_kick (sess->server, sess->channel, nick, reason);

		return TRUE;
	}
	return FALSE;
}

static int
cmd_killall (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	xchat_exit();
	return 2;
}

static int
cmd_lagcheck (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	lag_check ();
	return TRUE;
}

static void
lastlog (session *sess, char *search, gboolean regexp)
{
	session *lastlog_sess;

	if (!is_session (sess))
		return;

	lastlog_sess = find_dialog (sess->server, "(lastlog)");
	if (!lastlog_sess)
		lastlog_sess = new_ircwindow (sess->server, "(lastlog)", SESS_DIALOG, 0);

	lastlog_sess->lastlog_sess = sess;
	lastlog_sess->lastlog_regexp = regexp;	/* remember the search type */

	fe_text_clear (lastlog_sess);
	fe_lastlog (sess, lastlog_sess, search, regexp);
}

static int
cmd_lastlog (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word_eol[2])
	{
		if (!strcmp (word[2], "-r"))
			lastlog (sess, word_eol[3], TRUE);
		else
			lastlog (sess, word_eol[2], FALSE);
		return TRUE;
	}

	return FALSE;
}

static int
cmd_list (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	sess->server->p_list_channels (sess->server, word_eol[2]);

	return TRUE;
}

static int
cmd_load (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	FILE *fp;
	char *error, *arg, *nl, *file;
	int len;

	if (!word[2][0])
		return FALSE;

	if (strcmp (word[2], "-e") == 0)
	{
		file = expand_homedir (word[3]);
		fp = xchat_fopen_file (file, "r", XOF_FULLPATH);
		if (!fp)
		{
			PrintTextf (sess, _("Cannot access %s\n"), file);
			PrintText (sess, errorstring (errno));
			free (file);
			return TRUE;
		}
		free (file);

		tbuf[1024] = 0;
		while (fgets (tbuf, 1024, fp))
		{
			nl = strchr (tbuf, '\n');
			if (nl == tbuf) /* skip empty commands */
				continue;
			if (nl)
				*nl = 0;
			if (tbuf[0] == prefs.cmdchar[0])
				handle_command (sess, tbuf + 1, TRUE);
			else
				handle_command (sess, tbuf, TRUE);
		}
		fclose (fp);
		return TRUE;
	}

#ifdef USE_PLUGIN
	len = strlen (word[2]);
#ifdef WIN32
	if (len > 4 && strcasecmp (".dll", word[2] + len - 4) == 0)
#else
#if defined(__hpux)
	if (len > 3 && strcasecmp (".sl", word[2] + len - 3) == 0)
#else
	if (len > 3 && strcasecmp (".so", word[2] + len - 3) == 0)
#endif
#endif
	{
		arg = NULL;
		if (word_eol[3][0])
			arg = word_eol[3];

		file = expand_homedir (word[2]);
		error = plugin_load (sess, file, arg);
		free (file);

		if (error)
			PrintText (sess, error);

		return TRUE;
	}
#endif

	sprintf (tbuf, "Unknown file type %s. Maybe you need to install the Perl or Python plugin?\n", word[2]);
	PrintText (sess, tbuf);

	return FALSE;
}

static int
cmd_me (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *act = word_eol[2];

	if (!(*act))
		return FALSE;

	if (sess->type == SESS_SERVER)
	{
		notj_msg (sess);
		return TRUE;
	}

	snprintf (tbuf, TBUFSIZE, "\001ACTION %s\001\r", act);
	/* first try through DCC CHAT */
	if (dcc_write_chat (sess->channel, tbuf))
	{
		/* print it to screen */
		inbound_action (sess, sess->channel, sess->server->nick, act, TRUE, FALSE);
	} else
	{
		/* DCC CHAT failed, try through server */
		if (sess->server->connected)
		{
			sess->server->p_action (sess->server, sess->channel, act);
			/* print it to screen */
			inbound_action (sess, sess->channel, sess->server->nick, act, TRUE, FALSE);
		} else
		{
			notc_msg (sess);
		}
	}

	return TRUE;
}

static int
cmd_mode (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	/* +channel channels are dying, let those servers whine about modes.
	 * return info about current channel if available and no info is given */
	if ((*word[2] == '+') || (*word[2] == 0) || (!is_channel(sess->server, word[2]) &&
				!(rfc_casecmp(sess->server->nick, word[2]) == 0)))
	{
		if(sess->channel[0] == 0)
			return FALSE;
		sess->server->p_mode (sess->server, sess->channel, word_eol[2]);
	}
	else
		sess->server->p_mode (sess->server, word[2], word_eol[3]);
	return TRUE;
}

static int
mop_cb (struct User *user, multidata *data)
{
	if (!user->op)
	{
		data->nicks[data->i] = user->nick;
		data->i++;
	}
	return TRUE;
}

static int
cmd_mop (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char **nicks = malloc (sizeof (char *) * (sess->total - sess->ops));
	multidata data;

	data.nicks = nicks;
	data.i = 0;
	tree_foreach (sess->usertree, (tree_traverse_func *)mop_cb, &data);
	send_channel_modes (sess, tbuf, nicks, 0, data.i, '+', 'o', 0);

	free (nicks);

	return TRUE;
}

static int
cmd_msg (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *nick = word[2];
	char *msg = word_eol[3];
	struct session *newsess;

	if (*nick)
	{
		if (*msg)
		{
			if (strcmp (nick, ".") == 0)
			{							  /* /msg the last nick /msg'ed */
				if (sess->lastnick[0])
					nick = sess->lastnick;
			} else
			{
				safe_strcpy (sess->lastnick, nick, NICKLEN);	/* prime the last nick memory */
			}

			if (*nick == '=')
			{
				nick++;
				if (!dcc_write_chat (nick, msg))
				{
					EMIT_SIGNAL (XP_TE_NODCC, sess, NULL, NULL, NULL, NULL, 0);
					return TRUE;
				}
			} else
			{
				if (!sess->server->connected)
				{
					notc_msg (sess);
					return TRUE;
				}
				sess->server->p_message (sess->server, nick, msg);
			}
			newsess = find_dialog (sess->server, nick);
			if (!newsess)
				newsess = find_channel (sess->server, nick);
			if (newsess)
				inbound_chanmsg (newsess->server, NULL, newsess->channel,
									  newsess->server->nick, msg, TRUE, FALSE);
			else
			{
				/* mask out passwords */
				if (strcasecmp (nick, "nickserv") == 0 &&
					 strncasecmp (msg, "identify ", 9) == 0)
					msg = "identify ****";
				EMIT_SIGNAL (XP_TE_MSGSEND, sess, nick, msg, NULL, NULL, 0);
			}

			return TRUE;
		}
	}
	return FALSE;
}

static int
cmd_names (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word[2])
	  	sess->server->p_names (sess->server, word[2]);
	else
		sess->server->p_names (sess->server, sess->channel);
	return TRUE;
}

static int
cmd_nctcp (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word_eol[3])
	{
		sess->server->p_nctcp (sess->server, word[2], word_eol[3]);
		return TRUE;
	}
	return FALSE;
}

static int
cmd_newserver (struct session *sess, char *tbuf, char *word[],
					char *word_eol[])
{
	if (strcmp (word[2], "-noconnect") == 0)
	{
		new_ircwindow (NULL, word[3], SESS_SERVER, 0);
		return TRUE;
	}
	
	sess = new_ircwindow (NULL, NULL, SESS_SERVER, 0);
	cmd_server (sess, tbuf, word, word_eol);
	return TRUE;
}

static int
cmd_nick (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *nick = word[2];
	if (*nick)
	{
		if (sess->server->connected)
			sess->server->p_change_nick (sess->server, nick);
		else
			inbound_newnick (sess->server, sess->server->nick, nick, TRUE);
		return TRUE;
	}
	return FALSE;
}

static int
cmd_notice (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word[2] && *word_eol[3])
	{
		sess->server->p_notice (sess->server, word[2], word_eol[3]);
		EMIT_SIGNAL (XP_TE_NOTICESEND, sess, word[2], word_eol[3], NULL, NULL, 0);
		return TRUE;
	}
	return FALSE;
}

static int
cmd_notify (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 1;
	char *net = NULL;

	if (*word[2])
	{
		if (strcmp (word[2], "-n") == 0)	/* comma sep network list */
		{
			net = word[3];
			i += 2;
		}

		while (1)
		{
			i++;
			if (!*word[i])
				break;
			if (notify_deluser (word[i]))
			{
				EMIT_SIGNAL (XP_TE_DELNOTIFY, sess, word[i], NULL, NULL, NULL, 0);
				return TRUE;
			}
			notify_adduser (word[i], net);
			EMIT_SIGNAL (XP_TE_ADDNOTIFY, sess, word[i], NULL, NULL, NULL, 0);
		}
	} else
		notify_showlist (sess);
	return TRUE;
}

static int
cmd_op (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 2;

	while (1)
	{
		if (!*word[i])
		{
			if (i == 2)
				return FALSE;
			send_channel_modes (sess, tbuf, word, 2, i, '+', 'o', 0);
			return TRUE;
		}
		i++;
	}
}

static int
cmd_part (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *chan = word[2];
	char *reason = word_eol[3];
	if (!*chan)
		chan = sess->channel;
	if ((*chan) && is_channel (sess->server, chan))
	{
		if (reason[0] == 0)
			reason = NULL;
		server_sendpart (sess->server, chan, reason);
		return TRUE;
	}
	return FALSE;
}

static int
cmd_ping (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char timestring[64];
	unsigned long tim;
	char *to = word[2];

	tim = make_ping_time ();

	snprintf (timestring, sizeof (timestring), "%lu", tim);
	sess->server->p_ping (sess->server, to, timestring);

	return TRUE;
}

void
open_query (server *serv, char *nick, gboolean focus_existing)
{
	session *sess;

	sess = find_dialog (serv, nick);
	if (!sess)
		new_ircwindow (serv, nick, SESS_DIALOG, 1);
	else if (focus_existing)
		fe_ctrl_gui (sess, 2, 0);	/* bring-to-front */
}

static int
cmd_query (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *nick = word[2];
	gboolean focus = TRUE;

	if (strcmp (word[2], "-nofocus") == 0)
	{
		nick = word[3];
		focus = FALSE;
	}

	if (*nick && !is_channel (sess->server, nick))
	{
		open_query (sess->server, nick, focus);
		return TRUE;
	}
	return FALSE;
}

static int
cmd_quote (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *raw = word_eol[2];

	return sess->server->p_raw (sess->server, raw);
}

static int
cmd_reconnect (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int tmp = prefs.recon_delay;
	GSList *list;
	server *serv = sess->server;

	prefs.recon_delay = 0;

	if (!strcasecmp (word[2], "ALL"))
	{
		list = serv_list;
		while (list)
		{
			serv = list->data;
			if (serv->connected)
				serv->auto_reconnect (serv, TRUE, -1);
			list = list->next;
		}
	}
	/* If it isn't "ALL" and there is something
	there it *should* be a server they are trying to connect to*/
	else if (*word[2])
	{
		int offset = 0;
#ifdef USE_OPENSSL
		int use_ssl = FALSE;

		if (strcmp (word[2], "-ssl") == 0)
		{
			use_ssl = TRUE;
			offset++;	/* args move up by 1 word */
		}
		serv->use_ssl = use_ssl;
		serv->accept_invalid_cert = TRUE;
#endif

		if (*word[4+offset])
			safe_strcpy (serv->password, word[4+offset], sizeof (serv->password));
		if (*word[3+offset])
			serv->port = atoi (word[3+offset]);
		safe_strcpy (serv->hostname, word[2+offset], sizeof (serv->hostname));
		serv->auto_reconnect (serv, TRUE, -1);
	}
	else
	{
		serv->auto_reconnect (serv, TRUE, -1);
	}
	prefs.recon_delay = tmp;

	return TRUE;
}

static int
cmd_recv (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word_eol[2])
	{
		sess->server->p_inline (sess->server, word_eol[2], strlen (word_eol[2]));
		return TRUE;
	}

	return FALSE;
}

static int
cmd_say (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *speech = word_eol[2];
	if (*speech)
	{
		handle_say (sess, speech, FALSE);
		return TRUE;
	}
	return FALSE;
}

static int
cmd_send (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	guint32 addr;
	socklen_t len;
	struct sockaddr_in SAddr;

	if (!word[2][0])
		return FALSE;

	addr = dcc_get_my_address ();
	if (addr == 0)
	{
		/* use the one from our connected server socket */
		memset (&SAddr, 0, sizeof (struct sockaddr_in));
		len = sizeof (SAddr);
		getsockname (sess->server->sok, (struct sockaddr *) &SAddr, &len);
		addr = SAddr.sin_addr.s_addr;
	}
	addr = ntohl (addr);

	if ((addr & 0xffff0000) == 0xc0a80000 ||	/* 192.168.x.x */
		 (addr & 0xff000000) == 0x0a000000)		/* 10.x.x.x */
		/* we got a private net address, let's PSEND or it'll fail */
		snprintf (tbuf, 512, "DCC PSEND %s", word_eol[2]);
	else
		snprintf (tbuf, 512, "DCC SEND %s", word_eol[2]);

	handle_command (sess, tbuf, FALSE);

	return TRUE;
}

static int
cmd_setcursor (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int delta = FALSE;

	if (*word[2])
	{
		if (word[2][0] == '-' || word[2][0] == '+')
			delta = TRUE;
		fe_set_inputbox_cursor (sess, delta, atoi (word[2]));
		return TRUE;
	}

	return FALSE;
}

static int
cmd_settab (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word_eol[2])
	{
		strcpy (tbuf, sess->channel);
		safe_strcpy (sess->channel, word_eol[2], CHANLEN);
		fe_set_channel (sess);
		strcpy (sess->channel, tbuf);
	}

	return TRUE;
}

static int
cmd_settext (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word_eol[2])
	{
		fe_set_inputbox_contents (sess, word_eol[2]);
		return TRUE;
	}

	return FALSE;
}

static int
cmd_splay (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word[2])
	{
		sound_play (word[2], FALSE);
		return TRUE;
	}

	return FALSE;
}

static int
parse_irc_url (char *url, char *server_name[], char *port[], char *channel[], int *use_ssl)
{
	char *co;
#ifdef USE_OPENSSL
	if (strncasecmp ("ircs://", url, 7) == 0)
	{
		*use_ssl = TRUE;
		*server_name = url + 7;
		goto urlserv;
	}
#endif

	if (strncasecmp ("irc://", url, 6) == 0)
	{
		*server_name = url + 6;
#ifdef USE_OPENSSL
urlserv:
#endif
		/* check for port */
		co = strchr (*server_name, ':');
		if (co)
		{
			*port = co + 1;
			*co = 0;
		} else
			co = *server_name;
		/* check for channel - mirc style */
		co = strchr (co + 1, '/');
		if (co)
		{
			*co = 0;
			co++;
			if (*co == '#')
				*channel = co+1;
			else
				*channel = co;
			
		}
		return TRUE;
	}
	return FALSE;
}

static int
cmd_server (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int offset = 0;
	char *server_name = NULL;
	char *port = NULL;
	char *pass = NULL;
	char *channel = NULL;
	int use_ssl = FALSE;
	int is_url = TRUE;
	server *serv = sess->server;

#ifdef USE_OPENSSL
	/* BitchX uses -ssl, mIRC uses -e, let's support both */
	if (strcmp (word[2], "-ssl") == 0 || strcmp (word[2], "-e") == 0)
	{
		use_ssl = TRUE;
		offset++;	/* args move up by 1 word */
	}
#endif

	if (!parse_irc_url (word[2 + offset], &server_name, &port, &channel, &use_ssl))
	{
		is_url = FALSE;
		server_name = word[2 + offset];
	}
	if (port)
		pass = word[3 + offset];
	else
	{
		port = word[3 + offset];
		pass = word[4 + offset];
	}
	
	if (!(*server_name))
		return FALSE;

	sess->server->network = NULL;

	/* dont clear it for /servchan */
	if (strncasecmp (word_eol[1], "SERVCHAN ", 9))
		sess->willjoinchannel[0] = 0;

	if (channel)
	{
		sess->willjoinchannel[0] = '#';
		safe_strcpy ((sess->willjoinchannel + 1), channel, (CHANLEN - 1));
	}

	/* support +7000 style ports like mIRC */
	if (port[0] == '+')
	{
		port++;
#ifdef USE_OPENSSL
		use_ssl = TRUE;
#endif
	}

	if (*pass)
	{
		safe_strcpy (serv->password, pass, sizeof (serv->password));
	}
#ifdef USE_OPENSSL
	serv->use_ssl = use_ssl;
	serv->accept_invalid_cert = TRUE;
#endif

	/* try to connect by Network name */
	if (servlist_connect_by_netname (sess, server_name, !is_url))
		return TRUE;

	if (*port)
	{
		serv->connect (serv, server_name, atoi (port), FALSE);
	} else
	{
		/* -1 for default port */
		serv->connect (serv, server_name, -1, FALSE);
	}

	/* try to associate this connection with a listed network */
	if (!serv->network)
		/* search for this hostname in the entire server list */
		serv->network = servlist_net_find_from_server (server_name);
		/* may return NULL, but that's OK */

	return TRUE;
}

static int
cmd_servchan (struct session *sess, char *tbuf, char *word[],
				  char *word_eol[])
{
	int offset = 0;

#ifdef USE_OPENSSL
	if (strcmp (word[2], "-ssl") == 0)
		offset++;
#endif

	if (*word[4 + offset])
	{
		safe_strcpy (sess->willjoinchannel, word[4 + offset], CHANLEN);
		return cmd_server (sess, tbuf, word, word_eol);
	}

	return FALSE;
}

static int
cmd_topic (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (word[2][0] && is_channel (sess->server, word[2]))
		sess->server->p_topic (sess->server, word[2], word_eol[3]);
	else
		sess->server->p_topic (sess->server, sess->channel, word_eol[2]);
	return TRUE;
}

static int
cmd_unignore (struct session *sess, char *tbuf, char *word[],
				  char *word_eol[])
{
	char *mask = word[2];
	char *arg = word[3];
	if (*mask)
	{
		if (ignore_del (mask, NULL))
		{
			if (strcasecmp (arg, "QUIET"))
				EMIT_SIGNAL (XP_TE_IGNOREREMOVE, sess, mask, NULL, NULL, NULL, 0);
		}
		return TRUE;
	}
	return FALSE;
}

static int
cmd_unload (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
#ifdef USE_PLUGIN
	int len, by_file = FALSE;

	len = strlen (word[2]);
#ifdef WIN32
	if (len > 4 && strcasecmp (word[2] + len - 4, ".dll") == 0)
#else
#if defined(__hpux)
	if (len > 3 && strcasecmp (word[2] + len - 3, ".sl") == 0)
#else
	if (len > 3 && strcasecmp (word[2] + len - 3, ".so") == 0)
#endif
#endif
		by_file = TRUE;

	switch (plugin_kill (word[2], by_file))
	{
	case 0:
			PrintText (sess, _("No such plugin found.\n"));
			break;
	case 1:
			return TRUE;
	case 2:
			PrintText (sess, _("That plugin is refusing to unload.\n"));
			break;
	}
#endif

	return FALSE;
}

static server *
find_server_from_hostname (char *hostname)
{
	GSList *list = serv_list;
	server *serv;

	while (list)
	{
		serv = list->data;
		if (!strcasecmp (hostname, serv->hostname) && serv->connected)
			return serv;
		list = list->next;
	}

	return NULL;
}

static server *
find_server_from_net (void *net)
{
	GSList *list = serv_list;
	server *serv;

	while (list)
	{
		serv = list->data;
		if (serv->network == net && serv->connected)
			return serv;
		list = list->next;
	}

	return NULL;
}

static void
url_join_only (server *serv, char *tbuf, char *channel)
{
	/* already connected, JOIN only. FIXME: support keys? */
	tbuf[0] = '#';
	/* tbuf is 4kb */
	safe_strcpy ((tbuf + 1), channel, 256);
	serv->p_join (serv, tbuf, "");
}

static int
cmd_url (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (word[2][0])
	{
		char *server_name = NULL;
		char *port = NULL;
		char *channel = NULL;
		char *url = g_strdup (word[2]);
		int use_ssl = FALSE;
		void *net;
		server *serv;

		if (parse_irc_url (url, &server_name, &port, &channel, &use_ssl))
		{
			/* maybe we're already connected to this net */

			/* check for "FreeNode" */
			net = servlist_net_find (server_name, NULL, strcasecmp);
			/* check for "irc.eu.freenode.net" */
			if (!net)
				net = servlist_net_find_from_server (server_name);

			if (net)
			{
				/* found the network, but are we connected? */
				serv = find_server_from_net (net);
				if (serv)
				{
					url_join_only (serv, tbuf, channel);
					g_free (url);
					return TRUE;
				}
			}
			else
			{
				/* an un-listed connection */
				serv = find_server_from_hostname (server_name);
				if (serv)
				{
					url_join_only (serv, tbuf, channel);
					g_free (url);
					return TRUE;
				}
			}

			/* not connected to this net, open new window */
			cmd_newserver (sess, tbuf, word, word_eol);

		} else
			fe_open_url (word[2]);
		g_free (url);
		return TRUE;
	}

	return FALSE;
}

static int
userlist_cb (struct User *user, session *sess)
{
	time_t lt;

	if (!user->lasttalk)
		lt = 0;
	else
		lt = time (0) - user->lasttalk;
	PrintTextf (sess,
				"\00306%s\t\00314[\00310%-38s\00314] \017ov\0033=\017%d%d away=%u lt\0033=\017%d\n",
				user->nick, user->hostname, user->op, user->voice, user->away, lt);

	return TRUE;
}

static int
cmd_uselect (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int idx = 2;
	int clear = TRUE;
	int scroll = FALSE;

	if (strcmp (word[2], "-a") == 0)	/* ADD (don't clear selections) */
	{
		clear = FALSE;
		idx++;
	}
	if (strcmp (word[idx], "-s") == 0)	/* SCROLL TO */
	{
		scroll = TRUE;
		idx++;
	}
	/* always valid, no args means clear the selection list */
	fe_uselect (sess, word + idx, clear, scroll);
	return TRUE;
}

static int
cmd_userlist (struct session *sess, char *tbuf, char *word[],
				  char *word_eol[])
{
	tree_foreach (sess->usertree, (tree_traverse_func *)userlist_cb, sess);
	return TRUE;
}

static int
wallchop_cb (struct User *user, multidata *data)
{
	if (user->op)
	{
		if (data->i)
			strcat (data->tbuf, ",");
		strcat (data->tbuf, user->nick);
		data->i++;
	}
	if (data->i == 5)
	{
		data->i = 0;
		sprintf (data->tbuf + strlen (data->tbuf),
					" :[@%s] %s", data->sess->channel, data->reason);
		data->sess->server->p_raw (data->sess->server, data->tbuf);
		strcpy (data->tbuf, "NOTICE ");
	}

	return TRUE;
}

static int
cmd_wallchop (struct session *sess, char *tbuf, char *word[],
				  char *word_eol[])
{
	multidata data;

	if (!(*word_eol[2]))
		return FALSE;

	strcpy (tbuf, "NOTICE ");

	data.reason = word_eol[2];
	data.tbuf = tbuf;
	data.i = 0;
	data.sess = sess;
	tree_foreach (sess->usertree, (tree_traverse_func*)wallchop_cb, &data);

	if (data.i)
	{
		sprintf (tbuf + strlen (tbuf),
					" :[@%s] %s", sess->channel, word_eol[2]);
		sess->server->p_raw (sess->server, tbuf);
	}

	return TRUE;
}

static int
cmd_wallchan (struct session *sess, char *tbuf, char *word[],
				  char *word_eol[])
{
	GSList *list;

	if (*word_eol[2])
	{
		list = sess_list;
		while (list)
		{
			sess = list->data;
			if (sess->type == SESS_CHANNEL)
			{
				inbound_chanmsg (sess->server, NULL, sess->channel,
									  sess->server->nick, word_eol[2], TRUE, FALSE);
				sess->server->p_message (sess->server, sess->channel, word_eol[2]);
			}
			list = list->next;
		}
		return TRUE;
	}
	return FALSE;
}

static int
cmd_hop (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 2;

	while (1)
	{
		if (!*word[i])
		{
			if (i == 2)
				return FALSE;
			send_channel_modes (sess, tbuf, word, 2, i, '+', 'h', 0);
			return TRUE;
		}
		i++;
	}
}

static int
cmd_voice (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 2;

	while (1)
	{
		if (!*word[i])
		{
			if (i == 2)
				return FALSE;
			send_channel_modes (sess, tbuf, word, 2, i, '+', 'v', 0);
			return TRUE;
		}
		i++;
	}
}

/* *MUST* be kept perfectly sorted for the bsearch to work */
const struct commands xc_cmds[] = {
	{"ADDBUTTON", cmd_addbutton, 0, 0, 1,
	 N_("ADDBUTTON <name> <action>, adds a button under the user-list")},
	{"ALLCHAN", cmd_allchannels, 0, 0, 1,
	 N_("ALLCHAN <cmd>, sends a command to all channels you're in")},
	{"ALLCHANL", cmd_allchannelslocal, 0, 0, 1,
	 N_("ALLCHANL <cmd>, sends a command to all channels you're in")},
	{"ALLSERV", cmd_allservers, 0, 0, 1,
	 N_("ALLSERV <cmd>, sends a command to all servers you're in")},
	{"AWAY", cmd_away, 1, 0, 1, N_("AWAY [<reason>], sets you away")},
	{"BAN", cmd_ban, 1, 1, 1,
	 N_("BAN <mask> [<bantype>], bans everyone matching the mask from the current channel. If they are already on the channel this doesn't kick them (needs chanop)")},
	{"CHANOPT", cmd_chanopt, 0, 0, 1,
	 N_("Set per channel options\n"
	 "CHANOPT CONFMODE ON|OFF - Toggle conf mode/showing of join and part messages\n"
	 "CHANOPT COLORPASTE ON|OFF - Toggle color paste\n"
	 "CHANOPT BEEP ON|OFF - Toggle beep on message"
	)},
	{"CHARSET", cmd_charset, 0, 0, 1, 0},
	{"CLEAR", cmd_clear, 0, 0, 1, N_("CLEAR [ALL|HISTORY], Clears the current text window or command history")},
	{"CLOSE", cmd_close, 0, 0, 1, N_("CLOSE, Closes the current window/tab")},

	{"COUNTRY", cmd_country, 0, 0, 1,
	 N_("COUNTRY [-s] <code|wildcard>, finds a country code, eg: au = australia")},
	{"CTCP", cmd_ctcp, 1, 0, 1,
	 N_("CTCP <nick> <message>, send the CTCP message to nick, common messages are VERSION and USERINFO")},
	{"CYCLE", cmd_cycle, 1, 1, 1,
	 N_("CYCLE, parts current channel and immediately rejoins")},
	{"DCC", cmd_dcc, 0, 0, 1,
	 N_("\n"
	 "DCC GET <nick>                      - accept an offered file\n"
	 "DCC SEND [-maxcps=#] <nick> [file]  - send a file to someone\n"
	 "DCC PSEND [-maxcps=#] <nick> [file] - send a file using passive mode\n"
	 "DCC LIST                            - show DCC list\n"
	 "DCC CHAT <nick>                     - offer DCC CHAT to someone\n"
	 "DCC PCHAT <nick>                    - offer DCC CHAT using passive mode\n"
	 "DCC CLOSE <type> <nick> <file>         example:\n"
	 "         /dcc close send johnsmith file.tar.gz")},
	{"DEBUG", cmd_debug, 0, 0, 1, 0},

	{"DEHOP", cmd_dehop, 1, 1, 1,
	 N_("DEHOP <nick>, removes chanhalf-op status from the nick on the current channel (needs chanop)")},
	{"DELBUTTON", cmd_delbutton, 0, 0, 1,
	 N_("DELBUTTON <name>, deletes a button from under the user-list")},
	{"DEOP", cmd_deop, 1, 1, 1,
	 N_("DEOP <nick>, removes chanop status from the nick on the current channel (needs chanop)")},
	{"DEVOICE", cmd_devoice, 1, 1, 1,
	 N_("DEVOICE <nick>, removes voice status from the nick on the current channel (needs chanop)")},
	{"DISCON", cmd_discon, 0, 0, 1, N_("DISCON, Disconnects from server")},
	{"DNS", cmd_dns, 0, 0, 1, N_("DNS <nick|host|ip>, Finds a users IP number")},
	{"ECHO", cmd_echo, 0, 0, 1, N_("ECHO <text>, Prints text locally")},
#ifndef WIN32
	{"EXEC", cmd_exec, 0, 0, 1,
	 N_("EXEC [-o] <command>, runs the command. If -o flag is used then output is sent to current channel, else is printed to current text box")},
#ifndef __EMX__
	{"EXECCONT", cmd_execc, 0, 0, 1, N_("EXECCONT, sends the process SIGCONT")},
#endif
	{"EXECKILL", cmd_execk, 0, 0, 1,
	 N_("EXECKILL [-9], kills a running exec in the current session. If -9 is given the process is SIGKILL'ed")},
#ifndef __EMX__
	{"EXECSTOP", cmd_execs, 0, 0, 1, N_("EXECSTOP, sends the process SIGSTOP")},
	{"EXECWRITE", cmd_execw, 0, 0, 1, N_("EXECWRITE, sends data to the processes stdin")},
#endif
#endif
	{"FLUSHQ", cmd_flushq, 0, 0, 1,
	 N_("FLUSHQ, flushes the current server's send queue")},
	{"GATE", cmd_gate, 0, 0, 1,
	 N_("GATE <host> [<port>], proxies through a host, port defaults to 23")},
	{"GETINT", cmd_getint, 0, 0, 1, "GETINT <default> <command> <prompt>"},
	{"GETFILE", cmd_getfile, 0, 0, 1, "GETFILE [-folder] [-multi] [-save] <command> <title> [<initial>]"},
	{"GETSTR", cmd_getstr, 0, 0, 1, "GETSTR <default> <command> <prompt>"},
	{"GHOST", cmd_ghost, 1, 0, 1, N_("GHOST <nick> <password>, Kills a ghosted nickname")},
	{"GUI", cmd_gui, 0, 0, 1, "GUI [ATTACH|DETACH|SHOW|HIDE|FOCUS|FLASH|ICONIFY|COLOR <n>]\n"
									  "       GUI [MSGBOX <text>|MENU TOGGLE]"},
	{"HELP", cmd_help, 0, 0, 1, 0},
	{"HOP", cmd_hop, 1, 1, 1,
	 N_("HOP <nick>, gives chanhalf-op status to the nick (needs chanop)")},
	{"ID", cmd_id, 1, 0, 1, N_("ID <password>, identifies yourself to nickserv")},
	{"IGNORE", cmd_ignore, 0, 0, 1,
	 N_("IGNORE <mask> <types..> <options..>\n"
	 "    mask - host mask to ignore, eg: *!*@*.aol.com\n"
	 "    types - types of data to ignore, one or all of:\n"
	 "            PRIV, CHAN, NOTI, CTCP, DCC, INVI, ALL\n"
	 "    options - NOSAVE, QUIET")},

	{"INVITE", cmd_invite, 1, 0, 1,
	 N_("INVITE <nick> [<channel>], invites someone to a channel, by default the current channel (needs chanop)")},
	{"JOIN", cmd_join, 1, 0, 0, N_("JOIN <channel>, joins the channel")},
	{"KICK", cmd_kick, 1, 1, 1,
	 N_("KICK <nick>, kicks the nick from the current channel (needs chanop)")},
	{"KICKBAN", cmd_kickban, 1, 1, 1,
	 N_("KICKBAN <nick>, bans then kicks the nick from the current channel (needs chanop)")},
	{"KILLALL", cmd_killall, 0, 0, 1, "KILLALL, immediately exit"},
	{"LAGCHECK", cmd_lagcheck, 0, 0, 1,
	 N_("LAGCHECK, forces a new lag check")},
	{"LASTLOG", cmd_lastlog, 0, 0, 1,
	 N_("LASTLOG <string>, searches for a string in the buffer")},
	{"LIST", cmd_list, 1, 0, 1, 0},
	{"LOAD", cmd_load, 0, 0, 1, N_("LOAD [-e] <file>, loads a plugin or script")},

	{"MDEHOP", cmd_mdehop, 1, 1, 1,
	 N_("MDEHOP, Mass deop's all chanhalf-ops in the current channel (needs chanop)")},
	{"MDEOP", cmd_mdeop, 1, 1, 1,
	 N_("MDEOP, Mass deop's all chanops in the current channel (needs chanop)")},
	{"ME", cmd_me, 0, 0, 1,
	 N_("ME <action>, sends the action to the current channel (actions are written in the 3rd person, like /me jumps)")},
	{"MENU", cmd_menu, 0, 0, 1, "MENU [-eX] [-k<mod>,<key>] [-m] [-pX] [-tX] {ADD|DEL} <path> [command] [unselect command]"},
	{"MKICK", cmd_mkick, 1, 1, 1,
	 N_("MKICK, Mass kicks everyone except you in the current channel (needs chanop)")},
	{"MODE", cmd_mode, 1, 0, 1, 0},
	{"MOP", cmd_mop, 1, 1, 1,
	 N_("MOP, Mass op's all users in the current channel (needs chanop)")},
	{"MSG", cmd_msg, 0, 0, 1, N_("MSG <nick> <message>, sends a private message")},

	{"NAMES", cmd_names, 1, 0, 1,
	 N_("NAMES, Lists the nicks on the current channel")},
	{"NCTCP", cmd_nctcp, 1, 0, 1,
	 N_("NCTCP <nick> <message>, Sends a CTCP notice")},
	{"NEWSERVER", cmd_newserver, 0, 0, 1, N_("NEWSERVER [-noconnect] <hostname> [<port>]")},
	{"NICK", cmd_nick, 0, 0, 1, N_("NICK <nickname>, sets your nick")},

	{"NOTICE", cmd_notice, 1, 0, 1,
	 N_("NOTICE <nick/channel> <message>, sends a notice. Notices are a type of message that should be auto reacted to")},
	{"NOTIFY", cmd_notify, 0, 0, 1,
	 N_("NOTIFY [-n network1[,network2,...]] [<nick>], displays your notify list or adds someone to it")},
	{"OP", cmd_op, 1, 1, 1,
	 N_("OP <nick>, gives chanop status to the nick (needs chanop)")},
	{"PART", cmd_part, 1, 1, 1,
	 N_("PART [<channel>] [<reason>], leaves the channel, by default the current one")},
	{"PING", cmd_ping, 1, 0, 1,
	 N_("PING <nick | channel>, CTCP pings nick or channel")},
	{"QUERY", cmd_query, 0, 0, 1,
	 N_("QUERY [-nofocus] <nick>, opens up a new privmsg window to someone")},
	{"QUIT", cmd_quit, 0, 0, 1,
	 N_("QUIT [<reason>], disconnects from the current server")},
	{"QUOTE", cmd_quote, 1, 0, 1,
	 N_("QUOTE <text>, sends the text in raw form to the server")},
#ifdef USE_OPENSSL
	{"RECONNECT", cmd_reconnect, 0, 0, 1,
	 N_("RECONNECT [-ssl] [<host>] [<port>] [<password>], Can be called just as /RECONNECT to reconnect to the current server or with /RECONNECT ALL to reconnect to all the open servers")},
#else
	{"RECONNECT", cmd_reconnect, 0, 0, 1,
	 N_("RECONNECT [<host>] [<port>] [<password>], Can be called just as /RECONNECT to reconnect to the current server or with /RECONNECT ALL to reconnect to all the open servers")},
#endif
	{"RECV", cmd_recv, 1, 0, 1, N_("RECV <text>, send raw data to xchat, as if it was received from the irc server")},

	{"SAY", cmd_say, 0, 0, 1,
	 N_("SAY <text>, sends the text to the object in the current window")},
	{"SEND", cmd_send, 0, 0, 1, N_("SEND <nick> [<file>]")},
#ifdef USE_OPENSSL
	{"SERVCHAN", cmd_servchan, 0, 0, 1,
	 N_("SERVCHAN [-ssl] <host> <port> <channel>, connects and joins a channel")},
#else
	{"SERVCHAN", cmd_servchan, 0, 0, 1,
	 N_("SERVCHAN <host> <port> <channel>, connects and joins a channel")},
#endif
#ifdef USE_OPENSSL
	{"SERVER", cmd_server, 0, 0, 1,
	 N_("SERVER [-ssl] <host> [<port>] [<password>], connects to a server, the default port is 6667 for normal connections, and 9999 for ssl connections")},
#else
	{"SERVER", cmd_server, 0, 0, 1,
	 N_("SERVER <host> [<port>] [<password>], connects to a server, the default port is 6667")},
#endif
	{"SET", cmd_set, 0, 0, 1, N_("SET [-e] [-quiet] <variable> [<value>]")},
	{"SETCURSOR", cmd_setcursor, 0, 0, 1, N_("SETCURSOR [-|+]<position>")},
	{"SETTAB", cmd_settab, 0, 0, 1, 0},
	{"SETTEXT", cmd_settext, 0, 0, 1, 0},
	{"SPLAY", cmd_splay, 0, 0, 1, "SPLAY <soundfile>"},
	{"TOPIC", cmd_topic, 1, 1, 1,
	 N_("TOPIC [<topic>], sets the topic if one is given, else shows the current topic")},
	{"UNBAN", cmd_unban, 1, 1, 1,
	 N_("UNBAN <mask> [<mask>...], unbans the specified masks.")},
	{"UNIGNORE", cmd_unignore, 0, 0, 1, N_("UNIGNORE <mask> [QUIET]")},
	{"UNLOAD", cmd_unload, 0, 0, 1, N_("UNLOAD <name>, unloads a plugin or script")},
	{"URL", cmd_url, 0, 0, 1, N_("URL <url>, opens a URL in your browser")},
	{"USELECT", cmd_uselect, 0, 1, 0,
	 N_("USELECT [-a] [-s] <nick1> <nick2> etc, highlights nick(s) in channel userlist")},
	{"USERLIST", cmd_userlist, 1, 1, 1, 0},
	{"VOICE", cmd_voice, 1, 1, 1,
	 N_("VOICE <nick>, gives voice status to someone (needs chanop)")},
	{"WALLCHAN", cmd_wallchan, 1, 1, 1,
	 N_("WALLCHAN <message>, writes the message to all channels")},
	{"WALLCHOP", cmd_wallchop, 1, 1, 1,
	 N_("WALLCHOP <message>, sends the message to all chanops on the current channel")},
	{0, 0, 0, 0, 0, 0}
};


static int
command_compare (const void *a, const void *b)
{
	return strcasecmp (a, ((struct commands *)b)->name);
}

static struct commands *
find_internal_command (char *name)
{
	/* the "-1" is to skip the NULL terminator */
	return bsearch (name, xc_cmds, (sizeof (xc_cmds) /
				sizeof (xc_cmds[0])) - 1, sizeof (xc_cmds[0]), command_compare);
}

static void
help (session *sess, char *tbuf, char *helpcmd, int quiet)
{
	struct commands *cmd;

	if (plugin_show_help (sess, helpcmd))
		return;

	cmd = find_internal_command (helpcmd);

	if (cmd)
	{
		if (cmd->help)
		{
			snprintf (tbuf, TBUFSIZE, _("Usage: %s\n"), _(cmd->help));
			PrintText (sess, tbuf);
		} else
		{
			if (!quiet)
				PrintText (sess, _("\nNo help available on that command.\n"));
		}
		return;
	}

	if (!quiet)
		PrintText (sess, _("No such command.\n"));
}

/* inserts %a, %c, %d etc into buffer. Also handles &x %x for word/word_eol. *
 *   returns 2 on buffer overflow
 *   returns 1 on success                                                    *
 *   returns 0 on bad-args-for-user-command                                  *
 * - word/word_eol args might be NULL                                        *
 * - this beast is used for UserCommands, UserlistButtons and CTCP replies   */

int
auto_insert (char *dest, int destlen, unsigned char *src, char *word[],
				 char *word_eol[], char *a, char *c, char *d, char *e, char *h,
				 char *n, char *s)
{
	int num;
	char buf[32];
	time_t now;
	struct tm *tm_ptr;
	char *utf;
	gsize utf_len;
	char *orig = dest;

	destlen--;

	while (src[0])
	{
		if (src[0] == '%' || src[0] == '&')
		{
			if (isdigit ((unsigned char) src[1]))
			{
				if (isdigit ((unsigned char) src[2]) && isdigit ((unsigned char) src[3]))
				{
					buf[0] = src[1];
					buf[1] = src[2];
					buf[2] = src[3];
					buf[3] = 0;
					dest[0] = atoi (buf);
					utf = g_locale_to_utf8 (dest, 1, 0, &utf_len, 0);
					if (utf)
					{
						if ((dest - orig) + utf_len >= destlen)
						{
							g_free (utf);
							return 2;
						}

						memcpy (dest, utf, utf_len);
						g_free (utf);
						dest += utf_len;
					}
					src += 3;
				} else
				{
					if (word)
					{
						src++;
						num = src[0] - '0';	/* ascii to decimal */
						if (*word[num] == 0)
							return 0;

						if (src[-1] == '%')
							utf = word[num];
						else
							utf = word_eol[num];

						/* avoid recusive usercommand overflow */
						if ((dest - orig) + strlen (utf) >= destlen)
							return 2;

						strcpy (dest, utf);
						dest += strlen (dest);
					}
				}
			} else
			{
				if (src[0] == '&')
					goto lamecode;
				src++;
				utf = NULL;
				switch (src[0])
				{
				case '%':
					if ((dest - orig) + 2 >= destlen)
						return 2;
					dest[0] = '%';
					dest[1] = 0;
					break;
				case 'a':
					utf = a; break;
				case 'c':
					utf = c; break;
				case 'd':
					utf = d; break;
				case 'e':
					utf = e; break;
				case 'h':
					utf = h; break;
				case 'm':
					utf = get_cpu_str (); break;
				case 'n':
					utf = n; break;
				case 's':
					utf = s; break;
				case 't':
					now = time (0);
					utf = ctime (&now);
					utf[19] = 0;
					break;
				case 'v':
					utf = PACKAGE_VERSION; break;
					break;
				case 'y':
					now = time (0);
					tm_ptr = localtime (&now);
					snprintf (buf, sizeof (buf), "%4d%02d%02d", 1900 +
								 tm_ptr->tm_year, 1 + tm_ptr->tm_mon, tm_ptr->tm_mday);
					utf = buf;
					break;
				default:
					src--;
					goto lamecode;
				}

				if (utf)
				{
					if ((dest - orig) + strlen (utf) >= destlen)
						return 2;
					strcpy (dest, utf);
					dest += strlen (dest);
				}

			}
			src++;
		} else
		{
			utf_len = g_utf8_skip[src[0]];

			if ((dest - orig) + utf_len >= destlen)
				return 2;

			if (utf_len == 1)
			{
		 lamecode:
				dest[0] = src[0];
				dest++;
				src++;
			} else
			{
				memcpy (dest, src, utf_len);
				dest += utf_len;
				src += utf_len;
			}
		}
	}

	dest[0] = 0;

	return 1;
}

void
check_special_chars (char *cmd, int do_ascii) /* check for %X */
{
	int occur = 0;
	int len = strlen (cmd);
	char *buf, *utf;
	char tbuf[4];
	int i = 0, j = 0;
	gsize utf_len;

	if (!len)
		return;

	buf = malloc (len + 1);

	if (buf)
	{
		while (cmd[j])
		{
			switch (cmd[j])
			{
			case '%':
				occur++;
				if (	do_ascii &&
						j + 3 < len &&
						(isdigit ((unsigned char) cmd[j + 1]) && isdigit ((unsigned char) cmd[j + 2]) &&
						isdigit ((unsigned char) cmd[j + 3])))
				{
					tbuf[0] = cmd[j + 1];
					tbuf[1] = cmd[j + 2];
					tbuf[2] = cmd[j + 3];
					tbuf[3] = 0;
					buf[i] = atoi (tbuf);
					utf = g_locale_to_utf8 (buf + i, 1, 0, &utf_len, 0);
					if (utf)
					{
						memcpy (buf + i, utf, utf_len);
						g_free (utf);
						i += (utf_len - 1);
					}
					j += 3;
				} else
				{
					switch (cmd[j + 1])
					{
					case 'R':
						buf[i] = '\026';
						break;
					case 'U':
						buf[i] = '\037';
						break;
					case 'B':
						buf[i] = '\002';
						break;
					case 'C':
						buf[i] = '\003';
						break;
					case 'O':
						buf[i] = '\017';
						break;
					case 'H':	/* CL: invisible text code */
						buf[i] = HIDDEN_CHAR;
						break;
					case '%':
						buf[i] = '%';
						break;
					default:
						buf[i] = '%';
						j--;
						break;
					}
					j++;
					break;
			default:
					buf[i] = cmd[j];
				}
			}
			j++;
			i++;
		}
		buf[i] = 0;
		if (occur)
			strcpy (cmd, buf);
		free (buf);
	}
}

typedef struct
{
	char *nick;
	int len;
	struct User *best;
	int bestlen;
	char *space;
	char *tbuf;
} nickdata;

static int
nick_comp_cb (struct User *user, nickdata *data)
{
	int lenu;

	if (!rfc_ncasecmp (user->nick, data->nick, data->len))
	{
		lenu = strlen (user->nick);
		if (lenu == data->len)
		{
			snprintf (data->tbuf, TBUFSIZE, "%s%s", user->nick, data->space);
			data->len = -1;
			return FALSE;
		} else if (lenu < data->bestlen)
		{
			data->bestlen = lenu;
			data->best = user;
		}
	}

	return TRUE;
}

static void
perform_nick_completion (struct session *sess, char *cmd, char *tbuf)
{
	int len;
	char *space = strchr (cmd, ' ');
	if (space && space != cmd)
	{
		if (space[-1] == prefs.nick_suffix[0] && space - 1 != cmd)
		{
			len = space - cmd - 1;
			if (len < NICKLEN)
			{
				char nick[NICKLEN];
				nickdata data;

				memcpy (nick, cmd, len);
				nick[len] = 0;

				data.nick = nick;
				data.len = len;
				data.bestlen = INT_MAX;
				data.best = NULL;
				data.tbuf = tbuf;
				data.space = space - 1;
				tree_foreach (sess->usertree, (tree_traverse_func *)nick_comp_cb, &data);

				if (data.len == -1)
					return;

				if (data.best)
				{
					snprintf (tbuf, TBUFSIZE, "%s%s", data.best->nick, space - 1);
					return;
				}
			}
		}
	}

	strcpy (tbuf, cmd);
}

static void
user_command (session * sess, char *tbuf, char *cmd, char *word[],
				  char *word_eol[])
{
	if (!auto_insert (tbuf, 2048, cmd, word, word_eol, "", sess->channel, "",
							server_get_network (sess->server, TRUE), "",
							sess->server->nick, ""))
	{
		PrintText (sess, _("Bad arguments for user command.\n"));
		return;
	}

	handle_command (sess, tbuf, TRUE);
}

/* handle text entered without a CMDchar prefix */

static void
handle_say (session *sess, char *text, int check_spch)
{
	struct DCC *dcc;
	char *word[PDIWORDS];
	char *word_eol[PDIWORDS];
	char pdibuf_static[1024];
	char newcmd_static[1024];
	char *pdibuf = pdibuf_static;
	char *newcmd = newcmd_static;
	int len;
	int newcmdlen = sizeof newcmd_static;

	if (strcmp (sess->channel, "(lastlog)") == 0)
	{
		lastlog (sess->lastlog_sess, text, sess->lastlog_regexp);
		return;
	}

	len = strlen (text);
	if (len >= sizeof pdibuf_static)
		pdibuf = malloc (len + 1);

	if (len + NICKLEN >= newcmdlen)
		newcmd = malloc (newcmdlen = len + NICKLEN + 1);

	if (check_spch && prefs.perc_color)
		check_special_chars (text, prefs.perc_ascii);

	/* split the text into words and word_eol */
	process_data_init (pdibuf, text, word, word_eol, TRUE, FALSE);

	/* a command of "" can be hooked for non-commands */
	if (plugin_emit_command (sess, "", word, word_eol))
		goto xit;

	/* incase a plugin did /close */
	if (!is_session (sess))
		goto xit;

	if (!sess->channel[0] || sess->type == SESS_SERVER || sess->type == SESS_NOTICES || sess->type == SESS_SNOTICES)
	{
		notj_msg (sess);
		goto xit;
	}

	if (prefs.nickcompletion)
		perform_nick_completion (sess, text, newcmd);
	else
		safe_strcpy (newcmd, text, newcmdlen);

	text = newcmd;

	if (sess->type == SESS_DIALOG)
	{
		/* try it via dcc, if possible */
		dcc = dcc_write_chat (sess->channel, text);
		if (dcc)
		{
			inbound_chanmsg (sess->server, NULL, sess->channel,
								  sess->server->nick, text, TRUE, FALSE);
			set_topic (sess, net_ip (dcc->addr));
			goto xit;
		}
	}

	if (sess->server->connected)
	{
		unsigned int max;
		unsigned char t = 0;

		/* maximum allowed message text */
		/* :nickname!username@host.com PRIVMSG #channel :text\r\n */
		max = 512;
		max -= 16;	/* :, !, @, " PRIVMSG ", " ", :, \r, \n */
		max -= strlen (sess->server->nick);
		max -= strlen (sess->channel);
		if (sess->me && sess->me->hostname)
			max -= strlen (sess->me->hostname);
		else
		{
			max -= 9;	/* username */
			max -= 65;	/* max possible hostname and '@' */
		}

		if (strlen (text) > max)
		{
			int i = 0, size;

			/* traverse the utf8 string and find the nearest cut point that
				doesn't split 1 char in half */
			while (1)
			{
				size = g_utf8_skip[((unsigned char *)text)[i]];
				if ((i + size) >= max)
					break;
				i += size;
			}
			max = i;
			t = text[max];
			text[max] = 0;			  /* insert a NULL terminator to shorten it */
		}

		inbound_chanmsg (sess->server, sess, sess->channel, sess->server->nick,
							  text, TRUE, FALSE);
		sess->server->p_message (sess->server, sess->channel, text);

		if (t)
		{
			text[max] = t;
			handle_say (sess, text + max, FALSE);
		}

	} else
	{
		notc_msg (sess);
	}

xit:
	if (pdibuf != pdibuf_static)
		free (pdibuf);

	if (newcmd != newcmd_static)
		free (newcmd);
}

/* handle a command, without the '/' prefix */

int
handle_command (session *sess, char *cmd, int check_spch)
{
	struct popup *pop;
	int user_cmd = FALSE;
	GSList *list;
	char *word[PDIWORDS];
	char *word_eol[PDIWORDS];
	static int command_level = 0;
	struct commands *int_cmd;
	char pdibuf_static[1024];
	char tbuf_static[TBUFSIZE];
	char *pdibuf;
	char *tbuf;
	int len;
	int ret = TRUE;

	if (command_level > 99)
	{
		fe_message (_("Too many recursive usercommands, aborting."), FE_MSG_ERROR);
		return TRUE;
	}
	command_level++;
	/* anything below MUST DEC command_level before returning */

	len = strlen (cmd);
	if (len >= sizeof (pdibuf_static))
		pdibuf = malloc (len + 1);
	else
		pdibuf = pdibuf_static;

	if ((len * 2) >= sizeof (tbuf_static))
		tbuf = malloc ((len * 2) + 1);
	else
		tbuf = tbuf_static;

	/* split the text into words and word_eol */
	process_data_init (pdibuf, cmd, word, word_eol, TRUE, TRUE);
	int_cmd = find_internal_command (word[1]);
	/* redo it without quotes processing, for some commands like /JOIN */
	if (int_cmd && !int_cmd->handle_quotes)
		process_data_init (pdibuf, cmd, word, word_eol, FALSE, FALSE);

	if (check_spch && prefs.perc_color)
		check_special_chars (cmd, prefs.perc_ascii);

	if (plugin_emit_command (sess, word[1], word, word_eol))
		goto xit;

	/* incase a plugin did /close */
	if (!is_session (sess))
		goto xit;

	/* first see if it's a userCommand */
	list = command_list;
	while (list)
	{
		pop = (struct popup *) list->data;
		if (!strcasecmp (pop->name, word[1]))
		{
			user_command (sess, tbuf, pop->cmd, word, word_eol);
			user_cmd = TRUE;
		}
		list = list->next;
	}

	if (user_cmd)
		goto xit;

	/* now check internal commands */
	int_cmd = find_internal_command (word[1]);

	if (int_cmd)
	{
		if (int_cmd->needserver && !sess->server->connected)
		{
			notc_msg (sess);
		} else if (int_cmd->needchannel && !sess->channel[0])
		{
			notj_msg (sess);
		} else
		{
			switch (int_cmd->callback (sess, tbuf, word, word_eol))
			{
			case FALSE:
				help (sess, tbuf, int_cmd->name, TRUE);
				break;
			case 2:
				ret = FALSE;
				goto xit;
			}
		}
	} else
	{
		/* unknown command, just send it to the server and hope */
		if (!sess->server->connected)
			PrintText (sess, _("Unknown Command. Try /help\n"));
		else
			sess->server->p_raw (sess->server, cmd);
	}

xit:
	command_level--;

	if (pdibuf != pdibuf_static)
		free (pdibuf);

	if (tbuf != tbuf_static)
		free (tbuf);

	return ret;
}

/* handle one line entered into the input box */

static int
handle_user_input (session *sess, char *text, int history, int nocommand)
{
	if (*text == '\0')
		return 1;

	if (history)
		history_add (&sess->history, text);

	/* is it NOT a command, just text? */
	if (nocommand || text[0] != prefs.cmdchar[0])
	{
		handle_say (sess, text, TRUE);
		return 1;
	}

	/* check for // */
	if (text[0] == prefs.cmdchar[0] && text[1] == prefs.cmdchar[0])
	{
		handle_say (sess, text + 1, TRUE);
		return 1;
	}

	if (prefs.cmdchar[0] == '/')
	{
		int i;
		const char *unix_dirs [] = {
			"/bin/", "/boot/", "/dev/",
			"/etc/", "/home/", "/lib/",
			"/lost+found/", "/mnt/", "/opt/",
			"/proc/", "/root/", "/sbin/",
			"/tmp/", "/usr/", "/var/",
			"/gnome/", NULL};
		for (i = 0; unix_dirs[i] != NULL; i++)
			if (strncmp (text, unix_dirs[i], strlen (unix_dirs[i]))==0)
			{
				handle_say (sess, text, TRUE);
				return 1;
			}
	}

	return handle_command (sess, text + 1, TRUE);
}

/* changed by Steve Green. Macs sometimes paste with imbedded \r */
void
handle_multiline (session *sess, char *cmd, int history, int nocommand)
{
	while (*cmd)
	{
		char *cr = cmd + strcspn (cmd, "\n\r");
		int end_of_string = *cr == 0;
		*cr = 0;
		if (!handle_user_input (sess, cmd, history, nocommand))
			return;
		if (end_of_string)
			break;
		cmd = cr + 1;
	}
}

/*void
handle_multiline (session *sess, char *cmd, int history, int nocommand)
{
	char *cr;

	cr = strchr (cmd, '\n');
	if (cr)
	{
		while (1)
		{
			if (cr)
				*cr = 0;
			if (!handle_user_input (sess, cmd, history, nocommand))
				return;
			if (!cr)
				break;
			cmd = cr + 1;
			if (*cmd == 0)
				break;
			cr = strchr (cmd, '\n');
		}
	} else
	{
		handle_user_input (sess, cmd, history, nocommand);
	}
}*/
