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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "fe-gtk.h"

#include <gtk/gtkstock.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvseparator.h>

#include "../common/xchat.h"
#include "../common/cfgfiles.h"
#include "../common/xchatc.h"
#include "../common/fe.h"
#include "menu.h"
#include "gtkutil.h"
#include "maingui.h"
#include "editlist.h"


static GtkWidget *editlist_gui_entry_name;
static GtkWidget *editlist_gui_entry_cmd;
static GtkWidget *editlist_gui_window;
static GtkWidget *editlist_gui_list;
static GSList *editlist_list;
static char *editlist_file;
static char *editlist_help;



static void
editlist_gui_load (GtkWidget * listgad)
{
	gchar *nnew[2];
	GSList *list = editlist_list;
	struct popup *pop;

	while (list)
	{
		pop = (struct popup *) list->data;
		nnew[0] = pop->name;
		nnew[1] = pop->cmd;
		gtk_clist_append (GTK_CLIST (listgad), nnew);
		list = list->next;
	}
}

static void
editlist_gui_row_unselected (GtkWidget * clist, gint row, gint column,
									  GdkEventButton * even, gpointer none)
{
	gtk_entry_set_text (GTK_ENTRY (editlist_gui_entry_name), "");
	gtk_entry_set_text (GTK_ENTRY (editlist_gui_entry_cmd), "");
}

static void
editlist_gui_row_selected (GtkWidget * clist, gint row, gint column,
									GdkEventButton * even, gpointer none)
{
	char *name, *cmd;

	row = gtkutil_clist_selection (editlist_gui_list);
	if (row != -1)
	{
		gtk_clist_get_text (GTK_CLIST (clist), row, 0, &name);
		gtk_clist_get_text (GTK_CLIST (clist), row, 1, &cmd);

		name = strdup (name);
		cmd = strdup (cmd);

		gtk_entry_set_text (GTK_ENTRY (editlist_gui_entry_name), name);
		gtk_entry_set_text (GTK_ENTRY (editlist_gui_entry_cmd), cmd);

		free (name);
		free (cmd);
	} else
	{
		editlist_gui_row_unselected (0, 0, 0, 0, 0);
	}
}

static void
editlist_gui_handle_cmd (GtkWidget * igad)
{
	int row;
	const char *reply;

	row = gtkutil_clist_selection (editlist_gui_list);
	if (row != -1)
	{
		reply = gtk_entry_get_text (GTK_ENTRY (igad));
		gtk_clist_set_text (GTK_CLIST (editlist_gui_list), row, 1, reply);
	}
}

static void
editlist_gui_handle_name (GtkWidget * igad)
{
	int row;
	const char *ctcp;

	row = gtkutil_clist_selection (editlist_gui_list);
	if (row != -1)
	{
		ctcp = gtk_entry_get_text (GTK_ENTRY (igad));
		gtk_clist_set_text (GTK_CLIST (editlist_gui_list), row, 0, ctcp);
	}
}

static void
editlist_gui_addnew (GtkWidget * igad)
{
	int i;
	gchar *nnew[2];

	nnew[0] = _("*NEW*");
	nnew[1] = _("EDIT ME");

	i = gtk_clist_append (GTK_CLIST (editlist_gui_list), nnew);
	gtk_clist_select_row (GTK_CLIST (editlist_gui_list), i, 0);
	gtk_clist_moveto (GTK_CLIST (editlist_gui_list), i, 0, 0.5, 0);
}

static void
editlist_gui_delete (GtkWidget * igad)
{
	int row;

	row = gtkutil_clist_selection (editlist_gui_list);
	if (row != -1)
	{
		gtk_clist_unselect_all (GTK_CLIST (editlist_gui_list));
		gtk_clist_remove (GTK_CLIST (editlist_gui_list), row);
	}
}

