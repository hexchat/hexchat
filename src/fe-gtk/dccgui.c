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
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define WANTSOCKET
#define WANTARPA
#include "../common/inet.h"
#include "fe-gtk.h"

#include <gtk/gtkclist.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkstock.h>

#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../common/fe.h"
#include "../common/util.h"
#include "../common/network.h"
#include "gtkutil.h"
#include "palette.h"
#include "maingui.h"

#ifdef USE_GNOME
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#endif

/*** UNIT PATCH ***/

#define KILOBYTE 1024
#define MEGABYTE (KILOBYTE * 1024)
#define GIGABYTE (MEGABYTE * 1024)

static void proper_unit (int size, char *buf, int buf_len)
{
	if (size <= KILOBYTE)
	{
		snprintf (buf, buf_len, "%uB", size);
	}
	else if (size > KILOBYTE && size <= MEGABYTE)
	{
		snprintf (buf, buf_len, "%ukB", size / KILOBYTE);
	}
	else
	{
		snprintf (buf, buf_len, "%.2fMB", (float)size / MEGABYTE);
	}
}

/*** UNIT PATCH ***/

struct dccwindow
{
	GtkWidget *window;
	GtkWidget *list;
};

struct my_dcc_send
{
	struct session *sess;
	char *nick;
	int maxcps;
};

static struct dccwindow dccrwin;	/* recv */
static struct dccwindow dccswin;	/* send */
static struct dccwindow dcccwin;	/* chat */


static void
dcc_send_filereq_done (struct my_dcc_send *mdc, void *dummy, char *file)
{
	char tbuf[400];

	if (file)
		dcc_send (mdc->sess, tbuf, mdc->nick, file, mdc->maxcps);
	free (mdc->nick);
	free (mdc);
}

void
fe_dcc_send_filereq (struct session *sess, char *nick, int maxcps)
{
	char tbuf[128];
	struct my_dcc_send *mdc;
	
	mdc = malloc (sizeof (*mdc));
	mdc->sess = sess;
	mdc->nick = strdup (nick);
	mdc->maxcps = maxcps;

	snprintf (tbuf, sizeof tbuf, _("Send file to %s"), nick);
	gtkutil_file_req (tbuf, dcc_send_filereq_done, mdc, NULL, FALSE);
}

void
fe_dcc_update_recv (struct DCC *dcc)
{
	char pos[16], kbs[14], perc[14], eta[14];
	gint row;
	int to_go;
	float per;

	if (!dccrwin.window)
		return;

	row =
		gtk_clist_find_row_from_data (GTK_CLIST (dccrwin.list), (gpointer) dcc);

	/* percentage done */
	per = (float) ((dcc->pos * 100.00) / dcc->size);

	proper_unit (dcc->pos, pos, sizeof (pos));
	snprintf (kbs, sizeof (kbs), "%.1f", ((float)dcc->cps) / 1024);
	snprintf (perc, sizeof (perc), "%.0f%%", per);

	gtk_clist_freeze (GTK_CLIST (dccrwin.list));
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 0,
							  _(dccstat[dcc->dccstat].name));
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 3, pos);
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 4, perc);
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 5, kbs);
	if (dcc->cps != 0)
	{
		to_go = (dcc->size - dcc->pos) / dcc->cps;
		snprintf (eta, sizeof (eta), "%.2d:%.2d:%.2d",
					 to_go / 3600, (to_go / 60) % 60, to_go % 60);
	} else
		strcpy (eta, "--:--:--");
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 6, eta);
	if (dccstat[dcc->dccstat].color != 1)
		gtk_clist_set_foreground
			(GTK_CLIST (dccrwin.list), row,
			 colors + dccstat[dcc->dccstat].color);
#ifdef USE_GNOME
	if (dcc->dccstat == STAT_DONE)
		gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 8,
								  (char *) gnome_vfs_get_mime_type (dcc->destfile));
#endif
	gtk_clist_thaw (GTK_CLIST (dccrwin.list));
}

