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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "fe-gtk.h"

#include "../common/hexchat.h"
#include "../common/ignore.h"
#include "../common/cfgfiles.h"
#include "../common/fe.h"
#include "gtkutil.h"
#include "maingui.h"

enum
{
	MASK_COLUMN,
	CHAN_COLUMN,
	PRIV_COLUMN,
	NOTICE_COLUMN,
	CTCP_COLUMN,
	DCC_COLUMN,
	INVITE_COLUMN,
	UNIGNORE_COLUMN,
	N_COLUMNS
};

static GtkWidget *ignorewin = 0;

static GtkWidget *num_ctcp;
static GtkWidget *num_priv;
static GtkWidget *num_chan;
static GtkWidget *num_noti;
static GtkWidget *num_invi;

static GtkTreeModel *
get_store (void)
{
	return gtk_tree_view_get_model (g_object_get_data (G_OBJECT (ignorewin), "view"));
}

static int
ignore_get_flags (GtkTreeModel *model, GtkTreeIter *iter)
{
	gboolean chan, priv, noti, ctcp, dcc, invi, unig;
	int flags = 0;

	gtk_tree_model_get (model, iter, 1, &chan, 2, &priv, 3, &noti,
	                    4, &ctcp, 5, &dcc, 6, &invi, 7, &unig, -1);
	if (chan)
		flags |= IG_CHAN;
	if (priv)
		flags |= IG_PRIV;
	if (noti)
		flags |= IG_NOTI;
	if (ctcp)
		flags |= IG_CTCP;
	if (dcc)
		flags |= IG_DCC;
	if (invi)
		flags |= IG_INVI;
	if (unig)
		flags |= IG_UNIG;
	return flags;
}

static void
mask_edited (GtkCellRendererText *render, gchar *path, gchar *new, gpointer dat)
{
	GtkListStore *store = GTK_LIST_STORE (get_store ());
	GtkTreeIter iter;
	char *old;
	int flags;

	gtkutil_treemodel_string_to_iter (GTK_TREE_MODEL (store), path, &iter);
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, 0, &old, -1);
	
	if (!strcmp (old, new))	/* no change */
		;
	else if (ignore_exists (new))	/* duplicate, ignore */
		fe_message (_("That mask already exists."), FE_MSG_ERROR);
	else
	{
		/* delete old mask, and add new one with original flags */
		ignore_del (old, NULL);
		flags = ignore_get_flags (GTK_TREE_MODEL (store), &iter);
		ignore_add (new, flags, TRUE);

		/* update tree */
		gtk_list_store_set (store, &iter, MASK_COLUMN, new, -1);
	}
	g_free (old);
	
}

static void
option_toggled (GtkCellRendererToggle *render, gchar *path, gpointer data)
{
	GtkListStore *store = GTK_LIST_STORE (get_store ());
	GtkTreeIter iter;
	int col_id = GPOINTER_TO_INT (data);
	gboolean active;
	char *mask;
	int flags;

	gtkutil_treemodel_string_to_iter (GTK_TREE_MODEL (store), path, &iter);

	/* update model */
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, col_id, &active, -1);
	gtk_list_store_set (store, &iter, col_id, !active, -1);

	/* update ignore list */
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, 0, &mask, -1);
	flags = ignore_get_flags (GTK_TREE_MODEL (store), &iter);
	if (ignore_add (mask, flags, TRUE) != 2)
		g_warning ("ignore treeview is out of sync!\n");
	
	g_free (mask);
}

static GtkWidget *
ignore_treeview_new (GtkWidget *box)
{
	GtkListStore *store;
	GtkWidget *view;
	GtkTreeViewColumn *col;
	GtkCellRenderer *render;
	int col_id;

	store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING,
	                            G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
	                            G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
	                            G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
	                            G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
	g_return_val_if_fail (store != NULL, NULL);

	view = gtkutil_treeview_new (box, GTK_TREE_MODEL (store),
	                             NULL,
	                             MASK_COLUMN, _("Mask"),
	                             CHAN_COLUMN, _("Channel"),
	                             PRIV_COLUMN, _("Private"),
	                             NOTICE_COLUMN, _("Notice"),
	                             CTCP_COLUMN, _("CTCP"),
	                             DCC_COLUMN, _("DCC"),
	                             INVITE_COLUMN, _("Invite"),
	                             UNIGNORE_COLUMN, _("Unignore"),
	                             -1);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (view), TRUE);
	gtk_tree_view_column_set_expand (gtk_tree_view_get_column (GTK_TREE_VIEW (view), 0), TRUE);

	/* attach to signals and customise columns */
	for (col_id=0; (col = gtk_tree_view_get_column (GTK_TREE_VIEW (view), col_id));
	     col_id++)
	{
		GList *list = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (col));
		GList *tmp;

		for (tmp = list; tmp; tmp = tmp->next)
		{
			render = tmp->data;
			if (col_id > 0)	/* it's a toggle button column */
			{
				g_signal_connect (render, "toggled", G_CALLBACK (option_toggled),
				                  GINT_TO_POINTER (col_id));
			} else	/* mask column */
			{
				g_object_set (G_OBJECT (render), "editable", TRUE, NULL);
				g_signal_connect (render, "edited", G_CALLBACK (mask_edited), NULL);
				/* make this column sortable */
				gtk_tree_view_column_set_sort_column_id (col, col_id);
				gtk_tree_view_column_set_min_width (col, 272);
			}
			/* centre titles */
			gtk_tree_view_column_set_alignment (col, 0.5);
		}

		g_list_free (list);
	}
	
	gtk_widget_show (view);
	return view;
}

