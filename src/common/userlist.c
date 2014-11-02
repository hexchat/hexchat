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
#include <stdlib.h>
#include <string.h>

#include "hexchat.h"
#include "modes.h"
#include "fe.h"
#include "notify.h"
#include "tree.h"
#include "hexchatc.h"
#include "util.h"


static int
nick_cmp_az_ops (server *serv, struct User *user1, struct User *user2)
{
	unsigned int access1 = user1->access;
	unsigned int access2 = user2->access;
	int pos;

	if (access1 != access2)
	{
		for (pos = 0; pos < USERACCESS_SIZE; pos++)
		{
			if ((access1&(1<<pos)) && (access2&(1<<pos)))
				break;
			if ((access1&(1<<pos)) && !(access2&(1<<pos)))
				return -1;
			if (!(access1&(1<<pos)) && (access2&(1<<pos)))
				return 1;
		}
	}

	return serv->p_cmp (user1->nick, user2->nick);
}

static int
nick_cmp_alpha (struct User *user1, struct User *user2, server *serv)
{
	return serv->p_cmp (user1->nick, user2->nick);
}

static int
nick_cmp (struct User *user1, struct User *user2, server *serv)
{
	switch (prefs.hex_gui_ulist_sort)
	{
	case 0:
		return nick_cmp_az_ops (serv, user1, user2);
	case 1:
		return serv->p_cmp (user1->nick, user2->nick);
	case 2:
		return -1 * nick_cmp_az_ops (serv, user1, user2);
	case 3:
		return -1 * serv->p_cmp (user1->nick, user2->nick);
	default:
		return -1;
	}
}

/*
 insert name in appropriate place in linked list. Returns row number or:
  -1: duplicate
*/

static int
userlist_insertname (session *sess, struct User *newuser)
{
	if (!sess->usertree)
	{
		sess->usertree = tree_new ((tree_cmp_func *)nick_cmp, sess->server);
		sess->usertree_alpha = tree_new ((tree_cmp_func *)nick_cmp_alpha, sess->server);
	}

	tree_insert (sess->usertree_alpha, newuser);
	return tree_insert (sess->usertree, newuser);
}

void
userlist_set_away (struct session *sess, char *nick, unsigned int away)
{
	struct User *user;

	user = userlist_find (sess, nick);
	if (user)
	{
		if (user->away != away)
		{
			user->away = away;
			/* rehash GUI */
			fe_userlist_rehash (sess, user);
			if (away)
				fe_userlist_update (sess, user);
		}
	}
}

void
userlist_set_account (struct session *sess, char *nick, char *account)
{
	struct User *user;

	user = userlist_find (sess, nick);
	if (user)
	{
		if (user->account)
			free (user->account);
			
		if (strcmp (account, "*") == 0)
			user->account = NULL;
		else
			user->account = strdup (account);
			
		/* gui doesnt currently reflect login status, maybe later
		fe_userlist_rehash (sess, user); */
	}
}

int
userlist_add_hostname (struct session *sess, char *nick, char *hostname,
							  char *realname, char *servername, char *account, unsigned int away)
{
	struct User *user;
	gboolean do_rehash = FALSE;

	user = userlist_find (sess, nick);
	if (user)
	{
		if (!user->hostname && hostname)
		{
			if (prefs.hex_gui_ulist_show_hosts)
				do_rehash = TRUE;
			user->hostname = strdup (hostname);
		}
		if (!user->realname && realname && *realname)
			user->realname = strdup (realname);
		if (!user->servername && servername)
			user->servername = strdup (servername);
		if (!user->account && account && strcmp (account, "0") != 0)
			user->account = strdup (account);
		if (away != 0xff)
		{
			if (user->away != away)
				do_rehash = TRUE;
			user->away = away;
		}

		fe_userlist_update (sess, user);
		if (do_rehash)
			fe_userlist_rehash (sess, user);

		return 1;
	}
	return 0;
}

static int
free_user (struct User *user, gpointer data)
{
	if (user->realname)
		free (user->realname);
	if (user->hostname)
		free (user->hostname);
	if (user->servername)
		free (user->servername);
	if (user->account)
		free (user->account);
	free (user);

	return TRUE;
}

void
userlist_free (session *sess)
{
	tree_foreach (sess->usertree, (tree_traverse_func *)free_user, NULL);
	tree_destroy (sess->usertree);
	tree_destroy (sess->usertree_alpha);

	sess->usertree = NULL;
	sess->usertree_alpha = NULL;
	sess->me = NULL;

	sess->ops = 0;
	sess->hops = 0;
	sess->voices = 0;
	sess->total = 0;
}

void
userlist_clear (session *sess)
{
	fe_userlist_clear (sess);
	userlist_free (sess);
	fe_userlist_numbers (sess);
}

static int
find_cmp (const char *name, struct User *user, server *serv)
{
	return serv->p_cmp ((char *)name, user->nick);
}

struct User *
userlist_find (struct session *sess, const char *name)
{
	int pos;

	if (sess->usertree_alpha)
		return tree_find (sess->usertree_alpha, name,
								(tree_cmp_func *)find_cmp, sess->server, &pos);

	return NULL;
}

struct User *
userlist_find_global (struct server *serv, char *name)
{
	struct User *user;
	session *sess;
	GSList *list = sess_list;
	while (list)
	{
		sess = (session *) list->data;
		if (sess->server == serv)
		{
			user = userlist_find (sess, name);
			if (user)
				return user;
		}
		list = list->next;
	}
	return 0;
}

