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
#include <time.h>

#include "fe-gtk.h"

#include <gtk/gtkcheckbutton.h>
#include <gtk/gtktable.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkstock.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkhbbox.h>

#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../common/outbound.h"
#include "../common/util.h"
#include "../common/fe.h"
#include "../common/server.h"
#include "gtkutil.h"
#include "maingui.h"


/**
 * Accepts a regex_t pointer and string to test it with 
 * Returns 0 if no match, 1 if a match.
 */
#ifndef WIN32

static int
reg_match (server *serv, const char *str)
{
	int m;

	if (!serv->gui->have_regex)
		return 0;

	m = regexec (&serv->gui->chanlist_match_regex, str, 1, NULL, REG_NOTBOL);

	/* regex returns 0 if it's a match: */
	return m == 0 ? 1 : 0;
}

#else

static int
reg_match (server *serv, const char *str)
{
	return match (serv->gui->chanlist_match_regex, str);
}

#endif

/**
 * Sorts the channel list based upon the user field.
 */
static gint
chanlist_compare_user (GtkCList * clist,
							  gconstpointer ptr1, gconstpointer ptr2)
{
	int int1;
	int int2;

	GtkCListRow *row1 = (GtkCListRow *) ptr1;
	GtkCListRow *row2 = (GtkCListRow *) ptr2;

	int1 = atoi (GTK_CELL_TEXT (row1->cell[clist->sort_column])->text);
	int2 = atoi (GTK_CELL_TEXT (row2->cell[clist->sort_column])->text);

	return int1 > int2 ? 1 : -1;
}

/**
 * Provides the default case-insensitive sorting for the channel 
 * list.
 */
static gint
chanlist_compare_text_ignore_case (GtkCList * clist,
											  gconstpointer ptr1, gconstpointer ptr2)
{
	GtkCListRow *row1 = (GtkCListRow *) ptr1;
	GtkCListRow *row2 = (GtkCListRow *) ptr2;

	return rfc_casecmp (GTK_CELL_TEXT (row1->cell[clist->sort_column])->text,
							 GTK_CELL_TEXT (row2->cell[clist->sort_column])->text);
}

/**
 * Updates the caption to reflect the number of users and channels
 */
static void
chanlist_update_caption (struct server *serv)
{
	static gchar *title =
		N_("User and Channel Statistics: %d/%d Users on %d/%d Channels");

	gchar tbuf[256];

	snprintf (tbuf, sizeof tbuf, _(title),
				 serv->gui->chanlist_users_shown_count,
				 serv->gui->chanlist_users_found_count,
				 serv->gui->chanlist_channels_shown_count,
				 serv->gui->chanlist_channels_found_count);

	gtk_label_set_text (GTK_LABEL (serv->gui->chanlist_label), tbuf);
}

/**
 * Resets the various integer counters 
 */
static void
chanlist_reset_counters (struct server *serv)
{
	serv->gui->chanlist_users_found_count = 0;
	serv->gui->chanlist_users_shown_count = 0;
	serv->gui->chanlist_channels_found_count = 0;
	serv->gui->chanlist_channels_shown_count = 0;

	chanlist_update_caption (serv);
}

/**
 * Resets the vars that keep track of sort options.
 */
static void
chanlist_reset_sort_vars (struct server *serv)
{
	serv->gui->chanlist_sort_type = GTK_SORT_ASCENDING;
	serv->gui->chanlist_last_column = 0;
}

/**
 * Frees up the dynamic memory needed to store the channel information.
 */
static void
chanlist_data_free (struct server *serv)
{
	GSList *rows;
	gchar **data;

	if (serv->gui->chanlist_data_stored_rows)
	{

		for (rows = serv->gui->chanlist_data_stored_rows; rows != NULL;
			  rows = rows->next)
		{
			data = (gchar **) rows->data;
			free (data[0]);
			free (data[1]);
			free (data[2]);
			free (data);
		}

		g_slist_free (serv->gui->chanlist_data_stored_rows);
		serv->gui->chanlist_data_stored_rows = NULL;
	}
}

