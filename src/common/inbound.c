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

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define WANTARPA
#define WANTDNS
#include "inet.h"

#include "xchat.h"
#include "util.h"
#include "ignore.h"
#include "fe.h"
#include "modes.h"
#include "notify.h"
#include "outbound.h"
#include "inbound.h"
#include "server.h"
#include "text.h"
#include "ctcp.h"
#include "plugin.h"
#include "xchatc.h"


/* black n white(0/1) are bad colors for nicks, and we'll use color 2 for us */
/* also light/dark gray (14/15) */
/* 5,7,8 are all shades of yellow which happen to look dman near the same */

static char rcolors[] = { 3, 4, 6, 8, 9, 10, 11, 12, 13 };

static int
color_of (char *name)
{
	int i = 0, sum = 0;

	while (name[i])
		sum += name[i++];
	sum %= sizeof (rcolors) / sizeof (char);
	return rcolors[sum];
}

void
clear_channel (session *sess)
{
	strcpy (sess->waitchannel, sess->channel);
	sess->channel[0] = 0;
	sess->doing_who = FALSE;
	sess->done_away_check = FALSE;

	log_close (sess);

	if (sess->current_modes)
	{
		free (sess->current_modes);
		sess->current_modes = NULL;
	}

	if (sess->mode_timeout_tag)
	{
		fe_timeout_remove (sess->mode_timeout_tag);
		sess->mode_timeout_tag = 0;
	}

	fe_clear_channel (sess);
	clear_user_list (sess);
	fe_set_nonchannel (sess, FALSE);
	fe_set_title (sess);
}

void
set_topic (session *sess, char *topic)
{
	if (sess->topic)
		free (sess->topic);
	sess->topic = strdup (topic);
	fe_set_topic (sess, topic);
}

static session *
find_session_from_nick (char *nick, server *serv)
{
	session *sess;
	GSList *list = sess_list;

	sess = find_dialog (serv, nick);
	if (sess)
		return sess;

	if (serv->front_session)
	{
		if (find_name (serv->front_session, nick))
			return serv->front_session;
	}

	if (current_sess && current_sess->server == serv)
	{
		if (find_name (current_sess, nick))
			return current_sess;
	}

	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
		{
			if (find_name (sess, nick))
				return sess;
		}
		list = list->next;
	}
	return 0;
}

void
inbound_privmsg (server *serv, char *from, char *ip, char *text)
{
	session *sess;

	sess = find_dialog (serv, from);

	if (prefs.beepmsg || (sess && sess->beep))
		fe_beep ();

	if (sess || prefs.autodialog)
	{
		/*0=ctcp  1=priv will set autodialog=0 here is flud detected */
		if (!sess)
		{
			if (flood_check (from, ip, serv, current_sess, 1))
				sess = new_ircwindow (serv, from, SESS_DIALOG);	/* Create a dialog session */
			else
				sess = serv->server_session;
		}
		if (ip && ip[0])
		{
			if (prefs.logging && sess->logfd != -1 &&
				(!sess->topic || strcmp(sess->topic, ip)))
			{
				char tbuf[1024];
				snprintf (tbuf, sizeof (tbuf), "[%s has address %s]\n", from, ip);
				write (sess->logfd, tbuf, strlen (tbuf));
			}
			set_topic (sess, ip);
		}
		inbound_chanmsg (serv, NULL, from, text, FALSE);
		return;
	}

	sess = find_session_from_nick (from, serv);
	if (!sess)
		sess = serv->front_session;

	if (sess->type == SESS_DIALOG)
		EMIT_SIGNAL (XP_TE_DPRIVMSG, sess, from, text, NULL, NULL, 0);
	else
		EMIT_SIGNAL (XP_TE_PRIVMSG, sess, from, text, NULL, NULL, 0);
}

static int
SearchNick (char *text, char *nicks)
{
	char S[300];	/* size of bluestring in xchatprefs */
	char *n;
	char *p;
	char *t;
	int ns;

	if (nicks == NULL)
		return 0;

	text = strip_color (text);

	safe_strcpy (S, nicks, sizeof (S));
	n = strtok (S, ",");
	while (n != NULL)
	{
		t = text;
		ns = strlen (n);
		while ((p = nocasestrstr (t, n)))
		{
			if ((p == text || !isalnum (*(p - 1))) && !isalnum (*(p + ns)))
			{
				free (text);
				return 1;
			}

			t = p + 1;
		}

		n = strtok (NULL, ",");
	}
	free (text);
	return 0;
}