static void
ignore_delete_entry_clicked (GtkWidget * wid, struct session *sess)
{
	GtkTreeView *view = g_object_get_data (G_OBJECT (ignorewin), "view");
	GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (view));
	GtkTreeIter iter;
	GtkTreePath *path;
	char *mask = NULL;

	if (gtkutil_treeview_get_selected (view, &iter, 0, &mask, -1))
	{
		/* delete this row, select next one */
		if (gtk_list_store_remove (store, &iter))
		{
			path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
			gtk_tree_view_scroll_to_cell (view, path, NULL, TRUE, 1.0, 0.0);
			gtk_tree_view_set_cursor (view, path, NULL, FALSE);
			gtk_tree_path_free (path);
		}

		ignore_del (mask, NULL);
		g_free (mask);
	}
}

static void
ignore_store_new (int cancel, char *mask, gpointer data)
{
	GtkTreeView *view = g_object_get_data (G_OBJECT (ignorewin), "view");
	GtkListStore *store = GTK_LIST_STORE (get_store ());
	GtkTreeIter iter;
	GtkTreePath *path;
	int flags = IG_CHAN | IG_PRIV | IG_NOTI | IG_CTCP | IG_DCC | IG_INVI;

	if (cancel)
		return;
	/* check if it already exists */
	if (ignore_exists (mask))
	{
		fe_message (_("That mask already exists."), FE_MSG_ERROR);
		return;
	}

	ignore_add (mask, flags, TRUE);

	gtk_list_store_append (store, &iter);
	/* ignore everything by default */
	gtk_list_store_set (store, &iter, 0, mask, 1, TRUE, 2, TRUE, 3, TRUE,
	                    4, TRUE, 5, TRUE, 6, TRUE, 7, FALSE, -1);
	/* make sure the new row is visible and selected */
	path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
	gtk_tree_view_scroll_to_cell (view, path, NULL, TRUE, 1.0, 0.0);
	gtk_tree_view_set_cursor (view, path, NULL, FALSE);
	gtk_tree_path_free (path);
}

static void
ignore_clear_cb (GtkDialog *dialog, gint response)
{
	GtkListStore *store = GTK_LIST_STORE (get_store ());
	GtkTreeIter iter;
	char *mask;

	gtk_widget_destroy (GTK_WIDGET (dialog));

	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter) && response == GTK_RESPONSE_OK)
	{
		/* remove from ignore_list */
		do
		{
			mask = NULL;
			gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, MASK_COLUMN, &mask, -1);
			ignore_del (mask, NULL);
			g_free (mask);
		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));

		/* remove from GUI */
		gtk_list_store_clear (store);
	}
}

static void
ignore_clear_entry_clicked (GtkWidget * wid)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (NULL, 0,
								GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL,
					_("Are you sure you want to remove all ignores?"));
	g_signal_connect (G_OBJECT (dialog), "response",
							G_CALLBACK (ignore_clear_cb), NULL);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
	gtk_widget_show (dialog);
}

static void
ignore_new_entry_clicked (GtkWidget * wid, struct session *sess)
{
	fe_get_str (_("Enter mask to ignore:"), "nick!userid@host.com",
	            ignore_store_new, NULL);

}

static void
close_ignore_gui_callback (void)
{
	ignore_save ();
	ignorewin = 0;
}