/**
 * Prepends a row of channel information to the chanlist_data_stored_rows 
 * GSList.
 */
static void
chanlist_data_prepend_row (struct server *serv, gchar ** next_row)
{
	serv->gui->chanlist_data_stored_rows =
		g_slist_prepend (serv->gui->chanlist_data_stored_rows, next_row);
}

/**
 * Places a data row into the gui GtkCList, if and only if the row matches
 * the user and regex requirements.
 */
static void
chanlist_place_row_in_gui (struct server *serv, gchar ** next_row)
{
	int num_users = atoi (next_row[1]);
	/*gfloat val, end;
	GtkAdjustment *adj;*/

	/* First, update the 'found' counter values */
	serv->gui->chanlist_users_found_count += num_users;
	serv->gui->chanlist_channels_found_count++;

	if (num_users < serv->gui->chanlist_minusers)
	{
		chanlist_update_caption (serv);
		return;
	}

	if (num_users > serv->gui->chanlist_maxusers
		 && serv->gui->chanlist_maxusers > 0)
	{
		chanlist_update_caption (serv);
		return;
	}

	if (serv->gui->chanlist_wild_text && serv->gui->chanlist_wild_text[0])
	{
		/* Check what the user wants to match. If both buttons or _neither_
		 * button is checked, look for match in both by default. 
		 */
		if ((serv->gui->chanlist_match_wants_channel
			  && serv->gui->chanlist_match_wants_topic)
			 ||
			 (!serv->gui->chanlist_match_wants_channel
			  && !serv->gui->chanlist_match_wants_topic))
		{
			if (!reg_match (serv, next_row[0])
				 && !reg_match (serv, next_row[2]))
			{
				chanlist_update_caption (serv);
				return;
			}
		}

		else if (serv->gui->chanlist_match_wants_channel)
		{
			if (!reg_match (serv, next_row[0]))
			{
				chanlist_update_caption (serv);
				return;
			}
		}

		else if (serv->gui->chanlist_match_wants_topic)
		{
			if (!reg_match (serv, next_row[2]))
			{
				chanlist_update_caption (serv);
				return;
			}
		}
	}

/*	adj = gtk_clist_get_vadjustment (GTK_CLIST (serv->gui->chanlist_list));
	val = adj->value;*/
	/*
	 * If all the above above tests passed or if no text was in the 
	 * chanlist_wild_text, add this entry to the GUI
	 */
	gtk_clist_prepend (GTK_CLIST (serv->gui->chanlist_list), next_row);

	/* Update the 'shown' counter values */
	serv->gui->chanlist_users_shown_count += num_users;
	serv->gui->chanlist_channels_shown_count++;

	/* restore original scrollbar position */
/*	end = adj->upper - adj->lower - adj->page_size;
	if (val > end)
		val = end;
	gtk_adjustment_set_value (adj, val);*/

	chanlist_update_caption (serv);
}

/**
 * Performs the actual refresh operations.
 */
static void
chanlist_do_refresh (struct server *serv)
{
	if (serv->connected)
	{
		chanlist_data_free (serv);
		chanlist_reset_counters (serv);
		chanlist_update_caption (serv);

		gtk_clist_clear (GTK_CLIST (serv->gui->chanlist_list));
		gtk_widget_set_sensitive (serv->gui->chanlist_refresh, FALSE);

		handle_command (serv->server_session, "list", FALSE);
	} else
		fe_message (_("Not connected."), FE_MSG_ERROR);
}

static void
chanlist_refresh (GtkWidget * wid, struct server *serv)
{
	/* JG NOTE: Didn't see actual use of wid here, so just forwarding
	 * this to chanlist_do_refresh because I use it without any widget
	 * param in chanlist_build_gui_list when the user presses enter
	 * or apply for the first time if the list has not yet been 
	 * received.
	 */
	chanlist_do_refresh (serv);
}

/**
 * Fills the gui GtkCList with stored items from the GSList.
 */
