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
#include <gtk/gtkstock.h>

#include "../common/xchat.h"
#include "../common/util.h"
#include "../common/xchatc.h"
#include "gtkutil.h"
#include "xtext.h"
#include "maingui.h"


static void
search_search (session * sess, const char *text)
{
	static void *last = NULL;

	if (!is_session (sess))
	{
		gtkutil_simpledialog (_("The window you opened this Search "
									 "for doesn't exist anymore."));
		return;
	}

	last = gtk_xtext_search (GTK_XTEXT (sess->gui->xtext), text, last);
}

static void
search_find_cb (GtkWidget * button, session * sess)
{
	GtkEntry *entry;
	const char *text;

	entry = g_object_get_data (G_OBJECT (button), "e");
	text = gtk_entry_get_text (entry);
	search_search (sess, text);
}

static void
search_clear_cb (GtkWidget * button, GtkEntry * entry)
{
	gtk_entry_set_text (entry, "");
	gtk_widget_grab_focus (GTK_WIDGET (entry));
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

void
search_open (session * sess)
{
	GtkWidget *win, *hbox, *vbox, *entry, *wid;

	win = mg_create_generic_tab ("search", _("X-Chat: Search"), TRUE, FALSE,
								 NULL, NULL, 0, 0, &vbox, 0);

	gtk_container_set_border_width (GTK_CONTAINER (win), 20);

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

	hbox = gtk_hbox_new (0, 10);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, 0, 0, 4);
	gtk_widget_show (hbox);

	wid = gtkutil_button (hbox, GTK_STOCK_FIND, 0, search_find_cb, sess,
								_("Find"));
	g_object_set_data (G_OBJECT (wid), "e", entry);
	gtkutil_button (hbox, GTK_STOCK_CLEAR, 0, search_clear_cb, entry,
						_("Clear"));
	gtkutil_button (hbox, GTK_STOCK_CLOSE, 0, search_close_cb, win,
						_("Close"));

	gtk_widget_show (win);
}
