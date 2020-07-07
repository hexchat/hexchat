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
#include <fcntl.h>
#include <time.h>

#include "fe-gtk.h"

#include "../common/hexchat.h"
#include "../common/notify.h"
#include "../common/cfgfiles.h"
#include "../common/fe.h"
#include "../common/server.h"
#include "../common/util.h"
#include "../common/userlist.h"
#include "../common/outbound.h"
#include "gtkutil.h"
#include "maingui.h"
#include "palette.h"
#include "notifygui.h"


/* model for the notify treeview */
enum
{
	USER_COLUMN,
	STATUS_COLUMN,
	SERVER_COLUMN,
	SEEN_COLUMN,
	COLOUR_COLUMN,
	NPS_COLUMN, 	/* struct notify_per_server * */
	N_COLUMNS
};


static GtkWidget *notify_window = 0;
static GtkWidget *notify_button_opendialog;
static GtkWidget *notify_button_remove;


static void
notify_closegui (void)
{
	notify_window = 0;
	notify_save ();
}

/* Need this to be able to set the foreground colour property of a row
 * from a GdkColor * in the model  -Vince
 */
static void
notify_treecell_property_mapper (GtkTreeViewColumn *col, GtkCellRenderer *cell,
                                 GtkTreeModel *model, GtkTreeIter *iter,
                                 gpointer data)
{
	gchar *text;
	GdkColor *colour;
	int model_column = GPOINTER_TO_INT (data);

	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, 
	                    COLOUR_COLUMN, &colour,
	                    model_column, &text, -1);
	g_object_set (G_OBJECT (cell), "text", text, NULL);
	g_object_set (G_OBJECT (cell), "foreground-gdk", colour, NULL);
	g_free (text);
}

static void
notify_row_cb (GtkTreeSelection *sel, GtkTreeView *view)
{
	GtkTreeIter iter;
	struct notify_per_server *servnot;

	if (gtkutil_treeview_get_selected (view, &iter, NPS_COLUMN, &servnot, -1))
	{
		gtk_widget_set_sensitive (notify_button_opendialog, servnot ? servnot->ison : 0);
		gtk_widget_set_sensitive (notify_button_remove, TRUE);
		return;
	}

	gtk_widget_set_sensitive (notify_button_opendialog, FALSE);
	gtk_widget_set_sensitive (notify_button_remove, FALSE);
}

static GtkWidget *
notify_treeview_new (GtkWidget *box)
{
	GtkListStore *store;
	GtkWidget *view;
	GtkTreeViewColumn *col;
	int col_id;

	store = gtk_list_store_new (N_COLUMNS,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_POINTER,	/* can't specify colour! */
										 G_TYPE_POINTER
	                           );
	g_return_val_if_fail (store != NULL, NULL);

	view = gtkutil_treeview_new (box, GTK_TREE_MODEL (store),
	                             notify_treecell_property_mapper,
	                             USER_COLUMN, _("Name"),
	                             STATUS_COLUMN, _("Status"),
	                             SERVER_COLUMN, _("Network"),
	                             SEEN_COLUMN, _("Last Seen"), -1);
	gtk_tree_view_column_set_expand (gtk_tree_view_get_column (GTK_TREE_VIEW (view), 0), TRUE);

	for (col_id=0; (col = gtk_tree_view_get_column (GTK_TREE_VIEW (view), col_id));
	     col_id++)
			gtk_tree_view_column_set_alignment (col, 0.5);

	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (view))),
							"changed", G_CALLBACK (notify_row_cb), view);

	gtk_widget_show (view);
	return view;
}

