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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "fe-gtk.h"

#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkhseparator.h>

#include "../common/xchat.h"
#include "../common/ignore.h"
#include "../common/cfgfiles.h"
#include "gtkutil.h"
#include "maingui.h"


static GtkWidget *ignorewin = 0;
static GtkWidget *ignorelist;
static GtkWidget *entry_mask;
static GtkWidget *entry_ctcp;
static GtkWidget *entry_private;
static GtkWidget *entry_channel;
static GtkWidget *entry_notice;
static GtkWidget *entry_invi;
static GtkWidget *entry_unignore;	/* these are toggles, not really entrys */

static GtkWidget *num_ctcp;
static GtkWidget *num_priv;
static GtkWidget *num_chan;
static GtkWidget *num_noti;
static GtkWidget *num_invi;

static void
ignore_save_clist_tomem (void)
{
	struct ignore *ignore;
	char *tmp;
	int i = 0;
	GSList *list;

	list = ignore_list;
	while (list)
	{
		ignore = (struct ignore *) list->data;
		ignore_list = g_slist_remove (ignore_list, ignore);
		free (ignore->mask);
		free (ignore);
		list = ignore_list;
	}

	while (1)
	{
		if (!gtk_clist_get_text (GTK_CLIST (ignorelist), i, 0, &tmp))
		{
			break;
		}
		ignore = malloc (sizeof (struct ignore));
		memset (ignore, 0, sizeof (struct ignore));
		ignore->mask = strdup (tmp);
		gtk_clist_get_text (GTK_CLIST (ignorelist), i, 1, &tmp);
		if (!strcmp (tmp, _("Yes")))
			ignore->type |= IG_CTCP;
		gtk_clist_get_text (GTK_CLIST (ignorelist), i, 2, &tmp);
		if (!strcmp (tmp, _("Yes")))
			ignore->type |= IG_PRIV;
		gtk_clist_get_text (GTK_CLIST (ignorelist), i, 3, &tmp);
		if (!strcmp (tmp, _("Yes")))
			ignore->type |= IG_CHAN;
		gtk_clist_get_text (GTK_CLIST (ignorelist), i, 4, &tmp);
		if (!strcmp (tmp, _("Yes")))
			ignore->type |= IG_NOTI;
		gtk_clist_get_text (GTK_CLIST (ignorelist), i, 5, &tmp);
		if (!strcmp (tmp, _("Yes")))
			ignore->type |= IG_INVI;
		gtk_clist_get_text (GTK_CLIST (ignorelist), i, 6, &tmp);
		if (!strcmp (tmp, _("Yes")))
			ignore->type |= IG_UNIG;
		ignore_list = g_slist_append (ignore_list, ignore);
		i++;
	}
}

static void
ignore_add_clist_entry (struct ignore *ignore)
{
	char *nnew[7];

	nnew[0] = ignore->mask;
	if (ignore->type & IG_CTCP)
		nnew[1] = _("Yes");
	else
		nnew[1] = _("No");
	if (ignore->type & IG_PRIV)
		nnew[2] = _("Yes");
	else
		nnew[2] = _("No");
	if (ignore->type & IG_CHAN)
		nnew[3] = _("Yes");
	else
		nnew[3] = _("No");
	if (ignore->type & IG_NOTI)
		nnew[4] = _("Yes");
	else
		nnew[4] = _("No");
	if (ignore->type & IG_INVI)
		nnew[5] = _("Yes");
	else
		nnew[5] = _("No");
	if (ignore->type & IG_UNIG)
		nnew[6] = _("Yes");
	else
		nnew[6] = _("No");

	gtk_clist_append (GTK_CLIST (ignorelist), nnew);
}

static void
ignore_sort_clicked (void)
{
	gtk_clist_sort (GTK_CLIST (ignorelist));
}

