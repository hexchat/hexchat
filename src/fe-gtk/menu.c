/* X-Chat
 * Copyright (C) 1998-2007 Peter Zelezny.
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
#include <fcntl.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include "fe-gtk.h"

#include <gdk/gdkkeysyms.h>

#include "../common/hexchat.h"
#include "../common/hexchatc.h"
#include "../common/cfgfiles.h"
#include "../common/outbound.h"
#include "../common/ignore.h"
#include "../common/fe.h"
#include "../common/server.h"
#include "../common/servlist.h"
#include "../common/notify.h"
#include "../common/util.h"
#include "../common/text.h"
#include "xtext.h"
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
#include "servlistgui.h"

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
	guint key;				/* GDK_KEY_x */
};

#define XCMENU_DOLIST 1
#define XCMENU_SHADED 1
#define XCMENU_MARKUP 2
#define XCMENU_MNEMONIC 4

/* execute a userlistbutton/popupmenu command */

static void
nick_command (session * sess, char *cmd)
{
	if (*cmd == '!')
		hexchat_exec (cmd + 1);
	else
		handle_command (sess, cmd, TRUE);
}

/* fill in the %a %s %n etc and execute the command */

void
nick_command_parse (session *sess, char *cmd, char *nick, char *allnick)
{
	char *buf;
	char *host = _("Host unknown");
	char *account = _("Account unknown");
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
		if (user)
		{
			if (user->hostname)
				host = strchr (user->hostname, '@') + 1;
			if (user->account)
				account = user->account;
		}
	}

	/* this can't overflow, since popup->cmd is only 256 */
	len = strlen (cmd) + strlen (nick) + strlen (allnick) + 512;
	buf = g_malloc (len);

	auto_insert (buf, len, cmd, 0, 0, allnick, sess->channel, "",
					 server_get_network (sess->server, TRUE), host,
					 sess->server->nick, nick, account);

	nick_command (sess, buf);

	g_free (buf);
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
		nicks = g_new (char *, 2);
		nicks[0] = g_strdup (sess->channel);
		nicks[1] = NULL;
		num_sel = 1;
	}
	else
	{
		/* find number of selected rows */
		nicks = userlist_selection_list (sess->gui->user_tree, &num_sel);
		if (num_sel < 1)
		{
			nick_command_parse (sess, cmd, "", "");

			g_free (nicks);
			return;
		}
	}

	/* create "allnicks" string */
	allnicks = g_malloc (((NICKLEN + 1) * num_sel) + 1);
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

	g_free (nicks);
	g_free (allnicks);
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

	item = gtk_check_menu_item_new_with_mnemonic (label);
	gtk_check_menu_item_set_active ((GtkCheckMenuItem*)item, state);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (G_OBJECT (item), "activate",
							G_CALLBACK (callback), userdata);
	gtk_widget_show (item);

	return item;
}

GtkWidget *
menu_quick_item (char *cmd, char *label, GtkWidget * menu, int flags,
					  gpointer userdata, char *icon)
{
	GtkWidget *img, *item;
	char *path;

	if (!label)
		item = gtk_menu_item_new ();
	else
	{
		if (icon)
		{
			/*if (flags & XCMENU_MARKUP)
				item = gtk_image_menu_item_new_with_markup (label);
			else*/
				item = gtk_image_menu_item_new_with_mnemonic (label);
			img = NULL;
			if (access (icon, R_OK) == 0)	/* try fullpath */
				img = gtk_image_new_from_file (icon);
			else
			{
				/* try relative to <xdir> */
				path = g_build_filename (get_xdir (), icon, NULL);
				if (access (path, R_OK) == 0)
					img = gtk_image_new_from_file (path);
				else
					img = gtk_image_new_from_stock (icon, GTK_ICON_SIZE_MENU);
				g_free (path);
			}

			if (img)
				gtk_image_menu_item_set_image ((GtkImageMenuItem *)item, img);
		}
		else
		{
			if (flags & XCMENU_MARKUP)
			{
				item = gtk_menu_item_new_with_label ("");
				if (flags & XCMENU_MNEMONIC)
					gtk_label_set_markup_with_mnemonic (GTK_LABEL (GTK_BIN (item)->child), label);
				else
					gtk_label_set_markup (GTK_LABEL (GTK_BIN (item)->child), label);
			} else
			{
				if (flags & XCMENU_MNEMONIC)
					item = gtk_menu_item_new_with_mnemonic (label);
				else
					item = gtk_menu_item_new_with_label (label);
			}
		}
	}
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_object_set_data (G_OBJECT (item), "u", userdata);
	if (cmd)
		g_signal_connect (G_OBJECT (item), "activate",
								G_CALLBACK (popup_menu_cb), cmd);
	if (flags & XCMENU_SHADED)
		gtk_widget_set_sensitive (GTK_WIDGET (item), FALSE);
	gtk_widget_show_all (item);

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

GtkWidget *
menu_quick_sub (char *name, GtkWidget *menu, GtkWidget **sub_item_ret, int flags, int pos)
{
	GtkWidget *sub_menu;
	GtkWidget *sub_item;

	if (!name)
		return menu;

	/* Code to add a submenu */
	sub_menu = gtk_menu_new ();
	if (flags & XCMENU_MARKUP)
	{
		sub_item = gtk_menu_item_new_with_label ("");
		gtk_label_set_markup (GTK_LABEL (GTK_BIN (sub_item)->child), name);
	}
	else
	{
		if (flags & XCMENU_MNEMONIC)
			sub_item = gtk_menu_item_new_with_mnemonic (name);
		else
			sub_item = gtk_menu_item_new_with_label (name);
	}
	gtk_menu_shell_insert (GTK_MENU_SHELL (menu), sub_item, pos);
	gtk_widget_show (sub_item);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (sub_item), sub_menu);

	if (sub_item_ret)
		*sub_item_ret = sub_item;

	if (flags & XCMENU_DOLIST)
		/* We create a new element in the list */
		submenu_list = g_slist_prepend (submenu_list, sub_menu);
	return sub_menu;
}

static GtkWidget *
menu_quick_endsub (void)
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
		g_snprintf (buf, sizeof (buf), "set %s 1", pref_name);
	else
		g_snprintf (buf, sizeof (buf), "set %s 0", pref_name);

	handle_command (current_sess, buf, FALSE);
}

static int
is_in_path (char *cmd)
{
	char *orig = g_strdup (cmd + 1);	/* 1st char is "!" */
	char *prog = orig;
	char **argv;
	int argc;

	/* special-case these default entries. */
	/*                  123456789012345678 */
	if (strncmp (prog, "gnome-terminal -x ", 18) == 0)
	/* don't check for gnome-terminal, but the thing it's executing! */
		prog += 18;

	if (g_shell_parse_argv (prog, &argc, &argv, NULL))
	{
		char *path = g_find_program_in_path (argv[0]);
		g_strfreev (argv);
		if (path)
		{
			g_free (path);
			g_free (orig);
			return 1;
		}
	}

	g_free (orig);
	return 0;
}

/* syntax: "LABEL~ICON~STUFF~ADDED~LATER~" */

