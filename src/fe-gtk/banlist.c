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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "fe-gtk.h"

#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeselection.h>

#include "../common/xchat.h"
#include "../common/modes.h"
#include "../common/outbound.h"
#include "../common/xchatc.h"
#include "gtkutil.h"
#include "maingui.h"
#include "banlist.h"

/* model for the banlist tree */
enum
{
	MASK_COLUMN,
	FROM_COLUMN,
	DATE_COLUMN,
	N_COLUMNS
};

static GtkTreeView *
get_view (struct session *sess)
{
	return GTK_TREE_VIEW (sess->res->banlist_treeview);
}

static GtkListStore *
get_store (struct session *sess)
{
	return GTK_LIST_STORE (gtk_tree_view_get_model (get_view (sess)));
}

void
fe_add_ban_list (struct session *sess, char *mask, char *who, char *when)
{
	GtkListStore *store;
	GtkTreeIter iter;

	store = get_store (sess);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, mask, 1, who, 2, when, -1);

}

void
fe_ban_list_end (struct session *sess)
{
	gtk_widget_set_sensitive (sess->res->banlist_butRefresh, TRUE);
}

/**
 *  * Performs the actual refresh operations.
 *  */
static void
banlist_do_refresh (struct session *sess)
{
	char tbuf[256];
	if (sess->server->connected)
	{
		GtkListStore *store;

		gtk_widget_set_sensitive (sess->res->banlist_butRefresh, FALSE);

		snprintf (tbuf, sizeof tbuf, "X-Chat: Ban List (%s, %s)",
						sess->channel, sess->server->servername);
		mg_set_title (sess->res->banlist_window, tbuf);

		store = get_store (sess);
		gtk_list_store_clear (store);

		handle_command (sess, "ban", FALSE);
	} else
		gtkutil_simpledialog ("Not connected.");
}

static void
banlist_refresh (GtkWidget * wid, struct session *sess)
{
	/* JG NOTE: Didn't see actual use of wid here, so just forwarding
	   *          * this to chanlist_do_refresh because I use it without any widget
	   *          * param in chanlist_build_gui_list when the user presses enter
	   *          * or apply for the first time if the list has not yet been
	   *          * received.
	   *          */
	banlist_do_refresh (sess);
}

static void
banlist_unban (GtkWidget * wid, struct session *sess)
{
	GtkTreeModel *model;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	char tbuf[2048];
	char **masks;
	int num_sel, i;

	/* grab the list of selected items */
	model = GTK_TREE_MODEL (get_store (sess));
	sel = gtk_tree_view_get_selection (get_view (sess));
	num_sel = 0;
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			if (gtk_tree_selection_iter_is_selected (sel, &iter))
				num_sel++;
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}

	if (num_sel < 1)
	{
		gtkutil_simpledialog (_("You must select some bans."));
		return;
	}

	/* create an array of all the masks */
	masks = malloc (num_sel * sizeof (char *));
	
	i = 0;
	gtk_tree_model_get_iter_first (model, &iter);
	do
	{
		if (gtk_tree_selection_iter_is_selected (sel, &iter))
		{
			gtk_tree_model_get (model, &iter, MASK_COLUMN, &masks[i], -1);
			i++;
		}
	}
	while (gtk_tree_model_iter_next (model, &iter));

	/* and send to server */
	send_channel_modes (sess, tbuf, masks, 0, i, '-', 'b');
	
	/* now free everything, and refresh banlist */	
	for (i=0; i < num_sel; i++)
		g_free (masks[i]);
	free (masks);

	banlist_do_refresh (sess);
}

static void
banlist_clear (GtkWidget * wid, struct session *sess)
{
	GtkTreeSelection *sel;

	sel = gtk_tree_view_get_selection (get_view (sess));
	gtk_tree_selection_select_all (sel);
	banlist_unban (wid, sess);
}

static void
banlist_add_selected_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	GSList **lp = data;
	GSList *list = NULL;
	GtkTreeIter *copy;

	if (!lp) return;
	list = *lp;
	copy = g_malloc (sizeof (GtkTreeIter));
	g_return_if_fail (copy != NULL);
	*copy = *iter;

	list = g_slist_append (list, copy);
	*(GSList **)data = list;
}


