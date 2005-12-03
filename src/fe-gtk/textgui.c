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
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fe-gtk.h"

#include <gtk/gtkbutton.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkvpaned.h>
#include <gtk/gtkvscrollbar.h>

#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../common/cfgfiles.h"
#include "../common/outbound.h"
#include "../common/fe.h"
#include "../common/text.h"
#include "gtkutil.h"
#include "xtext.h"
#include "maingui.h"
#include "palette.h"
#include "textgui.h"

extern struct text_event te[];
extern char *pntevts_text[];
extern char *pntevts[];

static GtkWidget *pevent_dialog = NULL, *pevent_dialog_twid,
	*pevent_dialog_entry,
	*pevent_dialog_list, *pevent_dialog_hlist;

enum
{
	COL_EVENT_NAME,
	COL_EVENT_TEXT,
	COL_ROW,
	N_COLUMNS
};


/* this is only used in xtext.c for indented timestamping */
int
xtext_get_stamp_str (time_t tim, char **ret)
{
	return get_stamp_str (prefs.stamp_format, tim, ret);
}

static void
PrintTextLine (xtext_buffer *xtbuf, unsigned char *text, int len, int indent)
{
	unsigned char *tab, *new_text;
	int leftlen;

	if (len == 0)
		len = 1;

	if (!indent)
	{
		if (prefs.timestamp)
		{
			int stamp_size;
			char *stamp;

			stamp_size = get_stamp_str (prefs.stamp_format, time (0), &stamp);
			new_text = malloc (len + stamp_size + 1);
			memcpy (new_text, stamp, stamp_size);
			g_free (stamp);
			memcpy (new_text + stamp_size, text, len);
			gtk_xtext_append (xtbuf, new_text, len + stamp_size);
			free (new_text);
		} else
			gtk_xtext_append (xtbuf, text, len);
		return;
	}

	tab = strchr (text, '\t');
	if (tab && tab < (text + len))
	{
		leftlen = tab - text;
		gtk_xtext_append_indent (xtbuf,
										 text, leftlen, tab + 1, len - (leftlen + 1));
	} else
		gtk_xtext_append_indent (xtbuf, 0, 0, text, len);
}

void
PrintTextRaw (void *xtbuf, unsigned char *text, int indent)
{
	char *last_text = text;
	int len = 0;
	int beep_done = FALSE;

	/* split the text into separate lines */
	while (1)
	{
		switch (*text)
		{
		case 0:
			PrintTextLine (xtbuf, last_text, len, indent);
			return;
		case '\n':
			PrintTextLine (xtbuf, last_text, len, indent);
			text++;
			if (*text == 0)
				return;
			last_text = text;
			len = 0;
			break;
		case ATTR_BEEP:
			*text = ' ';
			if (!beep_done) /* beeps may be slow, so only do 1 per line */
			{
				beep_done = TRUE;
				if (!prefs.filterbeep)
					gdk_beep ();
			}
		default:
			text++;
			len++;
		}
	}
}

static void
pevent_dialog_close (GtkWidget *wid, gpointer arg)
{
	pevent_dialog = NULL;
	pevent_save (NULL);
}

static void
pevent_dialog_update (GtkWidget * wid, GtkWidget * twid)
{
	int len, m;
	const char *text;
	char *out;
	int sig;
	GtkTreeIter iter;
	GtkListStore *store;

	if (!gtkutil_treeview_get_selected (GTK_TREE_VIEW (pevent_dialog_list),
													&iter, COL_ROW, &sig, -1))
		return;

	text = gtk_entry_get_text (GTK_ENTRY (wid));
	len = strlen (text);

	if (pevt_build_string (text, &out, &m) != 0)
	{
		fe_message (_("There was an error parsing the string"), FE_MSG_ERROR);
		return;
	}
	if (m > te[sig].num_args)
	{
		free (out);
		out = malloc (4096);
		snprintf (out, 4096,
					 _("This signal is only passed %d args, $%d is invalid"),
					 te[sig].num_args, m);
		fe_message (out, FE_MSG_WARN);
		free (out);
		return;
	}

	store = (GtkListStore *)gtk_tree_view_get_model (GTK_TREE_VIEW (pevent_dialog_list));
	gtk_list_store_set (store, &iter, COL_EVENT_TEXT, text, -1);

	if (pntevts_text[sig])
		free (pntevts_text[sig]);
	if (pntevts[sig])
		free (pntevts[sig]);

	pntevts_text[sig] = malloc (len + 1);
	memcpy (pntevts_text[sig], text, len + 1);
	pntevts[sig] = out;

	out = malloc (len + 2);
	memcpy (out, text, len + 1);
	out[len] = '\n';
	out[len + 1] = 0;
	check_special_chars (out, TRUE);

	PrintTextRaw (GTK_XTEXT (twid)->buffer, out, 0);
	free (out);

	/* save this when we exit */
	prefs.save_pevents = 1;
}

