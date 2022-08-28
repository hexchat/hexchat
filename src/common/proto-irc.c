/* X-Chat
 * Copyright (C) 2002 Peter Zelezny.
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

/* IRC RFC1459(+commonly used extensions) protocol implementation */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include "hexchat.h"
#include "proto-irc.h"
#include "ctcp.h"
#include "fe.h"
#include "ignore.h"
#include "inbound.h"
#include "modes.h"
#include "notify.h"
#include "plugin.h"
#include "server.h"
#include "text.h"
#include "outbound.h"
#include "util.h"
#include "hexchatc.h"
#include "url.h"
#include "servlist.h"

static void
irc_login (server *serv, char *user, char *realname)
{
	tcp_sendf (serv, "CAP LS 302\r\n");		/* start with CAP LS as Charybdis sasl.txt suggests */
	serv->sent_capend = FALSE;	/* track if we have finished */

	if (serv->password[0] && serv->loginmethod == LOGIN_PASS)
	{
		tcp_sendf (serv, "PASS %s%s\r\n",
			(serv->password[0] == ':' || strchr (serv->password, ' ')) ? ":" : "",
			serv->password);
	}

	tcp_sendf (serv,
				  "NICK %s\r\n"
				  "USER %s 0 * :%s\r\n",
				  serv->nick, user, realname);
}

static void
irc_nickserv (server *serv, char *cmd, char *arg1, char *arg2, char *arg3)
{
	/* are all ircd authors idiots? */
	switch (serv->loginmethod)
	{
		case LOGIN_MSG_NICKSERV:
			tcp_sendf (serv, "PRIVMSG NICKSERV :%s %s%s%s\r\n", cmd, arg1, arg2, arg3);
			break;
		case LOGIN_NICKSERV:
			tcp_sendf (serv, "NICKSERV %s %s%s%s\r\n", cmd, arg1, arg2, arg3);
			break;
		default: /* This may not work but at least it tries something when using /id or /ghost cmd */
			tcp_sendf (serv, "NICKSERV %s %s%s%s\r\n", cmd, arg1, arg2, arg3);
			break;
#if 0
		case LOGIN_MSG_NS:
			tcp_sendf (serv, "PRIVMSG NS :%s %s%s%s\r\n", cmd, arg1, arg2, arg3);
			break;
		case LOGIN_NS:
			tcp_sendf (serv, "NS %s %s%s%s\r\n", cmd, arg1, arg2, arg3);
			break;
		case LOGIN_AUTH:
			/* why couldn't QuakeNet implement one of the existing ones? */
			tcp_sendf (serv, "AUTH %s %s\r\n", arg1, arg2);
			break;
#endif
	}
}

static void
irc_ns_identify (server *serv, char *pass)
{
	switch (serv->loginmethod)
	{
		case LOGIN_CHALLENGEAUTH:
			tcp_sendf (serv, "PRIVMSG %s :CHALLENGE\r\n", CHALLENGEAUTH_NICK);	/* request a challenge from Q */
			break;
#if 0
		case LOGIN_AUTH:
			irc_nickserv (serv, "", serv->nick, pass, "");
			break;
#endif
		default:
			irc_nickserv (serv, "IDENTIFY", pass, "", "");
	}
}

static void
irc_ns_ghost (server *serv, char *usname, char *pass)
{
	if (serv->loginmethod != LOGIN_CHALLENGEAUTH)
	{
		irc_nickserv (serv, "GHOST", usname, " ", pass);
	}
}

static void
irc_join (server *serv, char *channel, char *key)
{
	if (key[0])
		tcp_sendf (serv, "JOIN %s %s\r\n", channel, key);
	else
		tcp_sendf (serv, "JOIN %s\r\n", channel);
}

static void
irc_join_list_flush (server *serv, GString *channels, GString *keys, int send_keys)
{
	char *chanstr;
	char *keystr;

	chanstr = g_string_free (channels, FALSE);				/* convert our strings to char arrays */
	keystr = g_string_free (keys, FALSE);

	if (send_keys)
	{
		tcp_sendf (serv, "JOIN %s %s\r\n", chanstr, keystr);	/* send the actual command */
	}
	else
	{
		tcp_sendf (serv, "JOIN %s\r\n", chanstr);	/* send the actual command */
	}

	g_free (chanstr);
	g_free (keystr);
}

/* Join a whole list of channels & keys, split to multiple lines
 * to get around the 512 limit.
 */

static void
irc_join_list (server *serv, GSList *favorites)
{
	int first_item = 1;										/* determine whether we add commas or not */
	int send_keys = 0;										/* if none of our channels have keys, we can omit the 'x' fillers altogether */
	int len = 9;											/* JOIN<space>channels<space>keys\r\n\0 */
	favchannel *fav;
	GString *chanlist = g_string_new (NULL);
	GString *keylist = g_string_new (NULL);
	GSList *favlist;

	favlist = favorites;

	while (favlist)
	{
		fav = favlist->data;

		len += strlen (fav->name);
		if (fav->key)
		{
			len += strlen (fav->key);
		}

		if (len >= 512)										/* command length exceeds the IRC hard limit, flush it and start from scratch */
		{
			irc_join_list_flush (serv, chanlist, keylist, send_keys);

			chanlist = g_string_new (NULL);
			keylist = g_string_new (NULL);

			len = 9;
			first_item = 1;									/* list dumped, omit commas once again */
			send_keys = 0;									/* also omit keys until we actually find one */
		}

		if (!first_item)
		{
			/* This should be done before the length check, but channel names
			 * are already at least 2 characters long so it would trigger the
			 * flush anyway.
			 */
			len += 2;

			/* add separators but only if it's not the 1st element */
			g_string_append_c (chanlist, ',');
			g_string_append_c (keylist, ',');
		}

		g_string_append (chanlist, fav->name);

		if (fav->key)
		{
			g_string_append (keylist, fav->key);
			send_keys = 1;
		}
		else
		{
			g_string_append_c (keylist, 'x');				/* 'x' filler for keyless channels so that our JOIN command is always well-formatted */
		}

		first_item = 0;
		favlist = favlist->next;
	}

	irc_join_list_flush (serv, chanlist, keylist, send_keys);
	g_slist_free (favlist);
}

static void
irc_part (server *serv, char *channel, char *reason)
{
	if (reason[0])
		tcp_sendf (serv, "PART %s :%s\r\n", channel, reason);
	else
		tcp_sendf (serv, "PART %s\r\n", channel);
}

static void
irc_quit (server *serv, char *reason)
{
	if (reason[0])
		tcp_sendf (serv, "QUIT :%s\r\n", reason);
	else
		tcp_send_len (serv, "QUIT\r\n", 6);
}

