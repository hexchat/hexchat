
#define GTK_DISABLE_DEPRECATED

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkhpaned.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktable.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtktree.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkwindow.h>

#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../pixmaps/xchat.h"
#include "../common/servlist.h"
#include "gtkutil.h"
#include "menu.h"
#include "pixmaps.h"



/* servlistgui.c globals */
static GtkWidget *serverlist_win = NULL;
static GtkWidget *servers_tree;	/* list of servers */
static GtkWidget *networks_tree;	/* network TreeView */
static GtkWidget *connect_button;
static GtkWidget *connectnew_button;

/* global user info */
static GtkWidget *entry_nick1;
static GtkWidget *entry_nick2;
static GtkWidget *entry_nick3;
static GtkWidget *entry_guser;
static GtkWidget *entry_greal;

/* edit area */
static GtkWidget *entry_nick;
static GtkWidget *entry_user;
static GtkWidget *entry_real;
static GtkWidget *entry_join;
static GtkWidget *entry_pass;
static GtkWidget *entry_cmd;
static GtkWidget *check_buttons[FLAG_COUNT];
static GtkWidget *editbox;	/* shown/hidden by "Edit Mode" checkbutton */

static ircnet *selected_net = NULL;
static session *servlist_sess;

static void servlist_network_row_cb (GtkTreeSelection *sel, gpointer user_data);


static void
servlist_toggles_populate (ircnet *net)
{
	int i;

	for (i = 0; i < FLAG_COUNT; i++)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_buttons[i]),
												net->flags & (1 << i));

}

static void
servlist_entries_populate (ircnet *net)
{
	gtk_entry_set_text (GTK_ENTRY (entry_nick), net->nick ? net->nick : "");
	gtk_entry_set_text (GTK_ENTRY (entry_user), net->user ? net->user : "");
	gtk_entry_set_text (GTK_ENTRY (entry_real), net->real ? net->real : "");
	gtk_entry_set_text (GTK_ENTRY (entry_pass), net->pass ? net->pass : "");
	gtk_entry_set_text (GTK_ENTRY (entry_join), net->autojoin ? net->autojoin : "");
	gtk_entry_set_text (GTK_ENTRY (entry_cmd), net->command ? net->command : "");

	if (net->flags & FLAG_USE_GLOBAL)
	{
		gtk_widget_set_sensitive (entry_nick, FALSE);
		gtk_widget_set_sensitive (entry_user, FALSE);
		gtk_widget_set_sensitive (entry_real, FALSE);
	} else
	{
		gtk_widget_set_sensitive (entry_nick, TRUE);
		gtk_widget_set_sensitive (entry_user, TRUE);
		gtk_widget_set_sensitive (entry_real, TRUE);
	}
}

static void
servlist_servers_populate (ircnet *net, GtkWidget *treeview, gboolean edit)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreePath *path;
	int i;
	ircserver *serv;
	char buf[256];
	GSList *list = net->servlist;
	GtkTreeSelection *sel;

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

	snprintf (buf, sizeof (buf), _("Settings for %s"), net->name);
	gtk_frame_set_label (GTK_FRAME (editbox), buf);

	store = (GtkListStore *)gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	gtk_list_store_clear (store);

	i = 0;
	while (list)
	{
		serv = list->data;
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, serv->hostname, 1, edit, -1);

		if (net->selected == i)
		{
			/* select this server */
			gtk_tree_selection_select_iter (sel, &iter);
			/* and make sure it's visible */
			path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
			gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (treeview), path,
													NULL, TRUE, 0.5, 0.5);
			gtk_tree_path_free (path);
		}
		i++;

		list = list->next;
	}
}

static void
servlist_networks_populate (GtkWidget *treeview, GSList *netlist, gboolean edit)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreeSelection *sel;
	GtkTreePath *path;
	int i;
	ircnet *net;

	if (!netlist)
	{
		net = servlist_net_add (_("New Network"), "");
		servlist_server_add (net, "newserver/6667");
		netlist = network_list;
	}
	store = (GtkListStore *)gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	gtk_list_store_clear (store);
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

	i = 0;
	while (netlist)
	{
		net = netlist->data;
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, net->name, 1, edit, -1);
		if (i == prefs.slist_select)
		{
			/* select this network */
			gtk_tree_selection_select_iter (sel, &iter);
			/* and make sure it's visible */
			path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
			gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (treeview), path,
													NULL, TRUE, 0.5, 0.5);
			gtk_tree_path_free (path);
		}
		i++;
		netlist = netlist->next;
	}
}

