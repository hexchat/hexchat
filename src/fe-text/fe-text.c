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
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../common/outbound.h"
#include "../common/util.h"
#include "../common/fe.h"
#include "fe-text.h"


static GSList *tmr_list;		  /* timer list */
static int tmr_list_count;
static GSList *se_list;			  /* socket event list */
static int se_list_count;
static int done = FALSE;		  /* finished ? */


static void
send_command (char *cmd)
{
	handle_multiline (sess_list->data, cmd, TRUE, FALSE);
}

static void
read_stdin (void)
{
	int len, i = 0;
	static int pos = 0;
	static char inbuf[1024];
	char tmpbuf[512];

	len = read (STDIN_FILENO, tmpbuf, sizeof tmpbuf - 1);

	while (i < len)
	{
		switch (tmpbuf[i])
		{
		case '\r':
			break;

		case '\n':
			inbuf[pos] = 0;
			pos = 0;
			send_command (inbuf);
			break;

		default:
			inbuf[pos] = tmpbuf[i];
			if (pos < (sizeof inbuf - 2))
				pos++;
		}
		i++;
	}
}

static int done_intro = 0;

void
fe_new_window (struct session *sess)
{
	char buf[512];

	sess->gui = malloc (4);

	if (!sess->server->front_session)
		sess->server->front_session = sess;
	if (!sess->server->server_session)
		sess->server->server_session = sess;
	if (!current_tab)
		current_tab = sess;

	if (done_intro)
		return;
	done_intro = 1;

	snprintf (buf, sizeof (buf),	
				"\n"
				" \017xchat \00310"VERSION"\n"
				" \017Running on \00310%s \017glib \00310%d.%d.%d\n"
				" \017This binary compiled \00310"__DATE__"\017\n",
				get_cpu_str(), 
				glib_major_version, glib_minor_version, glib_micro_version);
	fe_print_text (sess, buf);

	fe_print_text (sess, "\n\nCompiled in Features\0032:\017 "
#ifdef USE_PERL
	"Perl "
#endif
#ifdef USE_PYTHON
	"Python "
#endif
#ifdef USE_PLUGIN
	"Plugin "
#endif
#ifdef ENABLE_NLS
	"NLS "
#endif
#ifdef USE_TRANS
	"Trans "
#endif
#ifdef USE_HEBREW
	"Hebrew "
#endif
#ifdef USE_OPENSSL
	"OpenSSL "
#endif
#ifdef SOCKS
	"Socks5 "
#endif
#ifdef USE_JCODE
	"JCode "
#endif
#ifdef USE_IPV6
	"IPv6"
#endif
	"\n\n");
	fflush (stdout);
	fflush (stdin);
}

static int
get_stamp_str (time_t tim, char *dest, int size)
{
	return strftime (dest, size, prefs.stamp_format, localtime (&tim));
}

static int
timecat (char *buf)
{
	char stampbuf[64];

	get_stamp_str (time (0), stampbuf, sizeof (stampbuf));
	strcat (buf, stampbuf);
	return strlen (stampbuf);
}

/*                       0  1  2  3  4  5  6  7   8   9   10 11  12  13  14 15 */
static const short colconv[] = { 0, 7, 4, 2, 1, 3, 5, 11, 13, 12, 6, 16, 14, 15, 10, 7 };

