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

#define GTK_DISABLE_DEPRECATED

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "fe-gtk.h"

#include <gtk/gtkcheckmenuitem.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkstock.h>
#include <gdk/gdkkeysyms.h>

#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../common/cfgfiles.h"
#include "../common/outbound.h"
#include "../common/ignore.h"
#include "../common/fe.h"
#include "../common/server.h"
#include "../common/util.h"
#include "xtext.h"
#include "about.h"
#include "ascii.h"
#include "banlist.h"
#include "chanlist.h"
#include "editlist.h"
#include "fkeys.h"
#include "gtkutil.h"
#include "maingui.h"
#include "notifygui.h"
#include "pixmaps.h"
#include "rawlog.h"
#include "palette.h"
#include "plugingui.h"
#include "search.h"
#include "textgui.h"
#include "urlgrab.h"
#include "userlistgui.h"
#include "menu.h"

static GSList *submenu_list;

enum
{
	M_MENU,
	M_NEWMENU,
	M_END,
	M_SEP,
	M_MENUTOG,
	M_MENUD,
	M_MENUSTOCK,
	M_MENUPIX,
	M_MENUSUB,
};

struct mymenu
{
	char *text;
	void *callback;
	char *image;
	short type;
	unsigned int state:1;
	unsigned int activate:1;
	guint key;
};


void
goto_url (char *url)
{
#ifdef USE_GNOME
	gnome_url_show (url);
#else
#ifdef WIN32
	ShellExecute (0, "open", url, NULL, NULL, SW_SHOWNORMAL);
#else
	char tbuf[512], *moz;

	moz = g_find_program_in_path ("gnome-moz-remote");
	if (moz)
	{
		snprintf (tbuf, sizeof (tbuf), "%s %s", moz, url);
		g_free (moz);
	} else
	{
		snprintf (tbuf, sizeof (tbuf), "mozilla -remote 'openURL(%s)'", url);
	}
	xchat_exec (tbuf);
#endif
#endif
}

/* execute a userlistbutton/popupmenu command */

static void
nick_command (session * sess, char *cmd)
{
	if (*cmd == '!')
		xchat_exec (cmd + 1);
	else
		handle_command (sess, cmd, TRUE);
}

/* fill in the %a %s %n etc and execute the command */

void
nick_command_parse (session *sess, char *cmd, char *nick, char *allnick)
{
	char *buf;
	char *host = _("Host unknown");
	struct User *user;

	if (sess->type == SESS_DIALOG)
	{
		buf = (char *)(GTK_ENTRY (sess->gui->topic_entry)->text);
		buf = strrchr (buf, '@');
		if (buf)
			host = buf + 1;
	} else
	{
		user = find_name (sess, nick);
		if (user && user->hostname)
			host = strchr (user->hostname, '@') + 1;
	}

	/* this can't overflow, since popup->cmd is only 256 */
	buf = malloc (strlen (cmd) + strlen (nick) + strlen (allnick) + 512);

	auto_insert (buf, cmd, 0, 0, allnick, sess->channel, "", host,
						sess->server->nick, nick);

	nick_command (sess, buf);

	free (buf);
}

/* userlist button has been clicked */

void
userlist_button_cb (GtkWidget * button, char *cmd)
{
	int i, num_sel, using_allnicks = FALSE;
	char **nicks, *allnicks;
	char *nick = NULL;
	session *sess;

	sess = current_sess;

	if (strstr (cmd, "%a"))
		using_allnicks = TRUE;

	if (sess->type == SESS_DIALOG)
	{
		/* fake a selection */
		nicks = malloc (sizeof (char *) * 2);
		nicks[0] = sess->channel;
		nicks[1] = NULL;
	} else
	{
		/* find number of selected rows */
		nicks = userlist_selection_list (sess->gui->user_tree, &num_sel);
		if (num_sel < 1)
		{
			nick_command_parse (sess, cmd, "", "");
			return;
		}
	}

	/* create "allnicks" string */
	allnicks = malloc (((NICKLEN + 1) * num_sel) + 1);
	*allnicks = 0;

	i = 0;
	while (nicks[i])
	{
		if (i > 0)
			strcat (allnicks, " ");
		strcat (allnicks, nicks[i]);

		if (!nick)
			nick = nicks[0];

		/* if not using "%a", execute the command once for each nickname */
		if (!using_allnicks)
			nick_command_parse (sess, cmd, nicks[i], "");

		i++;
	}

	if (using_allnicks)
	{
		if (!nick)
			nick = "";
		nick_command_parse (sess, cmd, nick, allnicks);
	}

	if (nicks)
		free (nicks);

	free (allnicks);
}

/* a popup-menu-item has been selected */

static void
popup_menu_cb (GtkWidget * item, char *cmd)
{
	char *nick;

	/* the userdata is set in menu_quick_item() */
	nick = g_object_get_data (G_OBJECT (item), "u");

	if (!nick)	/* userlist popup menu */
	{
		/* treat it just like a userlist button */
		userlist_button_cb (NULL, cmd);
		return;
	}

	if (!current_sess)	/* for url grabber window */
		nick_command_parse (sess_list->data, cmd, nick, nick);
	else
		nick_command_parse (current_sess, cmd, nick, nick);
}