static void
menu_extract_icon (char *name, char **label, char **icon)
{
	char *p = name;
	char *start = NULL;
	char *end = NULL;

	while (*p)
	{
		if (*p == '~')
		{
			/* escape \~ */
			if (p == name || p[-1] != '\\')
			{
				if (!start)
					start = p + 1;
				else if (!end)
					end = p + 1;
			}
		}
		p++;
	}

	if (!end)
		end = p;

	if (start && start != end)
	{
		*label = g_strndup (name, (start - name) - 1);
		*icon = g_strndup (start, (end - start) - 1);
	}
	else
	{
		*label = g_strdup (name);
		*icon = NULL;
	}
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

		if (!g_ascii_strncasecmp (pop->name, "SUB", 3))
		{
			childcount = 0;
			tempmenu = menu_quick_sub (pop->cmd, tempmenu, &subitem, XCMENU_DOLIST|XCMENU_MNEMONIC, -1);

		} else if (!g_ascii_strncasecmp (pop->name, "TOGGLE", 6))
		{
			childcount++;
			menu_toggle_item (pop->name + 7, tempmenu, toggle_cb, pop->cmd,
									cfg_get_bool (pop->cmd));

		} else if (!g_ascii_strncasecmp (pop->name, "ENDSUB", 6))
		{
			/* empty sub menu due to no programs in PATH? */
			if (check_path && childcount < 1)
				gtk_widget_destroy (subitem);
			subitem = NULL;

			if (tempmenu != menu)
				tempmenu = menu_quick_endsub ();
			/* If we get here and tempmenu equals menu that means we havent got any submenus to exit from */

		} else if (!g_ascii_strncasecmp (pop->name, "SEP", 3))
		{
			menu_quick_item (0, 0, tempmenu, XCMENU_SHADED, 0, 0);

		} else
		{
			char *icon, *label;

			/* default command in hexchat.c */
			if (pop->cmd[0] == 'n' && !strcmp (pop->cmd, "notify -n ASK %s"))
			{
				/* don't create this item if already in notify list */
				if (!target || notify_is_in_list (current_sess->server, target))
				{
					list = list->next;
					continue;
				}
			}

			menu_extract_icon (pop->name, &label, &icon);

			if (!check_path || pop->cmd[0] != '!')
			{
				menu_quick_item (pop->cmd, label, tempmenu, 0, target, icon);
			/* check if the program is in path, if not, leave it out! */
			} else if (is_in_path (pop->cmd))
			{
				childcount++;
				menu_quick_item (pop->cmd, label, tempmenu, 0, target, icon);
			}

			g_free (label);
			g_free (icon);
		}

		list = list->next;
	}

	/* Let's clean up the linked list from mem */
	while (submenu_list)
		submenu_list = g_slist_remove (submenu_list, submenu_list->data);
}

static char *str_copy = NULL;		/* for all pop-up menus */
static GtkWidget *nick_submenu = NULL;	/* user info submenu */

static void
menu_destroy (GtkWidget *menu, gpointer objtounref)
{
	gtk_widget_destroy (menu);
	g_object_unref (menu);
	if (objtounref)
		g_object_unref (G_OBJECT (objtounref));
	nick_submenu = NULL;
}

static void
menu_popup (GtkWidget *menu, GdkEventButton *event, gpointer objtounref)
{
	if (event && event->window)
		gtk_menu_set_screen (GTK_MENU (menu), gdk_window_get_screen (event->window));

	g_object_ref (menu);
	g_object_ref_sink (menu);
	g_object_unref (menu);
	g_signal_connect (G_OBJECT (menu), "selection-done",
							G_CALLBACK (menu_destroy), objtounref);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
						 0, event ? event->time : 0);
}

static void
menu_nickinfo_cb (GtkWidget *menu, session *sess)
{
	char buf[512];

	if (!is_session (sess))
		return;

	/* issue a /WHOIS */
	g_snprintf (buf, sizeof (buf), "WHOIS %s %s", str_copy, str_copy);
	handle_command (sess, buf, FALSE);
	/* and hide the output */
	sess->server->skip_next_whois = 1;
}

static void
copy_to_clipboard_cb (GtkWidget *item, char *url)
{
	gtkutil_copy_to_clipboard (item, NULL, url);
}

/* returns boolean: Some data is missing */

static gboolean
menu_create_nickinfo_menu (struct User *user, GtkWidget *submenu)
{
	char buf[512];
	char unknown[96];
	char *real, *fmt, *users_country;
	struct away_msg *away;
	gboolean missing = FALSE;
	GtkWidget *item;

	/* let the translators tweak this if need be */
	fmt = _("<tt><b>%-11s</b></tt> %s");
	g_snprintf (unknown, sizeof (unknown), "<i>%s</i>", _("Unknown"));

	if (user->realname)
	{
		real = strip_color (user->realname, -1, STRIP_ALL|STRIP_ESCMARKUP);
		g_snprintf (buf, sizeof (buf), fmt, _("Real Name:"), real);
		g_free (real);
	} else
	{
		g_snprintf (buf, sizeof (buf), fmt, _("Real Name:"), unknown);
	}
	item = menu_quick_item (0, buf, submenu, XCMENU_MARKUP, 0, 0);
	g_signal_connect (G_OBJECT (item), "activate",
							G_CALLBACK (copy_to_clipboard_cb), 
							user->realname ? user->realname : unknown);

	g_snprintf (buf, sizeof (buf), fmt, _("User:"),
				 user->hostname ? user->hostname : unknown);
	item = menu_quick_item (0, buf, submenu, XCMENU_MARKUP, 0, 0);
	g_signal_connect (G_OBJECT (item), "activate",
							G_CALLBACK (copy_to_clipboard_cb), 
							user->hostname ? user->hostname : unknown);
	
	g_snprintf (buf, sizeof (buf), fmt, _("Account:"),
				 user->account ? user->account : unknown);
	item = menu_quick_item (0, buf, submenu, XCMENU_MARKUP, 0, 0);
	g_signal_connect (G_OBJECT (item), "activate",
							G_CALLBACK (copy_to_clipboard_cb), 
							user->account ? user->account : unknown);

	users_country = country (user->hostname);
	if (users_country)
	{
		g_snprintf (buf, sizeof (buf), fmt, _ ("Country:"), users_country);
		item = menu_quick_item (0, buf, submenu, XCMENU_MARKUP, 0, 0);
		g_signal_connect (G_OBJECT (item), "activate",
			G_CALLBACK (copy_to_clipboard_cb), users_country);
	}

	g_snprintf (buf, sizeof (buf), fmt, _("Server:"),
				 user->servername ? user->servername : unknown);
	item = menu_quick_item (0, buf, submenu, XCMENU_MARKUP, 0, 0);
	g_signal_connect (G_OBJECT (item), "activate",
							G_CALLBACK (copy_to_clipboard_cb), 
							user->servername ? user->servername : unknown);

	if (user->lasttalk)
	{
		char min[96];

		g_snprintf (min, sizeof (min), _("%u minutes ago"),
					(unsigned int) ((time (0) - user->lasttalk) / 60));
		g_snprintf (buf, sizeof (buf), fmt, _("Last Msg:"), min);
	} else
	{
		g_snprintf (buf, sizeof (buf), fmt, _("Last Msg:"), unknown);
	}
	menu_quick_item (0, buf, submenu, XCMENU_MARKUP, 0, 0);

	if (user->away)
	{
		away = server_away_find_message (current_sess->server, user->nick);
		if (away)
		{
			char *msg = strip_color (away->message ? away->message : unknown, -1, STRIP_ALL|STRIP_ESCMARKUP);
			g_snprintf (buf, sizeof (buf), fmt, _("Away Msg:"), msg);
			g_free (msg);
			item = menu_quick_item (0, buf, submenu, XCMENU_MARKUP, 0, 0);
			g_signal_connect (G_OBJECT (item), "activate",
									G_CALLBACK (copy_to_clipboard_cb), 
									away->message ? away->message : unknown);
		}
		else
			missing = TRUE;
	}

	return missing;
}

