/* X-Chat
 * Copyright (C) 1998-2006 Peter Zelezny.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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

#include "../common/hexchat.h"
#include "../common/hexchatc.h"
#include "../common/fe.h"
#include "../common/util.h"
#include "../common/network.h"
#include "gtkutil.h"
#include "palette.h"
#include "maingui.h"


enum	/* DCC SEND/RECV */
{
	COL_TYPE,
	COL_STATUS,
	COL_FILE,
	COL_SIZE,
	COL_POS,
	COL_PERC,
	COL_SPEED,
	COL_ETA,
	COL_NICK,
	COL_DCC, /* struct DCC * */
	COL_COLOR,	/* GdkColor */
	N_COLUMNS
};

enum	/* DCC CHAT */
{
	CCOL_STATUS,
	CCOL_NICK,
	CCOL_RECV,
	CCOL_SENT,
	CCOL_START,
	CCOL_DCC,	/* struct DCC * */
	CCOL_COLOR,	/* GdkColor * */
	CN_COLUMNS
};

struct dccwindow
{
	GtkWidget *window;

	GtkWidget *list;
	GtkListStore *store;
	GtkTreeSelection *sel;

	GtkWidget *abort_button;
	GtkWidget *accept_button;
	GtkWidget *resume_button;
	GtkWidget *open_button;
	GtkWidget *clear_button; /* clears aborted and completed requests */	

	GtkWidget *file_label;
	GtkWidget *address_label;
};

struct my_dcc_send
{
	struct session *sess;
	char *nick;
	gint64 maxcps;
	int passive;
};

static struct dccwindow dccfwin = {NULL, };	/* file */
static struct dccwindow dcccwin = {NULL, };	/* chat */
static GdkPixbuf *pix_up = NULL;	/* down arrow */
static GdkPixbuf *pix_dn = NULL;	/* up arrow */
static int win_width = 600;
static int win_height = 256;
static short view_mode;	/* 1=download 2=upload 3=both */
#define VIEW_DOWNLOAD 1
#define VIEW_UPLOAD 2
#define VIEW_BOTH 3


static void
proper_unit (guint64 size, char *buf, size_t buf_len)
{
	gchar *formatted_str;
	GFormatSizeFlags format_flags = G_FORMAT_SIZE_DEFAULT;

#ifndef __APPLE__ /* OS X uses SI */
#ifndef WIN32 /* Windows uses IEC size (with SI format) */
	if (prefs.hex_gui_filesize_iec) /* Linux can't decide... */
#endif
		format_flags = G_FORMAT_SIZE_IEC_UNITS;
#endif

	formatted_str = g_format_size_full (size, format_flags);
	g_strlcpy (buf, formatted_str, buf_len);

	g_free (formatted_str);
}

static void
dcc_send_filereq_file (struct my_dcc_send *mdc, char *file)
{
	if (file)
		dcc_send (mdc->sess, mdc->nick, file, mdc->maxcps, mdc->passive);
	else
	{
		g_free (mdc->nick);
		g_free (mdc);
	}
}

void
fe_dcc_send_filereq (struct session *sess, char *nick, int maxcps, int passive)
{
	char* tbuf = g_strdup_printf (_("Send file to %s"), nick);

	struct my_dcc_send *mdc = g_new (struct my_dcc_send, 1);
	mdc->sess = sess;
	mdc->nick = g_strdup (nick);
	mdc->maxcps = maxcps;
	mdc->passive = passive;

	gtkutil_file_req (NULL, tbuf, dcc_send_filereq_file, mdc, prefs.hex_dcc_dir, NULL, FRF_MULTIPLE|FRF_FILTERISINITIAL);

	g_free (tbuf);
}

static void
dcc_prepare_row_chat (struct DCC *dcc, GtkListStore *store, GtkTreeIter *iter,
							 gboolean update_only)
{
	static char pos[16], size[16];
	char *date;

	date = ctime (&dcc->starttime);
	date[strlen (date) - 1] = 0;	/* remove the \n */

	proper_unit (dcc->pos, pos, sizeof (pos));
	proper_unit (dcc->size, size, sizeof (size));

	gtk_list_store_set (store, iter,
							  CCOL_STATUS, _(dccstat[dcc->dccstat].name),
							  CCOL_NICK, dcc->nick,
							  CCOL_RECV, pos,
							  CCOL_SENT, size,
							  CCOL_START, date,
							  CCOL_DCC, dcc,
							  CCOL_COLOR,
							  dccstat[dcc->dccstat].color == 1 ?
								NULL :
								colors + dccstat[dcc->dccstat].color,
							  -1);
}

