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

#define WANTSOCKET
#define WANTARPA
#include "inet.h"

#ifdef WIN32
#include <winbase.h>
#include <io.h>
#else
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

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

#ifdef USE_OPENSSL
#include <openssl/ssl.h>		  /* SSL_() */
#include <openssl/err.h>		  /* ERR_() */
#include "ssl.h"
#endif

#define HEXCHAT_CONNECTION_ERROR g_quark_from_static_string("hexchat-connection-error")

#ifdef USE_OPENSSL
/* local variables */
static struct session *g_sess = NULL;
#endif

static GSList *away_list = NULL;
GSList *serv_list = NULL;

static void auto_reconnect (server *serv, int send_quit, int err);
static void server_disconnect (session * sess, int sendquit, int err);
static int server_cleanup (server * serv);
static void server_connect (server *serv, char *hostname, int port, int no_login);

enum
{
	RESOLVE_FAILED = 1,
	CONNECTION_FAILED,
	BIND_FAILED,
	PROXY_FAILED
};

/* actually send to the socket. This might do a character translation or
   send via SSL. server/dcc both use this function. */

int
tcp_send_real (void *ssl, int sok, char *encoding, int using_irc, char *buf, int len)
{
	int ret;
	char *locale;
	gsize loc_len;

	if (encoding == NULL)	/* system */
	{
		locale = NULL;
		if (!prefs.utf8_locale)
		{
			const gchar *charset;

			g_get_charset (&charset);
			locale = g_convert_with_fallback (buf, len, charset, "UTF-8",
														 "?", 0, &loc_len, 0);
		}
	} else
	{
		if (using_irc)	/* using "IRC" encoding (CP1252/UTF-8 hybrid) */
			/* if all chars fit inside CP1252, use that. Otherwise this
			   returns NULL and we send UTF-8. */
			locale = g_convert (buf, len, "CP1252", "UTF-8", 0, &loc_len, 0);
		else
			locale = g_convert_with_fallback (buf, len, encoding, "UTF-8",
														 "?", 0, &loc_len, 0);
	}

	if (locale)
	{
		len = loc_len;
#ifdef USE_OPENSSL
		if (!ssl)
			ret = send (sok, locale, len, 0);
		else
			ret = _SSL_send (ssl, locale, len);
#else
		ret = send (sok, locale, len, 0);
#endif
		g_free (locale);
	} else
	{
#ifdef USE_OPENSSL
		if (!ssl)
			ret = send (sok, buf, len, 0);
		else
			ret = _SSL_send (ssl, buf, len);
#else
		ret = send (sok, buf, len, 0);
#endif
	}

	return ret;
}

static int
server_send_real (server *serv, char *buf, int len)
{
	fe_add_rawlog (serv, buf, len, TRUE);

	url_check_line (buf);

	return tcp_send_real (serv->ssl, g_socket_get_fd (serv->sok), serv->encoding, serv->using_irc,
								 buf, len);
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
		/* WHO/MODE get the lowest priority */
		if (g_ascii_strncasecmp (dbuf + 1, "WHO ", 4) == 0 ||
		/* but only MODE queries, not changes */
			(g_ascii_strncasecmp (dbuf + 1, "MODE", 4) == 0 &&
			 strchr (dbuf, '-') == NULL &&
			 strchr (dbuf, '+') == NULL))
			dbuf[0] = 0;
	}

	serv->outbound_queue = g_slist_append (serv->outbound_queue, dbuf);
	serv->sendq_len += len; /* tcp_send_queue uses strlen */

	if (tcp_send_queue (serv) && noqueue)
		fe_timeout_add (500, tcp_send_queue, serv);

	return 1;
}

/*int
tcp_send (server *serv, char *buf)
{
	return tcp_send_len (serv, buf, strlen (buf));
}*/

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

static int
close_socket_cb (gpointer sok)
{
	g_object_unref (sok);
	return 0;
}

static void
close_socket (GSocket *sok)
{
	/* close the socket in 5 seconds so the QUIT message is not lost */
	fe_timeout_add (5000, close_socket_cb, sok);
}

/* handle 1 line of text received from the server */

