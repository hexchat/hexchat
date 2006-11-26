/* X-Chat
 * Copyright (C) 1998-2006 Peter Zelezny.
 * Copyright (C) 2006 Damjan Jovanovic
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
 *
 * Wayne Conrad, 3 Apr 1999: Color-coded DCC file transfer status windows
 * Bernhard Valenti <bernhard.valenti@gmx.net> 2000-11-21: Fixed DCC send behind nat
 *
 * 2001-03-08 Added support for getting "dcc_ip" config parameter.
 * Jim Seymour (jseymour@LinxNet.com)
 */

/* we only use 32 bits, but without this define, you get only 31! */
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define WANTSOCKET
#define WANTARPA
#define WANTDNS
#include "inet.h"

#ifdef WIN32
#include <windows.h>
#endif

#include "xchat.h"
#include "util.h"
#include "fe.h"
#include "outbound.h"
#include "inbound.h"
#include "network.h"
#include "plugin.h"
#include "server.h"
#include "text.h"
#include "url.h"
#include "xchatc.h"

#ifdef USE_DCC64
#define BIG_STR_TO_INT(x) strtoull(x,NULL,10)
#else
#define BIG_STR_TO_INT(x) strtoul(x,NULL,10)
#endif

static char *dcctypes[] = { "SEND", "RECV", "CHAT", "CHAT" };

struct dccstat_info dccstat[] = {
	{N_("Waiting"), 1 /*black */ },
	{N_("Active"), 12 /*cyan */ },
	{N_("Failed"), 4 /*red */ },
	{N_("Done"), 3 /*green */ },
	{N_("Connect"), 1 /*black */ },
	{N_("Aborted"), 4 /*red */ },
};

static int dcc_global_throttle;	/* 0x1 = sends, 0x2 = gets */
/*static*/ int dcc_sendcpssum, dcc_getcpssum;

static struct DCC *new_dcc (void);
static void dcc_close (struct DCC *dcc, int dccstat, int destroy);
static gboolean dcc_send_data (GIOChannel *, GIOCondition, struct DCC *);
static gboolean dcc_read (GIOChannel *, GIOCondition, struct DCC *);
static gboolean dcc_read_ack (GIOChannel *source, GIOCondition condition, struct DCC *dcc);

static int new_id()
{
	static int id = 0;
	if (id == 0)
	{
		/* start the first ID at a random number for pseudo security */
		/* 1 - 255 */
		id = RAND_INT(255) + 1;
		/* ignore overflows, since it can go to 2 billion */
	}
	return id++;
}

static double
timeval_diff (GTimeVal *greater,
				 GTimeVal *less)
{
	long usecdiff;
	double result;
	
	result = greater->tv_sec - less->tv_sec;
	usecdiff = (long) greater->tv_usec - less->tv_usec;
	result += (double) usecdiff / 1000000;
	
	return result;
}

static void
dcc_unthrottle (struct DCC *dcc)
{
	/* don't unthrottle here, but delegate to funcs */
	if (dcc->type == TYPE_RECV)
		dcc_read (NULL, 0, dcc);
	else
		dcc_send_data (NULL, 0, dcc);
}

static void
dcc_calc_cps (struct DCC *dcc)
{
	GTimeVal now;
	int oldcps;
	double timediff, startdiff;
	int glob_throttle_bit, wasthrottled;
	int *cpssum, glob_limit;
	DCC_SIZE pos, posdiff;

	g_get_current_time (&now);

	/* the pos we use for sends is an average
		between pos and ack */
	if (dcc->type == TYPE_SEND)
	{
		/* carefull to avoid 32bit overflow */
		pos = dcc->pos - ((dcc->pos - dcc->ack) / 2);
		glob_throttle_bit = 0x1;
		cpssum = &dcc_sendcpssum;
		glob_limit = prefs.dcc_global_max_send_cps;
	}
	else
	{
		pos = dcc->pos;
		glob_throttle_bit = 0x2;
		cpssum = &dcc_getcpssum;
		glob_limit = prefs.dcc_global_max_get_cps;
	}

	if (!dcc->firstcpstv.tv_sec && !dcc->firstcpstv.tv_usec)
		dcc->firstcpstv = now;
	else
	{
		startdiff = timeval_diff (&now, &dcc->firstcpstv);
		if (startdiff < 1)
			startdiff = 1;
		else if (startdiff > CPS_AVG_WINDOW)
			startdiff = CPS_AVG_WINDOW;

		timediff = timeval_diff (&now, &dcc->lastcpstv);
		if (timediff > startdiff)
			timediff = startdiff = 1;

		posdiff = pos - dcc->lastcpspos;
		oldcps = dcc->cps;
		dcc->cps = ((double) posdiff / timediff) * (timediff / startdiff) +
			(double) dcc->cps * (1.0 - (timediff / startdiff));

		*cpssum += dcc->cps - oldcps;
	}

	dcc->lastcpspos = pos;
	dcc->lastcpstv = now;

	/* now check cps against set limits... */
	wasthrottled = dcc->throttled;

	/* check global limits first */
	dcc->throttled &= ~0x2;
	if (glob_limit > 0 && *cpssum >= glob_limit)
	{
		dcc_global_throttle |= glob_throttle_bit;
		if (dcc->maxcps >= 0)
			dcc->throttled |= 0x2;
	}
	else
		dcc_global_throttle &= ~glob_throttle_bit;

	/* now check per-connection limit */
	if (dcc->maxcps > 0 && dcc->cps > dcc->maxcps)
		dcc->throttled |= 0x1;
	else
		dcc->throttled &= ~0x1;

	/* take action */
	if (wasthrottled && !dcc->throttled)
		dcc_unthrottle (dcc);
}

static void
dcc_remove_from_sum (struct DCC *dcc)
{
	if (dcc->dccstat != STAT_ACTIVE)
		return;
	if (dcc->type == TYPE_SEND)
		dcc_sendcpssum -= dcc->cps;
	else if (dcc->type == TYPE_RECV)
		dcc_getcpssum -= dcc->cps;
}

gboolean
is_dcc (struct DCC *dcc)
{
	GSList *list = dcc_list;
	while (list)
	{
		if (list->data == dcc)
			return TRUE;
		list = list->next;
	}
	return FALSE;
}

/* this is called from xchat.c:xchat_misc_checks() every 1 second. */

void
dcc_check_timeouts (void)
{
	struct DCC *dcc;
	time_t tim = time (0);
	GSList *next, *list = dcc_list;

	while (list)
	{
		dcc = (struct DCC *) list->data;
		next = list->next;

		switch (dcc->dccstat)
		{
		case STAT_ACTIVE:
			dcc_calc_cps (dcc);
			fe_dcc_update (dcc);

			if (dcc->type == TYPE_SEND || dcc->type == TYPE_RECV)
			{
				if (prefs.dccstalltimeout > 0)
				{
					if (!dcc->throttled
						&& tim - dcc->lasttime > prefs.dccstalltimeout)
					{
						EMIT_SIGNAL (XP_TE_DCCSTALL, dcc->serv->front_session,
										 dcctypes[dcc->type],
										 file_part (dcc->file), dcc->nick, NULL, 0);
						dcc_close (dcc, STAT_ABORTED, FALSE);
					}
				}
			}
			break;
		case STAT_QUEUED:
			if (dcc->type == TYPE_SEND || dcc->type == TYPE_CHATSEND)
			{
				if (tim - dcc->offertime > prefs.dcctimeout)
				{
					if (prefs.dcctimeout > 0)
					{
						EMIT_SIGNAL (XP_TE_DCCTOUT, dcc->serv->front_session,
										 dcctypes[dcc->type],
										 file_part (dcc->file), dcc->nick, NULL, 0);
						dcc_close (dcc, STAT_ABORTED, FALSE);
					}
				}
			}
			break;
		case STAT_DONE:
		case STAT_FAILED:
		case STAT_ABORTED:
			if (prefs.dcc_remove)
				dcc_close (dcc, 0, TRUE);
			break;
		}
		list = next;
	}
}

static int
dcc_lookup_proxy (char *host, struct sockaddr_in *addr)
{
	struct hostent *h;
	static char *cache_host = NULL;
	static guint32 cache_addr;

	/* too lazy to thread this, so we cache results */
	if (cache_host)
	{
		if (strcmp (host, cache_host) == 0)
		{
			memcpy (&addr->sin_addr, &cache_addr, 4);
			return TRUE;
		}
		free (cache_host);
		cache_host = NULL;
	}

	h = gethostbyname (host);
	if (h != NULL && h->h_length == 4 && h->h_addr_list[0] != NULL)
	{
		memcpy (&addr->sin_addr, h->h_addr, 4);
		memcpy (&cache_addr, h->h_addr, 4);
		cache_host = strdup (host);
		return TRUE;
	}

	return FALSE;
}

#define DCC_USE_PROXY() (prefs.proxy_host[0] && prefs.proxy_type>0 && prefs.proxy_type<5 && prefs.proxy_use!=1)

static int
dcc_connect_sok (struct DCC *dcc)
{
	int sok;
	struct sockaddr_in addr;

	sok = socket (AF_INET, SOCK_STREAM, 0);
	if (sok == -1)
		return -1;

	memset (&addr, 0, sizeof (addr));
	addr.sin_family = AF_INET;
	if (DCC_USE_PROXY ())
	{
		if (!dcc_lookup_proxy (prefs.proxy_host, &addr))
		{
			closesocket (sok);
			return -1;
		}
		addr.sin_port = htons (prefs.proxy_port);
	}
	else
	{
		addr.sin_port = htons (dcc->port);
		addr.sin_addr.s_addr = htonl (dcc->addr);
	}

	set_nonblocking (sok);
	connect (sok, (struct sockaddr *) &addr, sizeof (addr));

	return sok;
}