static void
update_counts (session *sess, struct User *user, char prefix,
					int level, int offset)
{
	switch (prefix)
	{
	case '@':
		user->op = level;
		sess->ops += offset;
		break;
	case '%':
		user->hop = level;
		sess->hops += offset;
		break;
	case '+':
		user->voice = level;
		sess->voices += offset;
		break;
	}
}

void
userlist_update_mode (session *sess, char *name, char mode, char sign)
{
	int access;
	int offset = 0;
	int level;
	int pos;
	char prefix;
	struct User *user;

	user = userlist_find (sess, name);
	if (!user)
		return;

	/* remove from binary trees, before we loose track of it */
	tree_remove (sess->usertree, user, &pos);
	tree_remove (sess->usertree_alpha, user, &pos);

	/* which bit number is affected? */
	access = mode_access (sess->server, mode, &prefix);

	if (sign == '+')
	{
		level = TRUE;
		if (!(user->access & (1 << access)))
		{
			offset = 1;
			user->access |= (1 << access);
		}
	} else
	{
		level = FALSE;
		if (user->access & (1 << access))
		{
			offset = -1;
			user->access &= ~(1 << access);
		}
	}

	/* now what is this users highest prefix? e.g. @ for ops */
	user->prefix[0] = get_nick_prefix (sess->server, user->access);

	/* update the various counts using the CHANGED prefix only */
	update_counts (sess, user, prefix, level, offset);

	/* insert it back into its new place */
	tree_insert (sess->usertree_alpha, user);
	pos = tree_insert (sess->usertree, user);

	/* let GTK move it too */
	fe_userlist_move (sess, user, pos);
	fe_userlist_numbers (sess);
}

int
userlist_change (struct session *sess, char *oldname, char *newname)
{
	struct User *user = userlist_find (sess, oldname);
	int pos;

	if (user)
	{
		tree_remove (sess->usertree, user, &pos);
		tree_remove (sess->usertree_alpha, user, &pos);

		safe_strcpy (user->nick, newname, NICKLEN);

		tree_insert (sess->usertree_alpha, user);

		fe_userlist_move (sess, user, tree_insert (sess->usertree, user));
		fe_userlist_numbers (sess);

		return 1;
	}

	return 0;
}

int
userlist_remove (struct session *sess, char *name)
{
	struct User *user;

	user = userlist_find (sess, name);
	if (!user)
		return FALSE;

	userlist_remove_user (sess, user);
	return TRUE;
}

void
userlist_remove_user (struct session *sess, struct User *user)
{
	int pos;
	if (user->voice)
		sess->voices--;
	if (user->op)
		sess->ops--;
	if (user->hop)
		sess->hops--;
	sess->total--;
	fe_userlist_numbers (sess);
	fe_userlist_remove (sess, user);

	if (user == sess->me)
		sess->me = NULL;

	tree_remove (sess->usertree, user, &pos);
	tree_remove (sess->usertree_alpha, user, &pos);
	free_user (user, NULL);
}

void
userlist_add (struct session *sess, char *name, char *hostname,
				  char *account, char *realname, const message_tags_data *tags_data)
{
	struct User *user;
	int row, prefix_chars;
	unsigned int acc;

	acc = nick_access (sess->server, name, &prefix_chars);

	notify_set_online (sess->server, name + prefix_chars, tags_data);

	user = calloc (1, sizeof (struct User));
	if (!user)
		return;

	user->access = acc;

	/* assume first char is the highest level nick prefix */
	if (prefix_chars)
		user->prefix[0] = name[0];

	/* add it to our linked list */
	if (hostname)
		user->hostname = strdup (hostname);
	safe_strcpy (user->nick, name + prefix_chars, NICKLEN);
	/* is it me? */
	if (!sess->server->p_cmp (user->nick, sess->server->nick))
		user->me = TRUE;
	/* extended join info */
	if (sess->server->have_extjoin)
	{
		if (account && *account)
			user->account = strdup (account);
		if (realname && *realname)
			user->realname = strdup (realname);
	}

	row = userlist_insertname (sess, user);

	/* duplicate? some broken servers trigger this */
	if (row == -1)
	{
		if (user->hostname)
			free (user->hostname);
		if (user->account)
			free (user->account);
		if (user->realname)
			free (user->realname);
		free (user);
		return;
	}

	sess->total++;

	/* most ircds don't support multiple modechars in front of the nickname
      for /NAMES - though they should. */
	while (prefix_chars)
	{
		update_counts (sess, user, name[0], TRUE, 1);
		name++;
		prefix_chars--;
	}

	if (user->me)
		sess->me = user;

	fe_userlist_insert (sess, user, row, FALSE);
	fe_userlist_numbers (sess);
}

static int
rehash_cb (struct User *user, session *sess)
{
	fe_userlist_rehash (sess, user);
	return TRUE;
}

void
userlist_rehash (session *sess)
{
	tree_foreach (sess->usertree_alpha, (tree_traverse_func *)rehash_cb, sess);
}

static int
flat_cb (struct User *user, GSList **list)
{
	*list = g_slist_prepend (*list, user);
	return TRUE;
}

GSList *
userlist_flat_list (session *sess)
{
	GSList *list = NULL;

	tree_foreach (sess->usertree_alpha, (tree_traverse_func *)flat_cb, &list);
	return g_slist_reverse (list);
}

static int
double_cb (struct User *user, GList **list)
{
	*list = g_list_prepend(*list, user);
	return TRUE;
}

GList *
userlist_double_list(session *sess)
{
	GList *list = NULL;

	tree_foreach (sess->usertree_alpha, (tree_traverse_func *)double_cb, &list);
	return list;
}
