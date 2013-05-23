/* HexChat
 * Copyright (C) 2013 Berke Viktor.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include "hexchat.h"
#include "hexchatc.h"
#include "server.h"
#include "profile.h"		/* brings us servlist.h too */
#include "cfgfiles.h"

#define PROFILE_CONF_NAME	"profile.conf"

GSList *profile_list = NULL;

static int
compare (const void *a, const void *b)
{
	return (*(int*) a - *(int*) b);
}

static int
profile_find_free_id (void)
{
	GSList *list;
	profile *prof;
	int *ids;
	int i;
	int j;
	int len = g_slist_length (profile_list);

	/* save the hassle for empty configs */
	if (len == 0)
	{
		return 1;
	}

	list = profile_list;
	ids = (int *) malloc (len * sizeof (int));
	i = 0;

	/* copy profile IDs to an int array for easy access */
	while (list)
	{
		prof = list->data;
		ids[i] = prof->id;
		i++;
		list = list->next;
	}

	/* quicksort the IDs */
	qsort (ids, len, sizeof (int), compare);

	/* go through the ID list, if there's a gap, we've found the lowest free ID */
	for (i = 0, j = 0; i < len; i++, j++)
	{
		/* IDs start at 1, not at 0 */
		if (ids[i] > j + 1)
		{
			g_free (ids);
			return j + 1;
		}
	}

	/* no gaps, return biggest + 1 */
	g_free (ids);
	return j + 1;
}

profile *
profile_add (char *name, char *nick1, char *nick2, char *nick3, char *user, char *real)
{
	profile *prof;

	if (!name)
	{
		return NULL;
	}

	prof = malloc (sizeof (profile));
	memset (prof, 0, sizeof (profile));

	prof->name = g_strdup (name);
	prof->id = profile_find_free_id ();
	prof->nickname1 = g_strdup (nick1);		/* g_strdup() handles NULL, no need to check */
	prof->nickname2 = g_strdup (nick2);
	prof->nickname3 = g_strdup (nick3);
	prof->username = g_strdup (user);
	prof->realname = g_strdup (real);

	profile_list = g_slist_append (profile_list, prof);

	return prof;
}

static void
profile_load_defaults (void)
{
	profile_add ("Profile1", "NewNick", "NewNick_", "NewNick__", "UserName", "RealName");
}

static gboolean
profile_load (void)
{
	FILE *fp;
	char buf[2048];
	int len;
	gboolean has_profiles = FALSE;		/* if the user removes all profiles by directly editing the file, make sure to create a new, empty one */
	profile *prof = NULL;

	fp = hexchat_fopen_file (PROFILE_CONF_NAME, "r", 0);

	if (!fp)
	{
		return FALSE;
	}

	while (fgets (buf, sizeof (buf) - 2, fp))
	{
		len = strlen (buf);
		buf[len] = 0;
		buf[len-1] = 0;					/* remove the trailing \n */

		if (prof)
		{
			switch (buf[0])
			{
				case 'X':
					prof->id = atoi (buf + 2);
					break;
				case '1':
					prof->nickname1 = g_strdup (buf + 2);
					break;
				case '2':
					prof->nickname2 = g_strdup (buf + 2);
					break;
				case '3':
					prof->nickname3 = g_strdup (buf + 2);
					break;
				case 'U':
					prof->username = g_strdup (buf + 2);
					break;
				case 'R':
					prof->realname = g_strdup (buf + 2);
					break;
			}
		}

		if (buf[0] == 'P')
		{
			prof = profile_add (buf + 2, NULL, NULL, NULL, NULL, NULL);
			has_profiles = TRUE;
		}
	}

	fclose (fp);
	return has_profiles;
}

void
profile_init (void)
{
	if (!profile_list)
	{
		if (!profile_load ())
		{
			profile_load_defaults ();
		}
	}
}