static int
is_hilight (char *text, session *sess, server *serv)
{
	if ((SearchNick (text, serv->nick)) || SearchNick (text, prefs.bluestring))
	{
		if (sess != current_tab)
			fe_set_hilight (sess);
		return 1;
	}
	return 0;
}

void
inbound_action (session *sess, char *chan, char *from, char *text, int fromme)
{
	session *def = sess;
	server *serv = sess->server;
	int beep = 0;

	if (!fromme)
	{
		if (is_channel (serv, chan))
		{
			sess = find_channel (serv, chan);
			beep = prefs.beepchans;
		} else
		{
			/* it's a private action! */
			beep = prefs.beepmsg;
			/* find a dialog tab for it */
			sess = find_dialog (serv, from);
			/* if non found, open a new one */
			if (!sess && prefs.autodialog)
				sess = new_ircwindow (serv, from, SESS_DIALOG);
		}
	}

	if (!sess)
		sess = def;

	if (sess != current_tab)
	{
		if (fromme)
		{
			sess->msg_said = FALSE;
			sess->new_data = TRUE;
		} else
		{
			sess->msg_said = TRUE;
			sess->new_data = FALSE;
		}
	}

	if (!fromme)
	{
		if (beep || sess->beep)
			fe_beep ();

		if (is_hilight (text, sess, serv))
		{
			EMIT_SIGNAL (XP_TE_HCHANACTION, sess, from, text, NULL, NULL, 0);
			return;
		}
	}

	if (prefs.colorednicks)
	{
		char tbuf[NICKLEN + 4];
		snprintf (tbuf, sizeof (tbuf), "\003%d%s", color_of (from), from);
		EMIT_SIGNAL (XP_TE_CHANACTION, sess, tbuf, text, NULL, NULL, 0);
	} else
	{
		EMIT_SIGNAL (XP_TE_CHANACTION, sess, from, text, NULL, NULL, 0);
	}
}

void
inbound_chanmsg (server *serv, char *chan, char *from, char *text, char fromme)
{
	struct User *user;
	session *sess;
	int hilight = FALSE;
	char nickchar[2] = "\000";

	if (chan)
	{
		sess = find_channel (serv, chan);
		if (!sess && !is_channel (serv, chan))
			sess = find_dialog (serv, chan);
	} else
	{
		sess = find_dialog (serv, from);
	}
	if (!sess)
		return;

	if (sess != current_tab)
	{
		sess->msg_said = TRUE;
		sess->new_data = FALSE;
	}

	user = find_name (sess, from);
	if (user)
	{
		nickchar[0] = user->prefix[0];
		user->lasttalk = time (0);
	}

	if (fromme)
	{
  		if (prefs.auto_unmark_away && serv->is_away)
			sess->server->p_set_back (sess->server);
		EMIT_SIGNAL (XP_TE_UCHANMSG, sess, from, text, nickchar, NULL, 0);
		return;
	}

	if (sess->type != SESS_DIALOG)
		if (prefs.beepchans || sess->beep)
			fe_beep ();

	if (is_hilight (text, sess, serv))
	{
		hilight = TRUE;
		if (prefs.beephilight)
			fe_beep ();
	}
	if (sess->type == SESS_DIALOG)
		EMIT_SIGNAL (XP_TE_DPRIVMSG, sess, from, text, NULL, NULL, 0);
	else if (hilight)
		EMIT_SIGNAL (XP_TE_HCHANMSG, sess, from, text, nickchar, NULL, 0);
	else if (prefs.colorednicks)
	{
		char tbuf[NICKLEN + 4];
		snprintf (tbuf, sizeof (tbuf), "\003%d%s", color_of (from), from);
		EMIT_SIGNAL (XP_TE_CHANMSG, sess, tbuf, text, nickchar, NULL, 0);
	}
	else
		EMIT_SIGNAL (XP_TE_CHANMSG, sess, from, text, nickchar, NULL, 0);
}