static void
dcc_close (struct DCC *dcc, int dccstat, int destroy)
{
	if (dcc->wiotag)
	{
		fe_input_remove (dcc->wiotag);
		dcc->wiotag = 0;
	}

	if (dcc->iotag)
	{
		fe_input_remove (dcc->iotag);
		dcc->iotag = 0;
	}

	if (dcc->sok != -1)
	{
		closesocket (dcc->sok);
		dcc->sok = -1;
	}

	dcc_remove_from_sum (dcc);

	if (dcc->fp != -1)
	{
		close (dcc->fp);
		dcc->fp = -1;

		if(dccstat == STAT_DONE)
		{
			/* if we just completed a dcc recieve, move the */
			/* completed file to the completed directory */
			if(dcc->type == TYPE_RECV)
			{			
				/* mgl: change this to use destfile_fs for correctness and to */
				/* handle the case where dccwithnick is set */
				move_file_utf8 (prefs.dccdir, prefs.dcc_completed_dir, 
									 file_part (dcc->destfile), prefs.dccpermissions);
			}

		}
	}

	dcc->dccstat = dccstat;
	if (dcc->dccchat)
	{
		free (dcc->dccchat);
		dcc->dccchat = NULL;
	}

	if (destroy)
	{
		dcc_list = g_slist_remove (dcc_list, dcc);
		fe_dcc_remove (dcc);
		if (dcc->proxy)
			free (dcc->proxy);
		if (dcc->file)
			free (dcc->file);
		if (dcc->destfile)
			g_free (dcc->destfile);
		if (dcc->destfile_fs)
			g_free (dcc->destfile_fs);
		free (dcc->nick);
		free (dcc);
		return;
	}

	fe_dcc_update (dcc);
}

void
dcc_abort (session *sess, struct DCC *dcc)
{
	if (dcc)
	{
		switch (dcc->dccstat)
		{
		case STAT_QUEUED:
		case STAT_CONNECTING:
		case STAT_ACTIVE:
			dcc_close (dcc, STAT_ABORTED, FALSE);
			switch (dcc->type)
			{
			case TYPE_CHATSEND:
			case TYPE_CHATRECV:
				EMIT_SIGNAL (XP_TE_DCCCHATABORT, sess, dcc->nick, NULL, NULL,
								 NULL, 0);
				break;
			case TYPE_SEND:
				EMIT_SIGNAL (XP_TE_DCCSENDABORT, sess, dcc->nick,
								 file_part (dcc->file), NULL, NULL, 0);
				break;
			case TYPE_RECV:
				EMIT_SIGNAL (XP_TE_DCCRECVABORT, sess, dcc->nick,
								 dcc->file, NULL, NULL, 0);
			}
			break;
		default:
			dcc_close (dcc, 0, TRUE);
		}
	}
}

void
dcc_notify_kill (struct server *serv)
{
	struct server *replaceserv = 0;
	struct DCC *dcc;
	GSList *list = dcc_list;
	if (serv_list)
		replaceserv = (struct server *) serv_list->data;
	while (list)
	{
		dcc = (struct DCC *) list->data;
		if (dcc->serv == serv)
			dcc->serv = replaceserv;
		list = list->next;
	}
}

struct DCC *
dcc_write_chat (char *nick, char *text)
{
	struct DCC *dcc;

	dcc = find_dcc (nick, "", TYPE_CHATRECV);
	if (!dcc)
		dcc = find_dcc (nick, "", TYPE_CHATSEND);
	if (dcc && dcc->dccstat == STAT_ACTIVE)
	{
		char *locale;
		gsize loc_len;
		int len;

		len = strlen (text);

		if (dcc->serv->encoding == NULL)	/* system */
		{
			locale = NULL;
			if (!prefs.utf8_locale)
				locale = g_locale_from_utf8 (text, len, NULL, &loc_len, NULL);
		} else
		{
			if (dcc->serv->using_irc)	/* using "IRC" encoding (CP1252/UTF-8 hybrid) */
				locale = g_convert (text, len, "CP1252", "UTF-8", 0, &loc_len, 0);
			else
				locale = g_convert (text, len, dcc->serv->encoding, "UTF-8", 0, &loc_len, 0);
		}

		if (locale)
		{
			text = locale;
			len = loc_len;
		}

		dcc->size += len;
		send (dcc->sok, text, len, 0);
		send (dcc->sok, "\n", 1, 0);
		fe_dcc_update (dcc);
		if (locale)
			g_free (locale);
		return dcc;
	}
	return 0;
}

/* returns: 0 - ok
				1 - the dcc is closed! */

static int
dcc_chat_line (struct DCC *dcc, char *line)
{
	session *sess;
	char *word[PDIWORDS];
	char *po;
	char *utf;
	char *conv;
	int ret, i;
	int len;
	gsize utf_len;
	char portbuf[32];

	len = strlen (line);
	if (dcc->serv->using_cp1255)
		len++;	/* include the NUL terminator */

	if (dcc->serv->using_irc) /* using "IRC" encoding (CP1252/UTF-8 hybrid) */
		utf = NULL;
	else if (dcc->serv->encoding == NULL)     /* system */
		utf = g_locale_to_utf8 (line, len, NULL, &utf_len, NULL);
	else
		utf = g_convert (line, len, "UTF-8", dcc->serv->encoding, 0, &utf_len, 0);

	if (utf)
	{
		line = utf;
		len = utf_len;
	}

	if (dcc->serv->using_cp1255 && len > 0)
		len--;

	/* we really need valid UTF-8 now */
	conv = text_validate (&line, &len);

	sess = find_dialog (dcc->serv, dcc->nick);
	if (!sess)
		sess = dcc->serv->front_session;

	sprintf (portbuf, "%d", dcc->port);

	word[0] = "DCC Chat Text";
	word[1] = net_ip (dcc->addr);
	word[2] = portbuf;
	word[3] = dcc->nick;
	word[4] = line;
	for (i = 5; i < PDIWORDS; i++)
		word[i] = "\000";

	ret = plugin_emit_print (sess, word);

	/* did the plugin close it? */
	if (!g_slist_find (dcc_list, dcc))
	{
		if (utf)
			g_free (utf);
		if (conv)
			g_free (conv);
		return 1;
	}

	/* did the plugin eat the event? */
	if (ret)
	{
		if (utf)
			g_free (utf);
		if (conv)
			g_free (conv);
		return 0;
	}

	url_check_line (line, len);

	if (line[0] == 1 && !strncasecmp (line + 1, "ACTION", 6))
	{
		po = strchr (line + 8, '\001');
		if (po)
			po[0] = 0;
		inbound_action (sess, dcc->serv->nick, dcc->nick, line + 8, FALSE, FALSE);
	} else
	{
		inbound_privmsg (dcc->serv, dcc->nick, "", line, FALSE);
	}
	if (utf)
		g_free (utf);
	if (conv)
		g_free (conv);
	return 0;
}

static gboolean
dcc_read_chat (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	int i, len, dead;
	char portbuf[32];
	char lbuf[2050];

	while (1)
	{
		if (dcc->throttled)
		{
			fe_input_remove (dcc->iotag);
			dcc->iotag = 0;
			return FALSE;
		}

		if (!dcc->iotag)
			dcc->iotag = fe_input_add (dcc->sok, FIA_READ|FIA_EX, dcc_read_chat, dcc);

		len = recv (dcc->sok, lbuf, sizeof (lbuf) - 2, 0);
		if (len < 1)
		{
			if (len < 0)
			{
				if (would_block ())
					return TRUE;
			}
			sprintf (portbuf, "%d", dcc->port);
			EMIT_SIGNAL (XP_TE_DCCCHATF, dcc->serv->front_session, dcc->nick,
							 net_ip (dcc->addr), portbuf,
							 errorstring ((len < 0) ? sock_error () : 0), 0);
			dcc_close (dcc, STAT_FAILED, FALSE);
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
				dcc->dccchat->linebuf[dcc->dccchat->pos] = 0;
				dead = dcc_chat_line (dcc, dcc->dccchat->linebuf);

				if (dead || !dcc->dccchat) /* the dcc has been closed, don't use (DCC *)! */
					return TRUE;

				dcc->pos += dcc->dccchat->pos;
				dcc->dccchat->pos = 0;
				fe_dcc_update (dcc);
				break;
			default:
				dcc->dccchat->linebuf[dcc->dccchat->pos] = lbuf[i];
				if (dcc->dccchat->pos < (sizeof (dcc->dccchat->linebuf) - 1))
					dcc->dccchat->pos++;
			}
			i++;
		}
	}
}

static void
dcc_calc_average_cps (struct DCC *dcc)
{
	time_t sec;

	sec = time (0) - dcc->starttime;
	if (sec < 1)
		sec = 1;
	if (dcc->type == TYPE_SEND)
		dcc->cps = (dcc->ack - dcc->resumable) / sec;
	else
		dcc->cps = (dcc->pos - dcc->resumable) / sec;
}

static void
dcc_send_ack (struct DCC *dcc)
{
	/* send in 32-bit big endian */
	guint32 pos = htonl (dcc->pos & 0xffffffff);
	send (dcc->sok, (char *) &pos, 4, 0);
}