void
fe_dcc_update_send (struct DCC *dcc)
{
	char pos[16], kbs[14], ack[14], perc[14], eta[14];
	gint row;
	int to_go;
	float per;

	if (!dccswin.window)
		return;

	row =
		gtk_clist_find_row_from_data (GTK_CLIST (dccswin.list), (gpointer) dcc);

	/* percentage ack'ed */
	per = (float) ((dcc->ack * 100.00) / dcc->size);

	proper_unit (dcc->pos, pos, sizeof (pos));
	snprintf (kbs, sizeof (kbs), "%.1f", ((float)dcc->cps) / 1024);
	snprintf (ack, sizeof (ack), "%u", dcc->ack);
	snprintf (perc, sizeof (perc), "%.0f%%", per);
	if (dcc->cps != 0)
	{
		to_go = (dcc->size - dcc->ack) / dcc->cps;
		snprintf (eta, sizeof (eta), "%.2d:%.2d:%.2d",
					 to_go / 3600, (to_go / 60) % 60, to_go % 60);
	} else
		strcpy (eta, "--:--:--");
	gtk_clist_freeze (GTK_CLIST (dccswin.list));
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 0,
							  _(dccstat[dcc->dccstat].name));
	if (dccstat[dcc->dccstat].color != 1)
		gtk_clist_set_foreground
			(GTK_CLIST (dccswin.list), row,
			 colors + dccstat[dcc->dccstat].color);
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 3, pos);
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 4, ack);
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 5, perc);
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 6, kbs);
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 7, eta);
	gtk_clist_thaw (GTK_CLIST (dccswin.list));
}

static void
close_dcc_recv_window (void)
{
	dccrwin.window = 0;
}

void
fe_dcc_update_recv_win (void)
{
	struct DCC *dcc;
	GSList *list = dcc_list;
	gchar *nnew[9];
	char size[16];
	char pos[16];
	char kbs[16];
	char perc[14];
	char eta[16];
	gint row;
	int selrow;
	int to_go;
	float per;

	if (!dccrwin.window)
		return;

	selrow = gtkutil_clist_selection (dccrwin.list);

	gtk_clist_clear ((GtkCList *) dccrwin.list);
	nnew[2] = size;
	nnew[3] = pos;
	nnew[4] = perc;
	nnew[5] = kbs;
	nnew[6] = eta;
	while (list)
	{
		dcc = (struct DCC *) list->data;
		if (dcc->type == TYPE_RECV)
		{
			nnew[0] = _(dccstat[dcc->dccstat].name);
			nnew[1] = dcc->file;
			nnew[7] = dcc->nick;
#ifdef USE_GNOME
			if (dcc->dccstat == STAT_DONE)
				nnew[8] = (char *) gnome_vfs_get_mime_type (dcc->destfile);
			else
				nnew[8] = "";
#endif
			proper_unit (dcc->size, size, sizeof (size));
			if (dcc->dccstat == STAT_QUEUED)
				proper_unit (dcc->resumable, pos, sizeof (pos));
			else
				proper_unit (dcc->pos, pos, sizeof (pos));
			snprintf (kbs, sizeof (kbs), "%.1f", ((float)dcc->cps) / 1024);
			/* percentage recv'ed */
			per = (float) ((dcc->pos * 100.00) / dcc->size);
			snprintf (perc, sizeof (perc), "%.0f%%", per);
			if (dcc->cps != 0)
			{
				to_go = (dcc->size - dcc->pos) / dcc->cps;
				snprintf (eta, sizeof (eta), "%.2d:%.2d:%.2d",
							 to_go / 3600, (to_go / 60) % 60, to_go % 60);
			} else
				strcpy (eta, "--:--:--");
			row = gtk_clist_append (GTK_CLIST (dccrwin.list), nnew);
			gtk_clist_set_row_data (GTK_CLIST (dccrwin.list), row,
											(gpointer) dcc);
			if (dccstat[dcc->dccstat].color != 1)
				gtk_clist_set_foreground (GTK_CLIST (dccrwin.list), row,
												  colors +
												  dccstat[dcc->dccstat].color);
		}
		list = list->next;
	}
	if (selrow != -1)
		gtk_clist_select_row ((GtkCList *) dccrwin.list, selrow, 0);
}

