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

#include <time.h>
#include "proto-irc.h"

#ifndef HEXCHAT_USERLIST_H
#define HEXCHAT_USERLIST_H

struct User
{
	char nick[NICKLEN];
	char *hostname;
	char *realname;
	char *servername;
	char *account;
	time_t lasttalk;
	unsigned int access;	/* axs bit field */
	char prefix[2]; /* @ + % */
	unsigned int op:1;
	unsigned int hop:1;
	unsigned int voice:1;
	unsigned int me:1;
	unsigned int away:1;
	unsigned int selected:1;
};

#define USERACCESS_SIZE 12

int userlist_add_hostname (session *sess, char *nick,
									char *hostname, char *realname,
									char *servername, char *account, unsigned int away);
void userlist_set_away (session *sess, char *nick, unsigned int away);
void userlist_set_account (session *sess, char *nick, char *account);
struct User *userlist_find (session *sess, const char *name);
struct User *userlist_find_global (server *serv, char *name);
void userlist_clear (session *sess);
void userlist_free (session *sess);
void userlist_add (session *sess, char *name, char *hostname, char *account,
						 char *realname, const message_tags_data *tags_data);
int userlist_remove (session *sess, char *name);
void userlist_remove_user (session *sess, struct User *user);
int userlist_change (session *sess, char *oldname, char *newname);
void userlist_update_mode (session *sess, char *name, char mode, char sign);
GSList *userlist_flat_list (session *sess);
GList *userlist_double_list (session *sess);
void userlist_rehash (session *sess);

#endif
