/* X-Chat
 * Copyright (C) 2004-2008 Peter Zelezny.
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
#include <ctype.h>

#include <gdk/gdkkeysyms.h>

#include "../common/hexchat.h"
#include "../common/hexchatc.h"
#include "../common/servlist.h"
#include "../common/cfgfiles.h"
#include "../common/fe.h"
#include "../common/util.h"

#include "fe-gtk.h"
#include "gtkutil.h"
#include "menu.h"
#include "pixmaps.h"
#include "fkeys.h"


/* servlistgui.c globals */
static GtkWidget *serverlist_win = NULL;
static GtkWidget *networks_tree;	/* network TreeView */
static int ignore_changed = FALSE;
#ifdef WIN32
static int win_width = 324;
static int win_height = 426;
#else
static int win_width = 364;
static int win_height = 478;
#endif

/* global user info */
static GtkWidget *entry_nick1;
static GtkWidget *entry_nick2;
static GtkWidget *entry_nick3;
static GtkWidget *entry_guser;
/* static GtkWidget *entry_greal; */

/* edit area */
static GtkWidget *edit_win;
static GtkWidget *edit_entry_nick;
static GtkWidget *edit_entry_nick2;
static GtkWidget *edit_entry_user;
static GtkWidget *edit_entry_real;
static GtkWidget *edit_entry_join;
static GtkWidget *edit_entry_pass;
static GtkWidget *edit_entry_saslpass;
static GtkWidget *edit_entry_cmd;
static GtkWidget *edit_entry_nickserv;
static GtkWidget *edit_label_nick;
static GtkWidget *edit_label_nick2;
static GtkWidget *edit_label_real;
static GtkWidget *edit_label_user;
static GtkWidget *edit_tree;

static ircnet *selected_net = NULL;
static ircserver *selected_serv = NULL;
static ircnet *fav_add_net = NULL; /* used in Add/Remove fav context menus */
static session *servlist_sess;

static void servlist_network_row_cb (GtkTreeSelection *sel, gpointer user_data);
static GtkWidget *servlist_open_edit (GtkWidget *parent, ircnet *net);


static const char *pages[]=
{
	IRC_DEFAULT_CHARSET,
	"ISO-8859-15 (Western Europe)",
	"ISO-8859-2 (Central Europe)",
	"ISO-8859-7 (Greek)",
	"ISO-8859-8 (Hebrew)",
	"ISO-8859-9 (Turkish)",
	"ISO-2022-JP (Japanese)",
	"SJIS (Japanese)",
	"CP949 (Korean)",
	"KOI8-R (Cyrillic)",
	"CP1251 (Cyrillic)",
	"CP1256 (Arabic)",
	"CP1257 (Baltic)",
	"GB18030 (Chinese)",
	"TIS-620 (Thai)",
	NULL
};

static void
servlist_select_and_show (GtkTreeView *treeview, GtkTreeIter *iter,
								  GtkListStore *store)
{
	GtkTreePath *path;
	GtkTreeSelection *sel;

	sel = gtk_tree_view_get_selection (treeview);

	/* select this network */
	gtk_tree_selection_select_iter (sel, iter);
	/* and make sure it's visible */
	path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), iter);
	if (path)
	{
		gtk_tree_view_scroll_to_cell (treeview, path, NULL, TRUE, 0.5, 0.5);
		gtk_tree_view_set_cursor (treeview, path, NULL, FALSE);
		gtk_tree_path_free (path);
	}
}

static void
servlist_servers_populate (ircnet *net, GtkWidget *treeview)
{
	GtkListStore *store;
	GtkTreeIter iter;
	int i;
	ircserver *serv;
	GSList *list = net->servlist;

	store = (GtkListStore *)gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	gtk_list_store_clear (store);

	i = 0;
	while (list)
	{
		serv = list->data;
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, serv->hostname, 1, 1, -1);

		if (net->selected == i)
			/* select this server */
			servlist_select_and_show (GTK_TREE_VIEW (treeview), &iter, store);

		i++;
		list = list->next;
	}
}

static void
servlist_networks_populate_ (GtkWidget *treeview, GSList *netlist, gboolean favorites)
{
	GtkListStore *store;
	GtkTreeIter iter;
	int i;
	ircnet *net;

	if (!netlist)
	{
		net = servlist_net_add (_("New Network"), "", FALSE);
		servlist_server_add (net, "newserver/6667");
		netlist = network_list;
	}
	store = (GtkListStore *)gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	gtk_list_store_clear (store);

	i = 0;
	while (netlist)
	{
		net = netlist->data;
		if (!favorites || (net->flags & FLAG_FAVORITE))
		{
			if (favorites)
				gtk_list_store_insert_with_values (store, &iter, 0x7fffffff, 0, net->name, 1, 1, 2, 400, -1);
			else
				gtk_list_store_insert_with_values (store, &iter, 0x7fffffff, 0, net->name, 1, 1, 2, (net->flags & FLAG_FAVORITE) ? 800 : 400, -1);
			if (i == prefs.hex_gui_slist_select)
			{
				/* select this network */
				servlist_select_and_show (GTK_TREE_VIEW (treeview), &iter, store);
				selected_net = net;
			}
		}
		i++;
		netlist = netlist->next;
	}
}

static void
servlist_networks_populate (GtkWidget *treeview, GSList *netlist)
{
	servlist_networks_populate_ (treeview, netlist, prefs.hex_gui_slist_fav);
}

static void
servlist_server_row_cb (GtkTreeSelection *sel, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	ircserver *serv;
	char *servname;
	int pos;

	if (!selected_net)
		return;

	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, 0, &servname, -1);
		serv = servlist_server_find (selected_net, servname, &pos);
		g_free (servname);
		if (serv)
			selected_net->selected = pos;
		selected_serv = serv;
	}
}

static void
servlist_start_editing (GtkTreeView *tree)
{
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;

	sel = gtk_tree_view_get_selection (tree);

	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter);
		if (path)
		{
			gtk_tree_view_set_cursor (tree, path,
									gtk_tree_view_get_column (tree, 0), TRUE);
			gtk_tree_path_free (path);
		}
	}
}

static void
servlist_addserver_cb (GtkWidget *item, GtkWidget *treeview)
{
	GtkTreeIter iter;
	GtkListStore *store;

	if (!selected_net)
		return;

	store = (GtkListStore *)gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	servlist_server_add (selected_net, "newserver/6667");

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, "newserver/6667", 1, 1, -1);

	/* select this server */
	servlist_select_and_show (GTK_TREE_VIEW (treeview), &iter, store);
	/*servlist_start_editing (GTK_TREE_VIEW (treeview));*/

	servlist_server_row_cb (gtk_tree_view_get_selection (GTK_TREE_VIEW (networks_tree)), NULL);
}

static void
servlist_addnet_cb (GtkWidget *item, GtkTreeView *treeview)
{
	GtkTreeIter iter;
	GtkListStore *store;
	ircnet *net;

	net = servlist_net_add (_("New Network"), "", TRUE);
	net->encoding = strdup (IRC_DEFAULT_CHARSET);
	servlist_server_add (net, "newserver/6667");

	store = (GtkListStore *)gtk_tree_view_get_model (treeview);
	gtk_list_store_prepend (store, &iter);
	gtk_list_store_set (store, &iter, 0, net->name, 1, 1, -1);

	/* select this network */
	servlist_select_and_show (GTK_TREE_VIEW (networks_tree), &iter, store);
	servlist_start_editing (GTK_TREE_VIEW (networks_tree));

	servlist_network_row_cb (gtk_tree_view_get_selection (GTK_TREE_VIEW (networks_tree)), NULL);
}