static void
dcc_prepare_row_send (struct DCC *dcc, GtkListStore *store, GtkTreeIter *iter,
							 gboolean update_only)
{
	static char pos[16], size[16], kbs[14], perc[14], eta[14];
	int to_go;
	float per;

	if (!pix_up)
		pix_up = gtk_widget_render_icon (dccfwin.window, "gtk-go-up",
													GTK_ICON_SIZE_MENU, NULL);

	/* percentage ack'ed */
	per = (float) ((dcc->ack * 100.00) / dcc->size);
	proper_unit (dcc->size, size, sizeof (size));
	proper_unit (dcc->pos, pos, sizeof (pos));
	g_snprintf (kbs, sizeof (kbs), "%.1f", ((float)dcc->cps) / 1024);
	g_snprintf (perc, sizeof (perc), "%.0f%%", per);
	if (dcc->cps != 0)
	{
		to_go = (dcc->size - dcc->ack) / dcc->cps;
		g_snprintf (eta, sizeof (eta), "%.2d:%.2d:%.2d",
					 to_go / 3600, (to_go / 60) % 60, to_go % 60);
	} else
		strcpy (eta, "--:--:--");

	if (update_only)
		gtk_list_store_set (store, iter,
								  COL_STATUS, _(dccstat[dcc->dccstat].name),
								  COL_POS, pos,
								  COL_PERC, perc,
								  COL_SPEED, kbs,
								  COL_ETA, eta,
								  COL_COLOR,
								  dccstat[dcc->dccstat].color == 1 ?
									NULL :
									colors + dccstat[dcc->dccstat].color,
									-1);
	else
		gtk_list_store_set (store, iter,
								  COL_TYPE, pix_up,
								  COL_STATUS, _(dccstat[dcc->dccstat].name),
								  COL_FILE, file_part (dcc->file),
								  COL_SIZE, size,
								  COL_POS, pos,
								  COL_PERC, perc,
								  COL_SPEED, kbs,
								  COL_ETA, eta,
								  COL_NICK, dcc->nick,
								  COL_DCC, dcc,
								  COL_COLOR,
								  dccstat[dcc->dccstat].color == 1 ?
									NULL :
									colors + dccstat[dcc->dccstat].color,
									-1);
}

static void
dcc_prepare_row_recv (struct DCC *dcc, GtkListStore *store, GtkTreeIter *iter,
							 gboolean update_only)
{
	static char size[16], pos[16], kbs[16], perc[14], eta[16];
	float per;
	int to_go;

	if (!pix_dn)
		pix_dn = gtk_widget_render_icon (dccfwin.window, "gtk-go-down",
													GTK_ICON_SIZE_MENU, NULL);

	proper_unit (dcc->size, size, sizeof (size));
	if (dcc->dccstat == STAT_QUEUED)
		proper_unit (dcc->resumable, pos, sizeof (pos));
	else
		proper_unit (dcc->pos, pos, sizeof (pos));
	g_snprintf (kbs, sizeof (kbs), "%.1f", ((float)dcc->cps) / 1024);
	/* percentage recv'ed */
	per = (float) ((dcc->pos * 100.00) / dcc->size);
	g_snprintf (perc, sizeof (perc), "%.0f%%", per);
	if (dcc->cps != 0)
	{
		to_go = (dcc->size - dcc->pos) / dcc->cps;
		g_snprintf (eta, sizeof (eta), "%.2d:%.2d:%.2d",
					 to_go / 3600, (to_go / 60) % 60, to_go % 60);
	} else
		strcpy (eta, "--:--:--");

	if (update_only)
		gtk_list_store_set (store, iter,
								  COL_STATUS, _(dccstat[dcc->dccstat].name),
								  COL_POS, pos,
								  COL_PERC, perc,
								  COL_SPEED, kbs,
								  COL_ETA, eta,
								  COL_COLOR,
								  dccstat[dcc->dccstat].color == 1 ?
									NULL :
									colors + dccstat[dcc->dccstat].color,
									-1);
	else
		gtk_list_store_set (store, iter,
								  COL_TYPE, pix_dn,
								  COL_STATUS, _(dccstat[dcc->dccstat].name),
								  COL_FILE, file_part (dcc->file),
								  COL_SIZE, size,
								  COL_POS, pos,
								  COL_PERC, perc,
								  COL_SPEED, kbs,
								  COL_ETA, eta,
								  COL_NICK, dcc->nick,
								  COL_DCC, dcc,
								  COL_COLOR,
								  dccstat[dcc->dccstat].color == 1 ?
									NULL :
									colors + dccstat[dcc->dccstat].color,
									-1);
}

