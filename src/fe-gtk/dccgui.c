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
#include <gtk/gtkmessagedialog.h>

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

static void proper_unit (DCC_SIZE size, char *buf, int buf_len)
{
	if (size <= KILOBYTE)
	{
		snprintf (buf, buf_len, "%"DCC_SFMT"B", size);
	}
	else if (size > KILOBYTE && size <= MEGABYTE)
	{
		snprintf (buf, buf_len, "%"DCC_SFMT"kB", size / KILOBYTE);
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
	int passive;
};

static struct dccwindow dccrwin;	/* recv */
static struct dccwindow dccswin;	/* send */
static struct dccwindow dcccwin;	/* chat */


static void
dcc_send_filereq_file (struct my_dcc_send *mdc, char *file)
{
	if (file)
		dcc_send (mdc->sess, mdc->nick, file, mdc->maxcps, mdc->passive);
	else
	{
		free (mdc->nick);
		free (mdc);
	}
}

void
fe_dcc_send_filereq (struct session *sess, char *nick, int maxcps, int passive)
{
	char tbuf[128];
	struct my_dcc_send *mdc;
	
	mdc = malloc (sizeof (*mdc));
	mdc->sess = sess;
	mdc->nick = strdup (nick);
	mdc->maxcps = maxcps;
	mdc->passive = passive;

	snprintf (tbuf, sizeof tbuf, _("Send file to %s"), nick);
	gtkutil_file_req (tbuf, dcc_send_filereq_file, mdc, NULL, FRF_MULTIPLE);
}

static void
dcc_prepare_row_chat (struct DCC *dcc, char *col[])
{
	static char pos[16], siz[16];

	col[0] = _(dccstat[dcc->dccstat].name);
	col[1] = dcc->nick;
	col[2] = pos;
	col[3] = siz;
	col[4] = ctime (&dcc->starttime);
	col[4][strlen (col[4]) - 1] = 0;	/* remove the \n */

	proper_unit (dcc->pos, pos, sizeof (pos));
	proper_unit (dcc->size, siz, sizeof (siz));
}

static void
dcc_prepare_row_send (struct DCC *dcc, char *col[])
{
	static char pos[16], size[16], kbs[14], ack[16], perc[14], eta[14];
	int to_go;
	float per;

	col[0] = _(dccstat[dcc->dccstat].name);
	col[1] = file_part (dcc->file);
	col[2] = size;
	col[3] = pos;
	col[4] = ack;
	col[5] = perc;
	col[6] = kbs;
	col[7] = eta;
	col[8] = dcc->nick;

	/* percentage ack'ed */
	per = (float) ((dcc->ack * 100.00) / dcc->size);
	proper_unit (dcc->size, size, sizeof (size));
	proper_unit (dcc->pos, pos, sizeof (pos));
	snprintf (kbs, sizeof (kbs), "%.1f", ((float)dcc->cps) / 1024);
	proper_unit (dcc->ack, ack, sizeof (ack));
	snprintf (perc, sizeof (perc), "%.0f%%", per);
	if (dcc->cps != 0)
	{
		to_go = (dcc->size - dcc->ack) / dcc->cps;
		snprintf (eta, sizeof (eta), "%.2d:%.2d:%.2d",
					 to_go / 3600, (to_go / 60) % 60, to_go % 60);
	} else
		strcpy (eta, "--:--:--");
}

static void
dcc_prepare_row_recv (struct DCC *dcc, char *col[])
{
	static char size[16], pos[16], kbs[16], perc[14], eta[16];
	float per;
	int to_go;

	col[0] = _(dccstat[dcc->dccstat].name);
	col[1] = dcc->file;
	col[2] = size;
	col[3] = pos;
	col[4] = perc;
	col[5] = kbs;
	col[6] = eta;
	col[7] = dcc->nick;
#ifdef USE_GNOME
	if (dcc->dccstat == STAT_DONE)
		col[8] = (char *) gnome_vfs_get_mime_type (dcc->destfile);
	else
		col[8] = "";
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
}

static void
dcc_update_recv (struct DCC *dcc)
{
	gint row;
	char *col[9];

	if (!dccrwin.window)
		return;

	row = gtk_clist_find_row_from_data (GTK_CLIST (dccrwin.list), dcc);

	dcc_prepare_row_recv (dcc, col);

	gtk_clist_freeze (GTK_CLIST (dccrwin.list));
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 0, col[0]);
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 3, col[3]);
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 4, col[4]);
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 5, col[5]);
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 6, col[6]);
#ifdef USE_GNOME
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 8, col[8]);
#endif

	if (dccstat[dcc->dccstat].color != 1)
		gtk_clist_set_foreground (GTK_CLIST (dccrwin.list), row,
										  colors + dccstat[dcc->dccstat].color);

	gtk_clist_thaw (GTK_CLIST (dccrwin.list));
}

