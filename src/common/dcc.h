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

enum dcc_state {
	STAT_QUEUED = 0,
	STAT_ACTIVE,
	STAT_FAILED,
	STAT_DONE,
	STAT_CONNECTING,
	STAT_ABORTED
};

enum dcc_type {
	TYPE_SEND = 0,
	TYPE_RECV,
	TYPE_CHATRECV,
	TYPE_CHATSEND
};

#define CPS_AVG_WINDOW 10

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
	gint64 cps;
	int resume_error;
	int resume_errno;

	GTimeVal lastcpstv, firstcpstv;
	goffset lastcpspos;
	gint64 maxcps;

	unsigned char ack_buf[4];	/* buffer for reading 4-byte ack */
	int ack_pos;

	guint64 size;
	guint64 resumable;
	guint64 ack;
	guint64 pos;
	time_t starttime;
	time_t offertime;
	time_t lasttime;
	char *file;					/* utf8 */
	char *destfile;			/* utf8 */
	char *nick;
	enum dcc_type type;
	enum dcc_state dccstat;
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
void dcc_change_nick (server *serv, char *oldnick, char *newnick);
void dcc_notify_kill (struct server *serv);
struct DCC *dcc_write_chat (char *nick, char *text);
void dcc_send (struct session *sess, char *to, char *file, gint64 maxcps, int passive);
struct DCC *find_dcc (char *nick, char *file, int type);
void dcc_get_nick (struct session *sess, char *nick);
void dcc_chat (session *sess, char *nick, int passive);
void handle_dcc (session *sess, char *nick, char *word[], char *word_eol[],
					  const message_tags_data *tags_data);
void dcc_show_list (session *sess);
guint32 dcc_get_my_address (session *sess);
void dcc_get_with_destfile (struct DCC *dcc, char *utf8file);

#endif