static void
editlist_gui_save (GtkWidget * igad)
{
	int fh, i = 0;
	char buf[512];
	char *a, *b;

	snprintf (buf, sizeof buf, "%s/%s", get_xdir (), editlist_file);
	fh = open (buf, O_TRUNC | O_WRONLY | O_CREAT | OFLAGS, 0600);
	if (fh != -1)
	{
		while (1)
		{
			if (!gtk_clist_get_text (GTK_CLIST (editlist_gui_list), i, 0, &a))
				break;
			gtk_clist_get_text (GTK_CLIST (editlist_gui_list), i, 1, &b);
			snprintf (buf, sizeof (buf), "NAME %s\nCMD %s\n\n", a, b);
			write (fh, buf, strlen (buf));
			i++;
		}
		close (fh);
		gtk_widget_destroy (editlist_gui_window);
		if (editlist_list == replace_list)
		{
			list_free (&replace_list);
			list_loadconf (editlist_file, &replace_list, 0);
		} else if (editlist_list == popup_list)
		{
			list_free (&popup_list);
			list_loadconf (editlist_file, &popup_list, 0);
		} else if (editlist_list == button_list)
		{
			GSList *list = sess_list;
			struct session *sess;
			list_free (&button_list);
			list_loadconf (editlist_file, &button_list, 0);
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
			list_loadconf (editlist_file, &dlgbutton_list, 0);
			while (list)
			{
				sess = (struct session *) list->data;
				fe_dlgbuttons_update (sess);
				list = list->next;
			}
		} else if (editlist_list == ctcp_list)
		{
			list_free (&ctcp_list);
			list_loadconf (editlist_file, &ctcp_list, 0);
		} else if (editlist_list == command_list)
		{
			list_free (&command_list);
			list_loadconf (editlist_file, &command_list, 0);
		} else if (editlist_list == usermenu_list)
		{
			list_free (&usermenu_list);
			list_loadconf (editlist_file, &usermenu_list, 0);
			usermenu_update ();
		} else
		{
			list_free (&urlhandler_list);
			list_loadconf (editlist_file, &urlhandler_list, 0);
		}
	}
}

static void
editlist_gui_help (GtkWidget * igad)
{
/*	if (editlist_help)*/
		gtkutil_simpledialog (editlist_help);
}

static void
editlist_gui_sort (GtkWidget * igad)
{
	int row;

	row = gtkutil_clist_selection (editlist_gui_list);
	if (row != -1)
		gtk_clist_unselect_row (GTK_CLIST (editlist_gui_list), row, 0);
	gtk_clist_sort (GTK_CLIST (editlist_gui_list));
}

static void
editlist_gui_movedown (GtkWidget * igad)
{
	int row;
	char *temp;

	row = gtkutil_clist_selection (editlist_gui_list);
	if (row != -1)
	{
		if (!gtk_clist_get_text (GTK_CLIST (editlist_gui_list), row + 1, 0, &temp))
			return;
		gtk_clist_freeze (GTK_CLIST (editlist_gui_list));
		gtk_clist_swap_rows (GTK_CLIST (editlist_gui_list), row, row + 1);
		gtk_clist_thaw (GTK_CLIST (editlist_gui_list));
		row++;
		if (!gtk_clist_row_is_visible (GTK_CLIST (editlist_gui_list), row) !=
			 GTK_VISIBILITY_FULL)
			gtk_clist_moveto (GTK_CLIST (editlist_gui_list), row, 0, 0.9, 0);
	}
}

static void
editlist_gui_moveup (GtkWidget * igad)
{
	int row;

	row = gtkutil_clist_selection (editlist_gui_list);
	if (row != -1 && row > 0)
	{
		gtk_clist_freeze (GTK_CLIST (editlist_gui_list));
		gtk_clist_swap_rows (GTK_CLIST (editlist_gui_list), row - 1, row);
		gtk_clist_thaw (GTK_CLIST (editlist_gui_list));
		row--;
		if (gtk_clist_row_is_visible (GTK_CLIST (editlist_gui_list), row) !=
			 GTK_VISIBILITY_FULL)
			gtk_clist_moveto (GTK_CLIST (editlist_gui_list), row, 0, 0.1, 0);
	}
}

static void
editlist_gui_close (void)
{
	editlist_gui_window = 0;
}