void
fe_print_text (struct session *sess, char *text)
{
	int dotime = FALSE;
	char num[8];
	int reverse = 0, under = 0, bold = 0,
		comma, k, i = 0, j = 0, len = strlen (text);
	unsigned char *newtext = malloc (len + 1024);

	if (prefs.timestamp)
	{
		newtext[0] = 0;
		j += timecat (newtext);
	}
	while (i < len)
	{
		if (dotime && text[i] != 0)
		{
			dotime = FALSE;
			newtext[j] = 0;
			j += timecat (newtext);
		}
		switch (text[i])
		{
		case 3:
			i++;
			if (!isdigit (text[i]))
			{
				newtext[j] = 27;
				j++;
				newtext[j] = '[';
				j++;
				newtext[j] = 'm';
				j++;
				i--;
				goto jump2;
			}
			k = 0;
			comma = FALSE;
			while (i < len)
			{
				if (text[i] >= '0' && text[i] <= '9' && k < 2)
				{
					num[k] = text[i];
					k++;
				} else
				{
					int col, mirc;
					num[k] = 0;
					newtext[j] = 27;
					j++;
					newtext[j] = '[';
					j++;
					if (k == 0)
					{
						newtext[j] = 'm';
						j++;
					} else
					{
						if (comma)
							col = 40;
						else
							col = 30;
						mirc = atoi (num);
						mirc = colconv[mirc];
						if (mirc > 9)
						{
							mirc += 50;
							sprintf ((char *) &newtext[j], "%dm", mirc + col);
						} else
						{
							sprintf ((char *) &newtext[j], "%dm", mirc + col);
						}
						j = strlen (newtext);
					}
					switch (text[i])
					{
					case ',':
						comma = TRUE;
						break;
					default:
						goto jump;
					}
					k = 0;
				}
				i++;
			}
			break;
		case '\026':				  /* REVERSE */
			if (reverse)
			{
				reverse = FALSE;
				strcpy (&newtext[j], "\033[27m");
			} else
			{
				reverse = TRUE;
				strcpy (&newtext[j], "\033[7m");
			}
			j = strlen (newtext);
			break;
		case '\037':				  /* underline */
			if (under)
			{
				under = FALSE;
				strcpy (&newtext[j], "\033[24m");
			} else
			{
				under = TRUE;
				strcpy (&newtext[j], "\033[4m");
			}
			j = strlen (newtext);
			break;
		case '\002':				  /* bold */
			if (bold)
			{
				bold = FALSE;
				strcpy (&newtext[j], "\033[22m");
			} else
			{
				bold = TRUE;
				strcpy (&newtext[j], "\033[1m");
			}
			j = strlen (newtext);
			break;
		case '\007':
			if (!prefs.filterbeep)
			{
				newtext[j] = text[i];
				j++;
			}
			break;
		case '\017':				  /* reset all */
			strcpy (&newtext[j], "\033[m");
			j += 3;
			reverse = FALSE;
			bold = FALSE;
			under = FALSE;
			break;
		case '\t':
			newtext[j] = ' ';
			j++;
			break;
		case '\n':
			newtext[j] = '\r';
			j++;
			if (prefs.timestamp)
				dotime = TRUE;
		default:
			newtext[j] = text[i];
			j++;
		}
	 jump2:
		i++;
	 jump:
		i += 0;
	}
	newtext[j] = 0;
	write (STDOUT_FILENO, newtext, j);
	free (newtext);
}

void
fe_timeout_remove (int tag)
{
	timerevent *te;
	GSList *list;

	list = tmr_list;
	while (list)
	{
		te = (timerevent *) list->data;
		if (te->tag == tag)
		{
			tmr_list = g_slist_remove (tmr_list, te);
			free (te);
			return;
		}
		list = list->next;
	}
}

int
fe_timeout_add (int interval, void *callback, void *userdata)
{
	struct timeval now;
	timerevent *te = malloc (sizeof (timerevent));

	tmr_list_count++;				  /* this overflows at 2.2Billion, who cares!! */

	te->tag = tmr_list_count;
	te->interval = interval;
	te->callback = callback;
	te->userdata = userdata;

	gettimeofday (&now, NULL);
	te->next_call = now.tv_sec * 1000 + (now.tv_usec / 1000) + te->interval;

	tmr_list = g_slist_prepend (tmr_list, te);

	return te->tag;
}

void
fe_input_remove (int tag)
{
	socketevent *se;
	GSList *list;

	list = se_list;
	while (list)
	{
		se = (socketevent *) list->data;
		if (se->tag == tag)
		{
			se_list = g_slist_remove (se_list, se);
			free (se);
			return;
		}
		list = list->next;
	}
}

