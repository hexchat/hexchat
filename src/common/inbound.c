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
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#define WANTARPA
#define WANTDNS
#include "inet.h"

#include "hexchat.h"
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
#include "hexchatc.h"
#include "chanopt.h"


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
		g_free (sess->current_modes);
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
set_topic (session *sess, char *topic, char *stripped_topic)
{
	/* The topic of dialogs are the users hostname which is logged is new */
	if (sess->type == SESS_DIALOG && (!sess->topic || strcmp(sess->topic, stripped_topic))
		&& sess->logfd != -1)
	{
		char tbuf[1024];
		g_snprintf (tbuf, sizeof (tbuf), "[%s has address %s]\n", sess->channel, stripped_topic);
		write (sess->logfd, tbuf, strlen (tbuf));
	}

	g_free (sess->topic);
	sess->topic = g_strdup (stripped_topic);
	fe_set_topic (sess, topic, stripped_topic);
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
		// If we are here for ChanServ, then it is usually a reply for the user
		if (!g_ascii_strcasecmp(nick, "ChanServ") || userlist_find (serv->front_session, nick))
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
	return NULL;
}

static session *
inbound_open_dialog (server *serv, char *from,
							const message_tags_data *tags_data)
{
	session *sess;

	sess = new_ircwindow (serv, from, SESS_DIALOG, 0);
	/* for playing sounds */
	EMIT_SIGNAL_TIMESTAMP (XP_TE_OPENDIALOG, sess, NULL, NULL, NULL, NULL, 0,
								  tags_data->timestamp);

	return sess;
}

static void
inbound_make_idtext (server *serv, char *idtext, int max, int id)
{
	idtext[0] = 0;
	if (serv->have_idmsg || serv->have_accnotify)
	{
		if (id)
		{
			safe_strcpy (idtext, prefs.hex_irc_id_ytext, max);
		} else
		{
			safe_strcpy (idtext, prefs.hex_irc_id_ntext, max);
		}
		/* convert codes like %C,%U to the proper ones */
		check_special_chars (idtext, TRUE);
	}
}

void
inbound_privmsg (server *serv, char *from, char *ip, char *text, int id,
					  const message_tags_data *tags_data)
{
	session *sess;
	struct User *user;
	char idtext[64];
	gboolean nodiag = FALSE;

	sess = find_dialog (serv, from);

	if (sess || prefs.hex_gui_autoopen_dialog)
	{
		/*0=ctcp  1=priv will set hex_gui_autoopen_dialog=0 here is flud detected */
		if (!sess)
		{
			if (flood_check (from, ip, serv, current_sess, 1))
				/* Create a dialog session */
				sess = inbound_open_dialog (serv, from, tags_data);
			else
				sess = serv->server_session;
			if (!sess)
				return; /* ?? */
		}

		if (ip && ip[0])
			set_topic (sess, ip, ip);
		inbound_chanmsg (serv, NULL, NULL, from, text, FALSE, tags_data->identified, tags_data);
		return;
	}

	sess = find_session_from_nick (from, serv);
	if (!sess)
	{
		sess = serv->front_session;
		nodiag = TRUE; /* We don't want it to look like a normal message in front sess */
	}

	user = userlist_find (sess, from);
	if (user)
	{
		user->lasttalk = time (0);
		if (user->account)
			id = TRUE;
	}
	
	inbound_make_idtext (serv, idtext, sizeof (idtext), id);

	if (sess->type == SESS_DIALOG && !nodiag)
		EMIT_SIGNAL_TIMESTAMP (XP_TE_DPRIVMSG, sess, from, text, idtext, NULL, 0,
									  tags_data->timestamp);
	else
		EMIT_SIGNAL_TIMESTAMP (XP_TE_PRIVMSG, sess, from, text, idtext, NULL, 0, 
									  tags_data->timestamp);
}

/* used for Alerts section. Masks can be separated by commas and spaces. */

gboolean
alert_match_word (char *word, char *masks)
{
	char *p = masks;
	char endchar;
	int res;

	if (masks[0] == 0)
		return FALSE;

	while (1)
	{
		/* if it's a 0, space or comma, the word has ended. */
		if (*p == 0 || *p == ' ' || *p == ',')
		{
			endchar = *p;
			*p = 0;
			res = match (g_strchug (masks), word);
			*p = endchar;

			if (res)
				return TRUE;	/* yes, matched! */

			masks = p + 1;
			if (*p == 0)
				return FALSE;
		}
		p++;
	}
}

gboolean
alert_match_text (char *text, char *masks)
{
	unsigned char *p = text;
	unsigned char endchar;
	int res;

	if (masks[0] == 0)
		return FALSE;

	while (1)
	{
		if (*p >= '0' && *p <= '9')
		{
			p++;
			continue;
		}

		/* if it's RFC1459 <special>, it can be inside a word */
		switch (*p)
		{
		case '-': case '[': case ']': case '\\':
		case '`': case '^': case '{': case '}':
		case '_': case '|':
			p++;
			continue;
		}

		/* if it's a 0, space or comma, the word has ended. */
		if (*p == 0 || *p == ' ' || *p == ',' ||
			/* if it's anything BUT a letter, the word has ended. */
			 (!g_unichar_isalpha (g_utf8_get_char (p))))
		{
			endchar = *p;
			*p = 0;
			res = alert_match_word (text, masks);
			*p = endchar;

			if (res)
				return TRUE;	/* yes, matched! */

			text = p + g_utf8_skip [p[0]];
			if (*p == 0)
				return FALSE;
		}

		p += g_utf8_skip [p[0]];
	}
}

static int
is_hilight (char *from, char *text, session *sess, server *serv)
{
	if (alert_match_word (from, prefs.hex_irc_no_hilight))
		return 0;

	text = strip_color (text, -1, STRIP_ALL);

	if (alert_match_text (text, serv->nick) ||
		 alert_match_text (text, prefs.hex_irc_extra_hilight) ||
		 alert_match_word (from, prefs.hex_irc_nick_hilight))
	{
		g_free (text);
		if (sess != current_tab)
		{
			sess->tab_state |= TAB_STATE_NEW_HILIGHT;
			lastact_update (sess);
		}
		return 1;
	}

	g_free (text);
	return 0;
}

