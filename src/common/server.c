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
 *
 * MS Proxy (ISA server) support is (c) 2006 Pavel Fedin <sonic_amiga@rambler.ru>
 * based on Dante source code
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006
 *      Inferno Nettverk A/S, Norway.  All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include "hexchat.h"
#include "fe.h"
#include "cfgfiles.h"
#include "network.h"
#include "notify.h"
#include "hexchatc.h"
#include "inbound.h"
#include "outbound.h"
#include "text.h"
#include "util.h"
#include "url.h"
#include "proto-irc.h"
#include "servlist.h"
#include "server.h"


static GSList *away_list = NULL;
GSList *serv_list = NULL;

static void auto_reconnect (server *serv, int send_quit, GError *err);
static void server_disconnect (session * sess, int sendquit, GError *err);
static int server_cleanup (server * serv);
static void server_connect (server *serv, char *hostname, int port, int no_login);

static int
server_send_real (server *serv, char *buf, int len)
{
	gsize buf_encoded_len;
	gchar *buf_encoded;

	buf_encoded = text_convert_invalid (buf, len, serv->write_converter, arbitrary_encoding_fallback_string, &buf_encoded_len);

	fe_add_rawlog (serv, buf_encoded, buf_encoded_len, TRUE);
	url_check_line (buf_encoded);

	// TODO: Error and cancellable
	return g_socket_send (serv->socket, buf_encoded, buf_encoded_len, NULL, NULL);
}

/* new throttling system, uses the same method as the Undernet
   ircu2.10 server; under test, a 200-line paste didn't flood
   off the client */

static int
tcp_send_queue (server *serv)
{
	char *buf, *p;
	int len, i, pri;
	GSList *list;
	time_t now = time (0);

	/* did the server close since the timeout was added? */
	if (!is_server (serv))
		return 0;

	/* try priority 2,1,0 */
	pri = 2;
	while (pri >= 0)
	{
		list = serv->outbound_queue;
		while (list)
		{
			buf = (char *) list->data;
			if (buf[0] == pri)
			{
				buf++;	/* skip the priority byte */
				len = strlen (buf);

				if (serv->next_send < now)
					serv->next_send = now;
				if (serv->next_send - now >= 10)
				{
					/* check for clock skew */
					if (now >= serv->prev_now)
						return 1;		  /* don't remove the timeout handler */
					/* it is skewed, reset to something sane */
					serv->next_send = now;
				}

				for (p = buf, i = len; i && *p != ' '; p++, i--);
				serv->next_send += (2 + i / 120);
				serv->sendq_len -= len;
				serv->prev_now = now;
				fe_set_throttle (serv);

				server_send_real (serv, buf, len);

				buf--;
				serv->outbound_queue = g_slist_remove (serv->outbound_queue, buf);
				g_free (buf);
				list = serv->outbound_queue;
			} else
			{
				list = list->next;
			}
		}
		/* now try pri 0 */
		pri--;
	}
	return 0;						  /* remove the timeout handler */
}

int
tcp_send_len (server *serv, char *buf, int len)
{
	char *dbuf;
	int noqueue = !serv->outbound_queue;

	if (!prefs.hex_net_throttle)
		return server_send_real (serv, buf, len);

	dbuf = g_malloc (len + 2);	/* first byte is the priority */
	dbuf[0] = 2;	/* pri 2 for most things */
	memcpy (dbuf + 1, buf, len);
	dbuf[len + 1] = 0;

	/* privmsg and notice get a lower priority */
	if (g_ascii_strncasecmp (dbuf + 1, "PRIVMSG", 7) == 0 ||
		 g_ascii_strncasecmp (dbuf + 1, "NOTICE", 6) == 0)
	{
		dbuf[0] = 1;
	}
	else
	{
		/* WHO gets the lowest priority */
		if (g_ascii_strncasecmp (dbuf + 1, "WHO ", 4) == 0)
			dbuf[0] = 0;
		/* as do MODE queries (but not changes) */
		else if (g_ascii_strncasecmp (dbuf + 1, "MODE ", 5) == 0)
		{
			char *mode_str, *mode_str_end, *loc;
			/* skip spaces before channel/nickname */
			for (mode_str = dbuf + 5; *mode_str == ' '; ++mode_str);
			/* skip over channel/nickname */
			mode_str = strchr (mode_str, ' ');
			if (mode_str)
			{
				/* skip spaces before mode string */
				for (; *mode_str == ' '; ++mode_str);
				/* find spaces after end of mode string */
				mode_str_end = strchr (mode_str, ' ');
				/* look for +/- within the mode string */
				loc = strchr (mode_str, '-');
				if (loc && (!mode_str_end || loc < mode_str_end))
					goto keep_priority;
				loc = strchr (mode_str, '+');
				if (loc && (!mode_str_end || loc < mode_str_end))
					goto keep_priority;
			}
			dbuf[0] = 0;
keep_priority:
			;
		}
	}

	serv->outbound_queue = g_slist_append (serv->outbound_queue, dbuf);
	serv->sendq_len += len; /* tcp_send_queue uses strlen */

	if (tcp_send_queue (serv) && noqueue)
		fe_timeout_add (500, tcp_send_queue, serv);

	return 1;
}