int
fe_input_add (int sok, int rread, int wwrite, int eexcept, void *func,
				  void *data)
{
	socketevent *se = malloc (sizeof (socketevent));

	se_list_count++;				  /* this overflows at 2.2Billion, who cares!! */

	se->tag = se_list_count;
	se->sok = sok;
	se->rread = rread;
	se->wwrite = wwrite;
	se->eexcept = eexcept;
	se->callback = func;
	se->userdata = data;
	se_list = g_slist_prepend (se_list, se);

	return se->tag;
}

int
fe_args (int argc, char *argv[])
{
	if (argc > 1)
	{
		if (!strcasecmp (argv[1], "--version") || !strcasecmp (argv[1], "-v"))
		{
			puts (VERSION);
			return 0;
		}
	}
	return 1;
}

void
fe_init (void)
{
	se_list = 0;
	se_list_count = 0;
	tmr_list = 0;
	tmr_list_count = 0;
	prefs.autosave = 0;
	prefs.use_server_tab = 0;
	prefs.autodialog = 0;
	prefs.lagometer = 0;
	prefs.slist_skip = 1;
}

void
fe_main (void)
{
	struct timeval timeout, now;
	socketevent *se;
	timerevent *te;
	fd_set rd, wd, ex;
	GSList *list;
	guint64 shortest, delay;

	if (!sess_list)
		new_ircwindow (NULL, NULL, SESS_SERVER);

#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, PREFIX"/share/locale");
	textdomain (PACKAGE);
#endif

	while (!done)
	{
		FD_ZERO (&rd);
		FD_ZERO (&wd);
		FD_ZERO (&ex);

		list = se_list;
		while (list)
		{
			se = (socketevent *) list->data;
			if (se->rread)
				FD_SET (se->sok, &rd);
			if (se->wwrite)
				FD_SET (se->sok, &wd);
			if (se->eexcept)
				FD_SET (se->sok, &ex);
			list = list->next;
		}

		FD_SET (STDIN_FILENO, &rd);	/* for reading keyboard */

		/* find the shortest timeout event */
		shortest = 0;
		list = tmr_list;
		while (list)
		{
			te = (timerevent *) list->data;
			if (te->next_call < shortest || shortest == 0)
				shortest = te->next_call;
			list = list->next;
		}
		gettimeofday (&now, NULL);
		delay = shortest - ((now.tv_sec * 1000) + (now.tv_usec / 1000));
		timeout.tv_sec = delay / 1000;
		timeout.tv_usec = (delay % 1000) * 1000;

		select (FD_SETSIZE, &rd, &wd, &ex, &timeout);

		if (FD_ISSET (STDIN_FILENO, &rd))
			read_stdin ();

		/* set all checked flags to false */
		list = se_list;
		while (list)
		{
			se = (socketevent *) list->data;
			se->checked = 0;
			list = list->next;
		}

		/* check all the socket callbacks */
		list = se_list;
		while (list)
		{
			se = (socketevent *) list->data;
			se->checked = 1;
			if (se->rread && FD_ISSET (se->sok, &rd))
			{
				se->callback (NULL, 1, se->userdata);
			} else if (se->wwrite && FD_ISSET (se->sok, &wd))
			{
				se->callback (NULL, 2, se->userdata);
			} else if (se->eexcept && FD_ISSET (se->sok, &ex))
			{
				se->callback (NULL, 4, se->userdata);
			}
			list = se_list;
			if (list)
			{
				se = (socketevent *) list->data;
				while (se->checked)
				{
					list = list->next;
					if (!list)
						break;
					se = (socketevent *) list->data;
				}
			}
		}

		/* now check our list of timeout events, some might need to be called! */
		gettimeofday (&now, NULL);
		list = tmr_list;
		while (list)
		{
			te = (timerevent *) list->data;
			list = list->next;
			if (now.tv_sec * 1000 + (now.tv_usec / 1000) >= te->next_call)
			{
				/* if the callback returns 0, it must be removed */
				if (te->callback (te->userdata) == 0)
				{
					fe_timeout_remove (te->tag);
				} else
				{
					te->next_call = now.tv_sec * 1000 + (now.tv_usec / 1000) + te->interval;
				}
			}
		}

	}
}