int
profile_save (void)
{
	FILE *fp;
	profile *prof;
	GSList *list;

#ifndef WIN32
	int first = FALSE;
	char *buf = g_build_filename (get_xdir (), PROFILE_CONF_NAME, NULL);

	if (g_access (buf, F_OK) != 0)
	{
		first = TRUE;
	}
#endif

	fp = hexchat_fopen_file (PROFILE_CONF_NAME, "w", 0);

	if (!fp)
	{
#ifndef WIN32
		g_free (buf);
#endif
		return FALSE;
	}

#ifndef WIN32
	if (first)
	{
		g_chmod (buf, 0600);
	}

	g_free (buf);
#endif

	list = profile_list;

	while (list)
	{
		prof = list->data;

		fprintf (fp, "P=%s\n", prof->name);
		fprintf (fp, "X=%d\n", prof->id);
		if (prof->nickname1)
			fprintf (fp, "1=%s\n", prof->nickname1);
		if (prof->nickname2)
			fprintf (fp, "2=%s\n", prof->nickname2);
		if (prof->nickname3)
			fprintf (fp, "3=%s\n", prof->nickname3);
		if (prof->username)
			fprintf (fp, "U=%s\n", prof->username);
		if (prof->realname)
			fprintf (fp, "R=%s\n", prof->realname);

		if (fprintf (fp, "\n") < 1)
		{
			fclose (fp);
			return FALSE;
		}

		list = list->next;
	}

	fclose (fp);
	return TRUE;
}

static profile *
profile_find_by_id (int id)
{
	profile *prof;
	GSList *list;

	list = profile_list;

	while (list)
	{
		prof = list->data;

		if (prof->id == id)
		{
			return prof;
		}

		list = list->next;
	}

	return NULL;
}

profile *
profile_find_default (void)
{
	profile *prof;
	GSList *list;

	prof = profile_find_by_id (prefs.hex_irc_default_profile);

	if (prof)
	{
		return prof;
	}
	else					/* default profile is set to an invalid value via /set or direct file edit, make the first profile we find the default */
	{
		list = profile_list;

		if (list)
		{
			prof = list->data;
			prefs.hex_irc_default_profile = prof->id;
			return prof;
		}
		else
		{
			return NULL;	/* this should never happen coz we don't allow the removal of the last profile */
		}
	}
}

profile *
profile_find_for_net (ircnet *net)
{
	profile *prof = NULL;

	if (!net)
	{
		return NULL;
	}

	if (net->account)
	{
		prof = profile_find_by_id (net->account);
	}

	/* If a profile ID is deleted, we reset network accounts, so prof should
	 * exist if net->account is set, but the user may set an invalid ID by
	 * directly editing the servlist file.
	 */
	if (!prof)
	{
		/* Profile is unset or invalid? Use the default profile! */
		prof = profile_find_default ();
	}

	return prof;
}

/* at this point profcache is already filled up, but we'll still check it, just in case */
profile *
profile_find_for_serv (server *serv)
{
	profile *prof = NULL;

	if (!serv)
	{
		return NULL;
	}

	if (serv->network)
	{
		prof = ((ircnet *)serv->network)->profcache;
	}
	/* this is the else, but this way we'll avoid the unlikely missing profcache tragedies */
	if (!prof)
	{
		prof = profile_find_default ();
	}

	return prof;
}

gboolean
profile_remove (profile *prof)
{
	GSList *list;
	ircnet *net;

	/* don't allow to remove the very last profile */
	if (g_slist_length (profile_list) > 1)
	{
		/* make sure that networks which used this profile will revert to the default profile, i.e. net->account is unset */
		list = network_list;

		while (list)
		{
			net = list->data;

			if (net->account == prof->id)
			{
				net->account = 0;
			}

			list = list->next;
		}

		profile_list = g_slist_remove (profile_list, prof);

		g_free (prof->name);
		/* these might be NULL but g_free() will take care of it */
		g_free (prof->nickname1);
		g_free (prof->nickname2);
		g_free (prof->nickname3);
		g_free (prof->username);
		g_free (prof->realname);
		g_free (prof);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
