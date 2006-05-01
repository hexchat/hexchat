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
#define _FILE_OFFSET_BITS 64 /* allow selection of large files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "fe-gtk.h"

#include <gtk/gtkbutton.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkimage.h>
#include <gtk/gtktooltips.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkclipboard.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtkversion.h>
#include <gtk/gtkfilechooserdialog.h>

#include "../common/xchat.h"
#include "../common/fe.h"
#include "gtkutil.h"
#include "pixmaps.h"

/* gtkutil.c, just some gtk wrappers */

extern void path_part (char *file, char *path, int pathlen);


struct file_req
{
	GtkWidget *dialog;
	void *userdata;
	filereqcallback callback;
	int flags;		/* FRF_* flags */
};

static char last_dir[256] = "";


static void
gtkutil_file_req_destroy (GtkWidget * wid, struct file_req *freq)
{
	freq->callback (freq->userdata, NULL);
	free (freq);
}

static int
gtkutil_check_file (char *file, struct file_req *freq)
{
	struct stat st;
	int axs = FALSE;

	path_part (file, last_dir, sizeof (last_dir));

	if (!(freq->flags & FRF_CHOOSEFOLDER) && stat (file, &st) != -1)
	{
		if (S_ISDIR (st.st_mode))
		{
			char *tmp = malloc (strlen (file) + 2);

			strcpy (tmp, file);

			if (tmp[strlen(tmp)-1] != '/')
				strcat (tmp, "/");
			gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (freq->dialog), tmp);
			free (tmp);
			return 0;
		}
	}
	if (freq->flags & FRF_WRITE)
	{
		if (access (last_dir, W_OK) == 0)
			axs = TRUE;
	} else
	{
		if (stat (file, &st) != -1)
		{
			if (!S_ISDIR (st.st_mode) || (freq->flags & FRF_CHOOSEFOLDER))
				axs = TRUE;
		}
	}

	if (axs)
	{
		char *utf8_file;
		/* convert to UTF8. It might be converted back to locale by
			server.c's g_convert */
		utf8_file = g_filename_to_utf8 (file, -1, NULL, NULL, NULL);
		if (utf8_file)
		{
			freq->callback (freq->userdata, utf8_file);
			g_free (utf8_file);
		} else
		{
			fe_message ("Filename encoding is corrupt.", FE_MSG_ERROR);
		}
	} else
	{
		if (freq->flags & FRF_WRITE)
			fe_message (_("Cannot write to that file."), FE_MSG_ERROR);
		else
			fe_message (_("Cannot read that file."), FE_MSG_ERROR);
	}

	return 1;
}

static void
gtkutil_file_req_done (GtkWidget * wid, struct file_req *freq)
{
	int kill = 0;
	GSList *files, *cur;
	GtkFileChooser *fs = GTK_FILE_CHOOSER (freq->dialog);

	if (freq->flags & FRF_MULTIPLE)
	{
		files = cur = gtk_file_chooser_get_filenames (fs);
		while (cur)
		{
			kill |= gtkutil_check_file (cur->data, freq);
			g_free (cur->data);
			cur = cur->next;
		}
		if (files)
			g_slist_free (files);
	} else
	{
		if (freq->flags & FRF_CHOOSEFOLDER)
			kill = gtkutil_check_file (gtk_file_chooser_get_current_folder (fs), freq);
		else
			kill = gtkutil_check_file (gtk_file_chooser_get_filename (fs), freq);
	}

	if (kill)	/* this should call the "destroy" cb, where we free(freq) */
		gtk_widget_destroy (freq->dialog);
}

static void
gtkutil_file_req_response (GtkWidget *dialog, gint res, struct file_req *freq)
{
	switch (res)
	{
	case GTK_RESPONSE_ACCEPT:
		gtkutil_file_req_done (dialog, freq);
		break;

	case GTK_RESPONSE_CANCEL:
		/* this should call the "destroy" cb, where we free(freq) */
		gtk_widget_destroy (freq->dialog);
	}
}