void
inbound_newnick (server *serv, char *nick, char *newnick, int quiet)
{
	int me = FALSE;
	session *sess;
	GSList *list = sess_list;

	if (!serv->p_cmp (nick, serv->nick))
	{
		me = TRUE;
		safe_strcpy (serv->nick, newnick, NICKLEN);
	}

	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
		{
			if (change_nick (sess, nick, newnick) || (me && sess->type == SESS_SERVER))
			{
				if (!quiet)
				{
					if (me)
						EMIT_SIGNAL (XP_TE_UCHANGENICK, sess, nick, newnick, NULL,
										 NULL, 0);
					else
						EMIT_SIGNAL (XP_TE_CHANGENICK, sess, nick, newnick, NULL,
										 NULL, 0);
				}
			}
			if (sess->type == SESS_DIALOG && !serv->p_cmp (sess->channel, nick))
			{
				safe_strcpy (sess->channel, newnick, CHANLEN);
				fe_set_channel (sess);
			}
			fe_set_title (sess);
		}
		list = list->next;
	}

	dcc_change_nick (serv, nick, newnick);

	if (me)
		fe_set_nick (serv, newnick);
}

/* find a "<none>" tab */
static session *
find_unused_session (server *serv)
{
	session *sess;
	GSList *list = sess_list;
	while (list)
	{
		sess = (session *) list->data;
		if (sess->type == SESS_CHANNEL && sess->channel[0] == 0 &&
			 sess->server == serv)
		{
			if (sess->waitchannel[0] == 0)
				return sess;
		}
		list = list->next;
	}
	return 0;
}

static session *
find_session_from_waitchannel (char *chan, struct server *serv)
{
	session *sess;
	GSList *list = sess_list;
	while (list)
	{
		sess = (session *) list->data;
		if (sess->server == serv && sess->channel[0] == 0 && sess->type == SESS_CHANNEL)
		{
			if (!serv->p_cmp (chan, sess->waitchannel))
				return sess;
		}
		list = list->next;
	}
	return 0;
}

void
inbound_ujoin (server *serv, char *chan, char *nick, char *ip)
{
	session *sess;

	/* already joined? probably a bnc */
	sess = find_channel (serv, chan);
	if (!sess)
	{
		/* see if a window is waiting to join this channel */
		sess = find_session_from_waitchannel (chan, serv);
		if (!sess)
		{
			/* find a "<none>" tab and use that */
			sess = find_unused_session (serv);
			if (!sess)
				/* last resort, open a new tab/window */
				sess = new_ircwindow (serv, chan, SESS_CHANNEL);
		}
	}

	safe_strcpy (sess->channel, chan, CHANLEN);

	fe_set_channel (sess);
	fe_set_title (sess);
	fe_set_nonchannel (sess, TRUE);
	clear_user_list (sess);

	if (prefs.logging)
		log_open (sess);

	sess->waitchannel[0] = 0;
	sess->ignore_date = TRUE;
	sess->ignore_mode = TRUE;
	sess->ignore_names = TRUE;
	sess->end_of_names = FALSE;

	/* sends a MODE */
	serv->p_join_info (sess->server, chan);

	EMIT_SIGNAL (XP_TE_UJOIN, sess, nick, chan, ip, NULL, 0);

	if (prefs.userhost)
	{
		/* sends WHO #channel */
		serv->p_user_list (sess->server, chan);
		sess->doing_who = TRUE;
	}
}

void
inbound_ukick (server *serv, char *chan, char *kicker, char *reason)
{
	session *sess = find_channel (serv, chan);
	if (sess)
	{
		EMIT_SIGNAL (XP_TE_UKICK, sess, serv->nick, chan, kicker, reason, 0);
		clear_channel (sess);
		if (prefs.autorejoin)
		{
			serv->p_join (serv, chan, sess->channelkey);
			safe_strcpy (sess->waitchannel, chan, CHANLEN);
		}
	}
}

void
inbound_upart (server *serv, char *chan, char *ip, char *reason)
{
	session *sess = find_channel (serv, chan);
	if (sess)
	{
		if (*reason)
			EMIT_SIGNAL (XP_TE_UPARTREASON, sess, serv->nick, ip, chan, reason,
							 0);
		else
			EMIT_SIGNAL (XP_TE_UPART, sess, serv->nick, ip, chan, NULL, 0);
		clear_channel (sess);
	}
}