static void
servlist_populate (ircnet *net)
{
	servlist_servers_populate (net, servers_tree, TRUE);
	servlist_entries_populate (net);
	servlist_toggles_populate (net);
	gtk_widget_set_sensitive (connect_button, TRUE);
	if (connectnew_button)
		gtk_widget_set_sensitive (connectnew_button, TRUE);
	gtk_widget_set_sensitive (editbox, TRUE);
}

static void
servlist_menu_destroy (GtkMenuShell *menushell, GtkWidget *menu)
{
	gtk_widget_destroy (menu);
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
}

static void
servlist_addnet_cb (GtkWidget *item, GtkTreeView *treeview)
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkListStore *store;
	GtkTreePath *path;
	ircnet *net;

	store = (GtkListStore *)gtk_tree_view_get_model (treeview);
	net = servlist_net_add (_("New Network"), "");
	servlist_server_add (net, "newserver/6667");

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, net->name, 1, 1, -1);

	/* select this server */
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (networks_tree));
	gtk_tree_selection_select_iter (sel, &iter);
	/* and make sure it's visible */
	path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (treeview), path,
											NULL, TRUE, 0.5, 0.5);
	gtk_tree_path_free (path);

	servlist_network_row_cb (gtk_tree_view_get_selection (GTK_TREE_VIEW (networks_tree)), NULL);
}

static void
servlist_deletenetdialog_cb (GtkDialog *dialog, gint arg1, ircnet *net)
{
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkListStore *store;

	gtk_widget_destroy (GTK_WIDGET (dialog));
	if (arg1 == GTK_RESPONSE_OK)
	{
		/* remove from GUI */
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (networks_tree));
		if (gtk_tree_selection_get_selected (sel, &model, &iter))
			gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

		/* don't allow user to play with freed memory */
		gtk_widget_set_sensitive (connect_button, FALSE);
		if (connectnew_button)
			gtk_widget_set_sensitive (connectnew_button, FALSE);
		gtk_widget_set_sensitive (editbox, FALSE);
		store = (GtkListStore *)gtk_tree_view_get_model (GTK_TREE_VIEW (servers_tree));
		gtk_list_store_clear (store);

		/* remove from list */
		servlist_net_remove (net);

		if (selected_net == net)
			selected_net = NULL;

		if (!network_list)
		{
			servlist_networks_populate (networks_tree, network_list, prefs.slist_edit);
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
			prefs.slist_select += delta;
			network_list = g_slist_remove (network_list, net);
			network_list = g_slist_insert (network_list, net, pos);
			servlist_networks_populate (networks_tree, network_list, FALSE);
		}
	}
}

static void
servlist_movedownnet_cb (GtkWidget *item, ircnet *net)
{
	servlist_move_network (net, +1);
}

static void
servlist_moveupnet_cb (GtkWidget *item, ircnet *net)
{
	servlist_move_network (net, -1);
}

static void
servlist_deletenet_cb (GtkWidget *item, ircnet *net)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (GTK_WINDOW (serverlist_win),
												GTK_DIALOG_DESTROY_WITH_PARENT,
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
servlist_deleteserver_cb (GtkWidget *item, ircserver *serv)
{
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;

	/* don't remove the last server */
	if (selected_net && g_slist_length (selected_net->servlist) < 2)
		return;

	/* remove from GUI */
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (servers_tree));
	if (gtk_tree_selection_get_selected (sel, &model, &iter))
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

	/* remove from list */
	if (selected_net)
		servlist_server_remove (selected_net, serv);
}

