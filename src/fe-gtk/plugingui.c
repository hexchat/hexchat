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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "fe-gtk.h"

#include <gtk/gtkdialog.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkbox.h>

#include "../common/xchat.h"
#define PLUGIN_C
typedef struct session xchat_context;
#include "../common/xchat-plugin.h"
#include "../common/plugin.h"
#include "../common/util.h"
#include "../common/outbound.h"
#include "../common/xchatc.h"
#include "gtkutil.h"


static GtkWidget *plugin_window = NULL;
static GtkWidget *plugin_clist;


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
	gchar *entry[4];
	GSList *list;

	if (!plugin_window)
		return;

	gtk_clist_clear (GTK_CLIST (plugin_clist));

	list = plugin_list;
	while (list)
	{
		pl = list->data;
		entry[1] = pl->version;
		if (entry[1][0] != 0)
		{
			entry[0] = pl->name;
			entry[2] = file_part (pl->filename);
			entry[3] = pl->desc;
			gtk_clist_append (GTK_CLIST (plugin_clist), entry);
		}
		list = list->next;
	}
}

static void
plugingui_load_cb (session *sess, void *data2, char *file)
{
	if (file)
	{
		char *buf = malloc (strlen (file) + 7);

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
	int row, len;
	char *modname, *file, *buf;

	row = gtkutil_clist_selection (plugin_clist);
	if (row == -1)
		return;
	gtk_clist_get_text (GTK_CLIST (plugin_clist), row, 0, &modname);
	gtk_clist_get_text (GTK_CLIST (plugin_clist), row, 2, &file);
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
		buf = malloc (strlen (file) + 8);
		sprintf (buf, "UNLOAD %s", file);
		handle_command (current_sess, buf, FALSE);
		free (buf);
	}
}

void
plugingui_open (void)
{
	gchar *titles[] = { _("Name"), _("Version"), _("File"), _("Description") };
	GtkWidget *okb;

	if (plugin_window)
	{
		gtk_window_present (GTK_WINDOW (plugin_window));
		return;
	}

	plugin_window = gtk_dialog_new ();
	g_signal_connect (G_OBJECT (plugin_window), "destroy",
							G_CALLBACK (plugingui_close), 0);
	gtk_widget_set_usize (plugin_window, 360, 200);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (plugin_window)->vbox),
											  4);
	gtk_window_set_position (GTK_WINDOW (plugin_window), GTK_WIN_POS_CENTER);
	gtk_window_set_title (GTK_WINDOW (plugin_window), _("X-Chat: Plugins"));

	plugin_clist = gtkutil_clist_new (4, titles,
						 						 GTK_DIALOG (plugin_window)->vbox,
						 						 GTK_POLICY_AUTOMATIC, 0, 0, 0, 0,
												 GTK_SELECTION_BROWSE);
	gtk_clist_column_titles_passive (GTK_CLIST (plugin_clist));
	gtk_widget_show (plugin_clist);

	gtk_clist_set_column_auto_resize (GTK_CLIST (plugin_clist), 0, TRUE);
	gtk_clist_set_column_auto_resize (GTK_CLIST (plugin_clist), 2, TRUE);

	gtkutil_button (GTK_DIALOG (plugin_window)->action_area,
						 GTK_STOCK_REVERT_TO_SAVED, NULL, plugingui_loadbutton_cb,
						 NULL, _("_Load..."));

	gtkutil_button (GTK_DIALOG (plugin_window)->action_area,
						 GTK_STOCK_DELETE, NULL, plugingui_unload,
						 NULL, _("_UnLoad"));

	okb = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_box_pack_end (GTK_BOX (GTK_DIALOG (plugin_window)->action_area),
							okb, 1, 1, 10);
	g_signal_connect (G_OBJECT (okb), "clicked",
							G_CALLBACK (plugingui_close_button), 0);
	gtk_widget_show (okb);

	fe_pluginlist_update ();

	gtk_widget_show (plugin_window);
}