static void
irc_set_back (server *serv)
{
	tcp_send_len (serv, "AWAY\r\n", 6);
}

static void
irc_set_away (server *serv, char *reason)
{
	if (reason)
	{
		if (!reason[0])
			reason = " ";
	}
	else
	{
		reason = " ";
	}

	tcp_sendf (serv, "AWAY :%s\r\n", reason);
}

static void
irc_ctcp (server *serv, char *to, char *msg)
{
	tcp_sendf (serv, "PRIVMSG %s :\001%s\001\r\n", to, msg);
}

static void
irc_nctcp (server *serv, char *to, char *msg)
{
	tcp_sendf (serv, "NOTICE %s :\001%s\001\r\n", to, msg);
}

static void
irc_cycle (server *serv, char *channel, char *key)
{
	tcp_sendf (serv, "PART %s\r\nJOIN %s %s\r\n", channel, channel, key);
}

static void
irc_kick (server *serv, char *channel, char *nick, char *reason)
{
	if (reason[0])
		tcp_sendf (serv, "KICK %s %s :%s\r\n", channel, nick, reason);
	else
		tcp_sendf (serv, "KICK %s %s\r\n", channel, nick);
}

static void
irc_invite (server *serv, char *channel, char *nick)
{
	tcp_sendf (serv, "INVITE %s %s\r\n", nick, channel);
}

static void
irc_mode (server *serv, char *target, char *mode)
{
	tcp_sendf (serv, "MODE %s %s\r\n", target, mode);
}

/* find channel info when joined */

static void
irc_join_info (server *serv, char *channel)
{
	tcp_sendf (serv, "MODE %s\r\n", channel);
}

/* initiate userlist retreival */

static void
irc_user_list (server *serv, char *channel)
{
	if (serv->have_whox)
		tcp_sendf (serv, "WHO %s %%chtsunfra,152\r\n", channel);
	else
		tcp_sendf (serv, "WHO %s\r\n", channel);
}

/* userhost */

static void
irc_userhost (server *serv, char *nick)
{
	tcp_sendf (serv, "USERHOST %s\r\n", nick);
}

static void
irc_away_status (server *serv, char *channel)
{
	if (serv->have_whox)
		tcp_sendf (serv, "WHO %s %%chtsunfra,152\r\n", channel);
	else
		tcp_sendf (serv, "WHO %s\r\n", channel);
}

/*static void
irc_get_ip (server *serv, char *nick)
{
	tcp_sendf (serv, "WHO %s\r\n", nick);
}*/


/*
 *  Command: WHOIS
 *     Parameters: [<server>] <nickmask>[,<nickmask>[,...]]
 */
static void
irc_user_whois (server *serv, char *nicks)
{
	tcp_sendf (serv, "WHOIS %s\r\n", nicks);
}

static void
irc_message (server *serv, char *channel, char *text)
{
	tcp_sendf (serv, "PRIVMSG %s :%s\r\n", channel, text);
}

static void
irc_action (server *serv, char *channel, char *act)
{
	tcp_sendf (serv, "PRIVMSG %s :\001ACTION %s\001\r\n", channel, act);
}

static void
irc_notice (server *serv, char *channel, char *text)
{
	tcp_sendf (serv, "NOTICE %s :%s\r\n", channel, text);
}

static void
irc_topic (server *serv, char *channel, char *topic)
{
	if (!topic)
		tcp_sendf (serv, "TOPIC %s :\r\n", channel);
	else if (topic[0])
		tcp_sendf (serv, "TOPIC %s :%s\r\n", channel, topic);
	else
		tcp_sendf (serv, "TOPIC %s\r\n", channel);
}

static void
irc_list_channels (server *serv, char *arg, int min_users)
{
	if (arg[0])
	{
		tcp_sendf (serv, "LIST %s\r\n", arg);
		return;
	}

	if (serv->use_listargs)
		tcp_sendf (serv, "LIST >%d,<10000\r\n", min_users - 1);
	else
		tcp_send_len (serv, "LIST\r\n", 6);
}

static void
irc_names (server *serv, char *channel)
{
	tcp_sendf (serv, "NAMES %s\r\n", channel);
}

static void
irc_change_nick (server *serv, char *new_nick)
{
	tcp_sendf (serv, "NICK %s\r\n", new_nick);
}

static void
irc_ping (server *serv, char *to, char *timestring)
{
	if (*to)
		tcp_sendf (serv, "PRIVMSG %s :\001PING %s\001\r\n", to, timestring);
	else
		tcp_sendf (serv, "PING %s\r\n", timestring);
}

static int
irc_raw (server *serv, char *raw)
{
	int len;
	char tbuf[4096];
	if (*raw)
	{
		len = strlen (raw);
		if (len < sizeof (tbuf) - 3)
		{
			len = g_snprintf (tbuf, sizeof (tbuf), "%s\r\n", raw);
			tcp_send_len (serv, tbuf, len);
		} else
		{
			tcp_send_len (serv, raw, len);
			tcp_send_len (serv, "\r\n", 2);
		}
		return TRUE;
	}
	return FALSE;
}

/* ============================================================== */
/* ======================= IRC INPUT ============================ */
/* ============================================================== */


static void
channel_date (session *sess, char *chan, char *timestr,
				  const message_tags_data *tags_data)
{
	time_t timestamp = (time_t) atol (timestr);
	char *tim = ctime (&timestamp);
	if (tim != NULL)
		tim[24] = 0;	/* get rid of the \n */
	EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANDATE, sess, chan, tim, NULL, NULL, 0,
								  tags_data->timestamp);
}

static int
trailing_index(char *word_eol[])
{
	int param_index;
	for (param_index = 3; param_index < PDIWORDS; ++param_index)
	{
		if (word_eol[param_index][0] == ':')
			break;
	}
	return param_index;
}