static void
ignore_delete_entry_clicked (GtkWidget * wid, struct session *sess)
{
	int row;

	row = gtkutil_clist_selection (ignorelist);
	if (row != -1)
	{
		gtk_clist_unselect_all (GTK_CLIST (ignorelist));
		gtk_clist_remove ((GtkCList *) ignorelist, row);
	}
}

static void
ignore_new_entry_clicked (GtkWidget * wid, struct session *sess)
{
	int i, row;
	gchar *nnew[7];
	/*
	   Serverlist copies an existing entry, but not much point to do so here
	 */
	nnew[0] = _("new!new@new.com");
	nnew[1] = _("No");
	nnew[2] = _("No");
	nnew[3] = _("No");
	nnew[4] = _("No");
	nnew[5] = _("No");
	nnew[6] = _("No");
	row = gtkutil_clist_selection (ignorelist);
	i = gtk_clist_insert (GTK_CLIST (ignorelist), row + 1, nnew);
	gtk_clist_select_row (GTK_CLIST (ignorelist), i, 0);
	gtk_clist_moveto (GTK_CLIST (ignorelist), i, 0, 0.5, 0);
}

static void
ignore_check_state_toggled (GtkWidget * wid, int row, int col)
{
	char *text;
	int state;

	gtk_clist_get_text (GTK_CLIST (ignorelist), row, col, &text);
	if (!strcmp (text, _("Yes")))
		state = 1;
	else
		state = 0;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid), state);
}

static void
ignore_row_unselected (GtkWidget * clist, int row, int column,
							  GdkEventButton * even)
{
	gtk_entry_set_text (GTK_ENTRY (entry_mask), "");
}

static void
ignore_row_selected (GtkWidget * clist, int row, int column,
							GdkEventButton * even, gpointer sess)
{
	char *mask;
	row = gtkutil_clist_selection (ignorelist);
	if (row != -1)
	{
		gtk_clist_get_text (GTK_CLIST (ignorelist), row, 0, &mask);
		mask = strdup (mask);
		gtk_entry_set_text (GTK_ENTRY (entry_mask), mask);
		free (mask);
		ignore_check_state_toggled (entry_ctcp, row, 1);
		ignore_check_state_toggled (entry_private, row, 2);
		ignore_check_state_toggled (entry_channel, row, 3);
		ignore_check_state_toggled (entry_notice, row, 4);
		ignore_check_state_toggled (entry_invi, row, 5);
		ignore_check_state_toggled (entry_unignore, row, 6);
	} else
		ignore_row_unselected (0, 0, 0, 0);
}

static void
ignore_handle_mask (GtkWidget * igad)
{
	int row;
	const char *mask;

	row = gtkutil_clist_selection (ignorelist);
	if (row != -1)
	{
		mask = gtk_entry_get_text ((GtkEntry *) igad);
		gtk_clist_set_text ((GtkCList *) ignorelist, row, 0, mask);
	}
}

static void
ignore_ctcp_toggled (GtkWidget * igad, gpointer serv)
{
	int row;
	row = gtkutil_clist_selection (ignorelist);
	if (GTK_TOGGLE_BUTTON (igad)->active)
		gtk_clist_set_text ((GtkCList *) ignorelist, row, 1, _("Yes"));
	else
		gtk_clist_set_text ((GtkCList *) ignorelist, row, 1, _("No"));
}

static void
ignore_private_toggled (GtkWidget * igad, gpointer serv)
{
	int row;
	row = gtkutil_clist_selection (ignorelist);
	if (GTK_TOGGLE_BUTTON (igad)->active)
		gtk_clist_set_text ((GtkCList *) ignorelist, row, 2, _("Yes"));
	else
		gtk_clist_set_text ((GtkCList *) ignorelist, row, 2, _("No"));
}

static void
ignore_channel_toggled (GtkWidget * igad, gpointer serv)
{
	int row;
	row = gtkutil_clist_selection (ignorelist);
	if (GTK_TOGGLE_BUTTON (igad)->active)
		gtk_clist_set_text ((GtkCList *) ignorelist, row, 3, _("Yes"));
	else
		gtk_clist_set_text ((GtkCList *) ignorelist, row, 3, _("No"));
}