void
inbound_nameslist (server *serv, char *chan, char *names)
{
	session *sess;
	char name[NICKLEN];
	int pos = 0;

	sess = find_channel (serv, chan);
	if (!sess)
	{
		EMIT_SIGNAL (XP_TE_USERSONCHAN, serv->server_session, chan, names, NULL,
						 NULL, 0);
		return;
	}
	if (!sess->ignore_names)
		EMIT_SIGNAL (XP_TE_USERSONCHAN, sess, chan, names, NULL, NULL, 0);

	if (sess->end_of_names)
	{
		sess->end_of_names = FALSE;
		clear_user_list (sess);
	}

	while (1)
	{
		switch (*names)
		{
		case 0:
			name[pos] = 0;
			if (pos != 0)
				add_name (sess, name, 0);
			return;
		case ' ':
			name[pos] = 0;
			pos = 0;
			add_name (sess, name, 0);
			break;
		default:
			name[pos] = *names;
			if (pos < (NICKLEN-1))
				pos++;
		}
		names++;
	}
}

void
inbound_topic (server *serv, char *chan, char *topic_text)
{
	session *sess = find_channel (serv, chan);
	char *new_topic;

	if (sess)
	{
		new_topic = strip_color (topic_text);
		set_topic (sess, new_topic);
		free (new_topic);
	} else
		sess = serv->server_session;

	EMIT_SIGNAL (XP_TE_TOPIC, sess, chan, topic_text, NULL, NULL, 0);
}

void
inbound_topicnew (server *serv, char *nick, char *chan, char *topic)
{
	session *sess;
	char *new_topic;

	sess = find_channel (serv, chan);
	if (sess)
	{
		new_topic = strip_color (topic);
		set_topic (sess, new_topic);
		free (new_topic);
		EMIT_SIGNAL (XP_TE_NEWTOPIC, sess, nick, topic, chan, NULL, 0);
	}
}

void
inbound_join (server *serv, char *chan, char *user, char *ip)
{
	session *sess = find_channel (serv, chan);
	if (sess)
	{
		if (!sess->hide_join_part)
			EMIT_SIGNAL (XP_TE_JOIN, sess, user, chan, ip, NULL, 0);
		add_name (sess, user, ip);
	}
}

void
inbound_kick (server *serv, char *chan, char *user, char *kicker, char *reason)
{
	session *sess = find_channel (serv, chan);
	if (sess)
	{
		EMIT_SIGNAL (XP_TE_KICK, sess, kicker, user, chan, reason, 0);
		sub_name (sess, user);
	}
}

void
inbound_part (server *serv, char *chan, char *user, char *ip, char *reason)
{
	session *sess = find_channel (serv, chan);
	if (sess)
	{
		if (!sess->hide_join_part)
		{
			if (*reason)
				EMIT_SIGNAL (XP_TE_PARTREASON, sess, user, ip, chan, reason, 0);
			else
				EMIT_SIGNAL (XP_TE_PART, sess, user, ip, chan, NULL, 0);
		}
		sub_name (sess, user);
	}
}

void
inbound_topictime (server *serv, char *chan, char *nick, time_t stamp)
{
	char *tim = ctime (&stamp);
	session *sess = find_channel (serv, chan);

	if (!sess)
		sess = serv->server_session;

	tim[19] = 0;	/* get rid of the \n */
	EMIT_SIGNAL (XP_TE_TOPICDATE, sess, chan, nick, tim, NULL, 0);
}

void
set_server_name (struct server *serv, char *name)
{
	GSList *list = sess_list;
	session *sess;

	if (name[0] == 0)
		name = serv->hostname;

	safe_strcpy (serv->servername, name, sizeof (serv->servername));
	while (list)
	{
		sess = (session *) list->data;
		if (sess->server == serv)
			fe_set_title (sess);
		list = list->next;
	}
	if (serv->server_session->type == SESS_SERVER)
	{
		if (serv->networkname)
		{
			safe_strcpy (serv->server_session->channel, serv->networkname, CHANLEN);
		} else
		{
			safe_strcpy (serv->server_session->channel, name, CHANLEN);
		}
		fe_set_channel (serv->server_session);
	}
}

