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

#include "proto-irc.h"

#ifndef HEXCHAT_NOTIFY_H
#define HEXCHAT_NOTIFY_H

struct notify
{
	char *name;
	char *networks;	/* network names, comma sep */
	GSList *server_list;
};

struct notify_per_server
{
	struct server *server;
	struct notify *notify;
	time_t laston;
	time_t lastseen;
	time_t lastoff;
	unsigned int ison:1;
};

extern GSList *notify_list;
extern int notify_tag;

/* the WATCH stuff */
void notify_set_online (server * serv, char *nick,
								const message_tags_data *tags_data);
void notify_set_offline (server * serv, char *nick, int quiet,
								 const message_tags_data *tags_data);
/* the MONITOR stuff */
void notify_set_online_list (server * serv, char *users,
								const message_tags_data *tags_data);
void notify_set_offline_list (server * serv, char *users, int quiet,
								 const message_tags_data *tags_data);
void notify_send_watches (server * serv);

/* the general stuff */
void notify_adduser (char *name, char *networks);
int notify_deluser (char *name);
void notify_cleanup (void);
void notify_load (void);
void notify_save (void);
void notify_showlist (session *sess, const message_tags_data *tags_data);
gboolean notify_is_in_list (server *serv, char *name);
int notify_isnotify (session *sess, char *name);
struct notify_per_server *notify_find_server_entry (struct notify *notify, struct server *serv);

/* the old ISON stuff - remove me? */
void notify_markonline (server *serv, char *word[], 
								const message_tags_data *tags_data);
int notify_checklist (void);

#endif
