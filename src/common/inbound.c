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
#include "servlist.h"
#include "text.h"
#include "ctcp.h"
#include "plugin.h"
#include "xchatc.h"


void
clear_channel (session *sess)
{
	if (sess->channel[0])
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
	userlist_clear (sess);
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
		if (userlist_find (serv->front_session, nick))
			return serv->front_session;
	}

	if (current_sess && current_sess->server == serv)
	{
		if (userlist_find (current_sess, nick))
			return current_sess;
	}

	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
		{
			if (userlist_find (sess, nick))
				return sess;
		}
		list = list->next;
	}
	return 0;
}

static session *
inbound_open_dialog (server *serv, char *from)
{
	session *sess;

	sess = new_ircwindow (serv, from, SESS_DIALOG, 0);
	/* for playing sounds */
	EMIT_SIGNAL (XP_TE_OPENDIALOG, sess, NULL, NULL, NULL, NULL, 0);

	return sess;
}

static void
inbound_make_idtext (server *serv, char *idtext, int max, int id)
{
	idtext[0] = 0;
	if (serv->have_idmsg)
	{
		if (id)
		{
			safe_strcpy (idtext, prefs.irc_id_ytext, max);
		} else
		{
			safe_strcpy (idtext, prefs.irc_id_ntext, max);
		}
		/* convert codes like %C,%U to the proper ones */
		check_special_chars (idtext, TRUE);
	}
}

void
inbound_privmsg (server *serv, char *from, char *ip, char *text, int id)
{
	session *sess;
	char idtext[64];

	sess = find_dialog (serv, from);

	if (sess || prefs.autodialog)
	{
		/*0=ctcp  1=priv will set autodialog=0 here is flud detected */
		if (!sess)
		{
			if (flood_check (from, ip, serv, current_sess, 1))
				/* Create a dialog session */
				sess = inbound_open_dialog (serv, from);
			else
				sess = serv->server_session;
		}

		if (prefs.beepmsg || (sess && sess->beep))
			sound_beep (sess);

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
		inbound_chanmsg (serv, NULL, NULL, from, text, FALSE, id);
		return;
	}

	inbound_make_idtext (serv, idtext, sizeof (idtext), id);

	sess = find_session_from_nick (from, serv);
	if (!sess)
	{
		sess = serv->front_session;

		if (prefs.beepmsg || (sess && sess->beep))
			sound_beep (sess);

		EMIT_SIGNAL (XP_TE_PRIVMSG, sess, from, text, idtext, NULL, 0);
		return;
	}

	if (prefs.beepmsg || sess->beep)
		sound_beep (sess);

	if (sess->type == SESS_DIALOG)
		EMIT_SIGNAL (XP_TE_DPRIVMSG, sess, from, text, idtext, NULL, 0);
	else
		EMIT_SIGNAL (XP_TE_PRIVMSG, sess, from, text, idtext, NULL, 0);
}

