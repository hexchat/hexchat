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
#include <unistd.h>
#include <fcntl.h>
#include "fe-gtk.h"

#include <gtk/gtkbutton.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtktooltips.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkspinbutton.h>

#include "../common/xchat.h"
#include "gtkutil.h"
#include "pixmaps.h"

/* gtkutil.c, just some gtk wrappers */

extern void path_part (char *file, char *path, int pathlen);


struct file_req
{
	GtkWidget *dialog;
	void *userdata;
	void *userdata2;
	filereqcallback callback;
	int write;
};

static char last_dir[256] = "";


GtkWidget *
gtkutil_simpledialog (char *msg)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
						 "%s", msg);
	g_signal_connect (G_OBJECT (dialog), "response",
							G_CALLBACK (gtk_widget_destroy), 0);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
	gtk_widget_show (dialog);

	return dialog;
}

static void
gtkutil_file_req_cancel (GtkWidget * wid, struct file_req *freq)
{
	gtk_widget_destroy (freq->dialog);
	freq->callback (freq->userdata, freq->userdata2, 0);
	free (freq);
}

static void
gtkutil_file_req_done (GtkWidget * wid, struct file_req *freq)
{
	struct stat st;
	int axs = FALSE;
	char *file;
	const char *f;

	f = gtk_file_selection_get_filename (GTK_FILE_SELECTION (freq->dialog));
	file = malloc (strlen (f) + 2);
	strcpy (file, f);
	path_part ((char *)f, last_dir, sizeof (last_dir));

	if (stat (file, &st) != -1)
	{
		if (S_ISDIR (st.st_mode))
		{
			if (file[strlen(file)-1] != '/')
				strcat (file, "/");
			gtk_file_selection_set_filename (GTK_FILE_SELECTION (freq->dialog),
														file);
			free (file);
			return;
		}
	}
	if (freq->write)
	{
		if (access (last_dir, W_OK) == 0)
			axs = TRUE;
	} else
	{
		if (stat (file, &st) != -1)
		{
			if (!S_ISDIR (st.st_mode))
				axs = TRUE;
		}
	}

	if (axs)
	{
		char *utf8_file;
		/* convert to UTF8. It might be converted back to locale by
			server.c's g_convert */
		utf8_file = g_filename_to_utf8 (file, -1, NULL, NULL, NULL);
		freq->callback (freq->userdata, freq->userdata2, utf8_file);
		g_free (utf8_file);
	} else
	{
		if (freq->write)
			gtkutil_simpledialog (_("Cannot write to that file."));
		else
			gtkutil_simpledialog (_("Cannot read that file."));
	}

	free (file);
	gtk_widget_destroy (freq->dialog);
	free (freq);
}

void
gtkutil_file_req (char *title, void *callback, void *userdata,
						void *userdata2, int write)
{
	struct file_req *freq;
	GtkWidget *dialog;

	dialog = gtk_file_selection_new (title);
	if (last_dir[0])
		gtk_file_selection_set_filename (GTK_FILE_SELECTION (dialog), last_dir);

	freq = malloc (sizeof (struct file_req));
	freq->dialog = dialog;
	freq->write = write;
	freq->callback = callback;
	freq->userdata = userdata;
	freq->userdata2 = userdata2;

	g_signal_connect (G_OBJECT
							  (GTK_FILE_SELECTION (dialog)->cancel_button),
							  "clicked", G_CALLBACK (gtkutil_file_req_cancel),
							  (gpointer) freq);
	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (dialog)->ok_button),
							  "clicked", G_CALLBACK (gtkutil_file_req_done),
							  (gpointer) freq);
	gtk_widget_show (dialog);
}

void
gtkutil_destroy (GtkWidget * igad, GtkWidget * dgad)
{
	gtk_widget_destroy (dgad);
}