static void
servlist_deletenetwork (ircnet *net)
{
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;

	/* remove from GUI */
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (networks_tree));
	if (gtk_tree_selection_get_selected (sel, &model, &iter))
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

	/* remove from list */
	servlist_net_remove (net);

	/* force something to be selected */
	gtk_tree_model_get_iter_first (model, &iter);
	servlist_select_and_show (GTK_TREE_VIEW (networks_tree), &iter,
									  GTK_LIST_STORE (model));
	servlist_network_row_cb (sel, NULL);
}

static void
servlist_deletenetdialog_cb (GtkDialog *dialog, gint arg1, ircnet *net)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
	if (arg1 == GTK_RESPONSE_OK)
		servlist_deletenetwork (net);
}

static void
servlist_move_server (ircserver *serv, int delta)
{
	int pos;

	pos = g_slist_index (selected_net->servlist, serv);
	if (pos >= 0)
	{
		pos += delta;
		if (pos >= 0)
		{
			selected_net->servlist = g_slist_remove (selected_net->servlist, serv);
			selected_net->servlist = g_slist_insert (selected_net->servlist, serv, pos);
			servlist_servers_populate (selected_net, edit_tree);
		}
	}
}

static void
servlist_move_network (ircnet *net, int delta)
{
	int pos;

	pos = g_slist_index (network_list, net);
	if (pos >= 0)
	{
		pos += delta;
		if (pos >= 0)
		{
			/*prefs.hex_gui_slist_select += delta;*/
			network_list = g_slist_remove (network_list, net);
			network_list = g_slist_insert (network_list, net, pos);
			servlist_networks_populate (networks_tree, network_list);
		}
	}
}

static gboolean
servlist_net_keypress_cb (GtkWidget *wid, GdkEventKey *evt, gpointer tree)
{
	if (!selected_net)
		return FALSE;

	if (evt->state & STATE_SHIFT)
	{
		if (evt->keyval == GDK_Up)
		{
			servlist_move_network (selected_net, -1);
		}
		else if (evt->keyval == GDK_Down)
		{
			servlist_move_network (selected_net, +1);
		}
	}

	return FALSE;
}

static gboolean
servlist_serv_keypress_cb (GtkWidget *wid, GdkEventKey *evt, gpointer userdata)
{
	if (!selected_net || !selected_serv)
		return FALSE;

	if (evt->state & STATE_SHIFT)
	{
		if (evt->keyval == GDK_Up)
		{
			servlist_move_server (selected_serv, -1);
		}
		else if (evt->keyval == GDK_Down)
		{
			servlist_move_server (selected_serv, +1);
		}
	}

	return FALSE;
}

static gint
servlist_compare (ircnet *net1, ircnet *net2)
{
	gchar *net1_casefolded, *net2_casefolded;
	int result=0;

	net1_casefolded=g_utf8_casefold(net1->name,-1),
	net2_casefolded=g_utf8_casefold(net2->name,-1),

	result=g_utf8_collate(net1_casefolded,net2_casefolded);

	g_free(net1_casefolded);
	g_free(net2_casefolded);

	return result;

}

static void
servlist_sort (GtkWidget *button, gpointer none)
{
	network_list=g_slist_sort(network_list,(GCompareFunc)servlist_compare);
	servlist_networks_populate (networks_tree, network_list);
}

static gboolean
servlist_has_selection (GtkTreeView *tree)
{
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;

	/* make sure something is selected */
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	return gtk_tree_selection_get_selected (sel, &model, &iter);
}

static void
servlist_favor (GtkWidget *button, gpointer none)
{
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (!selected_net)
		return;

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (networks_tree));
	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		if (selected_net->flags & FLAG_FAVORITE)
		{
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, 2, 400, -1);
			selected_net->flags &= ~FLAG_FAVORITE;
		}
		else
		{
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, 2, 800, -1);
			selected_net->flags |= FLAG_FAVORITE;
		}
	}
}

static void
servlist_update_from_entry (char **str, GtkWidget *entry)
{
	if (*str)
		free (*str);

	if (GTK_ENTRY (entry)->text[0] == 0)
		*str = NULL;
	else
		*str = strdup (GTK_ENTRY (entry)->text);
}

static void
servlist_edit_update (ircnet *net)
{
	servlist_update_from_entry (&net->nick, edit_entry_nick);
	servlist_update_from_entry (&net->nick2, edit_entry_nick2);
	servlist_update_from_entry (&net->user, edit_entry_user);
	servlist_update_from_entry (&net->real, edit_entry_real);

	servlist_update_from_entry (&net->autojoin, edit_entry_join);
	servlist_update_from_entry (&net->command, edit_entry_cmd);
	servlist_update_from_entry (&net->nickserv, edit_entry_nickserv);
	servlist_update_from_entry (&net->pass, edit_entry_pass);
	servlist_update_from_entry (&net->saslpass, edit_entry_saslpass);
}

static void
servlist_edit_close_cb (GtkWidget *button, gpointer userdata)
{
	if (selected_net)
		servlist_edit_update (selected_net);

	gtk_widget_destroy (edit_win);
	edit_win = NULL;
}

static gint
servlist_editwin_delete_cb (GtkWidget *win, GdkEventAny *event, gpointer none)
{
	servlist_edit_close_cb (NULL, NULL);
	return FALSE;
}

static gboolean
servlist_configure_cb (GtkWindow *win, GdkEventConfigure *event, gpointer none)
{
	/* remember the window size */
	gtk_window_get_size (win, &win_width, &win_height);
	return FALSE;
}

static void
servlist_edit_cb (GtkWidget *but, gpointer none)
{
	if (!servlist_has_selection (GTK_TREE_VIEW (networks_tree)))
		return;

	edit_win = servlist_open_edit (serverlist_win, selected_net);
	gtkutil_set_icon (edit_win);
	servlist_servers_populate (selected_net, edit_tree);
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (edit_tree))),
							"changed", G_CALLBACK (servlist_server_row_cb), NULL);
	g_signal_connect (G_OBJECT (edit_win), "delete_event",
						 	G_CALLBACK (servlist_editwin_delete_cb), 0);
	g_signal_connect (G_OBJECT (edit_tree), "key_press_event",
							G_CALLBACK (servlist_serv_keypress_cb), 0);
	gtk_widget_show (edit_win);
}

static void
servlist_deletenet_cb (GtkWidget *item, ircnet *net)
{
	GtkWidget *dialog;

	if (!servlist_has_selection (GTK_TREE_VIEW (networks_tree)))
		return;

	net = selected_net;
	if (!net)
		return;
	dialog = gtk_message_dialog_new (GTK_WINDOW (serverlist_win),
												GTK_DIALOG_DESTROY_WITH_PARENT |
												GTK_DIALOG_MODAL,
												GTK_MESSAGE_QUESTION,
												GTK_BUTTONS_OK_CANCEL,
							_("Really remove network \"%s\" and all its servers?"),
												net->name);
	g_signal_connect (dialog, "response",
							G_CALLBACK (servlist_deletenetdialog_cb), net);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
	gtk_widget_show (dialog);
}

static void
servlist_deleteserver (ircserver *serv, GtkTreeModel *model)
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;

	/* don't remove the last server */
	if (selected_net && g_slist_length (selected_net->servlist) < 2)
		return;

	/* remove from GUI */
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (edit_tree));
	if (gtk_tree_selection_get_selected (sel, &model, &iter))
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

	/* remove from list */
	if (selected_net)
		servlist_server_remove (selected_net, serv);
}