static void
server_inline (server *serv, char *line, gssize len)
{
	char *utf_line_allocated = NULL;

	/* Checks whether we're set to use UTF-8 charset */
	if (serv->using_irc ||				/* 1. using CP1252/UTF-8 Hybrid */
		(serv->encoding == NULL && prefs.utf8_locale) || /* OR 2. using system default->UTF-8 */
	    (serv->encoding != NULL &&				/* OR 3. explicitly set to UTF-8 */
		 (g_ascii_strcasecmp (serv->encoding, "UTF8") == 0 ||
		  g_ascii_strcasecmp (serv->encoding, "UTF-8") == 0)))
	{
		/* The user has the UTF-8 charset set, either via /charset
		command or from his UTF-8 locale. Thus, we first try the
		UTF-8 charset, and if we fail to convert, we assume
		it to be ISO-8859-1 (see text_validate). */

		utf_line_allocated = text_validate (&line, &len);
	}
	else
	{
		/* Since the user has an explicit charset set, either
		via /charset command or from his non-UTF8 locale,
		we don't fallback to ISO-8859-1 and instead try to remove
		errnoeous octets till the string is convertable in the
		said charset. */

		const char *encoding = NULL;

		if (serv->encoding != NULL)
			encoding = serv->encoding;
		else
			g_get_charset (&encoding);

		if (encoding != NULL)
		{
			char *conv_line; /* holds a copy of the original string */
			gsize conv_len; /* tells g_convert how much of line to convert */
			gsize utf_len;
			gsize read_len;
			GError *err;
			gboolean retry;

			conv_line = g_malloc (len + 1);
			memcpy (conv_line, line, len);
			conv_line[len] = 0;
			conv_len = len;

			/* if CP1255, convert it with the NUL terminator.
				Works around SF bug #1122089 */
			if (serv->using_cp1255)
				conv_len++;

			do
			{
				err = NULL;
				retry = FALSE;
				utf_line_allocated = g_convert_with_fallback (conv_line, conv_len, "UTF-8", encoding, "?", &read_len, &utf_len, &err);
				if (err != NULL)
				{
					if (err->code == G_CONVERT_ERROR_ILLEGAL_SEQUENCE && conv_len > (read_len + 1))
					{
						/* Make our best bet by removing the erroneous char.
						   This will work for casual 8-bit strings with non-standard chars. */
						memmove (conv_line + read_len, conv_line + read_len + 1, conv_len - read_len -1);
						conv_len--;
						retry = TRUE;
					}
					g_error_free (err);
				}
			} while (retry);

			g_free (conv_line);

			/* If any conversion has occured at all. Conversion might fail
			due to errors other than invalid sequences, e.g. unknown charset. */
			if (utf_line_allocated != NULL)
			{
				line = utf_line_allocated;
				len = utf_len;
				if (serv->using_cp1255 && len > 0)
					len--;
			}
			else
			{
				/* If all fails, treat as UTF-8 with fallback to ISO-8859-1. */
				utf_line_allocated = text_validate (&line, &len);
			}
		}
	}

	fe_add_rawlog (serv, line, len, FALSE);

	/* let proto-irc.c handle it */
	serv->p_inline (serv, line, len);

	g_free (utf_line_allocated);
}

/* read data from socket */