void
tcp_sendf (server *serv, const char *fmt, ...)
{
	va_list args;
	/* keep this buffer in BSS. Converting UTF-8 to ISO-8859-x might make the
      string shorter, so allow alot more than 512 for now. */
	static char send_buf[1540];	/* good code hey (no it's not overflowable) */
	int len;

	va_start (args, fmt);
	len = g_vsnprintf (send_buf, sizeof (send_buf) - 1, fmt, args);
	va_end (args);

	send_buf[sizeof (send_buf) - 1] = '\0';
	if (len < 0 || len > (sizeof (send_buf) - 1))
		len = strlen (send_buf);

	tcp_send_len (serv, send_buf, len);
}

/* handle 1 line of text received from the server */

static void
server_inline (server *serv, char *line, gssize len)
{
	gsize len_utf8;
	if (!strcmp (serv->encoding, "UTF-8"))
		line = text_fixup_invalid_utf8 (line, len, &len_utf8);
	else
		line = text_convert_invalid (line, len, serv->read_converter, unicode_fallback_string, &len_utf8);

	fe_add_rawlog (serv, line, len_utf8, FALSE);

	/* let proto-irc.c handle it */
	serv->p_inline (serv, line, len_utf8);

	g_free (line);
}

/* read data from socket */

static gboolean
on_socket_ready (GSocket *socket, GIOCondition condition, server *serv)
{
	GError *error = NULL;
	gssize len;
	int i;
	char lbuf[2050];

	while (1)
	{
		len = g_socket_receive (socket, lbuf, sizeof (lbuf) - 2, NULL, &error);
		if (error)
		{
			if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
			{
				/* Fall through */
			}
			else if (!serv->end_of_motd)
			{
				server_disconnect (serv->server_session, FALSE, error);
				if (!servlist_cycle (serv))
				{
					if (prefs.hex_net_auto_reconnect)
						auto_reconnect (serv, FALSE, error);
				}
			}
			else
			{
				if (prefs.hex_net_auto_reconnect)
					auto_reconnect (serv, FALSE, error);
				else
					server_disconnect (serv->server_session, FALSE, error);
			}

			g_error_free (error);
			return G_SOURCE_CONTINUE;
		}


		i = 0;
		lbuf[len] = 0;

		while (i < len)
		{
			// FIXME: Unsafe?
			switch (lbuf[i])
			{
			case '\r':
				break;

			case '\n':
				serv->linebuf[serv->pos] = 0;
				server_inline (serv, serv->linebuf, serv->pos);
				serv->pos = 0;
				break;

			default:
				serv->linebuf[serv->pos] = lbuf[i];
				if (serv->pos >= (sizeof (serv->linebuf) - 1))
					fprintf (stderr,
								"*** HEXCHAT WARNING: Buffer overflow - shit server!\n");
				else
					serv->pos++;
			}
			i++;
		}
	}
}

