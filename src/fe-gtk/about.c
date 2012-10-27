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

#include "../common/hexchat.h"
#include "../common/util.h"
#include "../common/hexchatc.h"
#include "palette.h"
#include "pixmaps.h"
#include "gtkutil.h"
#include "about.h"

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
	GtkWidget *vbox;									/* the main vertical box inside the about dialog */
	GtkWidget *hbox_main;								/* horizontal box for containing text on the left and logo on the right */
	GtkWidget *vbox_logo;								/* vertical box for our logo */
	GtkWidget *vbox_text;								/* vertical box for text */
	GtkWidget *label_title;								/* label for the main title */
	GtkWidget *label_subtitle;							/* label for the subtitle */
	GtkWidget *label_info;								/* for the informative text */
	GtkWidget *label_copyright;							/* for copyright notices */
	GdkColor color;										/* color buffer for our nice paintings */
	char buf[512];										/* text buffer for the labels */
	const char *locale = NULL;
	extern GtkWindow *parent_window;					/* maingui.c */

	if (about)
	{
		gtk_window_present (GTK_WINDOW (about));
		return;
	}

	/* general about dialog initialization */
	about = gtk_dialog_new ();
	gtk_window_set_position (GTK_WINDOW (about), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_resizable (GTK_WINDOW (about), FALSE);
	gtk_window_set_title (GTK_WINDOW (about), _("About "DISPLAY_NAME));
	if (parent_window)
	{
		gtk_window_set_transient_for (GTK_WINDOW (about), parent_window);
	}
	g_signal_connect (G_OBJECT (about), "destroy", G_CALLBACK (about_close), 0);
	vbox = GTK_DIALOG (about)->vbox;

	/* main horizontal box, text on the left, logo on the right */
	hbox_main = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (vbox), GTK_WIDGET (hbox_main));

	/* textbox on the left */
	vbox_text = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox_main), vbox_text, 0, 0, 5);

	/* label for title */
	snprintf (buf, sizeof (buf), "<span size=\"x-large\"><b>"DISPLAY_NAME"</b></span>");
	label_title = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label_title), 0, 0);
	gtk_box_pack_start (GTK_BOX (vbox_text), label_title, 0, 0, 10);
	color.red   = 0xd7d7;
	color.green = 0x4343;
	color.blue  = 0x0404;
	gtk_widget_modify_fg (label_title, GTK_STATE_NORMAL, &color);
	gtk_label_set_markup (GTK_LABEL (label_title), buf);

	/* label for subtitle */
	snprintf (buf, sizeof (buf), "%s", _("<b>A multiplatform IRC Client</b>"));
	label_subtitle = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label_subtitle), 0, 0);
	gtk_box_pack_start (GTK_BOX (vbox_text), label_subtitle, 0, 0, 10);
	color.red   = 0x5555;
	color.green = 0x5555;
	color.blue  = 0x5555;
	gtk_widget_modify_fg (label_subtitle, GTK_STATE_NORMAL, &color);
	gtk_label_set_markup (GTK_LABEL (label_subtitle), buf);

	/* label for additional info */
	g_get_charset (&locale);
	(snprintf) (buf, sizeof (buf),
				"<b>Version:</b> "PACKAGE_VERSION"\n"
				"<b>Compiled:</b> "__DATE__"\n"
#ifdef WIN32
				"<b>Portable Mode:</b> %s\n"
				"<b>Build Type:</b> x%d\n"
#endif
				"<b>OS:</b> %s\n"
				"<b>Charset:</b> %s",
#ifdef WIN32
				(portable_mode () ? "Yes" : "No"),
				get_cpu_arch (),
#endif
				get_sys_str (0),
				locale);

	label_info = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label_info), 0, 0);
	gtk_box_pack_start (GTK_BOX (vbox_text), label_info, 0, 0, 10);
	gtk_label_set_selectable (GTK_LABEL (label_info), TRUE);
	gtk_label_set_markup (GTK_LABEL (label_info), buf);

	/* label for copyright notices */
	snprintf (buf, sizeof (buf), "<small>\302\251 1998-2010 Peter \305\275elezn\303\275\n\302\251 2009-2012 Berke Viktor</small>");
	label_copyright = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label_copyright), 0, 0);
	gtk_box_pack_start (GTK_BOX (vbox_text), label_copyright, 0, 0, 10);
	gtk_label_set_markup (GTK_LABEL (label_copyright), buf);

	/* imagebox on the right */
	vbox_logo = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox_main), vbox_logo, 0, 0, 10);

	/* the actual image */
	wid = gtk_image_new_from_pixbuf (pix_xchat);
	gtk_box_pack_start (GTK_BOX (vbox_logo), wid, 0, 0, 10);

	/* our close button on the bottom right */
	wid = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (wid), GTK_CAN_DEFAULT);
	gtk_box_pack_end (GTK_BOX (GTK_DIALOG (about)->action_area), wid, 0, 0, 0);
	gtk_widget_grab_default (wid);
	g_signal_connect (G_OBJECT (wid), "clicked", G_CALLBACK (gtkutil_destroy), about);

	/* pure white background for the whole about widget*/
	color.red = color.green = color.blue = 0xffff;
	gtk_widget_modify_bg (about, GTK_STATE_NORMAL, &color);

	/* show off! */
	gtk_widget_show_all (about);
}