static gboolean
dcc_find_row (struct DCC *find_dcc, GtkTreeModel *model, GtkTreeIter *iter, int col)
{
	struct DCC *dcc;

	if (gtk_tree_model_get_iter_first (model, iter))
	{
		do
		{
			gtk_tree_model_get (model, iter, col, &dcc, -1);
			if (dcc == find_dcc)
				return TRUE;
		}
		while (gtk_tree_model_iter_next (model, iter));
	}

	return FALSE;
}

static void
dcc_update_recv (struct DCC *dcc)
{
	GtkTreeIter iter;

	if (!dccfwin.window)
		return;

	if (!dcc_find_row (dcc, GTK_TREE_MODEL (dccfwin.store), &iter, COL_DCC))
		return;

	dcc_prepare_row_recv (dcc, dccfwin.store, &iter, TRUE);
}

static void
dcc_update_chat (struct DCC *dcc)
{
	GtkTreeIter iter;

	if (!dcccwin.window)
		return;

	if (!dcc_find_row (dcc, GTK_TREE_MODEL (dcccwin.store), &iter, CCOL_DCC))
		return;

	dcc_prepare_row_chat (dcc, dcccwin.store, &iter, TRUE);
}

static void
dcc_update_send (struct DCC *dcc)
{
	GtkTreeIter iter;

	if (!dccfwin.window)
		return;

	if (!dcc_find_row (dcc, GTK_TREE_MODEL (dccfwin.store), &iter, COL_DCC))
		return;

	dcc_prepare_row_send (dcc, dccfwin.store, &iter, TRUE);
}

static void
close_dcc_file_window (GtkWindow *win, gpointer data)
{
	dccfwin.window = NULL;
}

static void
dcc_append (struct DCC *dcc, GtkListStore *store, gboolean prepend)
{
	GtkTreeIter iter;

	if (prepend)
		gtk_list_store_prepend (store, &iter);
	else
		gtk_list_store_append (store, &iter);

	if (dcc->type == TYPE_RECV)
		dcc_prepare_row_recv (dcc, store, &iter, FALSE);
	else
		dcc_prepare_row_send (dcc, store, &iter, FALSE);
}

/* Returns aborted and completed transfers. */
static GSList *
dcc_get_completed (void)
{
	struct DCC *dcc;
	GtkTreeIter iter;
	GtkTreeModel *model;	
	GSList *completed = NULL;

	model = GTK_TREE_MODEL (dccfwin.store);
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			gtk_tree_model_get (model, &iter, COL_DCC, &dcc, -1);
			if (is_dcc_completed (dcc))
				completed = g_slist_prepend (completed, dcc);
				
		} while (gtk_tree_model_iter_next (model, &iter));
	}

	return completed;
}

static gboolean
dcc_completed_transfer_exists (void)
{
	gboolean exist;
	GSList *comp_list;
	
	comp_list = dcc_get_completed (); 
	exist = comp_list != NULL;
	
	g_slist_free (comp_list);	
	return exist;
}

static void
update_clear_button_sensitivity (void)
{
	gboolean sensitive = dcc_completed_transfer_exists ();
	gtk_widget_set_sensitive (dccfwin.clear_button, sensitive);
}

static void
dcc_fill_window (int flags)
{
	struct DCC *dcc;
	GSList *list;
	GtkTreeIter iter;
	int i = 0;

	gtk_list_store_clear (GTK_LIST_STORE (dccfwin.store));

	if (flags & VIEW_UPLOAD)
	{
		list = dcc_list;
		while (list)
		{
			dcc = list->data;
			if (dcc->type == TYPE_SEND)
			{
				dcc_append (dcc, dccfwin.store, FALSE);
				i++;
			}
			list = list->next;
		}
	}

	if (flags & VIEW_DOWNLOAD)
	{
		list = dcc_list;
		while (list)
		{
			dcc = list->data;
			if (dcc->type == TYPE_RECV)
			{
				dcc_append (dcc, dccfwin.store, FALSE);
				i++;
			}
			list = list->next;
		}
	}

	/* if only one entry, select it (so Accept button can work) */
	if (i == 1)
	{
		gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dccfwin.store), &iter);
		gtk_tree_selection_select_iter (dccfwin.sel, &iter);
	}
	
	update_clear_button_sensitivity ();
}

/* return list of selected DCCs */