static void
servlist_server_popmenu (ircserver *serv, GtkTreeView *treeview, GdkEventButton *event)
{
	GtkWidget *item, *menu;
	char buf[256];

	menu = gtk_menu_new ();

	snprintf (buf, sizeof (buf), _("_Remove \"%s\""), serv->hostname);
	item = create_icon_menu (buf, GTK_STOCK_DELETE, TRUE);
	g_signal_connect (G_OBJECT (item), "activate",
							G_CALLBACK (servlist_deleteserver_cb), serv);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	item = create_icon_menu (_("_Add new server"), GTK_STOCK_NEW, TRUE);
	g_signal_connect (G_OBJECT (item), "activate",
							G_CALLBACK (servlist_addserver_cb), treeview);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	g_signal_connect (G_OBJECT (menu), "selection-done",
							G_CALLBACK (servlist_menu_destroy), menu);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, event->time);
}

static void
servlist_network_popmenu (ircnet *net, GtkTreeView *treeview, GdkEventButton *event)
{
	GtkWidget *item, *menu;
	char buf[256];

	menu = gtk_menu_new ();

	snprintf (buf, sizeof (buf), _("_Remove \"%s\""), net->name);
	item = create_icon_menu (buf, GTK_STOCK_DELETE, TRUE);
	g_signal_connect (G_OBJECT (item), "activate",
							G_CALLBACK (servlist_deletenet_cb), net);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	snprintf (buf, sizeof (buf), _("Move \"%s\" _down"), net->name);
	item = create_icon_menu (buf, GTK_STOCK_GO_DOWN, TRUE);
	g_signal_connect (G_OBJECT (item), "activate",
							G_CALLBACK (servlist_movedownnet_cb), net);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	snprintf (buf, sizeof (buf), _("Move \"%s\" _up"), net->name);
	item = create_icon_menu (buf, GTK_STOCK_GO_UP, TRUE);
	g_signal_connect (G_OBJECT (item), "activate",
							G_CALLBACK (servlist_moveupnet_cb), net);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	item = gtk_menu_item_new ();
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	item = create_icon_menu (_("_Add new network"), GTK_STOCK_NEW, TRUE);
	g_signal_connect (G_OBJECT (item), "activate",
							G_CALLBACK (servlist_addnet_cb), treeview);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	g_signal_connect (G_OBJECT (menu), "selection-done",
							G_CALLBACK (servlist_menu_destroy), menu);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, event->time);
}

static ircnet *
servlist_find_selected_net (GtkTreeSelection *sel, int *pos)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *netname;
	ircnet *net = NULL;

	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, 0, &netname, -1);
		net = servlist_net_find (netname, pos);
		if (net)
			prefs.slist_select = *pos;
	}

	return net;
}

static void
servlist_network_row_cb (GtkTreeSelection *sel, gpointer user_data)
{
	ircnet *net;
	int pos;

	selected_net = NULL;

	net = servlist_find_selected_net (sel, &pos);
	if (net)
	{
		selected_net = net;
		servlist_populate (net);
	}
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
		if (serv)
			selected_net->selected = pos;
	}
}

static void
servlist_savegui (void)
{
	strcpy (prefs.nick1, GTK_ENTRY (entry_nick1)->text);
	strcpy (prefs.nick2, GTK_ENTRY (entry_nick2)->text);
	strcpy (prefs.nick3, GTK_ENTRY (entry_nick3)->text);
	strcpy (prefs.username, GTK_ENTRY (entry_guser)->text);
	strcpy (prefs.realname, GTK_ENTRY (entry_greal)->text);
	servlist_save ();
}

static void
servlist_connectnew_cb (GtkWidget *button, gpointer userdata)
{
	/* give it a NULL sess and it'll open a new tab for us */
	servlist_connect (NULL, selected_net);

	servlist_savegui ();		/* why doesn't the delete_event trigger this? */
	gtk_widget_destroy (serverlist_win);
	serverlist_win = NULL;
}

static void
servlist_connect_cb (GtkWidget *button, gpointer userdata)
{
	if (!is_session (servlist_sess))
		servlist_sess = NULL;

	servlist_connect (servlist_sess, selected_net);

	servlist_savegui ();		/* why doesn't the delete_event trigger this? */
	gtk_widget_destroy (serverlist_win);
	serverlist_win = NULL;
}