void
notify_gui_update (void)
{
	struct notify *notify;
	struct notify_per_server *servnot;
	GSList *list = notify_list;
	GSList *slist;
	gchar *name, *status, *server, *seen;
	int online, servcount, lastseenminutes;
	time_t lastseen;
	char agobuf[128];

	GtkListStore *store;
	GtkTreeView *view;
	GtkTreeIter iter;
	gboolean valid;	/* true if we don't need to append a new tree row */

	if (!notify_window)
		return;

	view = g_object_get_data (G_OBJECT (notify_window), "view");
	store = GTK_LIST_STORE (gtk_tree_view_get_model (view));
	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);

	while (list)
	{
		notify = (struct notify *) list->data;
		name = notify->name;
		status = _("Offline");
		server = "";

		online = FALSE;
		lastseen = 0;
		/* First see if they're online on any servers */
		slist = notify->server_list;
		while (slist)
		{
			servnot = (struct notify_per_server *) slist->data;
			if (servnot->ison)
				online = TRUE;
			if (servnot->lastseen > lastseen)
				lastseen = servnot->lastseen;
			slist = slist->next;
		}

		if (!online)				  /* Offline on all servers */
		{
			if (!lastseen)
				seen = _("Never");
			else
			{
				lastseenminutes = (int)(time (0) - lastseen) / 60;
				if (lastseenminutes < 60) 
					g_snprintf (agobuf, sizeof (agobuf), _("%d minutes ago"), lastseenminutes);
				else if (lastseenminutes < 120)
					g_snprintf (agobuf, sizeof (agobuf), _("An hour ago"));
				else
					g_snprintf (agobuf, sizeof (agobuf), _("%d hours ago"), lastseenminutes / 60);
				seen = agobuf;
			}
			if (!valid)	/* create new tree row if required */
				gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, name, 1, status,
			                    2, server, 3, seen, 4, &colors[4], 5, NULL, -1);
			if (valid)
				valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);

		} else
		{
			/* Online - add one line per server */
			servcount = 0;
			slist = notify->server_list;
			status = _("Online");
			while (slist)
			{
				servnot = (struct notify_per_server *) slist->data;
				if (servnot->ison)
				{
					if (servcount > 0)
						name = "";
					server = server_get_network (servnot->server, TRUE);

					g_snprintf (agobuf, sizeof (agobuf), _("%d minutes ago"), (int)(time (0) - lastseen) / 60);
					seen = agobuf;

					if (!valid)	/* create new tree row if required */
						gtk_list_store_append (store, &iter);
					gtk_list_store_set (store, &iter, 0, name, 1, status,
					                    2, server, 3, seen, 4, &colors[3], 5, servnot, -1);
					if (valid)
						valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);

					servcount++;
				}
				slist = slist->next;
			}
		}
		
		list = list->next;
	}

	while (valid)
	{
		GtkTreeIter old = iter;
		/* get next iter now because removing invalidates old one */
		valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (store),
                                      &iter);
		gtk_list_store_remove (store, &old);
	}

	notify_row_cb (gtk_tree_view_get_selection (view), view);
}

static void
notify_opendialog_clicked (GtkWidget * igad)
{
	GtkTreeView *view;
	GtkTreeIter iter;
	struct notify_per_server *servnot;

	view = g_object_get_data (G_OBJECT (notify_window), "view");
	if (gtkutil_treeview_get_selected (view, &iter, NPS_COLUMN, &servnot, -1))
	{
		if (servnot)
			open_query (servnot->server, servnot->notify->name, TRUE);
	}
}

static void
notify_remove_clicked (GtkWidget * igad)
{
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path = NULL;
	gboolean found = FALSE;
	char *name;

	view = g_object_get_data (G_OBJECT (notify_window), "view");
	if (gtkutil_treeview_get_selected (view, &iter, USER_COLUMN, &name, -1))
	{
		model = gtk_tree_view_get_model (view);
		found = (*name != 0);
		while (!found)	/* the real nick is some previous node */
		{
			g_free (name); /* it's useless to us */
			if (!path)
				path = gtk_tree_model_get_path (model, &iter);
			if (!gtk_tree_path_prev (path))	/* arrgh! no previous node! */
			{
				g_warning ("notify list state is invalid\n");
				break;
			}
			if (!gtk_tree_model_get_iter (model, &iter, path))
				break;
			gtk_tree_model_get (model, &iter, USER_COLUMN, &name, -1);
			found = (*name != 0);
		}
		if (path)
			gtk_tree_path_free (path);
		if (!found)
			return;

		/* ok, now we can remove it */
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
		notify_deluser (name);
		g_free (name);
	}
}