void
inbound_action (session *sess, char *chan, char *from, char *ip, char *text,
					 int fromme, int id, const message_tags_data *tags_data)
{
	session *def = sess;
	server *serv = sess->server;
	struct User *user;
	char nickchar[2] = "\000";
	char idtext[64];
	int privaction = FALSE;

	if (!fromme)
	{
		if (is_channel (serv, chan))
		{
			sess = find_channel (serv, chan);
		} else
		{
			/* it's a private action! */
			privaction = TRUE;
			/* find a dialog tab for it */
			sess = find_dialog (serv, from);
			/* if non found, open a new one */
			if (!sess && prefs.hex_gui_autoopen_dialog)
			{
				/* but only if it wouldn't flood */
				if (flood_check (from, ip, serv, current_sess, 1))
					sess = inbound_open_dialog (serv, from, tags_data);
				else
					sess = serv->server_session;
			}
			if (!sess)
			{
				sess = find_session_from_nick (from, serv);
				/* still not good? */
				if (!sess)
					sess = serv->front_session;
			}
		}
	}

	if (!sess)
		sess = def;

	if (sess != current_tab)
	{
		if (fromme)
			sess->tab_state |= TAB_STATE_NEW_DATA;
		else
			sess->tab_state |= TAB_STATE_NEW_MSG;
		lastact_update (sess);
	}

	user = userlist_find (sess, from);
	if (user)
	{
		nickchar[0] = user->prefix[0];
		user->lasttalk = time (0);
		if (user->account)
			id = TRUE;
		if (user->me)
			fromme = TRUE;
	}

	inbound_make_idtext (serv, idtext, sizeof (idtext), id);

	if (!fromme && !privaction)
	{
		if (is_hilight (from, text, sess, serv))
		{
			EMIT_SIGNAL_TIMESTAMP (XP_TE_HCHANACTION, sess, from, text, nickchar,
										  idtext, 0, tags_data->timestamp);
			return;
		}
	}

	if (fromme)
		EMIT_SIGNAL_TIMESTAMP (XP_TE_UACTION, sess, from, text, nickchar, idtext,
									  0, tags_data->timestamp);
	else if (!privaction)
		EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANACTION, sess, from, text, nickchar,
									  idtext, 0, tags_data->timestamp);
	else if (sess->type == SESS_DIALOG)
		EMIT_SIGNAL_TIMESTAMP (XP_TE_DPRIVACTION, sess, from, text, idtext, NULL,
									  0, tags_data->timestamp);
	else
		EMIT_SIGNAL_TIMESTAMP (XP_TE_PRIVACTION, sess, from, text, idtext, NULL, 0,
									  tags_data->timestamp);
}

void
inbound_chanmsg (server *serv, session *sess, char *chan, char *from, 
					  char *text, char fromme, int id, 
					  const message_tags_data *tags_data)
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
		sess->tab_state |= TAB_STATE_NEW_MSG;
		lastact_update (sess);
	}

	user = userlist_find (sess, from);
	if (user)
	{
		if (user->account)
			id = TRUE;
		nickchar[0] = user->prefix[0];
		user->lasttalk = time (0);
		if (user->me)
			fromme = TRUE;
	}

	if (fromme)
	{
		if (prefs.hex_away_auto_unmark && serv->is_away && !tags_data->timestamp)
			sess->server->p_set_back (sess->server);
		EMIT_SIGNAL_TIMESTAMP (XP_TE_UCHANMSG, sess, from, text, nickchar, NULL,
									  0, tags_data->timestamp);
		return;
	}

	inbound_make_idtext (serv, idtext, sizeof (idtext), id);

	if (is_hilight (from, text, sess, serv))
		hilight = TRUE;

	if (sess->type == SESS_DIALOG)
		EMIT_SIGNAL_TIMESTAMP (XP_TE_DPRIVMSG, sess, from, text, idtext, NULL, 0,
									  tags_data->timestamp);
	else if (hilight)
		EMIT_SIGNAL_TIMESTAMP (XP_TE_HCHANMSG, sess, from, text, nickchar, idtext,
									  0, tags_data->timestamp);
	else
		EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANMSG, sess, from, text, nickchar, idtext,
									  0, tags_data->timestamp);
}

void
inbound_newnick (server *serv, char *nick, char *newnick, int quiet,
					  const message_tags_data *tags_data)
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
						EMIT_SIGNAL_TIMESTAMP (XP_TE_UCHANGENICK, sess, nick, 
													  newnick, NULL, NULL, 0,
													  tags_data->timestamp);
					else
						EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANGENICK, sess, nick,
													  newnick, NULL, NULL, 0, tags_data->timestamp);
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
	return NULL;
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
	return NULL;
}

void
inbound_ujoin (server *serv, char *chan, char *nick, char *ip,
					const message_tags_data *tags_data)
{
	session *sess;
	int found_unused = FALSE;

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
			found_unused = sess != NULL;
			if (!sess)
				/* last resort, open a new tab/window */
				sess = new_ircwindow (serv, chan, SESS_CHANNEL, 1);
		}
	}

	safe_strcpy (sess->channel, chan, CHANLEN);
	if (found_unused)
	{
		chanopt_load (sess);
		scrollback_load (sess);
		if (sess->scrollwritten && sess->scrollback_replay_marklast)
			sess->scrollback_replay_marklast (sess);
	}

	fe_set_channel (sess);
	fe_set_title (sess);
	fe_set_nonchannel (sess, TRUE);
	userlist_clear (sess);

	log_open_or_close (sess);

	sess->waitchannel[0] = 0;
	sess->ignore_date = TRUE;
	sess->ignore_mode = TRUE;
	sess->ignore_names = TRUE;
	sess->end_of_names = FALSE;

	/* sends a MODE */
	serv->p_join_info (sess->server, chan);

	EMIT_SIGNAL_TIMESTAMP (XP_TE_UJOIN, sess, nick, chan, ip, NULL, 0,
								  tags_data->timestamp);

	if (prefs.hex_irc_who_join)
	{
		/* sends WHO #channel */
		serv->p_user_list (sess->server, chan);
		sess->doing_who = TRUE;
	}
}

void
inbound_ukick (server *serv, char *chan, char *kicker, char *reason,
					const message_tags_data *tags_data)
{
	session *sess = find_channel (serv, chan);
	if (sess)
	{
		EMIT_SIGNAL_TIMESTAMP (XP_TE_UKICK, sess, serv->nick, chan, kicker, 
									  reason, 0, tags_data->timestamp);
		clear_channel (sess);
		if (prefs.hex_irc_auto_rejoin)
		{
			serv->p_join (serv, chan, sess->channelkey);
			safe_strcpy (sess->waitchannel, chan, CHANLEN);
		}
	}
}

void
inbound_upart (server *serv, char *chan, char *ip, char *reason,
					const message_tags_data *tags_data)
{
	session *sess = find_channel (serv, chan);
	if (sess)
	{
		if (*reason)
			EMIT_SIGNAL_TIMESTAMP (XP_TE_UPARTREASON, sess, serv->nick, ip, chan,
										  reason, 0, tags_data->timestamp);
		else
			EMIT_SIGNAL_TIMESTAMP (XP_TE_UPART, sess, serv->nick, ip, chan, NULL,
										  0, tags_data->timestamp);
		clear_channel (sess);
	}
}