void
menu_toggle_item (char *label, GtkWidget *menu, void *callback, void *userdata,
						int state)
{
	GtkWidget *item;

	item = gtk_check_menu_item_new_with_label (label);
	gtk_check_menu_item_set_active ((GtkCheckMenuItem*)item, state);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (G_OBJECT (item), "activate",
							G_CALLBACK (callback), userdata);
	gtk_widget_show (item);
}

static GtkWidget *
menu_quick_item (char *cmd, char *label, GtkWidget * menu, int flags,
					  gpointer userdata)
{
	GtkWidget *item;
	if (!label)
		item = gtk_menu_item_new ();
	else
		item = gtk_menu_item_new_with_label (label);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_object_set_data (G_OBJECT (item), "u", userdata);
	if (cmd)
		g_signal_connect (G_OBJECT (item), "activate",
								G_CALLBACK (popup_menu_cb), cmd);
	if (flags & (1 << 0))
		gtk_widget_set_sensitive (GTK_WIDGET (item), FALSE);
	gtk_widget_show (item);

	return item;
}

static void
menu_quick_item_with_callback (void *callback, char *label, GtkWidget * menu,
										 void *arg)
{
	GtkWidget *item;

	item = gtk_menu_item_new_with_label (label);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (G_OBJECT (item), "activate",
							G_CALLBACK (callback), arg);
	gtk_widget_show (item);
}

static GtkWidget *
menu_quick_sub (char *name, GtkWidget * menu)
{
	GtkWidget *sub_menu;
	GtkWidget *sub_item;

	if (!name)
		return menu;

	/* Code to add a submenu */
	sub_menu = gtk_menu_new ();
	sub_item = gtk_menu_item_new_with_label (name);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), sub_item);
	gtk_widget_show (sub_item);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (sub_item), sub_menu);

	/* We create a new element in the list */
	submenu_list = g_slist_prepend (submenu_list, sub_menu);
	return (sub_menu);
}

static GtkWidget *
menu_quick_endsub ()
{
	/* Just delete the first element in the linked list pointed to by first */
	if (submenu_list)
		submenu_list = g_slist_remove (submenu_list, submenu_list->data);

	if (submenu_list)
		return (submenu_list->data);
	else
		return NULL;
}

static void
toggle_cb (GtkWidget *item, char *pref_name)
{
	char buf[256];

	if (GTK_CHECK_MENU_ITEM (item)->active)
		snprintf (buf, sizeof (buf), "set %s 1", pref_name);
	else
		snprintf (buf, sizeof (buf), "set %s 0", pref_name);

	handle_command (current_sess, buf, FALSE);
}

/* append items to "menu" using the (struct popup*) list provided */

static void
menu_create (GtkWidget *menu, GSList *list, char *target)
{
	struct popup *pop;
	GtkWidget *tempmenu = menu;

	submenu_list = g_slist_prepend (0, menu);
	while (list)
	{
		pop = (struct popup *) list->data;
		if (!strncasecmp (pop->name, "SUB", 3))
			tempmenu = menu_quick_sub (pop->cmd, tempmenu);
		else if (!strncasecmp (pop->name, "TOGGLE", 6))
		{
			menu_toggle_item (pop->name + 7, tempmenu, toggle_cb, pop->cmd,
									cfg_get_bool (pop->cmd));
		}
		else if (!strncasecmp (pop->name, "ENDSUB", 6))
		{
			if (tempmenu != menu)
				tempmenu = menu_quick_endsub ();
			/* If we get here and tempmenu equals menu that means we havent got any submenus to exit from */
		} else if (!strncasecmp (pop->name, "SEP", 3))
			menu_quick_item (0, 0, tempmenu, 1, 0);
		else
			menu_quick_item (pop->cmd, pop->name, tempmenu, 0, target);

		list = list->next;

	}

	/* Let's clean up the linked list from mem */
	while (submenu_list)
		submenu_list = g_slist_remove (submenu_list, submenu_list->data);
}

static void
menu_destroy (GtkObject *object, gpointer objtounref)
{
	gtk_widget_destroy (GTK_WIDGET (object));
	if (objtounref)
		g_object_unref (G_OBJECT (objtounref));
}

static void
menu_popup (GtkWidget *menu, GdkEventButton *event, gpointer objtounref)
{
	g_signal_connect (G_OBJECT (menu), "selection-done",
							G_CALLBACK (menu_destroy), objtounref);
	if (event == NULL)
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, 0);
	else
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
							 0, event->time);
}

static char *str_copy = 0;		/* for all pop-up menus */