static void
gtkutil_get_str_response (GtkDialog *dialog, gint arg1, gpointer entry)
{
	void (*callback) (int cancel, char *text, void *user_data);
	char *text;
	void *user_data;

	text = (char *) gtk_entry_get_text (GTK_ENTRY (entry));
	callback = g_object_get_data (G_OBJECT (dialog), "cb");
	user_data = g_object_get_data (G_OBJECT (dialog), "ud");

	switch (arg1)
	{
	case GTK_RESPONSE_REJECT:
		callback (TRUE, text, user_data);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	case GTK_RESPONSE_ACCEPT:
		callback (FALSE, text, user_data);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	}
}

static void
gtkutil_str_enter (GtkWidget *entry, GtkWidget *dialog)
{
	gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
}

void
fe_get_str (char *msg, char *def, void *callback, void *userdata)
{
	GtkWidget *dialog;
	GtkWidget *entry;
	GtkWidget *hbox;
	GtkWidget *label;

	dialog = gtk_dialog_new_with_buttons (msg, NULL, 0,
										GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
										GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
										NULL);
	gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->vbox), TRUE);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
	hbox = gtk_hbox_new (TRUE, 0);

	g_object_set_data (G_OBJECT (dialog), "cb", callback);
	g_object_set_data (G_OBJECT (dialog), "ud", userdata);

	entry = gtk_entry_new ();
	g_signal_connect (G_OBJECT (entry), "activate",
						 	G_CALLBACK (gtkutil_str_enter), dialog);
	gtk_entry_set_text (GTK_ENTRY (entry), def);
	gtk_box_pack_end (GTK_BOX (hbox), entry, 0, 0, 0);

	label = gtk_label_new (msg);
	gtk_box_pack_end (GTK_BOX (hbox), label, 0, 0, 0);

	g_signal_connect (G_OBJECT (dialog), "response",
						   G_CALLBACK (gtkutil_get_str_response), entry);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

	gtk_widget_show_all (dialog);
}

static void
gtkutil_get_number_response (GtkDialog *dialog, gint arg1, gpointer spin)
{
	void (*callback) (int cancel, int value, void *user_data);
	int num;
	void *user_data;

	num = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spin));
	callback = g_object_get_data (G_OBJECT (dialog), "cb");
	user_data = g_object_get_data (G_OBJECT (dialog), "ud");

	switch (arg1)
	{
	case GTK_RESPONSE_REJECT:
		callback (TRUE, num, user_data);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	case GTK_RESPONSE_ACCEPT:
		callback (FALSE, num, user_data);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	}
}

void
fe_get_int (char *msg, int def, void *callback, void *userdata)
{
	GtkWidget *dialog;
	GtkWidget *spin;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkAdjustment *adj;

	dialog = gtk_dialog_new_with_buttons (msg, NULL, 0,
										GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
										GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
										NULL);
	gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->vbox), TRUE);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
	hbox = gtk_hbox_new (TRUE, 0);

	g_object_set_data (G_OBJECT (dialog), "cb", callback);
	g_object_set_data (G_OBJECT (dialog), "ud", userdata);

	spin = gtk_spin_button_new (NULL, 1, 0);
	adj = gtk_spin_button_get_adjustment ((GtkSpinButton*)spin);
	adj->lower = 0;
	adj->upper = 1024;
	adj->step_increment = 1;
	gtk_adjustment_changed (adj);
	gtk_spin_button_set_value ((GtkSpinButton*)spin, def);
	gtk_box_pack_end (GTK_BOX (hbox), spin, 0, 0, 0);

	label = gtk_label_new (msg);
	gtk_box_pack_end (GTK_BOX (hbox), label, 0, 0, 0);

	g_signal_connect (G_OBJECT (dialog), "response",
						   G_CALLBACK (gtkutil_get_number_response), spin);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

	gtk_widget_show_all (dialog);
}