static gboolean
servlist_net_press_cb (GtkWidget *widget, GdkEventButton *event,
							  gpointer user_data)
{
	GtkTreeSelection *sel;
	GtkTreePath *path;
	int pos;
	ircnet *net;

	if (event->type == GDK_2BUTTON_PRESS)
	{
		if (selected_net != NULL)
			servlist_connect_cb (widget, user_data);
	}

	if (event->button == 3)
	{
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (networks_tree));
		if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (networks_tree),
			 event->x, event->y, &path, 0, 0, 0))
		{
			gtk_tree_selection_unselect_all (sel);
			gtk_tree_selection_select_path (sel, path);
			gtk_tree_path_free (path);
			net = servlist_find_selected_net (sel, &pos);
			if (net)
				servlist_network_popmenu (net, GTK_TREE_VIEW (networks_tree), event);
		} else
		{
			gtk_tree_selection_unselect_all (sel);
		}

		return TRUE;
	}

	return FALSE;
}

static gboolean
servlist_serv_press_cb (GtkWidget *widget, GdkEventButton *event,
							   gpointer user_data)
{
	GtkTreeSelection *sel;
	GtkTreePath *path;
	GtkTreeIter iter;
	int pos;
	char *servname;
	ircserver *serv;
	GtkTreeModel *model;

	if (event->type == GDK_2BUTTON_PRESS)
	{
		if (selected_net != NULL)
			servlist_connect_cb (widget, user_data);
	}

	if (event->button == 3)
	{
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (servers_tree));
		if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (servers_tree),
			 event->x, event->y, &path, 0, 0, 0))
		{
			gtk_tree_selection_unselect_all (sel);
			gtk_tree_selection_select_path (sel, path);
			gtk_tree_path_free (path);

			if (gtk_tree_selection_get_selected (sel, &model, &iter))
			{
				gtk_tree_model_get (model, &iter, 0, &servname, -1);
				serv = servlist_server_find (selected_net, servname, &pos);
				if (serv)
					servlist_server_popmenu (serv, GTK_TREE_VIEW (servers_tree), event);
			}

		} else
		{
			gtk_tree_selection_unselect_all (sel);
		}

		return TRUE;
	}

	return FALSE;
}

static void
servlist_celledit_cb (GtkCellRendererText *cell, gchar *arg1, gchar *arg2,
							 gpointer user_data)
{
	GtkTreeModel *model = (GtkTreeModel *)user_data;
	GtkTreeIter iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (arg1);
	char *netname;
	ircnet *net;

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &netname, -1);

	net = servlist_net_find (netname, NULL);
	if (net)
	{
		netname = net->name;
		net->name = strdup (arg2);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, net->name, -1);
		free (netname);
	}

	gtk_tree_path_free (path);
}

static GtkWidget *
gtkutil_create_list (GtkWidget *box, char *title, void *select_callback,
							void *edit_callback, void *click_callback)
{
	GtkTreeModel *model;
	GtkWidget *treeview;
	GtkWidget *hbox, *sw;
	GtkCellRenderer *renderer;
	GtkListStore *store;

	hbox = gtk_hbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (box), hbox);

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
													 GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
											  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (hbox), sw, TRUE, TRUE, 0);

	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);

	model = GTK_TREE_MODEL (store);

	treeview = gtk_tree_view_new_with_model (model);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1,
						 		title, renderer,
						 		"text", 0,
								"editable", 1,
								NULL);
	g_signal_connect (G_OBJECT (renderer), "edited",
							G_CALLBACK (edit_callback), model);
	g_signal_connect (G_OBJECT (treeview), "button_press_event",
							G_CALLBACK (click_callback), model);
   gtk_container_add (GTK_CONTAINER (sw), treeview);
	g_object_unref (G_OBJECT (model));

	return treeview;
}

static void
servlist_check_cb (GtkWidget *but, gpointer num_p)
{
	int num = GPOINTER_TO_INT (num_p);

	if (!selected_net)
		return;

	if (GTK_TOGGLE_BUTTON (but)->active)
		selected_net->flags |= (1 << num);
	else
		selected_net->flags &= ~(1 << num);

	if ((1 << num) == FLAG_USE_GLOBAL)
		servlist_entries_populate (selected_net);
}

