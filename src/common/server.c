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

/*#define DEBUG_MSPROXY*/

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

#ifdef USE_MSPROXY
#include "msproxy.h"
#endif

#ifdef WIN32
#include "identd.h"
#endif

#ifdef USE_LIBPROXY
#include <proxy.h>
#endif

#ifdef USE_OPENSSL
extern SSL_CTX *ctx;				  /* hexchat.c */
/* local variables */
static struct session *g_sess = NULL;
#endif

static GSList *away_list = NULL;
GSList *serv_list = NULL;

static void auto_reconnect (server *serv, int send_quit, int err);
static void server_disconnect (session * sess, int sendquit, int err);
static int server_cleanup (server * serv);
static void server_connect (server *serv, char *hostname, int port, int no_login);

#ifdef USE_LIBPROXY
extern pxProxyFactory *libproxy_factory;
#endif

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

	url_check_line (buf, len);

	return tcp_send_real (serv->ssl, serv->sok, serv->encoding, serv->using_irc,
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
				free (buf);
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

	dbuf = malloc (len + 2);	/* first byte is the priority */
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
tcp_sendf (server *serv, char *fmt, ...)
{
	va_list args;
	/* keep this buffer in BSS. Converting UTF-8 to ISO-8859-x might make the
      string shorter, so allow alot more than 512 for now. */
	static char send_buf[1540];	/* good code hey (no it's not overflowable) */
	int len;

	va_start (args, fmt);
	len = vsnprintf (send_buf, sizeof (send_buf) - 1, fmt, args);
	va_end (args);

	send_buf[sizeof (send_buf) - 1] = '\0';
	if (len < 0 || len > (sizeof (send_buf) - 1))
		len = strlen (send_buf);

	tcp_send_len (serv, send_buf, len);
}

static int
close_socket_cb (gpointer sok)
{
	closesocket (GPOINTER_TO_INT (sok));
	return 0;
}

static void
close_socket (int sok)
{
	/* close the socket in 5 seconds so the QUIT message is not lost */
	fe_timeout_add (5000, close_socket_cb, GINT_TO_POINTER (sok));
}

/* handle 1 line of text received from the server */

static void
server_inline (server *serv, char *line, int len)
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

	} else
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
			int conv_len; /* tells g_convert how much of line to convert */
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

	if (utf_line_allocated != NULL) /* only if a special copy was allocated */
		g_free (utf_line_allocated);
}

/* read data from socket */