void
inbound_quit (server *serv, char *nick, char *ip, char *reason)
{
	GSList *list = sess_list;
	session *sess;
	int was_on_front_session = FALSE;

	while (list)
	{
		sess = (session *) list->data;
		if (sess->server == serv)
		{
 			if (sess == current_sess)
 				was_on_front_session = TRUE;
			if (sub_name (sess, nick))
			{
				if (!sess->hide_join_part)
					EMIT_SIGNAL (XP_TE_QUIT, sess, nick, reason, ip, NULL, 0);
			} else if (sess->type == SESS_DIALOG && !serv->p_cmp (sess->channel, nick))
			{
				EMIT_SIGNAL (XP_TE_QUIT, sess, nick, reason, ip, NULL, 0);
			}
		}
		list = list->next;
	}

	notify_set_offline (serv, nick, was_on_front_session);
}

void
inbound_ping_reply (session *sess, char *timestring, char *from)
{
	unsigned long tim, nowtim, dif;
	int lag = 0;
	char outbuf[64];

	if (strncmp (timestring, "LAG", 3) == 0)
	{
		timestring += 3;
		lag = 1;
	}

	sscanf (timestring, "%lu", &tim);
	nowtim = make_ping_time ();
	dif = nowtim - tim;

	sess->server->ping_recv = time (0);

	if (lag)
	{
		sess->server->lag_sent = 0;
		snprintf (outbuf, sizeof (outbuf), "%ld.%ld", dif / 100000, (dif / 10000) % 100);
		fe_set_lag (sess->server, (int)((float)atof (outbuf)));
		return;
	}

	if (atol (timestring) == 0)
	{
		if (sess->server->lag_sent)
			sess->server->lag_sent = 0;
		else
			EMIT_SIGNAL (XP_TE_PINGREP, sess, from, "?", NULL, NULL, 0);
	} else
	{
		snprintf (outbuf, sizeof (outbuf), "%ld.%ld%ld", dif / 1000000, (dif / 100000) % 10, dif % 10);
		EMIT_SIGNAL (XP_TE_PINGREP, sess, from, outbuf, NULL, NULL, 0);
	}
}

static session *
find_session_from_type (int type, server *serv)
{
	session *sess;
	GSList *list = sess_list;
	while (list)
	{
		sess = list->data;
		if (sess->type == type && serv == sess->server)
			return sess;
		list = list->next;
	}
	return 0;
}

void
inbound_notice (server *serv, char *to, char *nick, char *msg, char *ip)
{
	char *po,*ptr=to;
	session *sess = 0;
	int server_notice = FALSE;

	if (is_channel (serv, ptr))
		sess = find_channel (serv, ptr);

	if (!sess && ptr[0] == '@')
	{
		ptr++;
		sess = find_channel (serv, ptr);
	}

	if (!sess && ptr[0] == '+')
	{
		ptr++;
		sess = find_channel (serv, ptr);
	}

	if (strcmp (nick, ip) == 0)
		server_notice = TRUE;

	if (!sess)
	{
		ptr = 0;
		if (prefs.notices_tabs)
		{
			int stype = server_notice ? SESS_SNOTICES : SESS_NOTICES;
			sess = find_session_from_type (stype, serv);
			if (!sess)
			{
				register unsigned int oldh = prefs.hideuserlist;
				prefs.hideuserlist = 1;
				if (stype == SESS_NOTICES)
					sess = new_ircwindow (serv, "(notices)", SESS_NOTICES);
				else
					sess = new_ircwindow (serv, "(snotices)", SESS_SNOTICES);
				prefs.hideuserlist = oldh;
				fe_set_channel (sess);
				fe_set_title (sess);
				fe_set_nonchannel (sess, FALSE);
				clear_user_list (sess);
				if (prefs.logging)
					log_open (sess);
			}
			/* Avoid redundancy with some Undernet notices */
			if (!strncmp (msg, "*** Notice -- ", 14))
				msg += 14;
		} else
		{
			sess = find_session_from_nick (nick, serv);
		}
		if (!sess)
		{
			if (server_notice)	
				sess = serv->server_session;
			else
				sess = serv->front_session;
		}
	}

	if (msg[0] == 1)
	{
		msg++;
		if (!strncmp (msg, "PING", 4))
		{
			inbound_ping_reply (sess, msg + 5, nick);
			return;
		}
	}
	po = strchr (msg, '\001');
	if (po)
		po[0] = 0;

	if (server_notice)
		EMIT_SIGNAL (XP_TE_SERVNOTICE, sess, msg, nick, NULL, NULL, 0);
	else if (ptr)
		EMIT_SIGNAL (XP_TE_CHANNOTICE, sess, nick, to, msg, NULL, 0);
	else
		EMIT_SIGNAL (XP_TE_NOTICE, sess, nick, msg, NULL, NULL, 0);
}