static GSList *
treeview_get_selected (GtkTreeModel *model, GtkTreeSelection *sel, int column)
{
	GtkTreeIter iter;
	GSList *list = NULL;
	void *ptr;

	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			if (gtk_tree_selection_iter_is_selected (sel, &iter))
			{
				gtk_tree_model_get (model, &iter, column, &ptr, -1);
				list = g_slist_prepend (list, ptr);
			}
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}

	return g_slist_reverse (list);
}

static GSList *
dcc_get_selected (void)
{
	return treeview_get_selected (GTK_TREE_MODEL (dccfwin.store),
											dccfwin.sel, COL_DCC);
}

static void
resume_clicked (GtkWidget * wid, gpointer none)
{
	struct DCC *dcc;
	char buf[512];
	GSList *list;

	list = dcc_get_selected ();
	if (!list)
		return;
	dcc = list->data;
	g_slist_free (list);

	if (dcc->type == TYPE_RECV && !dcc_resume (dcc))
	{
		switch (dcc->resume_error)
		{
		case 0:	/* unknown error */
			fe_message (_("That file is not resumable."), FE_MSG_ERROR);
			break;
		case 1:
			g_snprintf (buf, sizeof (buf),
						_(	"Cannot access file: %s\n"
							"%s.\n"
							"Resuming not possible."), dcc->destfile,	
							errorstring (dcc->resume_errno));
			fe_message (buf, FE_MSG_ERROR);
			break;
		case 2:
			fe_message (_("File in download directory is larger "
							"than file offered. Resuming not possible."), FE_MSG_ERROR);
			break;
		case 3:
			fe_message (_("Cannot resume the same file from two people."), FE_MSG_ERROR);
		}
	}
}

static void
abort_clicked (GtkWidget * wid, gpointer none)
{
	struct DCC *dcc;
	GSList *start, *list;

	start = list = dcc_get_selected ();
	for (; list; list = list->next)
	{
		dcc = list->data;
		dcc_abort (dcc->serv->front_session, dcc);
	}
	g_slist_free (start);
	
	/* Enable the clear button if it wasn't already enabled */
	update_clear_button_sensitivity ();
}

static void
accept_clicked (GtkWidget * wid, gpointer none)
{
	struct DCC *dcc;
	GSList *start, *list;

	start = list = dcc_get_selected ();
	for (; list; list = list->next)
	{
		dcc = list->data;
		if (dcc->type != TYPE_SEND)
			dcc_get (dcc);
	}
	g_slist_free (start);
}

static void
clear_completed (GtkWidget * wid, gpointer none)
{
	struct DCC *dcc;
	GSList *completed;

	/* Make a new list of only the completed items and abort each item.
	 * A new list is made because calling dcc_abort removes items from the original list,
	 * making it impossible to iterate over that list directly.
	*/
	for (completed = dcc_get_completed (); completed; completed = completed->next)
	{
		dcc = completed->data;
		dcc_abort (dcc->serv->front_session, dcc);
	}

	/* The data was freed by dcc_close */
	g_slist_free (completed);
	update_clear_button_sensitivity ();
}

static void
browse_folder (char *dir)
{
#ifdef WIN32
	/* no need for file:// in ShellExecute() */
	fe_open_url (dir);
#else
	char buf[512];

	g_snprintf (buf, sizeof (buf), "file://%s", dir);
	fe_open_url (buf);
#endif
}

static void
browse_dcc_folder (void)
{
	if (prefs.hex_dcc_completed_dir[0])
		browse_folder (prefs.hex_dcc_completed_dir);
	else
		browse_folder (prefs.hex_dcc_dir);
}

static void
dcc_details_populate (struct DCC *dcc)
{
	char buf[128];

	if (!dcc)
	{
		gtk_label_set_text (GTK_LABEL (dccfwin.file_label), NULL);
		gtk_label_set_text (GTK_LABEL (dccfwin.address_label), NULL);
		return;
	}

	/* full path */
	if (dcc->type == TYPE_RECV)
		gtk_label_set_text (GTK_LABEL (dccfwin.file_label), dcc->destfile);
	else
		gtk_label_set_text (GTK_LABEL (dccfwin.file_label), dcc->file);

	/* address and port */
	g_snprintf (buf, sizeof (buf), "%s : %d", net_ip (dcc->addr), dcc->port);
	gtk_label_set_text (GTK_LABEL (dccfwin.address_label), buf);
}