static void
notifygui_add_cb (GtkDialog *dialog, gint response, gpointer entry)
{
	char *networks;
	char *text;

	text = (char *)gtk_entry_get_text (GTK_ENTRY (entry));
	if (text[0] && response == GTK_RESPONSE_ACCEPT)
	{
		networks = (char*)gtk_entry_get_text (GTK_ENTRY (g_object_get_data (G_OBJECT (entry), "net")));
		if (g_ascii_strcasecmp (networks, "ALL") == 0 || networks[0] == 0)
			notify_adduser (text, NULL);
		else
			notify_adduser (text, networks);
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
notifygui_add_enter (GtkWidget *entry, GtkWidget *dialog)
{
	gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
}

void
fe_notify_ask (char *nick, char *networks)
{
	GtkWidget *dialog;
	GtkWidget *entry;
	GtkWidget *label;
	GtkWidget *wid;
	GtkWidget *table;
	char *msg = _("Enter nickname to add:");
	char buf[256];

	dialog = gtk_dialog_new_with_buttons (msg, NULL, 0,
										GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
										GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
										NULL);
	if (parent_window)
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent_window));
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);

	table = gtk_table_new (2, 3, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (table), 3);
	gtk_table_set_col_spacings (GTK_TABLE (table), 8);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), table);

	label = gtk_label_new (msg);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

	entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry), nick);
	g_signal_connect (G_OBJECT (entry), "activate",
						 	G_CALLBACK (notifygui_add_enter), dialog);
	gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 0, 1);

	g_signal_connect (G_OBJECT (dialog), "response",
						   G_CALLBACK (notifygui_add_cb), entry);

	label = gtk_label_new (_("Notify on these networks:"));
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);

	wid = gtk_entry_new ();
	g_object_set_data (G_OBJECT (entry), "net", wid);
	g_signal_connect (G_OBJECT (wid), "activate",
						 	G_CALLBACK (notifygui_add_enter), dialog);
	gtk_entry_set_text (GTK_ENTRY (wid), networks ? networks : "ALL");
	gtk_table_attach_defaults (GTK_TABLE (table), wid, 1, 2, 2, 3);

	label = gtk_label_new (NULL);
	g_snprintf (buf, sizeof (buf), "<i><span size=\"smaller\">%s</span></i>", _("Comma separated list of networks is accepted."));
	gtk_label_set_markup (GTK_LABEL (label), buf);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 3, 4);

	gtk_widget_show_all (dialog);
}

static void
notify_add_clicked (GtkWidget * igad)
{
	fe_notify_ask ("", NULL);
}

void
notify_opengui (void)
{
	GtkWidget *vbox, *bbox;
	GtkWidget *view;
	char buf[128];

	if (notify_window)
	{
		mg_bring_tofront (notify_window);
		return;
	}

	g_snprintf(buf, sizeof(buf), _("Friends List - %s"), _(DISPLAY_NAME));
	notify_window =
		mg_create_generic_tab ("Notify", buf, FALSE, TRUE, notify_closegui, NULL, 400,
								250, &vbox, 0);
	gtkutil_destroy_on_esc (notify_window);

	view = notify_treeview_new (vbox);
	g_object_set_data (G_OBJECT (notify_window), "view", view);
  
	bbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
	gtk_container_set_border_width (GTK_CONTAINER (bbox), 5);
	gtk_box_pack_end (GTK_BOX (vbox), bbox, 0, 0, 0);
	gtk_widget_show (bbox);

	gtkutil_button (bbox, GTK_STOCK_NEW, 0, notify_add_clicked, 0,
	                _("Add..."));

	notify_button_remove =
	gtkutil_button (bbox, GTK_STOCK_DELETE, 0, notify_remove_clicked, 0,
	                _("Remove"));

	notify_button_opendialog =
	gtkutil_button (bbox, NULL, 0, notify_opendialog_clicked, 0,
	                _("Open Dialog"));

	gtk_widget_set_sensitive (notify_button_opendialog, FALSE);
	gtk_widget_set_sensitive (notify_button_remove, FALSE);

	notify_gui_update ();

	gtk_widget_show (notify_window);
}
