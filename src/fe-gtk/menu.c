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

#include <gtk/gtkhbox.h>
#include <gtk/gtkcheckmenuitem.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtkradiomenuitem.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkversion.h>
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
	M_MENUITEM,
	M_NEWMENU,
	M_END,
	M_SEP,
	M_MENUTOG,
	M_MENURADIO,
	M_MENUSTOCK,
	M_MENUPIX,
	M_MENUSUB
};

struct mymenu
{
	char *text;
	void *callback;
	char *image;
	unsigned char type;	/* M_XXX */
	unsigned char id;		/* MENU_ID_XXX (menu.h) */
	unsigned char state;	/* ticked or not? */
	unsigned char sensitive;	/* shaded out? */
	guint key;				/* GDK_x */
};


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
	int len;

/*	if (sess->type == SESS_DIALOG)
	{
		buf = (char *)(GTK_ENTRY (sess->gui->topic_entry)->text);
		buf = strrchr (buf, '@');
		if (buf)
			host = buf + 1;
	} else*/
	{
		user = userlist_find (sess, nick);
		if (user && user->hostname)
			host = strchr (user->hostname, '@') + 1;
	}

	/* this can't overflow, since popup->cmd is only 256 */
	len = strlen (cmd) + strlen (nick) + strlen (allnick) + 512;
	buf = malloc (len);

	auto_insert (buf, len, cmd, 0, 0, allnick, sess->channel, "", host,
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
		nicks[0] = g_strdup (sess->channel);
		nicks[1] = NULL;
		num_sel = 1;
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

	while (num_sel)
	{
		num_sel--;
		g_free (nicks[num_sel]);
	}

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

GtkWidget *
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

	return item;
}

static GtkWidget *
menu_quick_item (char *cmd, char *label, GtkWidget * menu, int flags,
					  gpointer userdata)
{
	GtkWidget *item;
	if (!label)
		item = gtk_menu_item_new ();
	else
	{
		if (flags & (1 << 1))
		{
			item = gtk_menu_item_new_with_label ("");
			gtk_label_set_markup (GTK_LABEL (GTK_BIN (item)->child), label);
		} else
		{
			if (flags & (1 << 2))
				item = gtk_menu_item_new_with_mnemonic (label);
			else
				item = gtk_menu_item_new_with_label (label);
		}
	}
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
menu_quick_sub (char *name, GtkWidget *menu, GtkWidget **sub_item_ret, int flags, int pos)
{
	GtkWidget *sub_menu;
	GtkWidget *sub_item;

	if (!name)
		return menu;

	/* Code to add a submenu */
	sub_menu = gtk_menu_new ();
	if (flags & 4)
		sub_item = gtk_menu_item_new_with_mnemonic (name);
	else
		sub_item = gtk_menu_item_new_with_label (name);
	gtk_menu_shell_insert (GTK_MENU_SHELL (menu), sub_item, pos);
	gtk_widget_show (sub_item);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (sub_item), sub_menu);

	if (sub_item_ret)
		*sub_item_ret = sub_item;

	if (flags & 1)
		/* We create a new element in the list */
		submenu_list = g_slist_prepend (submenu_list, sub_menu);
	return sub_menu;
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

static int
is_in_path (char *cmd)
{
	char *prog = strdup (cmd + 1);	/* 1st char is "!" */
	char *space, *path, *orig;

	orig = prog; /* save for free()ing */
	/* special-case these default entries. */
	/*                  123456789012345678 */
	if (strncmp (prog, "gnome-terminal -x ", 18) == 0)
	/* don't check for gnome-terminal, but the thing it's executing! */
		prog += 18;

	space = strchr (prog, ' ');	/* this isn't 100% but good enuf */
	if (space)
		*space = 0;

	path = g_find_program_in_path (prog);
	if (path)
	{
		g_free (path);
		g_free (orig);
		return 1;
	}

	g_free (orig);
	return 0;
}

/* append items to "menu" using the (struct popup*) list provided */

void
menu_create (GtkWidget *menu, GSList *list, char *target, int check_path)
{
	struct popup *pop;
	GtkWidget *tempmenu = menu, *subitem = NULL;
	int childcount = 0;

	submenu_list = g_slist_prepend (0, menu);
	while (list)
	{
		pop = (struct popup *) list->data;

		if (!strncasecmp (pop->name, "SUB", 3))
		{
			childcount = 0;
			tempmenu = menu_quick_sub (pop->cmd, tempmenu, &subitem, 1, -1);

		} else if (!strncasecmp (pop->name, "TOGGLE", 6))
		{
			childcount++;
			menu_toggle_item (pop->name + 7, tempmenu, toggle_cb, pop->cmd,
									cfg_get_bool (pop->cmd));

		} else if (!strncasecmp (pop->name, "ENDSUB", 6))
		{
			/* empty sub menu due to no programs in PATH? */
			if (check_path && childcount < 1)
				gtk_widget_destroy (subitem);
			subitem = NULL;

			if (tempmenu != menu)
				tempmenu = menu_quick_endsub ();
			/* If we get here and tempmenu equals menu that means we havent got any submenus to exit from */

		} else if (!strncasecmp (pop->name, "SEP", 3))
		{
			menu_quick_item (0, 0, tempmenu, 1, 0);

		} else
		{
			if (!check_path || pop->cmd[0] != '!')
			{
				menu_quick_item (pop->cmd, pop->name, tempmenu, 0, target);
			/* check if the program is in path, if not, leave it out! */
			} else if (is_in_path (pop->cmd))
			{
				childcount++;
				menu_quick_item (pop->cmd, pop->name, tempmenu, 0, target);
			}
		}

		list = list->next;
	}

	/* Let's clean up the linked list from mem */
	while (submenu_list)
		submenu_list = g_slist_remove (submenu_list, submenu_list->data);
}

static void
menu_destroy (GtkWidget *menu, gpointer objtounref)
{
	gtk_widget_destroy (menu);
	g_object_unref (menu);
	if (objtounref)
		g_object_unref (G_OBJECT (objtounref));
}

static void
menu_popup (GtkWidget *menu, GdkEventButton *event, gpointer objtounref)
{
#if (GTK_MAJOR_VERSION != 2) || (GTK_MINOR_VERSION != 0)
	if (event && event->window)
		gtk_menu_set_screen (GTK_MENU (menu), gdk_drawable_get_screen (event->window));
#endif

	g_object_ref (menu);
	gtk_object_sink (GTK_OBJECT (menu));
	g_signal_connect (G_OBJECT (menu), "selection-done",
							G_CALLBACK (menu_destroy), objtounref);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
						 0, event ? event->time : 0);
}