static void
dcc_row_cb (GtkTreeSelection *sel, gpointer user_data)
{
	struct DCC *dcc;
	GSList *list;

	list = dcc_get_selected ();
	if (!list)
	{
		gtk_widget_set_sensitive (dccfwin.accept_button, FALSE);
		gtk_widget_set_sensitive (dccfwin.resume_button, FALSE);
		gtk_widget_set_sensitive (dccfwin.abort_button, FALSE);
		dcc_details_populate (NULL);
		return;
	}

	gtk_widget_set_sensitive (dccfwin.abort_button, TRUE);

	if (list->next)	/* multi selection */
	{
		gtk_widget_set_sensitive (dccfwin.accept_button, TRUE);
		gtk_widget_set_sensitive (dccfwin.resume_button, TRUE);
		dcc_details_populate (list->data);
	}
	else
	{
		/* turn OFF/ON appropriate buttons */
		dcc = list->data;
		if (dcc->dccstat == STAT_QUEUED && dcc->type == TYPE_RECV)
		{
			gtk_widget_set_sensitive (dccfwin.accept_button, TRUE);
			gtk_widget_set_sensitive (dccfwin.resume_button, TRUE);
		}
		else
		{
			gtk_widget_set_sensitive (dccfwin.accept_button, FALSE);
			gtk_widget_set_sensitive (dccfwin.resume_button, FALSE);
		}

		dcc_details_populate (dcc);
	}

	g_slist_free (list);
}

static void
dcc_dclick_cb (GtkTreeView *view, GtkTreePath *path,
					GtkTreeViewColumn *column, gpointer data)
{
	struct DCC *dcc;
	GSList *list;

	list = dcc_get_selected ();
	if (!list)
		return;
	dcc = list->data;
	g_slist_free (list);

	if (dcc->type == TYPE_RECV)
	{
		accept_clicked (0, 0);
		return;
	}

	switch (dcc->dccstat)
	{
	case STAT_FAILED:
	case STAT_ABORTED:
	case STAT_DONE:
		dcc_abort (dcc->serv->front_session, dcc);
		break;
	case STAT_QUEUED:
	case STAT_ACTIVE:
	case STAT_CONNECTING:
		break;
	}
}

static void
dcc_add_column (GtkWidget *tree, int textcol, int colorcol, char *title, gboolean right_justified)
{
	GtkCellRenderer *renderer;

	renderer = gtk_cell_renderer_text_new ();
	if (right_justified)
		g_object_set (G_OBJECT (renderer), "xalign", (float) 1.0, NULL);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tree), -1, title, renderer,
																"text", textcol, "foreground-gdk", colorcol,
																NULL);
	gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT (renderer), 1);
}

static GtkWidget *
dcc_detail_label (char *text, GtkWidget *box, int num)
{
	GtkWidget *label;
	char buf[64];

	label = gtk_label_new (NULL);
	g_snprintf (buf, sizeof (buf), "<b>%s</b>", text);
	gtk_label_set_markup (GTK_LABEL (label), buf);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_table_attach (GTK_TABLE (box), label, 0, 1, 0 + num, 1 + num, GTK_FILL, GTK_FILL, 0, 0);

	label = gtk_label_new (NULL);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_table_attach (GTK_TABLE (box), label, 1, 2, 0 + num, 1 + num, GTK_FILL, GTK_FILL, 0, 0);

	return label;
}

static void
dcc_exp_cb (GtkWidget *exp, GtkWidget *box)
{
	if (gtk_widget_get_visible (box))
	{
		gtk_widget_hide (box);
	}
	else
	{
		gtk_widget_show (box);
	}
}

static void
dcc_toggle (GtkWidget *item, gpointer data)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (item)))
	{
		view_mode = GPOINTER_TO_INT (data);
		dcc_fill_window (GPOINTER_TO_INT (data));
	}
}

static gboolean
dcc_configure_cb (GtkWindow *win, GdkEventConfigure *event, gpointer data)
{
	/* remember the window size */
	gtk_window_get_size (win, &win_width, &win_height);
	return FALSE;
}