void
menu_nickmenu (session *sess, GdkEventButton *event, char *nick, int num_sel)
{
	char buf[256];
	struct User *user;
	GtkWidget *wid, *submenu, *menu = gtk_menu_new ();

	if (str_copy)
		free (str_copy);
	str_copy = strdup (nick);

	submenu_list = 0;	/* first time though, might not be 0 */

	/* more than 1 nick selected? */
	if (num_sel > 1)
	{
		snprintf (buf, sizeof (buf), "%d nicks selected.", num_sel);
		menu_quick_item (0, buf, menu, 0, 0);
		menu_quick_item (0, 0, menu, 1, 0);
	} else
	{
		user = find_name_global (current_sess->server, nick);
		if (user)
		{
			submenu = menu_quick_sub (nick, menu);

			snprintf (buf, sizeof (buf), _("User: %s"),
						user->hostname ? user->hostname : _("Unknown"));
			menu_quick_item (0, buf, submenu, 0, 0);

			snprintf (buf, sizeof (buf), _("Country: %s"),
						user->hostname ? country(user->hostname) : _("Unknown"));
			menu_quick_item (0, buf, submenu, 0, 0);

			snprintf (buf, sizeof (buf), _("Realname: %s"),
						user->realname ? user->realname : _("Unknown"));
			menu_quick_item (0, buf, submenu, 0, 0);

			snprintf (buf, sizeof (buf), _("Server: %s"),
						user->servername ? user->servername : _("Unknown"));
			menu_quick_item (0, buf, submenu, 0, 0);

			snprintf (buf, sizeof (buf), _("Last Msg: %s"),
						user->lasttalk ? ctime (&(user->lasttalk)) : _("Unknown"));
			if (user->lasttalk)
				buf[strlen (buf) - 1] = 0;
			wid = menu_quick_item (0, buf, submenu, 0, 0);

			gtk_label_set_justify (GTK_LABEL (GTK_BIN (wid)->child), GTK_JUSTIFY_LEFT);
			menu_quick_endsub ();
			menu_quick_item (0, 0, menu, 1, 0);
		}
	}

	if (num_sel > 1)
		menu_create (menu, popup_list, NULL);
	else
		menu_create (menu, popup_list, str_copy);
	menu_popup (menu, event, NULL);
}

static void
menu_showhide (void)
{
	session *sess;
	GSList *list;

	list = sess_list;
	while (list)
	{
		sess = list->data;

		if (sess->gui->menu)
		{
			if (prefs.hidemenu)
				gtk_widget_hide (sess->gui->menu);
			else
				gtk_widget_show (sess->gui->menu);
		}

		list = list->next;
	}
}

static void
menu_middle_cb (GtkWidget *item, int n)
{
	switch (n)
	{
	case 0:
		if (prefs.hidemenu)
			prefs.hidemenu = 0;
		else
			prefs.hidemenu = 1;
		menu_showhide ();
		break;
	case 1:
		mg_showhide_topic (current_sess);
		break;
	case 2:
		mg_userlist_toggle ();
		break;
	}
}

void
menu_middlemenu (session *sess, GdkEventButton *event)
{
	GtkWidget *menu, *away, *user;
	GtkAccelGroup *accel_group;

	accel_group = gtk_accel_group_new ();
	menu = menu_create_main (accel_group, FALSE, sess->server->is_away, &away, &user);

	menu_quick_item (0, 0, menu, 1, 0);	/* sep */

	menu_toggle_item (_("Menu Bar"), menu, menu_middle_cb, 0, !prefs.hidemenu);
	menu_toggle_item (_("Topic Bar"), menu, menu_middle_cb, (void*)1, prefs.topicbar);
	menu_toggle_item (_("User List"), menu, menu_middle_cb, (void*)2, !prefs.hideuserlist);

	menu_popup (menu, event, accel_group);
}

#ifdef WIN32
static void
open_url_cb (GtkWidget *item, char *url)
{
	goto_url (url);
}
#endif

void
menu_urlmenu (GdkEventButton *event, char *url)
{
	GtkWidget *menu;
	char *tmp;

	if (str_copy)
		free (str_copy);

#if 0
	/* replace all commas with %2c */
	str_copy = malloc ((strlen (url) * 3) + 1);
	i = 0;
	while (*url)
	{
		if (*url == ',')
		{
			str_copy[i++] = '%';
			str_copy[i++] = '2';
			str_copy[i] = 'c';
#ifdef WIN32
		} else if (*url == '\\')
		{
			str_copy[i++] = '\\';
			str_copy[i] = '\\';
#endif
		} else
			str_copy[i] = *url;
		i++;
		url++;
	}
	str_copy[i] = 0;
#else
	str_copy = strdup (url);
#endif

	menu = gtk_menu_new ();
	if (strlen (str_copy) > 51)
	{
		tmp = strdup (str_copy);
		tmp[47] = tmp[48] = tmp[49] = '.';
		tmp[50] = 0;
		menu_quick_item (0, tmp, menu, 1, 0);
		free (tmp);
	} else
	{
		menu_quick_item (0, str_copy, menu, 1, 0);
	}
	menu_quick_item (0, 0, menu, 1, 0);

#ifdef WIN32
	menu_quick_item_with_callback (open_url_cb, "Open URL", menu, str_copy);
#endif

	menu_create (menu, urlhandler_list, str_copy);
	menu_popup (menu, event, NULL);
}

static void
menu_chan_cycle (GtkWidget * menu, char *chan)
{
	if (current_sess)
		handle_command (current_sess, "cycle", FALSE);
}