void
fe_exit (void)
{
	done = TRUE;
}

void
fe_new_server (struct server *serv)
{
	serv->gui = malloc (4);
}

void
fe_message (char *msg, int wait)
{
	puts (msg);
}

void
fe_close_window (struct session *sess)
{
	kill_session_callback (sess);
	done = TRUE;
}

void
fe_beep (void)
{
	putchar (7);
}

void
fe_add_rawlog (struct server *serv, char *text, int len, int outbound)
{
}
void
fe_set_topic (struct session *sess, char *topic)
{
}
void
fe_cleanup (void)
{
}
void
fe_set_hilight (struct session *sess)
{
}
void
fe_update_mode_buttons (struct session *sess, char mode, char sign)
{
}
void
fe_update_channel_key (struct session *sess)
{
}
void
fe_update_channel_limit (struct session *sess)
{
}
int
fe_is_chanwindow (struct server *serv)
{
	return 0;
}

void
fe_add_chan_list (struct server *serv, char *chan, char *users, char *topic)
{
}
void
fe_chan_list_end (struct server *serv)
{
}
int
fe_is_banwindow (struct session *sess)
{
	return 0;
}
void
fe_add_ban_list (struct session *sess, char *chan, char *users, char *topic)
{
}               
void
fe_ban_list_end (struct session *sess)
{
}
void
fe_notify_update (char *name)
{
}
void
fe_text_clear (struct session *sess)
{
}
void
fe_progressbar_start (struct session *sess)
{
}
void
fe_progressbar_end (struct server *serv)
{
}
void
fe_userlist_insert (struct session *sess, struct User *newuser, int row, int sel)
{
}
int
fe_userlist_remove (struct session *sess, struct User *user)
{
	return 0;
}
void
fe_userlist_move (struct session *sess, struct User *user, int new_row)
{
}
void
fe_userlist_numbers (struct session *sess)
{
}
void
fe_userlist_clear (struct session *sess)
{
}
void
fe_dcc_update_recv_win (void)
{
}
void
fe_dcc_update_send_win (void)
{
}
void
fe_dcc_update_chat_win (void)
{
}
void
fe_dcc_update_send (struct DCC *dcc)
{
}
void
fe_dcc_update_recv (struct DCC *dcc)
{
}
void
fe_clear_channel (struct session *sess)
{
}
void
fe_session_callback (struct session *sess)
{
}
void
fe_server_callback (struct server *serv)
{
}
void
fe_url_add (const char *text)
{
}
void
fe_pluginlist_update (void)
{
}
void
fe_buttons_update (struct session *sess)
{
}
void
fe_dlgbuttons_update (struct session *sess)
{
}
void
fe_dcc_send_filereq (struct session *sess, char *nick, int maxcps)
{
}
void
fe_set_channel (struct session *sess)
{
}
void
fe_set_title (struct session *sess)
{
}
void
fe_set_nonchannel (struct session *sess, int state)
{
}
void
fe_set_nick (struct server *serv, char *newnick)
{
}
void
fe_change_nick (struct server *serv, char *nick, char *newnick)
{
}
void
fe_ignore_update (int level)
{
}
void
fe_dcc_open_recv_win (int passive)
{
}
void
fe_dcc_open_send_win (int passive)
{
}
void
fe_dcc_open_chat_win (int passive)
{
}
void
fe_userlist_hide (session * sess)
{
}
void
fe_lastlog (session * sess, session * lastlog_sess, char *sstr)
{
}
void
fe_set_lag (server * serv, int lag)
{
}
void
fe_set_throttle (server * serv)
{
}
void
fe_set_away (server *serv)
{
}
void
fe_serverlist_open (session *sess)
{
}
void
fe_get_str (char *prompt, char *def, void *callback, void *ud)
{
}
void
fe_get_int (char *prompt, int def, void *callback, void *ud)
{
}
void
fe_play_wave (const char *file)
{
    play_wave (file);
}
void
fe_idle_add (void *func, void *data)
{
}