static void
dcc_update_chat (struct DCC *dcc)
{
	gint row;
	char *col[5];

	if (!dcccwin.window)
		return;

	row = gtk_clist_find_row_from_data (GTK_CLIST (dcccwin.list), dcc);

	dcc_prepare_row_chat (dcc, col);

	gtk_clist_freeze (GTK_CLIST (dcccwin.list));
	gtk_clist_set_text (GTK_CLIST (dcccwin.list), row, 0, col[0]);
	gtk_clist_set_text (GTK_CLIST (dcccwin.list), row, 2, col[2]);
	gtk_clist_set_text (GTK_CLIST (dcccwin.list), row, 3, col[3]);
	gtk_clist_thaw (GTK_CLIST (dcccwin.list));
}

static void
dcc_update_send (struct DCC *dcc)
{
	gint row;
	char *col[9];

	if (!dccswin.window)
		return;

	row = gtk_clist_find_row_from_data (GTK_CLIST (dccswin.list), dcc);

	dcc_prepare_row_send (dcc, col);

	gtk_clist_freeze (GTK_CLIST (dccswin.list));
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 0,
							  _(dccstat[dcc->dccstat].name));
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 3, col[3]);
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 4, col[4]);
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 5, col[5]);
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 6, col[6]);
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 7, col[7]);

	if (dccstat[dcc->dccstat].color != 1)
		gtk_clist_set_foreground (GTK_CLIST (dccswin.list), row,
					 colors + dccstat[dcc->dccstat].color);

	gtk_clist_thaw (GTK_CLIST (dccswin.list));
}

static void
close_dcc_recv_window (void)
{
	dccrwin.window = 0;
}

static void
dcc_update_recv_win (void)
{
	struct DCC *dcc;
	GSList *list = dcc_list;
	gchar *nnew[9];
	gint row;
	int i = 0;

	gtk_clist_freeze (GTK_CLIST (dccrwin.list));
	gtk_clist_clear (GTK_CLIST (dccrwin.list));
	while (list)
	{
		dcc = list->data;
		if (dcc->type == TYPE_RECV)
		{
			dcc_prepare_row_recv (dcc, nnew);

			row = gtk_clist_append (GTK_CLIST (dccrwin.list), nnew);
			gtk_clist_set_row_data (GTK_CLIST (dccrwin.list), row, dcc);
			if (dccstat[dcc->dccstat].color != 1)
				gtk_clist_set_foreground (GTK_CLIST (dccrwin.list), row,
												  colors +
												  dccstat[dcc->dccstat].color);
			i++;
		}
		list = list->next;
	}

	/* if only one entry, select it (so Accept button can work) */
	if (i == 1)
		gtk_clist_select_row (GTK_CLIST (dccrwin.list), 0, 0);

	gtk_clist_thaw (GTK_CLIST (dccrwin.list));
}