static void
servlist_editserverbutton_cb (GtkWidget *item, gpointer none)
{
	servlist_start_editing (GTK_TREE_VIEW (edit_tree));
}

static void
servlist_deleteserver_cb (GtkWidget *item, gpointer none)
{
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *servname;
	ircserver *serv;
	int pos;

	/* find the selected item in the GUI */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (edit_tree));
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (edit_tree));

	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, 0, &servname, -1);
		serv = servlist_server_find (selected_net, servname, &pos);
		g_free (servname);
		if (serv)
			servlist_deleteserver (serv, model);
	}
}

static ircnet *
servlist_find_selected_net (GtkTreeSelection *sel)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *netname;
	int pos;
	ircnet *net = NULL;

	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, 0, &netname, -1);
		net = servlist_net_find (netname, &pos, strcmp);
		g_free (netname);
		if (net)
			prefs.hex_gui_slist_select = pos;
	}

	return net;
}

static void
servlist_network_row_cb (GtkTreeSelection *sel, gpointer user_data)
{
	ircnet *net;

	selected_net = NULL;

	net = servlist_find_selected_net (sel);
	if (net)
		selected_net = net;
}

static int
servlist_savegui (void)
{
	char *sp;

	/* check for blank username, ircd will not allow this */
	if (GTK_ENTRY (entry_guser)->text[0] == 0)
		return 1;

	/* if (GTK_ENTRY (entry_greal)->text[0] == 0)
		return 1; */

	strcpy (prefs.hex_irc_nick1, GTK_ENTRY (entry_nick1)->text);
	strcpy (prefs.hex_irc_nick2, GTK_ENTRY (entry_nick2)->text);
	strcpy (prefs.hex_irc_nick3, GTK_ENTRY (entry_nick3)->text);
	strcpy (prefs.hex_irc_user_name, GTK_ENTRY (entry_guser)->text);
	sp = strchr (prefs.hex_irc_user_name, ' ');
	if (sp)
		sp[0] = 0;	/* spaces will break the login */
	/* strcpy (prefs.hex_irc_real_name, GTK_ENTRY (entry_greal)->text); */
	servlist_save ();

	return 0;
}

static gboolean
servlist_get_iter_from_name (GtkTreeModel *model, gchar *name, GtkTreeIter *iter)
{
	GtkTreePath *path = gtk_tree_path_new_from_string (name);

	if (!gtk_tree_model_get_iter (model, iter, path))
	{
		gtk_tree_path_free (path);
		return FALSE;
	}

	gtk_tree_path_free (path);
	return TRUE;
}

static void
servlist_editchannel_cb (GtkCellRendererText *cell, gchar *name, gchar *newval, GtkTreeModel *model)
{
	GtkTreeIter iter;
	static int loop_guard = FALSE;

	if (loop_guard)
		return;

	if (!servlist_get_iter_from_name (model, name, &iter))
		return;

	loop_guard = TRUE;
	/* delete empty item */
	if (newval[0] == 0)
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
	else
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, newval, -1);
	loop_guard = FALSE;
}

static void
servlist_editkey_cb (GtkCellRendererText *cell, gchar *name, gchar *newval, GtkTreeModel *model)
{
	GtkTreeIter iter;

	if (!servlist_get_iter_from_name (model, name, &iter))
		return;

	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 1, newval, -1);
}

static void
servlist_addchannel (GtkWidget *tree, char *channel)
{
	GtkTreeIter iter;
	GtkListStore *store;

	store = (GtkListStore *)gtk_tree_view_get_model (GTK_TREE_VIEW (tree));

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, channel, 1, "", 2, TRUE, -1);

	/* select this server */
	servlist_select_and_show (GTK_TREE_VIEW (tree), &iter, store);
	servlist_start_editing (GTK_TREE_VIEW (tree));
}

static void
servlist_addchannel_cb (GtkWidget *item, GtkWidget *tree)
{
	servlist_addchannel (tree, _("#channel"));
}

static void
servlist_deletechannel_cb (GtkWidget *item, GtkWidget *tree)
{
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;

	/* find the selected item in the GUI */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

	if (gtk_tree_selection_get_selected (sel, &model, &iter))
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}

static void
servlist_editchannelbutton_cb (GtkWidget *item, GtkWidget *tree)
{
	servlist_start_editing (GTK_TREE_VIEW (tree));
}

/* save everything from the GUI to the GtkEntry */

static void
servlist_autojoineditok_cb (GtkWidget *button, GtkWidget *tree)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *channel, *key;
	char *autojoin;
	GSList *channels = NULL, *keys = NULL;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));

	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			gtk_tree_model_get (model, &iter, 0, &channel, 1, &key, -1);
			channels = g_slist_append (channels, channel);
			if (key && key[0] == 0)
			{
				/* NULL out empty keys */
				g_free (key);
				keys = g_slist_append (keys, NULL);				
			}
			else
				keys = g_slist_append (keys, key);
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}

	gtk_widget_destroy (gtk_widget_get_toplevel (button));

	autojoin = joinlist_merge (channels, keys);
	if (autojoin)
	{
		if (edit_win && selected_net)
			gtk_entry_set_text (GTK_ENTRY (edit_entry_join), autojoin);
		else
		{
			if (fav_add_net->autojoin)
				free (fav_add_net->autojoin);
			fav_add_net->autojoin = strdup (autojoin);
		}
		g_free (autojoin);
	}

	/* this does g_free too */
	joinlist_free (channels, keys);

	if (fav_add_net)
		servlist_save ();
}