void
inbound_nameslist (server *serv, char *chan, char *names,
						 const message_tags_data *tags_data)
{
	session *sess;
	char **name_list;
	char *host, *nopre_name;
	char name[NICKLEN];
	int i;
	size_t offset;

	sess = find_channel (serv, chan);
	if (!sess)
	{
		EMIT_SIGNAL_TIMESTAMP (XP_TE_USERSONCHAN, serv->server_session, chan,
									  names, NULL, NULL, 0, tags_data->timestamp);
		return;
	}
	if (!sess->ignore_names)
		EMIT_SIGNAL_TIMESTAMP (XP_TE_USERSONCHAN, sess, chan, names, NULL, NULL,
									  0, tags_data->timestamp);

	if (sess->end_of_names)
	{
		sess->end_of_names = FALSE;
		userlist_clear (sess);
	}

	name_list = g_strsplit (names, " ", -1);
	for (i = 0; name_list[i]; i++)
	{
		host = NULL;
		offset = sizeof(name);

		if (name_list[i][0] == 0)
			continue;

		if (serv->have_uhnames)
		{
			offset = 0;
			nopre_name = name_list[i];

			/* Ignore prefixes so '!' won't cause issues */
			while (strchr (serv->nick_prefixes, *nopre_name) != NULL)
			{
				nopre_name++;
				offset++;
			}

			offset += strcspn (nopre_name, "!");
			if (offset++ < strlen (name_list[i]))
				host = name_list[i] + offset;
		}

		g_strlcpy (name, name_list[i], MIN(offset, sizeof(name)));

		userlist_add (sess, name, host, NULL, NULL, tags_data);
	}
	g_strfreev (name_list);
}

void
inbound_topic (server *serv, char *chan, char *topic_text,
					const message_tags_data *tags_data)
{
	session *sess = find_channel (serv, chan);
	char *stripped_topic;

	if (sess)
	{
		stripped_topic = strip_color (topic_text, -1, STRIP_ALL);
		set_topic (sess, topic_text, stripped_topic);
		g_free (stripped_topic);
	} else
		sess = serv->server_session;

	EMIT_SIGNAL_TIMESTAMP (XP_TE_TOPIC, sess, chan, topic_text, NULL, NULL, 0,
								  tags_data->timestamp);
}

void
inbound_topicnew (server *serv, char *nick, char *chan, char *topic,
						const message_tags_data *tags_data)
{
	session *sess;
	char *stripped_topic;

	sess = find_channel (serv, chan);
	if (sess)
	{
		EMIT_SIGNAL_TIMESTAMP (XP_TE_NEWTOPIC, sess, nick, topic, chan, NULL, 0,
									  tags_data->timestamp);
		stripped_topic = strip_color (topic, -1, STRIP_ALL);
		set_topic (sess, topic, stripped_topic);
		g_free (stripped_topic);
	}
}

void
inbound_join (server *serv, char *chan, char *user, char *ip, char *account,
				  char *realname, const message_tags_data *tags_data)
{
	session *sess = find_channel (serv, chan);
	if (sess)
	{
		EMIT_SIGNAL_TIMESTAMP (XP_TE_JOIN, sess, user, chan, ip, account, 0,
									  tags_data->timestamp);
		userlist_add (sess, user, ip, account, realname, tags_data);
	}
}

void
inbound_kick (server *serv, char *chan, char *user, char *kicker, char *reason,
				  const message_tags_data *tags_data)
{
	session *sess = find_channel (serv, chan);
	if (sess)
	{
		EMIT_SIGNAL_TIMESTAMP (XP_TE_KICK, sess, kicker, user, chan, reason, 0,
									  tags_data->timestamp);
		userlist_remove (sess, user);
	}
}

void
inbound_part (server *serv, char *chan, char *user, char *ip, char *reason,
				  const message_tags_data *tags_data)
{
	session *sess = find_channel (serv, chan);
	if (sess)
	{
		if (*reason)
			EMIT_SIGNAL_TIMESTAMP (XP_TE_PARTREASON, sess, user, ip, chan, reason,
										  0, tags_data->timestamp);
		else
			EMIT_SIGNAL_TIMESTAMP (XP_TE_PART, sess, user, ip, chan, NULL, 0,
										  tags_data->timestamp);
		userlist_remove (sess, user);
	}
}

void
inbound_topictime (server *serv, char *chan, char *nick, time_t stamp,
						 const message_tags_data *tags_data)
{
	char *tim = ctime (&stamp);
	session *sess = find_channel (serv, chan);

	if (!sess)
		sess = serv->server_session;

	if (tim != NULL)
		tim[24] = 0;	/* get rid of the \n */

	EMIT_SIGNAL_TIMESTAMP (XP_TE_TOPICDATE, sess, chan, nick, tim, NULL, 0,
								  tags_data->timestamp);
}

void
inbound_quit (server *serv, char *nick, char *ip, char *reason,
				  const message_tags_data *tags_data)
{
	GSList *list = sess_list;
	session *sess;
	struct User *user;
	int was_on_front_session = FALSE;

	while (list)
	{
		sess = (session *) list->data;
		if (sess->server == serv)
		{
 			if (sess == current_sess)
 				was_on_front_session = TRUE;
			if ((user = userlist_find (sess, nick)))
			{
				EMIT_SIGNAL_TIMESTAMP (XP_TE_QUIT, sess, nick, reason, ip, NULL, 0,
											  tags_data->timestamp);
				userlist_remove_user (sess, user);
			} else if (sess->type == SESS_DIALOG && !serv->p_cmp (sess->channel, nick))
			{
				EMIT_SIGNAL_TIMESTAMP (XP_TE_QUIT, sess, nick, reason, ip, NULL, 0,
											  tags_data->timestamp);
			}
		}
		list = list->next;
	}

	notify_set_offline (serv, nick, was_on_front_session, tags_data);
}

void
inbound_account (server *serv, char *nick, char *account,
					  const message_tags_data *tags_data)
{
	session *sess = NULL;
	GSList *list;

	list = sess_list;
	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
			userlist_set_account (sess, nick, account);
		list = list->next;
	}
}