static void
server_connected (server * serv)
{
	prefs.wait_on_exit = TRUE;
	serv->ping_recv = time (0);
	serv->lag_sent = 0;
	serv->connected = TRUE;

	serv->socket_read_source = g_socket_create_source (serv->socket, G_IO_IN|G_IO_ERR, NULL);
	g_source_set_callback (serv->socket_read_source, (GSourceFunc)on_socket_ready, serv, NULL);
	g_source_attach (serv->socket_read_source, g_main_context_default ());
	g_source_unref (serv->socket_read_source);

	if (!serv->no_login)
	{
		EMIT_SIGNAL (XP_TE_CONNECTED, serv->server_session, NULL, NULL, NULL,
						 NULL, 0);
		if (serv->network)
		{
			serv->p_login (serv,
								(!(((ircnet *)serv->network)->flags & FLAG_USE_GLOBAL) &&
								 (((ircnet *)serv->network)->user)) ?
								(((ircnet *)serv->network)->user) :
								prefs.hex_irc_user_name,
								(!(((ircnet *)serv->network)->flags & FLAG_USE_GLOBAL) &&
								 (((ircnet *)serv->network)->real)) ?
								(((ircnet *)serv->network)->real) :
								prefs.hex_irc_real_name);
		} else
		{
			serv->p_login (serv, prefs.hex_irc_user_name, prefs.hex_irc_real_name);
		}
	} else
	{
		EMIT_SIGNAL (XP_TE_SERVERCONNECTED, serv->server_session, NULL, NULL,
						 NULL, NULL, 0);
	}

	server_set_name (serv, serv->servername);
	fe_server_event (serv, FE_SE_CONNECT, 0);
}

#ifdef WIN32

static gboolean
server_close_pipe (int *pipefd)	/* see comments below */
{
	close (pipefd[0]);	/* close WRITE end first to cause an EOF on READ */
	close (pipefd[1]);	/* in giowin32, and end that thread. */
	g_free (pipefd);
	return FALSE;
}

#endif

static void
server_stopconnecting (server * serv)
{
	g_assert (!serv->socket_read_source);

	if (serv->joindelay_tag)
	{
		fe_timeout_remove (serv->joindelay_tag);
		serv->joindelay_tag = 0;
	}

	if (serv->connection_cancellable)
		g_cancellable_cancel (serv->connection_cancellable);

	fe_progressbar_end (serv);

	serv->connecting = FALSE;
	fe_server_event (serv, FE_SE_DISCONNECT, 0);
}

static int
timeout_auto_reconnect (server *serv)
{
	if (is_server (serv))  /* make sure it hasnt been closed during the delay */
	{
		serv->recondelay_tag = 0;
		if (!serv->connected && !serv->connecting && serv->server_session)
		{
			server_connect (serv, serv->hostname, serv->port, FALSE);
		}
	}
	return 0;			  /* returning 0 should remove the timeout handler */
}

static void
auto_reconnect (server *serv, int send_quit, GError *error)
{
	session *s;
	int del;

	if (serv->server_session == NULL)
		return;

	if (prefs.hex_irc_reconnect_rejoin)
	{
		GSList *list;
		list = sess_list;
		while (list)				  /* make sure auto rejoin can work */
		{
			s = list->data;
			if (s->type == SESS_CHANNEL && s->channel[0])
			{
				strcpy (s->waitchannel, s->channel);
				strcpy (s->willjoinchannel, s->channel);
			}
			list = list->next;
		}
	}

	if (serv->connected)
		server_disconnect (serv->server_session, send_quit, error);

	del = prefs.hex_net_reconnect_delay * 1000;
	if (del < 1000)
		del = 500;				  /* so it doesn't block the gui */

	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CONNECTION_CLOSED) || g_error_matches (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT))
		serv->reconnect_away = serv->is_away;

	/* is this server in a reconnect delay? remove it! */
	if (serv->recondelay_tag)
	{
		fe_timeout_remove (serv->recondelay_tag);
		serv->recondelay_tag = 0;
	}

	serv->recondelay_tag = fe_timeout_add (del, timeout_auto_reconnect, serv);
	fe_server_event (serv, FE_SE_RECONDELAY, del);
}

static void
server_flush_queue (server *serv)
{
	list_free (&serv->outbound_queue);
	serv->sendq_len = 0;
	fe_set_throttle (serv);
}

/* connect() successed */

static void
server_connect_success (server *serv)
{
	server_stopconnecting (serv);	/* ->connecting = FALSE */
	/* activate glib poll */
	server_connected (serv);
}

/* receive info from the child-process about connection progress */

#if 0
static gboolean
server_read_child (GIOChannel *source, GIOCondition condition, server *serv)
{
	session *sess = serv->server_session;
	char tbuf[128];
	char outbuf[512];
	char host[100];
	char ip[100];

	waitline2 (source, tbuf, sizeof tbuf);

	switch (tbuf[0])
	{
	case '0':	/* print some text */
		waitline2 (source, tbuf, sizeof tbuf);
		PrintText (serv->server_session, tbuf);
		break;

	case '7':						  /* gethostbyname (prefs.hex_net_bind_host) failed */
		sprintf (outbuf,
					_("Cannot resolve hostname %s\nCheck your IP Settings!\n"),
					prefs.hex_net_bind_host);
		PrintText (sess, outbuf);
		break;
	case '8':
		PrintText (sess, _("Proxy traversal failed.\n"));
		server_disconnect (sess, FALSE, -1);
		break;
	}

	return TRUE;
}