static gboolean
dcc_read (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	char *old;
	char buf[4096];
	int n;
	gboolean need_ack = FALSE;

	if (dcc->fp == -1)
	{

		/* try to create the download dir (even if it exists, no harm) */
		mkdir_utf8 (prefs.dccdir);

		if (dcc->resumable)
		{
			dcc->fp = open (dcc->destfile_fs, O_WRONLY | O_APPEND | OFLAGS);
			dcc->pos = dcc->resumable;
			dcc->ack = dcc->resumable;
		} else
		{
			if (access (dcc->destfile_fs, F_OK) == 0)
			{
				n = 0;
				do
				{
					n++;
					snprintf (buf, sizeof (buf), "%s.%d", dcc->destfile_fs, n);
				}
				while (access (buf, F_OK) == 0);

				g_free (dcc->destfile_fs);
				dcc->destfile_fs = g_strdup (buf);

				old = dcc->destfile;
				dcc->destfile = xchat_filename_to_utf8 (buf, -1, 0, 0, 0);

				EMIT_SIGNAL (XP_TE_DCCRENAME, dcc->serv->front_session,
								 old, dcc->destfile, NULL, NULL, 0);
				g_free (old);
			}
			dcc->fp =
				open (dcc->destfile_fs, OFLAGS | O_TRUNC | O_WRONLY | O_CREAT,
						prefs.dccpermissions);
		}
	}
	if (dcc->fp == -1)
	{
		/* the last executed function is open(), errno should be valid */
		EMIT_SIGNAL (XP_TE_DCCFILEERR, dcc->serv->front_session, dcc->destfile,
						 errorstring (errno), NULL, NULL, 0);
		dcc_close (dcc, STAT_FAILED, FALSE);
		return TRUE;
	}
	while (1)
	{
		if (dcc->throttled)
		{
			if (need_ack)
				dcc_send_ack (dcc);

			fe_input_remove (dcc->iotag);
			dcc->iotag = 0;
			return FALSE;
		}

		if (!dcc->iotag)
			dcc->iotag = fe_input_add (dcc->sok, FIA_READ|FIA_EX, dcc_read, dcc);

		n = recv (dcc->sok, buf, sizeof (buf), 0);
		if (n < 1)
		{
			if (n < 0)
			{
				if (would_block ())
				{
					if (need_ack)
						dcc_send_ack (dcc);
					return TRUE;
				}
			}
			EMIT_SIGNAL (XP_TE_DCCRECVERR, dcc->serv->front_session, dcc->file,
							 dcc->destfile, dcc->nick,
							 errorstring ((n < 0) ? sock_error () : 0), 0);
			/* send ack here? but the socket is dead */
			/*if (need_ack)
				dcc_send_ack (dcc);*/
			dcc_close (dcc, STAT_FAILED, FALSE);
			return TRUE;
		}

		if (write (dcc->fp, buf, n) == -1) /* could be out of hdd space */
		{
			EMIT_SIGNAL (XP_TE_DCCRECVERR, dcc->serv->front_session, dcc->file,
							 dcc->destfile, dcc->nick, errorstring (errno), 0);
			if (need_ack)
				dcc_send_ack (dcc);
			dcc_close (dcc, STAT_FAILED, FALSE);
			return TRUE;
		}

		dcc->lasttime = time (0);
		dcc->pos += n;
		need_ack = TRUE;	/* send ack when we're done recv()ing */

		if (dcc->pos >= dcc->size)
		{
			dcc_send_ack (dcc);
			dcc_close (dcc, STAT_DONE, FALSE);
			dcc_calc_average_cps (dcc);	/* this must be done _after_ dcc_close, or dcc_remove_from_sum will see the wrong value in dcc->cps */
			sprintf (buf, "%d", dcc->cps);
			EMIT_SIGNAL (XP_TE_DCCRECVCOMP, dcc->serv->front_session,
							 dcc->file, dcc->destfile, dcc->nick, buf, 0);
			return TRUE;
		}
	}
}

static void
dcc_open_query (server *serv, char *nick)
{
	if (prefs.autodialog)
		open_query (serv, nick, FALSE);
}

static gboolean
dcc_did_connect (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	int er;
	struct sockaddr_in addr;
	
#ifdef WIN32
	if (condition & G_IO_ERR)
	{
		int len;

		/* find the last errno for this socket */
		len = sizeof (er);
		getsockopt (dcc->sok, SOL_SOCKET, SO_ERROR, (char *)&er, &len);
		EMIT_SIGNAL (XP_TE_DCCCONFAIL, dcc->serv->front_session,
						 dcctypes[dcc->type], dcc->nick, errorstring (er),
						 NULL, 0);
		dcc->dccstat = STAT_FAILED;
		fe_dcc_update (dcc);
		return FALSE;
	}

#else
	memset (&addr, 0, sizeof (addr));
	addr.sin_port = htons (dcc->port);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl (dcc->addr);

	/* check if it's already connected; This always fails on winXP */
	if (connect (dcc->sok, (struct sockaddr *) &addr, sizeof (addr)) != 0)
	{
		er = sock_error ();
		if (er != EISCONN)
		{
			EMIT_SIGNAL (XP_TE_DCCCONFAIL, dcc->serv->front_session,
							 dcctypes[dcc->type], dcc->nick, errorstring (er),
							 NULL, 0);
			dcc->dccstat = STAT_FAILED;
			fe_dcc_update (dcc);
			return FALSE;
		}
	}
#endif
	
	return TRUE;
}

static gboolean
dcc_connect_finished (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	char host[128];

	if (dcc->iotag)
	{
		fe_input_remove (dcc->iotag);
		dcc->iotag = 0;
	}

	if (!dcc_did_connect (source, condition, dcc))
		return TRUE;

	dcc->dccstat = STAT_ACTIVE;
	snprintf (host, sizeof host, "%s:%d", net_ip (dcc->addr), dcc->port);

	switch (dcc->type)
	{
	case TYPE_RECV:
		dcc->iotag = fe_input_add (dcc->sok, FIA_READ|FIA_EX, dcc_read, dcc);
		EMIT_SIGNAL (XP_TE_DCCCONRECV, dcc->serv->front_session,
						 dcc->nick, host, dcc->file, NULL, 0);
		break;
	case TYPE_SEND:
		/* passive send */
		dcc->fastsend = prefs.fastdccsend;
		if (dcc->fastsend)
			dcc->wiotag = fe_input_add (dcc->sok, FIA_WRITE, dcc_send_data, dcc);
		dcc->iotag = fe_input_add (dcc->sok, FIA_READ|FIA_EX, dcc_read_ack, dcc);
		dcc_send_data (NULL, 0, (gpointer)dcc);
		EMIT_SIGNAL (XP_TE_DCCCONSEND, dcc->serv->front_session,
						 dcc->nick, host, dcc->file, NULL, 0);
		break;
	case TYPE_CHATSEND:	/* pchat */
		dcc_open_query (dcc->serv, dcc->nick);
	case TYPE_CHATRECV:	/* normal chat */
		dcc->iotag = fe_input_add (dcc->sok, FIA_READ|FIA_EX, dcc_read_chat, dcc);
		dcc->dccchat = malloc (sizeof (struct dcc_chat));
		dcc->dccchat->pos = 0;
		EMIT_SIGNAL (XP_TE_DCCCONCHAT, dcc->serv->front_session,
						 dcc->nick, host, NULL, NULL, 0);
		break;
	}
	dcc->starttime = time (0);
	dcc->lasttime = dcc->starttime;
	fe_dcc_update (dcc);

	return TRUE;
}

static gboolean
read_proxy (struct DCC *dcc)
{
	struct proxy_state *proxy = dcc->proxy;
	while (proxy->bufferused < proxy->buffersize)
	{
		int ret = recv (dcc->sok, &proxy->buffer[proxy->bufferused],
						proxy->buffersize - proxy->bufferused, 0);
		if (ret > 0)
			proxy->bufferused += ret;
		else
		{
			if (would_block ())
				return FALSE;
			else
			{
				dcc->dccstat = STAT_FAILED;
				fe_dcc_update (dcc);
				if (dcc->iotag)
				{
					fe_input_remove (dcc->iotag);
					dcc->iotag = 0;
				}
				return FALSE;
			}
		}
	}
	return TRUE;
}

static gboolean
write_proxy (struct DCC *dcc)
{
	struct proxy_state *proxy = dcc->proxy;
	while (proxy->bufferused < proxy->buffersize)
	{
		int ret = send (dcc->sok, &proxy->buffer[proxy->bufferused],
						proxy->buffersize - proxy->bufferused, 0);
		if (ret >= 0)
			proxy->bufferused += ret;
		else
		{
			if (would_block ())
				return FALSE;
			else
			{
				dcc->dccstat = STAT_FAILED;
				fe_dcc_update (dcc);
				if (dcc->wiotag)
				{
					fe_input_remove (dcc->wiotag);
					dcc->wiotag = 0;
				}
				return FALSE;
			}
		}
	}
	return TRUE;
}

static gboolean
proxy_read_line (struct DCC *dcc)
{
	struct proxy_state *proxy = dcc->proxy;
	while (1)
	{
		proxy->buffersize = proxy->bufferused + 1;
		if (!read_proxy (dcc))
			return FALSE;
		if (proxy->buffer[proxy->bufferused - 1] == '\n'
			|| proxy->bufferused == MAX_PROXY_BUFFER)
		{
			proxy->buffer[proxy->bufferused - 1] = 0;
			return TRUE;
		}
	}
}

static gboolean
dcc_wingate_proxy_traverse (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	struct proxy_state *proxy = dcc->proxy;
	if (proxy->phase == 0)
	{
		proxy->buffersize = snprintf ((char*) proxy->buffer, MAX_PROXY_BUFFER,
										"%s %d\r\n", net_ip(dcc->addr),
										dcc->port);
		proxy->bufferused = 0;
		dcc->wiotag = fe_input_add (dcc->sok, FIA_WRITE|FIA_EX,
									dcc_wingate_proxy_traverse, dcc);
		++proxy->phase;
	}
	if (proxy->phase == 1)
	{
		if (!read_proxy (dcc))
			return TRUE;
		fe_input_remove (dcc->wiotag);
		dcc->wiotag = 0;
		dcc_connect_finished (source, 0, dcc);
	}
	return TRUE;
}