static void
dcc_info (struct DCC *dcc)
{
	char tbuf[256];
	snprintf (tbuf, 255, _("      File: %s\n"
				 "   To/From: %s\n"
				 "      Size: %u\n"
				 "      Port: %d\n"
				 " IP Number: %s\n"
				 "Start Time: %s"
				 "   Max CPS: %d\n"),
				 dcc->file,
				 dcc->nick,
				 dcc->size,
				 dcc->port,
				 net_ip (dcc->addr), ctime (&dcc->starttime),
				 dcc->maxcps);
	gtkutil_simpledialog (tbuf);
}

static void
resume_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dccrwin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccrwin.list), row);
		gtk_clist_unselect_row (GTK_CLIST (dccrwin.list), row, 0);
		dcc_resume (dcc);
	}
}

static void
abort_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dccrwin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccrwin.list), row);
		dcc_abort (dcc->serv->front_session, dcc);
	}
}

static void
accept_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dccrwin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccrwin.list), row);
		gtk_clist_unselect_row (GTK_CLIST (dccrwin.list), row, 0);
		dcc_get (dcc);
	}
}

static void
info_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dccrwin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccrwin.list), row);
		if (dcc)
			dcc_info (dcc);
	}
}

#ifdef USE_GNOME

static void
open_clicked (void)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dccrwin.list);
	if (row == -1)
		return;

	dcc = gtk_clist_get_row_data (GTK_CLIST (dccrwin.list), row);
	if (dcc || dcc->dccstat == STAT_DONE)
		return;

	/* do something with dcc->destfile */
}

#endif

static void
recv_row_selected (GtkWidget * clist, gint row, gint column,
						 GdkEventButton * even)
{
	if (even && even->type == GDK_2BUTTON_PRESS)
		accept_clicked (0, 0);
}

void
fe_dcc_open_recv_win (int passive)
{
	GtkWidget *vbox, *bbox;
#ifdef USE_GNOME
	gchar *titles[] =
		{ _("Status"), _("File"), _("Size"), _("Position"), "%", "KB/s", _("ETA"), _("From"),
			_("MIME Type") };
#else
	gchar *titles[] =
		{ _("Status"), _("File"), _("Size"), _("Position"), "%", "KB/s", _("ETA"), _("From") };
#endif

	if (dccrwin.window)
	{
		fe_dcc_update_recv_win ();
		if (!passive)
			mg_bring_tofront (dccrwin.window);
		return;
	}
	dccrwin.window =
			  mg_create_generic_tab ("dccrecv", _("X-Chat: File Receive List"),
						FALSE, TRUE, close_dcc_recv_window, NULL, 601, 0, &vbox, 0);

#ifdef USE_GNOME
	dccrwin.list = gtkutil_clist_new (9, titles, vbox, GTK_POLICY_ALWAYS,
												 recv_row_selected, 0,
												 0, 0, GTK_SELECTION_SINGLE);
#else
	dccrwin.list = gtkutil_clist_new (8, titles, vbox, GTK_POLICY_ALWAYS,
												 recv_row_selected, 0,
												 0, 0, GTK_SELECTION_SINGLE);
#endif
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 0, 65);
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 1, 100);
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 2, 50);
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 3, 50);
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 4, 30);
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 5, 50);
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 6, 60);
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 7, 60);
	gtk_clist_set_column_justification (GTK_CLIST (dccrwin.list), 4,
													GTK_JUSTIFY_CENTER);

	bbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
	gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 2);
	gtk_widget_show (bbox);

	gtkutil_button (bbox, GTK_STOCK_CANCEL, 0, abort_clicked, 0, _("Abort"));

	gtkutil_button (bbox, GTK_STOCK_APPLY, 0, accept_clicked, 0, _("Accept"));

	gtkutil_button (bbox, GTK_STOCK_REFRESH, 0, resume_clicked, 0, _("Resume"));

	gtkutil_button (bbox, GTK_STOCK_DIALOG_INFO, 0, info_clicked, 0, _("Info"));
#ifdef USE_GNOME
	gtkutil_button (bbox, GTK_STOCK_OPEN, 0, open_clicked, 0, _("Open"));
#endif
	gtk_widget_show (dccrwin.window);
	fe_dcc_update_recv_win ();
}