static void
banlist_crop (GtkWidget * wid, struct session *sess)
{
	GtkTreeSelection *select;
	GSList *list = NULL, *node;
	int num_sel;

	/* remember which bans are selected */
	select = gtk_tree_view_get_selection (get_view (sess));
	/* gtk_tree_selected_get_selected_rows() isn't present in gtk 2.0.x */
	gtk_tree_selection_selected_foreach (select, banlist_add_selected_cb,
	                                     &list);
	
	num_sel = g_slist_length (list);
	/* select all, then unselect those that we remembered */
	if (num_sel)
	{
		gtk_tree_selection_select_all (select);
		
		for (node = list; node; node = node->next)
			gtk_tree_selection_unselect_iter (select, node->data);
		
		g_slist_foreach (list, (GFunc)g_free, NULL);
		g_slist_free (list);

		banlist_unban (wid, sess);
	} else
		gtkutil_simpledialog (_("You must select some bans."));

}

static GtkWidget *
banlist_treeview_new (GtkWidget *box)
{
	GtkListStore *store;
	GtkWidget *view;
	GtkTreeSelection *select;
	GtkTreeViewColumn *col;

	store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING,
	                            G_TYPE_STRING);
	g_return_val_if_fail (store != NULL, NULL);
	view = gtkutil_treeview_new (box, GTK_TREE_MODEL (store), NULL,
	                             MASK_COLUMN, _("Mask"),
	                             FROM_COLUMN, _("From"),
	                             DATE_COLUMN, _("Date"), -1);

	col = gtk_tree_view_get_column (GTK_TREE_VIEW (view), MASK_COLUMN);
	gtk_tree_view_column_set_alignment (col, 0.5);
	gtk_tree_view_column_set_min_width (col, 300);
	gtk_tree_view_column_set_sort_column_id (col, MASK_COLUMN);

	col = gtk_tree_view_get_column (GTK_TREE_VIEW (view), FROM_COLUMN);
	gtk_tree_view_column_set_alignment (col, 0.5);
	gtk_tree_view_column_set_sort_column_id (col, FROM_COLUMN);

	col = gtk_tree_view_get_column (GTK_TREE_VIEW (view), DATE_COLUMN);
	gtk_tree_view_column_set_alignment (col, 0.5);

	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_MULTIPLE);

	gtk_widget_show (view);
	return view;
}


static void
banlist_closegui (GtkWidget *wid, session *sess)
{
	if (is_session (sess))
		sess->res->banlist_window = 0;
}

void
banlist_opengui (struct session *sess)
{
	GtkWidget *vbox1;
	GtkWidget *bbox;
	char tbuf[256];

	if (sess->res->banlist_window)
	{
		mg_bring_tofront (sess->res->banlist_window);
		return;
	}

	snprintf (tbuf, sizeof tbuf, _("X-Chat: Ban List (%s)"),
					sess->server->servername);

	sess->res->banlist_window = mg_create_generic_tab ("banlist", tbuf, FALSE,
					TRUE, banlist_closegui, sess, 550, 200, &vbox1, sess->server);

	/* create banlist view */
	sess->res->banlist_treeview = banlist_treeview_new (vbox1);

	bbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
	gtk_container_set_border_width (GTK_CONTAINER (bbox), 5);
	gtk_box_pack_end (GTK_BOX (vbox1), bbox, 0, 0, 0);
	gtk_widget_show (bbox);

	gtkutil_button (bbox, GTK_STOCK_REMOVE, 0, banlist_unban, sess,
	                _("Unban"));
	gtkutil_button (bbox, GTK_STOCK_REMOVE, 0, banlist_crop, sess,
	                _("Crop"));
	gtkutil_button (bbox, GTK_STOCK_CLEAR, 0, banlist_clear, sess,
	                _("Clear"));

	sess->res->banlist_butRefresh = gtkutil_button (bbox, GTK_STOCK_REFRESH, 0, banlist_refresh, sess, _("Refresh"));

	banlist_do_refresh (sess);

	gtk_widget_show (sess->res->banlist_window);
}
