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
#include <sys/types.h>
#include <sys/stat.h>

#include <gdk/gdkkeysyms.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "fe-gtk.h"

#include "../common/hexchat.h"
#include "../common/cfgfiles.h"
#include "../common/hexchatc.h"
#include "../common/fe.h"
#include "menu.h"
#include "gtkutil.h"
#include "maingui.h"
#include "editlist.h"

enum
{
	NAME_COLUMN,
	CMD_COLUMN,
	N_COLUMNS
};

static GtkWidget *editlist_win = NULL;
static GSList *editlist_list = NULL;

static GtkTreeModel *
get_store (void)
{
	return gtk_tree_view_get_model (g_object_get_data (G_OBJECT (editlist_win), "view"));
}

static void
editlist_save (GtkWidget *igad, gchar *file)
{
	GtkTreeModel *store = get_store ();
	GtkTreeIter iter;
	char buf[512];
	char *name, *cmd;
	int fh;

	fh = hexchat_open_file (file, O_TRUNC | O_WRONLY | O_CREAT, 0600, XOF_DOMODE);
	if (fh != -1)
	{
		if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
		{
			do
			{
				name = NULL;
				cmd = NULL;
				gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, NAME_COLUMN, &name, CMD_COLUMN, &cmd, -1);
				g_snprintf (buf, sizeof (buf), "NAME %s\nCMD %s\n\n", name, cmd);
				write (fh, buf, strlen (buf));
				g_free (name);
				g_free (cmd);
			} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));
		}

		close (fh);
		gtk_widget_destroy (editlist_win);
		if (editlist_list == replace_list)
		{
			list_free (&replace_list);
			list_loadconf (file, &replace_list, 0);
		} else if (editlist_list == popup_list)
		{
			list_free (&popup_list);
			list_loadconf (file, &popup_list, 0);
		} else if (editlist_list == button_list)
		{
			GSList *list = sess_list;
			struct session *sess;
			list_free (&button_list);
			list_loadconf (file, &button_list, 0);
			while (list)
			{
				sess = (struct session *) list->data;
				fe_buttons_update (sess);
				list = list->next;
			}
		} else if (editlist_list == dlgbutton_list)
		{
			GSList *list = sess_list;
			struct session *sess;
			list_free (&dlgbutton_list);
			list_loadconf (file, &dlgbutton_list, 0);
			while (list)
			{
				sess = (struct session *) list->data;
				fe_dlgbuttons_update (sess);
				list = list->next;
			}
		} else if (editlist_list == ctcp_list)
		{
			list_free (&ctcp_list);
			list_loadconf (file, &ctcp_list, 0);
		} else if (editlist_list == command_list)
		{
			list_free (&command_list);
			list_loadconf (file, &command_list, 0);
		} else if (editlist_list == usermenu_list)
		{
			list_free (&usermenu_list);
			list_loadconf (file, &usermenu_list, 0);
			usermenu_update ();
		} else
		{
			list_free (&urlhandler_list);
			list_loadconf (file, &urlhandler_list, 0);
		}
	}
}

static void
editlist_load (GtkListStore *store, GSList *list)
{
	struct popup *pop;
	gchar *name, *cmd;
	GtkTreeIter iter;

	while (list)
	{
		pop = (struct popup *) list->data;
		name = pop->name;
		cmd = pop->cmd;

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
						NAME_COLUMN, name,
						CMD_COLUMN, cmd, -1);

		list = list->next;
	}
}

static void
editlist_delete (GtkWidget *wid, gpointer userdata)
{
	GtkTreeView *view = g_object_get_data (G_OBJECT (editlist_win), "view");
	GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (view));
	GtkTreeIter iter;
	GtkTreePath *path;

	if (gtkutil_treeview_get_selected (view, &iter, -1))
	{
		/* delete this row, select next one */
		if (gtk_list_store_remove (store, &iter))
		{
			path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
			gtk_tree_view_scroll_to_cell (view, path, NULL, TRUE, 1.0, 0.0);
			gtk_tree_view_set_cursor (view, path, NULL, FALSE);
			gtk_tree_path_free (path);
		}
	}
}

static void
editlist_add (GtkWidget *wid, gpointer userdata)
{
	GtkTreeView *view = g_object_get_data (G_OBJECT (editlist_win), "view");
	GtkTreeViewColumn *col;
	GtkListStore *store = GTK_LIST_STORE (get_store ());
	GtkTreeIter iter;
	GtkTreePath *path;

	gtk_list_store_append (store, &iter);

	/* make sure the new row is visible and selected */
	path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
	col = gtk_tree_view_get_column (view, NAME_COLUMN);
	gtk_tree_view_scroll_to_cell (view, path, NULL, FALSE, 0.0, 0.0);
	gtk_tree_view_set_cursor (view, path, col, TRUE);
	gtk_tree_path_free (path);
}

static void
editlist_close (GtkWidget *wid, gpointer userdata)
{
	gtk_widget_destroy (editlist_win);
	editlist_win = NULL;
}