static void
menu_chan_part (GtkWidget * menu, char *chan)
{
	char tbuf[256];

	if (current_sess)
	{
		snprintf (tbuf, sizeof tbuf, "part %s", chan);
		handle_command (current_sess, tbuf, FALSE);
	}
}

static void
menu_chan_join (GtkWidget * menu, char *chan)
{
	char tbuf[256];

	if (current_sess)
	{
		snprintf (tbuf, sizeof tbuf, "join %s", chan);
		handle_command (current_sess, tbuf, FALSE);
	}
}

void
menu_chanmenu (struct session *sess, GdkEventButton * event, char *chan)
{
	GtkWidget *menu;
	int is_joined = FALSE;

	if (find_channel (sess->server, chan))
		is_joined = TRUE;

	if (str_copy)
		free (str_copy);
	str_copy = strdup (chan);

	menu = gtk_menu_new ();

	menu_quick_item (0, chan, menu, 1, str_copy);
	menu_quick_item (0, 0, menu, 1, str_copy);

	if (!is_joined)
		menu_quick_item_with_callback (menu_chan_join, _("Join Channel"), menu,
												 str_copy);
	else
	{
		menu_quick_item_with_callback (menu_chan_part, _("Part Channel"), menu,
												 str_copy);
		menu_quick_item_with_callback (menu_chan_cycle, _("Cycle Channel"), menu,
												 str_copy);
	}

	menu_popup (menu, event, NULL);
}

static void
menu_open_server_list (GtkWidget *wid, gpointer none)
{
	fe_serverlist_open (current_sess);
}

static void
menu_settings (GtkWidget * wid, gpointer none)
{
	extern void setup_open (void);
	setup_open ();
}

static void
menu_usermenu (void)
{
	editlist_gui_open (usermenu_list, _("X-Chat: User menu"), "usermenu",
							 "usermenu.conf", 0);
}

static void
usermenu_create (GtkWidget *menu)
{
	menu_create (menu, usermenu_list, "");
	menu_quick_item (0, 0, menu, 1, 0);	/* sep */
	menu_quick_item_with_callback (menu_usermenu, _("Edit This Menu..."), menu, 0);
}

static void
usermenu_destroy (GtkWidget * menu)
{
	GList *items = ((GtkMenuShell *) menu)->children;
	GList *next;

	while (items)
	{
		next = items->next;
		gtk_widget_destroy (items->data);
		items = next;
	}
}

void
usermenu_update (void)
{
	int done_main = FALSE;
	GSList *list = sess_list;
	session *sess;

	while (list)
	{
		sess = list->data;
		if (sess->gui->is_tab)
		{
			if (!done_main)
			{
				if (sess->gui->user_menu)
				{
					usermenu_destroy (sess->gui->user_menu);
					usermenu_create (sess->gui->user_menu);
					done_main = TRUE;
				}
			}
		} else
		{
			usermenu_destroy (sess->gui->user_menu);
			usermenu_create (sess->gui->user_menu);
		}
		list = list->next;
	}
}

static void
menu_newserver_window (GtkWidget * wid, gpointer none)
{
	int old = prefs.tabchannels;

	prefs.tabchannels = 0;
	new_ircwindow (NULL, NULL, SESS_SERVER);
	prefs.tabchannels = old;
}

static void
menu_newchannel_window (GtkWidget * wid, gpointer none)
{
	int old = prefs.tabchannels;

	prefs.tabchannels = 0;
	new_ircwindow (current_sess->server, NULL, SESS_CHANNEL);
	prefs.tabchannels = old;
}

static void
menu_newserver_tab (GtkWidget * wid, gpointer none)
{
	int old = prefs.tabchannels;

	prefs.tabchannels = 1;
	new_ircwindow (NULL, NULL, SESS_SERVER);
	prefs.tabchannels = old;
}

static void
menu_newchannel_tab (GtkWidget * wid, gpointer none)
{
	int old = prefs.tabchannels;

	prefs.tabchannels = 1;
	new_ircwindow (current_sess->server, NULL, SESS_CHANNEL);
	prefs.tabchannels = old;
}

static void
menu_rawlog (GtkWidget * wid, gpointer none)
{
	open_rawlog (current_sess->server);
}

static void
menu_autodccsend (GtkWidget * wid, gpointer none)
{
	prefs.autodccsend = !prefs.autodccsend;
#ifndef WIN32
	if (prefs.autodccsend)
	{
		if (!strcmp ((char *)g_get_home_dir (), prefs.dccdir))
		{
			gtkutil_simpledialog (_("*WARNING*\n"
										 "Auto accepting DCC to your home directory\n"
										 "can be dangerous and is exploitable. Eg:\n"
										 "Someone could send you a .bash_profile"));
		}
	}
#endif
}

static void
menu_detach (GtkWidget * wid, gpointer none)
{
	mg_link_cb (NULL, NULL);
}

static void
menu_close (GtkWidget * wid, gpointer none)
{
	mg_x_click_cb (NULL, NULL);
}

static void
menu_search ()
{
	search_open (current_sess);
}

