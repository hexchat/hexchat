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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "fe-gtk.h"

#include <gtk/gtkdialog.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkscrolledwindow.h>

#include <gtk/gtkliststore.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkcellrenderertext.h>

#include "../common/xchat.h"
#define PLUGIN_C
typedef struct session xchat_context;
#include "../common/xchat-plugin.h"
#include "../common/plugin.h"
#include "../common/util.h"
#include "../common/outbound.h"
#include "../common/xchatc.h"
#include "gtkutil.h"

/* model for the plugin treeview */
enum
{
	NAME_COLUMN,
	VERSION_COLUMN,
	FILE_COLUMN,
	DESC_COLUMN,
	N_COLUMNS
};

static GtkWidget *plugin_window = NULL;


static GtkWidget *
plugingui_treeview_new (GtkWidget *box)
{
	GtkListStore *store;
	GtkWidget *view;
	GtkTreeViewColumn *col;
	int col_id;

	store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING,
	                            G_TYPE_STRING, G_TYPE_STRING);
	g_return_val_if_fail (store != NULL, NULL);
	view = gtkutil_treeview_new (box, GTK_TREE_MODEL (store), NULL,
	                             NAME_COLUMN, _("Name"),
	                             VERSION_COLUMN, _("Version"),
	                             FILE_COLUMN, _("File"),
	                             DESC_COLUMN, _("Description"), -1);
	for (col_id=0; (col = gtk_tree_view_get_column (GTK_TREE_VIEW (view), col_id));
	     col_id++)
			gtk_tree_view_column_set_alignment (col, 0.5);

	gtk_widget_show (view);
	return view;
}

static void
plugingui_close_button (GtkWidget * wid, gpointer a)
{
	gtk_widget_destroy (plugin_window);
}

static void
plugingui_close (GtkWidget * wid, gpointer a)
{
	plugin_window = NULL;
}

extern GSList *plugin_list;

void
fe_pluginlist_update (void)
{
	xchat_plugin *pl;
	GSList *list;
	GtkTreeView *view;
	GtkListStore *store;
	GtkTreeIter iter;

	if (!plugin_window)
		return;

	view = g_object_get_data (G_OBJECT (plugin_window), "view");
	store = GTK_LIST_STORE (gtk_tree_view_get_model (view));
	gtk_list_store_clear (store);

	list = plugin_list;
	while (list)
	{
		pl = list->data;
		if (pl->version[0] != 0)
		{
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, NAME_COLUMN, pl->name,
			                    VERSION_COLUMN, pl->version,
			                    FILE_COLUMN, file_part (pl->filename),
			                    DESC_COLUMN, pl->desc, -1);
		}
		list = list->next;
	}
}

static void
plugingui_load_cb (session *sess, void *data2, char *file)
{
	if (file)
	{
		char *buf = malloc (strlen (file) + 9);

		if (strchr (file, ' '))
			sprintf (buf, "LOAD \"%s\"", file);
		else
			sprintf (buf, "LOAD %s", file);
		handle_command (sess, buf, FALSE);
		free (buf);
	}
}

void
plugingui_load (void)
{
	gtkutil_file_req (_("Select a Plugin or Script to load"), plugingui_load_cb,
							current_sess, 0, FALSE);
}

static void
plugingui_loadbutton_cb (GtkWidget * wid, gpointer unused)
{
	plugingui_load ();
}

static void
plugingui_unload (GtkWidget * wid, gpointer unused)
{
	int len;
	char *modname, *file, *buf;
	GtkTreeView *view;
	GtkTreeIter iter;
	
	view = g_object_get_data (G_OBJECT (plugin_window), "view");
	if (!gtkutil_treeview_get_selected (view, &iter, NAME_COLUMN, &modname,
	                                    FILE_COLUMN, &file, -1))
		return;

	len = strlen (file);
#ifdef WIN32
	if (len > 4 && strcasecmp (file + len - 4, ".dll") == 0)
#else
	if (len > 3 && strcasecmp (file + len - 3, ".so") == 0)
#endif
	{
		if (plugin_kill (modname, FALSE) == 2)
			gtkutil_simpledialog (_("That plugin is refusing to unload.\n"));
	} else
	{
		/* let python.so or perl.so handle it */
		buf = malloc (strlen (file) + 10);
		if (strchr (file, ' '))
			sprintf (buf, "UNLOAD \"%s\"", file);
		else
			sprintf (buf, "UNLOAD %s", file);
		handle_command (current_sess, buf, FALSE);
		free (buf);
	}

	g_free (modname);
	g_free (file);
}

void
plugingui_open (void)
{
	GtkWidget *view;
	GtkWidget *vbox, *action_area;

	if (plugin_window)
	{
		gtk_window_present (GTK_WINDOW (plugin_window));
		return;
	}

	plugin_window = gtk_dialog_new ();
	g_signal_connect (G_OBJECT (plugin_window), "destroy",
							G_CALLBACK (plugingui_close), 0);
	gtk_window_set_default_size (GTK_WINDOW (plugin_window), 450, 200);
	vbox = GTK_DIALOG (plugin_window)->vbox;
	action_area = GTK_DIALOG (plugin_window)->action_area;
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
	gtk_window_set_position (GTK_WINDOW (plugin_window), GTK_WIN_POS_CENTER);
	gtk_window_set_title (GTK_WINDOW (plugin_window), _("X-Chat: Plugins and Scripts"));

	view = plugingui_treeview_new (vbox);
	g_object_set_data (G_OBJECT (plugin_window), "view", view);

	gtkutil_button (action_area, GTK_STOCK_REVERT_TO_SAVED, NULL,
	                plugingui_loadbutton_cb, NULL, _("_Load..."));

	gtkutil_button (action_area, GTK_STOCK_DELETE, NULL,
	                plugingui_unload, NULL, _("_UnLoad"));

	gtkutil_button (action_area,
						 GTK_STOCK_CLOSE, NULL, plugingui_close_button,
						 NULL, _("_Close"));
 
	fe_pluginlist_update ();

	gtk_widget_show (plugin_window);
}