static void
process_numeric (session * sess, int n,
					  char *word[], char *word_eol[], char *text,
					  const message_tags_data *tags_data)
{
	server *serv = sess->server;
	/* show whois is the server tab */
	session *whois_sess = serv->server_session;
	
	/* unless this setting is on */
	if (prefs.hex_irc_whois_front)
		whois_sess = serv->front_session;

	switch (n)
	{
	case 1:
		inbound_login_start (sess, word[3], word[1], tags_data);
		/* if network is PTnet then you must get your IP address
			from "001" server message */
		if ((strncmp(word[7], "PTnet", 5) == 0) &&
			(strncmp(word[8], "IRC", 3) == 0) &&
			(strncmp(word[9], "Network", 7) == 0) &&
			(strrchr(word[10], '@') != NULL))
		{
			serv->use_who = FALSE;
			if (prefs.hex_dcc_ip_from_server)
				inbound_foundip (sess, strrchr(word[10], '@')+1, tags_data);
		}

		goto def;

	case 5:
		inbound_005 (serv, word, tags_data);
		goto def;

	case 263:	/*Server load is temporarily too heavy */
		if (fe_is_chanwindow (sess->server))
		{
			fe_chan_list_end (sess->server);
			fe_message (word_eol[4], FE_MSG_ERROR);
		}
		goto def;

	case 301:
		inbound_away (serv, word[4],
						  (word_eol[5][0] == ':') ? word_eol[5] + 1 : word_eol[5],
						  tags_data);
		break;

	case 302:
		if (serv->skip_next_userhost)
		{
			char *eq = strchr (word[4], '=');
			if (eq)
			{
				*eq = 0;
				if (!serv->p_cmp (word[4] + 1, serv->nick))
				{
					char *at = strrchr (eq + 1, '@');
					if (at)
						inbound_foundip (sess, at + 1, tags_data);
				}
			}

			serv->skip_next_userhost = FALSE;
			break;
		}
		else goto def;

	case 303:
		word[4]++;
		notify_markonline (serv, word, tags_data);
		break;

	case 305:
		inbound_uback (serv, tags_data);
		goto def;

	case 306:
		inbound_uaway (serv, tags_data);
		goto def;

	case 312:
		if (!serv->skip_next_whois)
			EMIT_SIGNAL_TIMESTAMP (XP_TE_WHOIS3, whois_sess, word[4], word_eol[5],
										  NULL, NULL, 0, tags_data->timestamp);
		else
			inbound_user_info (sess, NULL, NULL, NULL, word[5], word[4], NULL, NULL,
									 0xff, tags_data);
		break;

	case 311:	/* WHOIS 1st line */
		serv->inside_whois = 1;
		inbound_user_info_start (sess, word[4], tags_data);
		if (!serv->skip_next_whois)
			EMIT_SIGNAL_TIMESTAMP (XP_TE_WHOIS1, whois_sess, word[4], word[5],
										  word[6], (word_eol[8][0] == ':') ? word_eol[8] + 1 : word_eol[8],
										  0, tags_data->timestamp);
		else
			inbound_user_info (sess, NULL, word[5], word[6], NULL, word[4],
									 word_eol[8][0] == ':' ? word_eol[8] + 1 : word_eol[8],
									 NULL, 0xff, tags_data);
		break;

	case 314:	/* WHOWAS */
		inbound_user_info_start (sess, word[4], tags_data);
		EMIT_SIGNAL_TIMESTAMP (XP_TE_WHOIS1, whois_sess, word[4], word[5],
									  word[6], word_eol[8] + 1, 0, tags_data->timestamp);
		break;

	case 317:
		if (!serv->skip_next_whois)
		{
			time_t timestamp = (time_t) atol (word[6]);
			long idle = atol (word[5]);
			char *tim;
			char outbuf[64];

			g_snprintf (outbuf, sizeof (outbuf),
						"%02ld:%02ld:%02ld", idle / 3600, (idle / 60) % 60,
						idle % 60);
			if (timestamp == 0)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_WHOIS4, whois_sess, word[4],
											  outbuf, NULL, NULL, 0, tags_data->timestamp);
			else
			{
				tim = ctime (&timestamp);
				if (tim != NULL)
					tim[19] = 0; 	/* get rid of the \n */
				EMIT_SIGNAL_TIMESTAMP (XP_TE_WHOIS4T, whois_sess, word[4],
											  outbuf, tim, NULL, 0, tags_data->timestamp);
			}
		}
		break;

	case 318:	/* END OF WHOIS */
		if (!serv->skip_next_whois)
			EMIT_SIGNAL_TIMESTAMP (XP_TE_WHOIS6, whois_sess, word[4], NULL,
										  NULL, NULL, 0, tags_data->timestamp);
		serv->skip_next_whois = 0;
		serv->inside_whois = 0;
		break;

	case 313:
	case 319:
		if (!serv->skip_next_whois)
			EMIT_SIGNAL_TIMESTAMP (XP_TE_WHOIS2, whois_sess, word[4],
										  word_eol[5][0] == ':' ? word_eol[5] + 1 : word_eol[5], NULL, NULL, 0,
										  tags_data->timestamp);
		break;

	case 307:	/* dalnet version */
	case 320:	/* :is an identified user */
		if (!serv->skip_next_whois)
			EMIT_SIGNAL_TIMESTAMP (XP_TE_WHOIS_ID, whois_sess, word[4],
										  word_eol[5][0] == ':' ? word_eol[5] + 1 : word_eol[5], NULL, NULL, 0,
										  tags_data->timestamp);
		break;

	case 321:
		if (!fe_is_chanwindow (sess->server))
			EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANLISTHEAD, serv->server_session, NULL,
										  NULL, NULL, NULL, 0, tags_data->timestamp);
		break;

	case 322:
		if (fe_is_chanwindow (sess->server))
		{
			fe_add_chan_list (sess->server, word[4], word[5], word_eol[6] + 1);
		} else
		{
			PrintTextTimeStampf (serv->server_session, tags_data->timestamp,
										"%-16s %-7d %s\017\n", word[4], atoi (word[5]),
										word_eol[6] + 1);
		}
		break;

	case 323:
		if (!fe_is_chanwindow (sess->server))
			EMIT_SIGNAL_TIMESTAMP (XP_TE_SERVTEXT, serv->server_session, text, 
										  word[1], word[2], NULL, 0, tags_data->timestamp);
		else
			fe_chan_list_end (sess->server);
		break;

	case 324:
		sess = find_channel (serv, word[4]);
		if (!sess)
			sess = serv->server_session;
		if (sess->ignore_mode)
			sess->ignore_mode = FALSE;
		else
			EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANMODES, sess, word[4], (word_eol[5][0] == ':') ? word_eol[5] + 1 : word_eol[5],
										  NULL, NULL, 0, tags_data->timestamp);
		fe_update_mode_buttons (sess, 'c', '-');
		fe_update_mode_buttons (sess, 't', '-');
		fe_update_mode_buttons (sess, 'n', '-');
		fe_update_mode_buttons (sess, 'i', '-');
		fe_update_mode_buttons (sess, 'm', '-');
		fe_update_mode_buttons (sess, 'l', '-');
		fe_update_mode_buttons (sess, 'k', '-');
		handle_mode (serv, word, word_eol, "", TRUE, tags_data);
		break;

	case 328: /* channel url */
		sess = find_channel (serv, word[4]);
		if (sess)
		{
			EMIT_SIGNAL_TIMESTAMP (XP_TE_CHANURL, sess, word[4], (word_eol[5][0] == ':') ? word_eol[5] + 1 : word_eol[5],
									NULL, NULL, 0, tags_data->timestamp); 
		}
		break;

	case 329:
		sess = find_channel (serv, word[4]);
		if (sess)
		{
			if (sess->ignore_date)
				sess->ignore_date = FALSE;
			else
				channel_date (sess, word[4], (word[5][0] == ':') ? word[5] + 1 : word[5], tags_data);
		}
		break;

	case 330:
		if (!serv->skip_next_whois)
			EMIT_SIGNAL_TIMESTAMP (XP_TE_WHOIS_AUTH, whois_sess, word[4],
										  word_eol[6] + 1, word[5], NULL, 0,
										  tags_data->timestamp);
		inbound_user_info (sess, NULL, NULL, NULL, NULL, word[4], NULL, word[5],
								 0xff, tags_data);
		break;

	case 332:
		inbound_topic (serv, word[4],
							(word_eol[5][0] == ':') ? word_eol[5] + 1 : word_eol[5],
							tags_data);
		break;

	case 333:
		inbound_topictime (serv, word[4], word[5], atol (STRIP_COLON(word, word_eol, 6)), tags_data);
		break;

