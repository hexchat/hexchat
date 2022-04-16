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
#include <string.h>
#include <stdlib.h>

#include "fe-gtk.h"

#include "../common/hexchat.h"
#include "../common/hexchatc.h"
#include "../common/cfgfiles.h"
#include "../common/fe.h"
#include "../common/url.h"
#include "../common/tree.h"
#include "gtkutil.h"
#include "menu.h"
#include "maingui.h"
#include "urlgrab.h"

/* model for the URL treeview */
enum
{
	URL_COLUMN,
	N_COLUMNS
};

static GtkWidget *urlgrabberwindow = 0;


static gboolean
url_treeview_url_clicked_cb (GtkWidget *view, GdkEventButton *event,
                             gpointer data)
{
	GtkTreeIter iter;
	gchar *url;
	GtkTreeSelection *sel;
	GtkTreePath *path;
	GtkTreeView *tree = GTK_TREE_VIEW (view);

	if (!event || !gtk_tree_view_get_path_at_pos (tree, event->x, event->y, &path, 0, 0, 0))
		return FALSE;

	/* select what they right-clicked on */
	sel = gtk_tree_view_get_selection (tree); 
	gtk_tree_selection_unselect_all (sel);
	gtk_tree_selection_select_path (sel, path);
	gtk_tree_path_free (path); 

	if (!gtkutil_treeview_get_selected (GTK_TREE_VIEW (view), &iter,
	                                    URL_COLUMN, &url, -1))
		return FALSE;
	
	switch (event->button)
	{
		case 1:
			if (event->type == GDK_2BUTTON_PRESS)
				fe_open_url (url);
			break;
		case 3:
			menu_urlmenu (event, url);
			break;
		default:
			break;
	}
	g_free (url);

	return FALSE;
}

static GtkWidget *
url_treeview_new (GtkWidget *box)
{
	GtkListStore *store;
	GtkWidget *view;

	store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING);
	g_return_val_if_fail (store != NULL, NULL);

	view = gtkutil_treeview_new (box, GTK_TREE_MODEL (store), NULL,
	                             URL_COLUMN, _("URL"), -1);
	g_signal_connect (G_OBJECT (view), "button_press_event",
	                  G_CALLBACK (url_treeview_url_clicked_cb), NULL);
	/* don't want column headers */
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
	gtk_widget_show (view);
	return view;
}

static void
url_closegui (GtkWidget *wid, gpointer userdata)
{
	urlgrabberwindow = 0;
}

static void
url_button_clear (void)
{
	GtkListStore *store;
	
	url_clear ();
	store = GTK_LIST_STORE (g_object_get_data (G_OBJECT (urlgrabberwindow),
	                                           "model"));
	gtk_list_store_clear (store);
}

static void
url_button_copy (GtkWidget *widget, gpointer data)
{
	GtkTreeView *view = GTK_TREE_VIEW (data);
	GtkTreeIter iter;
	gchar *url = NULL;

	if (gtkutil_treeview_get_selected (view, &iter, URL_COLUMN, &url, -1))
	{
		gtkutil_copy_to_clipboard (GTK_WIDGET (view), NULL, url);
		g_free (url);
	}
}

static void
url_save_callback (void *arg1, char *file)
{
	if (file)
	{
		url_save_tree (file, "w", TRUE);
	}
}

static void
url_button_save (void)
{
	gtkutil_file_req (NULL, _("Select an output filename"),
							url_save_callback, NULL, NULL, NULL, FRF_WRITE);
}

void
fe_url_add (const char *urltext)
{
	GtkListStore *store;
	GtkTreeIter iter;
	gboolean valid;
	
	if (urlgrabberwindow)
	{
		store = GTK_LIST_STORE (g_object_get_data (G_OBJECT (urlgrabberwindow),
		                                           "model"));
		gtk_list_store_prepend (store, &iter);
		gtk_list_store_set (store, &iter,
		                    URL_COLUMN, urltext,
		                    -1);

		/* remove any overflow */
		if (prefs.hex_url_grabber_limit > 0)
		{
			valid = gtk_tree_model_iter_nth_child (
				GTK_TREE_MODEL (store), &iter, NULL, prefs.hex_url_grabber_limit);
			while (valid)
				valid = gtk_list_store_remove (store, &iter);
		}
	}
}

static int
populate_cb (char *urltext, gpointer userdata)
{
	fe_url_add (urltext);
	return TRUE;
}

void
url_opengui ()
{
	GtkWidget *vbox, *hbox, *view;
	char buf[128];

	if (urlgrabberwindow)
	{
		mg_bring_tofront (urlgrabberwindow);
		return;
	}

	g_snprintf(buf, sizeof(buf), _("URL Grabber - %s"), _(DISPLAY_NAME));
	urlgrabberwindow =
		mg_create_generic_tab ("UrlGrabber", buf, FALSE, TRUE, url_closegui, NULL,
							 400, 256, &vbox, 0);
	gtkutil_destroy_on_esc (urlgrabberwindow);
	view = url_treeview_new (vbox);
	g_object_set_data (G_OBJECT (urlgrabberwindow), "model",
	                   gtk_tree_view_get_model (GTK_TREE_VIEW (view)));

	hbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_SPREAD);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, 0, 0, 0);
	gtk_widget_show (hbox);

	gtkutil_button (hbox, GTK_STOCK_CLEAR,
						 _("Clear list"), url_button_clear, 0, _("Clear"));
	gtkutil_button (hbox, GTK_STOCK_COPY,
						 _("Copy selected URL"), url_button_copy, view, _("Copy"));
	gtkutil_button (hbox, GTK_STOCK_SAVE_AS,
						 _("Save list to a file"), url_button_save, 0, _("Save As..."));

	gtk_widget_show (urlgrabberwindow);

	if (prefs.hex_url_grabber)
		tree_foreach (url_tree, (tree_traverse_func *)populate_cb, NULL);
	else
	{
		gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view))));
		fe_url_add ("URL Grabber is disabled.");
	}
}