static void
close_dcc_send_window (void)
{
	dccswin.window = 0;
}

void
fe_dcc_update_send_win (void)
{
	struct DCC *dcc;
	GSList *list = dcc_list;
	gchar *nnew[9];
	char size[14];
	char pos[14];
	char kbs[14];
	char ack[14];
	char perc[14];
	char eta[14];
	gint row;
	int selrow;
	int to_go;
	float per;

	if (!dccswin.window)
		return;

	selrow = gtkutil_clist_selection (dccswin.list);

	gtk_clist_clear ((GtkCList *) dccswin.list);
	nnew[2] = size;
	nnew[3] = pos;
	nnew[4] = ack;
	nnew[5] = perc;
	nnew[6] = kbs;
	while (list)
	{
		nnew[7] = eta;
		dcc = (struct DCC *) list->data;
		if (dcc->type == TYPE_SEND)
		{
			nnew[0] = _(dccstat[dcc->dccstat].name);
			nnew[1] = file_part (dcc->file);
			nnew[8] = dcc->nick;
			/* percentage ack'ed */
			per = (float) ((dcc->ack * 100.00) / dcc->size);
			proper_unit (dcc->size, size, sizeof (size));
			proper_unit (dcc->pos, pos, sizeof (pos));
			snprintf (kbs, sizeof (kbs), "%.1f", ((float)dcc->cps) / 1024);
			snprintf (perc, sizeof (perc), "%.0f%%", per);
			snprintf (ack, sizeof (ack), "%u", dcc->ack);
			if (dcc->cps != 0)
			{
				to_go = (dcc->size - dcc->ack) / dcc->cps;
				snprintf (eta, sizeof (eta), "%.2d:%.2d:%.2d",
							 to_go / 3600, (to_go / 60) % 60, to_go % 60);
			} else
				strcpy (eta, "--:--:--");
			row = gtk_clist_append (GTK_CLIST (dccswin.list), nnew);
			gtk_clist_set_row_data (GTK_CLIST (dccswin.list), row,
											(gpointer) dcc);
			if (dccstat[dcc->dccstat].color != 1)
				gtk_clist_set_foreground
					(GTK_CLIST (dccswin.list), row,
					 colors + dccstat[dcc->dccstat].color);
		}
		list = list->next;
	}
	if (selrow != -1)
		gtk_clist_select_row ((GtkCList *) dccswin.list, selrow, 0);
}

static void
send_row_selected (GtkWidget * clist, gint row, gint column,
						 GdkEventButton * even)
{
	struct DCC *dcc;

	if (even && even->type == GDK_2BUTTON_PRESS)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (clist), row);
		if (dcc)
		{
			switch (dcc->dccstat)
			{
			case STAT_FAILED:
			case STAT_ABORTED:
			case STAT_DONE:
				dcc_abort (dcc->serv->front_session, dcc);
			}
		}
	}
}

static void
info_send_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dccswin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccswin.list), row);
		if (dcc)
			dcc_info (dcc);
	}
}

static void
abort_send_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dccswin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccswin.list), row);
		dcc_abort (dcc->serv->front_session, dcc);
	}
}

void
fe_dcc_open_send_win (int passive)
{
	GtkWidget *vbox, *bbox;
	gchar *titles[] =
		{ _("Status"), _("File"), _("Size"), _("Position"), _("Ack"), "%", "KB/s", _("ETA"), _("To") };

	if (dccswin.window)
	{
		fe_dcc_update_send_win ();
		if (!passive)
			mg_bring_tofront (dccswin.window);
		return;
	}

	dccswin.window =
			  mg_create_generic_tab ("dccsend", _("X-Chat: File Send List"),
						FALSE, TRUE, close_dcc_send_window, NULL, 595, 0, &vbox, 0);

	dccswin.list = gtkutil_clist_new (9, titles, vbox, GTK_POLICY_ALWAYS,
												 send_row_selected, 0,
												 0, 0, GTK_SELECTION_SINGLE);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 0, 65);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 1, 100);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 2, 50);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 3, 50);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 4, 50);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 5, 30);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 6, 50);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 7, 50);
	gtk_clist_set_column_justification (GTK_CLIST (dccswin.list), 5,
													GTK_JUSTIFY_CENTER);

	bbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
	gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 2);
	gtk_widget_show (bbox);

	gtkutil_button (bbox, GTK_STOCK_CANCEL, 0, abort_send_clicked, 0, _("Abort"));
	gtkutil_button (bbox, GTK_STOCK_DIALOG_INFO, 0, info_send_clicked, 0, _("Info"));

	gtk_widget_show (dccswin.window);
	fe_dcc_update_send_win ();
}