static void
dcc_info (struct DCC *dcc)
{
	char max[48];
	char siz[48];
	GtkWidget *dialog;

	if (dcc->maxcps)
		sprintf (max, "%.2f KB/s", (float)dcc->maxcps / 1024.0);
	else
		snprintf (max, sizeof (max), "%s", _("None"));

	proper_unit (dcc->size, siz, sizeof (siz));

	dialog = gtk_message_dialog_new_with_markup (NULL, 0, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
				 "<tt><b>%-13s</b></tt> %s\n"
				 "<tt><b>%-13s</b></tt> %s\n"
				 "<tt><b>%-13s</b></tt> %s (%"DCC_SFMT" bytes)\n"
				 "<tt><b>%-13s</b></tt> %s : %d\n"
				 "<tt><b>%-13s</b></tt> %s"
				 "<tt><b>%-13s</b></tt> %s\n",
				 _("File:"), (dcc->type == TYPE_RECV) ? dcc->destfile : dcc->file,
				 (dcc->type == TYPE_RECV) ? _("From:") : _("To:"), dcc->nick,
				 _("Size:"), siz, dcc->size,
				 _("Address:"), net_ip (dcc->addr), dcc->port,
				 _("Started:"), ctime (&dcc->starttime),
				 _("Speed limit:"), max);
	g_signal_connect (G_OBJECT (dialog), "response",
							G_CALLBACK (gtk_widget_destroy), 0);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
	gtk_widget_show (dialog);
}

static void
resume_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;
	char buf[512];

	row = gtkutil_clist_selection (dccrwin.list);
	if (row != -1)
	{
		gtk_clist_unselect_row (GTK_CLIST (dccrwin.list), row, 0);
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccrwin.list), row);
		if (!dcc_resume (dcc))
		{
			switch (dcc->resume_error)
			{
			case 0:	/* unknown error */
				gtkutil_simpledialog (_("That file is not resumable."));
				break;
			case 1:
				snprintf (buf, sizeof (buf),
							_(	"Cannot access file: %s\n"
								"%s.\n"
								"Resuming not possible."), dcc->destfile,	
								errorstring (dcc->resume_errno));
				gtkutil_simpledialog (buf);
				break;
			case 2:
				gtkutil_simpledialog (_("File in download directory is larger "
											"than file offered. Resuming not possible."));
				break;
			case 3:
				gtkutil_simpledialog (_("Cannot resume the same file from two people."));
			}
		}
	}
}

static void
abort_clicked (GtkWidget * wid, gpointer none)
{
	int * rows;
	int i, num_rows;
	struct DCC *dcc;

	num_rows = gtkutil_clist_multiple_selection (dccrwin.list, &rows, 256);
	gtk_clist_freeze( GTK_CLIST(dccrwin.list) );

	for (i = num_rows - 1; i >= 0; i--)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccrwin.list), rows[i]);
		if ( dcc )
		{
			dcc_abort (dcc->serv->front_session, dcc);
		}
	}
	gtk_clist_thaw( GTK_CLIST(dccrwin.list) );
	free ( rows );
}

static void
accept_clicked (GtkWidget * wid, gpointer none)
{
	int * rows;
	int i, num_rows;
	struct DCC *dcc;

	num_rows = gtkutil_clist_multiple_selection (dccrwin.list, &rows, 256);
	gtk_clist_freeze( GTK_CLIST(dccrwin.list) );

	for (i = num_rows - 1; i >= 0; i--)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccrwin.list), rows[i]);
		if ( dcc )
		{
			gtk_clist_unselect_row (GTK_CLIST (dccrwin.list), rows[i], 0);
			dcc_get (dcc);
		}
	}
	gtk_clist_thaw( GTK_CLIST(dccrwin.list) );
	free ( rows );
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

static void
browse_folder (char *dir)
{
#ifdef WIN32
	/* no need for file:// in ShellExecute() */
	fe_open_url (dir);
#else
	char buf[512];

	snprintf (buf, sizeof (buf), "file://%s", dir);
	fe_open_url (buf);
#endif
}

static gboolean
recv_key_press (GtkWidget *wid, GdkEventKey *evt, gpointer unused)
{
	if (evt->state & GDK_CONTROL_MASK && evt->keyval == 0x06f)
	{
		if (prefs.dcc_completed_dir[0])
			browse_folder (prefs.dcc_completed_dir);
		else
			browse_folder (prefs.dccdir);
	}

	return FALSE;
}