static void
menu_flushbuffer (GtkWidget * wid, gpointer none)
{
	fe_text_clear (current_sess);
}

static void
savebuffer_req_done (session *sess, void *arg2, char *file)
{
	int fh;

	if (!file)
		return;

	fh = open (file, O_TRUNC | O_WRONLY | O_CREAT, 0600);
	if (fh != -1)
	{
		gtk_xtext_save (GTK_XTEXT (sess->gui->xtext), fh);
		close (fh);
	}
}

static void
menu_savebuffer (GtkWidget * wid, gpointer none)
{
	gtkutil_file_req (_("Select an output filename"), savebuffer_req_done,
							current_sess, 0, TRUE);
}

static void
menu_wallops (GtkWidget * wid, gpointer none)
{
	char mode[3];
	server *serv = current_sess->server;
	prefs.wallops = !prefs.wallops;
	if (serv->connected)
	{
		mode[1] = 'w';
		mode[2] = '\0';
		if (prefs.wallops)
			mode[0] = '+';
		else
			mode[0] = '-';
		serv->p_nick_mode (serv, serv->nick, mode);
	}
}

static void
menu_servernotice (GtkWidget * wid, gpointer none)
{
	char mode[3];
	server *serv = current_sess->server;
	prefs.servernotice = !prefs.servernotice;
	if (serv->connected)
	{
		mode[1] = 's';
		mode[2] = '\0';
		if (prefs.servernotice)
			mode[0] = '+';
		else
			mode[0] = '-';
		serv->p_nick_mode (serv, serv->nick, mode);
	}
}

static void
menu_away (GtkWidget * wid, gpointer none)
{
	handle_command (current_sess, "away", FALSE);
}

static void
menu_invisible (GtkWidget * wid, gpointer none)
{
	char mode[3];
	server *serv = current_sess->server;
	prefs.invisible = !prefs.invisible;
	if (current_sess->server->connected)
	{
		mode[1] = 'i';
		mode[2] = '\0';
		if (prefs.invisible)
			mode[0] = '+';
		else
			mode[0] = '-';
		serv->p_nick_mode (serv, serv->nick, mode);
	}
}

#if 0
static void
menu_savedefault (GtkWidget * wid, gpointer none)
{
	palette_save ();
	if (save_config ())
		gtkutil_simpledialog (_("Settings saved."));
}
#endif

static void
menu_chanlist (GtkWidget * wid, gpointer none)
{
	chanlist_opengui (current_sess->server);
}

static void
menu_banlist (GtkWidget * wid, gpointer none)
{
	banlist_opengui (current_sess);
}

#ifdef USE_PLUGIN

static void
menu_loadplugin (void)
{
	plugingui_load ();
}

static void
menu_pluginlist (void)
{
	plugingui_open ();
}

#else

#define menu_pluginlist 0
#define menu_loadplugin 0

#endif

#define usercommands_help  _("User Commands - Special codes:\n\n"\
                           "%c  =  current channel\n"\
									"%m  =  machine info\n"\
                           "%n  =  your nick\n"\
									"%t  =  time/date\n"\
                           "%v  =  xchat version\n"\
                           "%2  =  word 2\n"\
                           "%3  =  word 3\n"\
                           "&2  =  word 2 to the end of line\n"\
                           "&3  =  word 3 to the end of line\n\n"\
                           "eg:\n"\
                           "/cmd john hello\n\n"\
                           "%2 would be \042john\042\n"\
                           "&2 would be \042john hello\042.")

#define ulbutton_help       _("Userlist Buttons - Special codes:\n\n"\
                           "%a  =  all selected nicks\n"\
                           "%c  =  current channel\n"\
                           "%h  =  selected nick's hostname\n"\
									"%m  =  machine info\n"\
                           "%n  =  your nick\n"\
                           "%s  =  selected nick\n"\
									"%t  =  time/date\n")

#define dlgbutton_help      _("Dialog Buttons - Special codes:\n\n"\
                           "%a  =  all selected nicks\n"\
                           "%c  =  current channel\n"\
                           "%h  =  selected nick's hostname\n"\
									"%m  =  machine info\n"\
                           "%n  =  your nick\n"\
                           "%s  =  selected nick\n"\
									"%t  =  time/date\n")

#define ctcp_help          _("CTCP Replies - Special codes:\n\n"\
                           "%d  =  data (the whole ctcp)\n"\
									"%m  =  machine info\n"\
                           "%s  =  nick who sent the ctcp\n"\
                           "%t  =  time/date\n"\
                           "%2  =  word 2\n"\
                           "%3  =  word 3\n"\
                           "&2  =  word 2 to the end of line\n"\
                           "&3  =  word 3 to the end of line\n\n")

#define url_help           _("URL Handlers - Special codes:\n\n"\
                           "%s  =  the URL string\n\n"\
                           "Putting a ! infront of the command\n"\
                           "indicates it should be sent to a\n"\
                           "shell instead of X-Chat")

static void
menu_usercommands (void)
{
	editlist_gui_open (command_list, _("X-Chat: User Defined Commands"),
							 "commands", "commands.conf", usercommands_help);
}

