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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/* IRC RFC1459(+commonly used extensions) protocol implementation */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#include "xchat.h"

#ifdef WIN32
#include "inet.h"	/* for gethostname() */
#endif

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
#include "xchatc.h"


static void
irc_login (server *serv, char *user, char *realname)
{
	char hostname[128];

	if (serv->password[0])
		tcp_sendf (serv, "PASS %s\r\n", serv->password);

	gethostname (hostname, sizeof (hostname) - 1);
	hostname[sizeof (hostname) - 1] = 0;
	if (hostname[0] == 0)
		strcpy (hostname, "0");

	tcp_sendf (serv,
				  "NICK %s\r\n"
				  "USER %s %s %s :%s\r\n",
				  serv->nick, user, hostname, serv->servername, realname);
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
irc_chan_mode (server *serv, char *channel, char *mode)
{
	tcp_sendf (serv, "MODE %s %s\r\n", channel, mode);
}

static void
irc_nick_mode (server *serv, char *nick, char *mode)
{
	tcp_sendf (serv, "MODE %s %s\r\n", nick, mode);
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
	if (topic[0])
		tcp_sendf (serv, "TOPIC %s :%s\r\n", channel, topic);
	else
		tcp_sendf (serv, "TOPIC %s\r\n", channel);
}

static void
irc_list_channels (server *serv, char *arg)
{
	if (arg[0])
	{
		tcp_sendf (serv, "LIST %s\r\n", arg);
		return;
	}

	if (serv->use_listargs)
								/* 1234567890123456 */
		tcp_send_len (serv, "LIST >0,<10000\r\n", 16);
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
			len = snprintf (tbuf, sizeof (tbuf), "%s\r\n", raw);
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
channel_date (session *sess, char *chan, char *timestr)
{
	long n = atol (timestr);
	char *tim = ctime (&n);
	tim[19] = 0;	/* get rid of the \n */
	EMIT_SIGNAL (XP_TE_CHANDATE, sess, chan, tim, NULL, NULL, 0);
}

static void
process_numeric (session * sess, char *outbuf, int n,
					  char *word[], char *word_eol[], char *text)
{
	session *realsess;
	server *serv = sess->server;

	switch (n)
	{
	case 1:
		inbound_login_start (sess, word[3], word[1]);
		goto def;

	case 4:	/* check the ircd type */
		serv->use_listargs = FALSE;
		serv->modes_per_line = 3;		/* default to IRC RFC */
		if (strncmp (word[5], "bahamut", 7) == 0)				/* DALNet */
		{
			serv->use_listargs = TRUE;		/* use the /list args */
		} else if (strncmp (word[5], "u2.10.", 6) == 0)		/* Undernet */
		{
			serv->use_listargs = TRUE;		/* use the /list args */
			serv->modes_per_line = 6;		/* allow 6 modes per line */
		}
		goto def;

	case 5:
		inbound_005 (serv, word);
		goto def;

	case 301:
		inbound_away (serv, word[4],
						(word_eol[5][0] == ':') ? word_eol[5] + 1 : word_eol[5]);
		break;

	case 303:
		word[4]++;
		notify_markonline (serv, word);
		break;

	case 305:
		inbound_uback (serv);
		goto def;

	case 306:
		inbound_uaway (serv);
		goto def;

	case 312:
		EMIT_SIGNAL (XP_TE_WHOIS3, sess, word[4], word_eol[5], NULL, NULL, 0);
		break;

	case 311:
		serv->inside_whois = 1;
		/* FALL THROUGH */

	case 314:
		EMIT_SIGNAL (XP_TE_WHOIS1, sess, word[4], word[5],
						 word[6], word_eol[8] + 1, 0);
		break;

	case 317:
		{
			long n = atol (word[6]);
			long idle = atol (word[5]);
			char *tim;
			sprintf (outbuf, "%02ld:%02ld:%02ld", idle / 3600, (idle / 60) % 60,
						idle % 60);
			if (n == 0)
				EMIT_SIGNAL (XP_TE_WHOIS4, serv->front_session,
								 word[4], outbuf, NULL, NULL, 0);
			else
			{
				tim = ctime (&n);
				tim[19] = 0; 	/* get rid of the \n */
				EMIT_SIGNAL (XP_TE_WHOIS4T, serv->front_session,
								 word[4], outbuf, tim, NULL, 0);
			}
		}
		break;

	case 318:
		serv->inside_whois = 0;
		EMIT_SIGNAL (XP_TE_WHOIS6, serv->front_session, word[4], NULL,
						 NULL, NULL, 0);
		break;

	case 313:
	case 319:
		EMIT_SIGNAL (XP_TE_WHOIS2, serv->front_session, word[4],
						 word_eol[5] + 1, NULL, NULL, 0);
		break;

	case 321:
		if (!fe_is_chanwindow (sess->server))
			EMIT_SIGNAL (XP_TE_CHANLISTHEAD, sess, NULL, NULL, NULL, NULL, 0);
		break;

	case 322:
		if (fe_is_chanwindow (sess->server))
		{
			fe_add_chan_list (sess->server, word[4], word[5], word_eol[6] + 1);
		} else
		{
			sprintf (outbuf, "%-16.16s %-7d %s\017\n",
						word[4], atoi (word[5]), word_eol[6] + 1);
			PrintText (sess, outbuf);
		}
		break;

	case 323:
		if (!fe_is_chanwindow (sess->server))
			goto def;
		fe_chan_list_end (sess->server);
		break;

	case 324:
		sess = find_channel (serv, word[4]);
		if (!sess)
			sess = serv->server_session;
		if (sess->ignore_mode)
			sess->ignore_mode = FALSE;
		else
			EMIT_SIGNAL (XP_TE_CHANMODES, sess, word[4], word_eol[5],
							 NULL, NULL, 0);
		fe_update_mode_buttons (sess, 't', '-');
		fe_update_mode_buttons (sess, 'n', '-');
		fe_update_mode_buttons (sess, 's', '-');
		fe_update_mode_buttons (sess, 'i', '-');
		fe_update_mode_buttons (sess, 'p', '-');
		fe_update_mode_buttons (sess, 'm', '-');
		fe_update_mode_buttons (sess, 'l', '-');
		fe_update_mode_buttons (sess, 'k', '-');
		handle_mode (serv, outbuf, word, word_eol, "", TRUE);
		break;

	case 329:
		sess = find_channel (serv, word[4]);
		if (sess)
		{
			if (sess->ignore_date)
				sess->ignore_date = FALSE;
			else
				channel_date (sess, word[4], word[5]);
		}
		break;

	case 332:
		inbound_topic (serv, word[4],
						(word_eol[5][0] == ':') ? word_eol[5] + 1 : word_eol[5]);
		break;

	case 333:
		inbound_topictime (serv, word[4], word[5], atol (word[6]));
		break;

	case 341:						  /* INVITE ACK */
		EMIT_SIGNAL (XP_TE_UINVITE, sess, word[4], word[5], serv->servername,
						 NULL, 0);
		break;

	case 352:						  /* WHO */
		if (!serv->skip_next_who)
		{
			if (!inbound_user_info (sess, outbuf, word[4], word[5], word[6],
											word[7], word[8], word_eol[11]))
				goto def;
		} else
		{
			if (!serv->p_cmp (word[8], serv->nick))
				inbound_foundip (sess, word[6]);
		}
		break;

	case 315:						  /* END OF WHO */
		if (serv->skip_next_who)
		{
			serv->skip_next_who = FALSE;
		} else
		{
			session *who_sess;
			who_sess = find_channel (serv, word[4]);
			if (who_sess)
			{
				if (!who_sess->doing_who)
					goto def;
				who_sess->doing_who = FALSE;
			} else
			{
				if (!serv->doing_who)
					goto def;
				serv->doing_who = FALSE;
			}
		}
		break;

	case 353:						  /* NAMES */
		inbound_nameslist (serv, word[5],
							(word_eol[6][0] == ':') ? word_eol[6] + 1 : word_eol[6]);
		break;

	case 366:
		if (!inbound_nameslist_end (serv, word[4]))
			goto def;
		break;

	case 367: /* banlist entry */
		inbound_banlist (sess, atol (word[7]), word[4], word[5], word[6]);
		break;

	case 368:
		sess = find_channel (serv, word[4]);
		if (!sess)
			sess = serv->front_session;
		if (!fe_is_banwindow (sess))
			goto def;
		fe_ban_list_end (sess);
		break;

	case 372:	/* motd text */
	case 375:	/* motd start */
		if (!prefs.skipmotd || serv->motd_skipped)
			EMIT_SIGNAL (XP_TE_MOTD, serv->server_session, text, NULL, NULL,
							 NULL, 0);
		break;

	case 376:	/* end of motd */
	case 422:	/* motd file is missing */
		inbound_login_end (sess, text);
		break;

	case 433:
		if (serv->end_of_motd)
			goto def;
		inbound_next_nick (sess,  word[4]);
		break;

	case 437:
		if (serv->end_of_motd || is_channel (serv, word[4]))
			goto def;
		inbound_next_nick (sess, word[4]);
		break;

	case 471:
		EMIT_SIGNAL (XP_TE_USERLIMIT, sess, word[4], NULL, NULL, NULL, 0);
		break;

	case 473:
		EMIT_SIGNAL (XP_TE_INVITE, sess, word[4], NULL, NULL, NULL, 0);
		break;

	case 474:
		EMIT_SIGNAL (XP_TE_BANNED, sess, word[4], NULL, NULL, NULL, 0);
		break;

	case 475:
		EMIT_SIGNAL (XP_TE_KEYWORD, sess, word[4], NULL, NULL, NULL, 0);
		break;

	case 601:
		notify_set_offline (serv, word[4], FALSE);
		break;

	case 605:
		notify_set_offline (serv, word[4], TRUE);
		break;

	case 600:
	case 604:
		notify_set_online (serv, word[4]);
		break;

	default:
	def:
		if (is_channel (serv, word[4]))
		{
			realsess = find_channel (serv, word[4]);
			if (!realsess)
				realsess = sess;
			EMIT_SIGNAL (XP_TE_SERVTEXT, realsess, text, word[1], NULL, NULL, 0);
		} else
		{
			EMIT_SIGNAL (XP_TE_SERVTEXT, serv->server_session, text, word[1],
							 NULL, NULL, 0);
		}
	}
}

/* handle named messages that starts with a ':' */

static void
process_named_msg (session *sess, char *type, char *outbuf,
					    char *word[], char *word_eol[])
{
	server *serv = sess->server;
	char ip[128], nick[NICKLEN];
	char *text, *ex;

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

	if (!strcmp ("INVITE", type))
	{
		if (ignore_check (word[1], IG_INVI))
			return;

		EMIT_SIGNAL (XP_TE_INVITED, sess, word[4] + 1, nick, serv->servername,
						 NULL, 0);
		return;
	}
	if (!strcmp ("JOIN", type))
	{
		char *chan = word[3];

		if (*chan == ':')
			chan++;
		if (!serv->p_cmp (nick, serv->nick))
			inbound_ujoin (serv, chan, nick, ip);
		else
			inbound_join (serv, chan, nick, ip);
		return;
	}
	if (!strcmp ("MODE", type))
	{
		handle_mode (serv, outbuf, word, word_eol, nick, FALSE);	/* modes.c */
		return;
	}
	if (!strcmp ("NICK", type))
	{
		inbound_newnick (serv, nick, (word_eol[3][0] == ':')
								? word_eol[3] + 1 : word_eol[3], FALSE);
		return;
	}
	if (!strcmp ("NOTICE", type))
	{
		if (!ignore_check (word[1], IG_NOTI))
			inbound_notice (serv, outbuf, word[3], nick,
					(word_eol[4][0] == ':') ? word_eol[4] + 1 : word_eol[4], ip);
		return;
	}
	if (!strcmp ("PART", type))
	{
		char *chan = word[3];
		char *reason = word_eol[4];

		if (*chan == ':')
			chan++;
		if (*reason == ':')
			reason++;
		if (!strcmp (nick, serv->nick))
			inbound_upart (serv, chan, ip, reason);
		else
			inbound_part (serv, chan, nick, ip, reason);
		return;
	}
	if (!strcmp ("PRIVMSG", type))
	{
		char *to = word[3];
		int len;
		if (*to)
		{
			text = word_eol[4];
			if (*text == ':')
				text++;
			len = strlen (text);
			if (text[0] == 1 && text[len - 1] == 1)	/* ctcp */
			{
				text[len - 1] = 0;
				text++;
				if (strncasecmp (text, "ACTION", 6) != 0)
					flood_check (nick, ip, serv, sess, 0);
				if (ignore_check (word[1], IG_CTCP))
					return;
				if (strncasecmp (text, "DCC ", 4) == 0)
					/* redo this with handle_quotes TRUE */
					process_data_init (word[1], word_eol[1], word, word_eol, TRUE);
				ctcp_handle (sess, outbuf, to, nick, text, word, word_eol);
			} else
			{
				if (is_channel (serv, to))
				{
					if (ignore_check (word[1], IG_CHAN))
						return;
					inbound_chanmsg (serv, outbuf, to, nick, text, FALSE);
				} else
				{
					if (ignore_check (word[1], IG_PRIV))
						return;
					inbound_privmsg (serv, outbuf, nick, ip, text);
				}
			}
		}
		return;
	}
	if (!strcmp ("PONG", type))
	{
		inbound_ping_reply (sess, outbuf,
							 (word[4][0] == ':') ? word[4] + 1 : word[4], word[3]);
		return;
	}
	if (!strcmp ("QUIT", type))
	{
		inbound_quit (serv, nick, ip,
			(word_eol[3][0] == ':') ? word_eol[3] + 1 : word_eol[3]);
		return;
	}
	if (!strcmp ("TOPIC", type))
	{
		inbound_topicnew (serv, nick, word[3],
			(word_eol[4][0] == ':') ? word_eol[4] + 1 : word_eol[4]);
		return;
	}
	if (!strcmp ("KICK", type))
	{
		char *kicked = word[4];
		char *reason = word_eol[5];
		if (*kicked)
		{
			if (*reason == ':')
				reason++;
			if (!strcmp (kicked, serv->nick))
 				inbound_ukick (serv, word[3], nick, reason);
			else
				inbound_kick (serv, word[3], kicked, nick, reason);
		}
		return;
	}
	if (!strcmp ("KILL", type))
	{
		EMIT_SIGNAL (XP_TE_KILL, sess, nick, word_eol[5], NULL, NULL, 0);
		return;
	}
	if (!strcmp ("WALLOPS", type))
	{
		text = word_eol[3];
		if (*text == ':')
			text++;
		EMIT_SIGNAL (XP_TE_WALLOPS, sess, nick, text, NULL, NULL, 0);
		return;
	}

	/* unknown message */
	sprintf (outbuf, "GARBAGE: %s\n", word_eol[1]);
	PrintText (sess, outbuf);
}

/* handle named messages that DON'T start with a ':' */

static void
process_named_servermsg (session *sess, char *buf, char *word_eol[])
{
	if (!strncmp (buf, "PING ", 5))
	{
		tcp_sendf (sess->server, "PONG %s\r\n", buf + 5);
		return;
	}
	if (!strncmp (buf, "ERROR", 5))
	{
		EMIT_SIGNAL (XP_TE_SERVERERROR, sess, buf + 7, NULL, NULL, NULL, 0);
		return;
	}
	if (!strncmp (buf, "NOTICE ", 7))
	{
		buf = word_eol[3];
		if (*buf == ':')
			buf++;
		EMIT_SIGNAL (XP_TE_SERVNOTICE, sess, buf, sess->server->servername, NULL, NULL, 0);
		return;
	}
	EMIT_SIGNAL (XP_TE_SERVTEXT, sess, buf, sess->server->servername, NULL, NULL, 0);
}

/* irc_inline() - 1 single line received from serv */

static void
irc_inline (server *serv, char *buf)
{
	session *sess, *tmp;
	char *type, *text;
	char *word[PDIWORDS];
	char *word_eol[PDIWORDS];
	char outbuf[4096];
	char pdibuf[522];				  /* 1 line can't exceed 512 bytes!! */

	sess = serv->front_session;

	if (buf[0] == ':')
	{
		/* split line into words and words_to_end_of_line */
		process_data_init (pdibuf, buf, word, word_eol, FALSE);

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
		if (plugin_emit_server (sess, type, word, word_eol))
			return;
		word[1]++;
		word_eol[1] = buf + 1;	/* but not for xchat internally */

	} else
	{
		process_data_init (pdibuf, buf, word, word_eol, FALSE);
		word[0] = type = word[1];
		if (plugin_emit_server (sess, type, word, word_eol))
			return;
	}

	if (buf[0] != ':')
	{
		process_named_servermsg (sess, buf, word_eol);
		return;
	}

	/* see if the second word is a numeric */
	if (isdigit (word[2][0]))
	{
		text = word_eol[4];
		if (*text == ':')
				text++;

		process_numeric (sess, outbuf, atoi (word[2]), word, word_eol, text);
		return;
	}

	process_named_msg (sess, type, outbuf, word, word_eol);	
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
	serv->p_join = irc_join;
	serv->p_login = irc_login;
	serv->p_join_info = irc_join_info;
	serv->p_chan_mode = irc_chan_mode;
	serv->p_nick_mode = irc_nick_mode;
	serv->p_user_list = irc_user_list;
	/*serv->p_get_ip = irc_get_ip;*/
	serv->p_whois = irc_user_whois;
	serv->p_get_ip = irc_user_list;
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