#if 0
	case 338:  /* Undernet Real user@host, Real IP */
		EMIT_SIGNAL_TIMESTAMP (XP_TE_WHOIS_REALHOST, sess, word[4], word[5], word[6], 
									  (word_eol[7][0]==':') ? word_eol[7]+1 : word_eol[7],
									  0, tags_data->timestamp);
		break;
#endif

	case 341:						  /* INVITE ACK */
		EMIT_SIGNAL_TIMESTAMP (XP_TE_UINVITE, sess, word[4], STRIP_COLON(word, word_eol, 5),
									  serv->servername, NULL, 0, tags_data->timestamp);
		break;

	case 352:						  /* WHO */
		{
			unsigned int away = 0;
			session *who_sess = find_channel (serv, word[4]);

			if (*word[9] == 'G')
				away = 1;

			inbound_user_info (sess, word[4], word[5], word[6], word[7],
									 word[8], word_eol[11], NULL, away,
									 tags_data);

			/* try to show only user initiated whos */
			if (!who_sess || !who_sess->doing_who)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_SERVTEXT, serv->server_session, text, word[1],
											  word[2], NULL, 0, tags_data->timestamp);
		}
		break;

	case 354:	/* undernet WHOX: used as a reply for irc_away_status */
		{
			unsigned int away = 0;
			session *who_sess;

			/* irc_away_status and irc_user_list sends out a "152" */
			if (!strcmp (word[4], "152"))
			{
				who_sess = find_channel (serv, word[5]);

				if (*word[10] == 'G')
					away = 1;

				/* :server 354 yournick 152 #channel ~ident host servname nick H account :realname */
				inbound_user_info (sess, word[5], word[6], word[7], word[8],
										 word[9], word_eol[12]+1, word[11], away,
										 tags_data);

				/* try to show only user initiated whos */
				if (!who_sess || !who_sess->doing_who)
					EMIT_SIGNAL_TIMESTAMP (XP_TE_SERVTEXT, serv->server_session, text,
												  word[1], word[2], NULL, 0,
												  tags_data->timestamp);
			} else
				goto def;
		}
		break;

	case 315:						  /* END OF WHO */
		{
			session *who_sess;
			who_sess = find_channel (serv, word[4]);
			if (who_sess)
			{
				if (!who_sess->doing_who)
					EMIT_SIGNAL_TIMESTAMP (XP_TE_SERVTEXT, serv->server_session, text,
												  word[1], word[2], NULL, 0,
												  tags_data->timestamp);
				who_sess->doing_who = FALSE;
			} else
			{
				if (!serv->doing_dns)
					EMIT_SIGNAL_TIMESTAMP (XP_TE_SERVTEXT, serv->server_session, text,
												  word[1], word[2], NULL, 0, tags_data->timestamp);
				serv->doing_dns = FALSE;
			}
		}
		break;

	case 346:	/* +I-list entry */
		if (!inbound_banlist (sess, atol (STRIP_COLON (word, word_eol, 7)), word[4], word[5], word[6], 346,
									 tags_data))
			goto def;
		break;

	case 347:	/* end of invite list */
		if (!fe_ban_list_end (sess, 347))
			goto def;
		break;

	case 348:	/* +e-list entry */
		if (!inbound_banlist (sess, atol (STRIP_COLON (word, word_eol, 7)), word[4], word[5], word[6], 348,
									 tags_data))
			goto def;
		break;

	case 349:	/* end of exemption list */
		sess = find_channel (serv, word[4]);
		if (!sess)
			goto def;
		if (!fe_ban_list_end (sess, 349))
			goto def;
		break;

	case 353:						  /* NAMES */
		inbound_nameslist (serv, word[5],
								 (word_eol[6][0] == ':') ? word_eol[6] + 1 : word_eol[6],
								 tags_data);
		break;

	case 366:
		if (!inbound_nameslist_end (serv, word[4], tags_data))
			goto def;
		break;

	case 367: /* banlist entry */
		if (!inbound_banlist (sess, atol (STRIP_COLON (word, word_eol, 7)), word[4], word[5], word[6], 367,
									 tags_data))
			goto def;
		break;

	case 368:
		sess = find_channel (serv, word[4]);
		if (!sess)
			goto def;
		if (!fe_ban_list_end (sess, 368))
			goto def;
		break;

	case 369:	/* WHOWAS end */
	case 406:	/* WHOWAS error */
		EMIT_SIGNAL_TIMESTAMP (XP_TE_SERVTEXT, whois_sess, text, word[1], word[2],
									  NULL, 0, tags_data->timestamp);
		serv->inside_whois = 0;
		break;

	case 372:	/* motd text */
	case 375:	/* motd start */
		if (!prefs.hex_irc_skip_motd || serv->motd_skipped)
			EMIT_SIGNAL_TIMESTAMP (XP_TE_MOTD, serv->server_session, text, NULL,
										  NULL, NULL, 0, tags_data->timestamp);
		break;

	case 376:	/* end of motd */
	case 422:	/* motd file is missing */
		inbound_login_end (sess, text, tags_data);
		break;

	case 432:	/* erroneous nickname */
		if (serv->end_of_motd)
		{
			goto def;
		}
		inbound_next_nick (sess,  word[4], 1, tags_data);
		break;

	case 433:	/* nickname in use */
		if (serv->end_of_motd)
		{
			goto def;
		}
		inbound_next_nick (sess,  word[4], 0, tags_data);
		break;

	case 437:
		if (serv->end_of_motd || is_channel (serv, word[4]))
			goto def;
		inbound_next_nick (sess, word[4], 0, tags_data);
		break;

	case 471:
		EMIT_SIGNAL_TIMESTAMP (XP_TE_USERLIMIT, sess, word[4], NULL, NULL, NULL, 0,
									  tags_data->timestamp);
		break;

	case 473:
		EMIT_SIGNAL_TIMESTAMP (XP_TE_INVITE, sess, word[4], NULL, NULL, NULL, 0,
									  tags_data->timestamp);
		break;

	case 474:
		EMIT_SIGNAL_TIMESTAMP (XP_TE_BANNED, sess, word[4], NULL, NULL, NULL, 0,
									  tags_data->timestamp);
		break;

	case 475:
		EMIT_SIGNAL_TIMESTAMP (XP_TE_KEYWORD, sess, word[4], NULL, NULL, NULL, 0,
									  tags_data->timestamp);
		break;

	case 601:
		notify_set_offline (serv, word[4], FALSE, tags_data);
		break;

	case 605:
		notify_set_offline (serv, word[4], TRUE, tags_data);
		break;

	case 600:
	case 604:
		notify_set_online (serv, word[4], tags_data);
		break;

	case 524: // ERR_HELPNOTFOUND
	case 704: // RPL_HELPSTART
	case 705: // RPL_HELPTXT
	case 706: // RPL_ENDOFHELP
		EMIT_SIGNAL_TIMESTAMP (XP_TE_SERVTEXT, sess, STRIP_COLON(word, word_eol, 5), NULL, NULL, NULL,
									  0, tags_data->timestamp);
		break;

	case 728:	/* +q-list entry */
		/* NOTE:  FREENODE returns these results inconsistent with e.g. +b */
		/* Who else has imlemented MODE_QUIET, I wonder? */
		if (!inbound_banlist (sess, atol (word[8]), word[4], word[6], word[7], 728,
									 tags_data))
			goto def;
		break;

	case 729:	/* end of quiet list */
		if (!fe_ban_list_end (sess, 729))
			goto def;
		break;

	case 730: /* RPL_MONONLINE */
		notify_set_online_list (serv, word[4] + 1, tags_data);
		break;

	case 731: /* RPL_MONOFFLINE */
		notify_set_offline_list (serv, word[4] + 1, FALSE, tags_data);
		break;

	case 900:	/* successful SASL 'logged in as ' */
		EMIT_SIGNAL_TIMESTAMP (XP_TE_SERVTEXT, serv->server_session, 
									  word_eol[6]+1, word[1], word[2], NULL, 0,
									  tags_data->timestamp);
		break;
	case 904:	/* failed SASL auth */
		inbound_sasl_error (serv);
	case 903:	/* successful SASL auth */
	case 905:	/* failed SASL auth */
	case 906:	/* aborted */
	case 907:	/* attempting to re-auth after a successful auth */
		EMIT_SIGNAL_TIMESTAMP (XP_TE_SASLRESPONSE, serv->server_session, word[1],
									  word[2], word[3], ++word_eol[4], 0,
									  tags_data->timestamp);
		serv->waiting_on_sasl = FALSE;
		if (!serv->sent_capend)
		{
			serv->sent_capend = TRUE;
			tcp_send_len (serv, "CAP END\r\n", 9);
		}
		break;
	case 908:	/* Supported SASL Mechs */
		/* ignored for now, SASL 3.2 is a better solution and we only do PLAIN atm */
		break;

	default:

		if (serv->inside_whois && word[4][0])
		{
			/* some unknown WHOIS reply, ircd coders make them up weekly */
			if (!serv->skip_next_whois)
				EMIT_SIGNAL_TIMESTAMP (XP_TE_WHOIS_SPECIAL, whois_sess, word[4],
											  (word_eol[5][0] == ':') ? word_eol[5] + 1 : word_eol[5],
											  word[2], NULL, 0, tags_data->timestamp);
			return;
		}

	def:
		{
			session *sess;
		
			if (is_channel (serv, word[4]))
			{
				sess = find_channel (serv, word[4]);
				if (!sess)
					sess = serv->server_session;
			}
			else if ((sess=find_dialog (serv,word[4]))) /* user with an open dialog */
				;
			else
				sess=serv->server_session;
			
			EMIT_SIGNAL_TIMESTAMP (XP_TE_SERVTEXT, sess, text, word[1], word[2],
										  NULL, 0, tags_data->timestamp);
		}
	}
}