#endif

/* kill all sockets & iotags of a server. Stop a connection attempt, or
   disconnect if already connected. */

static int
server_cleanup (server * serv)
{
	fe_set_lag (serv, 0);

	if (serv->socket_read_source)
	{
		g_clear_pointer (&serv->socket_read_source, g_source_destroy);
	}
	if (serv->joindelay_tag)
	{
		fe_timeout_remove (serv->joindelay_tag);
		serv->joindelay_tag = 0;
	}

	g_clear_object (&serv->socket_client);

	if (serv->connecting)
	{
		server_stopconnecting (serv);
		return 1;
	}

	if (serv->connected)
	{
		serv->connected = FALSE;
		serv->end_of_motd = FALSE;
		return 2;
	}

	/* is this server in a reconnect delay? remove it! */
	if (serv->recondelay_tag)
	{
		fe_timeout_remove (serv->recondelay_tag);
		serv->recondelay_tag = 0;
		return 3;
	}

	return 0;
}

static void
server_disconnect (session * sess, int sendquit, GError *err)
{
	server *serv = sess->server;
	GSList *list;
	gboolean shutup = FALSE;

	/* send our QUIT reason */
	if (sendquit && serv->connected)
	{
		server_sendquit (sess);
	}

	fe_server_event (serv, FE_SE_DISCONNECT, 0);

	/* close all sockets & io tags */
	switch (server_cleanup (serv))
	{
	case 0:							  /* it wasn't even connected! */
		notc_msg (sess);
		return;
	case 1:							  /* it was in the process of connecting */
		EMIT_SIGNAL (XP_TE_STOPCONNECT, sess, NULL, NULL, NULL, NULL, 0);
		return;
	case 3:
		shutup = TRUE;	/* won't print "disconnected" in channels */
	}

	server_flush_queue (serv);

	list = sess_list;
	while (list)
	{
		sess = (struct session *) list->data;
		if (sess->server == serv)
		{
			if (!shutup || sess->type == SESS_SERVER)
				/* print "Disconnected" to each window using this server */
				EMIT_SIGNAL (XP_TE_DISCON, sess, err ? err->message : NULL, NULL, NULL, NULL, 0);

			if (!sess->channel[0] || sess->type == SESS_CHANNEL)
				clear_channel (sess);
		}
		list = list->next;
	}

	serv->pos = 0;
	serv->motd_skipped = FALSE;
	serv->no_login = FALSE;
	serv->servername[0] = 0;
	serv->lag_sent = 0;

	notify_cleanup ();
}

/* send a "print text" command to the parent process - MUST END IN \n! */

/* this is the child process making the connection attempt */
#if 0