int
fe_dcc_open_recv_win (int passive)
{
	GtkWidget *radio, *table, *vbox, *bbox, *view, *exp, *detailbox;
	GtkListStore *store;
	GSList *group;
	char buf[128];

	if (dccfwin.window)
	{
		if (!passive)
			mg_bring_tofront (dccfwin.window);
		return TRUE;
	}
	g_snprintf(buf, sizeof(buf), _("Uploads and Downloads - %s"), _(DISPLAY_NAME));
	dccfwin.window = mg_create_generic_tab ("Transfers", buf, FALSE, TRUE, close_dcc_file_window,
														 NULL, win_width, win_height, &vbox, 0);
	gtkutil_destroy_on_esc (dccfwin.window);
	gtk_container_set_border_width (GTK_CONTAINER (dccfwin.window), 3);
	gtk_box_set_spacing (GTK_BOX (vbox), 3);

	store = gtk_list_store_new (N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING,
										 G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
										 G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
										 G_TYPE_STRING, G_TYPE_POINTER, GDK_TYPE_COLOR);
	view = gtkutil_treeview_new (vbox, GTK_TREE_MODEL (store), NULL, -1);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (view), TRUE);
	/* Up/Down Icon column */
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), -1, NULL,
																gtk_cell_renderer_pixbuf_new (),
																"pixbuf", COL_TYPE, NULL);
	dcc_add_column (view, COL_STATUS, COL_COLOR, _("Status"), FALSE);
	dcc_add_column (view, COL_FILE,   COL_COLOR, _("File"), FALSE);
	dcc_add_column (view, COL_SIZE,   COL_COLOR, _("Size"), TRUE);
	dcc_add_column (view, COL_POS,    COL_COLOR, _("Position"), TRUE);
	dcc_add_column (view, COL_PERC,   COL_COLOR, "%", TRUE);
	dcc_add_column (view, COL_SPEED,  COL_COLOR, "KB/s", TRUE);
	dcc_add_column (view, COL_ETA,    COL_COLOR, _("ETA"), FALSE);
	dcc_add_column (view, COL_NICK,   COL_COLOR, _("Nick"), FALSE);

	gtk_tree_view_column_set_expand (gtk_tree_view_get_column (GTK_TREE_VIEW (view), COL_FILE), TRUE);
	gtk_tree_view_column_set_expand (gtk_tree_view_get_column (GTK_TREE_VIEW (view), COL_NICK), TRUE);

	dccfwin.list = view;
	dccfwin.store = store;
	dccfwin.sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	view_mode = VIEW_BOTH;
	gtk_tree_selection_set_mode (dccfwin.sel, GTK_SELECTION_MULTIPLE);

	if (!prefs.hex_gui_tab_utils)
		g_signal_connect (G_OBJECT (dccfwin.window), "configure_event",
								G_CALLBACK (dcc_configure_cb), 0);
	g_signal_connect (G_OBJECT (dccfwin.sel), "changed",
							G_CALLBACK (dcc_row_cb), NULL);
	/* double click */
	g_signal_connect (G_OBJECT (view), "row-activated",
							G_CALLBACK (dcc_dclick_cb), NULL);

	table = gtk_table_new (1, 3, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 16);
	gtk_box_pack_start (GTK_BOX (vbox), table, 0, 0, 0);

	radio = gtk_radio_button_new_with_mnemonic (NULL, _("Both"));
	g_signal_connect (G_OBJECT (radio), "toggled",
							G_CALLBACK (dcc_toggle), GINT_TO_POINTER (VIEW_BOTH));
	gtk_table_attach (GTK_TABLE (table), radio, 3, 4, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));

	radio = gtk_radio_button_new_with_mnemonic (group, _("Uploads"));
	g_signal_connect (G_OBJECT (radio), "toggled",
							G_CALLBACK (dcc_toggle), GINT_TO_POINTER (VIEW_UPLOAD));
	gtk_table_attach (GTK_TABLE (table), radio, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));

	radio = gtk_radio_button_new_with_mnemonic (group, _("Downloads"));
	g_signal_connect (G_OBJECT (radio), "toggled",
							G_CALLBACK (dcc_toggle), GINT_TO_POINTER (VIEW_DOWNLOAD));
	gtk_table_attach (GTK_TABLE (table), radio, 2, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

	exp = gtk_expander_new (_("Details"));
	gtk_table_attach (GTK_TABLE (table), exp, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

	detailbox = gtk_table_new (3, 3, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (detailbox), 6);
	gtk_table_set_row_spacings (GTK_TABLE (detailbox), 2);
	gtk_container_set_border_width (GTK_CONTAINER (detailbox), 6);
	g_signal_connect (G_OBJECT (exp), "activate",
							G_CALLBACK (dcc_exp_cb), detailbox);
	gtk_table_attach (GTK_TABLE (table), detailbox, 0, 4, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

	dccfwin.file_label = dcc_detail_label (_("File:"), detailbox, 0);
	dccfwin.address_label = dcc_detail_label (_("Address:"), detailbox, 1);

	bbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
	gtk_box_pack_end (GTK_BOX (vbox), bbox, FALSE, FALSE, 2);

	dccfwin.abort_button = gtkutil_button (bbox, GTK_STOCK_CANCEL, 0, abort_clicked, 0, _("Abort"));
	dccfwin.accept_button = gtkutil_button (bbox, GTK_STOCK_APPLY, 0, accept_clicked, 0, _("Accept"));
	dccfwin.resume_button = gtkutil_button (bbox, GTK_STOCK_REFRESH, 0, resume_clicked, 0, _("Resume"));
	dccfwin.clear_button = gtkutil_button (bbox, GTK_STOCK_CLEAR, 0, clear_completed, 0, _("Clear"));
	dccfwin.open_button = gtkutil_button (bbox, 0, 0, browse_dcc_folder, 0, _("Open Folder..."));
	gtk_widget_set_sensitive (dccfwin.accept_button, FALSE);
	gtk_widget_set_sensitive (dccfwin.resume_button, FALSE);
	gtk_widget_set_sensitive (dccfwin.abort_button, FALSE);

	dcc_fill_window (3);
	gtk_widget_show_all (dccfwin.window);
	gtk_widget_hide (detailbox);

	return FALSE;
}

int
fe_dcc_open_send_win (int passive)
{
	/* combined send/recv GUI */
	return fe_dcc_open_recv_win (passive);
}


/* DCC CHAT GUIs BELOW */

static GSList *
dcc_chat_get_selected (void)
{
	return treeview_get_selected (GTK_TREE_MODEL (dcccwin.store),
											dcccwin.sel, CCOL_DCC);
}

static void
accept_chat_clicked (GtkWidget * wid, gpointer none)
{
	struct DCC *dcc;
	GSList *start, *list;

	start = list = dcc_chat_get_selected ();
	for (; list; list = list->next)
	{
		dcc = list->data;
		dcc_get (dcc);
	}
	g_slist_free (start);
}

static void
abort_chat_clicked (GtkWidget * wid, gpointer none)
{
	struct DCC *dcc;
	GSList *start, *list;

	start = list = dcc_chat_get_selected ();
	for (; list; list = list->next)
	{
		dcc = list->data;
		dcc_abort (dcc->serv->front_session, dcc);
	}
	g_slist_free (start);
}

static void
dcc_chat_close_cb (void)
{
	dcccwin.window = NULL;
}

static void
dcc_chat_append (struct DCC *dcc, GtkListStore *store, gboolean prepend)
{
	GtkTreeIter iter;

	if (prepend)
		gtk_list_store_prepend (store, &iter);
	else
		gtk_list_store_append (store, &iter);

	dcc_prepare_row_chat (dcc, store, &iter, FALSE);
}

static void
dcc_chat_fill_win (void)
{
	struct DCC *dcc;
	GSList *list;
	GtkTreeIter iter;
	int i = 0;

	gtk_list_store_clear (GTK_LIST_STORE (dcccwin.store));

	list = dcc_list;
	while (list)
	{
		dcc = list->data;
		if (dcc->type == TYPE_CHATSEND || dcc->type == TYPE_CHATRECV)
		{
			dcc_chat_append (dcc, dcccwin.store, FALSE);
			i++;
		}
		list = list->next;
	}

	/* if only one entry, select it (so Accept button can work) */
	if (i == 1)
	{
		gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dcccwin.store), &iter);
		gtk_tree_selection_select_iter (dcccwin.sel, &iter);
	}
}