static gboolean
server_read (GIOChannel *source, GIOCondition condition, server *serv)
{
	int sok = serv->sok;
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
	set_nonblocking (serv->sok);
	serv->iotag = fe_input_add (serv->sok, FIA_READ|FIA_EX, server_read, serv);
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
	free (pipefd);
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

#ifndef WIN32
	/* kill the child process trying to connect */
	kill (serv->childpid, SIGKILL);
	waitpid (serv->childpid, NULL, 0);

	close (serv->childwrite);
	close (serv->childread);
#else
	PostThreadMessage (serv->childpid, WM_QUIT, 0, 0);

	{
		/* if we close the pipe now, giowin32 will crash. */
		int *pipefd = malloc (sizeof (int) * 2);
		pipefd[0] = serv->childwrite;
		pipefd[1] = serv->childread;
		g_idle_add ((GSourceFunc)server_close_pipe, pipefd);
	}
#endif

#ifdef USE_OPENSSL
	if (serv->ssl_do_connect_tag)
	{
		fe_timeout_remove (serv->ssl_do_connect_tag);
		serv->ssl_do_connect_tag = 0;
	}
#endif

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

/*	snprintf (buf, sizeof (buf), "%s (%d)", SSL_state_string_long (s), where);
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

	snprintf (buf, sizeof (buf), "* Subject: %s", subject);
	EMIT_SIGNAL (XP_TE_SSLMESSAGE, g_sess, buf, NULL, NULL, NULL, 0);
	snprintf (buf, sizeof (buf), "* Issuer: %s", issuer);
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
			snprintf (buf, sizeof (buf), "(%d) %s", err, err_buf);
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
			snprintf (buf, sizeof (buf), "* Certification info:");
			EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
							 NULL, 0);
			snprintf (buf, sizeof (buf), "  Subject:");
			EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
							 NULL, 0);
			for (i = 0; cert_info.subject_word[i]; i++)
			{
				snprintf (buf, sizeof (buf), "    %s", cert_info.subject_word[i]);
				EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
								 NULL, 0);
			}
			snprintf (buf, sizeof (buf), "  Issuer:");
			EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
							 NULL, 0);
			for (i = 0; cert_info.issuer_word[i]; i++)
			{
				snprintf (buf, sizeof (buf), "    %s", cert_info.issuer_word[i]);
				EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
								 NULL, 0);
			}
			snprintf (buf, sizeof (buf), "  Public key algorithm: %s (%d bits)",
						 cert_info.algorithm, cert_info.algorithm_bits);
			EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
							 NULL, 0);
			/*if (cert_info.rsa_tmp_bits)
			{
				snprintf (buf, sizeof (buf),
							 "  Public key algorithm uses ephemeral key with %d bits",
							 cert_info.rsa_tmp_bits);
				EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
								 NULL, 0);
			}*/
			snprintf (buf, sizeof (buf), "  Sign algorithm %s",
						 cert_info.sign_algorithm/*, cert_info.sign_algorithm_bits*/);
			EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
							 NULL, 0);
			snprintf (buf, sizeof (buf), "  Valid since %s to %s",
						 cert_info.notbefore, cert_info.notafter);
			EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
							 NULL, 0);
		} else
		{
			snprintf (buf, sizeof (buf), " * No Certificate");
			EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
							 NULL, 0);
		}

		chiper_info = _SSL_get_cipher_info (serv->ssl);	/* static buffer */
		snprintf (buf, sizeof (buf), "* Cipher info:");
		EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL, NULL,
						 0);
		snprintf (buf, sizeof (buf), "  Version: %s, cipher %s (%u bits)",
					 chiper_info->version, chiper_info->chiper,
					 chiper_info->chiper_bits);
		EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL, NULL,
						 0);

		verify_error = SSL_get_verify_result (serv->ssl);
		switch (verify_error)
		{
		case X509_V_OK:
			/* snprintf (buf, sizeof (buf), "* Verify OK (?)"); */
			/* EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL, NULL, 0); */
			break;
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
		case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
		case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
		case X509_V_ERR_CERT_HAS_EXPIRED:
			if (serv->accept_invalid_cert)
			{
				snprintf (buf, sizeof (buf), "* Verify E: %s.? (%d) -- Ignored",
							 X509_verify_cert_error_string (verify_error),
							 verify_error);
				EMIT_SIGNAL (XP_TE_SSLMESSAGE, serv->server_session, buf, NULL, NULL,
								 NULL, 0);
				break;
			}
		default:
			snprintf (buf, sizeof (buf), "%s.? (%d)",
						 X509_verify_cert_error_string (verify_error),
						 verify_error);
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
			snprintf (buf, sizeof (buf), "SSL handshake timed out");
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
		serv->ssl = _SSL_socket (ctx, serv->sok);
		if ((err = _SSL_set_verify (ctx, ssl_cb_verify, NULL)))
		{
			EMIT_SIGNAL (XP_TE_CONNFAIL, serv->server_session, err, NULL,
							 NULL, NULL, 0);
			server_cleanup (serv);	/* ->connecting = FALSE */
			return;
		}
		/* FIXME: it'll be needed by new servers */
		/* send(serv->sok, "STLS\r\n", 6, 0); sleep(1); */
		set_nonblocking (serv->sok);
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

/* receive info from the child-process about connection progress */

static gboolean
server_read_child (GIOChannel *source, GIOCondition condition, server *serv)
{
	session *sess = serv->server_session;
	char tbuf[128];
	char outbuf[512];
	char host[100];
	char ip[100];
#ifdef USE_MSPROXY
	char *p;
#endif

	waitline2 (source, tbuf, sizeof tbuf);

	switch (tbuf[0])
	{
	case '0':	/* print some text */
		waitline2 (source, tbuf, sizeof tbuf);
		PrintText (serv->server_session, tbuf);
		break;
	case '1':						  /* unknown host */
		server_stopconnecting (serv);
		closesocket (serv->sok4);
		if (serv->proxy_sok4 != -1)
			closesocket (serv->proxy_sok4);
#ifdef USE_IPV6
		if (serv->sok6 != -1)
			closesocket (serv->sok6);
		if (serv->proxy_sok6 != -1)
			closesocket (serv->proxy_sok6);
#endif
		EMIT_SIGNAL (XP_TE_UKNHOST, sess, NULL, NULL, NULL, NULL, 0);
		if (!servlist_cycle (serv))
			if (prefs.hex_net_auto_reconnectonfail)
				auto_reconnect (serv, FALSE, -1);
		break;
	case '2':						  /* connection failed */
		waitline2 (source, tbuf, sizeof tbuf);
		server_stopconnecting (serv);
		closesocket (serv->sok4);
		if (serv->proxy_sok4 != -1)
			closesocket (serv->proxy_sok4);
#ifdef USE_IPV6
		if (serv->sok6 != -1)
			closesocket (serv->sok6);
		if (serv->proxy_sok6 != -1)
			closesocket (serv->proxy_sok6);
#endif
		EMIT_SIGNAL (XP_TE_CONNFAIL, sess, errorstring (atoi (tbuf)), NULL,
						 NULL, NULL, 0);
		if (!servlist_cycle (serv))
			if (prefs.hex_net_auto_reconnectonfail)
				auto_reconnect (serv, FALSE, -1);
		break;
	case '3':						  /* gethostbyname finished */
		waitline2 (source, host, sizeof host);
		waitline2 (source, ip, sizeof ip);
		waitline2 (source, outbuf, sizeof outbuf);
		EMIT_SIGNAL (XP_TE_CONNECT, sess, host, ip, outbuf, NULL, 0);
#ifdef WIN32
		if (prefs.hex_identd)
		{
			if (serv->network && ((ircnet *)serv->network)->user)
			{
				identd_start (((ircnet *)serv->network)->user);
			}
			else
			{
				identd_start (prefs.hex_irc_user_name);
			}
		}
#else
		snprintf (outbuf, sizeof (outbuf), "%s/auth/xchat_auth",
					 g_get_home_dir ());
		if (access (outbuf, X_OK) == 0)
		{
			snprintf (outbuf, sizeof (outbuf), "exec -d %s/auth/xchat_auth %s",
						 g_get_home_dir (), prefs.hex_irc_user_name);
			handle_command (serv->server_session, outbuf, FALSE);
		}
#endif
		break;
	case '4':						  /* success */
		waitline2 (source, tbuf, sizeof (tbuf));
#ifdef USE_MSPROXY
		serv->sok = strtol (tbuf, &p, 10);
		if (*p++ == ' ')
		{
			serv->proxy_sok = strtol (p, &p, 10);
			serv->msp_state.clientid = strtol (++p, &p, 10);
			serv->msp_state.serverid = strtol (++p, &p, 10);
			serv->msp_state.seq_sent = atoi (++p);
		} else
			serv->proxy_sok = -1;
#ifdef DEBUG_MSPROXY
		printf ("Parent got main socket: %d, proxy socket: %d\n", serv->sok, serv->proxy_sok);
		printf ("Client ID 0x%08x server ID 0x%08x seq_sent %d\n", serv->msp_state.clientid, serv->msp_state.serverid, serv->msp_state.seq_sent);
#endif
#else
		serv->sok = atoi (tbuf);
#endif
#ifdef USE_IPV6
		/* close the one we didn't end up using */
		if (serv->sok == serv->sok4)
			closesocket (serv->sok6);
		else
			closesocket (serv->sok4);
		if (serv->proxy_sok != -1)
		{
			if (serv->proxy_sok == serv->proxy_sok4)
				closesocket (serv->proxy_sok6);
			else
				closesocket (serv->proxy_sok4);
		}
#endif
		server_connect_success (serv);
		break;
	case '5':						  /* prefs ip discovered */
		waitline2 (source, tbuf, sizeof tbuf);
		prefs.local_ip = inet_addr (tbuf);
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
	case '9':
		waitline2 (source, tbuf, sizeof tbuf);
		EMIT_SIGNAL (XP_TE_SERVERLOOKUP, sess, tbuf, NULL, NULL, NULL, 0);
		break;
	}

	return TRUE;
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
		closesocket (serv->sok4);
		if (serv->proxy_sok4 != -1)
			closesocket (serv->proxy_sok4);
		if (serv->sok6 != -1)
			closesocket (serv->sok6);
		if (serv->proxy_sok6 != -1)
			closesocket (serv->proxy_sok6);
		return 1;
	}

	if (serv->connected)
	{
		close_socket (serv->sok);
		if (serv->proxy_sok)
			close_socket (serv->proxy_sok);
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
	char tbuf[64];
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
		sprintf (tbuf, "%d", sess->server->childpid);
		EMIT_SIGNAL (XP_TE_STOPCONNECT, sess, tbuf, NULL, NULL, NULL, 0);
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

/* send a "print text" command to the parent process - MUST END IN \n! */

static void
proxy_error (int fd, char *msg)
{
	write (fd, "0\n", 2);
	write (fd, msg, strlen (msg));
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
traverse_socks (int print_fd, int sok, char *serverAddr, int port)
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

	snprintf (buf, sizeof (buf), "SOCKS\tServer reported error %d,%d.\n", buf[0], buf[1]);
	proxy_error (print_fd, buf);
	return 1;
}

struct sock5_connect1
{
	char version;
	char nmethods;
	char method;
};

static int
traverse_socks5 (int print_fd, int sok, char *serverAddr, int port)
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
		proxy_error (print_fd, "SOCKS\tServer is not socks version 5.\n");
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
			proxy_error (print_fd, "SOCKS\tServer doesn't support UPA authentication.\n");
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
			proxy_error (print_fd, "SOCKS\tAuthentication failed. "
							 "Is username and password correct?\n");
			return 1; /* UPA failed! */
		}
	}
	else
	{
		if (buf[1] != 0)
		{
			proxy_error (print_fd, "SOCKS\tAuthentication required but disabled in settings.\n");
			return 1;
		}
	}

	addrlen = strlen (serverAddr);
	packetlen = 4 + 1 + addrlen + 2;
	sc2 = malloc (packetlen);
	sc2[0] = 5;						  /* version */
	sc2[1] = 1;						  /* command */
	sc2[2] = 0;						  /* reserved */
	sc2[3] = 3;						  /* address type */
	sc2[4] = (unsigned char) addrlen;	/* hostname length */
	memcpy (sc2 + 5, serverAddr, addrlen);
	*((unsigned short *) (sc2 + 5 + addrlen)) = htons (port);
	send (sok, sc2, packetlen, 0);
	free (sc2);

	/* consume all of the reply */
	if (recv (sok, buf, 4, 0) != 4)
		goto read_error;
	if (buf[0] != 5 || buf[1] != 0)
	{
		if (buf[1] == 2)
			snprintf (buf, sizeof (buf), "SOCKS\tProxy refused to connect to host (not allowed).\n");
		else
			snprintf (buf, sizeof (buf), "SOCKS\tProxy failed to connect to host (error %d).\n", buf[1]);
		proxy_error (print_fd, buf);
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
	proxy_error (print_fd, "SOCKS\tRead error from server.\n");
	return 1;
}