struct sock_connect
{
	char version;
	char type;
	guint16 port;
	guint32 address;
	char username[10];
};
static gboolean
dcc_socks_proxy_traverse (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	struct proxy_state *proxy = dcc->proxy;

	if (proxy->phase == 0)
	{
		struct sock_connect sc;
		sc.version = 4;
		sc.type = 1;
		sc.port = htons (dcc->port);
		sc.address = htonl (dcc->addr);
		strncpy (sc.username, prefs.username, 9);
		memcpy (proxy->buffer, &sc, sizeof (sc));
		proxy->buffersize = 8 + strlen (sc.username) + 1;
		proxy->bufferused = 0;
		dcc->wiotag = fe_input_add (dcc->sok, FIA_WRITE|FIA_EX,
									dcc_socks_proxy_traverse, dcc);
		++proxy->phase;
	}

	if (proxy->phase == 1)
	{
		if (!write_proxy (dcc))
			return TRUE;
		fe_input_remove (dcc->wiotag);
		dcc->wiotag = 0;
		proxy->bufferused = 0;
		proxy->buffersize = 8;
		dcc->iotag = fe_input_add (dcc->sok, FIA_READ|FIA_EX,
									dcc_socks_proxy_traverse, dcc);
		++proxy->phase;
	}

	if (proxy->phase == 2)
	{
		if (!read_proxy (dcc))
			return TRUE;
		fe_input_remove (dcc->iotag);
		dcc->iotag = 0;
		if (proxy->buffer[1] == 90)
			dcc_connect_finished (source, 0, dcc);
		else
		{
			dcc->dccstat = STAT_FAILED;
			fe_dcc_update (dcc);
		}
	}

	return TRUE;
}

struct sock5_connect1
{
        char version;
        char nmethods;
        char method;
};
static gboolean
dcc_socks5_proxy_traverse (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	struct proxy_state *proxy = dcc->proxy;
	int auth = prefs.proxy_auth && prefs.proxy_user[0] && prefs.proxy_pass[0];

	if (proxy->phase == 0)
	{
		struct sock5_connect1 sc1;

		sc1.version = 5;
		sc1.nmethods = 1;
		sc1.method = 0;
		if (auth)
			sc1.method = 2;
		memcpy (proxy->buffer, &sc1, 3);
		proxy->buffersize = 3;
		proxy->bufferused = 0;
		dcc->wiotag = fe_input_add (dcc->sok, FIA_WRITE|FIA_EX,
									dcc_socks5_proxy_traverse, dcc);
		++proxy->phase;
	}

	if (proxy->phase == 1)
	{
		if (!write_proxy (dcc))
			return TRUE;
		fe_input_remove (dcc->wiotag);
		dcc->wiotag = 0;
		proxy->bufferused = 0;
		proxy->buffersize = 2;
		dcc->iotag = fe_input_add (dcc->sok, FIA_READ|FIA_EX,
									dcc_socks5_proxy_traverse, dcc);
		++proxy->phase;
	}

	if (proxy->phase == 2)
	{
		if (!read_proxy (dcc))
			return TRUE;
		fe_input_remove (dcc->iotag);
		dcc->iotag = 0;

		/* did the server say no auth required? */
		if (proxy->buffer[0] == 5 && proxy->buffer[1] == 0)
			auth = 0;

		/* Set up authentication I/O */
		if (auth)
		{
			int len_u=0, len_p=0;

			/* authentication sub-negotiation (RFC1929) */
			if ( proxy->buffer[0] != 5 || proxy->buffer[1] != 2 )  /* UPA not supported by server */
			{
				PrintText (dcc->serv->front_session, "SOCKS\tServer doesn't support UPA authentication.\n");
				dcc->dccstat = STAT_FAILED;
				fe_dcc_update (dcc);
				return TRUE;
			}

			memset (proxy->buffer, 0, MAX_PROXY_BUFFER);

			/* form the UPA request */
			len_u = strlen (prefs.proxy_user);
			len_p = strlen (prefs.proxy_pass);
			proxy->buffer[0] = 1;
			proxy->buffer[1] = len_u;
			memcpy (proxy->buffer + 2, prefs.proxy_user, len_u);
			proxy->buffer[2 + len_u] = len_p;
			memcpy (proxy->buffer + 3 + len_u, prefs.proxy_pass, len_p);

			proxy->buffersize = 3 + len_u + len_p;
			proxy->bufferused = 0;
			dcc->wiotag = fe_input_add (dcc->sok, FIA_WRITE|FIA_EX,
										dcc_socks5_proxy_traverse, dcc);
			++proxy->phase;
		}
		else
		{
			if (proxy->buffer[0] != 5 || proxy->buffer[1] != 0)
			{
				PrintText (dcc->serv->front_session, "SOCKS\tAuthentication required but disabled in settings.\n");
				dcc->dccstat = STAT_FAILED;
				fe_dcc_update (dcc);
				return TRUE;
			}
			proxy->phase += 2;
		}
	}
	
	if (proxy->phase == 3)
	{
		if (!write_proxy (dcc))
			return TRUE;
		fe_input_remove (dcc->wiotag);
		dcc->wiotag = 0;
		proxy->buffersize = 2;
		proxy->bufferused = 0;
		dcc->iotag = fe_input_add (dcc->sok, FIA_READ|FIA_EX,
									dcc_socks5_proxy_traverse, dcc);
		++proxy->phase;
	}

	if (proxy->phase == 4)
	{
		if (!read_proxy (dcc))
			return TRUE;
		if (dcc->iotag)
		{
			fe_input_remove (dcc->iotag);
			dcc->iotag = 0;
		}
		if (proxy->buffer[1] != 0)
		{
			PrintText (dcc->serv->front_session, "SOCKS\tAuthentication failed. "
							 "Is username and password correct?\n");
			dcc->dccstat = STAT_FAILED;
			fe_dcc_update (dcc);
			return TRUE;
		}
		++proxy->phase;
	}

	if (proxy->phase == 5)
	{
		proxy->buffer[0] = 5;	/* version (socks 5) */
		proxy->buffer[1] = 1;	/* command (connect) */
		proxy->buffer[2] = 0;	/* reserved */
		proxy->buffer[3] = 1;	/* address type (IPv4) */
		proxy->buffer[4] = (dcc->addr >> 24) & 0xFF;	/* IP address */
		proxy->buffer[5] = (dcc->addr >> 16) & 0xFF;
		proxy->buffer[6] = (dcc->addr >> 8) & 0xFF;
		proxy->buffer[7] = (dcc->addr & 0xFF);
		proxy->buffer[8] = (dcc->port >> 8) & 0xFF;		/* port */
		proxy->buffer[9] = (dcc->port & 0xFF);
		proxy->buffersize = 10;
		proxy->bufferused = 0;
		dcc->wiotag = fe_input_add (dcc->sok, FIA_WRITE|FIA_EX,
									dcc_socks5_proxy_traverse, dcc);
		++proxy->phase;
	}

	if (proxy->phase == 6)
	{
		if (!write_proxy (dcc))
			return TRUE;
		fe_input_remove (dcc->wiotag);
		dcc->wiotag = 0;
		proxy->buffersize = 4;
		proxy->bufferused = 0;
		dcc->iotag = fe_input_add (dcc->sok, FIA_READ|FIA_EX,
									dcc_socks5_proxy_traverse, dcc);
		++proxy->phase;
	}

	if (proxy->phase == 7)
	{
		if (!read_proxy (dcc))
			return TRUE;
		if (proxy->buffer[0] != 5 || proxy->buffer[1] != 0)
		{
			fe_input_remove (dcc->iotag);
			dcc->iotag = 0;
			if (proxy->buffer[1] == 2)
				PrintText (dcc->serv->front_session, "SOCKS\tProxy refused to connect to host (not allowed).\n");
			else
				PrintTextf (dcc->serv->front_session, "SOCKS\tProxy failed to connect to host (error %d).\n", proxy->buffer[1]);
			dcc->dccstat = STAT_FAILED;
			fe_dcc_update (dcc);
			return TRUE;
		}
		switch (proxy->buffer[3])
		{
			case 1: proxy->buffersize += 6; break;
			case 3: proxy->buffersize += 1; break;
			case 4: proxy->buffersize += 18; break;
		};
		++proxy->phase;
	}

	if (proxy->phase == 8)
	{
		if (!read_proxy (dcc))
			return TRUE;
		/* handle domain name case */
		if (proxy->buffer[3] == 3)
		{
			proxy->buffersize = 5 + proxy->buffer[4] + 2;
		}
		/* everything done? */
		if (proxy->bufferused == proxy->buffersize)
		{
			fe_input_remove (dcc->iotag);
			dcc->iotag = 0;
			dcc_connect_finished (source, 0, dcc);
		}
	}
	return TRUE;
}