static void
dcc_chat_row_cb (GtkTreeSelection *sel, gpointer user_data)
{
	struct DCC *dcc;
	GSList *list;

	list = dcc_chat_get_selected ();
	if (!list)
	{
		gtk_widget_set_sensitive (dcccwin.accept_button, FALSE);
		gtk_widget_set_sensitive (dcccwin.abort_button, FALSE);
		return;
	}

	gtk_widget_set_sensitive (dcccwin.abort_button, TRUE);

	if (list->next)	/* multi selection */
		gtk_widget_set_sensitive (dcccwin.accept_button, TRUE);
	else
	{
		/* turn OFF/ON appropriate buttons */
		dcc = list->data;
		if (dcc->dccstat == STAT_QUEUED && dcc->type == TYPE_CHATRECV)
			gtk_widget_set_sensitive (dcccwin.accept_button, TRUE);
		else
			gtk_widget_set_sensitive (dcccwin.accept_button, FALSE);
	}

	g_slist_free (list);
}

static void
dcc_chat_dclick_cb (GtkTreeView *view, GtkTreePath *path,
						  GtkTreeViewColumn *column, gpointer data)
{
	accept_chat_clicked (0, 0);
}

int
fe_dcc_open_chat_win (int passive)
{
	GtkWidget *view, *vbox, *bbox;
	GtkListStore *store;
	char buf[128];

	if (dcccwin.window)
	{
		if (!passive)
			mg_bring_tofront (dcccwin.window);
		return TRUE;
	}

	g_snprintf(buf, sizeof(buf), _("DCC Chat List - %s"), _(DISPLAY_NAME));
	dcccwin.window =
			  mg_create_generic_tab ("DCCChat", buf, FALSE, TRUE, dcc_chat_close_cb,
						NULL, 550, 180, &vbox, 0);
	gtkutil_destroy_on_esc (dcccwin.window);
	gtk_container_set_border_width (GTK_CONTAINER (dcccwin.window), 3);
	gtk_box_set_spacing (GTK_BOX (vbox), 3);

	store = gtk_list_store_new (CN_COLUMNS, G_TYPE_STRING, G_TYPE_STRING,
										 G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
										 G_TYPE_POINTER, GDK_TYPE_COLOR);
	view = gtkutil_treeview_new (vbox, GTK_TREE_MODEL (store), NULL, -1);

	dcc_add_column (view, CCOL_STATUS, CCOL_COLOR, _("Status"), FALSE);
	dcc_add_column (view, CCOL_NICK,   CCOL_COLOR, _("Nick"), FALSE);
	dcc_add_column (view, CCOL_RECV,   CCOL_COLOR, _("Recv"), TRUE);
	dcc_add_column (view, CCOL_SENT,   CCOL_COLOR, _("Sent"), TRUE);
	dcc_add_column (view, CCOL_START,  CCOL_COLOR, _("Start Time"), FALSE);

	gtk_tree_view_column_set_expand (gtk_tree_view_get_column (GTK_TREE_VIEW (view), 1), TRUE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (view), TRUE);

	dcccwin.list = view;
	dcccwin.store = store;
	dcccwin.sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	gtk_tree_selection_set_mode (dcccwin.sel, GTK_SELECTION_MULTIPLE);

	g_signal_connect (G_OBJECT (dcccwin.sel), "changed",
							G_CALLBACK (dcc_chat_row_cb), NULL);
	/* double click */
	g_signal_connect (G_OBJECT (view), "row-activated",
							G_CALLBACK (dcc_chat_dclick_cb), NULL);

	bbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
	gtk_box_pack_end (GTK_BOX (vbox), bbox, FALSE, FALSE, 2);

	dcccwin.abort_button = gtkutil_button (bbox, GTK_STOCK_CANCEL, 0, abort_chat_clicked, 0, _("Abort"));
	dcccwin.accept_button = gtkutil_button (bbox, GTK_STOCK_APPLY, 0, accept_chat_clicked, 0, _("Accept"));
	gtk_widget_set_sensitive (dcccwin.accept_button, FALSE);
	gtk_widget_set_sensitive (dcccwin.abort_button, FALSE);

	dcc_chat_fill_win ();
	gtk_widget_show_all (dcccwin.window);

	return FALSE;
}