static void
pevent_dialog_hfill (GtkWidget * list, int e)
{
	int i = 0;
	char *text;
	GtkTreeIter iter;
	GtkListStore *store;

	store = (GtkListStore *)gtk_tree_view_get_model (GTK_TREE_VIEW (pevent_dialog_hlist));
	gtk_list_store_clear (store);
	while (i < te[e].num_args)
	{
		text = _(te[e].help[i]);
		i++;
		if (text[0] == '\001')
			text++;
		gtk_list_store_insert_with_values (store, &iter, -1,
													  0, i,
													  1, text, -1);
	}
}

static void
pevent_dialog_unselect (void)
{
	gtk_entry_set_text (GTK_ENTRY (pevent_dialog_entry), "");
	gtk_list_store_clear ((GtkListStore *)gtk_tree_view_get_model (GTK_TREE_VIEW (pevent_dialog_hlist)));
}

static void
pevent_dialog_select (GtkTreeSelection *sel, gpointer store)
{
	char *text;
	int sig;
	GtkTreeIter iter;

	if (!gtkutil_treeview_get_selected (GTK_TREE_VIEW (pevent_dialog_list),
													&iter, COL_ROW, &sig, -1))
	{
		pevent_dialog_unselect ();
	}
	else
	{
		gtk_tree_model_get (store, &iter, COL_EVENT_TEXT, &text, -1);
		gtk_entry_set_text (GTK_ENTRY (pevent_dialog_entry), text);
		g_free (text);
		pevent_dialog_hfill (pevent_dialog_hlist, sig);
	}
}

static void
pevent_dialog_fill (GtkWidget * list)
{
	int i;
	GtkListStore *store;
	GtkTreeIter iter;

	store = (GtkListStore *)gtk_tree_view_get_model (GTK_TREE_VIEW (list));
	gtk_list_store_clear (store);

	i = NUM_XP;
	do
	{
		i--;
		gtk_list_store_insert_with_values (store, &iter, 0,
													  COL_EVENT_NAME, te[i].name,
													  COL_EVENT_TEXT, pntevts_text[i],
													  COL_ROW, i, -1);
	}
	while (i != 0);
}

static void
pevent_save_req_cb (void *arg1, char *file)
{
	if (file)
		pevent_save (file);
}

static void
pevent_save_cb (GtkWidget * wid, void *data)
{
	if (data)
	{
		gtkutil_file_req (_("Print Texts File"), pevent_save_req_cb, NULL,
								NULL, FRF_WRITE);
		return;
	}
	pevent_save (NULL);
}

static void
pevent_load_req_cb (void *arg1, char *file)
{
	if (file)
	{
		pevent_load (file);
		pevent_make_pntevts ();
		pevent_dialog_fill (pevent_dialog_list);
		pevent_dialog_unselect ();
		prefs.save_pevents = 1;
	}
}

static void
pevent_load_cb (GtkWidget * wid, void *data)
{
	gtkutil_file_req (_("Print Texts File"), pevent_load_req_cb, NULL, NULL, 0);
}

static void
pevent_ok_cb (GtkWidget * wid, void *data)
{
	gtk_widget_destroy (pevent_dialog);
}

static void
pevent_test_cb (GtkWidget * wid, GtkWidget * twid)
{
	int len, n;
	char *out, *text;

	for (n = 0; n < NUM_XP; n++)
	{
		text = _(pntevts_text[n]);
		len = strlen (text);

		out = malloc (len + 2);
		memcpy (out, text, len + 1);
		out[len] = '\n';
		out[len + 1] = 0;
		check_special_chars (out, TRUE);

		PrintTextRaw (GTK_XTEXT (twid)->buffer, out, 0);
		free (out);
	}
}