static GtkWidget *
ignore_stats_entry (GtkWidget * box, char *label, int value)
{
	GtkWidget *wid;
	char buf[16];

	sprintf (buf, "%d", value);
	gtkutil_label_new (label, box);
	wid = gtkutil_entry_new (16, box, 0, 0);
	gtk_widget_set_size_request (wid, 30, -1);
	gtk_editable_set_editable (GTK_EDITABLE (wid), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (wid), FALSE);
	gtk_entry_set_text (GTK_ENTRY (wid), buf);

	return wid;
}

void
ignore_gui_open ()
{
	GtkWidget *vbox, *box, *stat_box, *frame;
	GtkWidget *view;
	GtkListStore *store;
	GtkTreeIter iter;
	GSList *temp = ignore_list;
	char *mask;
	gboolean private, chan, notice, ctcp, dcc, invite, unignore;

	if (ignorewin)
	{
		mg_bring_tofront (ignorewin);
		return;
	}

	ignorewin =
			  mg_create_generic_tab ("IgnoreList", _(DISPLAY_NAME": Ignore list"),
											FALSE, TRUE, close_ignore_gui_callback,
											NULL, 700, 300, &vbox, 0);
	gtkutil_destroy_on_esc (ignorewin);

	view = ignore_treeview_new (vbox);
	g_object_set_data (G_OBJECT (ignorewin), "view", view);
	
	frame = gtk_frame_new (_("Ignore Stats:"));
	gtk_widget_show (frame);

	stat_box = gtk_hbox_new (0, 2);
	gtk_container_set_border_width (GTK_CONTAINER (stat_box), 6);
	gtk_container_add (GTK_CONTAINER (frame), stat_box);
	gtk_widget_show (stat_box);

	num_chan = ignore_stats_entry (stat_box, _("Channel:"), ignored_chan);
	num_priv = ignore_stats_entry (stat_box, _("Private:"), ignored_priv);
	num_noti = ignore_stats_entry (stat_box, _("Notice:"), ignored_noti);
	num_ctcp = ignore_stats_entry (stat_box, _("CTCP:"), ignored_ctcp);
	num_invi = ignore_stats_entry (stat_box, _("Invite:"), ignored_invi);

	gtk_box_pack_start (GTK_BOX (vbox), frame, 0, 0, 5);

	box = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (box), GTK_BUTTONBOX_SPREAD);
	gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (box), 5);
	gtk_widget_show (box);

	gtkutil_button (box, GTK_STOCK_NEW, 0, ignore_new_entry_clicked, 0,
						 _("Add..."));
	gtkutil_button (box, GTK_STOCK_DELETE, 0, ignore_delete_entry_clicked,
						 0, _("Delete"));
	gtkutil_button (box, GTK_STOCK_CLEAR, 0, ignore_clear_entry_clicked,
						 0, _("Clear"));

	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));

	while (temp)
	{
		struct ignore *ignore = temp->data;

		mask = ignore->mask;
		chan = (ignore->type & IG_CHAN);
		private = (ignore->type & IG_PRIV);
		notice = (ignore->type & IG_NOTI);
		ctcp = (ignore->type & IG_CTCP);
		dcc = (ignore->type & IG_DCC);
		invite = (ignore->type & IG_INVI);
		unignore = (ignore->type & IG_UNIG);

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
		                    MASK_COLUMN, mask,
		                    CHAN_COLUMN, chan,
		                    PRIV_COLUMN, private,
		                    NOTICE_COLUMN, notice,
		                    CTCP_COLUMN, ctcp,
		                    DCC_COLUMN, dcc,
		                    INVITE_COLUMN, invite,
		                    UNIGNORE_COLUMN, unignore,
		                    -1);
		
		temp = temp->next;
	}
	gtk_widget_show (ignorewin);
}

void
fe_ignore_update (int level)
{
	/* some ignores have changed via /ignore, we should update
	   the gui now */
	/* level 1 = the list only. */
	/* level 2 = the numbers only. */
	/* for now, ignore level 1, since the ignore GUI isn't realtime,
	   only saved when you click OK */
	char buf[16];

	if (level == 2 && ignorewin)
	{
		sprintf (buf, "%d", ignored_ctcp);
		gtk_entry_set_text (GTK_ENTRY (num_ctcp), buf);

		sprintf (buf, "%d", ignored_noti);
		gtk_entry_set_text (GTK_ENTRY (num_noti), buf);

		sprintf (buf, "%d", ignored_chan);
		gtk_entry_set_text (GTK_ENTRY (num_chan), buf);

		sprintf (buf, "%d", ignored_invi);
		gtk_entry_set_text (GTK_ENTRY (num_invi), buf);

		sprintf (buf, "%d", ignored_priv);
		gtk_entry_set_text (GTK_ENTRY (num_priv), buf);
	}
}