static void
ignore_notice_toggled (GtkWidget * igad, gpointer serv)
{
	int row;
	row = gtkutil_clist_selection (ignorelist);
	if (GTK_TOGGLE_BUTTON (igad)->active)
		gtk_clist_set_text ((GtkCList *) ignorelist, row, 4, _("Yes"));
	else
		gtk_clist_set_text ((GtkCList *) ignorelist, row, 4, _("No"));
}

static void
ignore_invi_toggled (GtkWidget * igad, gpointer serv)
{
	int row;
	row = gtkutil_clist_selection (ignorelist);
	if (GTK_TOGGLE_BUTTON (igad)->active)
		gtk_clist_set_text ((GtkCList *) ignorelist, row, 5, _("Yes"));
	else
		gtk_clist_set_text ((GtkCList *) ignorelist, row, 5, _("No"));
}

static void
ignore_unignore_toggled (GtkWidget * igad, gpointer serv)
{
	int row;
	row = gtkutil_clist_selection (ignorelist);
	if (GTK_TOGGLE_BUTTON (igad)->active)
		gtk_clist_set_text ((GtkCList *) ignorelist, row, 6, _("Yes"));
	else
		gtk_clist_set_text ((GtkCList *) ignorelist, row, 6, _("No"));
}

static int
close_ignore_list ()
{
	ignore_save_clist_tomem ();
	ignore_save ();
	gtk_widget_destroy (ignorewin);
	return 0;
}

static void
close_ignore_gui_callback ()
{
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
	gtk_widget_set_usize (wid, 30, 0);
	gtk_editable_set_editable (GTK_EDITABLE (wid), FALSE);
	gtk_entry_set_text (GTK_ENTRY (wid), buf);

	return wid;
}