static int
server_child (server * serv)
{
	netstore *ns_server;
	netstore *ns_proxy = NULL;
	netstore *ns_local;
	int port = serv->port;
	int error;
	int sok, psok;
	char *hostname = serv->hostname;
	char *real_hostname = NULL;
	char *ip;
	char *proxy_ip = NULL;
	char *local_ip;
	int connect_port;
	char buf[512];
	char bound = 0;
	int proxy_type = 0;
	char *proxy_host = NULL;
	int proxy_port;

	ns_server = net_store_new ();

	if (!serv->dont_use_proxy) /* blocked in serverlist? */
	{
		if (prefs.hex_net_proxy_type == 5)
		{
			char **proxy_list;
			char *url, *proxy;
			GProxyResolver *resolver;
			GError *error = NULL;

			resolver = g_proxy_resolver_get_default ();
			url = g_strdup_printf ("irc://%s:%d", hostname, port);
			proxy_list = g_proxy_resolver_lookup (resolver, url, NULL, &error);

			if (proxy_list) {
				/* can use only one */
				proxy = proxy_list[0];
				if (!strncmp (proxy, "direct", 6))
					proxy_type = 0;
				else if (!strncmp (proxy, "http", 4))
					proxy_type = 4;
				else if (!strncmp (proxy, "socks5", 6))
					proxy_type = 3;
				else if (!strncmp (proxy, "socks", 5))
					proxy_type = 2;
			} else {
				write_error ("Failed to lookup proxy", &error);
			}

			if (proxy_type) {
				char *c;
				c = strchr (proxy, ':') + 3;
				proxy_host = g_strdup (c);
				c = strchr (proxy_host, ':');
				*c = '\0';
				proxy_port = atoi (c + 1);
			}

			g_strfreev (proxy_list);
			g_free (url);
		}

		if (prefs.hex_net_proxy_host[0] &&
			   prefs.hex_net_proxy_type > 0 &&
			   prefs.hex_net_proxy_use != 2) /* proxy is NOT dcc-only */
		{
			proxy_type = prefs.hex_net_proxy_type;
			proxy_host = g_strdup (prefs.hex_net_proxy_host);
			proxy_port = prefs.hex_net_proxy_port;
		}
	}


	serv->proxy_type = proxy_type;

	/* first resolve where we want to connect to */
	if (proxy_type > 0)
	{
		g_snprintf (buf, sizeof (buf), "9\n%s\n", proxy_host);
		write (serv->childwrite, buf, strlen (buf));
		ip = net_resolve (ns_server, proxy_host, proxy_port, &real_hostname);
		g_free (proxy_host);
		if (!ip)
		{
			write (serv->childwrite, "1\n", 2);
			goto xit;
		}
		connect_port = proxy_port;

		/* if using socks4 or MS Proxy, attempt to resolve ip for irc server */
		if ((proxy_type == 2) || (proxy_type == 5))
		{
			ns_proxy = net_store_new ();
			proxy_ip = net_resolve (ns_proxy, hostname, port, &real_hostname);
			if (!proxy_ip)
			{
				write (serv->childwrite, "1\n", 2);
				goto xit;
			}
		} else						  /* otherwise we can just use the hostname */
			proxy_ip = g_strdup (hostname);
	} else
	{
		ip = net_resolve (ns_server, hostname, port, &real_hostname);
		if (!ip)
		{
			write (serv->childwrite, "1\n", 2);
			goto xit;
		}
		connect_port = port;
	}

	g_snprintf (buf, sizeof (buf), "3\n%s\n%s\n%d\n",
				 real_hostname, ip, connect_port);
	write (serv->childwrite, buf, strlen (buf));

	if (!serv->dont_use_proxy && (proxy_type == 5))
		error = net_connect (ns_server, serv->proxy_sok4, serv->proxy_sok6, &psok);
	else
	{
		error = net_connect (ns_server, serv->sok4, serv->sok6, &sok);
		psok = sok;
	}

	if (error != 0)
	{
		g_snprintf (buf, sizeof (buf), "2\n%d\n", sock_error ());
		write (serv->childwrite, buf, strlen (buf));
	} else
	{
		/* connect succeeded */
		if (proxy_ip)
		{
			switch (traverse_proxy (proxy_type, serv->childwrite, psok, proxy_ip, port, ns_proxy, serv->sok4, serv->sok6, &sok, bound))
			{
			case 0:	/* success */
				g_snprintf (buf, sizeof (buf), "4\n%d\n", sok);	/* success */
				write (serv->childwrite, buf, strlen (buf));
				break;
			case 1:	/* socks traversal failed */
				write (serv->childwrite, "8\n", 2);
				break;
			}
		} else
		{
			g_snprintf (buf, sizeof (buf), "4\n%d\n", sok);	/* success */
			write (serv->childwrite, buf, strlen (buf));
		}
	}

xit:

	/* this is probably not needed */
	net_store_destroy (ns_server);
	if (ns_proxy)
		net_store_destroy (ns_proxy);

	/* no need to free ip/real_hostname, this process is exiting */
#ifdef WIN32
	/* under win32 we use a thread -> shared memory, must free! */
	g_free (proxy_ip);
	g_free (ip);
	g_free (real_hostname);
#endif

	return 0;
	/* cppcheck-suppress memleak */
}
#endif