void
pevent_dialog_show ()
{
	GtkWidget *vbox, *hbox, *tbox, *wid, *bh, *th;
	GtkListStore *store, *hstore;
	GtkTreeSelection *sel;

	if (pevent_dialog)
	{
		mg_bring_tofront (pevent_dialog);
		return;
	}

	pevent_dialog =
			  mg_create_generic_tab ("edit events", _("Edit Events"),
											 TRUE, FALSE, pevent_dialog_close, NULL,
											 600, 455, &vbox, 0);

	wid = gtk_vpaned_new ();
	th = gtk_vbox_new (0, 2);
	bh = gtk_vbox_new (0, 2);
	gtk_widget_show (th);
	gtk_widget_show (bh);
	gtk_paned_pack1 (GTK_PANED (wid), th, 1, 1);
	gtk_paned_pack2 (GTK_PANED (wid), bh, 0, 1);
	gtk_box_pack_start (GTK_BOX (vbox), wid, 1, 1, 0);
	gtk_widget_show (wid);

	store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING,
	                            G_TYPE_STRING, G_TYPE_INT);
	pevent_dialog_list = gtkutil_treeview_new (th, GTK_TREE_MODEL (store), NULL,
															 COL_EVENT_NAME, _("Event"),
															 COL_EVENT_TEXT, _("Text"), -1);
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (pevent_dialog_list));
	g_signal_connect (G_OBJECT (sel), "changed",
							G_CALLBACK (pevent_dialog_select), store);

	pevent_dialog_twid = gtk_xtext_new (colors, 0);
	gtk_xtext_set_tint (GTK_XTEXT (pevent_dialog_twid), prefs.tint_red, prefs.tint_green, prefs.tint_blue);
	gtk_xtext_set_background (GTK_XTEXT (pevent_dialog_twid),
									  channelwin_pix, prefs.transparent);

	pevent_dialog_entry = gtk_entry_new_with_max_length (255);
	g_signal_connect (G_OBJECT (pevent_dialog_entry), "activate",
							G_CALLBACK (pevent_dialog_update), pevent_dialog_twid);
	gtk_box_pack_start (GTK_BOX (bh), pevent_dialog_entry, 0, 0, 0);
	gtk_widget_show (pevent_dialog_entry);

	tbox = gtk_hbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (bh), tbox);
	gtk_widget_show (tbox);

	gtk_widget_set_usize (pevent_dialog_twid, 150, 20);
	gtk_container_add (GTK_CONTAINER (tbox), pevent_dialog_twid);
	gtk_xtext_set_font (GTK_XTEXT (pevent_dialog_twid), prefs.font_normal);

	wid = gtk_vscrollbar_new (GTK_XTEXT (pevent_dialog_twid)->adj);
	gtk_box_pack_start (GTK_BOX (tbox), wid, FALSE, FALSE, 0);
	show_and_unfocus (wid);

	gtk_widget_show (pevent_dialog_twid);

	hstore = gtk_list_store_new (2, G_TYPE_INT, G_TYPE_STRING);
	pevent_dialog_hlist = gtkutil_treeview_new (bh, GTK_TREE_MODEL (hstore),
															  NULL,
															  0, _("$ Number"),
															  1, _("Description"), -1);
	gtk_widget_show (pevent_dialog_hlist);

	pevent_dialog_fill (pevent_dialog_list);
	gtk_widget_show (pevent_dialog_list);

	hbox = gtk_hbutton_box_new ();
	gtk_box_pack_end (GTK_BOX (vbox), hbox, 0, 0, 2);
	/*wid = gtk_button_new_with_label (_("Save"));
	gtk_box_pack_end (GTK_BOX (hbox), wid, 0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (pevent_save_cb), NULL);
	gtk_widget_show (wid);*/
	gtkutil_button (hbox, GTK_STOCK_SAVE_AS, NULL, pevent_save_cb,
						 (void *) 1, _("Save As..."));
	gtkutil_button (hbox, GTK_STOCK_OPEN, NULL, pevent_load_cb,
						 (void *) 0, _("Load From..."));
	wid = gtk_button_new_with_label (_("Test All"));
	gtk_box_pack_end (GTK_BOX (hbox), wid, 0, 0, 0);
	g_signal_connect (G_OBJECT (wid), "clicked",
							G_CALLBACK (pevent_test_cb), pevent_dialog_twid);
	gtk_widget_show (wid);

	wid = gtk_button_new_from_stock (GTK_STOCK_OK);
	gtk_box_pack_start (GTK_BOX (hbox), wid, 0, 0, 0);
	g_signal_connect (G_OBJECT (wid), "clicked",
							G_CALLBACK (pevent_ok_cb), NULL);
	gtk_widget_show (wid);

	gtk_widget_show (hbox);

	gtk_widget_show (pevent_dialog);
}