static GtkWidget *
servlist_create_check (int num, GtkWidget *table, int row, int col, char *labeltext)
{
	GtkWidget *but;

	but = gtk_check_button_new_with_label (labeltext);
	g_signal_connect (G_OBJECT (but), "toggled",
							G_CALLBACK (servlist_check_cb), GINT_TO_POINTER (num));
	gtk_table_attach (GTK_TABLE (table), but, col, col+1, row, row+1,
						   GTK_FILL|GTK_EXPAND, 0, 0, 0);

	return but;
}

static void
servlist_entry_cb (GtkWidget *entry, gpointer userdata)
{
	int offset = GPOINTER_TO_INT (userdata);
	char **str = (char **) ((char *)selected_net + offset);

	if (!selected_net)
		return;

	free (*str);
	*str = strdup (GTK_ENTRY (entry)->text);
}

static GtkWidget *
servlist_create_entry (GtkWidget *table, char *labeltext, int row,
							  int struct_offset)
{
	GtkWidget *label, *entry;

	label = gtk_label_new (labeltext);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row+1,
							GTK_FILL|GTK_SHRINK, GTK_FILL, 4, 1);
	gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);

	entry = gtk_entry_new ();
	gtk_table_attach (GTK_TABLE (table), entry, 1, 2, row, row+1,
							GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 1);

	g_signal_connect (G_OBJECT (entry), "changed",
							G_CALLBACK (servlist_entry_cb),
							GINT_TO_POINTER (struct_offset));

	return entry;
}

static void
servlist_create_infobox (GtkWidget *box)
{
	GtkWidget *frame, *table, *label, *entry, *img, *hbox;

	frame = gtk_frame_new (_("Global User Info"));

	table = gtk_table_new (2, 5, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 5);

	img = gtk_image_new_from_pixbuf (pix_xchat);
	gtk_table_attach (GTK_TABLE (table), img, 4, 5, 0, 2, 0, 0, 0, 0);

	label = gtk_label_new (_("Nick Names:"));
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
							GTK_FILL, 0, 1, 0);

	entry_nick1 = entry = gtk_entry_new ();
	gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1,
							GTK_FILL|GTK_EXPAND, 0, 1, 0);
	gtk_entry_set_text (GTK_ENTRY (entry), prefs.nick1);

	entry_nick2 = entry = gtk_entry_new ();
	gtk_table_attach (GTK_TABLE (table), entry, 2, 3, 0, 1,
							GTK_FILL|GTK_EXPAND, 0, 1, 0);
	gtk_entry_set_text (GTK_ENTRY (entry), prefs.nick2);

	entry_nick3 = entry = gtk_entry_new ();
	gtk_table_attach (GTK_TABLE (table), entry, 3, 4, 0, 1,
							GTK_FILL|GTK_EXPAND, 0, 1, 0);
	gtk_entry_set_text (GTK_ENTRY (entry), prefs.nick3);

	label = gtk_label_new (_("User Name:"));
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
							GTK_FILL, 0, 1, 0);

	entry_guser = entry = gtk_entry_new ();
	gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2,
							GTK_FILL|GTK_EXPAND, 0, 1, 0);
	gtk_entry_set_text (GTK_ENTRY (entry), prefs.username);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_table_attach (GTK_TABLE (table), hbox, 2, 4, 1, 2,
							GTK_FILL|GTK_EXPAND, 0, 1, 0);

	label = gtk_label_new (_("Real Name:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, 0, 0, 10);

	entry_greal = entry = gtk_entry_new ();
	gtk_container_add (GTK_CONTAINER (hbox), entry);
	gtk_entry_set_text (GTK_ENTRY (entry), prefs.realname);

	gtk_container_add (GTK_CONTAINER (frame), table);
	gtk_box_pack_start (GTK_BOX (box), frame, 0, 0, 0);
}

static gint
servlist_delete_cb (GtkWidget *win, GdkEventAny *event, gpointer userdata)
{
	servlist_savegui ();
	serverlist_win = NULL;

	if (sess_list == NULL)
		xchat_exit ();

	return FALSE;
}

static void
servlist_close_cb (GtkWidget *button, gpointer userdata)
{
	servlist_savegui ();
	gtk_widget_destroy (serverlist_win);
	serverlist_win = NULL;

	if (sess_list == NULL)
		xchat_exit ();
}

static void
servlist_create_buttons (GtkWidget *box)
{
	GtkWidget *hbox, *but;

	hbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_SPREAD);

	connect_button = but = gtk_button_new_with_mnemonic (_("C_onnect"));
	g_signal_connect (G_OBJECT (but), "clicked",
							G_CALLBACK (servlist_connect_cb), 0);
	gtk_widget_set_sensitive (but, FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), but, 0, 0, 0);

	if (servlist_sess)
	{
		connectnew_button = but = gtk_button_new_with_mnemonic (_("Connect in a _new tab"));
		g_signal_connect (G_OBJECT (but), "clicked",
								G_CALLBACK (servlist_connectnew_cb), 0);
		gtk_widget_set_sensitive (but, FALSE);
		gtk_box_pack_start (GTK_BOX (hbox), but, 0, 0, 0);
	}

	but = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	g_signal_connect (G_OBJECT (but), "clicked",
							G_CALLBACK (servlist_close_cb), 0);
	gtk_box_pack_start (GTK_BOX (hbox), but, 0, 0, 0);

	gtk_box_pack_end (GTK_BOX (box), hbox, 0, 0, 0);
}