void
gtkutil_file_req (char *title, void *callback, void *userdata, char *filter,
						int flags)
{
	struct file_req *freq;
	GtkWidget *dialog;
	extern char *get_xdir_fs (void);

	if (flags & FRF_WRITE)
		dialog = gtk_file_chooser_dialog_new (title, NULL,
												GTK_FILE_CHOOSER_ACTION_SAVE,
												GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
												GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
												NULL);
	else
		dialog = gtk_file_chooser_dialog_new (title, NULL,
												GTK_FILE_CHOOSER_ACTION_OPEN,
												GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
												GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
												NULL);
	if (flags & FRF_MULTIPLE)
		gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);
	if (last_dir[0])
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), last_dir);
	if (flags & FRF_ADDFOLDER)
		gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
														  get_xdir_fs (), NULL);
	if (flags & FRF_CHOOSEFOLDER)
	{
		gtk_file_chooser_set_action (GTK_FILE_CHOOSER (dialog), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), filter);
	}

	freq = malloc (sizeof (struct file_req));
	freq->dialog = dialog;
	freq->flags = flags;
	freq->callback = callback;
	freq->userdata = userdata;

	g_signal_connect (G_OBJECT (dialog), "response",
							G_CALLBACK (gtkutil_file_req_response), freq);
	g_signal_connect (G_OBJECT (dialog), "destroy",
						   G_CALLBACK (gtkutil_file_req_destroy), (gpointer) freq);
	gtk_widget_show (dialog);
}

void
gtkutil_destroy (GtkWidget * igad, GtkWidget * dgad)
{
	gtk_widget_destroy (dgad);
}

static void
gtkutil_get_str_response (GtkDialog *dialog, gint arg1, gpointer entry)
{
	void (*callback) (int cancel, char *text, void *user_data);
	char *text;
	void *user_data;

	text = (char *) gtk_entry_get_text (GTK_ENTRY (entry));
	callback = g_object_get_data (G_OBJECT (dialog), "cb");
	user_data = g_object_get_data (G_OBJECT (dialog), "ud");

	switch (arg1)
	{
	case GTK_RESPONSE_REJECT:
		callback (TRUE, text, user_data);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	case GTK_RESPONSE_ACCEPT:
		callback (FALSE, text, user_data);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	}
}

static void
gtkutil_str_enter (GtkWidget *entry, GtkWidget *dialog)
{
	gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
}

void
fe_get_str (char *msg, char *def, void *callback, void *userdata)
{
	GtkWidget *dialog;
	GtkWidget *entry;
	GtkWidget *hbox;
	GtkWidget *label;

	dialog = gtk_dialog_new_with_buttons (msg, NULL, 0,
										GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
										GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
										NULL);
	gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->vbox), TRUE);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
	hbox = gtk_hbox_new (TRUE, 0);

	g_object_set_data (G_OBJECT (dialog), "cb", callback);
	g_object_set_data (G_OBJECT (dialog), "ud", userdata);

	entry = gtk_entry_new ();
	g_signal_connect (G_OBJECT (entry), "activate",
						 	G_CALLBACK (gtkutil_str_enter), dialog);
	gtk_entry_set_text (GTK_ENTRY (entry), def);
	gtk_box_pack_end (GTK_BOX (hbox), entry, 0, 0, 0);

	label = gtk_label_new (msg);
	gtk_box_pack_end (GTK_BOX (hbox), label, 0, 0, 0);

	g_signal_connect (G_OBJECT (dialog), "response",
						   G_CALLBACK (gtkutil_get_str_response), entry);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

	gtk_widget_show_all (dialog);
}