static void
menu_ulpopup (void)
{
	editlist_gui_open (popup_list, _("X-Chat: Userlist Popup menu"), "popup",
							 "popup.conf", ulbutton_help);
}

static void
menu_rpopup (void)
{
	editlist_gui_open (replace_list, _("X-Chat: Replace"), "replace",
							 "replace.conf", 0);
}

static void
menu_urlhandlers (void)
{
	editlist_gui_open (urlhandler_list, _("X-Chat: URL Handlers"), "urlhandlers",
							 "urlhandlers.conf", url_help);
}

static void
menu_evtpopup (void)
{
	pevent_dialog_show ();
}

static void
menu_keypopup (void)
{
	key_dialog_show ();
}

static void
menu_ulbuttons (void)
{
	editlist_gui_open (button_list, _("X-Chat: Userlist buttons"), "buttons",
							 "buttons.conf", ulbutton_help);
}

static void
menu_dlgbuttons (void)
{
	editlist_gui_open (dlgbutton_list, _("X-Chat: Dialog buttons"), "dlgbuttons",
							 "dlgbuttons.conf", dlgbutton_help);
}

static void
menu_ctcpguiopen (void)
{
	editlist_gui_open (ctcp_list, _("X-Chat: CTCP Replies"), "ctcpreply",
							 "ctcpreply.conf", ctcp_help);
}

#if 0
static void
menu_reload (void)
{
	char *buf = malloc (strlen (default_file ()) + 12);
	load_config ();
	sprintf (buf, "%s reloaded.", default_file ());
	fe_message (buf, FALSE);
	free (buf);
}
#endif

static void
menu_autorejoin (GtkWidget *wid, gpointer none)
{
	prefs.autorejoin = !prefs.autorejoin;
}

static void
menu_autoreconnect (GtkWidget *wid, gpointer none)
{
	prefs.autoreconnect = !prefs.autoreconnect;
}

static void
menu_autoreconnectonfail (GtkWidget *wid, gpointer none)
{
	prefs.autoreconnectonfail = !prefs.autoreconnectonfail;
}

static void
menu_autodialog (GtkWidget *wid, gpointer none)
{
	if (GTK_CHECK_MENU_ITEM (wid)->active)
		prefs.autodialog = 1;
	else
		prefs.autodialog = 0;
}

static void
menu_autodccchat (GtkWidget *wid, gpointer none)
{
	prefs.autodccchat = !prefs.autodccchat;
}

#if 0
static void
menu_saveexit (GtkWidget *wid, gpointer none)
{
	prefs.autosave = !prefs.autosave;
}
#endif

static void
menu_docs (GtkWidget *wid, gpointer none)
{
	goto_url ("http://xchat.org/docs.html");
}

/*static void
menu_webpage (GtkWidget *wid, gpointer none)
{
	goto_url ("http://xchat.org");
}*/

static void
menu_dcc_recv_win (GtkWidget *wid, gpointer none)
{
	fe_dcc_open_recv_win (FALSE);
}

static void
menu_dcc_send_win (GtkWidget *wid, gpointer none)
{
	fe_dcc_open_send_win (FALSE);
}

static void
menu_dcc_chat_win (GtkWidget *wid, gpointer none)
{
	fe_dcc_open_chat_win (FALSE);
}

static struct mymenu mymenu[] = {
	{N_("_X-Chat"), 0, 0, M_NEWMENU, 0, 1},
	{N_("Server List..."), menu_open_server_list, (char *)&pix_book, M_MENUPIX, 0, 1, GDK_s},
	{0, 0, 0, M_SEP, 0, 0},

	{N_("New"), 0, GTK_STOCK_NEW, M_MENUSUB, 0, 1},
	{N_("Server Tab..."), menu_newserver_tab, 0, M_MENU, 0, 1, GDK_t},
	{N_("Channel Tab..."), menu_newchannel_tab, 0, M_MENU, 0, 1},
	{N_("Server Window..."), menu_newserver_window, 0, M_MENU, 0, 1},
	{N_("Channel Window..."), menu_newchannel_window, 0, M_MENU, 0, 1},
	{0, 0, 0, M_END, 0, 0},
	{0, 0, 0, M_SEP, 0, 0},

#ifdef USE_PLUGIN
	{N_("Load Plugin..."), menu_loadplugin, GTK_STOCK_REVERT_TO_SAVED, M_MENUSTOCK, 0, 1},
#else
	{N_("Load Plugin..."), 0, GTK_STOCK_REVERT_TO_SAVED, M_MENUSTOCK, 0, 0},
#endif
	{0, 0, 0, M_SEP, 0, 0},	/* 11 */
#ifdef USE_ZVT
	{N_("New Shell Tab..."), menu_newshell_tab, 0, M_MENU, 0, 1},
	{0, 0, 0, M_SEP, 0, 0},
#define menuoffset 0
#else
#define menuoffset 2
#endif
	{N_("Detach Tab"), menu_detach, GTK_STOCK_REDO, M_MENUSTOCK, 0, 1, GDK_I},
	{N_("Close Tab"), menu_close, GTK_STOCK_CLOSE, M_MENUSTOCK, 0, 1, GDK_w},
	{0, 0, 0, M_SEP, 0, 0},
	{N_("Quit"), mg_safe_quit, GTK_STOCK_QUIT, M_MENUSTOCK, 0, 1, GDK_q},	/* 17 */