void
inbound_away (server *serv, char *nick, char *msg)
{
	struct away_msg *away = find_away_message (serv, nick);
	session *sess = NULL;
	GSList *list;

	if (away && !strcmp (msg, away->message))	/* Seen the msg before? */
	{
		if (prefs.show_away_once && !serv->inside_whois)
			return;
	} else
	{
		save_away_message (serv, nick, msg);
	}

	if (!serv->inside_whois)
		sess = find_session_from_nick (nick, serv);
	if (!sess)
		sess = serv->server_session;

	EMIT_SIGNAL (XP_TE_WHOIS5, sess, nick, msg, NULL, NULL, 0);

	list = sess_list;
	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
			userlist_set_away (sess, nick, TRUE);
		list = list->next;
	}
}

int
inbound_nameslist_end (server *serv, char *chan)
{
	session *sess;
	GSList *list;

	if (!strcmp (chan, "*"))
	{
		list = sess_list;
		while (list)
		{
			sess = list->data;
			if (sess->server == serv)
			{
				sess->end_of_names = TRUE;
				sess->ignore_names = FALSE;
			}
			list = list->next;
		}
		return TRUE;
	}
	sess = find_channel (serv, chan);
	if (sess)
	{
		sess->end_of_names = TRUE;
		sess->ignore_names = FALSE;
		return TRUE;
	}
	return FALSE;
}

static void
check_willjoin_channels (server *serv)
{
	char *po;
	session *sess;
	GSList *list = sess_list;

	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
		{
			if (sess->willjoinchannel[0] != 0)
			{
				strcpy (sess->waitchannel, sess->willjoinchannel);
				sess->willjoinchannel[0] = 0;
				serv->p_join (serv, sess->waitchannel, sess->channelkey);
				po = strchr (sess->waitchannel, ',');
				if (po)
					*po = 0;
				po = strchr (sess->waitchannel, ' ');
				if (po)
					*po = 0;
			}
		}
		list = list->next;
	}
}

void
inbound_next_nick (session *sess, char *nick)
{
	sess->server->nickcount++;

	switch (sess->server->nickcount)
	{
	case 2:
		sess->server->p_change_nick (sess->server, prefs.nick2);
		EMIT_SIGNAL (XP_TE_NICKCLASH, sess, nick, prefs.nick2, NULL, NULL, 0);
		break;

	case 3:
		sess->server->p_change_nick (sess->server, prefs.nick3);
		EMIT_SIGNAL (XP_TE_NICKCLASH, sess, nick, prefs.nick3, NULL, NULL, 0);
		break;

	default:
		EMIT_SIGNAL (XP_TE_NICKFAIL, sess, NULL, NULL, NULL, NULL, 0);
	}
}

void
do_dns (session *sess, char *nick, char *host)
{
	char *po;
	char tbuf[1024];

	po = strrchr (host, '@');
	if (po)
		host = po + 1;
	EMIT_SIGNAL (XP_TE_RESOLVINGUSER, sess, nick, host, NULL, NULL, 0);
	snprintf (tbuf, sizeof (tbuf), "exec -d %s %s", prefs.dnsprogram, host);
	handle_command (sess, tbuf, FALSE);
}

static void
set_default_modes (server *serv)
{
	char modes[8];

	modes[0] = '+';
	modes[1] = '\0';

	if (prefs.wallops)
		strcat (modes, "w");
	if (prefs.servernotice)
		strcat (modes, "s");
	if (prefs.invisible)
		strcat (modes, "i");

	if (modes[1] != '\0')
	{
		serv->p_nick_mode (serv, serv->nick, modes);
	}
}