void
fe_userlist_update (session *sess, struct User *user)
{
	GList *items, *next;

	if (!nick_submenu || !str_copy)
		return;

	/* not the same nick as the menu? */
	if (sess->server->p_cmp (user->nick, str_copy))
		return;

	/* get rid of the "show" signal */
	g_signal_handlers_disconnect_by_func (nick_submenu, menu_nickinfo_cb, sess);

	/* destroy all the old items */
	items = ((GtkMenuShell *) nick_submenu)->children;
	while (items)
	{
		next = items->next;
		gtk_widget_destroy (items->data);
		items = next;
	}

	/* and re-create them with new info */
	menu_create_nickinfo_menu (user, nick_submenu);
}

void
menu_nickmenu (session *sess, GdkEventButton *event, char *nick, int num_sel)
{
	char buf[512];
	struct User *user;
	GtkWidget *submenu, *menu = gtk_menu_new ();

	g_free (str_copy);
	str_copy = g_strdup (nick);

	submenu_list = 0;	/* first time through, might not be 0 */

	/* more than 1 nick selected? */
	if (num_sel > 1)
	{
		g_snprintf (buf, sizeof (buf), _("%d nicks selected."), num_sel);
		menu_quick_item (0, buf, menu, 0, 0, 0);
		menu_quick_item (0, 0, menu, XCMENU_SHADED, 0, 0);
	} else
	{
		user = userlist_find (sess, nick);	/* lasttalk is channel specific */
		if (!user)
			user = userlist_find_global (current_sess->server, nick);
		if (user)
		{
			nick_submenu = submenu = menu_quick_sub (nick, menu, NULL, XCMENU_DOLIST, -1);

			if (menu_create_nickinfo_menu (user, submenu) ||
				 !user->hostname || !user->realname || !user->servername)
			{
				g_signal_connect (G_OBJECT (submenu), "show", G_CALLBACK (menu_nickinfo_cb), sess);
			}

			menu_quick_endsub ();
			menu_quick_item (0, 0, menu, XCMENU_SHADED, 0, 0);
		}
	}

	if (num_sel > 1)
		menu_create (menu, popup_list, NULL, FALSE);
	else
		menu_create (menu, popup_list, str_copy, FALSE);

	if (num_sel == 0)	/* xtext click */
		menu_add_plugin_items (menu, "\x5$NICK", str_copy);
	else	/* userlist treeview click */
		menu_add_plugin_items (menu, "\x5$NICK", NULL);

	menu_popup (menu, event, NULL);
}

/* stuff for the View menu */

static void
menu_showhide_cb (session *sess)
{
	if (prefs.hex_gui_hide_menu)
		gtk_widget_hide (sess->gui->menu);
	else
		gtk_widget_show (sess->gui->menu);
}

static void
menu_topic_showhide_cb (session *sess)
{
	if (prefs.hex_gui_topicbar)
		gtk_widget_show (sess->gui->topic_bar);
	else
		gtk_widget_hide (sess->gui->topic_bar);
}

static void
menu_userlist_showhide_cb (session *sess)
{
	mg_decide_userlist (sess, TRUE);
}

static void
menu_ulbuttons_showhide_cb (session *sess)
{
	if (prefs.hex_gui_ulist_buttons)
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
		if (prefs.hex_gui_mode_buttons)
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
	prefs.hex_gui_hide_menu = !prefs.hex_gui_hide_menu;
	menu_setting_foreach (menu_showhide_cb, MENU_ID_MENUBAR, !prefs.hex_gui_hide_menu);
}

static void
menu_bar_toggle_cb (void)
{
	menu_bar_toggle ();
	if (prefs.hex_gui_hide_menu)
		fe_message (_("The Menubar is now hidden. You can show it again"
						  " by pressing Control+F9 or right-clicking in a blank part of"
						  " the main text area."), FE_MSG_INFO);
}

static void
menu_topicbar_toggle (GtkWidget *wid, gpointer ud)
{
	prefs.hex_gui_topicbar = !prefs.hex_gui_topicbar;
	menu_setting_foreach (menu_topic_showhide_cb, MENU_ID_TOPICBAR,
								 prefs.hex_gui_topicbar);
}

static void
menu_userlist_toggle (GtkWidget *wid, gpointer ud)
{
	prefs.hex_gui_ulist_hide = !prefs.hex_gui_ulist_hide;
	menu_setting_foreach (menu_userlist_showhide_cb, MENU_ID_USERLIST,
								 !prefs.hex_gui_ulist_hide);
}

static void
menu_ulbuttons_toggle (GtkWidget *wid, gpointer ud)
{
	prefs.hex_gui_ulist_buttons = !prefs.hex_gui_ulist_buttons;
	menu_setting_foreach (menu_ulbuttons_showhide_cb, MENU_ID_ULBUTTONS,
								 prefs.hex_gui_ulist_buttons);
}

static void
menu_cmbuttons_toggle (GtkWidget *wid, gpointer ud)
{
	prefs.hex_gui_mode_buttons = !prefs.hex_gui_mode_buttons;
	menu_setting_foreach (menu_cmbuttons_showhide_cb, MENU_ID_MODEBUTTONS,
								 prefs.hex_gui_mode_buttons);
}

static void
menu_fullscreen_toggle (GtkWidget *wid, gpointer ud)
{
	if (!prefs.hex_gui_win_fullscreen)
		gtk_window_fullscreen (GTK_WINDOW(parent_window));
	else
	{
		gtk_window_unfullscreen (GTK_WINDOW(parent_window));

#ifdef WIN32
		if (!prefs.hex_gui_win_state) /* not maximized */
		{
			/* other window managers seem to handle this */
			gtk_window_resize (GTK_WINDOW (parent_window),
				prefs.hex_gui_win_width, prefs.hex_gui_win_height);
			gtk_window_move (GTK_WINDOW (parent_window),
				prefs.hex_gui_win_left, prefs.hex_gui_win_top);
		}
#endif
	}
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
	g_snprintf (buf, sizeof (buf), "URL %s", url);
	handle_command (current_sess, buf, FALSE);
}

void
menu_urlmenu (GdkEventButton *event, char *url)
{
	GtkWidget *menu;
	char *tmp, *chop;

	g_free (str_copy);
	str_copy = g_strdup (url);

	menu = gtk_menu_new ();
	/* more than 51 chars? Chop it */
	if (g_utf8_strlen (str_copy, -1) >= 52)
	{
		tmp = g_strdup (str_copy);
		chop = g_utf8_offset_to_pointer (tmp, 48);
		chop[0] = chop[1] = chop[2] = '.';
		chop[3] = 0;
		menu_quick_item (0, tmp, menu, XCMENU_SHADED, 0, 0);
		g_free (tmp);
	} else
	{
		menu_quick_item (0, str_copy, menu, XCMENU_SHADED, 0, 0);
	}
	menu_quick_item (0, 0, menu, XCMENU_SHADED, 0, 0);

	/* Two hardcoded entries */
	if (strncmp (str_copy, "irc://", 6) == 0 ||
	    strncmp (str_copy, "ircs://",7) == 0)
		menu_quick_item_with_callback (open_url_cb, _("Connect"), menu, str_copy);
	else
		menu_quick_item_with_callback (open_url_cb, _("Open Link in Browser"), menu, str_copy);
	menu_quick_item_with_callback (copy_to_clipboard_cb, _("Copy Selected Link"), menu, str_copy);
	/* custom ones from urlhandlers.conf */
	menu_create (menu, urlhandler_list, str_copy, TRUE);
	menu_add_plugin_items (menu, "\x4$URL", str_copy);
	menu_popup (menu, event, NULL);
}