int
fe_dcc_open_recv_win (int passive)
{
	GtkWidget *vbox, *bbox;
	int i;
#ifdef USE_GNOME
	gchar *titles[] =
		{ NULL, NULL, NULL, NULL, "%", "KB/s", NULL, NULL, NULL };

	titles[8] = _("MIME Type");
#else
	gchar *titles[] =
		{ NULL, NULL, NULL, NULL, "%", "KB/s", NULL, NULL };
#endif

	titles[0] = _("Status");
	titles[1] = _("File");
	titles[2] = _("Size");
	titles[3] = _("Position");
	titles[6] = _("ETA");
	titles[7] = _("From");

	if (dccrwin.window)
	{
		if (!passive)
			mg_bring_tofront (dccrwin.window);
		return TRUE;
	}
	dccrwin.window =
			  mg_create_generic_tab ("Downloads", _("XChat: File Receive List"),
						FALSE, TRUE, close_dcc_recv_window, NULL, 601, 200, &vbox, 0);
	if (!prefs.windows_as_tabs)
		gtk_window_set_position (GTK_WINDOW (dccrwin.window), GTK_WIN_POS_NONE);
	g_signal_connect (G_OBJECT (dccrwin.window), "key_release_event",
							G_CALLBACK (recv_key_press), 0);
#ifdef USE_GNOME
	dccrwin.list = gtkutil_clist_new (9, titles, vbox, GTK_POLICY_ALWAYS,
												 recv_row_selected, 0,
												 0, 0, GTK_SELECTION_SINGLE);
	for(i=0; i < 9; i++)
	{
		gtk_clist_set_column_auto_resize(GTK_CLIST(dccrwin.list), i, TRUE);
	}
#else
	dccrwin.list = gtkutil_clist_new (8, titles, vbox, GTK_POLICY_ALWAYS,
												 recv_row_selected, 0,
												 0, 0, GTK_SELECTION_SINGLE);
	for(i=0; i < 8; i++)
	{
		gtk_clist_set_column_auto_resize(GTK_CLIST(dccrwin.list), i, TRUE);
	}
#endif
	gtk_clist_set_selection_mode(GTK_CLIST(dccrwin.list), GTK_SELECTION_MULTIPLE);
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
	dcc_update_recv_win ();

	return FALSE;
}

static void
close_dcc_send_window (void)
{
	dccswin.window = 0;
}

static void
dcc_update_send_win (void)
{
	struct DCC *dcc;
	GSList *list = dcc_list;
	gchar *nnew[9];
	gint row;
	int i = 0;

	gtk_clist_freeze (GTK_CLIST (dccswin.list));
	gtk_clist_clear ((GtkCList *) dccswin.list);
	while (list)
	{
		dcc = list->data;
		if (dcc->type == TYPE_SEND)
		{
			dcc_prepare_row_send (dcc, nnew);
			row = gtk_clist_append (GTK_CLIST (dccswin.list), nnew);
			gtk_clist_set_row_data (GTK_CLIST (dccswin.list), row, dcc);
			if (dccstat[dcc->dccstat].color != 1)
				gtk_clist_set_foreground
					(GTK_CLIST (dccswin.list), row,
					 colors + dccstat[dcc->dccstat].color);
			i++;
		}
		list = list->next;
	}

	/* if only one entry, select it (so Abort button can work) */
	if (i == 1)
		gtk_clist_select_row (GTK_CLIST (dccswin.list), 0, 0);

	gtk_clist_thaw (GTK_CLIST (dccswin.list));
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
	int * rows;
	int i, num_rows;
	struct DCC *dcc;

	num_rows = gtkutil_clist_multiple_selection (dccswin.list, &rows, 256);
	gtk_clist_freeze( GTK_CLIST(dccswin.list) );

	/* we go from the end to the begining, because if we remove a row it might affect
	the rows that's coming after it */
	for (i = num_rows - 1; i >= 0; i--)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccswin.list), rows[i]);
		if ( dcc )
		{
			dcc_abort (dcc->serv->front_session, dcc);
		}
	}
	gtk_clist_thaw( GTK_CLIST(dccswin.list) );
	free ( rows );
}