void
editlist_gui_open (GSList * list, char *title, char *wmclass, char *file,
						 char *help)
{
	gchar *titles[2];
	GtkWidget *vbox, *hbox, *button;

	titles[0] = _("Name");
	titles[1] = _("Command");

	if (editlist_gui_window)
	{
		mg_bring_tofront (editlist_gui_window);
		return;
	}

	editlist_list = list;
	editlist_file = file;
	editlist_help = help;

	editlist_gui_window =
			  mg_create_generic_tab (wmclass, title, TRUE, FALSE,
											 editlist_gui_close, NULL, 450, 250, &vbox, 0);

	editlist_gui_list = gtkutil_clist_new (2, titles, vbox, GTK_POLICY_ALWAYS,
														editlist_gui_row_selected, 0,
														editlist_gui_row_unselected, 0,
														GTK_SELECTION_BROWSE);
	gtk_clist_set_column_width (GTK_CLIST (editlist_gui_list), 0, 90);

	hbox = gtk_hbox_new (0, 2);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, 0, 0, 0);
	gtk_widget_show (hbox);

	button = gtkutil_button (hbox, GTK_STOCK_GO_UP, 0, editlist_gui_moveup,
									 0, _("Move Up"));
	gtk_widget_set_usize (button, 100, 0);

	button = gtkutil_button (hbox, GTK_STOCK_GO_DOWN, 0, editlist_gui_movedown,
									 0, _("Move Dn"));
	gtk_widget_set_usize (button, 100, 0);

	button = gtk_vseparator_new ();
	gtk_container_add (GTK_CONTAINER (hbox), button);
	gtk_widget_show (button);

	button = gtkutil_button (hbox, GTK_STOCK_CANCEL, 0, gtkutil_destroy,
									 editlist_gui_window, _("Cancel"));
	gtk_widget_set_usize (button, 100, 0);

	button = gtkutil_button (hbox, GTK_STOCK_SAVE, 0, editlist_gui_save,
									 0, _("Save"));
	gtk_widget_set_usize (button, 100, 0);

	hbox = gtk_hbox_new (0, 2);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, 0, 0, 0);
	gtk_widget_show (hbox);

	button = gtkutil_button (hbox, GTK_STOCK_NEW, 0, editlist_gui_addnew,
									 0, _("Add New"));
	gtk_widget_set_usize (button, 100, 0);

	button = gtkutil_button (hbox, GTK_STOCK_DELETE, 0, editlist_gui_delete,
									 0, _("Delete"));
	gtk_widget_set_usize (button, 100, 0);

	button = gtk_vseparator_new ();
	gtk_container_add (GTK_CONTAINER (hbox), button);
	gtk_widget_show (button);

	button = gtkutil_button (hbox, GTK_STOCK_SORT_ASCENDING, 0, editlist_gui_sort,
									 0, _("Sort"));
	gtk_widget_set_usize (button, 100, 0);

	button = gtkutil_button (hbox, GTK_STOCK_HELP, 0, editlist_gui_help,
									 0, _("Help"));
	gtk_widget_set_usize (button, 100, 0);

	if (!help)
		gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);

	hbox = gtk_hbox_new (0, 2);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, 0, 0, 0);
	gtk_widget_show (hbox);

	editlist_gui_entry_name = gtk_entry_new_with_max_length (40);
	gtk_widget_set_usize (editlist_gui_entry_name, 96, 0);
	gtk_signal_connect (GTK_OBJECT (editlist_gui_entry_name), "changed",
							  GTK_SIGNAL_FUNC (editlist_gui_handle_name), 0);
	gtk_box_pack_start (GTK_BOX (hbox), editlist_gui_entry_name, 0, 0, 0);
	gtk_widget_show (editlist_gui_entry_name);

	editlist_gui_entry_cmd = gtk_entry_new_with_max_length (255);
	gtk_signal_connect (GTK_OBJECT (editlist_gui_entry_cmd), "changed",
							  GTK_SIGNAL_FUNC (editlist_gui_handle_cmd), 0);
	gtk_container_add (GTK_CONTAINER (hbox), editlist_gui_entry_cmd);
	gtk_widget_show (editlist_gui_entry_cmd);

	hbox = gtk_hbox_new (0, 2);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, 0, 0, 0);
	gtk_widget_show (hbox);

	editlist_gui_load (editlist_gui_list);

	gtk_widget_show (editlist_gui_window);
}