static int
SearchNick (char *text, char *nicks)
{
	char S[300];	/* size of irc_extra_hilight in xchatprefs */
	char *n;
	char *p;
	char *t;
	int ns;

	if (nicks == NULL)
		return 0;

	text = strip_color (text, -1, STRIP_ALL);

	safe_strcpy (S, nicks, sizeof (S));
	n = strtok (S, ",");
	while (n != NULL)
	{
		t = text;
		ns = strlen (n);
		while ((p = nocasestrstr (t, n)))
		{
			char *prev_char = (p == text) ? NULL : g_utf8_prev_char (p);
			char *next_char = p + ns;
			if ((!prev_char ||
			     !g_unichar_isalnum (g_utf8_get_char(prev_char))) &&
			    !g_unichar_isalnum (g_utf8_get_char(next_char)))
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
FromNick (char *nick, char *nicks)
{
	char S[300];	/* size of irc_no_hilight in xchatprefs */
	char *n;
	char *t;

	if (nicks == NULL || nicks[0] == 0)
		return 0;

	safe_strcpy (S, nicks, sizeof (S));
	n = strtok (S, ",");
	while (n != NULL)
	{
		t = nick;
		if (nocasestrstr(t, n))
			return 1;
		n = strtok (NULL, ",");
	}
	return 0;
}

static int
is_hilight (char *from, char *text, session *sess, server *serv)
{
	if (FromNick(from, prefs.irc_no_hilight))
		return 0;

	if (SearchNick (text, serv->nick) ||
		 SearchNick (text, prefs.irc_extra_hilight) ||
		 FromNick (from, prefs.irc_nick_hilight))
	{
		if (sess != current_tab)
			sess->nick_said = TRUE;
		fe_set_hilight (sess);
		return 1;
	}
	return 0;
}

void
inbound_action (session *sess, char *chan, char *from, char *text, int fromme, int id)
{
	session *def = sess;
	server *serv = sess->server;
	int beep = FALSE;
	struct User *user;
	int hilight = FALSE;
	char nickchar[2] = "\000";

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
				sess = inbound_open_dialog (serv, from);
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

	user = userlist_find (sess, from);
	if (user)
	{
		nickchar[0] = user->prefix[0];
		user->lasttalk = time (0);
	}

	if (!fromme)
	{
		hilight = is_hilight (from, text, sess, serv);
		if (hilight && prefs.beephilight)
			beep = TRUE;

		if (beep || sess->beep)
			sound_beep (sess);

		if (hilight)
		{
			EMIT_SIGNAL (XP_TE_HCHANACTION, sess, from, text, nickchar, NULL, 0);
			return;
		}
	}

	EMIT_SIGNAL (XP_TE_CHANACTION, sess, from, text, nickchar, NULL, 0);
}

void
inbound_chanmsg (server *serv, session *sess, char *chan, char *from, char *text, char fromme, int id)
{
	struct User *user;
	int hilight = FALSE;
	char nickchar[2] = "\000";
	char idtext[64];

	if (!sess)
	{
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
	}

	if (sess != current_tab)
	{
		sess->msg_said = TRUE;
		sess->new_data = FALSE;
	}

	user = userlist_find (sess, from);
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

	inbound_make_idtext (serv, idtext, sizeof (idtext), id);

	if (sess->type != SESS_DIALOG)
		if (prefs.beepchans || sess->beep)
			sound_beep (sess);

	if (is_hilight (from, text, sess, serv))
	{
		hilight = TRUE;
		if (prefs.beephilight)
			sound_beep (sess);
	}
	if (sess->type == SESS_DIALOG)
		EMIT_SIGNAL (XP_TE_DPRIVMSG, sess, from, text, idtext, NULL, 0);
	else if (hilight)
		EMIT_SIGNAL (XP_TE_HCHANMSG, sess, from, text, nickchar, idtext, 0);
	else
		EMIT_SIGNAL (XP_TE_CHANMSG, sess, from, text, nickchar, idtext, 0);
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
			if (userlist_change (sess, nick, newnick) || (me && sess->type == SESS_SERVER))
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
				sess = new_ircwindow (serv, chan, SESS_CHANNEL, 1);
		}
	}

	safe_strcpy (sess->channel, chan, CHANLEN);

	fe_set_channel (sess);
	fe_set_title (sess);
	fe_set_nonchannel (sess, TRUE);
	userlist_clear (sess);

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
		userlist_clear (sess);
	}

	while (1)
	{
		switch (*names)
		{
		case 0:
			name[pos] = 0;
			if (pos != 0)
				userlist_add (sess, name, 0);
			return;
		case ' ':
			name[pos] = 0;
			pos = 0;
			userlist_add (sess, name, 0);
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
		new_topic = strip_color (topic_text, -1, STRIP_ALL);
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
		new_topic = strip_color (topic, -1, STRIP_ALL);
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
		userlist_add (sess, user, ip);
	}
}

void
inbound_kick (server *serv, char *chan, char *user, char *kicker, char *reason)
{
	session *sess = find_channel (serv, chan);
	if (sess)
	{
		EMIT_SIGNAL (XP_TE_KICK, sess, kicker, user, chan, reason, 0);
		userlist_remove (sess, user);
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
		userlist_remove (sess, user);
	}
}

void
inbound_topictime (server *serv, char *chan, char *nick, time_t stamp)
{
	char *tim = ctime (&stamp);
	session *sess = find_channel (serv, chan);

	if (!sess)
		sess = serv->server_session;

	tim[24] = 0;	/* get rid of the \n */
	EMIT_SIGNAL (XP_TE_TOPICDATE, sess, chan, nick, tim, NULL, 0);
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
			if (userlist_remove (sess, nick))
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

	tim = strtoul (timestring, NULL, 10);
	nowtim = make_ping_time ();
	dif = nowtim - tim;

	sess->server->ping_recv = time (0);

	if (lag)
	{
		sess->server->lag_sent = 0;
		sess->server->lag = dif / 1000;
		fe_set_lag (sess->server, dif / 100000);
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
inbound_notice (server *serv, char *to, char *nick, char *msg, char *ip, int id)
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

	if (!sess && ptr[0] == '%')
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
					sess = new_ircwindow (serv, "(notices)", SESS_NOTICES, 0);
				else
					sess = new_ircwindow (serv, "(snotices)", SESS_SNOTICES, 0);
				prefs.hideuserlist = oldh;
				fe_set_channel (sess);
				fe_set_title (sess);
				fe_set_nonchannel (sess, FALSE);
				userlist_clear (sess);
				if (prefs.logging)
					log_open (sess);
			}
			/* Avoid redundancy with some Undernet notices */
			if (!strncmp (msg, "*** Notice -- ", 14))
				msg += 14;
		} else
		{
											/* paranoia check */
			if (msg[0] == '[' && (!serv->have_idmsg || id))
			{
				/* guess where chanserv meant to post this -sigh- */
				if (!strcasecmp (nick, "ChanServ") && !find_dialog (serv, nick))
				{
					char *dest = strdup (msg + 1);
					char *end = strchr (dest, ']');
					if (end)
					{
						*end = 0;
						sess = find_channel (serv, dest);
					}
					free (dest);
				}
			}
			if (!sess)
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
	struct away_msg *away = server_away_find_message (serv, nick);
	session *sess = NULL;
	GSList *list;

	if (away && !strcmp (msg, away->message))	/* Seen the msg before? */
	{
		if (prefs.show_away_once && !serv->inside_whois)
			return;
	} else
	{
		server_away_save_message (serv, nick, msg);
	}

	if (prefs.irc_whois_front)
		sess = serv->front_session;
	else
	{
		if (!serv->inside_whois)
			sess = find_session_from_nick (nick, serv);
		if (!sess)
			sess = serv->server_session;
	}

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

static gboolean
check_willjoin_channels (server *serv)
{
	char *po;
	session *sess;
	GSList *list = sess_list;
	int i = 0;

	/* shouldnt really happen, the io tag is destroyed in server.c */
	if (!is_server (serv))
		return FALSE;

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
				i++;
			}
		}
		list = list->next;
	}
	serv->joindelay_tag = 0;
	fe_server_event (serv, FE_SE_LOGGEDIN, i);
	return FALSE;
}