static void
gtkutil_get_number_response (GtkDialog *dialog, gint arg1, gpointer spin)
{
	void (*callback) (int cancel, int value, void *user_data);
	int num;
	void *user_data;

	num = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spin));
	callback = g_object_get_data (G_OBJECT (dialog), "cb");
	user_data = g_object_get_data (G_OBJECT (dialog), "ud");

	switch (arg1)
	{
	case GTK_RESPONSE_REJECT:
		callback (TRUE, num, user_data);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	case GTK_RESPONSE_ACCEPT:
		callback (FALSE, num, user_data);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	}
}

void
fe_get_int (char *msg, int def, void *callback, void *userdata)
{
	GtkWidget *dialog;
	GtkWidget *spin;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkAdjustment *adj;

	dialog = gtk_dialog_new_with_buttons (msg, NULL, 0,
										GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
										GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
										NULL);
	gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->vbox), TRUE);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
	hbox = gtk_hbox_new (TRUE, 0);

	g_object_set_data (G_OBJECT (dialog), "cb", callback);
	g_object_set_data (G_OBJECT (dialog), "ud", userdata);

	spin = gtk_spin_button_new (NULL, 1, 0);
	adj = gtk_spin_button_get_adjustment ((GtkSpinButton*)spin);
	adj->lower = 0;
	adj->upper = 1024;
	adj->step_increment = 1;
	gtk_adjustment_changed (adj);
	gtk_spin_button_set_value ((GtkSpinButton*)spin, def);
	gtk_box_pack_end (GTK_BOX (hbox), spin, 0, 0, 0);

	label = gtk_label_new (msg);
	gtk_box_pack_end (GTK_BOX (hbox), label, 0, 0, 0);

	g_signal_connect (G_OBJECT (dialog), "response",
						   G_CALLBACK (gtkutil_get_number_response), spin);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

	gtk_widget_show_all (dialog);
}

GtkWidget *
gtkutil_button (GtkWidget *box, char *stock, char *tip, void *callback,
				 void *userdata, char *labeltext)
{
	GtkWidget *wid, *img, *label, *bbox;

	wid = gtk_button_new ();
	g_signal_connect (G_OBJECT (wid), "clicked",
							G_CALLBACK (callback), userdata);

	bbox = gtk_hbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (wid), bbox);
	gtk_widget_show (bbox);

	if (stock)
	{
		img = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_MENU);
		if (stock == GTK_STOCK_GOTO_LAST)
			gtk_widget_set_usize (img, 10, 6);
		gtk_container_add (GTK_CONTAINER (bbox), img);
		gtk_widget_show (img);
	}

	if (labeltext)
	{
		label = gtk_label_new_with_mnemonic (labeltext);
		gtk_container_add (GTK_CONTAINER (bbox), label);
		gtk_widget_show (label);
		gtk_container_add (GTK_CONTAINER (box), wid);
	} else
	{
		gtk_box_pack_start (GTK_BOX (box), wid, 0, 0, 0);
	}

	gtk_widget_show (wid);
	if (tip)
		add_tip (wid, tip);

	return wid;
}

void
gtkutil_label_new (char *text, GtkWidget * box)
{
	GtkWidget *label = gtk_label_new (text);
	gtk_container_add (GTK_CONTAINER (box), label);
	gtk_widget_show (label);
}

GtkWidget *
gtkutil_entry_new (int max, GtkWidget * box, void *callback,
						 gpointer userdata)
{
	GtkWidget *entry = gtk_entry_new_with_max_length (max);
	gtk_container_add (GTK_CONTAINER (box), entry);
	if (callback)
		g_signal_connect (G_OBJECT (entry), "changed",
								G_CALLBACK (callback), userdata);
	gtk_widget_show (entry);
	return entry;
}