void
inbound_ping_reply (session *sess, char *timestring, char *from,
						  const message_tags_data *tags_data)
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
		sess->server->lag = dif;
		fe_set_lag (sess->server, dif);
		return;
	}

	if (atol (timestring) == 0)
	{
		if (sess->server->lag_sent)
			sess->server->lag_sent = 0;
		else
			EMIT_SIGNAL_TIMESTAMP (XP_TE_PINGREP, sess, from, "?", NULL, NULL, 0,
										  tags_data->timestamp);
	} else
	{
		g_snprintf (outbuf, sizeof (outbuf), "%ld.%03ld", dif / 1000, dif % 1000);
		EMIT_SIGNAL_TIMESTAMP (XP_TE_PINGREP, sess, from, outbuf, NULL, NULL, 0,
									  tags_data->timestamp);
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
	return NULL;
}

void
inbound_notice (server *serv, char *to, char *nick, char *msg, char *ip, int id,
					 const message_tags_data *tags_data)
{
	char *ptr = to;
	session *sess = 0;
	int server_notice = FALSE;

	if (is_channel (serv, ptr))
		sess = find_channel (serv, ptr);

	/* /notice [mode-prefix]#channel should end up in that channel */
	if (!sess && ptr[0] && strchr(serv->nick_prefixes, ptr[0]) != NULL)
	{
		ptr++;
		sess = find_channel (serv, ptr);
	}

	if (strcmp (nick, ip) == 0)
		server_notice = TRUE;

	if (!sess)
	{
		ptr = 0;
		if (prefs.hex_irc_notice_pos == 0)
		{
											/* paranoia check */
			if (msg[0] == '[' && (!serv->have_idmsg || id))
			{
				/* guess where chanserv meant to post this -sigh- */
				if (!g_ascii_strcasecmp (nick, "ChanServ") && !find_dialog (serv, nick))
				{
					char *dest = g_strdup (msg + 1);
					char *end = strchr (dest, ']');
					if (end)
					{
						*end = 0;
						sess = find_channel (serv, dest);
					}
					g_free (dest);
				}
			}
			if (!sess)
				sess = find_session_from_nick (nick, serv);
		} else if (prefs.hex_irc_notice_pos == 1)
		{
			int stype = server_notice ? SESS_SNOTICES : SESS_NOTICES;
			sess = find_session_from_type (stype, serv);
			if (!sess)
			{
				if (stype == SESS_NOTICES)
					sess = new_ircwindow (serv, "(notices)", SESS_NOTICES, 0);
				else
					sess = new_ircwindow (serv, "(snotices)", SESS_SNOTICES, 0);
				fe_set_channel (sess);
				fe_set_title (sess);
				fe_set_nonchannel (sess, FALSE);
				userlist_clear (sess);
				log_open_or_close (sess);
			}
			/* Avoid redundancy with some Undernet notices */
			if (!strncmp (msg, "*** Notice -- ", 14))
				msg += 14;
		} else
		{
			sess = serv->front_session;
		}

		if (!sess)
		{
			if (server_notice)	
				sess = serv->server_session;
			else
				sess = serv->front_session;
		}
	}

	if (msg[0] == '\001')
	{
		size_t len;

		msg++;
		if (!strncmp (msg, "PING", 4))
		{
			inbound_ping_reply (sess, msg + 5, nick, tags_data);
			return;
		}

		len = strlen(msg);
		if (msg[len - 1] == '\001')
			msg[len - 1] = '\000';
	}

	if (server_notice)
		EMIT_SIGNAL_TIMESTAMP (XP_TE_SERVNOTICE, sess, msg, nick, NULL, NULL, 0,
									  tags_data->timestamp);
	else if (ptr)
		EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANNOTICE, sess, nick, to, msg, NULL, 0,
									  tags_data->timestamp);
	else
		EMIT_SIGNAL_TIMESTAMP (XP_TE_NOTICE, sess, nick, msg, NULL, NULL, 0,
									  tags_data->timestamp);
}

void
inbound_away (server *serv, char *nick, char *msg,
				  const message_tags_data *tags_data)
{
	struct away_msg *away = server_away_find_message (serv, nick);
	session *sess = NULL;
	GSList *list;

	if (away && !strcmp (msg, away->message))	/* Seen the msg before? */
	{
		if (prefs.hex_away_show_once && !serv->inside_whois)
			return;
	} else
	{
		server_away_save_message (serv, nick, msg);
	}

	if (prefs.hex_irc_whois_front)
		sess = serv->front_session;
	else
	{
		if (!serv->inside_whois)
			sess = find_session_from_nick (nick, serv);
		if (!sess)
			sess = serv->server_session;
	}

	/* possibly hide the output */
	if (!serv->inside_whois || !serv->skip_next_whois)
		EMIT_SIGNAL_TIMESTAMP (XP_TE_WHOIS5, sess, nick, msg, NULL, NULL, 0,
									  tags_data->timestamp);

	list = sess_list;
	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
			userlist_set_away (sess, nick, TRUE);
		list = list->next;
	}
}

void
inbound_away_notify (server *serv, char *nick, char *reason,
							const message_tags_data *tags_data)
{
	session *sess = NULL;
	GSList *list;

	list = sess_list;
	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
		{
			userlist_set_away (sess, nick, reason ? TRUE : FALSE);
			if (sess == serv->front_session && notify_is_in_list (serv, nick))
			{
				if (reason)
					EMIT_SIGNAL_TIMESTAMP (XP_TE_NOTIFYAWAY, sess, nick, reason, NULL,
												  NULL, 0, tags_data->timestamp);
				else
					EMIT_SIGNAL_TIMESTAMP (XP_TE_NOTIFYBACK, sess, nick, NULL, NULL, 
												  NULL, 0, tags_data->timestamp);
			}
		}
		list = list->next;
	}
}

int
inbound_nameslist_end (server *serv, char *chan,
							  const message_tags_data *tags_data)
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
				fe_userlist_numbers (sess);
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
		fe_userlist_numbers (sess);
		return TRUE;
	}
	return FALSE;
}