static void
menu_chan_cycle (GtkWidget * menu, char *chan)
{
	char tbuf[256];

	if (current_sess)
	{
		g_snprintf (tbuf, sizeof tbuf, "CYCLE %s", chan);
		handle_command (current_sess, tbuf, FALSE);
	}
}

static void
menu_chan_part (GtkWidget * menu, char *chan)
{
	char tbuf[256];

	if (current_sess)
	{
		g_snprintf (tbuf, sizeof tbuf, "part %s", chan);
		handle_command (current_sess, tbuf, FALSE);
	}
}

static void
menu_chan_join (GtkWidget * menu, char *chan)
{
	char tbuf[256];

	if (current_sess)
	{
		g_snprintf (tbuf, sizeof tbuf, "join %s", chan);
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

	g_free (str_copy);
	str_copy = g_strdup (chan);

	menu = gtk_menu_new ();

	menu_quick_item (0, chan, menu, XCMENU_SHADED, str_copy, 0);
	menu_quick_item (0, 0, menu, XCMENU_SHADED, str_copy, 0);

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

	menu_addfavoritemenu (sess->server, menu, str_copy, FALSE);

	menu_add_plugin_items (menu, "\x5$CHAN", str_copy);
	menu_popup (menu, event, NULL);
}

static void
menu_delfav_cb (GtkWidget *item, server *serv)
{
	servlist_autojoinedit (serv->network, str_copy, FALSE);
}

static void
menu_addfav_cb (GtkWidget *item, server *serv)
{
	servlist_autojoinedit (serv->network, str_copy, TRUE);
}

void
menu_addfavoritemenu (server *serv, GtkWidget *menu, char *channel, gboolean istree)
{
	char *str;
	
	if (!serv->network)
		return;

	if (channel != str_copy)
	{
		g_free (str_copy);
		str_copy = g_strdup (channel);
	}
	
	if (istree)
		str = _("_Autojoin");
	else
		str = _("Autojoin Channel");

	if (joinlist_is_in_list (serv, channel))
	{
		menu_toggle_item (str, menu, menu_delfav_cb, serv, TRUE);
	}
	else
	{
		menu_toggle_item (str, menu, menu_addfav_cb, serv, FALSE);
	}
}

static void
menu_delautoconn_cb (GtkWidget *item, server *serv)
{
	((ircnet*)serv->network)->flags &= ~FLAG_AUTO_CONNECT;
	servlist_save ();
}

static void
menu_addautoconn_cb (GtkWidget *item, server *serv)
{
	((ircnet*)serv->network)->flags |= FLAG_AUTO_CONNECT;
	servlist_save ();
}

void
menu_addconnectmenu (server *serv, GtkWidget *menu)
{
	if (!serv->network)
		return;

	if (((ircnet*)serv->network)->flags & FLAG_AUTO_CONNECT)
	{
		menu_toggle_item (_("_Auto-Connect"), menu, menu_delautoconn_cb, serv, TRUE);
	}
	else
	{
		menu_toggle_item (_("_Auto-Connect"), menu, menu_addautoconn_cb, serv, FALSE);
	}
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
	editlist_gui_open (NULL, NULL, usermenu_list, _(DISPLAY_NAME": User menu"),
							 "usermenu", "usermenu.conf", 0);
}

static void
usermenu_create (GtkWidget *menu)
{
	menu_create (menu, usermenu_list, "", FALSE);
	menu_quick_item (0, 0, menu, XCMENU_SHADED, 0, 0);	/* sep */
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
	int old = prefs.hex_gui_tab_chans;

	prefs.hex_gui_tab_chans = 0;
	new_ircwindow (NULL, NULL, SESS_SERVER, 0);
	prefs.hex_gui_tab_chans = old;
}

static void
menu_newchannel_window (GtkWidget * wid, gpointer none)
{
	int old = prefs.hex_gui_tab_chans;

	prefs.hex_gui_tab_chans = 0;
	new_ircwindow (current_sess->server, NULL, SESS_CHANNEL, 0);
	prefs.hex_gui_tab_chans = old;
}

static void
menu_newserver_tab (GtkWidget * wid, gpointer none)
{
	int old = prefs.hex_gui_tab_chans;
	int oldf = prefs.hex_gui_tab_newtofront;

	prefs.hex_gui_tab_chans = 1;
	/* force focus if setting is "only requested tabs" */
	if (prefs.hex_gui_tab_newtofront == 2)
		prefs.hex_gui_tab_newtofront = 1;
	new_ircwindow (NULL, NULL, SESS_SERVER, 0);
	prefs.hex_gui_tab_chans = old;
	prefs.hex_gui_tab_newtofront = oldf;
}

static void
menu_newchannel_tab (GtkWidget * wid, gpointer none)
{
	int old = prefs.hex_gui_tab_chans;

	prefs.hex_gui_tab_chans = 1;
	new_ircwindow (current_sess->server, NULL, SESS_CHANNEL, 0);
	prefs.hex_gui_tab_chans = old;
}

static void
menu_rawlog (GtkWidget * wid, gpointer none)
{
	open_rawlog (current_sess->server);
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
menu_quit (GtkWidget * wid, gpointer none)
{
	mg_open_quit_dialog (FALSE);
}

static void
menu_search (void)
{
	mg_search_toggle (current_sess);
}

static void
menu_search_next (GtkWidget *wid)
{
	mg_search_handle_next(wid, current_sess);
}

static void
menu_search_prev (GtkWidget *wid)
{
	mg_search_handle_previous(wid, current_sess);
}

static void
menu_resetmarker (GtkWidget * wid, gpointer none)
{
	gtk_xtext_reset_marker_pos (GTK_XTEXT (current_sess->gui->xtext));
}

static void
menu_movetomarker (GtkWidget *wid, gpointer none)
{
	marker_reset_reason reason;
	char *str;

	if (!prefs.hex_text_show_marker)
		PrintText (current_sess, _("Marker line disabled."));
	else
	{
		reason = gtk_xtext_moveto_marker_pos (GTK_XTEXT (current_sess->gui->xtext));
		switch (reason) {
		case MARKER_WAS_NEVER_SET:
			str = _("Marker line never set."); break;
		case MARKER_IS_SET:
			str = ""; break;
		case MARKER_RESET_MANUALLY:
			str = _("Marker line reset manually."); break;
		case MARKER_RESET_BY_KILL:
			str = _("Marker line reset because exceeded scrollback limit."); break;
		case MARKER_RESET_BY_CLEAR:
			str = _("Marker line reset by CLEAR command."); break;
		default:
			str = _("Marker line state unknown."); break;
		}
		if (str[0])
			PrintText (current_sess, str);
	}
}

static void
menu_copy_selection (GtkWidget * wid, gpointer none)
{
	gtk_xtext_copy_selection (GTK_XTEXT (current_sess->gui->xtext));
}

static void
menu_flushbuffer (GtkWidget * wid, gpointer none)
{
	fe_text_clear (current_sess, 0);
}

static void
savebuffer_req_done (session *sess, char *file)
{
	int fh;

	if (!file)
		return;

	fh = g_open (file, O_TRUNC | O_WRONLY | O_CREAT, 0600);
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
							current_sess, NULL, NULL, FRF_WRITE);
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
	GTK_ENTRY (entry)->editable = 0;	/* avoid auto-selection */
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

	gtk_editable_set_editable (GTK_EDITABLE (entry), TRUE);
	gtk_editable_set_position (GTK_EDITABLE (entry), 1);
}

static void
menu_away (GtkCheckMenuItem *item, gpointer none)
{
	handle_command (current_sess, gtk_check_menu_item_get_active (item) ? "away" : "back", FALSE);
}

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

static void
menu_noplugin_info (void)
{
	fe_message (_(DISPLAY_NAME " has been build without plugin support."), FE_MSG_INFO);
}

#define menu_loadplugin menu_noplugin_info
#define menu_pluginlist menu_noplugin_info

#endif

#define usercommands_help  _("User Commands - Special codes:\n\n"\
                           "%c  =  current channel\n"\
									"%e  =  current network name\n"\
									"%m  =  machine info\n"\
                           "%n  =  your nick\n"\
									"%t  =  time/date\n"\
                           "%v  =  HexChat version\n"\
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
							"%e  =  current network name\n"\
							"%h  =  selected nick's hostname\n"\
							"%m  =  machine info\n"\
							"%n  =  your nick\n"\
							"%s  =  selected nick\n"\
							"%t  =  time/date\n"\
							"%u  =  selected users account")