static gboolean
server_read (GIOChannel *source, GIOCondition condition, server *serv)
{
	int sok = g_socket_get_fd (serv->sok);
	int error, i, len;
	char lbuf[2050];

	while (1)
	{
#ifdef USE_OPENSSL
		if (!serv->ssl)
#endif
			len = recv (sok, lbuf, sizeof (lbuf) - 2, 0);
#ifdef USE_OPENSSL
		else
			len = _SSL_recv (serv->ssl, lbuf, sizeof (lbuf) - 2);
#endif
		if (len < 1)
		{
			error = 0;
			if (len < 0)
			{
				if (would_block ())
					return TRUE;
				error = sock_error ();
			}
			if (!serv->end_of_motd)
			{
				server_disconnect (serv->server_session, FALSE, error);
				if (!servlist_cycle (serv))
				{
					if (prefs.hex_net_auto_reconnect)
						auto_reconnect (serv, FALSE, error);
				}
			} else
			{
				if (prefs.hex_net_auto_reconnect)
					auto_reconnect (serv, FALSE, error);
				else
					server_disconnect (serv->server_session, FALSE, error);
			}
			return TRUE;
		}

		i = 0;

		lbuf[len] = 0;

		while (i < len)
		{
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
	g_socket_set_blocking (serv->sok, FALSE);
	serv->iotag = fe_input_add (g_socket_get_fd(serv->sok), G_IO_OUT|G_IO_ERR|G_IO_PRI, server_read, serv);
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
	if (serv->iotag)
	{
		fe_input_remove (serv->iotag);
		serv->iotag = 0;
	}

	if (serv->joindelay_tag)
	{
		fe_timeout_remove (serv->joindelay_tag);
		serv->joindelay_tag = 0;
	}

#ifdef USE_OPENSSL
	if (serv->ssl_do_connect_tag)
	{
		fe_timeout_remove (serv->ssl_do_connect_tag);
		serv->ssl_do_connect_tag = 0;
	}
#endif

	if (serv->cancellable)
	{
		g_object_unref (serv->cancellable);
		serv->cancellable = NULL;
	}

	fe_progressbar_end (serv);

	serv->connecting = FALSE;
	fe_server_event (serv, FE_SE_DISCONNECT, 0);
}

#ifdef USE_OPENSSL
#define	SSLTMOUT	90				  /* seconds */
static void
ssl_cb_info (SSL * s, int where, int ret)
{
/*	char buf[128];*/


	return;							  /* FIXME: make debug level adjustable in serverlist or settings */

/*	g_snprintf (buf, sizeof (buf), "%s (%d)", SSL_state_string_long (s), where);
	if (g_sess)
		EMIT_SIGNAL (XP_TE_SSLMESSAGE, g_sess, buf, NULL, NULL, NULL, 0);
	else
		fprintf (stderr, "%s\n", buf);*/
}

static int
ssl_cb_verify (int ok, X509_STORE_CTX * ctx)
{
	char subject[256];
	char issuer[256];
	char buf[512];


	X509_NAME_oneline (X509_get_subject_name (ctx->current_cert), subject,
							 sizeof (subject));
	X509_NAME_oneline (X509_get_issuer_name (ctx->current_cert), issuer,
							 sizeof (issuer));

	g_snprintf (buf, sizeof (buf), "* Subject: %s", subject);
	EMIT_SIGNAL (XP_TE_SSLMESSAGE, g_sess, buf, NULL, NULL, NULL, 0);
	g_snprintf (buf, sizeof (buf), "* Issuer: %s", issuer);
	EMIT_SIGNAL (XP_TE_SSLMESSAGE, g_sess, buf, NULL, NULL, NULL, 0);

	return (TRUE);					  /* always ok */
}

static int
ssl_do_connect (server * serv)
{
	char buf[128];

	g_sess = serv->server_session;
	if (SSL_connect (serv->ssl) <= 0)
	{
		char err_buf[128];
		int err;

		g_sess = NULL;
		if ((err = ERR_get_error ()) > 0)
		{
			ERR_error_string (err, err_buf);
			g_snprintf (buf, sizeof (buf), "(%d) %s", err, err_buf);
			EMIT_SIGNAL (XP_TE_CONNFAIL, serv->server_session, buf, NULL,
							 NULL, NULL, 0);

			if (ERR_GET_REASON (err) == SSL_R_WRONG_VERSION_NUMBER)
				PrintText (serv->server_session, _("Are you sure this is a SSL capable server and port?\n"));

			server_cleanup (serv);

			if (prefs.hex_net_auto_reconnectonfail)
				auto_reconnect (serv, FALSE, -1);

			return (0);				  /* remove it (0) */
		}
	}
	g_sess = NULL;

	if (SSL_is_init_finished (serv->ssl))
	{
		struct cert_info cert_info;
		struct chiper_info *chiper_info;
		int verify_error;
		int i;

		if (!_SSL_get_cert_info (&cert_info, serv->ssl))
		{
			g_snprintf (buf, sizeof (buf), "* Certification info:");
			EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
							 NULL, 0);
			g_snprintf (buf, sizeof (buf), "  Subject:");
			EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
							 NULL, 0);
			for (i = 0; cert_info.subject_word[i]; i++)
			{
				g_snprintf (buf, sizeof (buf), "    %s", cert_info.subject_word[i]);
				EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
								 NULL, 0);
			}
			g_snprintf (buf, sizeof (buf), "  Issuer:");
			EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
							 NULL, 0);
			for (i = 0; cert_info.issuer_word[i]; i++)
			{
				g_snprintf (buf, sizeof (buf), "    %s", cert_info.issuer_word[i]);
				EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
								 NULL, 0);
			}
			g_snprintf (buf, sizeof (buf), "  Public key algorithm: %s (%d bits)",
						 cert_info.algorithm, cert_info.algorithm_bits);
			EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
							 NULL, 0);
			/*if (cert_info.rsa_tmp_bits)
			{
				g_snprintf (buf, sizeof (buf),
							 "  Public key algorithm uses ephemeral key with %d bits",
							 cert_info.rsa_tmp_bits);
				EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
								 NULL, 0);
			}*/
			g_snprintf (buf, sizeof (buf), "  Sign algorithm %s",
						 cert_info.sign_algorithm/*, cert_info.sign_algorithm_bits*/);
			EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
							 NULL, 0);
			g_snprintf (buf, sizeof (buf), "  Valid since %s to %s",
						 cert_info.notbefore, cert_info.notafter);
			EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
							 NULL, 0);
		} else
		{
			g_snprintf (buf, sizeof (buf), " * No Certificate");
			EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
							 NULL, 0);
		}

		chiper_info = _SSL_get_cipher_info (serv->ssl);	/* static buffer */
		g_snprintf (buf, sizeof (buf), "* Cipher info:");
		EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL, NULL,
						 0);
		g_snprintf (buf, sizeof (buf), "  Version: %s, cipher %s (%u bits)",
					 chiper_info->version, chiper_info->chiper,
					 chiper_info->chiper_bits);
		EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL, NULL,
						 0);

		verify_error = SSL_get_verify_result (serv->ssl);
		switch (verify_error)
		{
		case X509_V_OK:
			{
				X509 *cert = SSL_get_peer_certificate (serv->ssl);
				int hostname_err;
				if ((hostname_err = _SSL_check_hostname(cert, serv->hostname)) != 0)
				{
					g_snprintf (buf, sizeof (buf), "* Verify E: Failed to validate hostname? (%d)%s",
							 hostname_err, serv->accept_invalid_cert ? " -- Ignored" : "");
					if (serv->accept_invalid_cert)
						EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL, NULL, 0);
					else
						goto conn_fail;
				}
				break;
			}
			/* g_snprintf (buf, sizeof (buf), "* Verify OK (?)"); */
			/* EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL, NULL, 0); */
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
		case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
		case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
		case X509_V_ERR_CERT_HAS_EXPIRED:
			if (serv->accept_invalid_cert)
			{
				g_snprintf (buf, sizeof (buf), "* Verify E: %s.? (%d) -- Ignored",
							 X509_verify_cert_error_string (verify_error),
							 verify_error);
				EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
								 NULL, 0);
				break;
			}
		default:
			g_snprintf (buf, sizeof (buf), "%s.? (%d)",
						 X509_verify_cert_error_string (verify_error),
						 verify_error);