static char *str_copy = 0;		/* for all pop-up menus */

void
menu_nickmenu (session *sess, GdkEventButton *event, char *nick, int num_sel)
{
	char buf[512];
	char *real, *fmt;
	struct User *user;
	struct away_msg *away;
	GtkWidget *submenu, *menu = gtk_menu_new ();

	if (str_copy)
		free (str_copy);
	str_copy = strdup (nick);

	submenu_list = 0;	/* first time through, might not be 0 */

	/* more than 1 nick selected? */
	if (num_sel > 1)
	{
		snprintf (buf, sizeof (buf), "%d nicks selected.", num_sel);
		menu_quick_item (0, buf, menu, 0, 0);
		menu_quick_item (0, 0, menu, 1, 0);
	} else
	{
		user = userlist_find (sess, nick);	/* lasttalk is channel specific */
		if (!user)
			user = userlist_find_global (current_sess->server, nick);
		if (user)
		{
			submenu = menu_quick_sub (nick, menu, NULL, 1, -1);

			/* let the translators tweak this if need be */
			fmt = _("<tt><b>%-11s</b></tt> %s");

			if (user->realname)
			{
				real = g_markup_escape_text (user->realname, -1);
				snprintf (buf, sizeof (buf), fmt, _("Real Name:"), real);
				g_free (real);
			} else
			{
				snprintf (buf, sizeof (buf), fmt, _("Real Name:"), _("Unknown"));
			}
			menu_quick_item (0, buf, submenu, 2, 0);

			snprintf (buf, sizeof (buf), fmt, _("User:"),
						user->hostname ? user->hostname : _("Unknown"));
			menu_quick_item (0, buf, submenu, 2, 0);

			snprintf (buf, sizeof (buf), fmt, _("Country:"),
						user->hostname ? country(user->hostname) : _("Unknown"));
			menu_quick_item (0, buf, submenu, 2, 0);

			snprintf (buf, sizeof (buf), fmt, _("Server:"),
						user->servername ? user->servername : _("Unknown"));
			menu_quick_item (0, buf, submenu, 2, 0);

			if (user->away)
			{
				away = server_away_find_message (current_sess->server, nick);
				if (away)
				{
					char *msg = strip_color (away->message ? away->message : _("Unknown"), -1, STRIP_ALL);
					real = g_markup_escape_text (msg, -1);
					free (msg);
					snprintf (buf, sizeof (buf), fmt, _("Away Msg:"), real);
					g_free (real);
					menu_quick_item (0, buf, submenu, 2, 0);
				}
			}

			if (user->lasttalk)
			{
				char min[96];

				snprintf (min, sizeof (min), _("%u minutes ago"),
							(unsigned int) ((time (0) - user->lasttalk) / 60));
				snprintf (buf, sizeof (buf), fmt, _("Last Msg:"), min);
			} else
			{
				snprintf (buf, sizeof (buf), fmt, _("Last Msg:"), _("Unknown"));
			}

			menu_quick_item (0, buf, submenu, 2, 0);

			menu_quick_endsub ();
			menu_quick_item (0, 0, menu, 1, 0);
		}
	}

	if (num_sel > 1)
		menu_create (menu, popup_list, NULL, FALSE);
	else
		menu_create (menu, popup_list, str_copy, FALSE);
	menu_popup (menu, event, NULL);
}

/* stuff for the View menu */

static void
menu_showhide_cb (session *sess)
{
	if (prefs.hidemenu)
		gtk_widget_hide (sess->gui->menu);
	else
		gtk_widget_show (sess->gui->menu);
}

static void
menu_topic_showhide_cb (session *sess)
{
	if (prefs.topicbar)
		gtk_widget_show (sess->gui->topic_bar);
	else
		gtk_widget_hide (sess->gui->topic_bar);
}

static void
menu_ulbuttons_showhide_cb (session *sess)
{
	if (prefs.userlistbuttons)
		gtk_widget_show (sess->gui->button_box);
	else
		gtk_widget_hide (sess->gui->button_box);
}

static void
menu_cmbuttons_showhide_cb (session *sess)
{
	switch (sess->type)
	{
	case SESS_CHANNEL:
		if (prefs.chanmodebuttons)
			gtk_widget_show (sess->gui->topicbutton_box);
		else
			gtk_widget_hide (sess->gui->topicbutton_box);
		break;
	default:
		gtk_widget_hide (sess->gui->topicbutton_box);
	}
}

static void
menu_setting_foreach (void (*callback) (session *), int id, guint state)
{
	session *sess;
	GSList *list;
	int maindone = FALSE;	/* do it only once for EVERY tab */

	list = sess_list;
	while (list)
	{
		sess = list->data;

		if (!sess->gui->is_tab || !maindone)
		{
			if (sess->gui->is_tab)
				maindone = TRUE;
			if (id != -1)
				GTK_CHECK_MENU_ITEM (sess->gui->menu_item[id])->active = state;
			if (callback)
				callback (sess);
		}

		list = list->next;
	}
}

void
menu_bar_toggle (void)
{
	prefs.hidemenu = !prefs.hidemenu;
	menu_setting_foreach (menu_showhide_cb, MENU_ID_MENUBAR, !prefs.hidemenu);
}