void
ignore_gui_open ()
{
	gchar *titles[] =
		{ _("Mask"), _("CTCP"), _("Private"), _("Chan"), _("Notice"), _("Invite"), _("Unignore") };
	GtkWidget *vbox, *box, *wid, *stat_box, *frame;
	GSList *temp = ignore_list;

	if (ignorewin)
	{
		mg_bring_tofront (ignorewin);
		return;
	}

	ignorewin =
			  mg_create_generic_tab ("ignorelist", _("X-Chat: Ignore list"),
											FALSE, TRUE, close_ignore_gui_callback,
											NULL, 0, 0, &vbox, 0);

	ignorelist = gtkutil_clist_new (7, titles, vbox, GTK_POLICY_ALWAYS,
											  ignore_row_selected, 0,
											  ignore_row_unselected, 0,
											  GTK_SELECTION_BROWSE);

	gtk_widget_set_usize (ignorelist, 500, 150);

	gtk_clist_set_column_width (GTK_CLIST (ignorelist), 0, 196);
	gtk_clist_set_column_width (GTK_CLIST (ignorelist), 1, 40);
	gtk_clist_set_column_width (GTK_CLIST (ignorelist), 2, 40);
	gtk_clist_set_column_width (GTK_CLIST (ignorelist), 3, 40);
	gtk_clist_set_column_width (GTK_CLIST (ignorelist), 4, 40);
	gtk_clist_set_column_width (GTK_CLIST (ignorelist), 5, 40);

	box = gtk_hbox_new (0, 0);
	gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, TRUE, 5);
	gtk_widget_show (box);

	gtkutil_label_new (_("Ignore Mask:"), box);
	entry_mask = gtkutil_entry_new (99, box, ignore_handle_mask, 0);

	box = gtk_hbox_new (0, 0);
	gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, TRUE, 5);
	gtk_widget_show (box);

	entry_ctcp = gtk_check_button_new_with_label (_("CTCP"));
	gtk_signal_connect (GTK_OBJECT (entry_ctcp), "toggled",
							  GTK_SIGNAL_FUNC (ignore_ctcp_toggled), 0);
	gtk_container_add (GTK_CONTAINER (box), entry_ctcp);
	gtk_widget_show (entry_ctcp);

	entry_private = gtk_check_button_new_with_label (_("Private"));
	gtk_signal_connect (GTK_OBJECT (entry_private), "toggled",
							  GTK_SIGNAL_FUNC (ignore_private_toggled), 0);
	gtk_container_add (GTK_CONTAINER (box), entry_private);
	gtk_widget_show (entry_private);

	entry_channel = gtk_check_button_new_with_label (_("Channel"));
	gtk_signal_connect (GTK_OBJECT (entry_channel), "toggled",
							  GTK_SIGNAL_FUNC (ignore_channel_toggled), 0);
	gtk_container_add (GTK_CONTAINER (box), entry_channel);
	gtk_widget_show (entry_channel);

	entry_notice = gtk_check_button_new_with_label (_("Notice"));
	gtk_signal_connect (GTK_OBJECT (entry_notice), "toggled",
							  GTK_SIGNAL_FUNC (ignore_notice_toggled), 0);
	gtk_container_add (GTK_CONTAINER (box), entry_notice);
	gtk_widget_show (entry_notice);

	entry_invi = gtk_check_button_new_with_label (_("Invite"));
	gtk_signal_connect (GTK_OBJECT (entry_invi), "toggled",
							  GTK_SIGNAL_FUNC (ignore_invi_toggled), 0);
	gtk_container_add (GTK_CONTAINER (box), entry_invi);
	gtk_widget_show (entry_invi);

	entry_unignore = gtk_check_button_new_with_label (_("Unignore"));
	gtk_signal_connect (GTK_OBJECT (entry_unignore), "toggled",
							  GTK_SIGNAL_FUNC (ignore_unignore_toggled), 0);
	gtk_container_add (GTK_CONTAINER (box), entry_unignore);
	gtk_widget_show (entry_unignore);

	wid = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (vbox), wid, FALSE, FALSE, 8);
	gtk_widget_show (wid);

	frame = gtk_frame_new (_("Ignore Stats:"));
	gtk_widget_show (frame);

	stat_box = gtk_hbox_new (0, 2);
	gtk_container_set_border_width (GTK_CONTAINER (stat_box), 6);
	gtk_container_add (GTK_CONTAINER (frame), stat_box);
	gtk_widget_show (stat_box);

	num_ctcp = ignore_stats_entry (stat_box, _("CTCP:"), ignored_ctcp);
	num_priv = ignore_stats_entry (stat_box, _("Private:"), ignored_priv);
	num_chan = ignore_stats_entry (stat_box, _("Channel:"), ignored_chan);
	num_noti = ignore_stats_entry (stat_box, _("Notice:"), ignored_noti);
	num_invi = ignore_stats_entry (stat_box, _("Invite:"), ignored_invi);

	gtk_container_add (GTK_CONTAINER (vbox), frame);

	box = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (box), GTK_BUTTONBOX_SPREAD);
	gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 2);
	gtk_widget_show (box);

	gtkutil_button (box, GTK_STOCK_OK, 0, close_ignore_list, 0, _("OK"));
	gtkutil_button (box, GTK_STOCK_NEW, 0, ignore_new_entry_clicked, 0,
						 _("New"));
	gtkutil_button (box, GTK_STOCK_CLOSE, 0, ignore_delete_entry_clicked,
						 0, ("Delete"));
	gtkutil_button (box, GTK_STOCK_SPELL_CHECK, 0, ignore_sort_clicked,
						 0, _("Sort"));
	gtkutil_button (box, GTK_STOCK_CANCEL, 0, gtkutil_destroy, ignorewin,
						 _("Cancel"));

	while (temp)
	{
		ignore_add_clist_entry ((struct ignore *) temp->data);
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