#define dlgbutton_help      _("Dialog Buttons - Special codes:\n\n"\
							"%a  =  all selected nicks\n"\
							"%c  =  current channel\n"\
							"%e  =  current network name\n"\
							"%h  =  selected nick's hostname\n"\
							"%m  =  machine info\n"\
							"%n  =  your nick\n"\
							"%s  =  selected nick\n"\
							"%t  =  time/date\n"\
							"%u  =  selected users account")

#define ctcp_help          _("CTCP Replies - Special codes:\n\n"\
                           "%d  =  data (the whole ctcp)\n"\
									"%e  =  current network name\n"\
									"%m  =  machine info\n"\
                           "%s  =  nick who sent the ctcp\n"\
                           "%t  =  time/date\n"\
                           "%2  =  word 2\n"\
                           "%3  =  word 3\n"\
                           "&2  =  word 2 to the end of line\n"\
                           "&3  =  word 3 to the end of line\n\n")

#define url_help           _("URL Handlers - Special codes:\n\n"\
                           "%s  =  the URL string\n\n"\
                           "Putting a ! in front of the command\n"\
                           "indicates it should be sent to a\n"\
                           "shell instead of HexChat")

static void
menu_usercommands (void)
{
	editlist_gui_open (NULL, NULL, command_list, _(DISPLAY_NAME": User Defined Commands"),
							 "commands", "commands.conf", usercommands_help);
}

static void
menu_ulpopup (void)
{
	editlist_gui_open (NULL, NULL, popup_list, _(DISPLAY_NAME": Userlist Popup menu"), "popup",
							 "popup.conf", ulbutton_help);
}

static void
menu_rpopup (void)
{
	editlist_gui_open (_("Text"), _("Replace with"), replace_list, _(DISPLAY_NAME": Replace"), "replace",
							 "replace.conf", 0);
}