/* DCC CHAT GUIs BELOW */

static void
accept_chat_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dcccwin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dcccwin.list), row);
		gtk_clist_unselect_row (GTK_CLIST (dcccwin.list), row, 0);
		dcc_get (dcc);
	}
}

static void
abort_chat_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dcccwin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dcccwin.list), row);
		dcc_abort (dcc->serv->front_session, dcc);
	}
}

static void
chat_row_selected (GtkWidget * clist, gint row, gint column,
						 GdkEventButton * even)
{
	if (even && even->type == GDK_2BUTTON_PRESS)
		accept_chat_clicked (0, 0);
}

void
fe_dcc_update_chat_win (void)
{
	struct DCC *dcc;
	GSList *list = dcc_list;
	gchar *nnew[5];
	char pos[16];
	char siz[16];
	gint row;
	int selrow;

	if (!dcccwin.window)
		return;

	selrow = gtkutil_clist_selection (dcccwin.list);

	gtk_clist_clear ((GtkCList *) dcccwin.list);
	nnew[2] = pos;
	nnew[3] = siz;
	while (list)
	{
		dcc = (struct DCC *) list->data;
		if ((dcc->type == TYPE_CHATSEND || dcc->type == TYPE_CHATRECV))
		{
			nnew[0] = _(dccstat[dcc->dccstat].name);
			nnew[1] = dcc->nick;
			proper_unit (dcc->pos, pos, sizeof (pos));
			proper_unit (dcc->size, siz, sizeof (siz));
			nnew[4] = ctime (&dcc->starttime);
			nnew[4][strlen (nnew[4]) - 1] = 0;	/* remove the \n */
			row = gtk_clist_append (GTK_CLIST (dcccwin.list), nnew);
			gtk_clist_set_row_data (GTK_CLIST (dcccwin.list), row,
											(gpointer) dcc);
		}
		list = list->next;
	}
	if (selrow != -1)
		gtk_clist_select_row ((GtkCList *) dcccwin.list, selrow, 0);
}

static void
close_dcc_chat_window (void)
{
	dcccwin.window = 0;
}

void
fe_dcc_open_chat_win (int passive)
{
	GtkWidget *vbox, *bbox;
	gchar *titles[] =
		{ _("Status"), _("To/From"), _("Recv"), _("Sent"), _("StartTime") };

	if (dcccwin.window)
	{
		fe_dcc_update_chat_win ();
		if (!passive)
			mg_bring_tofront (dcccwin.window);
		return;
	}

	dcccwin.window =
			  mg_create_generic_tab ("dccchat", _("X-Chat: DCC Chat List"),
						FALSE, TRUE, close_dcc_chat_window, NULL, 550, 0, &vbox, 0);

	dcccwin.list = gtkutil_clist_new (5, titles, vbox, GTK_POLICY_ALWAYS,
												 chat_row_selected, 0,
												 0, 0, GTK_SELECTION_BROWSE);
	gtk_clist_set_column_width (GTK_CLIST (dcccwin.list), 0, 65);
	gtk_clist_set_column_width (GTK_CLIST (dcccwin.list), 1, 100);
	gtk_clist_set_column_width (GTK_CLIST (dcccwin.list), 2, 65);
	gtk_clist_set_column_width (GTK_CLIST (dcccwin.list), 3, 65);

	bbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
	gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 2);
	gtk_widget_show (bbox);

	gtkutil_button (bbox, GTK_STOCK_CANCEL, 0, abort_chat_clicked, 0, _("Abort"));
	gtkutil_button (bbox, GTK_STOCK_APPLY, 0, accept_chat_clicked, 0, _("Accept"));

	gtk_widget_show (dcccwin.window);
	fe_dcc_update_chat_win ();
}