/* handle named messages that starts with a ':' */

static void
process_named_msg (session *sess, char *type, char *word[], char *word_eol[],
						 const message_tags_data *tags_data)
{
	server *serv = sess->server;
	char *account;
	char ip[128], nick[NICKLEN];
	char *text, *ex;
	int len = strlen (type);

	/* fill in the "ip" and "nick" buffers */
	ex = strchr (word[1], '!');
	if (!ex)							  /* no '!', must be a server message */
	{
		safe_strcpy (ip, word[1], sizeof (ip));
		safe_strcpy (nick, word[1], sizeof (nick));
	} else
	{
		safe_strcpy (ip, ex + 1, sizeof (ip));
		ex[0] = 0;
		safe_strcpy (nick, word[1], sizeof (nick));
		ex[0] = '!';
	}


	/** Update the account for this message's source. */
	if (serv->have_account_tag)
	{
		account = tags_data->account && *tags_data->account ? tags_data->account : "*";
		inbound_account (serv, nick, account, tags_data);
	}

	if (len == 4)
	{
		guint32 t;

		t = WORDL((guint8)type[0], (guint8)type[1], (guint8)type[2], (guint8)type[3]); 	
		/* this should compile to a bunch of: CMP.L, JE ... nice & fast */
		switch (t)
		{
		case WORDL('J','O','I','N'):
			{
				char *chan = word[3];
				char *account = word[4];
				char *realname = word_eol[5];

				if (account && strcmp (account, "*") == 0)
					account = NULL;
				if (realname && *realname == ':')
					realname++;
				if (*chan == ':')
					chan++;
				if (!serv->p_cmp (nick, serv->nick))
					inbound_ujoin (serv, chan, nick, ip, tags_data);
				else
					inbound_join (serv, chan, nick, ip, account, realname,
									  tags_data);
			}
			return;

		case WORDL('K','I','C','K'):
			{
				char *kicked = word[4];
				char *reason = word_eol[5];
				if (*kicked)
				{
					if (*reason == ':')
						reason++;
					if (!strcmp (kicked, serv->nick))
	 					inbound_ukick (serv, word[3], nick, reason, tags_data);
					else
						inbound_kick (serv, word[3], kicked, nick, reason, tags_data);
				}
			}
			return;

		case WORDL('K','I','L','L'):
			{
				char *reason = word_eol[4];
				if (*reason == ':')
					reason++;

				EMIT_SIGNAL_TIMESTAMP (XP_TE_KILL, sess, nick, reason, NULL, NULL,
											  0, tags_data->timestamp);
			}
			return;

		case WORDL('M','O','D','E'):
			handle_mode (serv, word, word_eol, nick, FALSE, tags_data);	/* modes.c */
			return;

		case WORDL('N','I','C','K'):
			inbound_newnick (serv, nick, 
								  (word_eol[3][0] == ':') ? word_eol[3] + 1 : word_eol[3],
								  FALSE, tags_data);
			return;

		case WORDL('P','A','R','T'):
			{
				char *chan = word[3];
				char *reason = word_eol[4];

				if (*chan == ':')
					chan++;
				if (*reason == ':')
					reason++;
				if (!strcmp (nick, serv->nick))
					inbound_upart (serv, chan, ip, reason, tags_data);
				else
					inbound_part (serv, chan, nick, ip, reason, tags_data);
			}
			return;

		case WORDL('P', 'I', 'N', 'G'):
			tcp_sendf (sess->server, "PONG %s\r\n", word_eol[3]);
			return;

		case WORDL('P','O','N','G'):
			inbound_ping_reply (serv->server_session,
									  (word[4][0] == ':') ? word[4] + 1 : word[4],
									  word[3], tags_data);
			return;

		case WORDL('Q','U','I','T'):
			inbound_quit (serv, nick, ip,
							  (word_eol[3][0] == ':') ? word_eol[3] + 1 : word_eol[3],
							  tags_data);
			return;

		case WORDL('A','W','A','Y'):
			inbound_away_notify (serv, nick,
										(word_eol[3][0] == ':') ? word_eol[3] + 1 : NULL,
										tags_data);
			return;

		case WORDL('F','A','I','L'):
			text = STRIP_COLON(word, word_eol, trailing_index(word_eol));
			if (g_strcmp0(word[3], "*") == 0)
			{
				EMIT_SIGNAL_TIMESTAMP (XP_TE_FAIL, sess, word[4], text, NULL, NULL, NULL, tags_data->timestamp);
			} else
			{
				EMIT_SIGNAL_TIMESTAMP (XP_TE_FAILCMD, sess, word[3], word[4], text, NULL, NULL, tags_data->timestamp);
			}
			return;

		case WORDL('W','A','R','N'):
			text = STRIP_COLON(word, word_eol, trailing_index(word_eol));
			if (g_strcmp0(word[3], "*") == 0)
			{
				EMIT_SIGNAL_TIMESTAMP (XP_TE_WARN, sess, word[4], text, NULL, NULL, NULL, tags_data->timestamp);
			} else
			{
				EMIT_SIGNAL_TIMESTAMP (XP_TE_WARNCMD, sess, word[3], word[4], text, NULL, NULL, tags_data->timestamp);
			}
			return;

		case WORDL('N','O','T','E'):
			text = STRIP_COLON(word, word_eol, trailing_index(word_eol));
			if (g_strcmp0(word[3], "*") == 0)
			{
				EMIT_SIGNAL_TIMESTAMP (XP_TE_NOTE, sess, word[4], text, NULL, NULL, NULL, tags_data->timestamp);
			} else
			{
				EMIT_SIGNAL_TIMESTAMP (XP_TE_NOTECMD, sess, word[3], word[4], text, NULL, NULL, tags_data->timestamp);
			}
			return;
		}

		goto garbage;
	}

	else if (len >= 5)
	{
		guint32 t;

		t = WORDL((guint8)type[0], (guint8)type[1], (guint8)type[2], (guint8)type[3]); 	
		/* this should compile to a bunch of: CMP.L, JE ... nice & fast */
		switch (t)
		{

		case WORDL('A','C','C','O'):
			inbound_account (serv, nick, STRIP_COLON(word, word_eol, 3), tags_data);
			return;

		case WORDL('A', 'U', 'T', 'H'):
			inbound_sasl_authenticate (sess->server, word_eol[3]);
			return;

		case WORDL('C', 'H', 'G', 'H'):
			inbound_user_info (sess, NULL, word[3], STRIP_COLON(word, word_eol, 4), NULL, nick, NULL,
							   NULL, 0xff, tags_data);
			return;

		case WORDL('S', 'E', 'T', 'N'):
			inbound_user_info (sess, NULL, NULL, NULL, NULL, nick, STRIP_COLON(word, word_eol, 3),
							   NULL, 0xff, tags_data);
			return;

		case WORDL('I','N','V','I'):
			if (ignore_check (word[1], IG_INVI))
				return;

			text = STRIP_COLON(word, word_eol, 4);
			if (serv->p_cmp (word[3], serv->nick))
				EMIT_SIGNAL_TIMESTAMP (XP_TE_INVITEDOTHER, sess, text, nick,
											  word[3], serv->servername, 0,
											  tags_data->timestamp);
			else
				EMIT_SIGNAL_TIMESTAMP (XP_TE_INVITED, sess, text, nick,
											  serv->servername, NULL, 0,
											  tags_data->timestamp);
				
			return;

		case WORDL('N','O','T','I'):
			{
				text = word_eol[4];
				if (*text == ':')
				{
					text++;
				}

#ifdef USE_OPENSSL
				/* QuakeNet CHALLENGE upon our request */
				if (serv->loginmethod == LOGIN_CHALLENGEAUTH && !serv->p_cmp (word[1], CHALLENGEAUTH_FULLHOST)
				    && !strncmp (text, "CHALLENGE ", 10) && *serv->password)
				{
					char *response;
					ircnet *net = serv->network;
					char *user = net && net->user ? net->user : prefs.hex_irc_user_name;

					response = challengeauth_response (user, serv->password, word[5]);

					tcp_sendf (serv, "PRIVMSG %s :CHALLENGEAUTH %s %s %s\r\n",
						CHALLENGEAUTH_NICK,
						user,
						response,
						CHALLENGEAUTH_ALGO);

					g_free (response);
					return;									/* omit the CHALLENGE <hash> ALGOS message */
				}
#endif

				if (!ignore_check (word[1], IG_NOTI))
					inbound_notice (serv, word[3], nick, text, ip, tags_data->identified, tags_data);
			}
			return;

		case WORDL('P','R','I','V'):
			{
				char *to = word[3];
				int len;
				if (*to)
				{
					/* Handle limited channel messages, for now no special event */
					if (strchr (serv->chantypes, to[0]) == NULL
						&& strchr (serv->nick_prefixes, to[0]) != NULL)
						to++;
						
					text = word_eol[4];
					if (*text == ':')
						text++;

					len = strlen (text);
					if (text[0] == 1)	/* ctcp */
					{
						char *new_pdibuf = NULL;
						if (text[len - 1] == 1)
						{
							text[len - 1] = 0;
						}
						text++;
						if (g_ascii_strncasecmp (text, "ACTION", 6) != 0)
							flood_check (nick, ip, serv, sess, 0);
						if (g_ascii_strncasecmp (text, "DCC ", 4) == 0)
						{
							int i;
							char *new_word[PDIWORDS+1] = { NULL };
							char *new_word_eol[PDIWORDS+1] = { NULL };

							new_pdibuf = g_malloc (strlen (word_eol[6]) + 1);

							/* This is a bit ugly but we handle the contents of the DCC message containing
							 * "quoted paths for files" here which means reparsing the message with handle_quotes.
							 * We avoid reparsing the entire message to avoid corrupting the non DCC parts.
							 * Greater than PDIWORD length DCC messages will be truncated. */
							process_data_init (new_pdibuf, word_eol[6], new_word, new_word_eol, TRUE, FALSE);
							for (i = 6; i < PDIWORDS; ++i)
							{
								word[i] = new_word[i - 5];
								word_eol[i] = new_word_eol[i - 5];
							}
						}

						ctcp_handle (sess, to, nick, ip, text, word, word_eol, tags_data->identified,
										 tags_data);

						/* Note word will be invalid beyond this scope */
						g_free (new_pdibuf);
					} else
					{
						if (is_channel (serv, to))
						{
							if (ignore_check (word[1], IG_CHAN))
								return;
							inbound_chanmsg (serv, NULL, to, nick, text, FALSE, tags_data->identified,
												  tags_data);
						} else
						{
							if (ignore_check (word[1], IG_PRIV))
								return;
							inbound_privmsg (serv, nick, ip, text, tags_data->identified, tags_data);
						}
					}
				}
			}
			return;

		case WORDL('T','O','P','I'):
			inbound_topicnew (serv, nick, word[3],
									(word_eol[4][0] == ':') ? word_eol[4] + 1 : word_eol[4],
									tags_data);
			return;

		case WORDL('W','A','L','L'):
			text = word_eol[3];
			if (*text == ':')
				text++;
			EMIT_SIGNAL_TIMESTAMP (XP_TE_WALLOPS, sess, nick, text, NULL, NULL, 0,
										  tags_data->timestamp);
			return;
		}
	}

	else if (len == 3)
	{
		guint32 t;

		t = WORDL((guint8)type[0], (guint8)type[1], (guint8)type[2], (guint8)type[3]);
		switch (t)
		{
			case WORDL('C','A','P','\0'):
				if (strncasecmp (word[4], "ACK", 3) == 0)
				{
					inbound_cap_ack (serv, word[1], 
										  word[5][0] == ':' ? word_eol[5] + 1 : word_eol[5],
										  tags_data);
				}
				else if (strncasecmp (word[4], "LS", 2) == 0 || strncasecmp (word[4], "NEW", 3) == 0)
				{
					inbound_cap_ls (serv, word[1], 
										 word[5][0] == ':' ? word_eol[5] + 1 : word_eol[5],
										 tags_data);
				}
				else if (strncasecmp (word[4], "NAK", 3) == 0)
				{
					inbound_cap_nak (serv, word[5][0] == ':' ? word_eol[5] + 1 : word_eol[5], tags_data);
				}
				else if (strncasecmp (word[4], "LIST", 4) == 0)	
				{
					inbound_cap_list (serv, word[1], 
											word[5][0] == ':' ? word_eol[5] + 1 : word_eol[5],
											tags_data);
				}
				else if (strncasecmp (word[4], "DEL", 3) == 0)
				{
					inbound_cap_del (serv, word[1],
											word[5][0] == ':' ? word_eol[5] + 1 : word_eol[5],
											tags_data);
				}

				return;
		}
	}

garbage:
	/* unknown message */
	PrintTextTimeStampf (sess, tags_data->timestamp, "GARBAGE: %s\n", word_eol[1]);
}