void
inbound_next_nick (session *sess, char *nick)
{
	char *newnick;
	server *serv = sess->server;
	ircnet *net;

	serv->nickcount++;

	switch (serv->nickcount)
	{
	case 2:
		newnick = prefs.nick2;
		net = serv->network;
		/* use network specific "Second choice"? */
		if (net && !(net->flags & FLAG_USE_GLOBAL) && net->nick2)
			newnick = net->nick2;
		serv->p_change_nick (serv, newnick);
		EMIT_SIGNAL (XP_TE_NICKCLASH, sess, nick, newnick, NULL, NULL, 0);
		break;

	case 3:
		serv->p_change_nick (serv, prefs.nick3);
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
		serv->p_mode (serv, serv->nick, modes);
	}
}

void
inbound_login_start (session *sess, char *nick, char *servname)
{
	inbound_newnick (sess->server, sess->server->nick, nick, TRUE);
	server_set_name (sess->server, servname);
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

int
inbound_banlist (session *sess, time_t stamp, char *chan, char *mask, char *banner, int is_exemption)
{
	char *time_str = ctime (&stamp);
	server *serv = sess->server;

	time_str[19] = 0;	/* get rid of the \n */
	if (stamp == 0)
		time_str = "";

	sess = find_channel (serv, chan);
	if (!sess)
	{
		sess = serv->front_session;
		goto nowindow;
	}

   if (!fe_is_banwindow (sess))
	{
nowindow:
		/* let proto-irc.c do the 'goto def' for exemptions */
		if (is_exemption)
			return FALSE;

		EMIT_SIGNAL (XP_TE_BANLIST, sess, chan, mask, banner, time_str, 0);
		return TRUE;
	}

	fe_add_ban_list (sess, mask, banner, time_str, is_exemption);
	return TRUE;
}

/* execute 1 end-of-motd command */

static int
inbound_exec_eom_cmd (char *str, void *sess)
{
	handle_command (sess, (str[0] == '/') ? str + 1 : str, TRUE);
	return 1;
}

void
inbound_login_end (session *sess, char *text)
{
	server *serv = sess->server;

	if (!serv->end_of_motd)
	{
		if (prefs.ip_from_server && serv->use_who)
		{
			serv->skip_next_userhost = TRUE;
			serv->p_get_ip_uh (serv, serv->nick);	/* sends USERHOST mynick */
		}
		set_default_modes (serv);

		if (serv->network)
		{
			/* there may be more than 1, separated by \n */
			if (((ircnet *)serv->network)->command)
				token_foreach (((ircnet *)serv->network)->command, '\n',
									inbound_exec_eom_cmd, sess);

			/* send nickserv password */
			if (((ircnet *)serv->network)->nickserv)
				serv->p_ns_identify (serv, ((ircnet *)serv->network)->nickserv);
		}

		/* send JOIN now or wait? */
		if (serv->network && ((ircnet *)serv->network)->nickserv &&
			 prefs.irc_join_delay)
			serv->joindelay_tag = fe_timeout_add (prefs.irc_join_delay * 1000,
															  check_willjoin_channels, serv);
		else
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

void
inbound_identified (server *serv)	/* 'MODE +e MYSELF' on freenode */
{
	if (serv->joindelay_tag)
	{
		/* stop waiting, just auto JOIN now */
		fe_timeout_remove (serv->joindelay_tag);
		serv->joindelay_tag = 0;
		check_willjoin_channels (serv);
	}
}