static void
menu_bar_toggle_cb (void)
{
	menu_bar_toggle ();
	if (prefs.hidemenu)
		fe_message (_("The Menubar is now hidden. You can show it again"
						  " by pressing F9 or right-clicking in a blank part of"
						  " the main text area."), FE_MSG_INFO);
}

static void
menu_topicbar_toggle (GtkWidget *wid, gpointer ud)
{
	prefs.topicbar = !prefs.topicbar;
	menu_setting_foreach (menu_topic_showhide_cb, MENU_ID_TOPICBAR,
								 prefs.topicbar);
}

static void
menu_ulbuttons_toggle (GtkWidget *wid, gpointer ud)
{
	prefs.userlistbuttons = !prefs.userlistbuttons;
	menu_setting_foreach (menu_ulbuttons_showhide_cb, MENU_ID_ULBUTTONS,
								 prefs.userlistbuttons);
}

static void
menu_cmbuttons_toggle (GtkWidget *wid, gpointer ud)
{
	prefs.chanmodebuttons = !prefs.chanmodebuttons;
	menu_setting_foreach (menu_cmbuttons_showhide_cb, MENU_ID_MODEBUTTONS,
								 prefs.chanmodebuttons);
}

void
menu_middlemenu (session *sess, GdkEventButton *event)
{
	GtkWidget *menu;
	GtkAccelGroup *accel_group;

	accel_group = gtk_accel_group_new ();
	menu = menu_create_main (accel_group, FALSE, sess->server->is_away, !sess->gui->is_tab, NULL);
	menu_popup (menu, event, accel_group);
}

static void
open_url_cb (GtkWidget *item, char *url)
{
	char buf[512];

	/* pass this to /URL so it can handle irc:// */
	snprintf (buf, sizeof (buf), "URL %s", url);
	handle_command (current_sess, buf, FALSE);
}

static void
copy_to_clipboard_cb (GtkWidget *item, char *url)
{
	gtkutil_copy_to_clipboard (item, NULL, url);
}

void
menu_urlmenu (GdkEventButton *event, char *url)
{
	GtkWidget *menu;
	char *tmp, *chop;

	if (str_copy)
		free (str_copy);
	str_copy = strdup (url);

	menu = gtk_menu_new ();
	/* more than 51 chars? Chop it */
	if (g_utf8_strlen (str_copy, -1) >= 52)
	{
		tmp = strdup (str_copy);
		chop = g_utf8_offset_to_pointer (tmp, 48);
		chop[0] = chop[1] = chop[2] = '.';
		chop[3] = 0;
		menu_quick_item (0, tmp, menu, 1, 0);
		free (tmp);
	} else
	{
		menu_quick_item (0, str_copy, menu, 1, 0);
	}
	menu_quick_item (0, 0, menu, 1, 0);

	/* Two hardcoded entries */
	if (strncmp (str_copy, "irc://", 6) == 0 ||
	    strncmp (str_copy, "ircs://",7) == 0)
		menu_quick_item_with_callback (open_url_cb, _("Connect"), menu, str_copy);
	else
		menu_quick_item_with_callback (open_url_cb, _("Open Link in Browser"), menu, str_copy);
	menu_quick_item_with_callback (copy_to_clipboard_cb, _("Copy Selected Link"), menu, str_copy);
	/* custom ones from urlhandlers.conf */
	menu_create (menu, urlhandler_list, str_copy, TRUE);
	menu_popup (menu, event, NULL);
}

static void
menu_chan_cycle (GtkWidget * menu, char *chan)
{
	if (current_sess)
		handle_command (current_sess, "CYCLE", FALSE);
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
	editlist_gui_open (NULL, NULL, usermenu_list, _("XChat: User menu"),
							 "usermenu", "usermenu.conf", 0);
}

static void
usermenu_create (GtkWidget *menu)
{
	menu_create (menu, usermenu_list, "", FALSE);
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
	GtkWidget *menu;

	while (list)
	{
		sess = list->data;
		menu = sess->gui->menu_item[MENU_ID_USERMENU];
		if (sess->gui->is_tab)
		{
			if (!done_main && menu)
			{
				usermenu_destroy (menu);
				usermenu_create (menu);
				done_main = TRUE;
			}
		} else if (menu)
		{
			usermenu_destroy (menu);
			usermenu_create (menu);
		}
		list = list->next;
	}
}

static void
menu_newserver_window (GtkWidget * wid, gpointer none)
{
	int old = prefs.tabchannels;

	prefs.tabchannels = 0;
	new_ircwindow (NULL, NULL, SESS_SERVER, 0);
	prefs.tabchannels = old;
}

static void
menu_newchannel_window (GtkWidget * wid, gpointer none)
{
	int old = prefs.tabchannels;

	prefs.tabchannels = 0;
	new_ircwindow (current_sess->server, NULL, SESS_CHANNEL, 0);
	prefs.tabchannels = old;
}

static void
menu_newserver_tab (GtkWidget * wid, gpointer none)
{
	int old = prefs.tabchannels;
	int oldf = prefs.newtabstofront;

	prefs.tabchannels = 1;
	/* force focus if setting is "only requested tabs" */
	if (prefs.newtabstofront == 2)
		prefs.newtabstofront = 1;
	new_ircwindow (NULL, NULL, SESS_SERVER, 0);
	prefs.tabchannels = old;
	prefs.newtabstofront = oldf;
}

static void
menu_newchannel_tab (GtkWidget * wid, gpointer none)
{
	int old = prefs.tabchannels;

	prefs.tabchannels = 1;
	new_ircwindow (current_sess->server, NULL, SESS_CHANNEL, 0);
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
			fe_message (_("*WARNING*\n"
							 "Auto accepting DCC to your home directory\n"
							 "can be dangerous and is exploitable. Eg:\n"
							 "Someone could send you a .bash_profile"), FE_MSG_WARN);
		}
	}
#endif
}

static void
menu_detach (GtkWidget * wid, gpointer none)
{
	mg_detach (current_sess, 0);
}

