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

#include <gtk/gtkeditable.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkbutton.h>

#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "gtkutil.h"
#include "ascii.h"
#include "maingui.h"


static void
ascii_click (GtkWidget * wid, int c)
{
	int tmp_pos, len;
	unsigned char str = c;
	char *locale;

	if (current_sess)
	{
		wid = current_sess->gui->input_box;
		tmp_pos = gtk_editable_get_position (GTK_EDITABLE (wid));
		locale = g_locale_to_utf8 (&str, 1, 0, &len, 0);
		gtk_editable_insert_text (GTK_EDITABLE (wid), locale, len, &tmp_pos);
		g_free (locale);
		gtk_editable_set_position (GTK_EDITABLE (wid), tmp_pos);
	}
}

void
ascii_open (void)
{
	int do_unref = TRUE;
	int i, j, val;
	unsigned char name[2], num[4];
	GtkWidget *wid, *but, *hbox, *vbox, *win;
	GtkStyle *style;
	char *utf;

	style = gtk_style_new ();
	/*gdk_font_unref (style->font);
	if (menu_sess && menu_sess->type == SESS_DIALOG)
	{
      style->font = dialog_font_normal;
		gdk_font_ref (dialog_font_normal);
	} else
	{
		style->font = font_normal;
		gdk_font_ref (font_normal);
	}*/

	win = mg_create_generic_tab ("asciichart", _("Ascii Chart"), TRUE, TRUE,
										  NULL, NULL, 0, 0, &vbox, NULL);

	name[1] = 0;

	for (i = 0; i < 16; i++)
	{
		hbox = gtk_hbox_new (0, 0);
		sprintf (num, "%03d", i * 16);
		wid = gtk_label_new (num);
		gtk_widget_set_size_request (wid, 36, 20);
		gtk_container_add (GTK_CONTAINER (hbox), wid);
		gtk_widget_show (wid);
		for (j = 0; j < 16; j++)
		{
			val = j + (i * 16);
			name[0] = val;
			utf = g_locale_to_utf8 (name, 1, NULL, NULL, NULL);
			if (!utf)
				utf = g_strdup ("");
			but = gtk_button_new_with_label (utf);
			g_free (utf);
			gtk_widget_set_style (GTK_BIN (but)->child, style);
			if (do_unref)
			{
				do_unref = FALSE;
				g_object_unref (style);
			}
			g_signal_connect (G_OBJECT (but), "clicked",
									G_CALLBACK (ascii_click), GINT_TO_POINTER(val));
			gtk_widget_set_size_request (but, 36, 20);
			gtk_container_add (GTK_CONTAINER (hbox), but);
			gtk_widget_show (but);
		}
		gtk_container_add (GTK_CONTAINER (vbox), hbox);
		gtk_widget_show (hbox);
	}

	gtk_widget_show (win);
}