GtkWidget *
gtkutil_clist_new (int columns, char *titles[],
						 GtkWidget * box, int policy,
						 void *select_callback, gpointer select_userdata,
						 void *unselect_callback,
						 gpointer unselect_userdata, int selection_mode)
{
	GtkWidget *clist, *win;

	win = gtk_scrolled_window_new (0, 0);
	gtk_container_add (GTK_CONTAINER (box), win);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win),
											  GTK_POLICY_AUTOMATIC, policy);
	gtk_widget_show (win);

	if (titles)
		clist = gtk_clist_new_with_titles (columns, titles);
	else
		clist = gtk_clist_new (columns);

	gtk_clist_set_selection_mode (GTK_CLIST (clist), selection_mode);
	gtk_clist_column_titles_passive (GTK_CLIST (clist));
	gtk_container_add (GTK_CONTAINER (win), clist);
	if (select_callback)
	{
		g_signal_connect (G_OBJECT (clist), "select_row",
								G_CALLBACK (select_callback), select_userdata);
	}
	if (unselect_callback)
	{
		g_signal_connect (G_OBJECT (clist), "unselect_row",
								G_CALLBACK (unselect_callback), unselect_userdata);
	}
	gtk_widget_show (clist);

	return clist;
}

int
gtkutil_clist_selection (GtkWidget * clist)
{
	if (GTK_CLIST (clist)->selection)
		return GPOINTER_TO_INT(GTK_CLIST (clist)->selection->data);
	return -1;
}

static int
int_compare (const int * elem1, const int * elem2)
{
	return (*elem1) - (*elem2);
}

int
gtkutil_clist_multiple_selection (GtkWidget * clist, int ** rows, const int max_rows)
{
	int i = 0;
	GList *tmp_clist;
	*rows = malloc (sizeof (int) * max_rows );
	memset( *rows, -1, max_rows * sizeof(int) );

	for( tmp_clist = GTK_CLIST(clist)->selection;
			tmp_clist && i < max_rows; tmp_clist = tmp_clist->next, i++)
	{
		(*rows)[i] = GPOINTER_TO_INT( tmp_clist->data );
	}
	qsort(*rows, i, sizeof(int), (void *)int_compare);
	return i;

}

void
add_tip (GtkWidget * wid, char *text)
{
	static GtkTooltips *tip = NULL;
	if (!tip)
		tip = gtk_tooltips_new ();
	gtk_tooltips_set_tip (tip, wid, text, 0);
}

void
show_and_unfocus (GtkWidget * wid)
{
	GTK_WIDGET_UNSET_FLAGS (wid, GTK_CAN_FOCUS);
	gtk_widget_show (wid);
}

void
gtkutil_set_icon (GtkWidget *win)
{
	gtk_window_set_icon (GTK_WINDOW (win), pix_xchat);
}

extern GtkWidget *parent_window;	/* maingui.c */

GtkWidget *
gtkutil_window_new (char *title, char *role, int width, int height, int flags)
{
	GtkWidget *win;

	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtkutil_set_icon (win);
#ifdef WIN32
	gtk_window_set_wmclass (GTK_WINDOW (win), "XChat", "xchat");
#endif
	gtk_window_set_title (GTK_WINDOW (win), title);
	gtk_window_set_default_size (GTK_WINDOW (win), width, height);
	gtk_window_set_role (GTK_WINDOW (win), role);
	if (flags & 1)
		gtk_window_set_position (GTK_WINDOW (win), GTK_WIN_POS_MOUSE);
	if ((flags & 2) && parent_window)
	{
		gtk_window_set_type_hint (GTK_WINDOW (win), GDK_WINDOW_TYPE_HINT_DIALOG);
		gtk_window_set_transient_for (GTK_WINDOW (win), GTK_WINDOW (parent_window));
	}

	return win;
}

