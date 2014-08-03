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

#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "hexchat.h"
#include "ignore.hpp"
#include "cfgfiles.h"
#include "fe.h"
#include "text.h"
#include "util.h"
#include "hexchatc.h"
#include "typedef.h"


int ignored_ctcp = 0;			  /* keep a count of all we ignore */
int ignored_priv = 0;
int ignored_chan = 0;
int ignored_noti = 0;
int ignored_invi = 0;
static int ignored_total = 0;

/* ignore_exists ():
 * returns: struct ig, if this mask is in the ignore list already
 *          NULL, otherwise
 */
struct ignore *
ignore_exists (const std::string& mask)
{
	struct ignore *ig = nullptr;
	GSList *list;

	list = ignore_list;
	while (list)
	{
		ig = static_cast<struct ignore *>(list->data);
		if (!rfc_casecmp (ig->mask.c_str(), mask.c_str()))
			return ig;
		list = list->next;
	}
	return nullptr;

}

/* ignore_add(...)

 * returns:
 *            0 fail
 *            1 success
 *            2 success (old ignore has been changed)
 */

int
ignore_add(const std::string& mask, int type, bool overwrite)
{
	struct ignore *ig = NULL;
	bool change_only = false;

	/* first check if it's already ignored */
	ig = ignore_exists (mask);
	if (ig)
		change_only = true;

	if (!change_only)
		ig = new struct ignore();

	if (!ig)
		return 0;

	ig->mask = mask;

	if (!overwrite && change_only)
		ig->type |= type;
	else
		ig->type = type;

	if (!change_only)
		ignore_list = g_slist_prepend (ignore_list, ig);
	fe_ignore_update (1);

	if (change_only)
		return 2;

	return 1;
}

void
ignore_showlist (session *sess)
{
	struct ignore *ig;
	GSList *list = ignore_list;
	char tbuf[256];
	int i = 0;

	EMIT_SIGNAL (XP_TE_IGNOREHEADER, sess, 0, 0, 0, 0, 0);

	while (list)
	{
		ig = static_cast<ignore*>(list->data);
		i++;

		snprintf (tbuf, sizeof (tbuf), " %-25s ", ig->mask.c_str());
		if (ig->type & IG_PRIV)
			strcat (tbuf, _("YES  "));
		else
			strcat (tbuf, _("NO   "));
		if (ig->type & IG_NOTI)
			strcat (tbuf, _("YES  "));
		else
			strcat (tbuf, _("NO   "));
		if (ig->type & IG_CHAN)
			strcat (tbuf, _("YES  "));
		else
			strcat (tbuf, _("NO   "));
		if (ig->type & IG_CTCP)
			strcat (tbuf, _("YES  "));
		else
			strcat (tbuf, _("NO   "));
		if (ig->type & IG_DCC)
			strcat (tbuf, _("YES  "));
		else
			strcat (tbuf, _("NO   "));
		if (ig->type & IG_INVI)
			strcat (tbuf, _("YES  "));
		else
			strcat (tbuf, _("NO   "));
		if (ig->type & IG_UNIG)
			strcat (tbuf, _("YES  "));
		else
			strcat (tbuf, _("NO   "));
		strcat (tbuf, "\n");
		PrintText (sess, tbuf);
		/*EMIT_SIGNAL (XP_TE_IGNORELIST, sess, ig->mask, 0, 0, 0, 0); */
		/* use this later, when TE's support 7 args */
		list = list->next;
	}

	if (!i)
		EMIT_SIGNAL (XP_TE_IGNOREEMPTY, sess, 0, 0, 0, 0, 0);

	EMIT_SIGNAL (XP_TE_IGNOREFOOTER, sess, 0, 0, 0, 0, 0);
}

/* ignore_del()

 * one of the args must be NULL, use mask OR *ig, not both
 *
 */

bool
ignore_del(const std::string& mask, struct ignore *ig)
{
	if (!ig)
	{
		GSList *list = ignore_list;

		while (list)
		{
			ig = (struct ignore *) list->data;
			if (!rfc_casecmp (ig->mask.c_str(), mask.c_str()))
				break;
			list = list->next;
			ig = 0;
		}
	}
	if (ig)
	{
		ignore_list = g_slist_remove (ignore_list, ig);
		delete ig;
		fe_ignore_update (1);
		return true;
	}
	return false;
}

/* check if a msg should be ignored by browsing our ignore list */

bool
ignore_check(const std::string& mask, int type)
{
	struct ignore *ig;
	GSList *list = ignore_list;

	/* check if there's an UNIGNORE first, they take precendance. */
	while (list)
	{
		ig = static_cast<struct ignore *>(list->data);
		if (ig->type & IG_UNIG)
		{
			if (ig->type & type)
			{
				if (match (ig->mask.c_str(), mask.c_str()))
					return false;
			}
		}
		list = list->next;
	}

	list = ignore_list;
	while (list)
	{
		ig = static_cast<struct ignore *>(list->data);

		if (ig->type & type)
		{
			if (match (ig->mask.c_str(), mask.c_str()))
			{
				ignored_total++;
				if (type & IG_PRIV)
					ignored_priv++;
				if (type & IG_NOTI)
					ignored_noti++;
				if (type & IG_CHAN)
					ignored_chan++;
				if (type & IG_CTCP)
					ignored_ctcp++;
				if (type & IG_INVI)
					ignored_invi++;
				fe_ignore_update (2);
				return true;
			}
		}
		list = list->next;
	}

	return true;
}