conn_fail:
			EMIT_SIGNAL (XP_TE_CONNFAIL, serv->server_session, buf, NULL, NULL,
							 NULL, 0);

			server_cleanup (serv);

			return (0);
		}

		server_stopconnecting (serv);

		/* activate gtk poll */
		server_connected (serv);

		return (0);					  /* remove it (0) */
	} else
	{
		if (serv->ssl->session && serv->ssl->session->time + SSLTMOUT < time (NULL))
		{
			g_snprintf (buf, sizeof (buf), "SSL handshake timed out");
			EMIT_SIGNAL (XP_TE_CONNFAIL, serv->server_session, buf, NULL,
							 NULL, NULL, 0);
			server_cleanup (serv); /* ->connecting = FALSE */

			if (prefs.hex_net_auto_reconnectonfail)
				auto_reconnect (serv, FALSE, -1);

			return (0);				  /* remove it (0) */
		}

		return (1);					  /* call it more (1) */
	}
}
#endif

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
auto_reconnect (server *serv, int send_quit, int err)
{
	session *s;
	GSList *list;
	int del;

	if (serv->server_session == NULL)
		return;

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

	if (serv->connected)
		server_disconnect (serv->server_session, send_quit, err);

	del = prefs.hex_net_reconnect_delay * 1000;
	if (del < 1000)
		del = 500;				  /* so it doesn't block the gui */

#ifndef WIN32
	if (err == -1 || err == 0 || err == ECONNRESET || err == ETIMEDOUT)
#else
	if (err == -1 || err == 0 || err == WSAECONNRESET || err == WSAETIMEDOUT)
#endif
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
#ifdef USE_OPENSSL
#define	SSLDOCONNTMOUT	300
	if (serv->use_ssl)
	{
		char *err;

		/* it'll be a memory leak, if connection isn't terminated by
		   server_cleanup() */
		serv->ssl = _SSL_socket (serv->ctx, g_socket_get_fd (serv->sok));
		if ((err = _SSL_set_verify (serv->ctx, ssl_cb_verify, NULL)))
		{
			EMIT_SIGNAL (XP_TE_CONNFAIL, serv->server_session, err, NULL,
							 NULL, NULL, 0);
			server_cleanup (serv);	/* ->connecting = FALSE */
			return;
		}
		/* FIXME: it'll be needed by new servers */
		/* send(serv->sok, "STLS\r\n", 6, 0); sleep(1); */
		g_socket_set_blocking (serv->sok, FALSE);
		serv->ssl_do_connect_tag = fe_timeout_add (SSLDOCONNTMOUT,
																 ssl_do_connect, serv);
		return;
	}

	serv->ssl = NULL;
#endif
	server_stopconnecting (serv);	/* ->connecting = FALSE */
	/* activate glib poll */
	server_connected (serv);
}

/* kill all sockets & iotags of a server. Stop a connection attempt, or
   disconnect if already connected. */