static void
chanlist_build_gui_list (struct server *serv)
{
	GSList *rows;
	GtkCList *clist;

	/* first check if the list is present */
	if (serv->gui->chanlist_data_stored_rows == NULL)
	{
		chanlist_do_refresh (serv);
		return;
	}

	clist = GTK_CLIST (serv->gui->chanlist_list);

	/* turn off sorting because this _greatly_ quickens the reinsertion */
	gtk_clist_set_auto_sort (clist, FALSE);
	/* freeze that GtkCList to make it go fasssster as well */
	gtk_clist_freeze (clist);
	gtk_clist_clear (clist);

	/* Reset the counters */
	chanlist_reset_counters (serv);

	/* Refill the list */
	for (rows = serv->gui->chanlist_data_stored_rows; rows != NULL;
		  rows = rows->next)
	{
		chanlist_place_row_in_gui (serv, (gchar **) rows->data);
	}

	gtk_clist_thaw (clist);
	gtk_clist_set_auto_sort (clist, TRUE);
	gtk_clist_sort (clist);
}

/**
 * Accepts incoming channel data from inbound.c, allocates new space for a
 * gchar**, forwards newly allocated row to chanlist_data_prepend_row
 * and chanlist_place_row_in_gui.
 */
void
fe_add_chan_list (struct server *serv, char *chan, char *users, char *topic)
{
	gchar **next_row;

	next_row = (gchar **) malloc (sizeof (gchar *) * 3);
	next_row[0] = strdup (chan);
	next_row[1] = strdup (users);
	next_row[2] = strip_color (topic, -1, STRIP_ALL);

	/* add this row to the data */
	chanlist_data_prepend_row (serv, next_row);

	/* _possibly_ add the row to the gui */
	chanlist_place_row_in_gui (serv, next_row);
}

void
fe_chan_list_end (struct server *serv)
{
	gtk_widget_set_sensitive (serv->gui->chanlist_refresh, TRUE);
}

/**
 * The next several functions simply handle signals from widgets to update 
 * the list and state variables. 
 */
static gboolean
chanlist_editable_keypress (GtkWidget * widget, GdkEventKey * event,
									 struct server *serv)
{
	if (event->keyval == GDK_Return)
		chanlist_build_gui_list (serv);

	return FALSE;
}

static void
chanlist_apply_pressed (GtkButton * button, struct server *serv)
{
	chanlist_build_gui_list (serv);
}

static void
chanlist_wild (GtkWidget * wid, struct server *serv)
{
	char *pattern = GTK_ENTRY (wid)->text;

	/* Store the pattern text in the wild_text var so next time the window is 
	 * opened it is remembered. 
	 */
	safe_strcpy (serv->gui->chanlist_wild_text, pattern,
					 sizeof (serv->gui->chanlist_wild_text));

#ifdef WIN32
	serv->gui->chanlist_match_regex = strdup (pattern);
#else
	/* recompile the regular expression. */
	if (serv->gui->have_regex)
	{
		serv->gui->have_regex = 0;
		regfree (&serv->gui->chanlist_match_regex);
	}

	if (regcomp (&serv->gui->chanlist_match_regex, pattern,
					 REG_ICASE | REG_EXTENDED | REG_NOSUB) == 0)
		serv->gui->have_regex = 1;
#endif
}

static void
chanlist_match_channel_button_toggled (GtkWidget * wid, struct server *serv)
{
	serv->gui->chanlist_match_wants_channel = GTK_TOGGLE_BUTTON (wid)->active;
}

static void
chanlist_match_topic_button_toggled (GtkWidget * wid, struct server *serv)
{
	serv->gui->chanlist_match_wants_topic = GTK_TOGGLE_BUTTON (wid)->active;
}

