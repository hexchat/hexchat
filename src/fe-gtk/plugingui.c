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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "fe-gtk.h"

#include "../common/hexchat.h"
#define PLUGIN_C
typedef struct session hexchat_context;
#include "../common/hexchat-plugin.h"
#include "../common/plugin.h"
#include "../common/util.h"
#include "../common/outbound.h"
#include "../common/fe.h"
#include "../common/hexchatc.h"
#include "../common/cfgfiles.h"
#include "gtkutil.h"
#include "maingui.h"

/* model for the plugin treeview */
enum
{
	NAME_COLUMN,
	VERSION_COLUMN,
	FILE_COLUMN,
	DESC_COLUMN,
	FILEPATH_COLUMN,
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
	                            G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	g_return_val_if_fail (store != NULL, NULL);
	view = gtkutil_treeview_new (box, GTK_TREE_MODEL (store), NULL,
	                             NAME_COLUMN, _("Name"),
	                             VERSION_COLUMN, _("Version"),
	                             FILE_COLUMN, _("File"),
	                             DESC_COLUMN, _("Description"),
	                             FILEPATH_COLUMN, NULL, -1);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (view), TRUE);
	for (col_id=0; (col = gtk_tree_view_get_column (GTK_TREE_VIEW (view), col_id));
	     col_id++)
			gtk_tree_view_column_set_alignment (col, 0.5);

	return view;
}

static char *
plugingui_getfilename (GtkTreeView *view)
{
	GtkTreeModel *model;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GValue file;
	char *str;

	memset (&file, 0, sizeof (file));

	sel = gtk_tree_view_get_selection (view);
	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		gtk_tree_model_get_value (model, &iter, FILEPATH_COLUMN, &file);

		str = g_value_dup_string (&file);
		g_value_unset (&file);

		return str;
	}

	return NULL;
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
	hexchat_plugin *pl;
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
			                    DESC_COLUMN, pl->desc,
			                    FILEPATH_COLUMN, pl->filename, -1);
		}
		list = list->next;
	}
}

static void
plugingui_load_cb (session *sess, char *file)
{
	if (file)
	{
		char *buf;

		if (strchr (file, ' '))
			buf = g_strdup_printf ("LOAD \"%s\"", file);
		else
			buf = g_strdup_printf ("LOAD %s", file);
		handle_command (sess, buf, FALSE);
		g_free (buf);
	}
}

void
plugingui_load (void)
{
	char *sub_dir = g_build_filename (get_xdir(), "addons", NULL);

	gtkutil_file_req (NULL, _("Select a Plugin or Script to load"), plugingui_load_cb, current_sess,
							sub_dir, "*."PLUGIN_SUFFIX";*.lua;*.pl;*.py;*.tcl;*.js", FRF_FILTERISINITIAL|FRF_EXTENSIONS);

	g_free (sub_dir);
}

static void
plugingui_loadbutton_cb (GtkWidget * wid, gpointer unused)
{
	plugingui_load ();
}

static void
plugingui_unload (GtkWidget * wid, gpointer unused)
{
	char *modname, *file;
	GtkTreeView *view;
	GtkTreeIter iter;
	
	view = g_object_get_data (G_OBJECT (plugin_window), "view");
	if (!gtkutil_treeview_get_selected (view, &iter, NAME_COLUMN, &modname,
	                                    FILEPATH_COLUMN, &file, -1))
		return;

	if (g_str_has_suffix (file, "."PLUGIN_SUFFIX))
	{
		if (plugin_kill (modname, FALSE) == 2)
			fe_message (_("That plugin is refusing to unload.\n"), FE_MSG_ERROR);
	}
	else
	{
		char *buf;
		/* let python.so or perl.so handle it */
		if (strchr (file, ' '))
			buf = g_strdup_printf ("UNLOAD \"%s\"", file);
		else
			buf = g_strdup_printf ("UNLOAD %s", file);
		handle_command (current_sess, buf, FALSE);
		g_free (buf);
	}

	g_free (modname);
	g_free (file);
}

static void
plugingui_reloadbutton_cb (GtkWidget *wid, GtkTreeView *view)
{
	char *file = plugingui_getfilename(view);

	if (file)
	{
		char *buf;

		if (strchr (file, ' '))
			buf = g_strdup_printf ("RELOAD \"%s\"", file);
		else
			buf = g_strdup_printf ("RELOAD %s", file);
		handle_command (current_sess, buf, FALSE);
		g_free (buf);
		g_free (file);
	}
}

void
plugingui_open (void)
{
	GtkWidget *view;
	GtkWidget *vbox, *hbox;
	char buf[128];

	if (plugin_window)
	{
		mg_bring_tofront (plugin_window);
		return;
	}

	g_snprintf(buf, sizeof(buf), _("Plugins and Scripts - %s"), _(DISPLAY_NAME));
	plugin_window = mg_create_generic_tab ("Addons", buf, FALSE, TRUE, plugingui_close, NULL,
														 700, 300, &vbox, 0);
	gtkutil_destroy_on_esc (plugin_window);

	view = plugingui_treeview_new (vbox);
	g_object_set_data (G_OBJECT (plugin_window), "view", view);


	hbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_SPREAD);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, 0, 0, 0);

	gtkutil_button (hbox, GTK_STOCK_REVERT_TO_SAVED, NULL,
	                plugingui_loadbutton_cb, NULL, _("_Load..."));

	gtkutil_button (hbox, GTK_STOCK_DELETE, NULL,
	                plugingui_unload, NULL, _("_Unload"));

	gtkutil_button (hbox, GTK_STOCK_REFRESH, NULL,
	                plugingui_reloadbutton_cb, view, _("_Reload"));

	fe_pluginlist_update ();

	gtk_widget_show_all (plugin_window);
}