void
servlist_autojoinedit (ircnet *net, char *channel, gboolean add)
{
	GtkWidget *win;
	GtkWidget *scrolledwindow;
	GtkTreeModel *model;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkWidget *tree;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *label2;
	GtkWidget *bbox;
	GtkWidget *wid;

	GtkWidget *vbuttonbox1;
	GtkWidget *buttonadd;
	GtkWidget *buttonremove;
	GtkWidget *buttonedit;

	char buf[128];
	char lab[128];
	GSList *channels, *keys;
	GSList *clist, *klist;
	GtkTreeIter iter;

	if (edit_win && selected_net)
		/* update net->autojoin */
		servlist_edit_update (selected_net);

	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width (GTK_CONTAINER (win), 4);
	gtk_window_set_title (GTK_WINDOW (win), _(DISPLAY_NAME": Favorite Channels (Auto-Join List)"));
	gtk_window_set_default_size (GTK_WINDOW (win), 354, 256);
	gtk_window_set_position (GTK_WINDOW (win), GTK_WIN_POS_MOUSE);
	if (edit_win)
		gtk_window_set_transient_for (GTK_WINDOW (win), GTK_WINDOW (edit_win));
	gtk_window_set_modal (GTK_WINDOW (win), TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (win), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_role (GTK_WINDOW (win), "editserv");

	table = gtk_table_new (1, 1, FALSE);
	gtk_container_add (GTK_CONTAINER (win), table);
	gtk_widget_show (table);

	snprintf (buf, sizeof (buf), _("These channels will be joined whenever you connect to %s."), net->name);
	label = gtk_label_new (buf);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
	gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1, GTK_FILL|GTK_EXPAND, GTK_FILL, 3, 3);
	gtk_widget_show (label);

	label2 = gtk_label_new ("");
	gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_CENTER);
	gtk_table_attach (GTK_TABLE (table), label2, 0, 2, 1, 2, GTK_FILL, 0, 3, 3);
	gtk_widget_show (label2);

	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_table_attach (GTK_TABLE (table), scrolledwindow, 0, 1, 2, 3, GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
											  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
													 GTK_SHADOW_IN);
	gtk_widget_show (scrolledwindow);

	store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
	model = GTK_TREE_MODEL (store);

	tree = gtk_tree_view_new_with_model (model);
	g_object_unref (model);
	gtk_container_add (GTK_CONTAINER (scrolledwindow), tree);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), TRUE);
	gtk_widget_show (tree);

	renderer = gtk_cell_renderer_text_new ();
	g_signal_connect (G_OBJECT (renderer), "edited",
							G_CALLBACK (servlist_editchannel_cb), model);
	gtk_tree_view_insert_column_with_attributes (
								GTK_TREE_VIEW (tree), -1,
						 		_("Channel"), renderer,
						 		"text", 0,
								"editable", 2,
								NULL);

	renderer = gtk_cell_renderer_text_new ();
	g_signal_connect (G_OBJECT (renderer), "edited",
							G_CALLBACK (servlist_editkey_cb), model);
	gtk_tree_view_insert_column_with_attributes (
								GTK_TREE_VIEW (tree), -1,
						 		_("Key (Password)"), renderer,
						 		"text", 1,
								"editable", 2,
								NULL);

	gtk_tree_view_column_set_expand (gtk_tree_view_get_column (GTK_TREE_VIEW (tree), 0), TRUE);
	gtk_tree_view_column_set_expand (gtk_tree_view_get_column (GTK_TREE_VIEW (tree), 1), TRUE);

	gtk_tree_sortable_set_sort_column_id ((GtkTreeSortable *)model, 0, GTK_SORT_ASCENDING);

	/* ===BUTTONS=== */
	vbuttonbox1 = gtk_vbutton_box_new ();
	gtk_box_set_spacing (GTK_BOX (vbuttonbox1), 3);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (vbuttonbox1), GTK_BUTTONBOX_START);
	gtk_widget_show (vbuttonbox1);
	gtk_table_attach (GTK_TABLE (table), vbuttonbox1, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 0);

	buttonadd = gtk_button_new_from_stock ("gtk-add");
	g_signal_connect (G_OBJECT (buttonadd), "clicked",
							G_CALLBACK (servlist_addchannel_cb), tree);
	gtk_widget_show (buttonadd);
	gtk_container_add (GTK_CONTAINER (vbuttonbox1), buttonadd);
	GTK_WIDGET_SET_FLAGS (buttonadd, GTK_CAN_DEFAULT);

	buttonremove = gtk_button_new_from_stock ("gtk-remove");
	g_signal_connect (G_OBJECT (buttonremove), "clicked",
							G_CALLBACK (servlist_deletechannel_cb), tree);
	gtk_widget_show (buttonremove);
	gtk_container_add (GTK_CONTAINER (vbuttonbox1), buttonremove);
	GTK_WIDGET_SET_FLAGS (buttonremove, GTK_CAN_DEFAULT);

	buttonedit = gtk_button_new_with_mnemonic (_("_Edit"));
	g_signal_connect (G_OBJECT (buttonedit), "clicked",
							G_CALLBACK (servlist_editchannelbutton_cb), tree);
	gtk_widget_show (buttonedit);
	gtk_container_add (GTK_CONTAINER (vbuttonbox1), buttonedit);
	GTK_WIDGET_SET_FLAGS (buttonedit, GTK_CAN_DEFAULT);

	bbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX (bbox), 4);
	gtk_table_attach (GTK_TABLE (table), bbox, 0, 1, 3, 4, GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 4);
	gtk_widget_show (bbox);

	wid = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	g_signal_connect (G_OBJECT (wid), "clicked", G_CALLBACK (gtkutil_destroy), win);
	gtk_container_add (GTK_CONTAINER (bbox), wid);
	gtk_widget_show (wid);

	wid = gtk_button_new_from_stock (GTK_STOCK_OK);
	g_signal_connect (G_OBJECT (wid), "clicked", G_CALLBACK (servlist_autojoineditok_cb), tree);
	gtk_container_add (GTK_CONTAINER (bbox), wid);
	gtk_widget_show (wid);
	gtk_widget_grab_focus (wid);
	/* =========== */

	if (net->autojoin)
	{
		joinlist_split (net->autojoin, &channels, &keys);

		clist = channels;
		klist = keys;

		while (clist)
		{
			if (channel && !add && !rfc_casecmp (channel, clist->data))
			{
				snprintf (buf, sizeof (buf), _("%s has been removed."), channel);
				snprintf (lab, sizeof (lab), "<span foreground=\"#2222DD\">%s</span>", buf);
				gtk_label_set_markup (GTK_LABEL (label2), lab);
			}
			else
			{
				gtk_list_store_append (store, &iter);
				gtk_list_store_set (store, &iter, 0, clist->data, 1, klist->data, 2, TRUE, -1);
			}

			klist = klist->next;
			clist = clist->next;
		}

		joinlist_free (channels, keys);
	}

	if (channel && add)
	{
		servlist_addchannel (tree, channel);
		snprintf (buf, sizeof (buf), _("%s has been added."), channel);
		snprintf (lab, sizeof (lab), "<span foreground=\"#2222DD\">%s</span>", buf);
		gtk_label_set_markup (GTK_LABEL (label2), lab);
	}

	fav_add_net = net;

	gtk_widget_show (win);
}

static void
servlist_autojoinedit_cb (GtkWidget *button, ircnet *net)
{
	servlist_autojoinedit (net, NULL, FALSE);
}

static void
servlist_connect_cb (GtkWidget *button, gpointer userdata)
{
	if (!selected_net)
		return;

	if (servlist_savegui () != 0)
	{
		fe_message (_("User name and Real name cannot be left blank."), FE_MSG_ERROR);
		return;
	}

 	if (!is_session (servlist_sess))
		servlist_sess = NULL;	/* open a new one */

	{
		GSList *list;
		session *sess;
		session *chosen = servlist_sess;

		servlist_sess = NULL;	/* open a new one */

		for (list = sess_list; list; list = list->next)
		{
			sess = list->data;
			if (sess->server->network == selected_net)
			{
				servlist_sess = sess;
				if (sess->server->connected)
					servlist_sess = NULL;	/* open a new one */
				break;
			}
		}

		/* use the chosen one, if it's empty */
		if (!servlist_sess &&
			  chosen &&
			 !chosen->server->connected &&
			  chosen->server->server_session->channel[0] == 0)
		{
			servlist_sess = chosen;
		}
	}

	servlist_connect (servlist_sess, selected_net, TRUE);

	gtk_widget_destroy (serverlist_win);
	serverlist_win = NULL;
	selected_net = NULL;
}

static void
servlist_celledit_cb (GtkCellRendererText *cell, gchar *arg1, gchar *arg2,
							 gpointer user_data)
{
	GtkTreeModel *model = (GtkTreeModel *)user_data;
	GtkTreeIter iter;
	GtkTreePath *path;
	char *netname;
	ircnet *net;

	if (!arg1 || !arg2)
		return;

	path = gtk_tree_path_new_from_string (arg1);
	if (!path)
		return;

	if (!gtk_tree_model_get_iter (model, &iter, path))
	{
		gtk_tree_path_free (path);
		return;
	}
	gtk_tree_model_get (model, &iter, 0, &netname, -1);

	net = servlist_net_find (netname, NULL, strcmp);
	g_free (netname);
	if (net)
	{
		/* delete empty item */
		if (arg2[0] == 0)
		{
			servlist_deletenetwork (net);
			gtk_tree_path_free (path);
			return;
		}

		netname = net->name;
		net->name = strdup (arg2);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, net->name, -1);
		free (netname);
	}

	gtk_tree_path_free (path);
}