static gboolean
check_autojoin_channels (server *serv)
{
	int i = 0;
	session *sess;
	GSList *list = sess_list;
	GSList *sess_channels = NULL;			/* joined channels that are not in the favorites list */
	favchannel *fav;

	/* shouldn't really happen, the io tag is destroyed in server.c */
	if (!is_server (serv))
	{
		return FALSE;
	}

	/* If there's a session (i.e. this is a reconnect), autojoin to everything that was open previously. */
	while (list)
	{
		sess = list->data;

		if (sess->server == serv)
		{
			if (sess->willjoinchannel[0] != 0)
			{
				strcpy (sess->waitchannel, sess->willjoinchannel);
				sess->willjoinchannel[0] = 0;

				fav = servlist_favchan_find (serv->network, sess->waitchannel, NULL);	/* Is this channel in our favorites? */

				/* session->channelkey is initially unset for channels joined from the favorites. You have to fill them up manually from favorites settings. */
				if (fav)
				{
					/* session->channelkey is set if there was a key change during the session. In that case, use the session key, not the one from favorites. */
					if (fav->key && !strlen (sess->channelkey))
					{
						safe_strcpy (sess->channelkey, fav->key, sizeof (sess->channelkey));
					}
				}

				/* for easier checks, ensure that favchannel->key is just NULL when session->channelkey is empty i.e. '' */
				if (strlen (sess->channelkey))
				{
					sess_channels = servlist_favchan_listadd (sess_channels, sess->waitchannel, sess->channelkey);
				}
				else
				{
					sess_channels = servlist_favchan_listadd (sess_channels, sess->waitchannel, NULL);
				}
				i++;
			}
		}

		list = list->next;
	}

	if (sess_channels)
	{
		serv->p_join_list (serv, sess_channels);
		g_slist_free_full (sess_channels, (GDestroyNotify) servlist_favchan_free);
	}
	else
	{
		/* If there's no session, just autojoin to favorites. */
		if (serv->favlist)
		{
			serv->p_join_list (serv, serv->favlist);
			i++;

			/* FIXME this is not going to work and is not needed either. server_free() does the job already. */
			/* g_slist_free_full (serv->favlist, (GDestroyNotify) servlist_favchan_free); */
		}
	}

	serv->joindelay_tag = 0;
	fe_server_event (serv, FE_SE_LOGGEDIN, i);
	return FALSE;
}

void
inbound_next_nick (session *sess, char *nick, int error,
						 const message_tags_data *tags_data)
{
	char *newnick;
	server *serv = sess->server;
	ircnet *net;

	serv->nickcount++;

	switch (serv->nickcount)
	{
	case 2:
		newnick = prefs.hex_irc_nick2;
		net = serv->network;
		/* use network specific "Second choice"? */
		if (net && !(net->flags & FLAG_USE_GLOBAL) && net->nick2)
		{
			newnick = net->nick2;
		}
		serv->p_change_nick (serv, newnick);
		if (error)
		{
			EMIT_SIGNAL_TIMESTAMP (XP_TE_NICKERROR, sess, nick, newnick, NULL, NULL,
										  0, tags_data->timestamp);
		}
		else
		{
			EMIT_SIGNAL_TIMESTAMP (XP_TE_NICKCLASH, sess, nick, newnick, NULL, NULL,
										  0, tags_data->timestamp);
		}
		break;

	case 3:
		serv->p_change_nick (serv, prefs.hex_irc_nick3);
		if (error)
		{
			EMIT_SIGNAL_TIMESTAMP (XP_TE_NICKERROR, sess, nick, prefs.hex_irc_nick3,
										  NULL, NULL, 0, tags_data->timestamp);
		}
		else
		{
			EMIT_SIGNAL_TIMESTAMP (XP_TE_NICKCLASH, sess, nick, prefs.hex_irc_nick3,
										  NULL, NULL, 0, tags_data->timestamp);
		}
		break;

	default:
		EMIT_SIGNAL_TIMESTAMP (XP_TE_NICKFAIL, sess, NULL, NULL, NULL, NULL, 0,
									  tags_data->timestamp);
	}
}


static void
dns_addr_callback (GObject *obj, GAsyncResult *result, gpointer user_data)
{
	GResolver *resolver = G_RESOLVER(obj);
	session *sess = (session*)user_data;
	gchar *addr;

	g_return_if_fail (is_session(sess));

	addr = g_resolver_lookup_by_address_finish (resolver, result, NULL);
	if (addr)
		PrintTextf (sess, _("Resolved to %s"), addr);
	else
		PrintText (sess, _("Not found"));
}

static void
dns_name_callback (GObject *obj, GAsyncResult *result, gpointer user_data)
{
	GResolver *resolver = G_RESOLVER(obj);
	session *sess = (session*)user_data;
	GList* addrs;
	gchar* addr;
	GList* list;

	g_return_if_fail (is_session (sess));

	addrs = g_resolver_lookup_by_name_finish (resolver, result, NULL);
	if (addrs)
	{
		PrintText (sess, _("Resolved to:"));

		for (list = g_list_first (addrs); list; list = g_list_next (list))
		{
			addr = g_inet_address_to_string (list->data);
			PrintTextf (sess, "    %s", addr);
		}

		g_resolver_free_addresses (addrs);
	}
	else
		PrintText (sess, _("Not found"));
}

void
do_dns (session *sess, char *nick, char *host,
		const message_tags_data *tags_data)
{
	GResolver *res = g_resolver_get_default ();
	GInetAddress *addr;
	char *po;

	po = strrchr (host, '@');
	if (po)
		host = po + 1;

	if (nick)
		EMIT_SIGNAL_TIMESTAMP (XP_TE_RESOLVINGUSER, sess, nick, host, NULL, NULL, 0,
								tags_data->timestamp);

	PrintTextf (sess, _("Looking up %s..."), host);

	addr = g_inet_address_new_from_string (host);
	if (addr)
		g_resolver_lookup_by_address_async (res, addr, NULL, dns_addr_callback, sess);
	else
		g_resolver_lookup_by_name_async (res, host, NULL, dns_name_callback, sess);
}

static void
set_default_modes (server *serv)
{
	char modes[8];

	modes[0] = '+';
	modes[1] = '\0';

	if (prefs.hex_irc_wallops)
		strcat (modes, "w");
	if (prefs.hex_irc_servernotice)
		strcat (modes, "s");
	if (prefs.hex_irc_invisible)
		strcat (modes, "i");
	if (prefs.hex_irc_hidehost)
		strcat (modes, "x");

	if (modes[1] != '\0')
	{
		serv->p_mode (serv, serv->nick, modes);
	}
}