static void
servlist_editserver_cb (GtkCellRendererText *cell, gchar *arg1, gchar *arg2,
								gpointer user_data)
{
	GtkTreeModel *model = (GtkTreeModel *)user_data;
	GtkTreeIter iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (arg1);
	char *servname;
	ircserver *serv;

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &servname, -1);

	if (!selected_net)
		return;

	serv = servlist_server_find (selected_net, servname, NULL);
	if (serv)
	{
		servname = serv->hostname;
		serv->hostname = strdup (arg2);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, serv->hostname, -1);
		free (servname);
	}

	gtk_tree_path_free (path);
}

static GtkWidget *
servlist_create_servlistbox (GtkWidget *box)
{
	GtkWidget *vbox, *tree;

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (box), vbox);

	tree = gtkutil_create_list (vbox, _("Servers"), servlist_server_row_cb,
										 servlist_editserver_cb, servlist_serv_press_cb);
#if 0
	hbox = gtk_hbox_new (FALSE, 2);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, 0, 0, 0);

	but = gtk_button_new_with_label ("Add Server");
	gtk_container_add (GTK_CONTAINER (hbox), but);

	but = gtk_button_new_with_label ("Delete Server");
	gtk_container_add (GTK_CONTAINER (hbox), but);
#endif
	return tree;
}

static void
servlist_create_editbox (GtkWidget *box)
{
	GtkWidget *table, *vbox, *hbox, *frame;

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 3);

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);

	table = gtk_table_new (10, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 5);

	entry_nick = servlist_create_entry (table, _("Nick Name:"), 1,
											G_STRUCT_OFFSET (ircnet, nick));
	entry_user = servlist_create_entry (table, _("User Name:"), 2,
											G_STRUCT_OFFSET (ircnet, user));
	entry_real = servlist_create_entry (table, _("Real Name:"), 3,																	G_STRUCT_OFFSET (ircnet, real));
	entry_pass = servlist_create_entry (table, _("Server Password:"), 4,
											G_STRUCT_OFFSET (ircnet, pass));
	entry_join = servlist_create_entry (table, _("Join Channels:"), 5,
											G_STRUCT_OFFSET (ircnet, autojoin));
	add_tip (entry_join, _("Channels to join, separated by commas, but not spaces!"));
	entry_cmd = servlist_create_entry (table, _("Connect Command:"), 6,
											G_STRUCT_OFFSET (ircnet, command));
	gtk_container_add (GTK_CONTAINER (hbox), table);

	table = gtk_table_new (2, 3, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 5);
	check_buttons[0] = servlist_create_check (0, table, 7, 0, _("Cycle until connected"));
	check_buttons[1] = servlist_create_check (1, table, 7, 1, _("Use global user info"));
	check_buttons[2] = servlist_create_check (2, table, 7, 2, _("Use secure SSL"));
	check_buttons[3] = servlist_create_check (3, table, 8, 0, _("Auto connect at startup"));
	check_buttons[4] = servlist_create_check (4, table, 8, 1, _("Use a proxy server"));
	check_buttons[5] = servlist_create_check (5, table, 8, 2, _("Accept invalid cert."));