static void
on_client_network_event (GSocketClient *client,
                         GSocketClientEvent event,
                         GSocketConnectable *connectable,
                         GIOStream *connection,
                         gpointer user_data)
{
	session *sess = user_data;

	// TODO: 		prefs.local_ip = inet_addr (tbuf);

	switch (event)
	{
	case G_SOCKET_CLIENT_RESOLVING:
		EMIT_SIGNAL (XP_TE_SERVERLOOKUP, sess, (char*)g_network_address_get_hostname (G_NETWORK_ADDRESS (connectable)),
		             NULL, NULL, NULL, 0);
		break;
	case G_SOCKET_CLIENT_RESOLVED:
		break;
	case G_SOCKET_CLIENT_CONNECTING: {
		GInetSocketAddress *remote_isaddr = G_INET_SOCKET_ADDRESS (g_socket_connection_get_remote_address (G_SOCKET_CONNECTION (connection), NULL));
		
		g_warn_if_fail (remote_isaddr != NULL);
		if (remote_isaddr)
		{
			GInetAddress *remote_iaddr = G_INET_ADDRESS (g_inet_socket_address_get_address (remote_isaddr));
			char *remote_iaddr_string = g_inet_address_to_string (remote_iaddr);
			char *hostname = (char*)g_network_address_get_hostname (G_NETWORK_ADDRESS (connectable));
			char port_str[16];
			g_snprintf (port_str, sizeof (port_str), "%"G_GUINT16_FORMAT, g_inet_socket_address_get_port (remote_isaddr));

			EMIT_SIGNAL (XP_TE_CONNECT, sess, hostname, remote_iaddr_string, port_str, NULL, 0);
			g_free (remote_iaddr_string);
			g_object_unref (remote_isaddr);
		}
		break;
	}
	case G_SOCKET_CLIENT_TLS_HANDSHAKED: {
		// TODO: Print out certificate information
		// EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
		// 			NULL, 0);
		break;
	}
	case G_SOCKET_CLIENT_CONNECTED: {
		char buf[512];
		ircnet *net = sess->server->network;
		GInetSocketAddress *remote_isaddr = G_INET_SOCKET_ADDRESS (g_socket_connection_get_remote_address (G_SOCKET_CONNECTION (connection), NULL));

		g_assert (remote_isaddr);
		
		g_snprintf (buf, sizeof (buf), "IDENTD %"G_GUINT16_FORMAT" ", g_inet_socket_address_get_port (remote_isaddr));
		if (net && net->user && !(net->flags & FLAG_USE_GLOBAL))
			g_strlcat (buf, net->user, sizeof (buf));
		else
			g_strlcat (buf, prefs.hex_irc_user_name, sizeof (buf));

		handle_command (sess->server->server_session, buf, FALSE);

		g_object_unref (remote_isaddr);
		break;
	}
	}
}

static void
on_client_connect_ready (GSocketClient *client, GAsyncResult *res, gpointer user_data)
{
	GSocketConnection *conn;
	GError *error = NULL;
	server *serv = user_data;

	conn = g_socket_client_connect_finish (client, res, &error);
	if (error)
	{
		g_debug ("Failed to connect: %s", error->message);
	
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
		{
			g_error_free (error);
			return;
		}

		server_stopconnecting (serv);
		if (g_error_matches (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND))
		{
			EMIT_SIGNAL (XP_TE_UKNHOST, serv->server_session, NULL, NULL, NULL, NULL, 0);
		}
		else if (error->domain == G_TLS_ERROR)
		{
			EMIT_SIGNAL (XP_TE_CONNFAIL, serv->server_session, error->message, NULL, NULL, NULL, 0);
		}
		else
		{
			EMIT_SIGNAL (XP_TE_CONNFAIL, serv->server_session, error->message, NULL, NULL, NULL, 0);
		}


		if (!servlist_cycle (serv))
			if (prefs.hex_net_auto_reconnectonfail)
				auto_reconnect (serv, FALSE, error);

		g_error_free (error);
		return;
	}

	serv->socket_conn = g_steal_pointer (&conn);
	serv->socket = g_socket_connection_get_socket (serv->socket_conn);
	g_socket_set_blocking (serv->socket, FALSE);

	server_connect_success (serv);
}