static int
traverse_wingate (int print_fd, int sok, char *serverAddr, int port)
{
	char buf[128];

	snprintf (buf, sizeof (buf), "%s %d\r\n", serverAddr, port);
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
http_read_line (int print_fd, int sok, char *buf, int len)
{
	len = waitline (sok, buf, len, TRUE);
	if (len >= 1)
	{
		/* print the message out (send it to the parent process) */
		write (print_fd, "0\n", 2);

		if (buf[len-1] == '\r')
		{
			buf[len-1] = '\n';
			write (print_fd, buf, len);
		} else
		{
			write (print_fd, buf, len);
			write (print_fd, "\n", 1);
		}
	}

	return len;
}

static int
traverse_http (int print_fd, int sok, char *serverAddr, int port)
{
	char buf[512];
	char auth_data[256];
	char auth_data2[252];
	int n, n2;

	n = snprintf (buf, sizeof (buf), "CONNECT %s:%d HTTP/1.0\r\n",
					  serverAddr, port);
	if (prefs.hex_net_proxy_auth)
	{
		n2 = snprintf (auth_data2, sizeof (auth_data2), "%s:%s",
							prefs.hex_net_proxy_user, prefs.hex_net_proxy_pass);
		base64_encode (auth_data, auth_data2, n2);
		n += snprintf (buf+n, sizeof (buf)-n, "Proxy-Authorization: Basic %s\r\n", auth_data);
	}
	n += snprintf (buf+n, sizeof (buf)-n, "\r\n");
	send (sok, buf, n, 0);

	n = http_read_line (print_fd, sok, buf, sizeof (buf));
	/* "HTTP/1.0 200 OK" */
	if (n < 12)
		return 1;
	if (memcmp (buf, "HTTP/", 5) || memcmp (buf + 9, "200", 3))
		return 1;
	while (1)
	{
		/* read until blank line */
		n = http_read_line (print_fd, sok, buf, sizeof (buf));
		if (n < 1 || (n == 1 && buf[0] == '\n'))
			break;
	}
	return 0;
}

static int
traverse_proxy (int proxy_type, int print_fd, int sok, char *ip, int port, struct msproxy_state_t *state, netstore *ns_proxy, int csok4, int csok6, int *csok, char bound)
{
	switch (proxy_type)
	{
	case 1:
		return traverse_wingate (print_fd, sok, ip, port);
	case 2:
		return traverse_socks (print_fd, sok, ip, port);
	case 3:
		return traverse_socks5 (print_fd, sok, ip, port);
	case 4:
		return traverse_http (print_fd, sok, ip, port);
#ifdef USE_MSPROXY
	case 5:
		return traverse_msproxy (sok, ip, port, state, ns_proxy, csok4, csok6, csok, bound);
#endif
	}

	return 1;
}

/* this is the child process making the connection attempt */

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

	/* is a hostname set? - bind to it */
	if (prefs.hex_net_bind_host[0])
	{
		ns_local = net_store_new ();
		local_ip = net_resolve (ns_local, prefs.hex_net_bind_host, 0, &real_hostname);
		if (local_ip != NULL)
		{
			snprintf (buf, sizeof (buf), "5\n%s\n", local_ip);
			write (serv->childwrite, buf, strlen (buf));
			net_bind (ns_local, serv->sok4, serv->sok6);
			bound = 1;
		} else
		{
			write (serv->childwrite, "7\n", 2);
		}
		net_store_destroy (ns_local);
	}

	if (!serv->dont_use_proxy) /* blocked in serverlist? */
	{
		if (FALSE)
			;
#ifdef USE_LIBPROXY
		else if (prefs.hex_net_proxy_type == 5)
		{
			char **proxy_list;
			char *url, *proxy;

			url = g_strdup_printf ("irc://%s:%d", hostname, port);
			proxy_list = px_proxy_factory_get_proxies (libproxy_factory, url);

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
			}

			if (proxy_type) {
				char *c;
				c = strchr (proxy, ':') + 3;
				proxy_host = strdup (c);
				c = strchr (proxy_host, ':');
				*c = '\0';
				proxy_port = atoi (c + 1);
			}

			g_strfreev (proxy_list);
			g_free (url);
		}
#endif
		else if (prefs.hex_net_proxy_host[0] &&
			   prefs.hex_net_proxy_type > 0 &&
			   prefs.hex_net_proxy_use != 2) /* proxy is NOT dcc-only */
		{
			proxy_type = prefs.hex_net_proxy_type;
			proxy_host = strdup (prefs.hex_net_proxy_host);
			proxy_port = prefs.hex_net_proxy_port;
		}
	}

	serv->proxy_type = proxy_type;

	/* first resolve where we want to connect to */
	if (proxy_type > 0)
	{
		snprintf (buf, sizeof (buf), "9\n%s\n", proxy_host);
		write (serv->childwrite, buf, strlen (buf));
		ip = net_resolve (ns_server, proxy_host, proxy_port, &real_hostname);
		free (proxy_host);
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
			proxy_ip = strdup (hostname);
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

	snprintf (buf, sizeof (buf), "3\n%s\n%s\n%d\n",
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
		snprintf (buf, sizeof (buf), "2\n%d\n", sock_error ());
		write (serv->childwrite, buf, strlen (buf));
	} else
	{
		/* connect succeeded */
		if (proxy_ip)
		{
			switch (traverse_proxy (proxy_type, serv->childwrite, psok, proxy_ip, port, &serv->msp_state, ns_proxy, serv->sok4, serv->sok6, &sok, bound))
			{
			case 0:	/* success */
#ifdef USE_MSPROXY
				if (!serv->dont_use_proxy && (proxy_type == 5))
					snprintf (buf, sizeof (buf), "4\n%d %d %d %d %d\n", sok, psok, serv->msp_state.clientid, serv->msp_state.serverid,
						serv->msp_state.seq_sent);
				else
#endif
					snprintf (buf, sizeof (buf), "4\n%d\n", sok);	/* success */
				write (serv->childwrite, buf, strlen (buf));
				break;
			case 1:	/* socks traversal failed */
				write (serv->childwrite, "8\n", 2);
				break;
			}
		} else
		{
			snprintf (buf, sizeof (buf), "4\n%d\n", sok);	/* success */
			write (serv->childwrite, buf, strlen (buf));
		}
	}

xit:

#if defined (USE_IPV6) || defined (WIN32)
	/* this is probably not needed */
	net_store_destroy (ns_server);
	if (ns_proxy)
		net_store_destroy (ns_proxy);
#endif

	/* no need to free ip/real_hostname, this process is exiting */
#ifdef WIN32
	/* under win32 we use a thread -> shared memory, must free! */
	if (proxy_ip)
		free (proxy_ip);
	if (ip)
		free (ip);
	if (real_hostname)
		free (real_hostname);
#endif

	return 0;
	/* cppcheck-suppress memleak */
}