static void
menu_close (GtkWidget * wid, gpointer none)
{
	mg_close_sess (current_sess);
}

static void
menu_search ()
{
	search_open (current_sess);
}

static void
menu_resetmarker (GtkWidget * wid, gpointer none)
{
	gtk_xtext_reset_marker_pos (GTK_XTEXT (current_sess->gui->xtext));
}

static void
menu_flushbuffer (GtkWidget * wid, gpointer none)
{
	fe_text_clear (current_sess);
}

static void
savebuffer_req_done (session *sess, char *file)
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
							current_sess, NULL, FRF_WRITE);
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
		serv->p_mode (serv, serv->nick, mode);
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
		serv->p_mode (serv, serv->nick, mode);
	}
}

static void
menu_disconnect (GtkWidget * wid, gpointer none)
{
	handle_command (current_sess, "DISCON", FALSE);
}

static void
menu_reconnect (GtkWidget * wid, gpointer none)
{
	if (current_sess->server->hostname[0])
		handle_command (current_sess, "RECONNECT", FALSE);
	else
		fe_serverlist_open (current_sess);
}

static void
menu_join_cb (GtkWidget *dialog, gint response, GtkEntry *entry)
{
	switch (response)
	{
	case GTK_RESPONSE_ACCEPT:
		menu_chan_join (NULL, entry->text);
		break;

	case GTK_RESPONSE_HELP:
		chanlist_opengui (current_sess->server, TRUE);
		break;
	}

	gtk_widget_destroy (dialog);
}

static void
menu_join_entry_cb (GtkWidget *entry, GtkDialog *dialog)
{
	gtk_dialog_response (dialog, GTK_RESPONSE_ACCEPT);
}

static void
menu_join (GtkWidget * wid, gpointer none)
{
	GtkWidget *hbox, *dialog, *entry, *label;

	dialog = gtk_dialog_new_with_buttons (_("Join Channel"),
									GTK_WINDOW (parent_window), 0,
									_("Retrieve channel list..."), GTK_RESPONSE_HELP,
									GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
									GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
									NULL);
	gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->vbox), TRUE);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
	hbox = gtk_hbox_new (TRUE, 0);

	entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry), "#");
	g_signal_connect (G_OBJECT (entry), "activate",
						 	G_CALLBACK (menu_join_entry_cb), dialog);
	gtk_box_pack_end (GTK_BOX (hbox), entry, 0, 0, 0);

	label = gtk_label_new (_("Enter Channel to Join:"));
	gtk_box_pack_end (GTK_BOX (hbox), label, 0, 0, 0);

	g_signal_connect (G_OBJECT (dialog), "response",
						   G_CALLBACK (menu_join_cb), entry);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

	gtk_widget_show_all (dialog);
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
		serv->p_mode (serv, serv->nick, mode);
	}
}

#if 0
static void
menu_savedefault (GtkWidget * wid, gpointer none)
{
	palette_save ();
	if (save_config ())
		fe_message (_("Settings saved."), FE_MSG_INFO);
}
#endif

static void
menu_chanlist (GtkWidget * wid, gpointer none)
{
	chanlist_opengui (current_sess->server, FALSE);
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
                           "shell instead of XChat")

static void
menu_usercommands (void)
{
	editlist_gui_open (NULL, NULL, command_list, _("XChat: User Defined Commands"),
							 "commands", "commands.conf", usercommands_help);
}

static void
menu_ulpopup (void)
{
	editlist_gui_open (NULL, NULL, popup_list, _("XChat: Userlist Popup menu"), "popup",
							 "popup.conf", ulbutton_help);
}

static void
menu_rpopup (void)
{
	editlist_gui_open (_("Text"), _("Replace with"), replace_list, _("XChat: Replace"), "replace",
							 "replace.conf", 0);
}