static void
chanlist_click_column (GtkWidget * clist, gint column, struct server *serv)
{
	/* If the user clicks the same column twice in a row,
	 * swap the sort type. Otherwise, assume he wishes
	 * to sort by another column, but using the same
	 * direction. 
	 */

	if (serv->gui->chanlist_last_column == column)
	{
		if (serv->gui->chanlist_sort_type == GTK_SORT_ASCENDING)
			serv->gui->chanlist_sort_type = GTK_SORT_DESCENDING;
		else
			serv->gui->chanlist_sort_type = GTK_SORT_ASCENDING;
	}

	serv->gui->chanlist_last_column = column;

	gtk_clist_set_sort_type (GTK_CLIST (clist), serv->gui->chanlist_sort_type);
	gtk_clist_set_sort_column (GTK_CLIST (clist), column);

	/* Since ascii sorting the numbers is a no go, use a custom
	 * sorter function for the 'users' column.
	 */
	if (column == 1)
		gtk_clist_set_compare_func (GTK_CLIST (clist), (GtkCListCompareFunc)
											 chanlist_compare_user);
	else
		/* In the 0 or 2 case, use the case-insensitive string 
		 * compare function. 
		 */
		gtk_clist_set_compare_func (GTK_CLIST (clist), (GtkCListCompareFunc)
											 chanlist_compare_text_ignore_case);

	gtk_clist_sort (GTK_CLIST (clist));
}

static void
chanlist_join (GtkWidget * wid, struct server *serv)
{
	int row;
	char *chan;
	char tbuf[CHANLEN + 6];

	row = gtkutil_clist_selection (serv->gui->chanlist_list);
	if (row != -1)
	{
		gtk_clist_get_text (GTK_CLIST (serv->gui->chanlist_list), row, 0,
								  &chan);
		if (serv->connected && (strcmp (chan, "*") != 0))
		{
			snprintf (tbuf, sizeof (tbuf), "join %s", chan);
			handle_command (serv->server_session, tbuf, FALSE);
		} else
			gdk_beep ();
	}
}

static void
chanlist_filereq_done (struct server *serv, char *file)
{
	time_t t = time (0);
	int i = 0;
	int fh;
	char *chan, *users, *topic;
	char buf[1024];

	if (!file)
		return;

	fh = open (file, O_TRUNC | O_WRONLY | O_CREAT, 0600);
	if (fh == -1)
		return;

	snprintf (buf, sizeof buf, "XChat Channel List: %s - %s\n",
				 serv->servername, ctime (&t));
	write (fh, buf, strlen (buf));

	while (1)
	{
		if (!gtk_clist_get_text
			 (GTK_CLIST (serv->gui->chanlist_list), i, 0, &chan))
			break;
		gtk_clist_get_text (GTK_CLIST (serv->gui->chanlist_list), i, 1, &users);
		gtk_clist_get_text (GTK_CLIST (serv->gui->chanlist_list), i, 2, &topic);
		i++;
		snprintf (buf, sizeof buf, "%-16s %-5s%s\n", chan, users, topic);
		write (fh, buf, strlen (buf));
	}

	close (fh);
}

static void
chanlist_save (GtkWidget * wid, struct server *serv)
{
	char *temp;

	if (!gtk_clist_get_text
		 (GTK_CLIST (serv->gui->chanlist_list), 0, 0, &temp))
	{
		fe_message (_("I can't save an empty list!"), FE_MSG_ERROR);
		return;
	}
	gtkutil_file_req (_("Select an output filename"), chanlist_filereq_done,
							serv, NULL, FRF_WRITE);
}

static void
chanlist_minusers (GtkSpinButton *wid, struct server *serv)
{
	serv->gui->chanlist_minusers = gtk_spin_button_get_value_as_int (wid);
}

static void
chanlist_maxusers (GtkSpinButton *wid, struct server *serv)
{
	serv->gui->chanlist_maxusers = gtk_spin_button_get_value_as_int (wid);
}

static void
chanlist_row_selected (GtkWidget * clist, gint row, gint column,
							  GdkEventButton * even, struct server *serv)
{
	if (even && even->type == GDK_2BUTTON_PRESS)
	{
		chanlist_join (0, (struct server *) serv);
	}
}

/**
 * Handles the window's destroy event to free allocated memory.
 */