void
inbound_login_start (session *sess, char *nick, char *servname)
{
	inbound_newnick (sess->server, sess->server->nick, nick, TRUE);
	set_server_name (sess->server, servname);
	if (sess->type == SESS_SERVER && prefs.logging)
		log_open (sess);
	/* reset our away status */
	if (sess->server->reconnect_away)
	{
		handle_command (sess->server->server_session, "away", FALSE);
		sess->server->reconnect_away = FALSE;
	}
}

static void
inbound_set_all_away_status (server *serv, char *nick, unsigned int status)
{
	GSList *list;
	session *sess;

	list = sess_list;
	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
			userlist_set_away (sess, nick, status);
		list = list->next;
	}
}

void
inbound_uaway (server *serv)
{
	serv->is_away = TRUE;
	serv->away_time = time (NULL);
	fe_set_away (serv);

	inbound_set_all_away_status (serv, serv->nick, 1);
}

void
inbound_uback (server *serv)
{
	serv->is_away = FALSE;
	serv->reconnect_away = FALSE;
	fe_set_away (serv);

	inbound_set_all_away_status (serv, serv->nick, 0);
}

void
inbound_foundip (session *sess, char *ip)
{
	struct hostent *HostAddr;

	HostAddr = gethostbyname (ip);
	if (HostAddr)
	{
		prefs.dcc_ip = ((struct in_addr *) HostAddr->h_addr)->s_addr;
		EMIT_SIGNAL (XP_TE_FOUNDIP, sess,
						 inet_ntoa (*((struct in_addr *) HostAddr->h_addr)),
						 NULL, NULL, NULL, 0);
	}
}

void
inbound_user_info_start (session *sess, char *nick)
{
	/* set away to FALSE now, 301 may turn it back on */
	inbound_set_all_away_status (sess->server, nick, 0);
}

int
inbound_user_info (session *sess, char *chan, char *user, char *host,
						 char *servname, char *nick, char *realname,
						 unsigned int away)
{
	server *serv = sess->server;
	session *who_sess;
	char *uhost;

	who_sess = find_channel (serv, chan);
	if (who_sess)
	{
		if (user && host)
		{
			uhost = malloc (strlen (user) + strlen (host) + 2);
			sprintf (uhost, "%s@%s", user, host);
			if (!userlist_add_hostname (who_sess, nick, uhost, realname, servname, away))
			{
				if (!who_sess->doing_who)
				{
					free (uhost);
					return 0;
				}
			}
			free (uhost);
		} else
		{
			if (!userlist_add_hostname (who_sess, nick, NULL, realname, servname, away))
			{
				if (!who_sess->doing_who)
					return 0;
			}
		}
	} else
	{
		if (!serv->doing_dns)
			return 0;
		if (nick && host)
			do_dns (sess, nick, host);
	}
	return 1;
}

void
inbound_banlist (session *sess, time_t stamp, char *chan, char *mask, char *banner)
{
	char *time_str = ctime (&stamp);
	server *serv = sess->server;

	time_str[19] = 0;	/* get rid of the \n */
	if (stamp == 0)
		time_str = "";

	sess = find_channel (serv, chan);
	if (!sess)
		sess = serv->front_session;
   if (!fe_is_banwindow (sess))
		EMIT_SIGNAL (XP_TE_BANLIST, sess, chan, mask, banner, time_str, 0);
	else
		fe_add_ban_list (sess, mask, banner, time_str);
}

void
inbound_login_end (session *sess, char *text)
{
	server *serv = sess->server;

	if (!serv->end_of_motd)
	{
		if (prefs.ip_from_server)
		{
			serv->skip_next_who = TRUE;
			serv->p_get_ip (serv, serv->nick);	/* sends WHO mynick */
		}
		set_default_modes (serv);
		if (serv->eom_cmd)
			handle_command (sess, serv->eom_cmd, TRUE);
		check_willjoin_channels (serv);
		if (serv->supports_watch)
			notify_send_watches (serv);
		serv->end_of_motd = TRUE;
	}
	if (prefs.skipmotd && !serv->motd_skipped)
	{
		serv->motd_skipped = TRUE;
		EMIT_SIGNAL (XP_TE_MOTDSKIP, serv->server_session, NULL, NULL,
						 NULL, NULL, 0);
		return;
	}
	EMIT_SIGNAL (XP_TE_MOTD, serv->server_session, text, NULL,
					 NULL, NULL, 0);
}