static void
menu_urlhandlers (void)
{
	editlist_gui_open (NULL, NULL, urlhandler_list, _("XChat: URL Handlers"), "urlhandlers",
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
	editlist_gui_open (NULL, NULL, button_list, _("XChat: Userlist buttons"), "buttons",
							 "buttons.conf", ulbutton_help);
}

static void
menu_dlgbuttons (void)
{
	editlist_gui_open (NULL, NULL, dlgbutton_list, _("XChat: Dialog buttons"), "dlgbuttons",
							 "dlgbuttons.conf", dlgbutton_help);
}

static void
menu_ctcpguiopen (void)
{
	editlist_gui_open (NULL, NULL, ctcp_list, _("XChat: CTCP Replies"), "ctcpreply",
							 "ctcpreply.conf", ctcp_help);
}

#if 0
static void
menu_reload (void)
{
	char *buf = malloc (strlen (default_file ()) + 12);
	load_config ();
	sprintf (buf, "%s reloaded.", default_file ());
	fe_message (buf, FE_MSG_INFO);
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
	fe_open_url ("http://xchat.org/docs/");
}

/*static void
menu_webpage (GtkWidget *wid, gpointer none)
{
	fe_open_url ("http://xchat.org");
}*/

static void
menu_dcc_win (GtkWidget *wid, gpointer none)
{
	fe_dcc_open_recv_win (FALSE);
	fe_dcc_open_send_win (FALSE);
}

static void
menu_dcc_chat_win (GtkWidget *wid, gpointer none)
{
	fe_dcc_open_chat_win (FALSE);
}

static void
menu_layout_cb (GtkWidget *item, gpointer none)
{
	if (GTK_CHECK_MENU_ITEM (item)->active)
	{
		prefs.tab_layout = 0;
		menu_setting_foreach (NULL, MENU_ID_LAYOUT_TABS, 1);
		menu_setting_foreach (NULL, MENU_ID_LAYOUT_TREE, 0);
		mg_change_layout (0);
	} else
	{
		prefs.tab_layout = 2;
		menu_setting_foreach (NULL, MENU_ID_LAYOUT_TABS, 0);
		menu_setting_foreach (NULL, MENU_ID_LAYOUT_TREE, 1);
		mg_change_layout (2);
	}
}

static void
menu_apply_metres_cb (session *sess)
{
	mg_update_meters (sess->gui);
}

static void
menu_metres_off (GtkWidget *item, gpointer none)
{
	if (GTK_CHECK_MENU_ITEM (item)->active)
	{
		prefs.lagometer = 0;
		prefs.throttlemeter = 0;
		menu_setting_foreach (menu_apply_metres_cb, -1, 0);
	}
}

static void
menu_metres_text (GtkWidget *item, gpointer none)
{
	if (GTK_CHECK_MENU_ITEM (item)->active)
	{
		prefs.lagometer = 2;
		prefs.throttlemeter = 2;
		menu_setting_foreach (menu_apply_metres_cb, -1, 0);
	}
}

static void
menu_metres_graph (GtkWidget *item, gpointer none)
{
	if (GTK_CHECK_MENU_ITEM (item)->active)
	{
		prefs.lagometer = 1;
		prefs.throttlemeter = 1;
		menu_setting_foreach (menu_apply_metres_cb, -1, 0);
	}
}

static void
menu_metres_both (GtkWidget *item, gpointer none)
{
	if (GTK_CHECK_MENU_ITEM (item)->active)
	{
		prefs.lagometer = 3;
		prefs.throttlemeter = 3;
		menu_setting_foreach (menu_apply_metres_cb, -1, 0);
	}
}

static struct mymenu mymenu[] = {
	{N_("_XChat"), 0, 0, M_NEWMENU, 0, 0, 1},
	{N_("_Server List..."), menu_open_server_list, (char *)&pix_book, M_MENUPIX, 0, 0, 1, GDK_s},
	{0, 0, 0, M_SEP, 0, 0, 0},

	{N_("_New"), 0, GTK_STOCK_NEW, M_MENUSUB, 0, 0, 1},
		{N_("Server Tab..."), menu_newserver_tab, 0, M_MENUITEM, 0, 0, 1, GDK_t},
		{N_("Channel Tab..."), menu_newchannel_tab, 0, M_MENUITEM, 0, 0, 1},
		{N_("Server Window..."), menu_newserver_window, 0, M_MENUITEM, 0, 0, 1},
		{N_("Channel Window..."), menu_newchannel_window, 0, M_MENUITEM, 0, 0, 1},
		{0, 0, 0, M_END, 0, 0, 0},
	{0, 0, 0, M_SEP, 0, 0, 0},

#ifdef USE_PLUGIN
	{N_("_Load Plugin or Script..."), menu_loadplugin, GTK_STOCK_REVERT_TO_SAVED, M_MENUSTOCK, 0, 0, 1},
#else
	{N_("_Load Plugin or Script..."), 0, GTK_STOCK_REVERT_TO_SAVED, M_MENUSTOCK, 0, 0, 0},
#endif
	{0, 0, 0, M_SEP, 0, 0, 0},	/* 11 */
#define DETACH_OFFSET (12)
	{0, menu_detach, GTK_STOCK_REDO, M_MENUSTOCK, 0, 0, 1, GDK_I},	/* 12 */
#define CLOSE_OFFSET (13)
	{0, menu_close, GTK_STOCK_CLOSE, M_MENUSTOCK, 0, 0, 1, GDK_w},
	{0, 0, 0, M_SEP, 0, 0, 0},
	{N_("_Quit"), mg_safe_quit, GTK_STOCK_QUIT, M_MENUSTOCK, 0, 0, 1, GDK_q},	/* 15 */

	{N_("_View"), 0, 0, M_NEWMENU, 0, 0, 1},
#define MENUBAR_OFFSET (17)
	{N_("_Menubar"), menu_bar_toggle_cb, 0, M_MENUTOG, MENU_ID_MENUBAR, 0, 1, GDK_F9},
	{N_("_Topicbar"), menu_topicbar_toggle, 0, M_MENUTOG, MENU_ID_TOPICBAR, 0, 1},
	{N_("_Userlist Buttons"), menu_ulbuttons_toggle, 0, M_MENUTOG, MENU_ID_ULBUTTONS, 0, 1},
	{N_("M_ode Buttons"), menu_cmbuttons_toggle, 0, M_MENUTOG, MENU_ID_MODEBUTTONS, 0, 1},
	{0, 0, 0, M_SEP, 0, 0, 0},
	{N_("_Layout"), 0, 0, M_MENUSUB, 0, 0, 1},	/* 22 */
#define TABS_OFFSET (23)
		{N_("_Tabs"), menu_layout_cb, 0, M_MENURADIO, MENU_ID_LAYOUT_TABS, 0, 1},
		{N_("T_ree"), 0, 0, M_MENURADIO, MENU_ID_LAYOUT_TREE, 0, 1},
		{0, 0, 0, M_END, 0, 0, 0},
	{N_("_Network Meters"), 0, 0, M_MENUSUB, 0, 0, 1},	/* 26 */
#define METRE_OFFSET (27)
		{N_("Off"), menu_metres_off, 0, M_MENURADIO, 0, 0, 1},
		{N_("Graph"), menu_metres_graph, 0, M_MENURADIO, 0, 0, 1},
		{N_("Text"), menu_metres_text, 0, M_MENURADIO, 0, 0, 1},
		{N_("Both"), menu_metres_both, 0, M_MENURADIO, 0, 0, 1},
		{0, 0, 0, M_END, 0, 0, 0},	/* 31 */

	{N_("_Server"), 0, 0, M_NEWMENU, 0, 0, 1},
	{N_("_Disconnect"), menu_disconnect, GTK_STOCK_DISCONNECT, M_MENUSTOCK, MENU_ID_DISCONNECT, 0, 1},
	{N_("_Reconnect"), menu_reconnect, GTK_STOCK_CONNECT, M_MENUSTOCK, MENU_ID_RECONNECT, 0, 1},
	{N_("Join Channel..."), menu_join, GTK_STOCK_JUMP_TO, M_MENUSTOCK, MENU_ID_JOIN, 0, 1},
	{0, 0, 0, M_SEP, 0, 0, 0},
#define AWAY_OFFSET (37)
	{N_("Marked Away"), menu_away, 0, M_MENUTOG, MENU_ID_AWAY, 0, 1, GDK_a},

	{N_("_Usermenu"), 0, 0, M_NEWMENU, MENU_ID_USERMENU, 0, 1},	/* 38 */

	{N_("S_ettings"), 0, 0, M_NEWMENU, 0, 0, 1},
	{N_("_Preferences"), menu_settings, GTK_STOCK_PREFERENCES, M_MENUSTOCK, 0, 0, 1},

	{N_("Advanced"), 0, GTK_STOCK_JUSTIFY_LEFT, M_MENUSUB, 0, 0, 1},
		{N_("Auto Replace..."), menu_rpopup, 0, M_MENUITEM, 0, 0, 1},
		{N_("CTCP Replies..."), menu_ctcpguiopen, 0, M_MENUITEM, 0, 0, 1},
		{N_("Dialog Buttons..."), menu_dlgbuttons, 0, M_MENUITEM, 0, 0, 1},
		{N_("Keyboard Shortcuts..."), menu_keypopup, 0, M_MENUITEM, 0, 0, 1},
		{N_("Text Events..."), menu_evtpopup, 0, M_MENUITEM, 0, 0, 1},
		{N_("URL Handlers..."), menu_urlhandlers, 0, M_MENUITEM, 0, 0, 1},
		{N_("User Commands..."), menu_usercommands, 0, M_MENUITEM, 0, 0, 1},
		{N_("Userlist Buttons..."), menu_ulbuttons, 0, M_MENUITEM, 0, 0, 1},
		{N_("Userlist Popup..."), menu_ulpopup, 0, M_MENUITEM, 0, 0, 1},
		{0, 0, 0, M_END, 0, 0, 0},		/* 51 */

	{N_("_Window"), 0, 0, M_NEWMENU, 0, 0, 1},
	{N_("Ban List..."), menu_banlist, 0, M_MENUITEM, 0, 0, 1},
	{N_("Channel List..."), menu_chanlist, 0, M_MENUITEM, 0, 0, 1},
	{N_("Character Chart..."), ascii_open, 0, M_MENUITEM, 0, 0, 1},
	{N_("Direct Chat..."), menu_dcc_chat_win, 0, M_MENUITEM, 0, 0, 1},
	{N_("File Transfers..."), menu_dcc_win, 0, M_MENUITEM, 0, 0, 1},
	{N_("Ignore List..."), ignore_gui_open, 0, M_MENUITEM, 0, 0, 1},
	{N_("Notify List..."), notify_opengui, 0, M_MENUITEM, 0, 0, 1},
	{N_("Plugins and Scripts..."), menu_pluginlist, 0, M_MENUITEM, 0, 0, 1},
	{N_("Raw Log..."), menu_rawlog, 0, M_MENUITEM, 0, 0, 1},	/* 61 */
	{N_("URL Grabber..."), url_opengui, 0, M_MENUITEM, 0, 0, 1},
	{0, 0, 0, M_SEP, 0, 0, 0},
	{N_("Reset Marker Line"), menu_resetmarker, 0, M_MENUITEM, 0, 0, 1, GDK_m},
	{N_("C_lear Text"), menu_flushbuffer, GTK_STOCK_CLEAR, M_MENUSTOCK, 0, 0, 1, GDK_l},
	{N_("Search Text..."), menu_search, GTK_STOCK_FIND, M_MENUSTOCK, 0, 0, 1, GDK_f},
	{N_("Save Text..."), menu_savebuffer, GTK_STOCK_SAVE, M_MENUSTOCK, 0, 0, 1},

	{N_("_Help"), 0, 0, M_NEWMENU, 0, 0, 1},	/* 68 */
	{N_("_Contents"), menu_docs, GTK_STOCK_HELP, M_MENUSTOCK, 0, 0, 1, GDK_F1},
	{N_("_About"), menu_about, GTK_STOCK_ABOUT, M_MENUSTOCK, 0, 0, 1},

	{0, 0, 0, M_END, 0, 0, 0},
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

#if GTK_CHECK_VERSION(2,4,0)

/* Override the default GTK2.4 handler, which would make menu
   bindings not work when the menu-bar is hidden. */
static gboolean
menu_canacaccel (GtkWidget *widget, guint signal_id, gpointer user_data)
{
	/* GTK2.2 behaviour */
	return GTK_WIDGET_IS_SENSITIVE (widget);
}

#endif


/* === STUFF FOR /MENU === */

static GtkMenuItem *
menu_find_item (GtkWidget *menu, char *name)
{
	GList *items = ((GtkMenuShell *) menu)->children;
	GtkMenuItem *item;
	GtkWidget *child;
	const char *labeltext;

	while (items)
	{
		item = items->data;
		child = GTK_BIN (item)->child;
		if (child)	/* separators arn't labels, skip them */
		{
			labeltext = g_object_get_data (G_OBJECT (item), "name");
			if (!labeltext)
				labeltext = gtk_label_get_text (GTK_LABEL (child));
			if (!menu_streq (labeltext, name, 1))
				return item;
		} else if (name == NULL)
		{
			return item;
		}
		items = items->next;
	}

	return NULL;
}

static GtkWidget *
menu_find_path (GtkWidget *menu, char *path)
{
	GtkMenuItem *item;
	char *s;
	char name[128];
	int len;

	/* grab the next part of the path */
	s = strchr (path, '/');
	len = s - path;
	if (!s)
		len = strlen (path);
	len = MIN (len, sizeof (name) - 1);
	memcpy (name, path, len);
	name[len] = 0;

	item = menu_find_item (menu, name);
	if (!item)
		return NULL;

	menu = gtk_menu_item_get_submenu (item);
	if (!menu)
		return NULL;

	path += len;
	if (*path == 0)
		return menu;

	return menu_find_path (menu, path + 1);
}

static GtkWidget *
menu_find (GtkWidget *menu, char *path, char *label)
{
	GtkWidget *item = NULL;

	if (path[0] != 0)
		menu = menu_find_path (menu, path);
	if (menu)
		item = (GtkWidget *)menu_find_item (menu, label);
	return item;
}

static void
menu_foreach_gui (menu_entry *me, void (*callback) (GtkWidget *, menu_entry *))
{
	GSList *list = sess_list;
	int tabdone = FALSE;
	session *sess;

	while (list)
	{
		sess = list->data;
		/* do it only once for tab sessions, since they share a GUI */
		if (!sess->gui->is_tab || !tabdone)
		{
			callback (sess->gui->menu, me);
			if (sess->gui->is_tab)
				tabdone = TRUE;
		}
		list = list->next;
	}
}

static void
menu_update_cb (GtkWidget *menu, menu_entry *me)
{
	GtkWidget *item;

	item = menu_find (menu, me->path, me->label);
	if (item)
	{
		gtk_widget_set_sensitive (item, me->enable);
		/* must do it without triggering the callback */
		if (GTK_IS_CHECK_MENU_ITEM (item))
			GTK_CHECK_MENU_ITEM (item)->active = me->state;
	}
}

/* toggle state changed via mouse click */
static void
menu_toggle_cb (GtkCheckMenuItem *item, menu_entry *me)
{
	if (item->active)
	{
		me->state = 1;
		handle_command (current_sess, me->cmd, FALSE);
	} else
	{
		me->state = 0;
		handle_command (current_sess, me->ucmd, FALSE);
	}

	/* update the state, incase this was changed via right-click. */
	/* This will update all other windows and menu bars */
	menu_foreach_gui (me, menu_update_cb);
}

static GtkWidget *
menu_add_toggle (GtkWidget *menu, menu_entry *me)
{
	GtkWidget *item = NULL;

	menu = menu_find_path (menu, me->path);
	if (menu)
	{
		item = menu_toggle_item (me->label, menu, menu_toggle_cb, me, me->state);
		if (me->pos != -1)
			gtk_menu_reorder_child (GTK_MENU (menu), item, me->pos);
	}
	return item;
}

static GtkWidget *
menu_add_item (GtkWidget *menu, menu_entry *me)
{
	GtkWidget *item = NULL;

	menu = menu_find_path (menu, me->path);
	if (menu)
	{
		item = menu_quick_item (me->cmd, me->label, menu, 4, 0);
		if (me->pos != -1)
			gtk_menu_reorder_child (GTK_MENU (menu), item, me->pos);
	}
	return item;
}

static GtkWidget *
menu_add_sub (GtkWidget *menu, menu_entry *me)
{
	GtkWidget *item = NULL;

	if (me->path[0] != 0)
		menu = menu_find_path (menu, me->path);
	if (menu)
		menu_quick_sub (me->label, menu, &item, 4, me->pos);
	return item;
}

static void
menu_del_cb (GtkWidget *menu, menu_entry *me)
{
	GtkWidget *item = menu_find (menu, me->path, me->label);
	if (item)
		gtk_widget_destroy (item);
}

static void
menu_add_cb (GtkWidget *menu, menu_entry *me)
{
	GtkWidget *item;

	if (me->ucmd)	/* have unselect-cmd? Must be a toggle item */
		item = menu_add_toggle (menu, me);
	else if (me->cmd || !me->label)	/* label=NULL for separators */
		item = menu_add_item (menu, me);
	else
		item = menu_add_sub (menu, me);

	if (item)
	{
		gtk_widget_set_sensitive (item, me->enable);
		if (me->key)
			gtk_widget_add_accelerator (item, "activate",
											g_object_get_data (G_OBJECT (menu), "accel"),
											me->key, me->modifier, GTK_ACCEL_VISIBLE);
	}
}

void
fe_menu_add (menu_entry *me)
{
	menu_foreach_gui (me, menu_add_cb);
}

void
fe_menu_del (menu_entry *me)
{
	menu_foreach_gui (me, menu_del_cb);
}

void
fe_menu_update (menu_entry *me)
{
	menu_foreach_gui (me, menu_update_cb);
}

/* used to add custom menus to the right-click menu */

static void
menu_add_plugin_items (GtkWidget *menu)
{
	GSList *list;
	menu_entry *me;

	list = menu_list;	/* outbound.c */
	while (list)
	{
		me = list->data;
		menu_add_cb (menu, me);
		list = list->next;
	}
}

/* === END STUFF FOR /MENU === */

GtkWidget *
menu_create_main (void *accel_group, int bar, int away, int toplevel,
						GtkWidget **menu_widgets)
{
	int i = 0;
	GtkWidget *item;
	GtkWidget *menu = 0;
	GtkWidget *menu_item = 0;
	GtkWidget *menu_bar;
	GtkWidget *usermenu = 0;
	GtkWidget *submenu = 0;
	int close_mask = GDK_CONTROL_MASK;
	int away_mask = GDK_MOD1_MASK;
	char *key_theme = NULL;
	GtkSettings *settings;
	GSList *group = NULL;

	if (bar)
		menu_bar = gtk_menu_bar_new ();
	else
		menu_bar = gtk_menu_new ();

	/* /MENU needs to know this later */
	g_object_set_data (G_OBJECT (menu_bar), "accel", accel_group);

#if GTK_CHECK_VERSION(2,4,0)
	g_signal_connect (G_OBJECT (menu_bar), "can-activate-accel",
							G_CALLBACK (menu_canacaccel), 0);
#endif

	/* set the initial state of toggles */
	mymenu[MENUBAR_OFFSET].state = !prefs.hidemenu;
	mymenu[MENUBAR_OFFSET+1].state = prefs.topicbar;
	mymenu[MENUBAR_OFFSET+2].state = prefs.userlistbuttons;
	mymenu[MENUBAR_OFFSET+3].state = prefs.chanmodebuttons;

	mymenu[AWAY_OFFSET].state = away;

	switch (prefs.tab_layout)
	{
	case 0:
		mymenu[TABS_OFFSET].state = 1;
		mymenu[TABS_OFFSET+1].state = 0;
		break;
	default:
		mymenu[TABS_OFFSET].state = 0;
		mymenu[TABS_OFFSET+1].state = 1;
	}

	mymenu[METRE_OFFSET].state = 0;
	mymenu[METRE_OFFSET+1].state = 0;
	mymenu[METRE_OFFSET+2].state = 0;
	mymenu[METRE_OFFSET+3].state = 0;
	switch (prefs.lagometer)
	{
	case 0:
		mymenu[METRE_OFFSET].state = 1;
		break;
	case 1:
		mymenu[METRE_OFFSET+1].state = 1;
		break;
	case 2:
		mymenu[METRE_OFFSET+2].state = 1;
		break;
	default:
		mymenu[METRE_OFFSET+3].state = 1;
	}

	/* change Close binding to ctrl-shift-w when using emacs keys */
	settings = gtk_widget_get_settings (menu_bar);
	if (settings)
	{
		g_object_get (settings, "gtk-key-theme-name", &key_theme, NULL);
		if (key_theme)
		{
			if (!strcasecmp (key_theme, "Emacs"))
				close_mask = GDK_SHIFT_MASK | GDK_CONTROL_MASK;
			g_free (key_theme);
		}
	}

	/* Away binding to ctrl-alt-a if the _Help menu conflicts (FR/PT/IT) */
	{
		char *help = _("_Help");
		char *under = strchr (help, '_');
		if (under && (under[1] == 'a' || under[1] == 'A'))
			away_mask = GDK_MOD1_MASK | GDK_CONTROL_MASK;
	}

	if (!toplevel)
	{
		mymenu[DETACH_OFFSET].text = N_("_Detach Tab");
		mymenu[CLOSE_OFFSET].text = N_("_Close Tab");
	}
	else
	{
		mymenu[DETACH_OFFSET].text = N_("_Attach Window");
		mymenu[CLOSE_OFFSET].text = N_("_Close Window");
	}

	while (1)
	{
		item = NULL;
		if (mymenu[i].id == MENU_ID_USERMENU && !prefs.gui_usermenu)
		{
			i++;
			continue;
		}

		switch (mymenu[i].type)
		{
		case M_NEWMENU:
			if (menu)
				gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
			item = menu = gtk_menu_new ();
			if (mymenu[i].id == MENU_ID_USERMENU)
				usermenu = menu;
			menu_item = gtk_menu_item_new_with_mnemonic (_(mymenu[i].text));
			/* record the English name for /menu */
			g_object_set_data (G_OBJECT (menu_item), "name", mymenu[i].text);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), menu_item);
			gtk_widget_show (menu_item);
			break;

		case M_MENUPIX:
			item = create_icon_menu (_(mymenu[i].text), mymenu[i].image, FALSE);
			goto normalitem;

		case M_MENUSTOCK:
			item = create_icon_menu (_(mymenu[i].text), mymenu[i].image, TRUE);
			goto normalitem;

		case M_MENUITEM:
			item = gtk_menu_item_new_with_mnemonic (_(mymenu[i].text));
normalitem:
			if (mymenu[i].key != 0)
				gtk_widget_add_accelerator (item, "activate", accel_group,
										mymenu[i].key,
										mymenu[i].key == GDK_F1 ? 0 :
										mymenu[i].key == GDK_w ? close_mask :
										GDK_CONTROL_MASK,
										GTK_ACCEL_VISIBLE);
			if (mymenu[i].callback)
				g_signal_connect (G_OBJECT (item), "activate",
										G_CALLBACK (mymenu[i].callback), 0);
			if (submenu)
				gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
			else
				gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			gtk_widget_show (item);
			break;

		case M_MENUTOG:
			item = gtk_check_menu_item_new_with_mnemonic (_(mymenu[i].text));
togitem:
			/* must avoid callback for Radio buttons */
			GTK_CHECK_MENU_ITEM (item)->active = mymenu[i].state;
			/*gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item),
													 mymenu[i].state);*/
			if (mymenu[i].key != 0)
				gtk_widget_add_accelerator (item, "activate", accel_group,
									mymenu[i].key, mymenu[i].id == MENU_ID_AWAY ?
									away_mask : GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
			if (mymenu[i].callback)
				g_signal_connect (G_OBJECT (item), "toggled",
										G_CALLBACK (mymenu[i].callback), 0);
			if (submenu)
				gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
			else
				gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			gtk_widget_show (item);
			gtk_widget_set_sensitive (item, mymenu[i].sensitive);
			break;

		case M_MENURADIO:
			item = gtk_radio_menu_item_new_with_mnemonic (group, _(mymenu[i].text));
			group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
			goto togitem;

		case M_SEP:
			item = gtk_menu_item_new ();
			gtk_widget_set_sensitive (item, FALSE);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			gtk_widget_show (item);
			break;

		case M_MENUSUB:
			group = NULL;
			submenu = gtk_menu_new ();
			item = create_icon_menu (_(mymenu[i].text), mymenu[i].image, TRUE);
			/* record the English name for /menu */
			g_object_set_data (G_OBJECT (item), "name", mymenu[i].text);
			gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			gtk_widget_show (item);
			break;

		/*case M_END:*/ default:
			if (!submenu)
			{
				if (menu)
				{
					gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
					menu_add_plugin_items (menu_bar);
				}
				if (usermenu)
					usermenu_create (usermenu);
				return (menu_bar);
			}
			submenu = NULL;
		}

		/* record this GtkWidget * so it's state might be changed later */
		if (mymenu[i].id != 0 && menu_widgets)
			/* this ends up in sess->gui->menu_item[MENU_ID_XXX] */
			menu_widgets[mymenu[i].id] = item;

		i++;
	}
}
