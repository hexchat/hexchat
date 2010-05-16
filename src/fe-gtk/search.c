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


static textentry *last;	/* our last search pos */
static int case_match = 0;
static int search_backward = 0;


static void
search_search (session * sess, const gchar *text)
{
	if (!is_session (sess))
	{
		fe_message (_("The window you opened this Search "
						"for doesn't exist anymore."), FE_MSG_ERROR);
		return;
	}

	last = gtk_xtext_search (GTK_XTEXT (sess->gui->xtext), text,
									 last, case_match, search_backward);
	if (!last)
		fe_message (_("Search hit end, not found."), FE_MSG_ERROR);
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
	case_match = (but->active)? 1: 0;
}

static void
search_dirbwd_cb (GtkToggleButton * but, session * sess)
{
	search_backward = (but->active)? 1: 0;
}

void
search_open (session * sess)
{
	GtkWidget *win, *hbox, *vbox, *entry, *wid;

	last = NULL;
	win = mg_create_generic_tab ("search", _("XChat: Search"), TRUE, FALSE,
								 NULL, NULL, 0, 0, &vbox, 0);
	gtk_container_set_border_width (GTK_CONTAINER (win), 12);
	gtk_box_set_spacing (GTK_BOX (vbox), 4);

	hbox = gtk_hbox_new (0, 10);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);
	gtk_widget_show (hbox);

	gtkutil_label_new (_("Find:"), hbox);

	entry = gtk_entry_new ();
	g_signal_connect (G_OBJECT (entry), "activate",
							G_CALLBACK (search_entry_cb), sess);
	gtk_container_add (GTK_CONTAINER (hbox), entry);
	gtk_widget_show (entry);
	gtk_widget_grab_focus (entry);

	wid = gtk_check_button_new_with_mnemonic (_("_Match case"));
	GTK_TOGGLE_BUTTON (wid)->active = case_match;
	g_signal_connect (G_OBJECT (wid), "toggled", G_CALLBACK (search_caseign_cb), sess);
	gtk_container_add (GTK_CONTAINER (vbox), wid);
	gtk_widget_show (wid);

	wid = gtk_check_button_new_with_mnemonic (_("Search _backwards"));
	GTK_TOGGLE_BUTTON (wid)->active = search_backward;
	g_signal_connect (G_OBJECT (wid), "toggled", G_CALLBACK (search_dirbwd_cb), sess);
	gtk_container_add (GTK_CONTAINER (vbox), wid);
	gtk_widget_show (wid);

	hbox = gtk_hbutton_box_new ();
	gtk_box_pack_start (GTK_BOX (vbox), hbox, 0, 0, 4);
	gtk_widget_show (hbox);

	gtkutil_button (hbox, GTK_STOCK_CLOSE, 0, search_close_cb, win,
						_("_Close"));
	wid = gtkutil_button (hbox, GTK_STOCK_FIND, 0, search_find_cb, sess,
								_("_Find"));
	g_object_set_data (G_OBJECT (wid), "e", entry);

	g_signal_connect (G_OBJECT (win), "key-press-event", G_CALLBACK (search_key_cb), win);

	gtk_widget_show (win);
}