static int
server_cleanup (server * serv)
{
	fe_set_lag (serv, 0);

	if (serv->iotag)
	{
		fe_input_remove (serv->iotag);
		serv->iotag = 0;
	}

	if (serv->joindelay_tag)
	{
		fe_timeout_remove (serv->joindelay_tag);
		serv->joindelay_tag = 0;
	}

#ifdef USE_OPENSSL
	if (serv->ssl)
	{
		SSL_shutdown (serv->ssl);
		SSL_free (serv->ssl);
		serv->ssl = NULL;
	}
#endif

	if (serv->connecting)
	{
		server_stopconnecting (serv);
		if (serv->sok4)
			g_object_unref (serv->sok4);
		if (serv->sok6)
			g_object_unref (serv->sok6);
		return 1;
	}

	if (serv->connected)
	{
		close_socket (serv->sok);
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
server_disconnect (session * sess, int sendquit, int err)
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
		g_cancellable_cancel (serv->cancellable);
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
				EMIT_SIGNAL (XP_TE_DISCON, sess, errorstring (err), NULL, NULL, NULL, 0);

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

struct sock_connect
{
	char version;
	char type;
	guint16 port;
	guint32 address;
	char username[10];
};

/* traverse_socks() returns:
 *				0 success                *
 *          1 socks traversal failed */

static int
traverse_socks (int sok, char *serverAddr, int port, GError **error)
{
	struct sock_connect sc;
	unsigned char buf[256];

	sc.version = 4;
	sc.type = 1;
	sc.port = htons (port);
	sc.address = inet_addr (serverAddr);
	strncpy (sc.username, prefs.hex_irc_user_name, 9);

	send (sok, (char *) &sc, 8 + strlen (sc.username) + 1, 0);
	buf[1] = 0;
	recv (sok, buf, 10, 0);
	if (buf[1] == 90)
		return 0;

	g_snprintf (buf, sizeof (buf), "SOCKS\tServer reported error %d,%d.\n", buf[0], buf[1]);
	*error = g_error_new_literal (HEXCHAT_CONNECTION_ERROR, PROXY_FAILED, buf);
	return 1;
}

struct sock5_connect1
{
	char version;
	char nmethods;
	char method;
};

static int
traverse_socks5 (int sok, char *serverAddr, int port, GError **error)
{
	struct sock5_connect1 sc1;
	unsigned char *sc2;
	unsigned int packetlen, addrlen;
	unsigned char buf[260];
	int auth = prefs.hex_net_proxy_auth && prefs.hex_net_proxy_user[0] && prefs.hex_net_proxy_pass[0];

	sc1.version = 5;
	sc1.nmethods = 1;
	if (auth)
		sc1.method = 2;  /* Username/Password Authentication (UPA) */
	else
		sc1.method = 0;  /* NO Authentication */
	send (sok, (char *) &sc1, 3, 0);
	if (recv (sok, buf, 2, 0) != 2)
		goto read_error;

	if (buf[0] != 5)
	{
		*error = g_error_new_literal (HEXCHAT_CONNECTION_ERROR, PROXY_FAILED, "SOCKS\tServer is not socks version 5.\n");
		return 1;
	}

	/* did the server say no auth required? */
	if (buf[1] == 0)
		auth = 0;

	if (auth)
	{
		int len_u=0, len_p=0;

		/* authentication sub-negotiation (RFC1929) */
		if (buf[1] != 2)  /* UPA not supported by server */
		{
			*error = g_error_new_literal (HEXCHAT_CONNECTION_ERROR, PROXY_FAILED, "SOCKS\tServer doesn't support UPA authentication.\n");
			return 1;
		}

		memset (buf, 0, sizeof(buf));

		/* form the UPA request */
		len_u = strlen (prefs.hex_net_proxy_user);
		len_p = strlen (prefs.hex_net_proxy_pass);
		buf[0] = 1;
		buf[1] = len_u;
		memcpy (buf + 2, prefs.hex_net_proxy_user, len_u);
		buf[2 + len_u] = len_p;
		memcpy (buf + 3 + len_u, prefs.hex_net_proxy_pass, len_p);

		send (sok, buf, 3 + len_u + len_p, 0);
		if ( recv (sok, buf, 2, 0) != 2 )
			goto read_error;
		if ( buf[1] != 0 )
		{
			*error = g_error_new_literal (HEXCHAT_CONNECTION_ERROR, PROXY_FAILED, "SOCKS\tAuthentication failed. "
							 "Is username and password correct?\n");
			return 1; /* UPA failed! */
		}
	}
	else
	{
		if (buf[1] != 0)
		{
			*error = g_error_new_literal (HEXCHAT_CONNECTION_ERROR, PROXY_FAILED, "SOCKS\tAuthentication required but disabled in settings.\n");
			return 1;
		}
	}

	addrlen = strlen (serverAddr);
	packetlen = 4 + 1 + addrlen + 2;
	sc2 = g_malloc (packetlen);
	sc2[0] = 5;						  /* version */
	sc2[1] = 1;						  /* command */
	sc2[2] = 0;						  /* reserved */
	sc2[3] = 3;						  /* address type */
	sc2[4] = (unsigned char) addrlen;	/* hostname length */
	memcpy (sc2 + 5, serverAddr, addrlen);
	*((unsigned short *) (sc2 + 5 + addrlen)) = htons (port);
	send (sok, sc2, packetlen, 0);
	g_free (sc2);

	/* consume all of the reply */
	if (recv (sok, buf, 4, 0) != 4)
		goto read_error;
	if (buf[0] != 5 || buf[1] != 0)
	{
		if (buf[1] == 2)
			g_snprintf (buf, sizeof (buf), "SOCKS\tProxy refused to connect to host (not allowed).\n");
		else
			g_snprintf (buf, sizeof (buf), "SOCKS\tProxy failed to connect to host (error %d).\n", buf[1]);
		*error = g_error_new_literal (HEXCHAT_CONNECTION_ERROR, PROXY_FAILED, buf);
		return 1;
	}
	if (buf[3] == 1)	/* IPV4 32bit address */
	{
		if (recv (sok, buf, 6, 0) != 6)
			goto read_error;
	} else if (buf[3] == 4)	/* IPV6 128bit address */
	{
		if (recv (sok, buf, 18, 0) != 18)
			goto read_error;
	} else if (buf[3] == 3)	/* string, 1st byte is size */
	{
		if (recv (sok, buf, 1, 0) != 1)	/* read the string size */
			goto read_error;
		packetlen = buf[0] + 2;	/* can't exceed 260 */
		if (recv (sok, buf, packetlen, 0) != packetlen)
			goto read_error;
	}

	return 0;	/* success */

read_error:
	*error = g_error_new_literal (HEXCHAT_CONNECTION_ERROR, PROXY_FAILED, "SOCKS\tRead error from server.\n");
	return 1;
}

static int
traverse_wingate (int sok, char *serverAddr, int port, GError **error)
{
	char buf[128];

	g_snprintf (buf, sizeof (buf), "%s %d\r\n", serverAddr, port);
	send (sok, buf, strlen (buf), 0);

	return 0;
}

/* stuff for HTTP auth is here */

static void
three_to_four (char *from, char *to)
{
	static const char tab64[64]=
	{
		'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
		'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
		'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
		'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
	};

	to[0] = tab64 [ (from[0] >> 2) & 63 ];
	to[1] = tab64 [ ((from[0] << 4) | (from[1] >> 4)) & 63 ];
	to[2] = tab64 [ ((from[1] << 2) | (from[2] >> 6)) & 63 ];
	to[3] = tab64 [ from[2] & 63 ];
};

void
base64_encode (char *to, char *from, unsigned int len)
{
	while (len >= 3)
	{
		three_to_four (from, to);
		len -= 3;
		from += 3;
		to += 4;
	}
	if (len)
	{
		char three[3] = {0,0,0};
		unsigned int i;
		for (i = 0; i < len; i++)
		{
			three[i] = *from++;
		}
		three_to_four (three, to);
		if (len == 1)
		{
			to[2] = to[3] = '=';
		}
		else if (len == 2)
		{
			to[3] = '=';
		}
		to += 4;
	};
	to[0] = 0;
}

static int
traverse_http (int sok, char *serverAddr, int port, GError **error)
{
	char buf[512];
	char auth_data[256];
	char auth_data2[252];
	int n, n2;

	n = g_snprintf (buf, sizeof (buf), "CONNECT %s:%d HTTP/1.0\r\n",
					  serverAddr, port);
	if (prefs.hex_net_proxy_auth)
	{
		n2 = g_snprintf (auth_data2, sizeof (auth_data2), "%s:%s",
							prefs.hex_net_proxy_user, prefs.hex_net_proxy_pass);
		base64_encode (auth_data, auth_data2, n2);
		n += g_snprintf (buf+n, sizeof (buf)-n, "Proxy-Authorization: Basic %s\r\n", auth_data);
	}
	n += g_snprintf (buf+n, sizeof (buf)-n, "\r\n");
	send (sok, buf, n, 0);

	n = waitline (sok, buf, sizeof(buf), TRUE);

	/* "HTTP/1.0 200 OK" */
	if (n < 12 || (memcmp (buf, "HTTP/", 5) || memcmp (buf + 9, "200", 3)))
	{
		*error = g_error_new_literal (HEXCHAT_CONNECTION_ERROR, PROXY_FAILED, buf);
		return 1;
	}
	while (1)
	{
		/* read until blank line */
		n = waitline (sok, buf, sizeof(buf), TRUE);
		if (n < 1 || (n == 1 && buf[0] == '\n'))
			break;
	}
	return 0;
}

static int
traverse_proxy (int proxy_type, GSocket *sok_r, char *ip, int port, GError **error)
{
	int sok = g_socket_get_fd (sok_r);
	switch (proxy_type)
	{
	case 1:
		return traverse_wingate (sok, ip, port, error);
	case 2:
		return traverse_socks (sok, ip, port, error);
	case 3:
		return traverse_socks5 (sok, ip, port, error);
	case 4:
		return traverse_http (sok, ip, port, error);
	}

	return 1;
}

struct connect_data
{
	server *serv;
	char *local_ip;
	char *hostname;
	char *proxy_hostname;
	char *ip;
	char *port;
};

static void
connect_data_free (struct connect_data *data)
{
	if (data)
	{
		g_free (data->local_ip);
		g_free (data->hostname);
		g_free (data->proxy_hostname);
		g_free (data->ip);
		g_free (data->port);
		g_free (data);
	}
}

static void
server_connect_finish (GObject *source, GAsyncResult *res, gpointer user_data)
{
	GError *error = NULL;
	server *serv;
	session *sess;
	struct connect_data *data;
	gboolean retry = FALSE;
	char buf[256];

	data = (struct connect_data*)g_task_get_task_data (G_TASK(res));
	serv = data->serv;

	if (!is_server(serv))
	{
		g_warning ("Server destroyed before connection finished!\n");
		return;
	}

	sess = serv->server_session;

	/* These print after the fact, but should be good enough */
	if (data->proxy_hostname)
		EMIT_SIGNAL (XP_TE_SERVERLOOKUP, sess, data->proxy_hostname, NULL, NULL, NULL, 0);

	EMIT_SIGNAL (XP_TE_CONNECT, sess, data->hostname, data->ip, data->port, NULL, 0);

	/* Update local ip for dcc */
	if (data->local_ip)
		prefs.local_ip = inet_addr (data->local_ip);

	serv->sok = g_task_propagate_pointer (G_TASK(res), &error);
	if (!error)
	{
		if (serv->sok == serv->sok4)
			g_object_unref (serv->sok6);
		else
			g_object_unref (serv->sok4);
		{
			guint16 port = net_get_local_port (serv->sok);
			g_snprintf (buf, sizeof (buf), "IDENTD %"G_GUINT16_FORMAT" ", port);
			if (serv->network && ((ircnet *)serv->network)->user)
				g_strlcat (buf, ((ircnet *)serv->network)->user, sizeof (buf));
			else
				g_strlcat (buf, prefs.hex_irc_user_name, sizeof (buf));

			handle_command (serv->server_session, buf, FALSE);
		}
		server_connect_success (serv);
		return;
	}

	g_warning ("%s\n", error->message);

	switch (error->code)
	{
	case RESOLVE_FAILED:
		retry = TRUE;
		EMIT_SIGNAL (XP_TE_UKNHOST, sess, NULL, NULL, NULL, NULL, 0);
		goto close_connection;
	case CONNECTION_FAILED:
		retry = TRUE;
		EMIT_SIGNAL (XP_TE_CONNFAIL, sess, error->message, NULL, NULL, NULL, 0);
		goto close_connection;
	case BIND_FAILED:
		PrintText (sess, error->message);
		break;
	case PROXY_FAILED:
		PrintTextf (sess, _("Proxy Traveral Failed: %s\n"), error->message);
		server_disconnect (sess, FALSE, -1);
		break;
	case 9:
		EMIT_SIGNAL (XP_TE_SERVERLOOKUP, sess, error->message, NULL, NULL, NULL, 0);
		break;
	case G_IO_ERROR_CANCELLED:
		EMIT_SIGNAL (XP_TE_STOPCONNECT, sess, NULL, NULL, NULL, NULL, 0);
		goto close_connection;
	default:
		g_warning ("%d\n", error->code);
		g_assert_not_reached ();
	}

	return;

close_connection:
	server_stopconnecting (serv);
	if (serv->sok4)
		g_object_unref (serv->sok4);
	if (serv->sok6)
		g_object_unref (serv->sok6);

	if (retry && !servlist_cycle (serv) && prefs.hex_net_auto_reconnectonfail)
		auto_reconnect (serv, FALSE, -1);
}

static void
server_connect_thread (GTask *task, gpointer source, gpointer task_data, GCancellable *cancellable)
{
	netstore *ns_server;
	struct connect_data *data = (struct connect_data*)task_data;
	server *serv = data->serv;
	GSocket *sok = NULL, *psok;
	char *proxy_ip = NULL;
	int proxy_type = 0;
	char *proxy_host = NULL, *hostname;
	guint16 proxy_port, connect_port, port;
	GError *err = NULL, *err4 = NULL, *err6 = NULL;

	if (!is_server(serv))
	{
		g_warning ("Server destroyed before connection started!\n");
		return;
	}
	port = serv->port;
	hostname = serv->hostname;

	ns_server = net_store_new ();

	/* Bind local address if configured */
	if (prefs.hex_net_bind_host[0])
	{
		netstore *ns_local = net_store_new ();

		data->local_ip = net_resolve (ns_local, prefs.hex_net_bind_host, NULL, &err);
		if (!err)
		{
			net_bind (ns_local, serv->sok4, serv->sok6, &err4, &err6);
			if (err4 || err6)
			{
				net_store_destroy (ns_local);
				if (err4) /* TODO: could handle this better? */
				{
					g_task_return_error (task, err4);
					g_clear_error (&err6);
					goto error_cleanup;
				}
				else
				{
					g_task_return_error (task, err4);
					goto error_cleanup;
				}
			}
		}
		else
		{
			g_task_return_error (task, err);
			net_store_destroy (ns_local);
			goto error_cleanup;
		}

		net_store_destroy (ns_local);
	}

	/* Connect to proxy if configured */
	if (!serv->dont_use_proxy && prefs.hex_net_proxy_use != 2 && prefs.hex_net_proxy_type > 0)
	{
		if (prefs.hex_net_proxy_type == 5)
		{
			char *proxy;

			proxy = net_resolve_proxy (hostname, port, &err);
			if (err)
			{
				g_task_return_error (task, err);
				goto error_cleanup;
			}

			if (proxy)
			{
				net_parse_proxy_uri (proxy, &proxy_host, &proxy_port, &proxy_type);
				g_free (proxy);
			}
			else
				proxy_type = 0;
		}
		else if (prefs.hex_net_proxy_host[0])
		{
			proxy_type = prefs.hex_net_proxy_type;
			proxy_host = g_strdup (prefs.hex_net_proxy_host);
			proxy_port = prefs.hex_net_proxy_port;
		}
	}
	data->proxy_hostname = proxy_host;

	if (proxy_type > 0)
	{
		data->ip = net_resolve (ns_server, proxy_host, &data->hostname, &err);
		if (!data->ip)
		{
			g_task_return_error (task, err);
			goto error_cleanup;
		}
		connect_port = proxy_port;

		/* if using socks4, attempt to resolve ip for irc server */
		if (proxy_type == 2)
		{
			netstore *ns_proxy = net_store_new ();
			g_free (data->hostname);
			proxy_ip = net_resolve (ns_proxy, hostname, &data->hostname, &err);
			net_store_destroy (ns_proxy);
			if (err)
			{
				g_task_return_error (task, err);
				goto error_cleanup;
			}
		} else						  /* otherwise we can just use the hostname */
			proxy_ip = g_strdup (hostname);
	}
	else
	{
		data->ip = net_resolve (ns_server, hostname, &data->hostname, &err);
		if (err)
		{
			g_task_return_error (task, err);
			goto error_cleanup;
		}
		connect_port = port;
	}

	data->port = g_strdup_printf ("%d", connect_port);

	if (!serv->dont_use_proxy)
		psok = net_connect (ns_server, connect_port, serv->sok4, serv->sok6, &err);
	else
	{
		sok = net_connect (ns_server, connect_port, serv->sok4, serv->sok6, &err);
		psok = sok;
	}

	if (err)
	{
		g_task_return_error (task, err);
		goto error_cleanup;
	}
	else
	{
		/* connect succeeded */
		if (proxy_ip && traverse_proxy (proxy_type, psok, proxy_ip, port, &err))
		{
			g_task_return_error (task, err);
			goto error_cleanup;
		}
	}

error_cleanup:
	net_store_destroy (ns_server);
	g_free (proxy_ip);
	g_task_return_pointer (task, sok, g_object_unref); /* Does nothing if error'd */
	return;
}

static void
server_connect (server *serv, char *hostname, int port, int no_login)
{
	session *sess = serv->server_session;
	GTask *task;
	struct connect_data *data;

#ifdef USE_OPENSSL
	if (!serv->ctx && serv->use_ssl)
	{
		if (!(serv->ctx = _SSL_context_init (ssl_cb_info, FALSE)))
		{
			fprintf (stderr, "_SSL_context_init failed\n");
			exit (1);
		}
	}
#endif

	if (!hostname[0])
		return;

	if (port < 0)
	{
		/* use default port for this server type */
		port = 6667;
#ifdef USE_OPENSSL
		if (serv->use_ssl)
			port = 6697;
#endif
	}
	port &= 0xffff;	/* wrap around */

	if (serv->connected || serv->connecting || serv->recondelay_tag)
		server_disconnect (sess, TRUE, -1);

	fe_progressbar_start (sess);

	EMIT_SIGNAL (XP_TE_SERVERLOOKUP, sess, hostname, NULL, NULL, NULL, 0);

	safe_strcpy (serv->servername, hostname, sizeof (serv->servername));
	/* overlap illegal in strncpy */
	if (hostname != serv->hostname)
		safe_strcpy (serv->hostname, hostname, sizeof (serv->hostname));

#ifdef USE_OPENSSL
	if (serv->use_ssl)
	{
		char *cert_file;
		serv->have_cert = FALSE;

		/* first try network specific cert/key */
		cert_file = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "certs" G_DIR_SEPARATOR_S "%s.pem",
					 get_xdir (), server_get_network (serv, TRUE));
		if (SSL_CTX_use_certificate_file (serv->ctx, cert_file, SSL_FILETYPE_PEM) == 1)
		{
			if (SSL_CTX_use_PrivateKey_file (serv->ctx, cert_file, SSL_FILETYPE_PEM) == 1)
				serv->have_cert = TRUE;
		}
		else
		{
			/* if that doesn't exist, try <config>/certs/client.pem */
			cert_file = g_build_filename (get_xdir (), "certs", "client.pem", NULL);
			if (SSL_CTX_use_certificate_file (serv->ctx, cert_file, SSL_FILETYPE_PEM) == 1)
			{
				if (SSL_CTX_use_PrivateKey_file (serv->ctx, cert_file, SSL_FILETYPE_PEM) == 1)
					serv->have_cert = TRUE;
			}
		}
		g_free (cert_file);
	}
#endif

	server_set_defaults (serv);
	serv->connecting = TRUE;
	serv->port = port;
	serv->no_login = no_login;

	fe_server_event (serv, FE_SE_CONNECTING, 0);
	fe_set_away (serv);
	server_flush_queue (serv);

	net_sockets (&serv->sok4, &serv->sok6, NULL, NULL);

	/* start the connection in a thread */
	data = g_new0 (struct connect_data, 1);
	data->serv = serv;
	serv->cancellable = g_cancellable_new ();
	task = g_task_new (NULL, serv->cancellable, server_connect_finish, NULL);
	g_task_set_task_data (task, data, (GDestroyNotify)connect_data_free);
	g_task_set_return_on_cancel (task, TRUE);
	g_task_run_in_thread (task, server_connect_thread);
	g_object_unref (task);
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

	if (serv->encoding)
	{
		g_free (serv->encoding);
		/* can be left as NULL to indicate system encoding */
		serv->encoding = NULL;
		serv->using_cp1255 = FALSE;
		serv->using_irc = FALSE;
	}

	if (new_encoding)
	{
		serv->encoding = g_strdup (new_encoding);
		/* the serverlist GUI might have added a space 
			and short description - remove it. */
		space = strchr (serv->encoding, ' ');
		if (space)
			space[0] = 0;

		/* server_inline() uses these flags */
		if (!g_ascii_strcasecmp (serv->encoding, "CP1255") ||
			 !g_ascii_strcasecmp (serv->encoding, "WINDOWS-1255"))
			serv->using_cp1255 = TRUE;
		else if (!g_ascii_strcasecmp (serv->encoding, "IRC"))
			serv->using_irc = TRUE;
	}
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

	serv->nickcount = 1;
	serv->end_of_motd = FALSE;
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
	if (serv->favlist)
		g_slist_free_full (serv->favlist, (GDestroyNotify) servlist_favchan_free);
#ifdef USE_OPENSSL
	if (serv->ctx)
		_SSL_context_free (serv->ctx);
#endif

	fe_server_callback (serv);

	g_free (serv);

	notify_cleanup ();
}