static gboolean
dcc_http_proxy_traverse (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	struct proxy_state *proxy = dcc->proxy;

	if (proxy->phase == 0)
	{
		char buf[256];
		char auth_data[128];
		char auth_data2[68];
		int n, n2;

		n = snprintf (buf, sizeof (buf), "CONNECT %s:%d HTTP/1.0\r\n",
                                          net_ip(dcc->addr), dcc->port);
		if (prefs.proxy_auth)
		{
			n2 = snprintf (auth_data2, sizeof (auth_data2), "%s:%s",
							prefs.proxy_user, prefs.proxy_pass);
			base64_encode (auth_data, auth_data2, n2);
			n += snprintf (buf+n, sizeof (buf)-n, "Proxy-Authorization: Basic %s\r\n", auth_data);
		}
		n += snprintf (buf+n, sizeof (buf)-n, "\r\n");
		proxy->buffersize = n;
		proxy->bufferused = 0;
		memcpy (proxy->buffer, buf, proxy->buffersize);
		dcc->wiotag = fe_input_add (dcc->sok, FIA_WRITE|FIA_EX,
									dcc_http_proxy_traverse, dcc);
		++proxy->phase;
	}

	if (proxy->phase == 1)
	{
		if (!write_proxy (dcc))
			return TRUE;
		fe_input_remove (dcc->wiotag);
		dcc->wiotag = 0;
		proxy->bufferused = 0;
		dcc->iotag = fe_input_add (dcc->sok, FIA_READ|FIA_EX,
										dcc_http_proxy_traverse, dcc);
		++proxy->phase;
	}

	if (proxy->phase == 2)
	{
		if (!proxy_read_line (dcc))
			return TRUE;
		/* "HTTP/1.0 200 OK" */
		if (proxy->bufferused < 12 ||
			 memcmp (proxy->buffer, "HTTP/", 5) || memcmp (proxy->buffer + 9, "200", 3))
		{
			fe_input_remove (dcc->iotag);
			dcc->iotag = 0;
			PrintText (dcc->serv->front_session, proxy->buffer);
			dcc->dccstat = STAT_FAILED;
			fe_dcc_update (dcc);
			return TRUE;
		}
		proxy->bufferused = 0;
		++proxy->phase;
	}

	if (proxy->phase == 3)
	{
		while (1)
		{
			/* read until blank line */
			if (proxy_read_line (dcc))
			{
				if (proxy->bufferused < 1 ||
					(proxy->bufferused == 2 && proxy->buffer[0] == '\r'))
				{
					break;
				}
				if (proxy->bufferused > 1)
					PrintText (dcc->serv->front_session, proxy->buffer);
				proxy->bufferused = 0;
			}
			else
				return TRUE;
		}
		fe_input_remove (dcc->iotag);
		dcc->iotag = 0;
		dcc_connect_finished (source, 0, dcc);
	}

	return TRUE;
}

static gboolean
dcc_proxy_connect (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	fe_input_remove (dcc->iotag);
	dcc->iotag = 0;

	if (!dcc_did_connect (source, condition, dcc))
		return TRUE;

	dcc->proxy = malloc (sizeof (struct proxy_state));
	if (!dcc->proxy)
	{
		dcc->dccstat = STAT_FAILED;
		fe_dcc_update (dcc);
		return TRUE;
	}
	memset (dcc->proxy, 0, sizeof (struct proxy_state));

	switch (prefs.proxy_type)
	{
		case 1: return dcc_wingate_proxy_traverse (source, condition, dcc);
		case 2: return dcc_socks_proxy_traverse (source, condition, dcc);
		case 3: return dcc_socks5_proxy_traverse (source, condition, dcc);
		case 4: return dcc_http_proxy_traverse (source, condition, dcc);
	}
	return TRUE;
}

static int dcc_listen_init (struct DCC *, struct session *);

static void
dcc_connect (struct DCC *dcc)
{
	int ret;
	char tbuf[400];

	if (dcc->dccstat == STAT_CONNECTING)
		return;
	dcc->dccstat = STAT_CONNECTING;

	if (dcc->pasvid && dcc->port == 0)
	{
		/* accepted a passive dcc send */
		ret = dcc_listen_init (dcc, dcc->serv->front_session);
		if (!ret)
		{
			dcc_close (dcc, STAT_FAILED, FALSE);
			return;
		}
		/* possible problems with filenames containing spaces? */
		if (dcc->type == TYPE_RECV)
			snprintf (tbuf, sizeof (tbuf), strchr (dcc->file, ' ') ?
					"DCC SEND \"%s\" %u %d %"DCC_SFMT" %d" :
					"DCC SEND %s %u %d %"DCC_SFMT" %d", dcc->file,
					dcc->addr, dcc->port, dcc->size, dcc->pasvid);
		else
			snprintf (tbuf, sizeof (tbuf), "DCC CHAT chat %u %d %d",
				dcc->addr, dcc->port, dcc->pasvid);
		dcc->serv->p_ctcp (dcc->serv, dcc->nick, tbuf);
	}
	else
	{
		dcc->sok = dcc_connect_sok (dcc);
		if (dcc->sok == -1)
		{
			dcc->dccstat = STAT_FAILED;
			fe_dcc_update (dcc);
			return;
		}
		if (DCC_USE_PROXY ())
			dcc->iotag = fe_input_add (dcc->sok, FIA_WRITE|FIA_EX, dcc_proxy_connect, dcc);
		else
			dcc->iotag = fe_input_add (dcc->sok, FIA_WRITE|FIA_EX, dcc_connect_finished, dcc);
	}
	
	fe_dcc_update (dcc);
}

static gboolean
dcc_send_data (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	char *buf;
	int len, sent, sok = dcc->sok;

	if (prefs.dcc_blocksize < 1) /* this is too little! */
		prefs.dcc_blocksize = 1024;

	if (prefs.dcc_blocksize > 102400)	/* this is too much! */
		prefs.dcc_blocksize = 102400;

	if (dcc->throttled)
	{
		fe_input_remove (dcc->wiotag);
		dcc->wiotag = 0;
		return FALSE;
	}

	if (!dcc->fastsend)
	{
		if (dcc->ack < dcc->pos)
			return TRUE;
	}
	else if (!dcc->wiotag)
		dcc->wiotag = fe_input_add (sok, FIA_WRITE, dcc_send_data, dcc);

	buf = malloc (prefs.dcc_blocksize);
	if (!buf)
		return TRUE;

	lseek (dcc->fp, dcc->pos, SEEK_SET);
	len = read (dcc->fp, buf, prefs.dcc_blocksize);
	if (len < 1)
		goto abortit;
	sent = send (sok, buf, len, 0);

	if (sent < 0 && !(would_block ()))
	{
abortit:
		free (buf);
		EMIT_SIGNAL (XP_TE_DCCSENDFAIL, dcc->serv->front_session,
						 file_part (dcc->file), dcc->nick,
						 errorstring (sock_error ()), NULL, 0);
		dcc_close (dcc, STAT_FAILED, FALSE);
		return TRUE;
	}
	if (sent > 0)
	{
		dcc->pos += sent;
		dcc->lasttime = time (0);
	}

	/* have we sent it all yet? */
	if (dcc->pos >= dcc->size)
	{
		/* it's all sent now, so remove the WRITE/SEND handler */
		if (dcc->wiotag)
		{
			fe_input_remove (dcc->wiotag);
			dcc->wiotag = 0;
		}
	}

	free (buf);

	return TRUE;
}

static gboolean
dcc_handle_new_ack (struct DCC *dcc)
{
	guint32 ack;
	char buf[16];
	gboolean done = FALSE;

	memcpy (&ack, dcc->ack_buf, 4);
	dcc->ack = ntohl (ack);

	/* this could mess up when xfering >32bit files */
	if (dcc->size <= 0xffffffff)
	{
		/* fix for BitchX */
		if (dcc->ack < dcc->resumable)
			dcc->ackoffset = TRUE;
		if (dcc->ackoffset)
			dcc->ack += dcc->resumable;
	}

	/* DCC complete check */
	if (dcc->pos >= dcc->size && dcc->ack >= (dcc->size & 0xffffffff))
	{
		dcc->ack = dcc->size;	/* force 100% ack for >4 GB */
		dcc_close (dcc, STAT_DONE, FALSE);
		dcc_calc_average_cps (dcc);	/* this must be done _after_ dcc_close, or dcc_remove_from_sum will see the wrong value in dcc->cps */
		sprintf (buf, "%d", dcc->cps);
		EMIT_SIGNAL (XP_TE_DCCSENDCOMP, dcc->serv->front_session,
						 file_part (dcc->file), dcc->nick, buf, NULL, 0);
		done = TRUE;
	}
	else if ((!dcc->fastsend) && (dcc->ack >= (dcc->pos & 0xffffffff)))
	{
		dcc_send_data (NULL, 0, (gpointer)dcc);
	}

#ifdef USE_DCC64
	/* take the top 32 of "bytes send" and bottom 32 of "ack" */
	dcc->ack = (dcc->pos & G_GINT64_CONSTANT (0xffffffff00000000)) |
					(dcc->ack & 0xffffffff);
	/* dcc->ack is only used for CPS and PERCENTAGE calcs from now on... */
#endif

	return done;
}

static gboolean
dcc_read_ack (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	int len;

	while (1)
	{
		/* try to fill up 4 bytes */
		len = recv (dcc->sok, dcc->ack_buf, 4 - dcc->ack_pos, 0);
		if (len < 1)
		{
			if (len < 0)
			{
				if (would_block ())	/* ok - keep waiting */
					return TRUE;
			}
			EMIT_SIGNAL (XP_TE_DCCSENDFAIL, dcc->serv->front_session,
							 file_part (dcc->file), dcc->nick,
							 errorstring ((len < 0) ? sock_error () : 0), NULL, 0);
			dcc_close (dcc, STAT_FAILED, FALSE);
			return TRUE;
		}

		dcc->ack_pos += len;
		if (dcc->ack_pos >= 4)
		{
			dcc->ack_pos = 0;
			if (dcc_handle_new_ack (dcc))
				return TRUE;
		}
		/* loop again until would_block() returns true */
	}
}