static void
servlist_check_cb (GtkWidget *but, gpointer num_p)
{
	int num = GPOINTER_TO_INT (num_p);

	if (!selected_net)
		return;

	if ((1 << num) == FLAG_CYCLE || (1 << num) == FLAG_USE_PROXY)
	{
		/* these ones are reversed, so it's compat with 2.0.x */
		if (GTK_TOGGLE_BUTTON (but)->active)
			selected_net->flags &= ~(1 << num);
		else
			selected_net->flags |= (1 << num);
	} else
	{
		if (GTK_TOGGLE_BUTTON (but)->active)
			selected_net->flags |= (1 << num);
		else
			selected_net->flags &= ~(1 << num);
	}

	if ((1 << num) == FLAG_USE_GLOBAL)
	{
		if (GTK_TOGGLE_BUTTON (but)->active)
		{
			gtk_widget_hide (edit_label_nick);
			gtk_widget_hide (edit_entry_nick);

			gtk_widget_hide (edit_label_nick2);
			gtk_widget_hide (edit_entry_nick2);

			gtk_widget_hide (edit_label_user);
			gtk_widget_hide (edit_entry_user);

			gtk_widget_hide (edit_label_real);
			gtk_widget_hide (edit_entry_real);
		} else
		{
			gtk_widget_show (edit_label_nick);
			gtk_widget_show (edit_entry_nick);

			gtk_widget_show (edit_label_nick2);
			gtk_widget_show (edit_entry_nick2);

			gtk_widget_show (edit_label_user);
			gtk_widget_show (edit_entry_user);

			gtk_widget_show (edit_label_real);
			gtk_widget_show (edit_entry_real);
		}
	}
}

static GtkWidget *
servlist_create_check (int num, int state, GtkWidget *table, int row, int col, char *labeltext)
{
	GtkWidget *but;

	but = gtk_check_button_new_with_label (labeltext);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (but), state);
	g_signal_connect (G_OBJECT (but), "toggled",
							G_CALLBACK (servlist_check_cb), GINT_TO_POINTER (num));
	gtk_table_attach (GTK_TABLE (table), but, col, col+2, row, row+1,
						   GTK_FILL|GTK_EXPAND, 0, 0, 0);
	gtk_widget_show (but);

	return but;
}

static GtkWidget *
servlist_create_entry (GtkWidget *table, char *labeltext, int row,
							  char *def, GtkWidget **label_ret, char *tip)
{
	GtkWidget *label, *entry;

	label = gtk_label_new_with_mnemonic (labeltext);
	if (label_ret)
		*label_ret = label;
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, row, row+1,
							GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

	entry = gtk_entry_new ();
	add_tip (entry, tip);
	gtk_widget_show (entry);
	gtk_entry_set_text (GTK_ENTRY (entry), def ? def : "");
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

	if (row == 15)	/* for "Channels to Join:" */
	{
		GtkWidget *button, *box;

		box = gtk_hbox_new (0, 0);
		button = gtk_button_new_with_label ("...");
		g_signal_connect (G_OBJECT (button), "clicked",
								G_CALLBACK (servlist_autojoinedit_cb), selected_net);

		gtk_box_pack_start (GTK_BOX (box), entry, TRUE, TRUE, 0);
		gtk_box_pack_end (GTK_BOX (box), button, 0, 0, 0);
		gtk_widget_show_all (box);

		gtk_table_attach (GTK_TABLE (table), box, 2, 3, row, row+1,
								GTK_FILL|GTK_EXPAND, 0, 0, 0);
	}
	else
	{
		gtk_table_attach (GTK_TABLE (table), entry, 2, 3, row, row+1,
								GTK_FILL|GTK_EXPAND, 0, 0, 0);
	}

	return entry;
}

static gint
servlist_delete_cb (GtkWidget *win, GdkEventAny *event, gpointer userdata)
{
	servlist_savegui ();
	serverlist_win = NULL;
	selected_net = NULL;

	if (sess_list == NULL)
		hexchat_exit ();

	return FALSE;
}

static void
servlist_close_cb (GtkWidget *button, gpointer userdata)
{
	servlist_savegui ();
	gtk_widget_destroy (serverlist_win);
	serverlist_win = NULL;
	selected_net = NULL;

	if (sess_list == NULL)
		hexchat_exit ();
}

/* convert "host:port" format to "host/port" */

static char *
servlist_sanitize_hostname (char *host)
{
	char *ret, *c, *e;

	ret = strdup (host);

	c = strchr  (ret, ':');
	e = strrchr (ret, ':');

	/* if only one colon exists it's probably not IPv6 */
	if (c && c == e)
		*c = '/';

	return ret;
}

static void
servlist_editserver_cb (GtkCellRendererText *cell, gchar *arg1, gchar *arg2,
								gpointer user_data)
{
	GtkTreeModel *model = (GtkTreeModel *)user_data;
	GtkTreeIter iter;
	GtkTreePath *path;
	char *servname;
	ircserver *serv;

	if (!selected_net)
		return;

	path = gtk_tree_path_new_from_string (arg1);

	if (!gtk_tree_model_get_iter (model, &iter, path))
	{
		gtk_tree_path_free (path);
		return;
	}

	gtk_tree_model_get (model, &iter, 0, &servname, -1);
	serv = servlist_server_find (selected_net, servname, NULL);
	g_free (servname);

	if (serv)
	{
		/* delete empty item */
		if (arg2[0] == 0)
		{
			servlist_deleteserver (serv, model);
			gtk_tree_path_free (path);
			return;
		}

		servname = serv->hostname;
		serv->hostname = servlist_sanitize_hostname (arg2);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, serv->hostname, -1);
		free (servname);
	}

	gtk_tree_path_free (path);
}

static void
servlist_combo_cb (GtkEntry *entry, gpointer userdata)
{
	if (!selected_net)
		return;

	if (!ignore_changed)
	{
		if (selected_net->encoding)
			free (selected_net->encoding);
		selected_net->encoding = strdup (entry->text);
	}
}

static GtkWidget *
servlist_create_charsetcombo (void)
{
	GtkWidget *cb;
	int i;

	cb = gtk_combo_box_entry_new_text ();
	gtk_combo_box_append_text (GTK_COMBO_BOX (cb), "System default");
	i = 0;
	while (pages[i])
	{
		gtk_combo_box_append_text (GTK_COMBO_BOX (cb), (char *)pages[i]);
		i++;
	}
	g_signal_connect (G_OBJECT (GTK_BIN (cb)->child), "changed",
							G_CALLBACK (servlist_combo_cb), NULL);

	return cb;
}

static void
no_servlist (GtkWidget * igad, gpointer serv)
{
	if (GTK_TOGGLE_BUTTON (igad)->active)
		prefs.hex_gui_slist_skip = TRUE;
	else
		prefs.hex_gui_slist_skip = FALSE;
}

static void
fav_servlist (GtkWidget * igad, gpointer serv)
{
	if (GTK_TOGGLE_BUTTON (igad)->active)
		prefs.hex_gui_slist_fav = TRUE;
	else
		prefs.hex_gui_slist_fav = FALSE;

	servlist_networks_populate (networks_tree, network_list);
}

static GtkWidget *
bold_label (char *text)
{
	char buf[128];
	GtkWidget *label;

	snprintf (buf, sizeof (buf), "<b>%s</b>", text);
	label = gtk_label_new (buf);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_widget_show (label);

	return label;
}