static void
server_connect (server *serv, char *hostname, int port, int no_login)
{
	session *sess = serv->server_session;
	GSocketConnectable *connectable;

	if (!hostname[0])
	{
		g_debug ("server_connect: no hostname");
		return;
	}

	if (port <= 0 || port > G_MAXUINT16)
		port = serv->use_ssl ? 6697 : 6667;

	connectable = g_network_address_new (hostname, port);
	
	serv->socket_client = g_socket_client_new ();
	g_socket_client_set_tls (serv->socket_client, serv->use_ssl);
	g_socket_client_set_enable_proxy (serv->socket_client, !serv->dont_use_proxy);
	g_socket_client_set_timeout (serv->socket_client, 60); // FIXME
	g_signal_connect (serv->socket_client, "event", G_CALLBACK (on_client_network_event), sess);

	if (prefs.hex_net_bind_host[0])
	{
		GSocketAddress *local_addr = g_inet_socket_address_new_from_string (prefs.hex_net_bind_host, 0);
		g_socket_client_set_local_address (serv->socket_client, local_addr);
		g_object_unref (local_addr);
	}

	
	if (!serv->dont_use_proxy)
	{
		//GProxyResolver *proxy_resolver = g_proxy_resolver_get_default ();
		// TODO:
	}

	// if (serv->connected || serv->connecting || serv->recondelay_tag)
	// 	server_disconnect (sess, TRUE, -1);

	fe_progressbar_start (sess);

	safe_strcpy (serv->servername, hostname, sizeof (serv->servername));
	/* overlap illegal in strncpy */
	if (hostname != serv->hostname)
		safe_strcpy (serv->hostname, hostname, sizeof (serv->hostname));

	if (serv->use_ssl)
	{
		char *certificate_paths[2];
		GError *error = NULL;
		serv->have_cert = FALSE;
		guint i;

		/* first try network specific cert/key */
		certificate_paths[0] = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "certs" G_DIR_SEPARATOR_S "%s.pem",
                                                get_xdir (), server_get_network (serv, TRUE));
		/* if that doesn't exist, try <config>/certs/client.pem */
        certificate_paths[1] = g_build_filename (get_xdir (), "certs", "client.pem", NULL);

		for (i = 0; i < G_N_ELEMENTS (certificate_paths); i++)
		{
			serv->client_cert = g_tls_certificate_new_from_file (certificate_paths[i], &error);
			if (error)
			{
				g_debug ("Failed opening client cert (%s): %s", certificate_paths[i], error->message);
				g_clear_error (&error);
				continue;
			}

			serv->have_cert = TRUE;
			break;
		}

		g_free (certificate_paths[0]);
		g_free (certificate_paths[1]);
	}

	server_set_defaults (serv);
	serv->connecting = TRUE;
	serv->port = port;
	serv->no_login = no_login;

	fe_server_event (serv, FE_SE_CONNECTING, 0);
	fe_set_away (serv);
	server_flush_queue (serv);

	g_socket_client_connect_async (serv->socket_client, connectable, NULL, (GAsyncReadyCallback)on_client_connect_ready, serv);
}

void
server_fill_her_up (server *serv)
{
	serv->connect = server_connect;
	serv->disconnect = server_disconnect;
	serv->cleanup = server_cleanup;
	serv->flush_queue = server_flush_queue;
	serv->auto_reconnect = auto_reconnect;

	proto_fill_her_up (serv);
}

void
server_set_encoding (server *serv, char *new_encoding)
{
	char *space;

	g_free (serv->encoding);

	if (new_encoding)
	{
		serv->encoding = g_strdup (new_encoding);
		/* the serverlist GUI might have added a space 
			and short description - remove it. */
		space = strchr (serv->encoding, ' ');
		if (space)
			space[0] = 0;

		/* Default legacy "IRC" encoding to utf-8. */
		if (g_ascii_strcasecmp (serv->encoding, "IRC") == 0)
		{
			g_free (serv->encoding);
			serv->encoding = g_strdup ("UTF-8");
		}

		else if (!servlist_check_encoding (serv->encoding))
		{
			g_free (serv->encoding);
			serv->encoding = g_strdup ("UTF-8");
		}
	}
	else
	{
		serv->encoding = g_strdup ("UTF-8");
	}

	if (serv->read_converter != NULL)
	{
		g_iconv_close (serv->read_converter);
	}
	serv->read_converter = g_iconv_open ("UTF-8", serv->encoding);

	if (serv->write_converter != NULL)
	{
		g_iconv_close (serv->write_converter);
	}
	serv->write_converter = g_iconv_open (serv->encoding, "UTF-8");
}

server *
server_new (void)
{
	static int id = 0;
	server *serv;

	serv = g_new0 (struct server, 1);

	/* use server.c and proto-irc.c functions */
	server_fill_her_up (serv);

	serv->id = id++;
	strcpy (serv->nick, prefs.hex_irc_nick1);
	server_set_defaults (serv);

	serv_list = g_slist_prepend (serv_list, serv);

	fe_new_server (serv);

	return serv;
}

int
is_server (server *serv)
{
	return g_slist_find (serv_list, serv) ? 1 : 0;
}

