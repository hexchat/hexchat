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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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
#include <gtk/gtk.h>
#include "../common/hexchat.h"
#include "../common/util.h"
#include "palette.h"
#include "pixmaps.h"
#include "gtkutil.h"

void (*server_callback)(int, void *) = 0;

static void
sslalert_cb (GtkDialog *dialog, gint response, gpointer data)
{
	if (response < 0) /* Such as window deleted */
		server_callback (SSLALERT_RESPONSE_ABORT, data);
	else
		server_callback (response, data);

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
fe_sslalert_open (struct server *serv, void (*callback)(int, void *), void *callback_data)
{
	GtkWidget *sslalert;
	GtkWidget *wid;
	GtkWidget *dialog_vbox;
	GtkWidget *expander;
	GtkWidget *hbox1, *vbox1, *vbox2;
	GtkWidget *img_vbox;
	char *cert_buf;
	char buf[256];
	char buf2[256];

	server_callback = callback;

	sslalert = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (sslalert), _ (DISPLAY_NAME": Security Alert"));
	gtk_window_set_type_hint (GTK_WINDOW (sslalert), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_position (GTK_WINDOW (sslalert), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_transient_for (GTK_WINDOW (sslalert), GTK_WINDOW (serv->front_session->gui->window));
	gtk_window_set_modal (GTK_WINDOW (sslalert), TRUE);
	gtk_window_set_resizable (GTK_WINDOW (sslalert), FALSE);

	dialog_vbox = gtk_dialog_get_content_area (GTK_DIALOG (sslalert));

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), vbox1, TRUE, TRUE, 0);

	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

	img_vbox = gtk_vbox_new (FALSE, 10);
	gtk_container_set_border_width (GTK_CONTAINER (img_vbox), 6);
	gtk_box_pack_start (GTK_BOX (hbox1), img_vbox, TRUE, TRUE, 0);

	wid = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start (GTK_BOX (img_vbox), wid, FALSE, TRUE, 24);
	gtk_misc_set_alignment (GTK_MISC (wid), 0.5f, 0.06f);

	vbox2 = gtk_vbox_new (FALSE, 10);
	gtk_container_set_border_width (GTK_CONTAINER (vbox2), 6);
	gtk_box_pack_start (GTK_BOX (hbox1), vbox2, TRUE, TRUE, 0);

	snprintf (buf2, sizeof (buf2), _ ("Connecting to %s (+%d)"),
		serv->hostname, serv->port);
	snprintf (buf, sizeof (buf), "\n<b>%s</b>", buf2);
	wid = gtk_label_new (buf);
	gtk_box_pack_start (GTK_BOX (vbox2), wid, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (wid), TRUE);
	gtk_misc_set_alignment (GTK_MISC (wid), 0, 0.5);

	wid = gtk_label_new (_ ("This server has presented an invalid certificate, and is self-signed, expired, or has another problem."));
	gtk_box_pack_start (GTK_BOX (vbox2), wid, FALSE, FALSE, 0);
	gtk_label_set_line_wrap (GTK_LABEL (wid), TRUE);
	gtk_misc_set_alignment (GTK_MISC (wid), 0, 0.5);

	wid = gtk_label_new (_ ("If you are certain that your connection is not being tampered with, you can continue and your connection will be secure."));
	gtk_box_pack_start (GTK_BOX (vbox2), wid, FALSE, FALSE, 0);
	gtk_label_set_line_wrap (GTK_LABEL (wid), TRUE);
	gtk_misc_set_alignment (GTK_MISC (wid), 0, 0.5);

	if (serv->cert_info)
	{
		char *subject;
		char *issuer;

		expander = gtk_expander_new (_ ("More details:"));
		gtk_widget_set_can_focus (expander, FALSE);
		gtk_container_set_border_width (GTK_CONTAINER (expander), 10);
		gtk_box_pack_start (GTK_BOX (vbox1), expander, FALSE, FALSE, 0);

		wid = gtk_label_new (NULL);
		gtk_label_set_use_markup (GTK_LABEL (wid), TRUE);
		gtk_label_set_justify (GTK_LABEL (wid), GTK_JUSTIFY_LEFT);
		gtk_container_add (GTK_CONTAINER (expander), wid);

		issuer = g_strjoinv ("\n\t\t", serv->cert_info->issuer_word);
		subject = g_strjoinv ("\n\t\t", serv->cert_info->subject_word);
		cert_buf = g_markup_printf_escaped ("<b>Issuer:</b>\t%s\n\n"\
											"<b>Subject:</b> %s\n\n"\
											"<b>Valid:</b>\tAfter: %s\n\t\tBefore: %s\n\n"\
											"<b>Algorithm:</b> %s (%d bits)",
											issuer, subject,
											serv->cert_info->notbefore, serv->cert_info->notafter,
											serv->cert_info->algorithm, serv->cert_info->algorithm_bits);

		gtk_label_set_markup (GTK_LABEL (wid), cert_buf);

		g_free (cert_buf);
		g_free (issuer);
		g_free (subject);
	}

	gtk_dialog_add_buttons (GTK_DIALOG (sslalert), _ ("Abort"), SSLALERT_RESPONSE_ABORT,
													_("Accept Once"), SSLALERT_RESPONSE_ACCEPT,
													_("Always Accept"), SSLALERT_RESPONSE_SAVE, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (sslalert), SSLALERT_RESPONSE_ABORT);

	g_signal_connect (G_OBJECT (sslalert), "response", G_CALLBACK (sslalert_cb), callback_data);

	gtk_widget_show_all (sslalert);
}