/* handle named messages that DON'T start with a ':' */

static void
process_named_servermsg (session *sess, char *buf, char *rawname, char *word_eol[],
								 const message_tags_data *tags_data)
{
	sess = sess->server->server_session;

	if (!strncmp (buf, "PING ", 5))
	{
		tcp_sendf (sess->server, "PONG %s\r\n", buf + 5);
		return;
	}
	if (!strncmp (buf, "ERROR", 5))
	{
		EMIT_SIGNAL_TIMESTAMP (XP_TE_SERVERERROR, sess, buf + 7, NULL, NULL, NULL,
									  0, tags_data->timestamp);
		return;
	}
	if (!strncmp (buf, "NOTICE ", 7))
	{
		buf = word_eol[3];
		if (*buf == ':')
			buf++;
		EMIT_SIGNAL_TIMESTAMP (XP_TE_SERVNOTICE, sess, buf, 
									  sess->server->servername, NULL, NULL, 0,
									  tags_data->timestamp);
		return;
	}
	if (!strncmp (buf, "AUTHENTICATE", 12))
	{
		inbound_sasl_authenticate (sess->server, word_eol[2]);
		return;
	}

	EMIT_SIGNAL_TIMESTAMP (XP_TE_SERVTEXT, sess, buf, sess->server->servername,
								  rawname, NULL, 0, tags_data->timestamp);
}