int
fe_dcc_open_send_win (int passive)
{
	GtkWidget *vbox, *bbox;
	int i;
	gchar *titles[] =
		{ NULL, NULL, NULL, NULL, NULL, "%", "KB/s", NULL, NULL };

	titles[0] = _("Status");
	titles[1] = _("File");
	titles[2] = _("Size");
	titles[3] = _("Position");
	titles[4] = _("Ack");
	titles[7] = _("ETA");
	titles[8] = _("To");

	if (dccswin.window)
	{
		if (!passive)
			mg_bring_tofront (dccswin.window);
		return TRUE;
	}

	dccswin.window =
			  mg_create_generic_tab ("Uploads", _("XChat: File Send List"),
						FALSE, TRUE, close_dcc_send_window, NULL, 595, 200, &vbox, 0);

	dccswin.list = gtkutil_clist_new (9, titles, vbox, GTK_POLICY_ALWAYS,
												 send_row_selected, 0,
												 0, 0, GTK_SELECTION_SINGLE);

	for(i=0; i < 9; i++)
	{
		gtk_clist_set_column_auto_resize(GTK_CLIST(dccswin.list), i, TRUE);
	}
	gtk_clist_set_selection_mode(GTK_CLIST(dccswin.list), GTK_SELECTION_MULTIPLE);

	gtk_clist_set_column_justification (GTK_CLIST (dccswin.list), 5,
													GTK_JUSTIFY_CENTER);

	bbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
	gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 2);
	gtk_widget_show (bbox);

	gtkutil_button (bbox, GTK_STOCK_CANCEL, 0, abort_send_clicked, 0, _("Abort"));
	gtkutil_button (bbox, GTK_STOCK_DIALOG_INFO, 0, info_send_clicked, 0, _("Info"));

	gtk_widget_show (dccswin.window);
	dcc_update_send_win ();

	return FALSE;
}


/* DCC CHAT GUIs BELOW */

static void
accept_chat_clicked (GtkWidget * wid, gpointer none)
{
	int * rows;
	int i, num_rows;
	struct DCC *dcc;

	num_rows = gtkutil_clist_multiple_selection (dcccwin.list, &rows, 256);
	gtk_clist_freeze( GTK_CLIST(dcccwin.list) );

	/* we go from the end to the begining, because if we remove a row it might affect
	the rows that's coming after it */
	for (i = num_rows - 1; i >= 0; i--)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dcccwin.list), rows[i]);
		if ( dcc )
		{
			gtk_clist_unselect_row (GTK_CLIST (dcccwin.list), rows[i], 0);
			dcc_get (dcc);
		}
	}
	gtk_clist_thaw( GTK_CLIST(dcccwin.list) );
	free ( rows );
}

static void
abort_chat_clicked (GtkWidget * wid, gpointer none)
{
	int * rows;
	int i, num_rows;
	struct DCC *dcc;

	num_rows = gtkutil_clist_multiple_selection (dcccwin.list, &rows, 256);
	gtk_clist_freeze( GTK_CLIST(dcccwin.list) );

	/* we go from the end to the begining, because if we remove a row it might affect
	the rows that's coming after it */
	for (i = num_rows - 1; i >= 0; i--)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dcccwin.list), rows[i]);
		if ( dcc )
		{
			dcc_abort (dcc->serv->front_session, dcc);
		}
	}
	gtk_clist_thaw( GTK_CLIST(dcccwin.list) );
	free ( rows );
}

static void
chat_row_selected (GtkWidget * clist, gint row, gint column,
						 GdkEventButton * even)
{
	if (even && even->type == GDK_2BUTTON_PRESS)
		accept_chat_clicked (0, 0);
}

static void
close_dcc_chat_window (void)
{
	dcccwin.window = 0;
}

static void
dcc_update_chat_win (void)
{
	struct DCC *dcc;
	GSList *list = dcc_list;
	gchar *col[5];
	gint row;

	gtk_clist_freeze (GTK_CLIST (dcccwin.list));
	gtk_clist_clear ((GtkCList *) dcccwin.list);
	while (list)
	{
		dcc = list->data;
		if ((dcc->type == TYPE_CHATSEND || dcc->type == TYPE_CHATRECV))
		{
			dcc_prepare_row_chat (dcc, col);
			row = gtk_clist_append (GTK_CLIST (dcccwin.list), col);
			gtk_clist_set_row_data (GTK_CLIST (dcccwin.list), row, dcc);
		}
		list = list->next;
	}
	gtk_clist_thaw (GTK_CLIST (dcccwin.list));
}