static char *
ignore_read_next_entry (char *my_cfg, struct ignore *ignore)
{
	char tbuf[1024];

	/* Casting to char * done below just to satisfy compiler */

	if (my_cfg)
	{
		my_cfg = cfg_get_str (my_cfg, "mask", tbuf, sizeof (tbuf));
		if (!my_cfg)
			return NULL;
		ignore->mask = tbuf;
	}
	if (my_cfg)
	{
		my_cfg = cfg_get_str (my_cfg, "type", tbuf, sizeof (tbuf));
		ignore->type = atoi (tbuf);
	}
	return my_cfg;
}

void
ignore_load ()
{
	struct ignore *ignore;
	struct stat st;
	char *my_cfg;
	int fh, i;

	fh = hexchat_open_file ("ignore.conf", O_RDONLY, 0, 0);
	if (fh != -1)
	{
		fstat (fh, &st);
		if (st.st_size)
		{
			std::string cfg(st.st_size + 1, '\0');
			cfg[0] = '\0';
			i = read (fh, &cfg[0], st.st_size);
			if (i >= 0)
				cfg[i] = '\0';
			my_cfg = &cfg[0];
			while (my_cfg)
			{
				ignore = new struct ignore();
				if ((my_cfg = ignore_read_next_entry(my_cfg, ignore)))
					ignore_list = g_slist_prepend(ignore_list, ignore);
				else
					delete ignore;
			}
		}
		close (fh);
	}
}

void
ignore_save ()
{
	char buf[1024];
	int fh;
	GSList *temp = ignore_list;
	struct ignore *ig;

	fh = hexchat_open_file ("ignore.conf", O_TRUNC | O_WRONLY | O_CREAT, 0600, XOF_DOMODE);
	if (fh != -1)
	{
		while (temp)
		{
			ig = static_cast<struct ignore *>(temp->data);
			if (!(ig->type & IG_NOSAVE))
			{
				snprintf (buf, sizeof (buf), "mask = %s\ntype = %u\n\n",
							 ig->mask.c_str(), ig->type);
				write (fh, buf, strlen (buf));
			}
			temp = temp->next;
		}
		close (fh);
	}

}

static gboolean
flood_autodialog_timeout (gpointer data)
{
	prefs.hex_gui_autoopen_dialog = 1;
	return FALSE;
}

int
flood_check (char *nick, char *ip, server *serv, session *sess, flood_check_type what)	/*0=ctcp  1=priv */
{
	/*
	   serv
	   int ctcp_counter; 
	   time_t ctcp_last_time;
	   prefs
	   unsigned int ctcp_number_limit;
	   unsigned int ctcp_time_limit;
	 */
	char buf[512];
	char real_ip[132];
	int i;
	time_t current_time;
	current_time = time (NULL);

	if (what == flood_check_type::CTCP)
	{
		if (serv->ctcp_last_time == 0)	/*first ctcp in this server */
		{
			serv->ctcp_last_time = time (NULL);
			serv->ctcp_counter++;
		} else
		{
			if (difftime (current_time, serv->ctcp_last_time) < prefs.hex_flood_ctcp_time)	/*if we got the ctcp in the seconds limit */
			{
				serv->ctcp_counter++;
				if (serv->ctcp_counter == prefs.hex_flood_ctcp_num)	/*if we reached the maximun numbers of ctcp in the seconds limits */
				{
					serv->ctcp_last_time = current_time;	/*we got the flood, restore all the vars for next one */
					serv->ctcp_counter = 0;
					for (i = 0; i < 128; i++)
						if (ip[i] == '@')
							break;
					snprintf (real_ip, sizeof (real_ip), "*!*%s", &ip[i]);

					snprintf (buf, sizeof (buf),
								 _("You are being CTCP flooded from %s, ignoring %s\n"),
								 nick, real_ip);
					PrintText (sess, buf);

					/* ignore CTCP */
					ignore_add (real_ip, IG_CTCP, FALSE);
					return 0;
				}
			}
		}
	} else
	{
		if (serv->msg_last_time == 0)
		{
			serv->msg_last_time = time (NULL);
			serv->ctcp_counter++;
		} else
		{
			if (difftime (current_time, serv->msg_last_time) <
				 prefs.hex_flood_msg_time)
			{
				serv->msg_counter++;
				if (serv->msg_counter == prefs.hex_flood_msg_num)	/*if we reached the maximun numbers of ctcp in the seconds limits */
				{
					snprintf (buf, sizeof (buf),
					 _("You are being MSG flooded from %s, setting gui_autoopen_dialog OFF.\n"),
								 ip);
					PrintText (sess, buf);
					serv->msg_last_time = current_time;	/*we got the flood, restore all the vars for next one */
					serv->msg_counter = 0;

					if (prefs.hex_gui_autoopen_dialog)
					{
						prefs.hex_gui_autoopen_dialog = 0;
						/* turn it back on in 30 secs */
						fe_timeout_add (30000, flood_autodialog_timeout, NULL);
					}
					return 0;
				}
			}
		}
	}
	return 1;
}