#ifndef USE_OPENSSL
	gtk_widget_set_sensitive (check_buttons[2], FALSE);
	gtk_widget_set_sensitive (check_buttons[5], FALSE);
#endif
	gtk_box_pack_end (GTK_BOX (vbox), table, 0, 0, 0);

	servers_tree = servlist_create_servlistbox (hbox);

	editbox = frame = gtk_frame_new (_("Settings for Selected Network"));
	gtk_widget_set_sensitive (frame, FALSE);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_paned_add2 (GTK_PANED (box), frame);
}

static void
servlist_editmode_cb (GtkToggleButton *but, gpointer userdata)
{
	if (but->active)
	{
		prefs.slist_edit = 1;
	  	servlist_networks_populate (networks_tree, network_list, TRUE);
		gtk_widget_show (editbox);
		return;
	}

	prefs.slist_edit = 0;
	servlist_networks_populate (networks_tree, network_list, FALSE);
	gtk_widget_hide (editbox);
}

static GtkWidget *
servlist_create_list (GtkWidget *box)
{
	GtkWidget *vbox, *hbox, *pane, *tree, *but;

	pane = gtk_hpaned_new ();
	gtk_container_add (GTK_CONTAINER (box), pane);
	gtk_paned_set_position (GTK_PANED (pane), 120);

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_paned_pack1 (GTK_PANED (pane), hbox, TRUE, TRUE);

	vbox = gtk_vbox_new (FALSE, 3);
	gtk_container_add (GTK_CONTAINER (hbox), vbox);

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, 0, 0, 0);

	but = gtk_check_button_new_with_label (_("Edit mode"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (but),
											prefs.slist_edit);
	g_signal_connect (G_OBJECT (but), "toggled",
							G_CALLBACK (servlist_editmode_cb), 0);
	gtk_box_pack_start (GTK_BOX (hbox), but, 0, 0, 0);

	tree = gtkutil_create_list (vbox, _("Networks"), servlist_network_row_cb,
										 servlist_celledit_cb, servlist_net_press_cb);
	servlist_networks_populate (tree, network_list, prefs.slist_edit);

	servlist_create_editbox (pane);

	return tree;
}

void
fe_serverlist_open (session *sess)
{
	GtkWidget *win, *vbox;

	if (serverlist_win)
	{
		gtk_window_present (GTK_WINDOW (serverlist_win));
		return;
	}

	servlist_sess = sess;
	connectnew_button = NULL;

	serverlist_win = win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (win), _("X-Chat: Server List"));
	gtk_window_set_position (GTK_WINDOW (win), GTK_WIN_POS_MOUSE);
	gtkutil_set_icon (win);
	gtk_container_set_border_width (GTK_CONTAINER (win), 5);

	g_signal_connect (G_OBJECT (win), "delete_event",
						 	G_CALLBACK (servlist_delete_cb), 0);

	vbox = gtk_vbox_new (FALSE, 5);
	gtk_container_add (GTK_CONTAINER (win), vbox);

	servlist_create_infobox (vbox);
	networks_tree = servlist_create_list (vbox);
	servlist_create_buttons (vbox);

	gtk_widget_show_all (win);
	if (!prefs.slist_edit)
		gtk_widget_hide (editbox);

	/* force selection */
	servlist_network_row_cb (gtk_tree_view_get_selection (GTK_TREE_VIEW (networks_tree)), NULL);

	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (networks_tree))),
							"changed", G_CALLBACK (servlist_network_row_cb), NULL);

	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (servers_tree))),
							"changed", G_CALLBACK (servlist_server_row_cb), NULL);
}