static void
chanlist_destroy_widget (GtkObject * object, struct server *serv)
{
	chanlist_data_free (serv);
#ifndef WIN32
	if (serv->gui->have_regex)
	{
		regfree (&serv->gui->chanlist_match_regex);
		serv->gui->have_regex = 0;
	}
#else
	free (serv->gui->chanlist_match_regex);
#endif
}

static void
chanlist_closegui (GtkWidget *wid, server *serv)
{
	if (is_server (serv))
		serv->gui->chanlist_window = 0;
}

void
chanlist_opengui (struct server *serv, int do_refresh)
{
	gchar *titles[3];
	GtkWidget *frame, *vbox, *hbox, *table, *wid;
	char tbuf[256];

	titles[0] = _("Channel");
	titles[1] = _("Users");
	titles[2] = _("Topic");

	if (serv->gui->chanlist_window)
	{
		mg_bring_tofront (serv->gui->chanlist_window);
		return;
	}

	snprintf (tbuf, sizeof tbuf, _("XChat: Channel List (%s)"),
				 serv->servername);

	serv->gui->chanlist_data_stored_rows = NULL;

	if (!serv->gui->chanlist_minusers)
		serv->gui->chanlist_minusers = 3;

	if (!serv->gui->chanlist_maxusers)
		serv->gui->chanlist_maxusers = 9999;

	serv->gui->chanlist_window =
		mg_create_generic_tab ("ChanList", tbuf, FALSE, TRUE, chanlist_closegui,
								serv, 450, 356, &vbox, serv);

	frame = gtk_frame_new (_("List display options:"));
	gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
	gtk_widget_show (frame);

	table = gtk_table_new (6, 3, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 25);
	gtk_table_set_row_spacings (GTK_TABLE (table), 2);
	gtk_container_set_border_width (GTK_CONTAINER (table), 2);
	gtk_container_add (GTK_CONTAINER (frame), table);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 5);

	wid = gtk_label_new (_("Minimum Users:"));
	gtk_misc_set_alignment (GTK_MISC (wid), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), wid, 0, 1, 0, 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_widget_show (wid);

	wid = gtk_spin_button_new_with_range (0, 999999, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (wid),
										serv->gui->chanlist_minusers);
	g_signal_connect (G_OBJECT (wid), "value_changed",
							G_CALLBACK (chanlist_minusers), serv);
	gtk_table_attach (GTK_TABLE (table), wid, 1, 2, 0, 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_widget_show (wid);

	wid = gtk_label_new (_("Maximum Users:"));
	gtk_misc_set_alignment (GTK_MISC (wid), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), wid, 0, 1, 2, 3,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_widget_show (wid);

	wid = gtk_spin_button_new_with_range (0, 999999, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (wid),
										serv->gui->chanlist_maxusers);
	g_signal_connect (G_OBJECT (wid), "value_changed",
							G_CALLBACK (chanlist_maxusers), serv);
	gtk_table_attach (GTK_TABLE (table), wid, 1, 2, 2, 3,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_widget_show (wid);

#ifdef WIN32
	wid = gtk_label_new (_("Pattern Match:"));
#else
	wid = gtk_label_new (_("Regex Match:"));
#endif
	gtk_misc_set_alignment (GTK_MISC (wid), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), wid, 2, 3, 0, 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_widget_show (wid);

	wid = gtk_entry_new_with_max_length (255);
	gtk_widget_set_usize (wid, 155, 0);
	gtk_entry_set_text (GTK_ENTRY (wid), serv->gui->chanlist_wild_text);
	gtk_signal_connect (GTK_OBJECT (wid), "changed",
							  GTK_SIGNAL_FUNC (chanlist_wild), serv);
	gtk_signal_connect (GTK_OBJECT (wid), "key_press_event",
							  GTK_SIGNAL_FUNC (chanlist_editable_keypress),
							  (gpointer) serv);
	gtk_table_attach (GTK_TABLE (table), wid, 3, 6, 0, 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_widget_show (wid);
	serv->gui->chanlist_wild = wid;

	chanlist_wild (wid, serv);

	wid = gtk_label_new (_("Apply Match to:"));
	gtk_misc_set_alignment (GTK_MISC (wid), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), wid, 2, 3, 2, 3,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_widget_show (wid);

	wid = gtk_check_button_new_with_label (_("Channel"));
	gtk_signal_connect (GTK_OBJECT (wid), "toggled",
							  GTK_SIGNAL_FUNC
							  (chanlist_match_channel_button_toggled), serv);
	gtk_table_attach (GTK_TABLE (table), wid, 3, 4, 2, 3,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid), TRUE);
	gtk_widget_show (wid);

	wid = gtk_check_button_new_with_label (_("Topic"));
	gtk_signal_connect (GTK_OBJECT (wid), "toggled",
							  GTK_SIGNAL_FUNC (chanlist_match_topic_button_toggled),
							  serv);
	gtk_table_attach (GTK_TABLE (table), wid, 4, 5, 2, 3,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid), TRUE);
	gtk_widget_show (wid);

	wid = gtk_button_new_with_label (_("Apply"));
	gtk_table_attach (GTK_TABLE (table), wid, 7, 8, 2, 3,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK,  0, 0);
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (chanlist_apply_pressed),
							  (gpointer) serv);
	gtk_widget_show (wid);

	serv->gui->chanlist_list =
		gtkutil_clist_new (3, titles, vbox, GTK_POLICY_ALWAYS,
								 chanlist_row_selected, (gpointer) serv, 0, 0,
								 GTK_SELECTION_BROWSE);
	gtk_clist_set_column_width (GTK_CLIST (serv->gui->chanlist_list), 0, 90);
	gtk_clist_set_column_width (GTK_CLIST (serv->gui->chanlist_list), 1, 45);
	gtk_clist_set_column_width (GTK_CLIST (serv->gui->chanlist_list), 2, 165);
	gtk_clist_column_titles_active (GTK_CLIST (serv->gui->chanlist_list));
	gtk_signal_connect (GTK_OBJECT (serv->gui->chanlist_list), "click_column",
							  GTK_SIGNAL_FUNC (chanlist_click_column),
							  (gpointer) serv);
	gtk_clist_set_compare_func (GTK_CLIST (serv->gui->chanlist_list),
										 (GtkCListCompareFunc)
										 chanlist_compare_text_ignore_case);
	gtk_clist_set_sort_column (GTK_CLIST (serv->gui->chanlist_list), 0);
	gtk_clist_set_auto_sort (GTK_CLIST (serv->gui->chanlist_list), 1);
	/* makes the horiz. scrollbar appear when needed */
	gtk_clist_set_column_auto_resize (GTK_CLIST (serv->gui->chanlist_list),
											2, TRUE);

	/* make a label to store the user/channel info */
	wid = gtk_label_new ("");
	gtk_widget_show (wid);
	gtk_box_pack_start (GTK_BOX (vbox), wid, 0, 0, 0);

	serv->gui->chanlist_label = wid;

	hbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_SPREAD);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, 0, 0, 0);
	gtk_widget_show (hbox);

	serv->gui->chanlist_refresh =
		gtkutil_button (hbox, GTK_STOCK_REFRESH, 0, chanlist_refresh, serv,
							_("Refresh the list"));
	gtkutil_button (hbox, GTK_STOCK_SAVE_AS, 0, chanlist_save, serv,
						_("Save the list"));
	gtkutil_button (hbox, GTK_STOCK_JUMP_TO, 0, chanlist_join, serv,
						 _("Join Channel"));

	/* connect a destroy event handler to this window so that the dynamic
	   memory can be freed */
	gtk_signal_connect (GTK_OBJECT (serv->gui->chanlist_window), "destroy",
							  GTK_SIGNAL_FUNC (chanlist_destroy_widget),
							  (gpointer) serv);

	/* reset the sort vars and counters. */
	chanlist_reset_counters (serv);
	chanlist_reset_sort_vars (serv);

	gtk_widget_show (serv->gui->chanlist_window);

	if (do_refresh)
		chanlist_do_refresh (serv);
}