int
fe_dcc_open_chat_win (int passive)
{
	GtkWidget *vbox, *bbox;
	gchar *titles[5];
	int i;

	titles[0] = _("Status");
	titles[1] = _("To/From");
	titles[2] = _("Recv");
	titles[3] = _("Sent");
	titles[4] = _("StartTime");

	if (dcccwin.window)
	{
		if (!passive)
			mg_bring_tofront (dcccwin.window);
		return TRUE;
	}

	dcccwin.window =
			  mg_create_generic_tab ("DCCChat", _("X-Chat: DCC Chat List"),
						FALSE, TRUE, close_dcc_chat_window, NULL, 550, 180, &vbox, 0);

	dcccwin.list = gtkutil_clist_new (5, titles, vbox, GTK_POLICY_ALWAYS,
												 chat_row_selected, 0,
												 0, 0, GTK_SELECTION_BROWSE);

	for(i=0; i < 5; i++)
	{
		gtk_clist_set_column_auto_resize(GTK_CLIST(dcccwin.list), i, TRUE);
	}
	gtk_clist_set_selection_mode(GTK_CLIST(dcccwin.list), GTK_SELECTION_MULTIPLE);

	bbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
	gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 2);
	gtk_widget_show (bbox);

	gtkutil_button (bbox, GTK_STOCK_CANCEL, 0, abort_chat_clicked, 0, _("Abort"));
	gtkutil_button (bbox, GTK_STOCK_APPLY, 0, accept_chat_clicked, 0, _("Accept"));

	gtk_widget_show (dcccwin.window);
	dcc_update_chat_win ();

	return FALSE;
}

void
fe_dcc_add (struct DCC *dcc)
{
	int row;
	char *col[9];

	switch (dcc->type)
	{
	case TYPE_RECV:
		if (!dccrwin.window)
			return;
		dcc_prepare_row_recv (dcc, col);
		row = gtk_clist_prepend (GTK_CLIST (dccrwin.list), col);
		gtk_clist_set_row_data (GTK_CLIST (dccrwin.list), row, dcc);
		if (dccstat[dcc->dccstat].color != 1)
			gtk_clist_set_foreground (GTK_CLIST (dccrwin.list), row,
											  colors + dccstat[dcc->dccstat].color);
		break;

	case TYPE_SEND:
		if (!dccswin.window)
			return;
		dcc_prepare_row_send (dcc, col);
		row = gtk_clist_prepend (GTK_CLIST (dccswin.list), col);
		gtk_clist_set_row_data (GTK_CLIST (dccswin.list), row, dcc);
		if (dccstat[dcc->dccstat].color != 1)
			gtk_clist_set_foreground (GTK_CLIST (dccswin.list), row,
											  colors + dccstat[dcc->dccstat].color);
		break;

	default: /* chat */
		if (!dcccwin.window)
			return;
		dcc_prepare_row_chat (dcc, col);
		row = gtk_clist_prepend (GTK_CLIST (dcccwin.list), col);
		gtk_clist_set_row_data (GTK_CLIST (dcccwin.list), row, dcc);
	}
}

void
fe_dcc_update (struct DCC *dcc)
{
	switch (dcc->type)
	{
	case TYPE_SEND:
		dcc_update_send (dcc);
		break;

	case TYPE_RECV:
		dcc_update_recv (dcc);
		break;

	default:
		dcc_update_chat (dcc);
	}
}

void
fe_dcc_remove (struct DCC *dcc)
{
	GtkCList *list = NULL;
	int row;

	switch (dcc->type)
	{
	case TYPE_SEND:
		if (dccswin.window)
			list = GTK_CLIST (dccswin.list);
		break;

	case TYPE_RECV:
		if (dccrwin.window)
			list = GTK_CLIST (dccrwin.list);
		break;

	default:	/* chat */
		if (dcccwin.window)
			list = GTK_CLIST (dcccwin.list);
		break;
	}

	if (list)
	{
		row = gtk_clist_find_row_from_data (list, dcc);
		if (row != -1)
			gtk_clist_remove (list, row);
	}
}
