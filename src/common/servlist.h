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

#ifndef HEXCHAT_SERVLIST_H
#define HEXCHAT_SERVLIST_H

typedef struct ircserver
{
	char *hostname;
} ircserver;

typedef struct ircnet
{
	char *name;
	char *nick;
	char *nick2;
	char *user;
	char *real;
	char *pass;
	char *saslpass;
	char *autojoin;
	char *command;
	char *nickserv;
	int nstype;
	char *comment;
	char *encoding;
	GSList *servlist;
	int selected;
	guint32 flags;
} ircnet;

extern GSList *network_list;

#define FLAG_CYCLE				1
#define FLAG_USE_GLOBAL			2
#define FLAG_USE_SSL				4
#define FLAG_AUTO_CONNECT		8
#define FLAG_USE_PROXY			16
#define FLAG_ALLOW_INVALID		32
#define FLAG_FAVORITE			64
#define FLAG_COUNT				7

/* DEFAULT_CHARSET is already defined in wingdi.h */
#define IRC_DEFAULT_CHARSET		"UTF-8 (Unicode)"

void servlist_init (void);
int servlist_save (void);
int servlist_cycle (server *serv);
void servlist_connect (session *sess, ircnet *net, gboolean join);
int servlist_connect_by_netname (session *sess, char *network, gboolean join);
int servlist_auto_connect (session *sess);
int servlist_have_auto (void);
int servlist_check_encoding (char *charset);
void servlist_cleanup (void);

ircnet *servlist_net_add (char *name, char *comment, int prepend);
void servlist_net_remove (ircnet *net);
ircnet *servlist_net_find (char *name, int *pos, int (*cmpfunc) (const char *, const char *));
ircnet *servlist_net_find_from_server (char *server_name);

void servlist_server_remove (ircnet *net, ircserver *serv);
ircserver *servlist_server_add (ircnet *net, char *name);
ircserver *servlist_server_find (ircnet *net, char *name, int *pos);

void joinlist_split (char *autojoin, GSList **channels, GSList **keys);
gboolean joinlist_is_in_list (server *serv, char *channel);
void joinlist_free (GSList *channels, GSList *keys);
gchar *joinlist_merge (GSList *channels, GSList *keys);

#endif