static GtkWidget *
servlist_open_edit (GtkWidget *parent, ircnet *net)
{
	GtkWidget *editwindow;
	GtkWidget *vbox5;
	GtkWidget *table3;
	GtkWidget *label17;
	GtkWidget *label16;
	GtkWidget *label21;
	GtkWidget *label34;
	GtkWidget *comboboxentry_charset;
	GtkWidget *hbox1;
	GtkWidget *scrolledwindow2;
	GtkWidget *treeview_servers;
	GtkWidget *vbuttonbox1;
	GtkWidget *buttonadd;
	GtkWidget *buttonremove;
	GtkWidget *buttonedit;
	GtkWidget *hseparator2;
	GtkWidget *hbuttonbox4;
	GtkWidget *button10;
	GtkWidget *check;
	GtkTreeModel *model;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	char buf[128];
	char buf2[128 + 8];

	editwindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width (GTK_CONTAINER (editwindow), 4);
	snprintf (buf, sizeof (buf), _(DISPLAY_NAME": Edit %s"), net->name);
	gtk_window_set_title (GTK_WINDOW (editwindow), buf);
	gtk_window_set_default_size (GTK_WINDOW (editwindow), 354, 0);
	gtk_window_set_position (GTK_WINDOW (editwindow), GTK_WIN_POS_MOUSE);
	gtk_window_set_transient_for (GTK_WINDOW (editwindow), GTK_WINDOW (parent));
	gtk_window_set_modal (GTK_WINDOW (editwindow), TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (editwindow), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_role (GTK_WINDOW (editwindow), "editserv");

	vbox5 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox5);
	gtk_container_add (GTK_CONTAINER (editwindow), vbox5);

	table3 = gtk_table_new (17, 3, FALSE);
	gtk_widget_show (table3);
	gtk_box_pack_start (GTK_BOX (vbox5), table3, TRUE, TRUE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table3), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table3), 8);

	snprintf (buf, sizeof (buf), _("Servers for %s"), net->name);
	snprintf (buf2, sizeof (buf2), "<b>%s</b>", buf);
	label16 = gtk_label_new (buf2);
	gtk_widget_show (label16);
	gtk_table_attach (GTK_TABLE (table3), label16, 0, 3, 0, 1,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 3);
	gtk_label_set_use_markup (GTK_LABEL (label16), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label16), 0, 0.5);

	check = servlist_create_check (0, !(net->flags & FLAG_CYCLE), table3,
								  2, 1, _("Connect to selected server only"));
	add_tip (check, _("Don't cycle through all the servers when the connection fails."));

	label17 = bold_label (_("Your Details"));
	gtk_table_attach (GTK_TABLE (table3), label17, 0, 3, 3, 4,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 3);

	servlist_create_check (1, net->flags & FLAG_USE_GLOBAL, table3,
								  4, 1, _("Use global user information"));

	edit_entry_nick =
		servlist_create_entry (table3, _("_Nick name:"), 5, net->nick,
									  &edit_label_nick, 0);

	edit_entry_nick2 =
		servlist_create_entry (table3, _("Second choice:"), 6, net->nick2,
									  &edit_label_nick2, 0);

	edit_entry_user =
		servlist_create_entry (table3, _("_User name:"), 7, net->user,
									  &edit_label_user, 0);

	edit_entry_real =
		servlist_create_entry (table3, _("Rea_l name:"), 8, net->real,
									  &edit_label_real, 0);

	label21 = bold_label (_("Connecting"));
	gtk_table_attach (GTK_TABLE (table3), label21, 0, 3, 9, 10,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 3);

	servlist_create_check (3, net->flags & FLAG_AUTO_CONNECT, table3,
								  11, 1, _("Auto connect to this network at startup"));
	servlist_create_check (4, !(net->flags & FLAG_USE_PROXY), table3,
								  12, 1, _("Bypass proxy server"));
	check = servlist_create_check (2, net->flags & FLAG_USE_SSL, table3,
								  13, 1, _("Use SSL for all the servers on this network"));
#ifndef USE_OPENSSL
	gtk_widget_set_sensitive (check, FALSE);
#endif
	check = servlist_create_check (5, net->flags & FLAG_ALLOW_INVALID, table3,
								  14, 1, _("Accept invalid SSL certificate"));
#ifndef USE_OPENSSL
	gtk_widget_set_sensitive (check, FALSE);
#endif

	edit_entry_join =
		servlist_create_entry (table3, _("_Favorite channels:"), 15,
									  net->autojoin, 0,
				  _("Channels to join, separated by commas, but not spaces!"));

	edit_entry_cmd =
		servlist_create_entry (table3, _("Connect command:"), 16,
									  net->command, 0,
					_("Extra command to execute after connecting. If you need more than one, set this to LOAD -e <filename>, where <filename> is a text-file full of commands to execute."));

	edit_entry_nickserv =
		servlist_create_entry (table3, _("Nickserv password:"), 17,
									  net->nickserv, 0,
					_("If your nickname requires a password, enter it here. Not all IRC networks support this."));
	gtk_entry_set_visibility (GTK_ENTRY (edit_entry_nickserv), FALSE);

	edit_entry_pass =
		servlist_create_entry (table3, _("Server password:"), 18,
									  net->pass, 0,
					_("Password for the server, if in doubt, leave blank."));
	gtk_entry_set_visibility (GTK_ENTRY (edit_entry_pass), FALSE);

	edit_entry_saslpass =
		servlist_create_entry (table3, _("SASL password:"), 19,
									  net->saslpass, 0,
					_("Password for SASL authentication, if in doubt, leave blank."));
	gtk_entry_set_visibility (GTK_ENTRY (edit_entry_saslpass), FALSE);

	label34 = gtk_label_new (_("Character set:"));
	gtk_widget_show (label34);
	gtk_table_attach (GTK_TABLE (table3), label34, 1, 2, 20, 21,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label34), 0, 0.5);

	comboboxentry_charset = servlist_create_charsetcombo ();
	ignore_changed = TRUE;
	gtk_entry_set_text (GTK_ENTRY (GTK_BIN (comboboxentry_charset)->child), net->encoding ? net->encoding : "System default");
	ignore_changed = FALSE;
	gtk_widget_show (comboboxentry_charset);
	gtk_table_attach (GTK_TABLE (table3), comboboxentry_charset, 2, 3, 20, 21,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (GTK_FILL), 0, 0);

	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox1);
	gtk_table_attach (GTK_TABLE (table3), hbox1, 1, 3, 1, 2,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

	scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow2);
	gtk_box_pack_start (GTK_BOX (hbox1), scrolledwindow2, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2),
											  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow2),
													 GTK_SHADOW_IN);

	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);
	model = GTK_TREE_MODEL (store);

	edit_tree = treeview_servers = gtk_tree_view_new_with_model (model);
	g_object_unref (model);
	gtk_widget_show (treeview_servers);
	gtk_container_add (GTK_CONTAINER (scrolledwindow2), treeview_servers);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview_servers),
												  FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_signal_connect (G_OBJECT (renderer), "edited",
							G_CALLBACK (servlist_editserver_cb), model);
	gtk_tree_view_insert_column_with_attributes (
								GTK_TREE_VIEW (treeview_servers), -1,
						 		0, renderer,
						 		"text", 0,
								"editable", 1,
								NULL);

	vbuttonbox1 = gtk_vbutton_box_new ();
	gtk_box_set_spacing (GTK_BOX (vbuttonbox1), 3);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (vbuttonbox1), GTK_BUTTONBOX_START);
	gtk_widget_show (vbuttonbox1);
	gtk_box_pack_start (GTK_BOX (hbox1), vbuttonbox1, FALSE, FALSE, 3);

	buttonadd = gtk_button_new_from_stock ("gtk-add");
	g_signal_connect (G_OBJECT (buttonadd), "clicked",
							G_CALLBACK (servlist_addserver_cb), edit_tree);
	gtk_widget_show (buttonadd);
	gtk_container_add (GTK_CONTAINER (vbuttonbox1), buttonadd);
	GTK_WIDGET_SET_FLAGS (buttonadd, GTK_CAN_DEFAULT);

	buttonremove = gtk_button_new_from_stock ("gtk-remove");
	g_signal_connect (G_OBJECT (buttonremove), "clicked",
							G_CALLBACK (servlist_deleteserver_cb), NULL);
	gtk_widget_show (buttonremove);
	gtk_container_add (GTK_CONTAINER (vbuttonbox1), buttonremove);
	GTK_WIDGET_SET_FLAGS (buttonremove, GTK_CAN_DEFAULT);

	buttonedit = gtk_button_new_with_mnemonic (_("_Edit"));
	g_signal_connect (G_OBJECT (buttonedit), "clicked",
							G_CALLBACK (servlist_editserverbutton_cb), NULL);
	gtk_widget_show (buttonedit);
	gtk_container_add (GTK_CONTAINER (vbuttonbox1), buttonedit);
	GTK_WIDGET_SET_FLAGS (buttonedit, GTK_CAN_DEFAULT);

	hseparator2 = gtk_hseparator_new ();
	gtk_widget_show (hseparator2);
	gtk_box_pack_start (GTK_BOX (vbox5), hseparator2, FALSE, FALSE, 8);

	hbuttonbox4 = gtk_hbutton_box_new ();
	gtk_widget_show (hbuttonbox4);
	gtk_box_pack_start (GTK_BOX (vbox5), hbuttonbox4, FALSE, FALSE, 0);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox4),
										GTK_BUTTONBOX_END);

	button10 = gtk_button_new_from_stock ("gtk-close");
	g_signal_connect (G_OBJECT (button10), "clicked",
							G_CALLBACK (servlist_edit_close_cb), 0);
	gtk_widget_show (button10);
	gtk_container_add (GTK_CONTAINER (hbuttonbox4), button10);
	GTK_WIDGET_SET_FLAGS (button10, GTK_CAN_DEFAULT);

	if (net->flags & FLAG_USE_GLOBAL)
	{
		gtk_widget_hide (edit_label_nick);
		gtk_widget_hide (edit_entry_nick);

		gtk_widget_hide (edit_label_nick2);
		gtk_widget_hide (edit_entry_nick2);

		gtk_widget_hide (edit_label_user);
		gtk_widget_hide (edit_entry_user);

		gtk_widget_hide (edit_label_real);
		gtk_widget_hide (edit_entry_real);
	}

	gtk_widget_grab_focus (button10);
	gtk_widget_grab_default (button10);

	return editwindow;
}

