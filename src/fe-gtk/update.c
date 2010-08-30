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

#include "../common/xchat.h"
#include "../common/util.h"
#include "../common/portable.h"
#include "check-version.h"
#include "palette.h"
#include "pixmaps.h"
#include "gtkutil.h"
#include "update.h"

#include <windows.h>
#include <wininet.h>

char* check_version ()
{
	HINTERNET hINet, hFile;
	hINet = InternetOpen("XChat-WDK Update Checker", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	
	if (!hINet)
	{
		return "Unknown";
	}

	hFile = InternetOpenUrl (hINet, "http://xchat-wdk.googlecode.com/hg/version.txt", NULL, 0, 0, 0);
	
	if (hFile)
	{
		static char buffer[1024];
		DWORD dwRead;
		while (InternetReadFile(hFile, buffer, 1023, &dwRead))
		{
			if (dwRead == 0)
			{
				break;
			}
			buffer[dwRead] = 0;
		}
		
		return buffer;

		InternetCloseHandle (hFile);
	}
	
	InternetCloseHandle (hINet);

	return "Unknown";
}

static GtkWidget *about = 0;

static int
about_close (void)
{
	about = 0;
	return 0;
}

void
menu_update (GtkWidget * wid, gpointer sess)
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
	gtk_window_set_title (GTK_WINDOW (about), _("Update Checker"));
	if (parent_window)
		gtk_window_set_transient_for (GTK_WINDOW (about), parent_window);
	g_signal_connect (G_OBJECT (about), "destroy",
							G_CALLBACK (about_close), 0);

	vbox = GTK_DIALOG (about)->vbox;

	wid = gtk_image_new_from_pixbuf (pix_xchat);
	gtk_container_add (GTK_CONTAINER (vbox), wid);

	label = gtk_label_new (NULL);
	gtk_container_add (GTK_CONTAINER (vbox), label);
	g_get_charset (&locale);
	(snprintf) (buf, sizeof (buf),
				"<b>Your Version</b>: "PACKAGE_VERSION"\n"
				"<b>Latest Version</b>: %s%s",
				check_version ()
				/*((strcmp (check_version (), PACKAGE_VERSION) != 0) && (strcmp (check_version (), "Unknown") != 0)) ? "\n\nDownload the new version from\n\n<b>http://code.google.com/p/xchat-wdk/</b>" : ""*/
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