/* pass NULL as selection to paste to both clipboard & X11 text */
void
gtkutil_copy_to_clipboard (GtkWidget *widget, GdkAtom selection,
                           const gchar *str)
{
	GtkWidget *win;
	GtkClipboard *clip, *clip2;

	win = gtk_widget_get_toplevel (GTK_WIDGET (widget));
	if (GTK_WIDGET_TOPLEVEL (win))
	{
		glong len = g_utf8_strlen (str, -1);

		if (selection)
		{
#if (GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION == 0)
			gtk_clipboard_set_text (gtk_clipboard_get (selection), str, len);
		} else
		{
			clip = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
			clip2 = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
#else
			clip = gtk_widget_get_clipboard (win, selection);
			gtk_clipboard_set_text (clip, str, len);
		} else
		{
			/* copy to both primary X selection and clipboard */
			clip = gtk_widget_get_clipboard (win, GDK_SELECTION_PRIMARY);
			clip2 = gtk_widget_get_clipboard (win, GDK_SELECTION_CLIPBOARD);
#endif
			gtk_clipboard_set_text (clip, str, len);
			gtk_clipboard_set_text (clip2, str, len);
		}
	}
}

/* Treeview util functions */

GtkWidget *
gtkutil_treeview_new (GtkWidget *box, GtkTreeModel *model,
                      GtkTreeCellDataFunc mapper, ...)
{
	GtkWidget *win, *view;
	GtkCellRenderer *renderer = NULL;
	GtkTreeViewColumn *col;
	va_list args;
	int col_id = 0;
	GType type;
	char *title, *attr;

	win = gtk_scrolled_window_new (0, 0);
	gtk_container_add (GTK_CONTAINER (box), win);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win),
											  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show (win);

	view = gtk_tree_view_new_with_model (model);
	/* the view now has a ref on the model, we can unref it */
	g_object_unref (G_OBJECT (model));
	gtk_container_add (GTK_CONTAINER (win), view);

	va_start (args, mapper);
	for (col_id = va_arg (args, int); col_id != -1; col_id = va_arg (args, int))
	{
		type = gtk_tree_model_get_column_type (model, col_id);
		switch (type)
		{
			case G_TYPE_BOOLEAN:
				renderer = gtk_cell_renderer_toggle_new ();
				attr = "active";
				break;
			case G_TYPE_STRING:	/* fall through */
			default:
				renderer = gtk_cell_renderer_text_new ();
				attr = "text";
				break;
		}

		title = va_arg (args, char *);
		if (mapper)	/* user-specified function to set renderer attributes */
		{
			col = gtk_tree_view_column_new_with_attributes (title, renderer, NULL);
			gtk_tree_view_column_set_cell_data_func (col, renderer, mapper,
			                                         GINT_TO_POINTER (col_id), NULL);
		} else
		{
			/* just set the typical attribute for this type of renderer */
			col = gtk_tree_view_column_new_with_attributes (title, renderer,
			                                                attr, col_id, NULL);
		}
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);
	}

	va_end (args);

	return view;
}

gboolean
gtkutil_treemodel_string_to_iter (GtkTreeModel *model, gchar *pathstr, GtkTreeIter *iter_ret)
{
	GtkTreePath *path = gtk_tree_path_new_from_string (pathstr);
	gboolean success;

	success = gtk_tree_model_get_iter (model, iter_ret, path);
	gtk_tree_path_free (path);
	return success;
}

/*gboolean
gtkutil_treeview_get_selected_iter (GtkTreeView *view, GtkTreeIter *iter_ret)
{
	GtkTreeModel *store;
	GtkTreeSelection *select;
	
	select = gtk_tree_view_get_selection (view);
	return gtk_tree_selection_get_selected (select, &store, iter_ret);
}*/

gboolean
gtkutil_treeview_get_selected (GtkTreeView *view, GtkTreeIter *iter_ret, ...)
{
	GtkTreeModel *store;
	GtkTreeSelection *select;
	gboolean has_selected;
	va_list args;
	
	select = gtk_tree_view_get_selection (view);
	has_selected = gtk_tree_selection_get_selected (select, &store, iter_ret);

	if (has_selected) {
		va_start (args, iter_ret);
		gtk_tree_model_get_valist (store, iter_ret, args);
		va_end (args);
	}

	return has_selected;
}