	{N_("_IRC"), 0, 0, M_NEWMENU, 0, 1},
	{N_("Invisible"), menu_invisible, 0, M_MENUTOG, 1, 1},
	{N_("Receive Wallops"), menu_wallops, 0, M_MENUTOG, 1, 1},
	{N_("Receive Server Notices"), menu_servernotice, 0, M_MENUTOG, 1, 1},
	{0, 0, 0, M_SEP, 0, 0},
	{N_("Marked Away"), menu_away, 0, M_MENUTOG, 0, 1, GDK_a},
	{0, 0, 0, M_SEP, 0, 0},														/* 24 */
	{N_("Auto Rejoin when Kicked"), menu_autorejoin, 0, M_MENUTOG, 0, 1},
	{N_("Auto Reconnect to Server"), menu_autoreconnect, 0, M_MENUTOG, 0, 1},
	{N_("Never-give-up ReConnect"), menu_autoreconnectonfail, 0, M_MENUTOG, 0, 1},
	{0, 0, 0, M_SEP, 0, 0},														/* 28 */
	{N_("Auto Open Dialog Windows"), menu_autodialog, 0, M_MENUTOG, 0, 1},
	{N_("Auto Accept Direct Chat"), menu_autodccchat, 0, M_MENUTOG, 0, 1},
	{N_("Auto Accept Files"), menu_autodccsend, 0, M_MENUTOG, 0, 1},

	{N_("_Server"), (void *) -1, 0, M_NEWMENU, 0, 1},	/* 32 */

	{N_("S_ettings"), 0, 0, M_NEWMENU, 0, 1},	/* 33 */
	{N_("Preferences..."), menu_settings, GTK_STOCK_PREFERENCES, M_MENUSTOCK, 0, 1},

	{N_("Lists"), 0, GTK_STOCK_JUSTIFY_LEFT, M_MENUSUB, 0, 1},
	{N_("User Commands..."), menu_usercommands, 0, M_MENU, 0, 1},
	{N_("CTCP Replies..."), menu_ctcpguiopen, 0, M_MENU, 0, 1},
	{N_("Userlist Buttons..."), menu_ulbuttons, 0, M_MENU, 0, 1},
	{N_("Userlist Popup..."), menu_ulpopup, 0, M_MENU, 0, 1},
	{N_("Dialog Buttons..."), menu_dlgbuttons, 0, M_MENU, 0, 1},
	{N_("Replace Popup..."), menu_rpopup, 0, M_MENU, 0, 1},
	{N_("URL Handlers..."), menu_urlhandlers, 0, M_MENU, 0, 1},
	{N_("Event Texts..."), menu_evtpopup, 0, M_MENU, 0, 1},
	{N_("Key Bindings..."), menu_keypopup, 0, M_MENU, 0, 1},	/* 44 */
	{0, 0, 0, M_END, 0, 0},

#if 0
	{0, 0, 0, M_SEP, 0, 0},	/* 56 */
	{N_("Reload Settings"), menu_reload, GTK_STOCK_REVERT_TO_SAVED, M_MENUSTOCK, 0, 1},
	{0, 0, 0, M_SEP, 0, 0},
	{N_("Save Settings now"), menu_savedefault, GTK_STOCK_SAVE, M_MENUSTOCK, 0, 1},
	{N_("Save Settings on exit"), menu_saveexit, 0, M_MENUTOG, 1, 1},
#endif

	{N_("_Window"), 0, 0, M_NEWMENU, 0, 1},
	{N_("Channel List..."), menu_chanlist, 0, M_MENU, 0, 1},
	{N_("File Send..."), menu_dcc_send_win, 0, M_MENU, 0, 1},
	{N_("File Receive..."), menu_dcc_recv_win, 0, M_MENU, 0, 1},
	{N_("Direct Chat..."), menu_dcc_chat_win, 0, M_MENU, 0, 1},
	{N_("Raw Log..."), menu_rawlog, 0, M_MENU, 0, 1},	/* 51 */
	{N_("URL Grabber..."), url_opengui, 0, M_MENU, 0, 1},
	{N_("Notify List..."), notify_opengui, 0, M_MENU, 0, 1},
	{N_("Ignore List..."), ignore_gui_open, 0, M_MENU, 0, 1},
	{N_("Ban List..."), menu_banlist, 0, M_MENU, 0, 1},
	{N_("Character Chart..."), ascii_open, 0, M_MENU, 0, 1},
	{N_("Plugin List..."), menu_pluginlist, 0, M_MENU, 0, 1},
	{0, 0, 0, M_SEP, 0, 0},
	{N_("C_lear Text"), menu_flushbuffer, GTK_STOCK_CLEAR, M_MENUSTOCK, 0, 1, GDK_l},
	{N_("Search Text..."), menu_search, GTK_STOCK_FIND, M_MENUSTOCK, 0, 1, GDK_f},
	{N_("Save Text..."), menu_savebuffer, GTK_STOCK_SAVE, M_MENUSTOCK, 0, 1},