/* Returns the timezone offset. This should be the same as the variable
 * "timezone" in time.h, but *BSD doesn't have it.
 */
static time_t
get_timezone (void)
{
	struct tm tm_utc, tm_local;
	time_t t, time_utc, time_local;

	time (&t);

	/* gmtime() and localtime() are thread-safe on windows.
	 * on other systems we should use {gmtime,localtime}_r().
	 */
#if WIN32
	tm_utc = *gmtime (&t);
	tm_local = *localtime (&t);
#else
	gmtime_r (&t, &tm_utc);
	localtime_r (&t, &tm_local);
#endif

	time_utc = mktime (&tm_utc);
	time_local = mktime (&tm_local);

	return time_utc - time_local;
}

/* Handle time-server tags.
 * 
 * Sets tags_data->timestamp to the correct time (in unix time). 
 * This received time is always in UTC.
 *
 * See http://ircv3.atheme.org/extensions/server-time-3.2
 */
static void
handle_message_tag_time (const char *time, message_tags_data *tags_data)
{
	/* The time format defined in the ircv3.2 specification is
	 *       YYYY-MM-DDThh:mm:ss.sssZ
	 * but znc simply sends a unix time (with 3 decimal places for miliseconds)
	 * so we might as well support both.
	 */
	if (!*time)
		return;
	
	if (time[strlen (time) - 1] == 'Z')
	{
		/* as defined in the specification */
		struct tm t;
		int z;

		/* we ignore the milisecond part */
		z = sscanf (time, "%d-%d-%dT%d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday,
					&t.tm_hour, &t.tm_min, &t.tm_sec);

		if (z != 6)
			return;

		t.tm_year -= 1900;
		t.tm_mon -= 1;
		t.tm_isdst = 0; /* day light saving time */

		tags_data->timestamp = mktime (&t);

		if (tags_data->timestamp < 0)
		{
			tags_data->timestamp = 0;
			return;
		}

		/* get rid of the local time (mktime() receives a local calendar time) */
		tags_data->timestamp -= get_timezone();
	}
	else
	{
		/* znc */
		long long int t;

		/* we ignore the milisecond part */
		if (
#if defined(__MINGW64__) || defined(__MINGW32__)
		__mingw_sscanf
#else
		sscanf
#endif
		(time, "%lld", &t) != 1)
			return;

		tags_data->timestamp = (time_t) t;
	}
}