static gboolean
dcc_accept (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	char host[128];
	struct sockaddr_in CAddr;
	int sok;
	socklen_t len;

	len = sizeof (CAddr);
	sok = accept (dcc->sok, (struct sockaddr *) &CAddr, &len);
	fe_input_remove (dcc->iotag);
	dcc->iotag = 0;
	closesocket (dcc->sok);
	if (sok < 0)
	{
		dcc->sok = -1;
		dcc_close (dcc, STAT_FAILED, FALSE);
		return TRUE;
	}
	set_nonblocking (sok);
	dcc->sok = sok;
	dcc->addr = ntohl (CAddr.sin_addr.s_addr);

	if (dcc->pasvid)
		return dcc_connect_finished (NULL, 0, dcc);

	dcc->dccstat = STAT_ACTIVE;
	dcc->lasttime = dcc->starttime = time (0);
	dcc->fastsend = prefs.fastdccsend;

	snprintf (host, sizeof (host), "%s:%d", net_ip (dcc->addr), dcc->port);

	switch (dcc->type)
	{
	case TYPE_SEND:
		if (dcc->fastsend)
			dcc->wiotag = fe_input_add (sok, FIA_WRITE, dcc_send_data, dcc);
		dcc->iotag = fe_input_add (sok, FIA_READ|FIA_EX, dcc_read_ack, dcc);
		dcc_send_data (NULL, 0, (gpointer)dcc);
		EMIT_SIGNAL (XP_TE_DCCCONSEND, dcc->serv->front_session,
						 dcc->nick, host, dcc->file, NULL, 0);
		break;

	case TYPE_CHATSEND:
		dcc_open_query (dcc->serv, dcc->nick);
		dcc->iotag = fe_input_add (dcc->sok, FIA_READ|FIA_EX, dcc_read_chat, dcc);
		dcc->dccchat = malloc (sizeof (struct dcc_chat));
		dcc->dccchat->pos = 0;
		EMIT_SIGNAL (XP_TE_DCCCONCHAT, dcc->serv->front_session,
						 dcc->nick, host, NULL, NULL, 0);
		break;
	}

	fe_dcc_update (dcc);

	return TRUE;
}

guint32
dcc_get_my_address (void)	/* the address we'll tell the other person */
{
	struct hostent *dns_query;
	guint32 addr = 0;

	if (prefs.ip_from_server && prefs.dcc_ip)
		addr = prefs.dcc_ip;
	else if (prefs.dcc_ip_str[0])
	{
	   dns_query = gethostbyname ((const char *) prefs.dcc_ip_str);

	   if (dns_query != NULL &&
	       dns_query->h_length == 4 &&
	       dns_query->h_addr_list[0] != NULL)
		{
			/*we're offered at least one IPv4 address: we take the first*/
			addr = *((guint32*) dns_query->h_addr_list[0]);
		}
	}

	return addr;
}

static int
dcc_listen_init (struct DCC *dcc, session *sess)
{
	guint32 my_addr;
	struct sockaddr_in SAddr;
	int i, bindretval = -1;
	socklen_t len;

	dcc->sok = socket (AF_INET, SOCK_STREAM, 0);
	if (dcc->sok == -1)
		return FALSE;

	memset (&SAddr, 0, sizeof (struct sockaddr_in));

	len = sizeof (SAddr);
	getsockname (dcc->serv->sok, (struct sockaddr *) &SAddr, &len);

	SAddr.sin_family = AF_INET;

	/*if local_ip is specified use that*/
	if (prefs.local_ip != 0xffffffff)
	{
		my_addr = prefs.local_ip;
		SAddr.sin_addr.s_addr = prefs.local_ip;
	}
	/*otherwise use the default*/
	else
		my_addr = SAddr.sin_addr.s_addr;

	/*if we have a valid portrange try to use that*/
	if (prefs.first_dcc_send_port > 0)
	{
		SAddr.sin_port = 0;
		i = 0;
		while ((prefs.last_dcc_send_port > ntohs(SAddr.sin_port)) &&
				(bindretval == -1))
		{
			SAddr.sin_port = htons (prefs.first_dcc_send_port + i);
			i++;
			/*printf("Trying to bind against port: %d\n",ntohs(SAddr.sin_port));*/
			bindretval = bind (dcc->sok, (struct sockaddr *) &SAddr, sizeof (SAddr));
		}

		/* with a small port range, reUseAddr is needed */
		len = 1;
		setsockopt (dcc->sok, SOL_SOCKET, SO_REUSEADDR, (char *) &len, sizeof (len));

	} else
	{
		/* try random port */
		SAddr.sin_port = 0;
		bindretval = bind (dcc->sok, (struct sockaddr *) &SAddr, sizeof (SAddr));
	}

	if (bindretval == -1)
	{
		/* failed to bind */
		PrintText (sess, "Failed to bind to any address or port.\n");
		return FALSE;
	}

	len = sizeof (SAddr);
	getsockname (dcc->sok, (struct sockaddr *) &SAddr, &len);

	dcc->port = ntohs (SAddr.sin_port);

	/*if we have a dcc_ip, we use that, so the remote client can connect*/
	/*else we try to take an address from dcc_ip_str*/
	/*if something goes wrong we tell the client to connect to our LAN ip*/
	dcc->addr = dcc_get_my_address ();

	/*if nothing else worked we use the address we bound to*/
	if (dcc->addr == 0)
	   dcc->addr = my_addr;

	dcc->addr = ntohl (dcc->addr);

	set_nonblocking (dcc->sok);
	listen (dcc->sok, 1);
	set_blocking (dcc->sok);

	dcc->iotag = fe_input_add (dcc->sok, FIA_READ|FIA_EX, dcc_accept, dcc);

	return TRUE;
}

static struct session *dccsess;
static char *dccto;				  /* lame!! */
static int dccmaxcps;
static int recursive = FALSE;

static void
dcc_send_wild (char *file)
{
	dcc_send (dccsess, dccto, file, dccmaxcps, 0);
}

void
dcc_send (struct session *sess, char *to, char *file, int maxcps, int passive)
{
	char outbuf[512];
	struct stat st;
	struct DCC *dcc;
	char *file_fs;

	/* this is utf8 */
	file = expand_homedir (file);

	if (!recursive && (strchr (file, '*') || strchr (file, '?')))
	{
		char path[256];
		char wild[256];
		char *path_fs;	/* local filesystem encoding */

		safe_strcpy (wild, file_part (file), sizeof (wild));
		path_part (file, path, sizeof (path));
		if (path[0] != '/' || path[1] != '\0')
			path[strlen (path) - 1] = 0;	/* remove trailing slash */

		dccsess = sess;
		dccto = to;
		dccmaxcps = maxcps;

		free (file);

		/* for_files() will use opendir, so we need local FS encoding */
		path_fs = xchat_filename_from_utf8 (path, -1, 0, 0, 0);
		if (path_fs)
		{
			recursive = TRUE;
			for_files (path_fs, wild, dcc_send_wild);
			recursive = FALSE;
			g_free (path_fs);
		}

		return;
	}

	dcc = new_dcc ();
	if (!dcc)
		return;
	dcc->file = file;
	dcc->maxcps = maxcps;

	/* get the local filesystem encoding */
	file_fs = xchat_filename_from_utf8 (file, -1, 0, 0, 0);

	if (stat (file_fs, &st) != -1)
	{

#ifndef USE_DCC64
		if (sizeof (st.st_size) > 4 && st.st_size > 4294967295U)
		{
			PrintText (sess, "Cannot send files larger than 4 GB.\n");
			goto xit;
		}
#endif

		if (!(*file_part (file_fs)) || S_ISDIR (st.st_mode) || st.st_size < 1)
		{
			PrintText (sess, "Cannot send directories or empty files.\n");
			goto xit;
		}

		dcc->starttime = dcc->offertime = time (0);
		dcc->serv = sess->server;
		dcc->dccstat = STAT_QUEUED;
		dcc->size = st.st_size;
		dcc->type = TYPE_SEND;
		dcc->fp = open (file_fs, OFLAGS | O_RDONLY);
		if (dcc->fp != -1)
		{
			g_free (file_fs);
			if (passive || dcc_listen_init (dcc, sess))
			{
				char havespaces = 0;
				file = dcc->file;
				while (*file)
				{
					if (*file == ' ')
					{
						if (prefs.dcc_send_fillspaces)
				    		*file = '_';
					  	else
					   	havespaces = 1;
					}
					file++;
				}
				dcc->nick = strdup (to);
				if (prefs.autoopendccsendwindow)
				{
					if (fe_dcc_open_send_win (TRUE))	/* already open? add */
						fe_dcc_add (dcc);
				} else
					fe_dcc_add (dcc);

				if (passive)
				{
					dcc->pasvid = new_id();
					snprintf (outbuf, sizeof (outbuf), (havespaces) ?
							"DCC SEND \"%s\" 199 0 %"DCC_SFMT" %d" :
							"DCC SEND %s 199 0 %"DCC_SFMT" %d",
							file_part (dcc->file),
							dcc->size, dcc->pasvid);
				} else
				{
					snprintf (outbuf, sizeof (outbuf), (havespaces) ?
							"DCC SEND \"%s\" %u %d %"DCC_SFMT :
							"DCC SEND %s %u %d %"DCC_SFMT,
							file_part (dcc->file), dcc->addr,
							dcc->port, dcc->size);
				}
				sess->server->p_ctcp (sess->server, to, outbuf);

				EMIT_SIGNAL (XP_TE_DCCOFFER, sess, file_part (dcc->file),
								 to, dcc->file, NULL, 0);
			} else
			{
				dcc_close (dcc, 0, TRUE);
			}
			return;
		}
	}
	PrintTextf (sess, _("Cannot access %s\n"), dcc->file);
	PrintTextf (sess, "%s %d: %s\n", _("Error"), errno, errorstring (errno));
xit:
	g_free (file_fs);
	dcc_close (dcc, 0, TRUE);
}

static struct DCC *
find_dcc_from_id (int id, int type)
{
	struct DCC *dcc;
	GSList *list = dcc_list;
	while (list)
	{
		dcc = (struct DCC *) list->data;
		if (dcc->pasvid == id &&
			 dcc->dccstat == STAT_QUEUED && dcc->type == type)
		return dcc;
		list = list->next;
	}
	return 0;
}