static GtkWidget *
servlist_open_networks (void)
{
	GtkWidget *servlist;
	GtkWidget *vbox1;
	GtkWidget *label2;
	GtkWidget *table1;
	GtkWidget *label3;
	GtkWidget *label4;
	GtkWidget *label5;
	GtkWidget *label6;
	/* GtkWidget *label7; */
	GtkWidget *entry1;
	GtkWidget *entry2;
	GtkWidget *entry3;
	GtkWidget *entry4;
	/* GtkWidget *entry5; */
	GtkWidget *vbox2;
	GtkWidget *label1;
	GtkWidget *table4;
	GtkWidget *scrolledwindow3;
	GtkWidget *treeview_networks;
	GtkWidget *checkbutton_skip;
	GtkWidget *checkbutton_fav;
	GtkWidget *hbox;
	GtkWidget *vbuttonbox2;
	GtkWidget *button_add;
	GtkWidget *button_remove;
	GtkWidget *button_edit;
	GtkWidget *button_sort;
	GtkWidget *hseparator1;
	GtkWidget *hbuttonbox1;
	GtkWidget *button_connect;
	GtkWidget *button_close;
	GtkTreeModel *model;
	GtkListStore *store;
	GtkCellRenderer *renderer;

	servlist = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width (GTK_CONTAINER (servlist), 4);
	gtk_window_set_title (GTK_WINDOW (servlist), _(DISPLAY_NAME": Network List"));
	gtk_window_set_default_size (GTK_WINDOW (servlist), win_width, win_height);
	gtk_window_set_position (GTK_WINDOW (servlist), GTK_WIN_POS_MOUSE);
	gtk_window_set_role (GTK_WINDOW (servlist), "servlist");
	gtk_window_set_type_hint (GTK_WINDOW (servlist), GDK_WINDOW_TYPE_HINT_DIALOG);
	if (current_sess)
		gtk_window_set_transient_for (GTK_WINDOW (servlist), GTK_WINDOW (current_sess->gui->window));

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (servlist), vbox1);

	label2 = bold_label (_("User Information"));
	gtk_box_pack_start (GTK_BOX (vbox1), label2, FALSE, FALSE, 0);

	table1 = gtk_table_new (5, 2, FALSE);
	gtk_widget_show (table1);
	gtk_box_pack_start (GTK_BOX (vbox1), table1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (table1), 8);
	gtk_table_set_row_spacings (GTK_TABLE (table1), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table1), 4);

	label3 = gtk_label_new_with_mnemonic (_("_Nick name:"));
	gtk_widget_show (label3);
	gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 0, 1,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);

	label4 = gtk_label_new (_("Second choice:"));
	gtk_widget_show (label4);
	gtk_table_attach (GTK_TABLE (table1), label4, 0, 1, 1, 2,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

	label5 = gtk_label_new (_("Third choice:"));
	gtk_widget_show (label5);
	gtk_table_attach (GTK_TABLE (table1), label5, 0, 1, 2, 3,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label5), 0, 0.5);

	label6 = gtk_label_new_with_mnemonic (_("_User name:"));
	gtk_widget_show (label6);
	gtk_table_attach (GTK_TABLE (table1), label6, 0, 1, 3, 4,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

	/* label7 = gtk_label_new_with_mnemonic (_("Rea_l name:"));
	gtk_widget_show (label7);
	gtk_table_attach (GTK_TABLE (table1), label7, 0, 1, 4, 5,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label7), 0, 0.5);*/

	entry_nick1 = entry1 = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry1), prefs.hex_irc_nick1);
	gtk_widget_show (entry1);
	gtk_table_attach (GTK_TABLE (table1), entry1, 1, 2, 0, 1,
							(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);

	entry_nick2 = entry2 = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry2), prefs.hex_irc_nick2);
	gtk_widget_show (entry2);
	gtk_table_attach (GTK_TABLE (table1), entry2, 1, 2, 1, 2,
							(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);

	entry_nick3 = entry3 = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry3), prefs.hex_irc_nick3);
	gtk_widget_show (entry3);
	gtk_table_attach (GTK_TABLE (table1), entry3, 1, 2, 2, 3,
							(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);

	entry_guser = entry4 = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry4), prefs.hex_irc_user_name);
	gtk_widget_show (entry4);
	gtk_table_attach (GTK_TABLE (table1), entry4, 1, 2, 3, 4,
							(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);

	/* entry_greal = entry5 = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry5), prefs.hex_irc_real_name);
	gtk_widget_show (entry5);
	gtk_table_attach (GTK_TABLE (table1), entry5, 1, 2, 4, 5,
							(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
							(GtkAttachOptions) (0), 0, 0); */

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (vbox1), vbox2, TRUE, TRUE, 0);

	label1 = bold_label (_("Networks"));
	gtk_box_pack_start (GTK_BOX (vbox2), label1, FALSE, FALSE, 0);

	table4 = gtk_table_new (2, 2, FALSE);
	gtk_widget_show (table4);
	gtk_box_pack_start (GTK_BOX (vbox2), table4, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (table4), 8);
	gtk_table_set_row_spacings (GTK_TABLE (table4), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table4), 3);

	scrolledwindow3 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow3);
	gtk_table_attach (GTK_TABLE (table4), scrolledwindow3, 0, 1, 0, 1,
							(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
							(GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow3),
											  GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow3),
													 GTK_SHADOW_IN);

	store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_INT);
	model = GTK_TREE_MODEL (store);

	networks_tree = treeview_networks = gtk_tree_view_new_with_model (model);
	g_object_unref (model);
	gtk_widget_show (treeview_networks);
	gtk_container_add (GTK_CONTAINER (scrolledwindow3), treeview_networks);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview_networks),
												  FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_signal_connect (G_OBJECT (renderer), "edited",
							G_CALLBACK (servlist_celledit_cb), model);
	gtk_tree_view_insert_column_with_attributes (
								GTK_TREE_VIEW (treeview_networks), -1,
						 		0, renderer,
						 		"text", 0,
								"editable", 1,
								"weight", 2,
								NULL);

	hbox = gtk_hbox_new (0, FALSE);
	gtk_table_attach (GTK_TABLE (table4), hbox, 0, 2, 1, 2,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);
	gtk_widget_show (hbox);

	checkbutton_skip =
		gtk_check_button_new_with_mnemonic (_("Skip network list on startup"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_skip),
											prefs.hex_gui_slist_skip);
	gtk_container_add (GTK_CONTAINER (hbox), checkbutton_skip);
	g_signal_connect (G_OBJECT (checkbutton_skip), "toggled",
							G_CALLBACK (no_servlist), 0);
	gtk_widget_show (checkbutton_skip);

	checkbutton_fav =
		gtk_check_button_new_with_mnemonic (_("Show favorites only"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_fav),
											prefs.hex_gui_slist_fav);
	gtk_container_add (GTK_CONTAINER (hbox), checkbutton_fav);
	g_signal_connect (G_OBJECT (checkbutton_fav), "toggled",
							G_CALLBACK (fav_servlist), 0);
	gtk_widget_show (checkbutton_fav);

	vbuttonbox2 = gtk_vbutton_box_new ();
	gtk_box_set_spacing (GTK_BOX (vbuttonbox2), 3);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (vbuttonbox2), GTK_BUTTONBOX_START);
	gtk_widget_show (vbuttonbox2);
	gtk_table_attach (GTK_TABLE (table4), vbuttonbox2, 1, 2, 0, 1,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (GTK_FILL), 0, 0);

	button_add = gtk_button_new_from_stock ("gtk-add");
	g_signal_connect (G_OBJECT (button_add), "clicked",
							G_CALLBACK (servlist_addnet_cb), networks_tree);
	gtk_widget_show (button_add);
	gtk_container_add (GTK_CONTAINER (vbuttonbox2), button_add);
	GTK_WIDGET_SET_FLAGS (button_add, GTK_CAN_DEFAULT);

	button_remove = gtk_button_new_from_stock ("gtk-remove");
	g_signal_connect (G_OBJECT (button_remove), "clicked",
							G_CALLBACK (servlist_deletenet_cb), 0);
	gtk_widget_show (button_remove);
	gtk_container_add (GTK_CONTAINER (vbuttonbox2), button_remove);
	GTK_WIDGET_SET_FLAGS (button_remove, GTK_CAN_DEFAULT);

	button_edit = gtk_button_new_with_mnemonic (_("_Edit..."));
	g_signal_connect (G_OBJECT (button_edit), "clicked",
							G_CALLBACK (servlist_edit_cb), 0);
	gtk_widget_show (button_edit);
	gtk_container_add (GTK_CONTAINER (vbuttonbox2), button_edit);
	GTK_WIDGET_SET_FLAGS (button_edit, GTK_CAN_DEFAULT);

	button_sort = gtk_button_new_with_mnemonic (_("_Sort"));
	add_tip (button_sort, _("Sorts the network list in alphabetical order. "
				"Use SHIFT-UP and SHIFT-DOWN keys to move a row."));
	g_signal_connect (G_OBJECT (button_sort), "clicked",
							G_CALLBACK (servlist_sort), 0);
	gtk_widget_show (button_sort);
	gtk_container_add (GTK_CONTAINER (vbuttonbox2), button_sort);
	GTK_WIDGET_SET_FLAGS (button_sort, GTK_CAN_DEFAULT);

	button_sort = gtk_button_new_with_mnemonic (_("_Favor"));
	add_tip (button_sort, _("Mark or unmark this network as a favorite."));
	g_signal_connect (G_OBJECT (button_sort), "clicked",
							G_CALLBACK (servlist_favor), 0);
	gtk_widget_show (button_sort);
	gtk_container_add (GTK_CONTAINER (vbuttonbox2), button_sort);
	GTK_WIDGET_SET_FLAGS (button_sort, GTK_CAN_DEFAULT);

	hseparator1 = gtk_hseparator_new ();
	gtk_widget_show (hseparator1);
	gtk_box_pack_start (GTK_BOX (vbox1), hseparator1, FALSE, TRUE, 4);

	hbuttonbox1 = gtk_hbutton_box_new ();
	gtk_widget_show (hbuttonbox1);
	gtk_box_pack_start (GTK_BOX (vbox1), hbuttonbox1, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbuttonbox1), 8);

	button_close = gtk_button_new_from_stock ("gtk-close");
	gtk_widget_show (button_close);
	g_signal_connect (G_OBJECT (button_close), "clicked",
							G_CALLBACK (servlist_close_cb), 0);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), button_close);
	GTK_WIDGET_SET_FLAGS (button_close, GTK_CAN_DEFAULT);

	button_connect = gtkutil_button (hbuttonbox1, GTK_STOCK_CONNECT, NULL,
												servlist_connect_cb, NULL, _("C_onnect"));
	GTK_WIDGET_SET_FLAGS (button_connect, GTK_CAN_DEFAULT);

	gtk_label_set_mnemonic_widget (GTK_LABEL (label3), entry1);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label6), entry4);
	/* gtk_label_set_mnemonic_widget (GTK_LABEL (label7), entry5); */

	gtk_widget_grab_focus (networks_tree);
	gtk_widget_grab_default (button_close);
	return servlist;
}

void
fe_serverlist_open (session *sess)
{
	if (serverlist_win)
	{
		gtk_window_present (GTK_WINDOW (serverlist_win));
		return;
	}

	servlist_sess = sess;

	serverlist_win = servlist_open_networks ();
	gtkutil_set_icon (serverlist_win);

	servlist_networks_populate (networks_tree, network_list);

	g_signal_connect (G_OBJECT (serverlist_win), "delete_event",
						 	G_CALLBACK (servlist_delete_cb), 0);
	g_signal_connect (G_OBJECT (serverlist_win), "configure_event",
							G_CALLBACK (servlist_configure_cb), 0);
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (networks_tree))),
							"changed", G_CALLBACK (servlist_network_row_cb), NULL);
	g_signal_connect (G_OBJECT (networks_tree), "key_press_event",
							G_CALLBACK (servlist_net_keypress_cb), networks_tree);

	gtk_widget_show (serverlist_win);
}