/* Handle message tags.
 *
 * See http://ircv3.atheme.org/specification/message-tags-3.2 
 */
static void
handle_message_tags (server *serv, const char *tags_str,
							message_tags_data *tags_data)
{
	char **tags;
	int i;

	/* FIXME We might want to avoid the allocation overhead here since 
	 * this might be called for every message from the server.
	 */
	tags = g_strsplit (tags_str, ";", 0);

	for (i=0; tags[i]; i++)
	{
		char *key = tags[i];
		char *value = strchr (tags[i], '=');

		if (!value)
			continue;

		*value = '\0';
		value++;

		if (serv->have_account_tag && !strcmp (key, "account"))
			tags_data->account = g_strdup (value);

		if (serv->have_idmsg && strcmp (key, "solanum.chat/identified"))
			tags_data->identified = TRUE;

		if (serv->have_server_time && !strcmp (key, "time"))
			handle_message_tag_time (value, tags_data);
	}
	
	g_strfreev (tags);
}

/* irc_inline() - 1 single line received from serv */
static void
irc_inline (server *serv, char *buf, int len)
{
	session *sess, *tmp;
	char *type, *text;
	char *word[PDIWORDS+1];
	char *word_eol[PDIWORDS+1];
	char *pdibuf;
	message_tags_data tags_data = MESSAGE_TAGS_DATA_INIT;

	pdibuf = g_malloc (len + 1);

	sess = serv->front_session;

	/* Python relies on this */
	word[PDIWORDS] = NULL;
	word_eol[PDIWORDS] = NULL;

	if (*buf == '@')
	{
		char *tags = buf + 1; /* skip the '@' */
		char *sep = strchr (buf, ' ');

		if (!sep)
			goto xit;
		
		*sep = '\0';
		buf = sep + 1;

		handle_message_tags(serv, tags, &tags_data);
	}

	url_check_line (buf);

	/* split line into words and words_to_end_of_line */
	process_data_init (pdibuf, buf, word, word_eol, FALSE, FALSE);

	if (buf[0] == ':')
	{
		/* find a context for this message */
		if (is_channel (serv, word[3]))
		{
			tmp = find_channel (serv, word[3]);
			if (tmp)
				sess = tmp;
		}

		/* for server messages, the 2nd word is the "message type" */
		type = word[2];

		word[0] = type;
		word_eol[1] = buf;	/* keep the ":" for plugins */

		if (plugin_emit_server (sess, type, word, word_eol,
								tags_data.timestamp))
			goto xit;

		word[1]++;
		word_eol[1] = buf + 1;	/* but not for HexChat internally */

	} else
	{
		word[0] = type = word[1];

		if (plugin_emit_server (sess, type, word, word_eol,
								tags_data.timestamp))
			goto xit;
	}

	if (buf[0] != ':')
	{
		process_named_servermsg (sess, buf, word[0], word_eol, &tags_data);
		goto xit;
	}

	/* see if the second word is a numeric */
	if (isdigit ((unsigned char) word[2][0]))
	{
		text = word_eol[4];
		if (*text == ':')
			text++;

		process_numeric (sess, atoi (word[2]), word, word_eol, text, &tags_data);
	} else
	{
		process_named_msg (sess, type, word, word_eol, &tags_data);
	}

xit:
	message_tags_data_free (&tags_data);
	g_free (pdibuf);
}

void
message_tags_data_free (message_tags_data *tags_data)
{
	g_clear_pointer (&tags_data->account, g_free);
}

void
proto_fill_her_up (server *serv)
{
	serv->p_inline = irc_inline;
	serv->p_invite = irc_invite;
	serv->p_cycle = irc_cycle;
	serv->p_ctcp = irc_ctcp;
	serv->p_nctcp = irc_nctcp;
	serv->p_quit = irc_quit;
	serv->p_kick = irc_kick;
	serv->p_part = irc_part;
	serv->p_ns_identify = irc_ns_identify;
	serv->p_ns_ghost = irc_ns_ghost;
	serv->p_join = irc_join;
	serv->p_join_list = irc_join_list;
	serv->p_login = irc_login;
	serv->p_join_info = irc_join_info;
	serv->p_mode = irc_mode;
	serv->p_user_list = irc_user_list;
	serv->p_away_status = irc_away_status;
	/*serv->p_get_ip = irc_get_ip;*/
	serv->p_whois = irc_user_whois;
	serv->p_get_ip = irc_user_list;
	serv->p_get_ip_uh = irc_userhost;
	serv->p_set_back = irc_set_back;
	serv->p_set_away = irc_set_away;
	serv->p_message = irc_message;
	serv->p_action = irc_action;
	serv->p_notice = irc_notice;
	serv->p_topic = irc_topic;
	serv->p_list_channels = irc_list_channels;
	serv->p_change_nick = irc_change_nick;
	serv->p_names = irc_names;
	serv->p_ping = irc_ping;
	serv->p_raw = irc_raw;
	serv->p_cmp = rfc_casecmp;	/* can be changed by 005 in modes.c */
}