static struct DCC *
find_dcc_from_port (int port, int type)
{
	struct DCC *dcc;
	GSList *list = dcc_list;
	while (list)
	{
		dcc = (struct DCC *) list->data;
		if (dcc->port == port &&
			 dcc->dccstat == STAT_QUEUED && dcc->type == type)
			return dcc;
		list = list->next;
	}
	return 0;
}

struct DCC *
find_dcc (char *nick, char *file, int type)
{
	GSList *list = dcc_list;
	struct DCC *dcc;
	while (list)
	{
		dcc = (struct DCC *) list->data;
		if (nick == NULL || !rfc_casecmp (nick, dcc->nick))
		{
			if (type == -1 || dcc->type == type)
			{
				if (!file[0])
					return dcc;
				if (!strcasecmp (file, file_part (dcc->file)))
					return dcc;
				if (!strcasecmp (file, dcc->file))
					return dcc;
			}
		}
		list = list->next;
	}
	return 0;
}

/* called when we receive a NICK change from server */

void
dcc_change_nick (struct server *serv, char *oldnick, char *newnick)
{
	struct DCC *dcc;
	GSList *list = dcc_list;

	while (list)
	{
		dcc = (struct DCC *) list->data;
		if (dcc->serv == serv)
		{
			if (!serv->p_cmp (dcc->nick, oldnick))
			{
				if (dcc->nick)
					free (dcc->nick);
				dcc->nick = strdup (newnick);
			}
		}
		list = list->next;
	}
}

/* is the destination file the same? new_dcc is not opened yet */

static int
is_same_file (struct DCC *dcc, struct DCC *new_dcc)
{
	struct stat st_a, st_b;

	/* if it's the same filename, must be same */
	if (strcmp (dcc->destfile, new_dcc->destfile) == 0)
		return TRUE;

	/* now handle case-insensitive Filesystems: HFS+, FAT */
#ifdef WIN32
#warning no win32 implementation - behaviour may be unreliable
#else
	/* this fstat() shouldn't really fail */
	if ((dcc->fp == -1 ? stat (dcc->destfile_fs, &st_a) : fstat (dcc->fp, &st_a)) == -1)
		return FALSE;
	if (stat (new_dcc->destfile_fs, &st_b) == -1)
		return FALSE;

	/* same inode, same device, same file! */
	if (st_a.st_ino == st_b.st_ino &&
		 st_a.st_dev == st_b.st_dev)
		return TRUE;
#endif

	return FALSE;
}

static int
is_resumable (struct DCC *dcc)
{
	dcc->resumable = 0;

	/* Check the file size */
	if (access (dcc->destfile_fs, W_OK) == 0)
	{
		struct stat st;

		if (stat (dcc->destfile_fs, &st) != -1)
		{
			if (st.st_size < dcc->size)
			{
				dcc->resumable = st.st_size;
				dcc->pos = st.st_size;
			}
			else
				dcc->resume_error = 2;
		} else
		{
			dcc->resume_errno = errno;
			dcc->resume_error = 1;
		}
	} else
	{
		dcc->resume_errno = errno;
		dcc->resume_error = 1;
	}

	/* Now verify that this DCC is not already in progress from someone else */

	if (dcc->resumable)
	{
		GSList *list = dcc_list;
		struct DCC *d;
		while (list)
		{
			d = list->data;
			if (d->type == TYPE_RECV && d->dccstat != STAT_ABORTED &&
				 d->dccstat != STAT_DONE && d->dccstat != STAT_FAILED)
			{
				if (d != dcc && is_same_file (d, dcc))
				{
					dcc->resume_error = 3;	/* dccgui.c uses it */
					dcc->resumable = 0;
					dcc->pos = 0;
					break;
				}
			}
			list = list->next;
		}
	}

	return dcc->resumable;
}

void
dcc_get (struct DCC *dcc)
{
	switch (dcc->dccstat)
	{
	case STAT_QUEUED:
		if (dcc->type != TYPE_CHATSEND)
		{
			if (dcc->type == TYPE_RECV && prefs.autoresume && dcc->resumable)
			{
				dcc_resume (dcc);
			}
			else
			{
				dcc->resumable = 0;
				dcc->pos = 0;
				dcc_connect (dcc);
			}
		}
		break;
	case STAT_DONE:
	case STAT_FAILED:
	case STAT_ABORTED:
		dcc_close (dcc, 0, TRUE);
		break;
	}
}

/* for "Save As..." dialog */

void
dcc_get_with_destfile (struct DCC *dcc, char *file)
{
	g_free (dcc->destfile);
	dcc->destfile = g_strdup (file);	/* utf-8 */

	/* get the local filesystem encoding */
	g_free (dcc->destfile_fs);
	dcc->destfile_fs = xchat_filename_from_utf8 (dcc->destfile, -1, 0, 0, 0);

	/* since destfile changed, must check resumability again */
	is_resumable (dcc);

	dcc_get (dcc);
}

void
dcc_get_nick (struct session *sess, char *nick)
{
	struct DCC *dcc;
	GSList *list = dcc_list;
	while (list)
	{
		dcc = (struct DCC *) list->data;
		if (!sess->server->p_cmp (nick, dcc->nick))
		{
			if (dcc->dccstat == STAT_QUEUED && dcc->type == TYPE_RECV)
			{
				dcc->resumable = 0;
				dcc->pos = 0;
				dcc->ack = 0;
				dcc_connect (dcc);
				return;
			}
		}
		list = list->next;
	}
	if (sess)
		EMIT_SIGNAL (XP_TE_DCCIVAL, sess, NULL, NULL, NULL, NULL, 0);
}

static struct DCC *
new_dcc (void)
{
	struct DCC *dcc = malloc (sizeof (struct DCC));
	if (!dcc)
		return 0;
	memset (dcc, 0, sizeof (struct DCC));
	dcc->sok = -1;
	dcc->fp = -1;
	dcc_list = g_slist_prepend (dcc_list, dcc);
	return (dcc);
}

void
dcc_chat (struct session *sess, char *nick, int passive)
{
	char outbuf[512];
	struct DCC *dcc;

	dcc = find_dcc (nick, "", TYPE_CHATSEND);
	if (dcc)
	{
		switch (dcc->dccstat)
		{
		case STAT_ACTIVE:
		case STAT_QUEUED:
		case STAT_CONNECTING:
			EMIT_SIGNAL (XP_TE_DCCCHATREOFFER, sess, nick, NULL, NULL, NULL, 0);
			return;
		case STAT_ABORTED:
		case STAT_FAILED:
			dcc_close (dcc, 0, TRUE);
		}
	}
	dcc = find_dcc (nick, "", TYPE_CHATRECV);
	if (dcc)
	{
		switch (dcc->dccstat)
		{
		case STAT_QUEUED:
			dcc_connect (dcc);
			break;
		case STAT_FAILED:
		case STAT_ABORTED:
			dcc_close (dcc, 0, TRUE);
		}
		return;
	}
	/* offer DCC CHAT */

	dcc = new_dcc ();
	if (!dcc)
		return;
	dcc->starttime = dcc->offertime = time (0);
	dcc->serv = sess->server;
	dcc->dccstat = STAT_QUEUED;
	dcc->type = TYPE_CHATSEND;
	dcc->nick = strdup (nick);
	if (passive || dcc_listen_init (dcc, sess))
	{
		if (prefs.autoopendccchatwindow)
		{
			if (fe_dcc_open_chat_win (TRUE))	/* already open? add only */
				fe_dcc_add (dcc);
		} else
			fe_dcc_add (dcc);

		if (passive)
		{
			dcc->pasvid = new_id ();
			snprintf (outbuf, sizeof (outbuf), "DCC CHAT chat 199 %d %d",
						 dcc->port, dcc->pasvid);
		} else
		{
			snprintf (outbuf, sizeof (outbuf), "DCC CHAT chat %u %d",
						 dcc->addr, dcc->port);
		}
		dcc->serv->p_ctcp (dcc->serv, nick, outbuf);
		EMIT_SIGNAL (XP_TE_DCCCHATOFFERING, sess, nick, NULL, NULL, NULL, 0);
	} else
	{
		dcc_close (dcc, 0, TRUE);
	}
}

static void
dcc_malformed (struct session *sess, char *nick, char *data)
{
	EMIT_SIGNAL (XP_TE_MALFORMED, sess, nick, data, NULL, NULL, 0);
}

int
dcc_resume (struct DCC *dcc)
{
	char tbuf[500];

	if (dcc->dccstat == STAT_QUEUED && dcc->resumable)
	{
		dcc->resume_sent = 1;
		/* filename contains spaces? Quote them! */
		snprintf (tbuf, sizeof (tbuf) - 10, strchr (dcc->file, ' ') ?
					  "DCC RESUME \"%s\" %d %"DCC_SFMT :
					  "DCC RESUME %s %d %"DCC_SFMT,
					  dcc->file, dcc->port, dcc->resumable);

		if (dcc->pasvid)
 			sprintf (tbuf + strlen (tbuf), " %d", dcc->pasvid);

		dcc->serv->p_ctcp (dcc->serv, dcc->nick, tbuf);
		return 1;
	}

	return 0;
}

static void
dcc_confirm_send (void *ud)
{
	struct DCC *dcc = (struct DCC *) ud;
	dcc_get (dcc);
}

static void
dcc_deny_send (void *ud)
{
	struct DCC *dcc = (struct DCC *) ud;
	dcc_abort (dcc->serv->front_session, dcc);
}

static void
dcc_confirm_chat (void *ud)
{
	struct DCC *dcc = (struct DCC *) ud;
	dcc_connect (dcc);
}

static void
dcc_deny_chat (void *ud)
{
	struct DCC *dcc = (struct DCC *) ud;
	dcc_abort (dcc->serv->front_session, dcc);
}