static void
menu_urlhandlers (void)
{
	editlist_gui_open (NULL, NULL, urlhandler_list, _(DISPLAY_NAME": URL Handlers"), "urlhandlers",
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
	editlist_gui_open (NULL, NULL, button_list, _(DISPLAY_NAME": Userlist buttons"), "buttons",
							 "buttons.conf", ulbutton_help);
}

static void
menu_dlgbuttons (void)
{
	editlist_gui_open (NULL, NULL, dlgbutton_list, _(DISPLAY_NAME": Dialog buttons"), "dlgbuttons",
							 "dlgbuttons.conf", dlgbutton_help);
}

static void
menu_ctcpguiopen (void)
{
	editlist_gui_open (NULL, NULL, ctcp_list, _(DISPLAY_NAME": CTCP Replies"), "ctcpreply",
							 "ctcpreply.conf", ctcp_help);
}

static void
menu_docs (GtkWidget *wid, gpointer none)
{
	fe_open_url ("http://hexchat.readthedocs.org");
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

void
menu_change_layout (void)
{
	if (prefs.hex_gui_tab_layout == 0)
	{
		menu_setting_foreach (NULL, MENU_ID_LAYOUT_TABS, 1);
		menu_setting_foreach (NULL, MENU_ID_LAYOUT_TREE, 0);
		mg_change_layout (0);
	} else
	{
		menu_setting_foreach (NULL, MENU_ID_LAYOUT_TABS, 0);
		menu_setting_foreach (NULL, MENU_ID_LAYOUT_TREE, 1);
		mg_change_layout (2);
	}
}

static void
menu_layout_cb (GtkWidget *item, gpointer none)
{
	prefs.hex_gui_tab_layout = 2;
	if (GTK_CHECK_MENU_ITEM (item)->active)
		prefs.hex_gui_tab_layout = 0;

	menu_change_layout ();
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
		prefs.hex_gui_lagometer = 0;
		prefs.hex_gui_throttlemeter = 0;
		hexchat_reinit_timers ();
		menu_setting_foreach (menu_apply_metres_cb, -1, 0);
	}
}

static void
menu_metres_text (GtkWidget *item, gpointer none)
{
	if (GTK_CHECK_MENU_ITEM (item)->active)
	{
		prefs.hex_gui_lagometer = 2;
		prefs.hex_gui_throttlemeter = 2;
		hexchat_reinit_timers ();
		menu_setting_foreach (menu_apply_metres_cb, -1, 0);
	}
}

static void
menu_metres_graph (GtkWidget *item, gpointer none)
{
	if (GTK_CHECK_MENU_ITEM (item)->active)
	{
		prefs.hex_gui_lagometer = 1;
		prefs.hex_gui_throttlemeter = 1;
		hexchat_reinit_timers ();
		menu_setting_foreach (menu_apply_metres_cb, -1, 0);
	}
}

static void
menu_metres_both (GtkWidget *item, gpointer none)
{
	if (GTK_CHECK_MENU_ITEM (item)->active)
	{
		prefs.hex_gui_lagometer = 3;
		prefs.hex_gui_throttlemeter = 3;
		hexchat_reinit_timers ();
		menu_setting_foreach (menu_apply_metres_cb, -1, 0);
	}
}

static void
about_dialog_close (GtkDialog *dialog, int response, gpointer data)
{
	gtk_widget_destroy (GTK_WIDGET(dialog));
}

static gboolean
about_dialog_openurl (GtkAboutDialog *dialog, char *uri, gpointer data)
{
	fe_open_url (uri);
	return TRUE;
}

static void
menu_about (GtkWidget *wid, gpointer sess)
{
	GtkAboutDialog *dialog = GTK_ABOUT_DIALOG(gtk_about_dialog_new());
	char comment[512];
	char *license = "This program is free software; you can redistribute it and/or modify\n" \
					"it under the terms of the GNU General Public License as published by\n" \
					"the Free Software Foundation; version 2.\n\n" \
					"This program is distributed in the hope that it will be useful,\n" \
					"but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
					"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n" \
					"GNU General Public License for more details.\n\n" \
					"You should have received a copy of the GNU General Public License\n" \
					"along with this program. If not, see <http://www.gnu.org/licenses/>";

	g_snprintf  (comment, sizeof(comment), ""
#ifdef WIN32
				"Portable Mode: %s\n"
				"Build Type: x%d\n"
#endif
				"OS: %s",
#ifdef WIN32
				(portable_mode () ? "Yes" : "No"),
				get_cpu_arch (),
#endif
				get_sys_str (0));

	gtk_about_dialog_set_program_name (dialog, DISPLAY_NAME);
	gtk_about_dialog_set_version (dialog, PACKAGE_VERSION);
	gtk_about_dialog_set_license (dialog, license); /* gtk3 can use GTK_LICENSE_GPL_2_0 */
	gtk_about_dialog_set_website (dialog, "http://hexchat.github.io");
	gtk_about_dialog_set_website_label (dialog, "Website");
	gtk_about_dialog_set_logo (dialog, pix_hexchat);
	gtk_about_dialog_set_copyright (dialog, "\302\251 1998-2010 Peter \305\275elezn\303\275\n\302\251 2009-2014 Berke Viktor");
	gtk_about_dialog_set_comments (dialog, comment);

	gtk_window_set_transient_for (GTK_WINDOW(dialog), GTK_WINDOW(parent_window));
	g_signal_connect (G_OBJECT(dialog), "response", G_CALLBACK(about_dialog_close), NULL);
	g_signal_connect (G_OBJECT(dialog), "activate-link", G_CALLBACK(about_dialog_openurl), NULL);
	
	gtk_widget_show_all (GTK_WIDGET(dialog));
}

static struct mymenu mymenu[] = {
	{N_("He_xChat"), 0, 0, M_NEWMENU, MENU_ID_HEXCHAT, 0, 1},
	{N_("Network Li_st..."), menu_open_server_list, (char *)&pix_book, M_MENUPIX, 0, 0, 1, GDK_KEY_s},
	{0, 0, 0, M_SEP, 0, 0, 0},

	{N_("_New"), 0, GTK_STOCK_NEW, M_MENUSUB, 0, 0, 1},
		{N_("Server Tab..."), menu_newserver_tab, 0, M_MENUITEM, 0, 0, 1, GDK_KEY_t},
		{N_("Channel Tab..."), menu_newchannel_tab, 0, M_MENUITEM, 0, 0, 1},
		{N_("Server Window..."), menu_newserver_window, 0, M_MENUITEM, 0, 0, 1, GDK_KEY_n},
		{N_("Channel Window..."), menu_newchannel_window, 0, M_MENUITEM, 0, 0, 1},
		{0, 0, 0, M_END, 0, 0, 0},
	{0, 0, 0, M_SEP, 0, 0, 0},

	{N_("_Load Plugin or Script..."), menu_loadplugin, GTK_STOCK_REVERT_TO_SAVED, M_MENUSTOCK, 0, 0, 1},
	{0, 0, 0, M_SEP, 0, 0, 0},	/* 11 */
#define DETACH_OFFSET (12)
	{0, menu_detach, GTK_STOCK_REDO, M_MENUSTOCK, 0, 0, 1},	/* 12 */
#define CLOSE_OFFSET (13)
	{0, menu_close, GTK_STOCK_CLOSE, M_MENUSTOCK, 0, 0, 1, GDK_KEY_w},
	{0, 0, 0, M_SEP, 0, 0, 0},
	{N_("_Quit"), menu_quit, GTK_STOCK_QUIT, M_MENUSTOCK, 0, 0, 1, GDK_KEY_q},	/* 15 */

	{N_("_View"), 0, 0, M_NEWMENU, 0, 0, 1},
#define MENUBAR_OFFSET (17)
	{N_("_Menu Bar"), menu_bar_toggle_cb, 0, M_MENUTOG, MENU_ID_MENUBAR, 0, 1, GDK_KEY_F9},
	{N_("_Topic Bar"), menu_topicbar_toggle, 0, M_MENUTOG, MENU_ID_TOPICBAR, 0, 1},
	{N_("_User List"), menu_userlist_toggle, 0, M_MENUTOG, MENU_ID_USERLIST, 0, 1, GDK_KEY_F7},
	{N_("U_serlist Buttons"), menu_ulbuttons_toggle, 0, M_MENUTOG, MENU_ID_ULBUTTONS, 0, 1},
	{N_("M_ode Buttons"), menu_cmbuttons_toggle, 0, M_MENUTOG, MENU_ID_MODEBUTTONS, 0, 1},
	{0, 0, 0, M_SEP, 0, 0, 0},
	{N_("_Channel Switcher"), 0, 0, M_MENUSUB, 0, 0, 1},	/* 23 */
#define TABS_OFFSET (24)
		{N_("_Tabs"), menu_layout_cb, 0, M_MENURADIO, MENU_ID_LAYOUT_TABS, 0, 1},
		{N_("T_ree"), 0, 0, M_MENURADIO, MENU_ID_LAYOUT_TREE, 0, 1},
		{0, 0, 0, M_END, 0, 0, 0},
	{N_("_Network Meters"), 0, 0, M_MENUSUB, 0, 0, 1},	/* 27 */
#define METRE_OFFSET (28)
		{N_("Off"), menu_metres_off, 0, M_MENURADIO, 0, 0, 1},
		{N_("Graph"), menu_metres_graph, 0, M_MENURADIO, 0, 0, 1},
		{N_("Text"), menu_metres_text, 0, M_MENURADIO, 0, 0, 1},
		{N_("Both"), menu_metres_both, 0, M_MENURADIO, 0, 0, 1},
		{0, 0, 0, M_END, 0, 0, 0},	/* 32 */
	{ 0, 0, 0, M_SEP, 0, 0, 0 },
	{N_ ("_Fullscreen"), menu_fullscreen_toggle, 0, M_MENUTOG, MENU_ID_FULLSCREEN, 0, 1, GDK_KEY_F11},

	{N_("_Server"), 0, 0, M_NEWMENU, 0, 0, 1},
	{N_("_Disconnect"), menu_disconnect, GTK_STOCK_DISCONNECT, M_MENUSTOCK, MENU_ID_DISCONNECT, 0, 1},
	{N_("_Reconnect"), menu_reconnect, GTK_STOCK_CONNECT, M_MENUSTOCK, MENU_ID_RECONNECT, 0, 1},
	{N_("_Join a Channel..."), menu_join, GTK_STOCK_JUMP_TO, M_MENUSTOCK, MENU_ID_JOIN, 0, 1},
	{N_("_List of Channels..."), menu_chanlist, GTK_STOCK_INDEX, M_MENUITEM, 0, 0, 1},
	{0, 0, 0, M_SEP, 0, 0, 0},
#define AWAY_OFFSET (41)
	{N_("Marked _Away"), menu_away, 0, M_MENUTOG, MENU_ID_AWAY, 0, 1, GDK_KEY_a},

	{N_("_Usermenu"), 0, 0, M_NEWMENU, MENU_ID_USERMENU, 0, 1},	/* 40 */

	{N_("S_ettings"), 0, 0, M_NEWMENU, 0, 0, 1},
	{N_("_Preferences"), menu_settings, GTK_STOCK_PREFERENCES, M_MENUSTOCK, 0, 0, 1},
	{0, 0, 0, M_SEP, 0, 0, 0},
	{N_("Auto Replace..."), menu_rpopup, 0, M_MENUITEM, 0, 0, 1},
	{N_("CTCP Replies..."), menu_ctcpguiopen, 0, M_MENUITEM, 0, 0, 1},
	{N_("Dialog Buttons..."), menu_dlgbuttons, 0, M_MENUITEM, 0, 0, 1},
	{N_("Keyboard Shortcuts..."), menu_keypopup, 0, M_MENUITEM, 0, 0, 1},
	{N_("Text Events..."), menu_evtpopup, 0, M_MENUITEM, 0, 0, 1},
	{N_("URL Handlers..."), menu_urlhandlers, 0, M_MENUITEM, 0, 0, 1},
	{N_("User Commands..."), menu_usercommands, 0, M_MENUITEM, 0, 0, 1},
	{N_("Userlist Buttons..."), menu_ulbuttons, 0, M_MENUITEM, 0, 0, 1},
	{N_("Userlist Popup..."), menu_ulpopup, 0, M_MENUITEM, 0, 0, 1},	/* 52 */

	{N_("_Window"), 0, 0, M_NEWMENU, 0, 0, 1},
	{N_("_Ban List..."), menu_banlist, 0, M_MENUITEM, 0, 0, 1},
	{N_("Character Chart..."), ascii_open, 0, M_MENUITEM, 0, 0, 1},
	{N_("Direct Chat..."), menu_dcc_chat_win, 0, M_MENUITEM, 0, 0, 1},
	{N_("File _Transfers..."), menu_dcc_win, 0, M_MENUITEM, 0, 0, 1},
	{N_("Friends List..."), notify_opengui, 0, M_MENUITEM, 0, 0, 1},
	{N_("Ignore List..."), ignore_gui_open, 0, M_MENUITEM, 0, 0, 1},
	{N_("_Plugins and Scripts..."), menu_pluginlist, 0, M_MENUITEM, 0, 0, 1},
	{N_("_Raw Log..."), menu_rawlog, 0, M_MENUITEM, 0, 0, 1},	/* 61 */
	{N_("URL Grabber..."), url_opengui, 0, M_MENUITEM, 0, 0, 1},
	{0, 0, 0, M_SEP, 0, 0, 0},
	{N_("Reset Marker Line"), menu_resetmarker, 0, M_MENUITEM, 0, 0, 1, GDK_KEY_m},
	{N_("Move to Marker Line"), menu_movetomarker, 0, M_MENUITEM, 0, 0, 1, GDK_KEY_M},
	{N_("_Copy Selection"), menu_copy_selection, 0, M_MENUITEM, 0, 0, 1, GDK_KEY_C},
	{N_("C_lear Text"), menu_flushbuffer, GTK_STOCK_CLEAR, M_MENUSTOCK, 0, 0, 1},
	{N_("Save Text..."), menu_savebuffer, GTK_STOCK_SAVE, M_MENUSTOCK, 0, 0, 1},
#define SEARCH_OFFSET (70)
	{N_("Search"), 0, GTK_STOCK_JUSTIFY_LEFT, M_MENUSUB, 0, 0, 1},
		{N_("Search Text..."), menu_search, GTK_STOCK_FIND, M_MENUSTOCK, 0, 0, 1, GDK_KEY_f},
		{N_("Search Next"   ), menu_search_next, GTK_STOCK_FIND, M_MENUSTOCK, 0, 0, 1, GDK_KEY_g},
		{N_("Search Previous"   ), menu_search_prev, GTK_STOCK_FIND, M_MENUSTOCK, 0, 0, 1, GDK_KEY_G},
		{0, 0, 0, M_END, 0, 0, 0},

	{N_("_Help"), 0, 0, M_NEWMENU, 0, 0, 1},	/* 74 */
	{N_("_Contents"), menu_docs, GTK_STOCK_HELP, M_MENUSTOCK, 0, 0, 1, GDK_KEY_F1},
	{N_("_About"), menu_about, GTK_STOCK_ABOUT, M_MENUSTOCK, 0, 0, 1},

	{0, 0, 0, M_END, 0, 0, 0},
};

void
menu_set_away (session_gui *gui, int away)
{
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM (gui->menu_item[MENU_ID_AWAY]);

	g_signal_handlers_block_by_func (G_OBJECT (item), menu_away, NULL);
	gtk_check_menu_item_set_active (item, away);
	g_signal_handlers_unblock_by_func (G_OBJECT (item), menu_away, NULL);
}

void
menu_set_fullscreen (session_gui *gui, int full)
{
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM (gui->menu_item[MENU_ID_FULLSCREEN]);

	g_signal_handlers_block_by_func (G_OBJECT (item), menu_fullscreen_toggle, NULL);
	gtk_check_menu_item_set_active (item, full);
	g_signal_handlers_unblock_by_func (G_OBJECT (item), menu_fullscreen_toggle, NULL);
}

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

/* Override the default GTK2.4 handler, which would make menu
   bindings not work when the menu-bar is hidden. */
static gboolean
menu_canacaccel (GtkWidget *widget, guint signal_id, gpointer user_data)
{
	/* GTK2.2 behaviour */
	return gtk_widget_is_sensitive (widget);
}

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
menu_foreach_gui (menu_entry *me, void (*callback) (GtkWidget *, menu_entry *, char *))
{
	GSList *list = sess_list;
	int tabdone = FALSE;
	session *sess;

	if (!me->is_main)
		return;	/* not main menu */

	while (list)
	{
		sess = list->data;
		/* do it only once for tab sessions, since they share a GUI */
		if (!sess->gui->is_tab || !tabdone)
		{
			callback (sess->gui->menu, me, NULL);
			if (sess->gui->is_tab)
				tabdone = TRUE;
		}
		list = list->next;
	}
}

static void
menu_update_cb (GtkWidget *menu, menu_entry *me, char *target)
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

/* radio state changed via mouse click */
static void
menu_radio_cb (GtkCheckMenuItem *item, menu_entry *me)
{
	me->state = 0;
	if (item->active)
		me->state = 1;

	/* update the state, incase this was changed via right-click. */
	/* This will update all other windows and menu bars */
	menu_foreach_gui (me, menu_update_cb);

	if (me->state && me->cmd)
		handle_command (current_sess, me->cmd, FALSE);
}

/* toggle state changed via mouse click */
static void
menu_toggle_cb (GtkCheckMenuItem *item, menu_entry *me)
{
	me->state = 0;
	if (item->active)
		me->state = 1;

	/* update the state, incase this was changed via right-click. */
	/* This will update all other windows and menu bars */
	menu_foreach_gui (me, menu_update_cb);

	if (me->state)
		handle_command (current_sess, me->cmd, FALSE);
	else
		handle_command (current_sess, me->ucmd, FALSE);
}

static GtkWidget *
menu_radio_item (char *label, GtkWidget *menu, void *callback, void *userdata,
						int state, char *groupname)
{
	GtkWidget *item;
	GtkMenuItem *parent;
	GSList *grouplist = NULL;

	parent = menu_find_item (menu, groupname);
	if (parent)
		grouplist = gtk_radio_menu_item_get_group ((GtkRadioMenuItem *)parent);

	item = gtk_radio_menu_item_new_with_label (grouplist, label);
	gtk_check_menu_item_set_active ((GtkCheckMenuItem*)item, state);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (G_OBJECT (item), "activate",
							G_CALLBACK (callback), userdata);
	gtk_widget_show (item);

	return item;
}

static void
menu_reorder (GtkMenu *menu, GtkWidget *item, int pos)
{
	if (pos == 0xffff)	/* outbound.c uses this default */
		return;

	if (pos < 0)	/* position offset from end/bottom */
		gtk_menu_reorder_child (menu, item, (g_list_length (GTK_MENU_SHELL (menu)->children) + pos) - 1);
	else
		gtk_menu_reorder_child (menu, item, pos);
}

static GtkWidget *
menu_add_radio (GtkWidget *menu, menu_entry *me)
{
	GtkWidget *item = NULL;
	char *path = me->path + me->root_offset;

	if (path[0] != 0)
		menu = menu_find_path (menu, path);
	if (menu)
	{
		item = menu_radio_item (me->label, menu, menu_radio_cb, me, me->state, me->group);
		menu_reorder (GTK_MENU (menu), item, me->pos);
	}
	return item;
}

static GtkWidget *
menu_add_toggle (GtkWidget *menu, menu_entry *me)
{
	GtkWidget *item = NULL;
	char *path = me->path + me->root_offset;

	if (path[0] != 0)
		menu = menu_find_path (menu, path);
	if (menu)
	{
		item = menu_toggle_item (me->label, menu, menu_toggle_cb, me, me->state);
		menu_reorder (GTK_MENU (menu), item, me->pos);
	}
	return item;
}

static GtkWidget *
menu_add_item (GtkWidget *menu, menu_entry *me, char *target)
{
	GtkWidget *item = NULL;
	char *path = me->path + me->root_offset;

	if (path[0] != 0)
		menu = menu_find_path (menu, path);
	if (menu)
	{
		item = menu_quick_item (me->cmd, me->label, menu, me->markup ? XCMENU_MARKUP|XCMENU_MNEMONIC : XCMENU_MNEMONIC, target, me->icon);
		menu_reorder (GTK_MENU (menu), item, me->pos);
	}
	return item;
}

static GtkWidget *
menu_add_sub (GtkWidget *menu, menu_entry *me)
{
	GtkWidget *item = NULL;
	char *path = me->path + me->root_offset;
	int pos;

	if (path[0] != 0)
		menu = menu_find_path (menu, path);
	if (menu)
	{
		pos = me->pos;
		if (pos < 0)	/* position offset from end/bottom */
			pos = g_list_length (GTK_MENU_SHELL (menu)->children) + pos;
		menu_quick_sub (me->label, menu, &item, me->markup ? XCMENU_MARKUP|XCMENU_MNEMONIC : XCMENU_MNEMONIC, pos);
	}
	return item;
}

static void
menu_del_cb (GtkWidget *menu, menu_entry *me, char *target)
{
	GtkWidget *item = menu_find (menu, me->path + me->root_offset, me->label);
	if (item)
		gtk_widget_destroy (item);
}

static void
menu_add_cb (GtkWidget *menu, menu_entry *me, char *target)
{
	GtkWidget *item;
	GtkAccelGroup *accel_group;

	if (me->group)	/* have a group name? Must be a radio item */
		item = menu_add_radio (menu, me);
	else if (me->ucmd)	/* have unselect-cmd? Must be a toggle item */
		item = menu_add_toggle (menu, me);
	else if (me->cmd || !me->label)	/* label=NULL for separators */
		item = menu_add_item (menu, me, target);
	else
		item = menu_add_sub (menu, me);

	if (item)
	{
		gtk_widget_set_sensitive (item, me->enable);
		if (me->key)
		{
			accel_group = g_object_get_data (G_OBJECT (menu), "accel");
			if (accel_group)	/* popup menus don't have them */
				gtk_widget_add_accelerator (item, "activate", accel_group, me->key,
													 me->modifier, GTK_ACCEL_VISIBLE);
		}
	}
}

char *
fe_menu_add (menu_entry *me)
{
	char *text;

	menu_foreach_gui (me, menu_add_cb);

	if (!me->markup)
		return NULL;

	if (!pango_parse_markup (me->label, -1, 0, NULL, &text, NULL, NULL))
		return NULL;

	/* return the label with markup stripped */
	return text;
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
menu_add_plugin_mainmenu_items (GtkWidget *menu)
{
	GSList *list;
	menu_entry *me;

	list = menu_list;	/* outbound.c */
	while (list)
	{
		me = list->data;
		if (me->is_main)
			menu_add_cb (menu, me, NULL);
		list = list->next;
	}
}

void
menu_add_plugin_items (GtkWidget *menu, char *root, char *target)
{
	GSList *list;
	menu_entry *me;

	list = menu_list;	/* outbound.c */
	while (list)
	{
		me = list->data;
		if (!me->is_main && !strncmp (me->path, root + 1, root[0]))
			menu_add_cb (menu, me, target);
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
	int close_mask = STATE_CTRL;
	int away_mask = STATE_ALT;
	char *key_theme = NULL;
	GtkSettings *settings;
	GSList *group = NULL;
#ifdef HAVE_GTK_MAC
	int appmenu_offset = 1; /* 0 is for about */
#endif

	if (bar)
	{
		menu_bar = gtk_menu_bar_new ();
#ifdef HAVE_GTK_MAC
		gtkosx_application_set_menu_bar (osx_app, GTK_MENU_SHELL (menu_bar));
#endif
	}
	else
		menu_bar = gtk_menu_new ();

	/* /MENU needs to know this later */
	g_object_set_data (G_OBJECT (menu_bar), "accel", accel_group);

	g_signal_connect (G_OBJECT (menu_bar), "can-activate-accel",
							G_CALLBACK (menu_canacaccel), 0);

	/* set the initial state of toggles */
	mymenu[MENUBAR_OFFSET].state = !prefs.hex_gui_hide_menu;
	mymenu[MENUBAR_OFFSET+1].state = prefs.hex_gui_topicbar;
	mymenu[MENUBAR_OFFSET+2].state = !prefs.hex_gui_ulist_hide;
	mymenu[MENUBAR_OFFSET+3].state = prefs.hex_gui_ulist_buttons;
	mymenu[MENUBAR_OFFSET+4].state = prefs.hex_gui_mode_buttons;

	mymenu[AWAY_OFFSET].state = away;

	switch (prefs.hex_gui_tab_layout)
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
	switch (prefs.hex_gui_lagometer)
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
			if (!g_ascii_strcasecmp (key_theme, "Emacs"))
			{
				close_mask = STATE_SHIFT | STATE_CTRL;
				mymenu[SEARCH_OFFSET].key = 0;
			}
			g_free (key_theme);
		}
	}

	/* Away binding to ctrl-alt-a if the _Help menu conflicts (FR/PT/IT) */
	{
		char *help = _("_Help");
		char *under = strchr (help, '_');
		if (under && (under[1] == 'a' || under[1] == 'A'))
			away_mask = STATE_ALT | STATE_CTRL;
	}

	if (!toplevel)
	{
		mymenu[DETACH_OFFSET].text = N_("_Detach");
		mymenu[CLOSE_OFFSET].text = N_("_Close");
	}
	else
	{
		mymenu[DETACH_OFFSET].text = N_("_Attach");
		mymenu[CLOSE_OFFSET].text = N_("_Close");
	}

	while (1)
	{
		item = NULL;
		if (mymenu[i].id == MENU_ID_USERMENU && !prefs.hex_gui_usermenu)
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
#ifdef HAVE_GTK_MAC /* Added to app menu, see below */
			if (!bar || mymenu[i].id != MENU_ID_HEXCHAT)		
#endif
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
										mymenu[i].key == GDK_KEY_F1 ? 0 :
										mymenu[i].key == GDK_KEY_w ? close_mask :
										(g_ascii_isupper (mymenu[i].key)) ?
											STATE_SHIFT | STATE_CTRL :
											STATE_CTRL,
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
											mymenu[i].key,
											mymenu[i].id == MENU_ID_FULLSCREEN ? 0 :
											mymenu[i].id == MENU_ID_AWAY ? away_mask :
											STATE_CTRL, GTK_ACCEL_VISIBLE);
			if (mymenu[i].callback)
				g_signal_connect (G_OBJECT (item), "toggled",
									G_CALLBACK (mymenu[i].callback), NULL);

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
					menu_add_plugin_mainmenu_items (menu_bar);
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

#ifdef HAVE_GTK_MAC
		/* We want HexChat to be the app menu, not including Quit or HexChat itself */
		if (bar && item && i <= CLOSE_OFFSET + 1 && mymenu[i].id != MENU_ID_HEXCHAT)
		{
			if (!submenu || mymenu[i].type == M_MENUSUB)
				gtkosx_application_insert_app_menu_item (osx_app, item, appmenu_offset++);
		}
#endif

		i++;
	}
}
