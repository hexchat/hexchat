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
#include "../../config.h"
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <stdlib.h>

#include "fe-gtk.h"

#include <gtk/gtkclist.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkhbbox.h>

#include "../common/xchat.h"
#include "../common/cfgfiles.h"
#include "../common/url.h"
#include "gtkutil.h"
#include "menu.h"
#include "maingui.h"
#include "urlgrab.h"


static GtkWidget *urlgrabberwindow = 0;
static GtkWidget *urlgrabberlist;


static void
url_closegui (GtkWidget *wid, gpointer userdata)
{
	urlgrabberwindow = 0;
}

static void
url_button_clear (void)
{
	url_clear ();
	gtk_clist_clear (GTK_CLIST (urlgrabberlist));
}

static void
url_save_callback (void *arg1, void *arg2, char *file)
{
	if (file)
	{
		url_save (file, "w");
		free (file);
	}
}

static void
url_button_save (void)
{
	gtkutil_file_req (_("Select a file to save to"),
							url_save_callback, 0, 0, TRUE);
}

static gboolean
url_clicklist (GtkWidget * widget, GdkEventButton * event, gpointer userdata)
{
	int row, col;
	char *text;

	if (event->button == 3)
	{
		if (gtk_clist_get_selection_info
			 (GTK_CLIST (widget), event->x, event->y, &row, &col) < 0)
			return FALSE;
		gtk_clist_unselect_all (GTK_CLIST (widget));
		gtk_clist_select_row (GTK_CLIST (widget), row, 0);
		if (gtk_clist_get_text (GTK_CLIST (widget), row, 0, &text))
		{
			if (text && text[0])
			{
				menu_urlmenu (event, text);
			}
		}
	}
	return FALSE;
}

void
fe_url_add (char *urltext)
{
	if (urlgrabberwindow)
		gtk_clist_prepend ((GtkCList *) urlgrabberlist, &urltext);
}

void
url_opengui ()
{
	GtkWidget *vbox, *hbox;
	GSList *list;

	if (urlgrabberwindow)
	{
		mg_bring_tofront (urlgrabberwindow);
		return;
	}

	urlgrabberwindow =
		mg_create_generic_tab ("urlgrabber", _("X-Chat: URL Grabber"), FALSE,
							 TRUE, url_closegui, NULL, 350, 100, &vbox, 0);

	urlgrabberlist = gtkutil_clist_new (1, 0, vbox, GTK_POLICY_AUTOMATIC,
													0, 0, 0, 0, GTK_SELECTION_BROWSE);
	gtk_signal_connect (GTK_OBJECT (urlgrabberlist), "button_press_event",
							  GTK_SIGNAL_FUNC (url_clicklist), 0);
	gtk_widget_set_usize (urlgrabberlist, 350, 0);
	gtk_clist_set_column_width (GTK_CLIST (urlgrabberlist), 0, 100);

	hbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_SPREAD);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, 0, 0, 0);
	gtk_widget_show (hbox);

	gtkutil_button (hbox, GTK_STOCK_CLEAR,
						 0, url_button_clear, 0, _("Clear"));
	gtkutil_button (hbox, GTK_STOCK_SAVE,
						 0, url_button_save, 0, _("Save"));

	gtk_widget_show (urlgrabberwindow);

	list = url_list;
	while (list)
	{
		fe_url_add ((char *) list->data);
		list = list->next;
	}
}