GtkWidget *
gtkutil_button (GtkWidget *box, char *stock, char *tip, void *callback,
				 void *userdata, char *labeltext)
{
	GtkWidget *wid, *img, *label, *bbox;

	wid = gtk_button_new ();
	g_signal_connect (G_OBJECT (wid), "clicked",
							G_CALLBACK (callback), userdata);

	bbox = gtk_hbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (wid), bbox);
	gtk_widget_show (bbox);

	if (stock)
	{
		img = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_MENU);
		gtk_widget_set_usize (img, 8, 6);
		gtk_container_add (GTK_CONTAINER (bbox), img);
		gtk_widget_show (img);
	}

	if (labeltext)
	{
		label = gtk_label_new_with_mnemonic (labeltext);
		gtk_container_add (GTK_CONTAINER (bbox), label);
		gtk_widget_show (label);
		gtk_container_add (GTK_CONTAINER (box), wid);
	} else
	{
		gtk_box_pack_start (GTK_BOX (box), wid, 0, 0, 0);
	}

	gtk_widget_show (wid);
	if (tip)
		add_tip (wid, tip);

	return wid;
}

void
gtkutil_label_new (char *text, GtkWidget * box)
{
	GtkWidget *label = gtk_label_new (text);
	gtk_container_add (GTK_CONTAINER (box), label);
	gtk_widget_show (label);
}

GtkWidget *
gtkutil_entry_new (int max, GtkWidget * box, void *callback,
						 gpointer userdata)
{
	GtkWidget *entry = gtk_entry_new_with_max_length (max);
	gtk_container_add (GTK_CONTAINER (box), entry);
	if (callback)
		g_signal_connect (G_OBJECT (entry), "changed",
								G_CALLBACK (callback), userdata);
	gtk_widget_show (entry);
	return entry;
}

GtkWidget *
gtkutil_clist_new (int columns, char *titles[],
						 GtkWidget * box, int policy,
						 void *select_callback, gpointer select_userdata,
						 void *unselect_callback,
						 gpointer unselect_userdata, int selection_mode)
{
	GtkWidget *clist, *win;

	win = gtk_scrolled_window_new (0, 0);
	gtk_container_add (GTK_CONTAINER (box), win);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win),
											  GTK_POLICY_AUTOMATIC, policy);
	gtk_widget_show (win);

	if (titles)
		clist = gtk_clist_new_with_titles (columns, titles);
	else
		clist = gtk_clist_new (columns);

	gtk_clist_set_selection_mode (GTK_CLIST (clist), selection_mode);
	gtk_clist_column_titles_passive (GTK_CLIST (clist));
	gtk_container_add (GTK_CONTAINER (win), clist);
	if (select_callback)
	{
		g_signal_connect (G_OBJECT (clist), "select_row",
								G_CALLBACK (select_callback), select_userdata);
	}
	if (unselect_callback)
	{
		g_signal_connect (G_OBJECT (clist), "unselect_row",
								G_CALLBACK (unselect_callback), unselect_userdata);
	}
	gtk_widget_show (clist);

	return clist;
}

int
gtkutil_clist_selection (GtkWidget * clist)
{
	if (GTK_CLIST (clist)->selection)
		return GPOINTER_TO_INT(GTK_CLIST (clist)->selection->data);
	return -1;
}

void
add_tip (GtkWidget * wid, char *text)
{
	GtkTooltips *tip = gtk_tooltips_new ();
	gtk_tooltips_set_tip (tip, wid, text, 0);
}

void
show_and_unfocus (GtkWidget * wid)
{
	GTK_WIDGET_UNSET_FLAGS (wid, GTK_CAN_FOCUS);
	gtk_widget_show (wid);
}

void
gtkutil_set_icon (GtkWidget *win)
{
	gtk_window_set_icon (GTK_WINDOW (win), pix_xchat);
}

GtkWidget *
gtkutil_window_new (char *title, int width, int height, int mouse_pos)
{
	GtkWidget *win;

	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtkutil_set_icon (win);
	gtk_window_set_title (GTK_WINDOW (win), title);
	gtk_window_set_default_size (GTK_WINDOW (win), width, height);
	if (mouse_pos)
		gtk_window_set_position (GTK_WINDOW (win), GTK_WIN_POS_MOUSE);

	return win;
}
