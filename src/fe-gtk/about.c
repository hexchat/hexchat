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
#include <stdlib.h>
#include <string.h>

#include "fe-gtk.h"

#include <gtk/gtkmain.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkwindow.h>

#ifdef USE_XLIB
#include <gdk/gdkx.h>
#endif

#include "../common/xchat.h"
#include "../common/util.h"
#include "palette.h"
#include "pixmaps.h"
#include "gtkutil.h"
#include "about.h"


#if 0 /*def USE_GNOME*/

void
menu_about (GtkWidget * wid, gpointer sess)
{
	char buf[512];
	const gchar *author[] = { "Peter Zelezny <zed@xchat.org>", 0 };

	(snprintf) (buf, sizeof (buf),
				 "An IRC Client for UNIX.\n\n"
				 "This binary was compiled on "__DATE__"\n"
				 "Using GTK %d.%d.%d X %d\n"
				 "Running on %s",
				 gtk_major_version, gtk_minor_version, gtk_micro_version,
#ifdef USE_XLIB
				VendorRelease (GDK_DISPLAY ()), get_cpu_str());
#else
				666, get_cpu_str());
#endif

	gtk_widget_show (gnome_about_new ("X-Chat", VERSION,
							"(C) 1998-2004 Peter Zelezny", author, buf, 0));
}

#else

static GtkWidget *about = 0;

static int
about_close (void)
{
	about = 0;
	return 0;
}

void
menu_about (GtkWidget * wid, gpointer sess)
{
	GtkWidget *vbox, *label, *hbox;
	char buf[512];
	const char *locale = NULL;
	extern GtkWindow *parent_window;      /* maingui.c */

	if (about)
	{
		gtk_window_present (GTK_WINDOW (about));
		return;
	}

	about = gtk_dialog_new ();
	gtk_window_set_position (GTK_WINDOW (about), GTK_WIN_POS_CENTER);
	gtk_window_set_resizable (GTK_WINDOW (about), FALSE);
	gtk_window_set_title (GTK_WINDOW (about), _("About X-Chat"));
	if (parent_window)
		gtk_window_set_transient_for (GTK_WINDOW (about), parent_window);
	g_signal_connect (G_OBJECT (about), "destroy",
							G_CALLBACK (about_close), 0);

	vbox = GTK_DIALOG (about)->vbox;

	wid = gtk_image_new_from_pixbuf (pix_xchat);
	gtk_container_add (GTK_CONTAINER (vbox), wid);

	label = gtk_label_new (NULL);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);
	gtk_container_add (GTK_CONTAINER (vbox), label);
	g_get_charset (&locale);
	(snprintf) (buf, sizeof (buf),
				"<span size=\"x-large\"><b>X-Chat "VERSION"</b></span>\n\n"
				"%s\n\n"
				"%s\n"
				"<b>Charset</b>: %s <b>Renderer</b>: %s\n"
				"<b>Compiled</b>: "__DATE__"\n\n"
				"<small>\302\251 1998-2004 Peter \305\275elezn\303\275 &lt;zed@xchat.org></small>",
					_("A multiplatform IRC Client"),
					get_cpu_str(),
					locale,
#ifdef USE_XFT
					"Xft"
#else
					"Pango"
#endif
					);
	gtk_label_set_markup (GTK_LABEL (label), buf);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);

	hbox = gtk_hbox_new (0, 2);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);

	wid = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (wid), GTK_CAN_DEFAULT);
	gtk_box_pack_end (GTK_BOX (GTK_DIALOG (about)->action_area), wid, 0, 0, 0);
	gtk_widget_grab_default (wid);
	g_signal_connect (G_OBJECT (wid), "clicked",
							G_CALLBACK (gtkutil_destroy), about);

	gtk_widget_show_all (about);
}
#endif