void
fe_dcc_add (struct DCC *dcc)
{
	switch (dcc->type)
	{
	case TYPE_RECV:
		if (dccfwin.window && (view_mode & VIEW_DOWNLOAD))
			dcc_append (dcc, dccfwin.store, TRUE);
		break;

	case TYPE_SEND:
		if (dccfwin.window && (view_mode & VIEW_UPLOAD))
			dcc_append (dcc, dccfwin.store, TRUE);
		break;

	default: /* chat */
		if (dcccwin.window)
			dcc_chat_append (dcc, dcccwin.store, TRUE);
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

	if (dccfwin.window)
		update_clear_button_sensitivity();
}

void
fe_dcc_remove (struct DCC *dcc)
{
	GtkTreeIter iter;

	switch (dcc->type)
	{
	case TYPE_SEND:
	case TYPE_RECV:
		if (dccfwin.window)
		{
			if (dcc_find_row (dcc, GTK_TREE_MODEL (dccfwin.store), &iter, COL_DCC))
				gtk_list_store_remove (dccfwin.store, &iter);
		}
		break;

	default:	/* chat */
		if (dcccwin.window)
		{
			if (dcc_find_row (dcc, GTK_TREE_MODEL (dcccwin.store), &iter, CCOL_DCC))
				gtk_list_store_remove (dcccwin.store, &iter);
		}
		break;
	}
}
