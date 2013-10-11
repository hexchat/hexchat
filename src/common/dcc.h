/* HexChat
 * Copyright (C) 1998-2010 Peter Zelezny.
 * Copyright (C) 2009-2013 Berke Viktor.
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

/* dcc.h */

#include <time.h>						/* for time_t */
#include "proto-irc.h"

#ifndef HEXCHAT_DCC_H
#define HEXCHAT_DCC_H

#define STAT_QUEUED 0
#define STAT_ACTIVE 1
#define STAT_FAILED 2
#define STAT_DONE 3
#define STAT_CONNECTING 4
#define STAT_ABORTED 5

#define TYPE_SEND 0
#define TYPE_RECV 1
#define TYPE_CHATRECV 2
#define TYPE_CHATSEND 3

#define CPS_AVG_WINDOW 10

/* can we do 64-bit dcc? */
#if defined(G_GINT64_FORMAT) && defined(HAVE_STRTOULL)
#define USE_DCC64
/* we really get only 63 bits, since st_size is signed */
#define DCC_SIZE gint64
#define DCC_SFMT G_GINT64_FORMAT
#else
#define DCC_SIZE unsigned int
#define DCC_SFMT "u"
#endif

struct DCC
{
	struct server *serv;
	struct dcc_chat *dccchat;
	struct proxy_state *proxy;
	guint32 addr;					/* the 32bit IP number, host byte order */
	int fp;							/* file pointer */
	int sok;
	int iotag;						/* reading io tag */
	int wiotag;						/* writing/sending io tag */
	int port;
	int pasvid;						/* mIRC's passive DCC id */
	int cps;
	int resume_error;
	int resume_errno;

	GTimeVal lastcpstv, firstcpstv;
	DCC_SIZE lastcpspos;
	int maxcps;

	unsigned char ack_buf[4];	/* buffer for reading 4-byte ack */
	int ack_pos;

	DCC_SIZE size;
	DCC_SIZE resumable;
	DCC_SIZE ack;
	DCC_SIZE pos;
	time_t starttime;
	time_t offertime;
	time_t lasttime;
	char *file;					/* utf8 */
	char *destfile;			/* utf8 */
	char *nick;
	unsigned char type;		  /* 0 = SEND  1 = RECV  2 = CHAT */
	unsigned char dccstat;	  /* 0 = QUEUED  1 = ACTIVE  2 = FAILED  3 = DONE */
	unsigned int resume_sent:1;	/* resume request sent */
	unsigned int fastsend:1;
	unsigned int ackoffset:1;	/* is receiver sending acks as an offset from */
										/* the resume point? */
	unsigned int throttled:2;	/* 0x1 = per send/get throttle
											0x2 = global throttle */
};

#define MAX_PROXY_BUFFER 1024
struct proxy_state
{
	int phase;
	unsigned char buffer[MAX_PROXY_BUFFER];
	int buffersize;
	int bufferused;
};

struct dcc_chat
{
	char linebuf[2048];
	int pos;
};

struct dccstat_info
{
	char *name;						  /* Display name */
	int color;						  /* Display color (index into colors[] ) */
};

extern struct dccstat_info dccstat[];

gboolean is_dcc (struct DCC *dcc);
gboolean is_dcc_completed (struct DCC *dcc);
void dcc_abort (session *sess, struct DCC *dcc);
void dcc_get (struct DCC *dcc);
int dcc_resume (struct DCC *dcc);
void dcc_check_timeouts (void);
void dcc_change_nick (server *serv, char *oldnick, char *newnick);
void dcc_notify_kill (struct server *serv);
struct DCC *dcc_write_chat (char *nick, char *text);
void dcc_send (struct session *sess, char *to, char *file, int maxcps, int passive);
struct DCC *find_dcc (char *nick, char *file, int type);
void dcc_get_nick (struct session *sess, char *nick);
void dcc_chat (session *sess, char *nick, int passive);
void handle_dcc (session *sess, char *nick, char *word[], char *word_eol[],
					  const message_tags_data *tags_data);
void dcc_show_list (session *sess);
guint32 dcc_get_my_address (void);
void dcc_get_with_destfile (struct DCC *dcc, char *utf8file);

#endif
