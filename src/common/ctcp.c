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
#include <string.h>
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include "hexchat.h"
#include "cfgfiles.h"
#include "util.h"
#include "modes.h"
#include "outbound.h"
#include "ignore.h"
#include "inbound.h"
#include "dcc.h"
#include "text.h"
#include "ctcp.h"
#include "server.h"
#include "hexchatc.h"


static void
ctcp_reply (session *sess, char *nick, char *word[], char *word_eol[],
				char *conf)
{
	char tbuf[4096];	/* can receive 2048 from IRC, so this is enough */

	conf = strdup (conf);
	/* process %C %B etc */
	check_special_chars (conf, TRUE);
	auto_insert (tbuf, sizeof (tbuf), conf, word, word_eol, "", "", word_eol[5],
					 server_get_network (sess->server, TRUE), "", "", nick, "");
	free (conf);
	handle_command (sess, tbuf, FALSE);
}

static int
ctcp_check (session *sess, char *nick, char *word[], char *word_eol[],
				char *ctcp)
{
	int ret = 0;
	char *po;
	struct popup *pop;
	GSList *list = ctcp_list;

	po = strchr (ctcp, '\001');
	if (po)
		*po = 0;

	po = strchr (word_eol[5], '\001');
	if (po)
		*po = 0;

	while (list)
	{
		pop = (struct popup *) list->data;
		if (!g_ascii_strcasecmp (ctcp, pop->name))
		{
			ctcp_reply (sess, nick, word, word_eol, pop->cmd);
			ret = 1;
		}
		list = list->next;
	}
	return ret;
}

void
ctcp_handle (session *sess, char *to, char *nick, char *ip,
				 char *msg, char *word[], char *word_eol[], int id,
				 const message_tags_data *tags_data)
{
	char *po;
	session *chansess;
	server *serv = sess->server;
	char outbuf[1024];
	int ctcp_offset = 2;

	if (serv->have_idmsg && (word[4][1] == '+' || word[4][1] == '-') )
			ctcp_offset = 3;

	/* consider DCC to be different from other CTCPs */
	if (!g_ascii_strncasecmp (msg, "DCC", 3))
	{
		/* but still let CTCP replies override it */
		if (!ctcp_check (sess, nick, word, word_eol, word[4] + ctcp_offset))
		{
			if (!ignore_check (word[1], IG_DCC))
				handle_dcc (sess, nick, word, word_eol, tags_data);
		}
		return;
	}

	/* consider ACTION to be different from other CTCPs. Check
      ignore as if it was a PRIV/CHAN. */
	if (!g_ascii_strncasecmp (msg, "ACTION ", 7))
	{
		if (is_channel (serv, to))
		{
			/* treat a channel action as a CHAN */
			if (ignore_check (word[1], IG_CHAN))
				return;
		} else
		{
			/* treat a private action as a PRIV */
			if (ignore_check (word[1], IG_PRIV))
				return;
		}

		/* but still let CTCP replies override it */
		if (ctcp_check (sess, nick, word, word_eol, word[4] + ctcp_offset))
			goto generic;

		inbound_action (sess, to, nick, ip, msg + 7, FALSE, id, tags_data);
		return;
	}

	if (ignore_check (word[1], IG_CTCP))
		return;

	if (!g_ascii_strcasecmp (msg, "VERSION") && !prefs.hex_irc_hide_version)
	{
#ifdef WIN32
		snprintf (outbuf, sizeof (outbuf), "VERSION HexChat "PACKAGE_VERSION" [x%d] / %s",
					 get_cpu_arch (), get_sys_str (1));
#else
		snprintf (outbuf, sizeof (outbuf), "VERSION HexChat "PACKAGE_VERSION" / %s",
					 get_sys_str (1));
#endif
		serv->p_nctcp (serv, nick, outbuf);
	}

	if (!ctcp_check (sess, nick, word, word_eol, word[4] + ctcp_offset))
	{
		if (!g_ascii_strncasecmp (msg, "SOUND", 5))
		{
			po = strchr (word[5], '\001');
			if (po)
				po[0] = 0;

			if (is_channel (sess->server, to))
			{
				chansess = find_channel (sess->server, to);
				if (!chansess)
					chansess = sess;

				EMIT_SIGNAL_TIMESTAMP (XP_TE_CTCPSNDC, chansess, word[5],
											  nick, to, NULL, 0, tags_data->timestamp);
			} else
			{
				EMIT_SIGNAL_TIMESTAMP (XP_TE_CTCPSND, sess->server->front_session,
											  word[5], nick, NULL, NULL, 0,
											  tags_data->timestamp);
			}

			/* don't let IRCers specify path */
#ifdef WIN32
			if (strchr (word[5], '/') == NULL && strchr (word[5], '\\') == NULL)
#else
			if (strchr (word[5], '/') == NULL)
#endif
				sound_play (word[5], TRUE);
			return;
		}
	}

generic:
	po = strchr (msg, '\001');
	if (po)
		po[0] = 0;

	if (!is_channel (sess->server, to))
	{
		EMIT_SIGNAL_TIMESTAMP (XP_TE_CTCPGEN, sess->server->front_session, msg,
									  nick, NULL, NULL, 0, tags_data->timestamp);
	} else
	{
		chansess = find_channel (sess->server, to);
		if (!chansess)
			chansess = sess;
		EMIT_SIGNAL_TIMESTAMP (XP_TE_CTCPGENC, chansess, msg, nick, to, NULL, 0,
									  tags_data->timestamp);
	}
}