static struct DCC *
dcc_add_chat (session *sess, char *nick, int port, guint32 addr, int pasvid)
{
	struct DCC *dcc;

	dcc = new_dcc ();
	if (dcc)
	{
		dcc->serv = sess->server;
		dcc->type = TYPE_CHATRECV;
		dcc->dccstat = STAT_QUEUED;
		dcc->addr = addr;
		dcc->port = port;
		dcc->pasvid = pasvid;
		dcc->nick = strdup (nick);
		dcc->starttime = time (0);

		EMIT_SIGNAL (XP_TE_DCCCHATOFFER, sess->server->front_session, nick,
						 NULL, NULL, NULL, 0);

		if (prefs.autoopendccchatwindow)
		{
			if (fe_dcc_open_chat_win (TRUE))	/* already open? add only */
				fe_dcc_add (dcc);
		} else
			fe_dcc_add (dcc);

		if (prefs.autodccchat == 1)
			dcc_connect (dcc);
		else if (prefs.autodccchat == 2)
		{
			char buff[128];
			snprintf (buff, sizeof (buff), "%s is offering DCC Chat. Do you want to accept?", nick);
			fe_confirm (buff, dcc_confirm_chat, dcc_deny_chat, dcc);
		}
	}

	return dcc;
}

static struct DCC *
dcc_add_file (session *sess, char *file, DCC_SIZE size, int port, char *nick, guint32 addr, int pasvid)
{
	struct DCC *dcc;
	char tbuf[512];

	dcc = new_dcc ();
	if (dcc)
	{
		dcc->file = strdup (file);

		dcc->destfile = g_malloc (strlen (prefs.dccdir) + strlen (nick) +
										  strlen (file) + 4);

		strcpy (dcc->destfile, prefs.dccdir);
		if (prefs.dccdir[strlen (prefs.dccdir) - 1] != '/')
			strcat (dcc->destfile, "/");
		if (prefs.dccwithnick)
		{
#ifdef WIN32
			char *t = strlen (dcc->destfile) + dcc->destfile;
			strcpy (t, nick);
			while (*t)
			{
				if (*t == '\\' || *t == '|')
					*t = '_';
				t++;
			}
#else
			strcat (dcc->destfile, nick);
#endif
			strcat (dcc->destfile, ".");
		}
		strcat (dcc->destfile, file);

		/* get the local filesystem encoding */
		dcc->destfile_fs = xchat_filename_from_utf8 (dcc->destfile, -1, 0, 0, 0);

		dcc->resumable = 0;
		dcc->pos = 0;
		dcc->serv = sess->server;
		dcc->type = TYPE_RECV;
		dcc->dccstat = STAT_QUEUED;
		dcc->addr = addr;
		dcc->port = port;
		dcc->pasvid = pasvid;
		dcc->size = size;
		dcc->nick = strdup (nick);
		dcc->maxcps = prefs.dcc_max_get_cps;

		is_resumable (dcc);

		/* autodccsend is really autodccrecv.. right? */

		if (prefs.autodccsend == 1)
		{
			dcc_get (dcc);
		}
		else if (prefs.autodccsend == 2)
		{
			snprintf (tbuf, sizeof (tbuf), _("%s is offering \"%s\". Do you want to accept?"), nick, file);
			fe_confirm (tbuf, dcc_confirm_send, dcc_deny_send, dcc);
		}
		if (prefs.autoopendccrecvwindow)
		{
			if (fe_dcc_open_recv_win (TRUE))	/* was already open? just add*/
				fe_dcc_add (dcc);
		} else
			fe_dcc_add (dcc);
	}
	sprintf (tbuf, "%"DCC_SFMT, size);
	snprintf (tbuf + 24, 300, "%s:%d", net_ip (addr), port);
	EMIT_SIGNAL (XP_TE_DCCSENDOFFER, sess->server->front_session, nick,
					 file, tbuf, tbuf + 24, 0);

	return dcc;
}

void
handle_dcc (struct session *sess, char *nick, char *word[],
				char *word_eol[])
{
	char tbuf[512];
	struct DCC *dcc;
	char *type = word[5];
	int port, pasvid = 0;
	guint32 addr;
	DCC_SIZE size;
	int psend = 0;

	if (!strcasecmp (type, "CHAT"))
	{
		port = atoi (word[8]);
		addr = strtoul (word[7], NULL, 10);

		if (port == 0)
			pasvid = atoi (word[9]);
		else if (word[9][0] != 0)
		{
			pasvid = atoi (word[9]);
			psend = 1;
		}

		if (!addr /*|| (port < 1024 && port != 0)*/
			|| port > 0xffff || (port == 0 && pasvid == 0))
		{
			dcc_malformed (sess, nick, word_eol[4] + 2);
			return;
		}

		if (psend)
		{
			dcc = find_dcc_from_id (pasvid, TYPE_CHATSEND);
			if (dcc)
			{
				dcc->addr = addr;
				dcc->port = port;
				dcc_connect (dcc);
			} else
			{
				dcc_malformed (sess, nick, word_eol[4] + 2);
			}
			return;
		}

		dcc = find_dcc (nick, "", TYPE_CHATSEND);
		if (dcc)
			dcc_close (dcc, 0, TRUE);

		dcc = find_dcc (nick, "", TYPE_CHATRECV);
		if (dcc)
			dcc_close (dcc, 0, TRUE);

		dcc_add_chat (sess, nick, port, addr, pasvid);
		return;
	}

	if (!strcasecmp (type, "Resume"))
	{
		port = atoi (word[7]);

		if (port == 0)
		{ /* PASSIVE */
			pasvid = atoi(word[9]);
			dcc = find_dcc_from_id(pasvid, TYPE_SEND);
		} else
		{
			dcc = find_dcc_from_port (port, TYPE_SEND);
		}
		if (!dcc)
			dcc = find_dcc (nick, word[6], TYPE_SEND);
		if (dcc)
		{
			size = BIG_STR_TO_INT (word[8]);
			dcc->resumable = size;
			if (dcc->resumable < dcc->size)
			{
				dcc->pos = dcc->resumable;
				dcc->ack = dcc->resumable;
				lseek (dcc->fp, dcc->pos, SEEK_SET);

				/* Checking if dcc is passive and if filename contains spaces */
				if (dcc->pasvid)
					snprintf (tbuf, sizeof (tbuf), strchr (file_part (dcc->file), ' ') ?
							"DCC ACCEPT \"%s\" %d %"DCC_SFMT" %d" :
							"DCC ACCEPT %s %d %"DCC_SFMT" %d",
							file_part (dcc->file), port, dcc->resumable, dcc->pasvid);
				else
					snprintf (tbuf, sizeof (tbuf), strchr (file_part (dcc->file), ' ') ?
							"DCC ACCEPT \"%s\" %d %"DCC_SFMT :
							"DCC ACCEPT %s %d %"DCC_SFMT,
							file_part (dcc->file), port, dcc->resumable);

				dcc->serv->p_ctcp (dcc->serv, dcc->nick, tbuf);
			}
			sprintf (tbuf, "%"DCC_SFMT, dcc->pos);
			EMIT_SIGNAL (XP_TE_DCCRESUMEREQUEST, sess, nick,
							 file_part (dcc->file), tbuf, NULL, 0);
		}
		return;
	}
	if (!strcasecmp (type, "Accept"))
	{
		port = atoi (word[7]);
		dcc = find_dcc_from_port (port, TYPE_RECV);
		if (dcc && dcc->dccstat == STAT_QUEUED)
		{
			dcc_connect (dcc);
		}
		return;
	}
	if (!strcasecmp (type, "SEND"))
	{
		char *file = file_part (word[6]);

		port = atoi (word[8]);
		addr = strtoul (word[7], NULL, 10);
		size = BIG_STR_TO_INT (word[9]);

		if (port == 0) /* Passive dcc requested */
			pasvid = atoi (word[10]);
		else if (word[10][0] != 0)
		{
			/* Requesting passive dcc.
			 * Destination user of an active dcc is giving his
			 * TRUE address/port/pasvid data.
			 * This information will be used later to
			 * establish the connection to the user.
			 * We can recognize this type of dcc using word[10]
			 * because this field is always null (no pasvid)
			 * in normal dcc sends.
			 */
			pasvid = atoi (word[10]);
			psend = 1;
		}


		if (!addr || !size /*|| (port < 1024 && port != 0)*/
			|| port > 0xffff || (port == 0 && pasvid == 0))
		{
			dcc_malformed (sess, nick, word_eol[4] + 2);
			return;
		}

		if (psend)
		{
			/* Third Step of Passive send.
			 * Connecting to the destination and finally
			 * sending file.
			 */
			dcc = find_dcc_from_id (pasvid, TYPE_SEND);
			if (dcc)
			{
				dcc->addr = addr;
				dcc->port = port;
				dcc_connect (dcc);
			} else
			{
				dcc_malformed (sess, nick, word_eol[4] + 2);
			}
			return;
		}

		dcc_add_file (sess, file, size, port, nick, addr, pasvid);

	} else
	{
		EMIT_SIGNAL (XP_TE_DCCGENERICOFFER, sess->server->front_session,
						 word_eol[4] + 2, nick, NULL, NULL, 0);
	}
}

void
dcc_show_list (struct session *sess)
{
	int i = 0;
	struct DCC *dcc;
	GSList *list = dcc_list;

	EMIT_SIGNAL (XP_TE_DCCHEAD, sess, NULL, NULL, NULL, NULL, 0);
	while (list)
	{
		dcc = (struct DCC *) list->data;
		i++;
		PrintTextf (sess, " %s  %-10.10s %-7.7s %-7"DCC_SFMT" %-7"DCC_SFMT" %s\n",
					 dcctypes[dcc->type], dcc->nick,
					 _(dccstat[dcc->dccstat].name), dcc->size, dcc->pos,
					 file_part (dcc->file));
		list = list->next;
	}
	if (!i)
		PrintText (sess, _("No active DCCs\n"));
}
