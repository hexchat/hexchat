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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gtree.h>

#include "../../config.h"
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include "xchat.h"
#include "modes.h"
#include "fe.h"
#include "notify.h"
#include "xchatc.h"
#include "util.h"

#define USE_TREE	/* use binary tree storage? Fast, but more memory */

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
nick_cmp (server *serv, struct User *user1, struct User *user2)
{
	switch (prefs.userlist_sort)
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
		return 1;
	}
}

/*
 insert name in appropriate place in linked list. Returns
 row number or:
  -1: append (last row)
  -2: duplicate
*/

static int
userlist_insertname (session *sess, struct User *newuser, struct User **after)
{
	int c, row = 0;
	struct User *user;
	GSList *list = sess->userlist;
	GSList *prev = NULL;
	GSList *node = g_slist_alloc ();

	node->data = newuser;
	*after = NULL;


	while (list)
	{
		user = list->data;

		c = nick_cmp (sess->server, newuser, user);

		if (c == 0)
		{
			g_slist_free (node);
			return -2;	/* duplicate */
		}

		if (c < 0)
		{
			/* this saves a loop inside g_slist_insert */
			node->next = list;
			if (prev)
			{
				prev->next = node;
				*after = prev->data;
			} else
				sess->userlist = node;

#ifdef USE_TREE
			if (!sess->usertree)
				sess->usertree = g_tree_new ((GCompareFunc)strcasecmp);
			g_tree_insert (sess->usertree, newuser->nick, newuser);
#endif

			return row;
		}

		row++;
		prev = list;
		list = list->next;
	}

	/* avoid calling g_slist_last() */
	if (sess->userlist)
	{
		prev->next = node;
		*after = prev->data;
	} else
		sess->userlist = node;

#ifdef USE_TREE
	if (!sess->usertree)
		sess->usertree = g_tree_new ((GCompareFunc)strcasecmp);
	g_tree_insert (sess->usertree, newuser->nick, newuser);
#endif

	return -1;
}

static void
update_entry (struct session *sess, struct User *user)
{
	int row;
	struct User *after;

#ifdef USE_TREE
	g_tree_steal (sess->usertree, user->nick);
	g_tree_insert (sess->usertree, user->nick, user);
#endif

	sess->userlist = g_slist_remove (sess->userlist, user);
	row = userlist_insertname (sess, user, &after);

	fe_userlist_move (sess, user, row);
	fe_userlist_numbers (sess);
}

int
userlist_add_hostname (struct session *sess, char *nick, char *hostname,
							  char *realname, char *servername)
{
	struct User *user;

	user = find_name (sess, nick);
	if (user && !user->hostname)
	{
		user->hostname = strdup (hostname);
		if (!user->realname)
			user->realname = strdup (realname);
		if (!user->servername)
			user->servername = strdup (servername);
		if (prefs.showhostname_in_userlist)
			update_entry (sess, user);
		return 1;
	}
	return 0;
}

static void
free_user (struct User *user, GSList **list)
{
	if (user->realname)
		free (user->realname);
	if (user->hostname)
		free (user->hostname);
	if (user->servername)
		free (user->servername);
	free (user);
	*list = g_slist_remove (*list, user);
}

void
free_userlist (session *sess)
{
	struct User *user;

#ifdef USE_TREE
	if (sess->usertree)
	{
		g_tree_destroy (sess->usertree);
		sess->usertree = NULL;
	}
#endif

	while (sess->userlist)
	{
		user = (struct User *) sess->userlist->data;
		free_user (user, &sess->userlist);
	}
	sess->ops = 0;
	sess->hops = 0;
	sess->voices = 0;
	sess->total = 0;
}

void
clear_user_list (session *sess)
{
	fe_userlist_clear (sess);
	free_userlist (sess);
	fe_userlist_numbers (sess);
}

struct User *
find_name (struct session *sess, char *name)
{
#ifdef USE_TREE
	if (sess->usertree)
		return g_tree_lookup (sess->usertree, name);
#else
	struct User *user;
	GSList *list;

	list = sess->userlist;
	while (list)
	{
		user = (struct User *) list->data;
		if (!sess->server->p_cmp (user->nick, name))
			return user;
		list = list->next;
	}
#endif
	return NULL;
}

struct User *
find_name_global (struct server *serv, char *name)
{
	struct User *user;
	session *sess;
	GSList *list = sess_list;
	while (list)
	{
		sess = (session *) list->data;
		if (sess->server == serv)
		{
			user = find_name (sess, name);
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
ul_update_entry (session *sess, char *name, char mode, char sign)
{
	int access;
	int offset = 0;
	int level;
	char prefix;
	struct User *user;

	user = find_name (sess, name);
	if (!user)
		return;

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

	update_entry (sess, user);
}

int
change_nick (struct session *sess, char *oldname, char *newname)
{
	struct User *user = find_name (sess, oldname);
	if (user)
	{
		safe_strcpy (user->nick, newname, NICKLEN);
		update_entry (sess, user);
		return 1;
	}

	return 0;
}

int
sub_name (struct session *sess, char *name)
{
	struct User *user;

	user = find_name (sess, name);
	if (!user)
		return FALSE;

	if (user->voice)
		sess->voices--;
	if (user->op)
		sess->ops--;
	if (user->hop)
		sess->hops--;
	sess->total--;
	fe_userlist_numbers (sess);
	fe_userlist_remove (sess, user);

	free_user (user, &sess->userlist);
#ifdef USE_TREE
	g_tree_steal (sess->usertree, user->nick);
#endif

	return TRUE;
}

void
add_name (struct session *sess, char *name, char *hostname)
{
	struct User *user, *after;
	int row, prefix_chars;
	unsigned int acc;

	acc = nick_access (sess->server, name, &prefix_chars);

	notify_set_online (sess->server, name + prefix_chars);

	user = malloc (sizeof (struct User));
	memset (user, 0, sizeof (struct User));

	user->access = acc;

	/* assume first char is the highest level nick prefix */
	if (prefix_chars)
		user->prefix[0] = name[0];

	/* add it to our linked list */
	if (hostname)
		user->hostname = strdup (hostname);
	safe_strcpy (user->nick, name + prefix_chars, NICKLEN);
	/* is it me? */
	if (sess->server->p_cmp (user->nick, sess->server->nick))
		user->me = TRUE;
	row = userlist_insertname (sess, user, &after);

	/* duplicate? some broken servers trigger this */
	if (row == -2)
	{
		if (user->hostname)
			free (user->hostname);
		free (user);
		return;
	}

	sess->total++;

	/* most ircds don't support multiple modechars infront of the nickname
      for /NAMES - though they should. */
	while (prefix_chars)
	{
		update_counts (sess, user, name[0], TRUE, 1);
		name++;
		prefix_chars--;
	}

	fe_userlist_insert (sess, user, row, FALSE, after);
	fe_userlist_numbers (sess);
}

void
update_all_of (char *name)
{
#if 0
	struct User *user;
	struct session *sess;
	GSList *list = sess_list;
	while (list)
	{
		sess = (struct session *) list->data;
		user = find_name (sess, name);
		if (user)
		{
			update_entry (sess, user);
		}
		list = list->next;
	}
#endif
}
