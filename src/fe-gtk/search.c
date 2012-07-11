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
#include <string.h>
#include <stdlib.h>

#include "fe-gtk.h"

#include <gtk/gtkentry.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtkvseparator.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtktogglebutton.h>
#include <gdk/gdkkeysyms.h>

#include "../common/xchat.h"
#include "../common/fe.h"
#include "../common/util.h"
#include "../common/xchatc.h"
#include "gtkutil.h"
#include "xtext.h"
#include "maingui.h"

GtkWidget *searchwin;

static void
search_search (session * sess, const gchar *text)
{
	gtk_xtext_search_flags flags;
	textentry *last;
	GError *err = NULL;

	flags = ((prefs.text_search_case_match == 1? case_match: 0) |
				(prefs.text_search_backward == 1? backward: 0) |
				(prefs.text_search_highlight_all == 1? highlight: 0) |
				(prefs.text_search_follow == 1? follow: 0) |
				(prefs.text_search_regexp == 1? regexp: 0));
	if (!is_session (sess))
	{
		fe_message (_("The window you opened this Search "
						"for doesn't exist anymore."), FE_MSG_ERROR);
		return;
	}

	last = gtk_xtext_search (GTK_XTEXT (sess->gui->xtext), text, flags, &err);
	if (text == NULL || text[0] == 0)
	{
		return;
	}
	if (err)
	{
		fe_message (_(err->message), FE_MSG_ERROR);
		g_error_free (err);
	}
	else if (!last)
	{
		fe_message (_("Search hit end, not found."), FE_MSG_ERROR);
	}
}

static void
search_find_cb (GtkWidget * button, session * sess)
{
	GtkEntry *entry;
	const gchar *text;

	entry = g_object_get_data (G_OBJECT (button), "e");
	text = gtk_entry_get_text (entry);
	search_search (sess, text);
}

static void
search_close_cb (GtkWidget * button, GtkWidget * win)
{
	gtk_widget_destroy (win);
	searchwin = NULL;
}

static void
search_reset_cb (GtkWidget * button, session * sess)
{
	search_search (sess, "");
	if (searchwin)
	{
		search_close_cb (button, searchwin);
	}
}

static void
search_cleanup_cb (GtkWidget * button, GtkWidget * win)
{
	searchwin = NULL;
}

static void
search_entry_cb (GtkWidget * entry, session * sess)
{
	search_search (sess, gtk_entry_get_text (GTK_ENTRY (entry)));
}

static gboolean 
search_key_cb (GtkWidget * window, GdkEventKey * key, gpointer userdata)
{
	if (key->keyval == GDK_Escape)
		gtk_widget_destroy (window);
	return FALSE;
}

static void
search_caseign_cb (GtkToggleButton * but, session * sess)
{
	prefs.text_search_case_match = (but->active)? 1: 0;
}

static void
search_dirbwd_cb (GtkToggleButton * but, session * sess)
{
	prefs.text_search_backward = (but->active)? 1: 0;
}

static void
search_regexp_cb (GtkToggleButton * but, session * sess)
{
	prefs.text_search_regexp = (but->active)? 1: 0;
}

static void
search_highlight_cb (GtkToggleButton * but, session * sess)
{
	prefs.text_search_highlight_all = (but->active)? 1: 0;
	search_search (sess, NULL);
}

void
search_open (session * sess)
{
	GtkWidget *win, *hbox, *vbox, *entry, *wid;

	if (searchwin)
	{
		gtk_widget_destroy (searchwin);
		searchwin = NULL;
	}
	win = mg_create_generic_tab ("search", _("XChat: Search"), TRUE, FALSE,
								 search_cleanup_cb, NULL, 0, 0, &vbox, 0);
	gtk_container_set_border_width (GTK_CONTAINER (win), 12);
	gtk_box_set_spacing (GTK_BOX (vbox), 4);

	/* First line:  _____________________   _Find */
	hbox = gtk_hbox_new (0, 10);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);
	gtk_widget_show (hbox);

	entry = gtk_entry_new ();
	g_signal_connect (G_OBJECT (entry), "activate",
							G_CALLBACK (search_entry_cb), sess);
	gtk_container_add (GTK_CONTAINER (hbox), entry);
	gtk_widget_show (entry);
	gtk_widget_grab_focus (entry);

	wid = gtk_hbutton_box_new ();
	gtk_container_add (GTK_CONTAINER (hbox), wid);
	gtk_widget_show (wid);
	wid = gtkutil_button (wid, GTK_STOCK_FIND, 0, search_find_cb, sess,
								 _("_Find"));
	g_object_set_data (G_OBJECT (wid), "e", entry);

	/* Second line:  X Match case */
	wid = gtk_check_button_new_with_mnemonic (_("_Match case"));
	GTK_TOGGLE_BUTTON (wid)->active = prefs.text_search_case_match;
	g_signal_connect (G_OBJECT (wid), "toggled", G_CALLBACK (search_caseign_cb), sess);
	gtk_container_add (GTK_CONTAINER (vbox), wid);
	add_tip (wid, "Perform a case-sensitive search.");
	gtk_widget_show (wid);

	/* Third line:  X Search backwards */
	wid = gtk_check_button_new_with_mnemonic (_("Search _backwards"));
	GTK_TOGGLE_BUTTON (wid)->active = prefs.text_search_backward;
	g_signal_connect (G_OBJECT (wid), "toggled", G_CALLBACK (search_dirbwd_cb), sess);
	gtk_container_add (GTK_CONTAINER (vbox), wid);
	add_tip (wid, "Search from the newest text line to the oldest.");
	gtk_widget_show (wid);

	/* Fourth line:  X Highlight all */
	wid = gtk_check_button_new_with_mnemonic (_("_Highlight all"));
	GTK_TOGGLE_BUTTON (wid)->active = prefs.text_search_highlight_all;
	g_signal_connect (G_OBJECT (wid), "toggled", G_CALLBACK (search_highlight_cb), sess);
	gtk_container_add (GTK_CONTAINER (vbox), wid);
	add_tip (wid, "Highlight all occurrences, and underline the current occurrence.");
	gtk_widget_show (wid);

	/* Fifth line:  X Regular expression */
	wid = gtk_check_button_new_with_mnemonic (_("R_egular expression"));
	GTK_TOGGLE_BUTTON (wid)->active = prefs.text_search_regexp;
	g_signal_connect (G_OBJECT (wid), "toggled", G_CALLBACK (search_regexp_cb), sess);
	gtk_container_add (GTK_CONTAINER (vbox), wid);
	add_tip (wid, "Regard search string as a regular expression.");
	gtk_widget_show (wid);

	/* Sixth line:  _Close    Close and _Reset */
	hbox = gtk_hbutton_box_new ();
	gtk_box_pack_start (GTK_BOX (vbox), hbox, 0, 0, 4);
	gtk_widget_show (hbox);

	wid = gtkutil_button (hbox, GTK_STOCK_CLOSE, 0, search_close_cb, win,
						_("_Close"));
	add_tip (wid, "Close this box, but continue searching new lines.");
	wid = gtkutil_button (hbox, "gtk-reset", 0, search_reset_cb, sess,
						_("Close and _Reset"));
	add_tip (wid, "Close this box, reset highlighted search items, and stop searching new lines.");

	/* Add recognition of the ESC key to close the box */
	g_signal_connect (G_OBJECT (win), "key_press_event", G_CALLBACK (search_key_cb), win);

	/* That's all, folks */
	searchwin = win;
	gtk_widget_show (win);
}