static void
server_connect (server *serv, char *hostname, int port, int no_login)
{
	int pid, read_des[2];
	session *sess = serv->server_session;

#ifdef USE_OPENSSL
	if (!ctx && serv->use_ssl)
	{
		if (!(ctx = _SSL_context_init (ssl_cb_info, FALSE)))
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
		if (SSL_CTX_use_certificate_file (ctx, cert_file, SSL_FILETYPE_PEM) == 1)
		{
			if (SSL_CTX_use_PrivateKey_file (ctx, cert_file, SSL_FILETYPE_PEM) == 1)
				serv->have_cert = TRUE;
		}
		else
		{
			/* if that doesn't exist, try <config>/certs/client.pem */
			cert_file = g_build_filename (get_xdir (), "certs", "client.pem", NULL);
			if (SSL_CTX_use_certificate_file (ctx, cert_file, SSL_FILETYPE_PEM) == 1)
			{
				if (SSL_CTX_use_PrivateKey_file (ctx, cert_file, SSL_FILETYPE_PEM) == 1)
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

#ifdef WIN32
	if (_pipe (read_des, 4096, _O_BINARY) < 0)
#else
	if (pipe (read_des) < 0)
#endif
		return;
#ifdef __EMX__ /* os/2 */
	setmode (read_des[0], O_BINARY);
	setmode (read_des[1], O_BINARY);
#endif
	serv->childread = read_des[0];
	serv->childwrite = read_des[1];

	/* create both sockets now, drop one later */
	net_sockets (&serv->sok4, &serv->sok6);
#ifdef USE_MSPROXY
	/* In case of MS Proxy we have a separate UDP control connection */
	if (!serv->dont_use_proxy && (serv->proxy_type == 5))
		udp_sockets (&serv->proxy_sok4, &serv->proxy_sok6);
	else
#endif
	{
		serv->proxy_sok4 = -1;
		serv->proxy_sok6 = -1;
	}

#ifdef WIN32
	CloseHandle (CreateThread (NULL, 0,
										(LPTHREAD_START_ROUTINE)server_child,
										serv, 0, (DWORD *)&pid));
#else
#ifdef LOOKUPD
	/* CL: net_resolve calls rand() when LOOKUPD is set, so prepare a different
	 * seed for each child. This method gives a bigger variation in seed values
	 * than calling srand(time(0)) in the child itself.
	 */
	rand();
#endif
	switch (pid = fork ())
	{
	case -1:
		return;

	case 0:
		/* this is the child */
		setuid (getuid ());
		server_child (serv);
		_exit (0);
	}
#endif
	serv->childpid = pid;
#ifdef WIN32
	serv->iotag = fe_input_add (serv->childread, FIA_READ|FIA_FD, server_read_child,
#else
	serv->iotag = fe_input_add (serv->childread, FIA_READ, server_read_child,
#endif
										 serv);
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
		free (serv->encoding);
		/* can be left as NULL to indicate system encoding */
		serv->encoding = NULL;
		serv->using_cp1255 = FALSE;
		serv->using_irc = FALSE;
	}

	if (new_encoding)
	{
		serv->encoding = strdup (new_encoding);
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

	serv = malloc (sizeof (struct server));
	memset (serv, 0, sizeof (struct server));

	/* use server.c and proto-irc.c functions */
	server_fill_her_up (serv);

	serv->id = id++;
	serv->sok = -1;
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
	if (serv->chantypes)
		free (serv->chantypes);
	if (serv->chanmodes)
		free (serv->chanmodes);
	if (serv->nick_prefixes)
		free (serv->nick_prefixes);
	if (serv->nick_modes)
		free (serv->nick_modes);

	serv->chantypes = strdup ("#&!+");
	serv->chanmodes = strdup ("beI,k,l");
	serv->nick_prefixes = strdup ("@%+");
	serv->nick_modes = strdup ("ohv");

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
			if (away->message)
				free (away->message);
			free (away);
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
		if (away->message)
			free (away->message);
		away->message = strdup (msg);
	} else
		/* Create brand new entry */
	{
		away = malloc (sizeof (struct away_msg));
		if (away)
		{
			away->server = serv;
			safe_strcpy (away->nick, nick, sizeof (away->nick));
			away->message = strdup (msg);
			away_list = g_slist_prepend (away_list, away);
		}
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

	free (serv->nick_modes);
	free (serv->nick_prefixes);
	free (serv->chanmodes);
	free (serv->chantypes);
	if (serv->bad_nick_prefixes)
		free (serv->bad_nick_prefixes);
	if (serv->last_away_reason)
		free (serv->last_away_reason);
	if (serv->encoding)
		free (serv->encoding);
	if (serv->favlist)
		g_slist_free_full (serv->favlist, (GDestroyNotify) servlist_favchan_free);

	fe_server_callback (serv);

	free (serv);

	notify_cleanup ();
}