	{N_("_Help"), 0, 0, M_NEWMENU, 0, 1},	/* 62 */
	{N_("_Contents"), menu_docs, GTK_STOCK_HELP, M_MENUSTOCK, 0, 1, GDK_F1},
	{N_("_About"), menu_about, 0, M_MENU, 0, 1},

	{0, 0, 0, M_END, 0, 0},
};

GtkWidget *
create_icon_menu (char *labeltext, void *stock_name, int is_stock)
{
	GtkWidget *item, *img;

	if (is_stock)
		img = gtk_image_new_from_stock (stock_name, GTK_ICON_SIZE_MENU);
	else
		img = gtk_image_new_from_pixbuf (*((GdkPixbuf **)stock_name));
	item = gtk_image_menu_item_new_with_mnemonic (labeltext);
	gtk_image_menu_item_set_image ((GtkImageMenuItem *)item, img);
	gtk_widget_show (img);

	return item;
}

GtkWidget *
menu_create_main (void *accel_group, int bar, int away,
						GtkWidget **away_item_ret, GtkWidget **user_menu_ret)
{
	int i = 0;
	GtkWidget *item;
	GtkWidget *menu = 0;
	GtkWidget *menu_item = 0;
	GtkWidget *menu_bar;
	GtkWidget *usermenu = 0;
	GtkWidget *submenu = 0;

	if (bar)
		menu_bar = gtk_menu_bar_new ();
	else
		menu_bar = gtk_menu_new ();

	mymenu[19-menuoffset].state = prefs.invisible;
	mymenu[20-menuoffset].state = prefs.wallops;
	mymenu[21-menuoffset].state = prefs.servernotice;
	mymenu[23-menuoffset].state = away;
	mymenu[25-menuoffset].state = prefs.autorejoin;
	mymenu[26-menuoffset].state = prefs.autoreconnect;
	mymenu[27-menuoffset].state = prefs.autoreconnectonfail;
	mymenu[29-menuoffset].state = prefs.autodialog;
	mymenu[30-menuoffset].state = prefs.autodccchat;
	mymenu[31-menuoffset].state = prefs.autodccsend;
	/*mymenu[60-menuoffset].state = prefs.autosave;*/

	while (1)
	{
		switch (mymenu[i].type)
		{
		case M_NEWMENU:
			if (menu)
				gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
			menu = gtk_menu_new ();
			if (mymenu[i].callback == (void *) -1)
				usermenu = menu;
			menu_item = gtk_menu_item_new_with_mnemonic (_(mymenu[i].text));
			gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), menu_item);
			gtk_widget_show (menu_item);
			break;

		case M_MENUPIX:
			item = create_icon_menu (_(mymenu[i].text), mymenu[i].image, FALSE);
			goto normalitem;

		case M_MENUSTOCK:
			item = create_icon_menu (_(mymenu[i].text), mymenu[i].image, TRUE);
			goto normalitem;

		case M_MENU:
			item = gtk_menu_item_new_with_mnemonic (_(mymenu[i].text));
normalitem:
			if (mymenu[i].key != 0)
				gtk_widget_add_accelerator (item, "activate", accel_group,
										mymenu[i].key,
										mymenu[i].key == GDK_F1 ? 0 : GDK_CONTROL_MASK,
										GTK_ACCEL_VISIBLE);
			if (mymenu[i].callback)
				g_signal_connect (G_OBJECT (item), "activate",
										G_CALLBACK (mymenu[i].callback), 0);
			if (submenu)
				gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
			else
				gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			gtk_widget_show (item);
			gtk_widget_set_sensitive (item, mymenu[i].activate);
			break;

		case M_MENUTOG:
			item = gtk_check_menu_item_new_with_label (_(mymenu[i].text));
			gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item),
													 mymenu[i].state);
			if (mymenu[i].key != 0)
				gtk_widget_add_accelerator (item, "activate", accel_group,
									mymenu[i].key, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
			if (mymenu[i].callback)
				g_signal_connect (G_OBJECT (item), "toggled",
										G_CALLBACK (mymenu[i].callback), 0);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			gtk_widget_show (item);
			gtk_widget_set_sensitive (item, mymenu[i].activate);
			if (bar && i == 23 - menuoffset)
				*away_item_ret = item;
			break;

		case M_SEP:
			item = gtk_menu_item_new ();
			gtk_widget_set_sensitive (item, FALSE);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			gtk_widget_show (item);
			break;

		case M_MENUSUB:
			submenu = gtk_menu_new ();
			item = create_icon_menu (_(mymenu[i].text), mymenu[i].image, TRUE);
			gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			gtk_widget_show (item);
			break;

		case M_END:
			if (!submenu)
			{
				if (menu)
					gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
				if (usermenu)
					usermenu_create (usermenu);
				if (bar)
					*user_menu_ret = usermenu;
				return (menu_bar);
			}
			submenu = 0;
		}
		i++;
	}
}