void
inbound_login_start (session *sess, char *nick, char *servname,
							const message_tags_data *tags_data)
{
	inbound_newnick (sess->server, sess->server->nick, nick, TRUE, tags_data);
	server_set_name (sess->server, servname);
	if (sess->type == SESS_SERVER)
		log_open_or_close (sess);
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
inbound_uaway (server *serv, const message_tags_data *tags_data)
{
	serv->is_away = TRUE;
	serv->away_time = time (NULL);
	fe_set_away (serv);

	inbound_set_all_away_status (serv, serv->nick, 1);
}

void
inbound_uback (server *serv, const message_tags_data *tags_data)
{
	serv->is_away = FALSE;
	serv->reconnect_away = FALSE;
	fe_set_away (serv);

	inbound_set_all_away_status (serv, serv->nick, 0);
}

void
inbound_foundip (session *sess, char *ip, const message_tags_data *tags_data)
{
	struct hostent *HostAddr;

	HostAddr = gethostbyname (ip);
	if (HostAddr)
	{
		sess->server->dcc_ip = ((struct in_addr *) HostAddr->h_addr)->s_addr;
		EMIT_SIGNAL_TIMESTAMP (XP_TE_FOUNDIP, sess->server->server_session,
									  inet_ntoa (*((struct in_addr *) HostAddr->h_addr)),
									  NULL, NULL, NULL, 0, tags_data->timestamp);
	}
}

void
inbound_user_info_start (session *sess, char *nick,
								 const message_tags_data *tags_data)
{
	/* set away to FALSE now, 301 may turn it back on */
	inbound_set_all_away_status (sess->server, nick, 0);
}

/* reporting new information found about this user. chan may be NULL.
 * away may be 0xff to indicate UNKNOWN. */

void
inbound_user_info (session *sess, char *chan, char *user, char *host,
						 char *servname, char *nick, char *realname,
						 char *account, unsigned int away,
						 const message_tags_data *tags_data)
{
	server *serv = sess->server;
	session *who_sess;
	GSList *list;
	char *uhost = NULL;

	if (user && host)
	{
		uhost = g_strdup_printf ("%s@%s", user, host);
	}

	if (chan)
	{
		who_sess = find_channel (serv, chan);
		if (who_sess)
			userlist_add_hostname (who_sess, nick, uhost, realname, servname, account, away);
		else
		{
			if (serv->doing_dns && nick && host)
				do_dns (sess, nick, host, tags_data);
		}
	}
	else
	{
		/* came from WHOIS, not channel specific */
		for (list = sess_list; list; list = list->next)
		{
			sess = list->data;
			if (sess->server != serv)
				continue;

			if (sess->type == SESS_CHANNEL)
			{
				userlist_add_hostname (sess, nick, uhost, realname, servname, account, away);
			}
			else if (sess->type == SESS_DIALOG && uhost && !serv->p_cmp (sess->channel, nick))
			{
				set_topic (sess, uhost, uhost);
			}
		}
	}

	g_free (uhost);
}

int
inbound_banlist (session *sess, time_t stamp, char *chan, char *mask, 
					  char *banner, int rplcode, const message_tags_data *tags_data)
{
	char *time_str = ctime (&stamp);
	server *serv = sess->server;
	char *nl;

	if (stamp <= 0 || time_str == NULL)
	{
		time_str = "";
	}
	else
	{
		if ((nl = strchr (time_str, '\n')))
			*nl = 0;
	}

	sess = find_channel (serv, chan);
	if (!sess)
	{
		sess = serv->front_session;
		goto nowindow;
	}

	if (!fe_add_ban_list (sess, mask, banner, time_str, rplcode))
	{
nowindow:

		EMIT_SIGNAL_TIMESTAMP (XP_TE_BANLIST, sess, chan, mask, banner, time_str,
									  0, tags_data->timestamp);
		return TRUE;
	}

	return TRUE;
}

/* execute 1 end-of-motd command */

static int
inbound_exec_eom_cmd (char *str, void *sess)
{
	char *cmd;

	cmd = command_insert_vars ((session*)sess, (str[0] == '/') ? str + 1 : str);
	handle_command ((session*)sess, cmd, TRUE);
	g_free (cmd);

	return 1;
}

static int
inbound_nickserv_login (server *serv)
{
	/* this could grow ugly, but let's hope there won't be new NickServ types */
	switch (serv->loginmethod)
	{
		case LOGIN_MSG_NICKSERV:
		case LOGIN_NICKSERV:
		case LOGIN_CHALLENGEAUTH:
#if 0
		case LOGIN_NS:
		case LOGIN_MSG_NS:
		case LOGIN_AUTH:
#endif
			return 1;
		default:
			return 0;
	}
}

void
inbound_login_end (session *sess, char *text, const message_tags_data *tags_data)
{
	GSList *cmdlist;
	commandentry *cmd;
	server *serv = sess->server;
	ircnet *net = serv->network;

	if (!serv->end_of_motd)
	{
		if (prefs.hex_dcc_ip_from_server && serv->use_who)
		{
			serv->skip_next_userhost = TRUE;
			serv->p_get_ip_uh (serv, serv->nick);	/* sends USERHOST mynick */
		}
		set_default_modes (serv);

		if (net)
		{
			/* there may be more than 1, separated by \n */

			cmdlist = net->commandlist;
			while (cmdlist)
			{
				cmd = cmdlist->data;
				inbound_exec_eom_cmd (cmd->command, sess);
				cmdlist = cmdlist->next;
			}
		}
		/* The previously ran commands can alter the state of the server */
		if (serv->network != net)
			return;

		/* send nickserv password */
		if (net && net->pass && inbound_nickserv_login (serv))
		{
			serv->p_ns_identify (serv, net->pass);
		}

		/* wait for join if command or nickserv set */
		if (net && prefs.hex_irc_join_delay
			&& ((net->pass && inbound_nickserv_login (serv))
				|| net->commandlist))
		{
			serv->joindelay_tag = fe_timeout_add_seconds (prefs.hex_irc_join_delay, check_autojoin_channels, serv);
		}
		else
		{
			check_autojoin_channels (serv);
		}

		if (serv->supports_watch || serv->supports_monitor)
		{
			notify_send_watches (serv);
		}

		serv->end_of_motd = TRUE;
	}

	if (prefs.hex_irc_skip_motd && !serv->motd_skipped)
	{
		serv->motd_skipped = TRUE;
		EMIT_SIGNAL_TIMESTAMP (XP_TE_MOTDSKIP, serv->server_session, NULL, NULL,
									  NULL, NULL, 0, tags_data->timestamp);
		return;
	}

	EMIT_SIGNAL_TIMESTAMP (XP_TE_MOTD, serv->server_session, text, NULL, NULL,
								  NULL, 0, tags_data->timestamp);
}

void
inbound_identified (server *serv)	/* 'MODE +e MYSELF' on freenode */
{
	if (serv->joindelay_tag)
	{
		/* stop waiting, just auto JOIN now */
		fe_timeout_remove (serv->joindelay_tag);
		serv->joindelay_tag = 0;
		check_autojoin_channels (serv);
	}
}

static const char *sasl_mechanisms[] =
{
	"PLAIN",
	"EXTERNAL",
	"SCRAM-SHA-1",
	"SCRAM-SHA-256",
	"SCRAM-SHA-512"
};

static void
inbound_toggle_caps (server *serv, const char *extensions_str, gboolean enable)
{
	char **extensions;
	gsize i;

	extensions = g_strsplit (extensions_str, " ", 0);

	for (i = 0; extensions[i]; i++)
	{
		const char *extension = extensions[i];

		if (!strcmp (extension, "solanum.chat/identify-msg"))
			serv->have_idmsg = enable;
		else if (!strcmp (extension, "multi-prefix"))
			serv->have_namesx = enable;
		else if (!strcmp (extension, "account-notify"))
			serv->have_accnotify = enable;
		else if (!strcmp (extension, "extended-join"))
			serv->have_extjoin = enable;
		else if (!strcmp (extension, "userhost-in-names"))
			serv->have_uhnames = enable;
		else if (!strcmp (extension, "server-time")
				|| !strcmp (extension, "znc.in/server-time")
				|| !strcmp (extension, "znc.in/server-time-iso"))
			serv->have_server_time = enable;
		else if (!strcmp (extension, "away-notify"))
			serv->have_awaynotify = enable;
		else if (!strcmp (extension, "account-tag"))
			serv->have_account_tag = enable;
		else if (!strcmp (extension, "sasl"))
		{
			serv->have_sasl = enable;
			if (enable)
			{
#ifdef USE_OPENSSL
				if (serv->loginmethod == LOGIN_SASLEXTERNAL)
					serv->sasl_mech = MECH_EXTERNAL;
				else if (serv->loginmethod == LOGIN_SASL_SCRAM_SHA_1)
					serv->sasl_mech = MECH_SCRAM_SHA_1;
				else if (serv->loginmethod == LOGIN_SASL_SCRAM_SHA_256)
					serv->sasl_mech = MECH_SCRAM_SHA_256;
				else if (serv->loginmethod == LOGIN_SASL_SCRAM_SHA_512)
					serv->sasl_mech = MECH_SCRAM_SHA_512;
#endif
				/* Mechanism either defaulted to PLAIN or server gave us list */
				tcp_sendf (serv, "AUTHENTICATE %s\r\n", sasl_mechanisms[serv->sasl_mech]);
			}
		}
	}

	g_strfreev (extensions);
}

void
inbound_cap_ack (server *serv, char *nick, char *extensions,
					  const message_tags_data *tags_data)
{
	EMIT_SIGNAL_TIMESTAMP (XP_TE_CAPACK, serv->server_session, nick, extensions,
								  NULL, NULL, 0, tags_data->timestamp);

	inbound_toggle_caps (serv, extensions, TRUE);
}

void
inbound_cap_del (server *serv, char *nick, char *extensions,
					 const message_tags_data *tags_data)
{
	EMIT_SIGNAL_TIMESTAMP (XP_TE_CAPDEL, serv->server_session, nick, extensions,
								  NULL, NULL, 0, tags_data->timestamp);

	inbound_toggle_caps (serv, extensions, FALSE);
}

static const char * const supported_caps[] = {
	/* IRCv3.1 */
	"multi-prefix",
	"away-notify",
	"account-notify",
	"extended-join",
	/* "sasl", Handled manually */

	/* IRCv3.2 */
	"server-time",
	"userhost-in-names",
	"cap-notify",
	"chghost",
	"setname",
	"invite-notify",
	"account-tag",
	"extended-monitor",

	/* ZNC */
	"znc.in/server-time-iso",
	"znc.in/server-time",

	/* Twitch */
	"twitch.tv/membership",

	/* Solanum */
	"solanum.chat/identify-msg",
};

static int
get_supported_mech (server *serv, const char *list)
{
	char **mechs = g_strsplit (list, ",", 0);
	gsize i;
	int ret = -1;

	for (i = 0; mechs[i]; ++i)
	{
#ifdef USE_OPENSSL
		if (serv->loginmethod == LOGIN_SASLEXTERNAL)
		{
			if (!strcmp (mechs[i], "EXTERNAL"))
			{
				ret = MECH_EXTERNAL;
				break;
			}
		}
		else if (serv->loginmethod == LOGIN_SASL_SCRAM_SHA_1)
		{
			if (!strcmp(mechs[i], "SCRAM-SHA-1"))
			{
				ret = MECH_SCRAM_SHA_1;
				break;
			}
		}
		else if (serv->loginmethod == LOGIN_SASL_SCRAM_SHA_256)
		{
			if (!strcmp(mechs[i], "SCRAM-SHA-256"))
			{
				ret = MECH_SCRAM_SHA_256;
				break;
			}
		}
		else if (serv->loginmethod == LOGIN_SASL_SCRAM_SHA_512)
		{
			if (!strcmp(mechs[i], "SCRAM-SHA-512"))
			{
				ret = MECH_SCRAM_SHA_512;
				break;
			}
        }
		else
#endif
		if (!strcmp (mechs[i], "PLAIN"))
		{
			ret = MECH_PLAIN;
			break;
		}
	}

	g_strfreev (mechs);
	return ret;
}

void
inbound_cap_ls (server *serv, char *nick, char *extensions_str,
					 const message_tags_data *tags_data)
{
	char buffer[500];	/* buffer for requesting capabilities and emitting the signal */
	gboolean want_cap = FALSE; /* format the CAP REQ string based on previous capabilities being requested or not */
	char **extensions;
	int i;

	if (g_str_has_prefix (extensions_str, "* "))
	{
		serv->waiting_on_cap = TRUE;
		extensions_str += 2;
		extensions_str += extensions_str[0] == ':' ? 1 : 0;
	}
	else
	{
		serv->waiting_on_cap = FALSE;
	}

	EMIT_SIGNAL_TIMESTAMP (XP_TE_CAPLIST, serv->server_session, nick,
								  extensions_str, NULL, NULL, 0, tags_data->timestamp);

	extensions = g_strsplit (extensions_str, " ", 0);

	strcpy (buffer, "CAP REQ :");

	for (i=0; extensions[i]; i++)
	{
		char *extension = extensions[i];
		char *value;
		gsize x;

		/* CAP 3.2 can provide values */
		if ((value = strchr (extension, '=')))
		{
			*value = '\0';
			value++;
		}

		/* if the SASL password is set AND auth mode is set to SASL, request SASL auth */
		if (!g_strcmp0 (extension, "sasl") &&
			(((serv->loginmethod == LOGIN_SASL
				|| serv->loginmethod == LOGIN_SASL_SCRAM_SHA_1
				|| serv->loginmethod == LOGIN_SASL_SCRAM_SHA_256
				|| serv->loginmethod == LOGIN_SASL_SCRAM_SHA_512)
					&& strlen (serv->password) != 0)
				|| serv->loginmethod == LOGIN_SASLEXTERNAL))
		{
			if (value)
			{
				int sasl_mech = get_supported_mech (serv, value);
				if (sasl_mech == -1) /* No supported mech */
					continue;
				serv->sasl_mech = sasl_mech;
			}
			want_cap = TRUE;
			serv->waiting_on_sasl = TRUE;
			g_strlcat (buffer, "sasl ", sizeof(buffer));
			continue;
		}

		for (x = 0; x < G_N_ELEMENTS(supported_caps); ++x)
		{
			if (!g_strcmp0 (extension, supported_caps[x]))
			{
				g_strlcat (buffer, extension, sizeof(buffer));
				g_strlcat (buffer, " ", sizeof(buffer));
				want_cap = TRUE;
			}
		}
	}

	g_strfreev (extensions);

	if (want_cap)
	{
		/* buffer + 9 = emit buffer without "CAP REQ :" */
		EMIT_SIGNAL_TIMESTAMP (XP_TE_CAPREQ, serv->server_session,
									  buffer + 9, NULL, NULL, NULL, 0,
									  tags_data->timestamp);
		tcp_sendf (serv, "%s\r\n", g_strchomp (buffer));
	}
	if (!serv->waiting_on_sasl && !serv->waiting_on_cap)
	{
		/* if we use SASL, CAP END is dealt via raw numerics */
		serv->sent_capend = TRUE;
		tcp_send_len (serv, "CAP END\r\n", 9);
	}
}

void
inbound_cap_nak (server *serv, char *extensions_str, const message_tags_data *tags_data)
{
	char **extensions;
	int i;

	extensions = g_strsplit (extensions_str, " ", 0);
	for (i=0; extensions[i]; i++)
	{
		if (!g_strcmp0 (extensions[i], "sasl"))
			serv->waiting_on_sasl = FALSE;
	}

	if (!serv->waiting_on_cap && !serv->waiting_on_sasl && !serv->sent_capend)
	{
		serv->sent_capend = TRUE;
		tcp_send_len (serv, "CAP END\r\n", 9);
	}

	g_strfreev (extensions);
}

void
inbound_cap_list (server *serv, char *nick, char *extensions,
						const message_tags_data *tags_data)
{
	if (g_str_has_prefix (extensions, "* "))
	{
		extensions += 2;
		extensions += extensions[0] == ':' ? 1 : 0;
	}
	EMIT_SIGNAL_TIMESTAMP (XP_TE_CAPACK, serv->server_session, nick, extensions,
								  NULL, NULL, 0, tags_data->timestamp);
}

static void
plain_authenticate (server *serv, char *user, char *password)
{
	char *pass = encode_sasl_pass_plain (user, password);

	if (pass == NULL)
	{
		/* something went wrong abort */
		tcp_sendf (serv, "AUTHENTICATE *\r\n");
		return;
	}

	/* long SASL passwords must be split into 400-byte chunks
	   https://ircv3.net/specs/extensions/sasl-3.1#the-authenticate-command */
	size_t pass_len = strlen (pass);
	if (pass_len <= 400)
		tcp_sendf (serv, "AUTHENTICATE %s\r\n", pass);
	else
	{
		size_t sent = 0;
		while (sent < pass_len)
		{
			char *pass_chunk = g_strndup (pass + sent, 400);
			tcp_sendf (serv, "AUTHENTICATE %s\r\n", pass_chunk);
			sent += 400;
			g_free (pass_chunk);
		}
	}
	if (pass_len % 400 == 0)
		tcp_sendf (serv, "AUTHENTICATE +\r\n");
}

#ifdef USE_OPENSSL
/*
 * Sends AUTHENTICATE messages to log in via SCRAM.
 */
static void
scram_authenticate (server *serv, const char *data, const char *digest,
					const char *user, const char *password)
{
	char *encoded, *decoded, *output;
	scram_status status;
	size_t output_len;
	gsize decoded_len;

	if (serv->scram_session == NULL)
	{
		serv->scram_session = scram_session_create (digest, user, password);

		if (serv->scram_session == NULL)
		{
			PrintTextf (serv->server_session, _("Could not create SCRAM session with digest %s"), digest);
			g_warning ("Could not create SCRAM session with digest %s", digest);
			tcp_sendf (serv, "AUTHENTICATE *\r\n");
			return;
		}
	}

	decoded = g_base64_decode (data, &decoded_len);
	status = scram_process (serv->scram_session, decoded, &output, &output_len);
	g_free (decoded);

	if (status == SCRAM_IN_PROGRESS)
	{
		// Authentication is still in progress
		encoded = g_base64_encode ((guchar *) output, output_len);
		tcp_sendf (serv, "AUTHENTICATE %s\r\n", encoded);
		g_free (encoded);
		g_free (output);
	}
	else if (status == SCRAM_SUCCESS)
	{
		// Authentication succeeded
		tcp_sendf (serv, "AUTHENTICATE +\r\n");
		g_clear_pointer (&serv->scram_session, scram_session_free);
	}
	else if (status == SCRAM_ERROR)
	{
		// Authentication failed
		tcp_sendf (serv, "AUTHENTICATE *\r\n");

		if (serv->scram_session->error != NULL)
		{
			PrintTextf (serv->server_session, _("SASL SCRAM authentication failed: %s"), serv->scram_session->error);
			g_info ("SASL SCRAM authentication failed: %s", serv->scram_session->error);
		}

		g_clear_pointer (&serv->scram_session, scram_session_free);
	}
}
#endif

void
inbound_sasl_authenticate (server *serv, char *data)
{
		ircnet *net = (ircnet*)serv->network;
		char *user;
		const char *mech = sasl_mechanisms[serv->sasl_mech];

		/* Got a list of supported mechanisms from outdated inspircd
		 * just ignore it as it goes against spec */
		if (strchr (data, ',') != NULL)
			return;

		if (net->user && !(net->flags & FLAG_USE_GLOBAL))
			user = net->user;
		else
			user = prefs.hex_irc_user_name;

		switch (serv->sasl_mech)
		{
		case MECH_PLAIN:
			plain_authenticate(serv, user, serv->password);
			break;
#ifdef USE_OPENSSL
		case MECH_EXTERNAL:
			tcp_sendf (serv, "AUTHENTICATE +\r\n");
			break;
		case MECH_SCRAM_SHA_1:
			scram_authenticate(serv, data, "SHA1", user, serv->password);
			return;
		case MECH_SCRAM_SHA_256:
			scram_authenticate(serv, data, "SHA256", user, serv->password);
			return;
		case MECH_SCRAM_SHA_512:
			scram_authenticate(serv, data, "SHA512", user, serv->password);
			return;
#endif
		}

		EMIT_SIGNAL_TIMESTAMP (XP_TE_SASLAUTH, serv->server_session, user, (char*)mech,
								NULL,	NULL,	0,	0);
}

void
inbound_sasl_error (server *serv)
{
#ifdef USE_OPENSSL
    g_clear_pointer (&serv->scram_session, scram_session_free);
#endif
	/* Just abort, not much we can do */
	tcp_sendf (serv, "AUTHENTICATE *\r\n");
}