void
server_set_defaults (server *serv)
{
	g_free (serv->chantypes);
	g_free (serv->chanmodes);
	g_free (serv->nick_prefixes);
	g_free (serv->nick_modes);

	serv->chantypes = g_strdup ("#&!+");
	serv->chanmodes = g_strdup ("beI,k,l");
	serv->nick_prefixes = g_strdup ("@%+");
	serv->nick_modes = g_strdup ("ohv");
	serv->sasl_mech = MECH_PLAIN;

	if (!serv->encoding)
		server_set_encoding (serv, "UTF-8");

	serv->nickcount = 1;
	serv->end_of_motd = FALSE;
	serv->sent_capend = FALSE;
	serv->is_away = FALSE;
	serv->supports_watch = FALSE;
	serv->supports_monitor = FALSE;
	serv->bad_prefix = FALSE;
	serv->use_who = TRUE;
	serv->have_namesx = FALSE;
	serv->have_awaynotify = FALSE;
	serv->have_uhnames = FALSE;
	serv->have_whox = FALSE;
	serv->have_idmsg = FALSE;
	serv->have_accnotify = FALSE;
	serv->have_extjoin = FALSE;
	serv->have_account_tag = FALSE;
	serv->have_server_time = FALSE;
	serv->have_sasl = FALSE;
	serv->have_except = FALSE;
	serv->have_invite = FALSE;
}

char *
server_get_network (server *serv, gboolean fallback)
{
	/* check the network list */
	if (serv->network)
		return ((ircnet *)serv->network)->name;

	/* check the network name given in 005 NETWORK=... */
	if (serv->server_session && *serv->server_session->channel)
		return serv->server_session->channel;

	if (fallback)
		return serv->servername;

	return NULL;
}

void
server_set_name (server *serv, char *name)
{
	GSList *list = sess_list;
	session *sess;

	if (name[0] == 0)
		name = serv->hostname;

	/* strncpy parameters must NOT overlap */
	if (name != serv->servername)
	{
		safe_strcpy (serv->servername, name, sizeof (serv->servername));
	}

	while (list)
	{
		sess = (session *) list->data;
		if (sess->server == serv)
			fe_set_title (sess);
		list = list->next;
	}

	if (serv->server_session->type == SESS_SERVER)
	{
		if (serv->network)
		{
			safe_strcpy (serv->server_session->channel, ((ircnet *)serv->network)->name, CHANLEN);
		} else
		{
			safe_strcpy (serv->server_session->channel, name, CHANLEN);
		}
		fe_set_channel (serv->server_session);
	}
}

struct away_msg *
server_away_find_message (server *serv, char *nick)
{
	struct away_msg *away;
	GSList *list = away_list;
	while (list)
	{
		away = (struct away_msg *) list->data;
		if (away->server == serv && !serv->p_cmp (nick, away->nick))
			return away;
		list = list->next;
	}
	return NULL;
}

static void
server_away_free_messages (server *serv)
{
	GSList *list, *next;
	struct away_msg *away;

	list = away_list;
	while (list)
	{
		away = list->data;
		next = list->next;
		if (away->server == serv)
		{
			away_list = g_slist_remove (away_list, away);
			g_free (away->message);
			g_free (away);
			next = away_list;
		}
		list = next;
	}
}

void
server_away_save_message (server *serv, char *nick, char *msg)
{
	struct away_msg *away = server_away_find_message (serv, nick);

	if (away)						  /* Change message for known user */
	{
		g_free (away->message);
		away->message = g_strdup (msg);
	}
	else
	{
		/* Create brand new entry */
		away = g_new(struct away_msg, 1);
		away->server = serv;
		safe_strcpy (away->nick, nick, sizeof (away->nick));
		away->message = g_strdup (msg);
		away_list = g_slist_prepend (away_list, away);
	}
}

void
server_free (server *serv)
{
	serv->cleanup (serv);

	serv_list = g_slist_remove (serv_list, serv);

	dcc_notify_kill (serv);
	serv->flush_queue (serv);
	server_away_free_messages (serv);

	g_free (serv->nick_modes);
	g_free (serv->nick_prefixes);
	g_free (serv->chanmodes);
	g_free (serv->chantypes);
	g_free (serv->bad_nick_prefixes);
	g_free (serv->last_away_reason);
	g_free (serv->encoding);

	g_iconv_close (serv->read_converter);
	g_iconv_close (serv->write_converter);

	if (serv->favlist)
		g_slist_free_full (serv->favlist, (GDestroyNotify) servlist_favchan_free);

	fe_server_callback (serv);

	g_free (serv);

	notify_cleanup ();
}