static void
editlist_edited (GtkCellRendererText *render, gchar *pathstr, gchar *new_text, gpointer data)
{
	GtkTreeModel *model = get_store ();
	GtkTreePath *path = gtk_tree_path_new_from_string (pathstr);
	GtkTreeIter iter;
	gint column = GPOINTER_TO_INT (data);

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, new_text, -1);

	gtk_tree_path_free (path);
}

static gboolean
editlist_keypress (GtkWidget *wid, GdkEventKey *evt, gpointer userdata)
{
	GtkTreeView *view = g_object_get_data (G_OBJECT (editlist_win), "view");
	GtkTreeModel *store;
	GtkTreeIter iter1, iter2;
	GtkTreeSelection *sel;
	GtkTreePath *path;
	gboolean handled = FALSE;
	int delta;

	if (evt->state & GDK_SHIFT_MASK)
	{
		if (evt->keyval == GDK_KEY_Up)
		{
			handled = TRUE;
			delta = -1;
		}
		else if (evt->keyval == GDK_KEY_Down)
		{
			handled = TRUE;
			delta = 1;
		}
	}

	if (handled)
	{
		sel = gtk_tree_view_get_selection (view);
		gtk_tree_selection_get_selected (sel, &store, &iter1);
		path = gtk_tree_model_get_path (store, &iter1);
		if (delta == 1)
			gtk_tree_path_next (path);
		else
			gtk_tree_path_prev (path);
		gtk_tree_model_get_iter (store, &iter2, path);
		gtk_tree_path_free (path);
		gtk_list_store_swap (GTK_LIST_STORE (store), &iter1, &iter2);
	}

	return handled;
}

static GtkWidget *
editlist_treeview_new (GtkWidget *box, char *title1, char *title2)
{
	GtkWidget *scroll;
	GtkListStore *store;
	GtkTreeViewColumn *col;
	GtkWidget *view;
	GtkCellRenderer *render;

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);

	store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
	g_return_val_if_fail (store != NULL, NULL);

	view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (view), TRUE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (view), FALSE);
	gtk_tree_view_set_reorderable (GTK_TREE_VIEW (view), TRUE);
	
	g_signal_connect (G_OBJECT (view), "key_press_event",
						G_CALLBACK (editlist_keypress), NULL);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (view), TRUE);

	render = gtk_cell_renderer_text_new ();
	g_object_set (render, "editable", TRUE, NULL);
	g_signal_connect (G_OBJECT (render), "edited",
					G_CALLBACK (editlist_edited), GINT_TO_POINTER(NAME_COLUMN));
	gtk_tree_view_insert_column_with_attributes (
					GTK_TREE_VIEW (view), NAME_COLUMN,
					title1, render,
					"text", NAME_COLUMN,
					NULL);

	render = gtk_cell_renderer_text_new ();
	g_object_set (render, "editable", TRUE, NULL);
	g_signal_connect (G_OBJECT (render), "edited",
					G_CALLBACK (editlist_edited), GINT_TO_POINTER(CMD_COLUMN));
	gtk_tree_view_insert_column_with_attributes (
					GTK_TREE_VIEW (view), CMD_COLUMN,
					title2, render,
					"text", CMD_COLUMN,
					NULL);

	col = gtk_tree_view_get_column (GTK_TREE_VIEW (view), NAME_COLUMN);
	gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_set_min_width (col, 100);

	gtk_container_add (GTK_CONTAINER (scroll), view);
	gtk_container_add (GTK_CONTAINER (box), scroll);
	gtk_widget_show_all (box);

	return view;
}


void
editlist_gui_open (char *title1, char *title2, GSList *list, char *title, char *wmclass,
					char *file, char *help)
{
	GtkWidget *vbox, *box;
	GtkWidget *view;
	GtkListStore *store;

	if (editlist_win)
	{
		mg_bring_tofront (editlist_win);
		return;
	}

	editlist_win = mg_create_generic_tab (wmclass, title, TRUE, FALSE,
												editlist_close, NULL, 450, 250, &vbox, 0);

	editlist_list = list;

	view = editlist_treeview_new (vbox, title1, title2);
	g_object_set_data (G_OBJECT (editlist_win), "view", view);

	if (help)
		gtk_widget_set_tooltip_text (view, help);

	box = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (box), GTK_BUTTONBOX_SPREAD);
	gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (box), 5);
	gtk_widget_show (box);

	gtkutil_button (box, GTK_STOCK_NEW, 0, editlist_add,
					NULL, _("Add"));
	gtkutil_button (box, GTK_STOCK_DELETE, 0, editlist_delete,
					NULL, _("Delete"));
	gtkutil_button (box, GTK_STOCK_CANCEL, 0, editlist_close,
					NULL, _("Cancel"));
	gtkutil_button (box, GTK_STOCK_SAVE, 0, editlist_save,
					file, _("Save"));

	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));
	editlist_load (store, list);

	gtk_widget_show (editlist_win);
}
