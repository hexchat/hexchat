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
#include <string.h>
#include <stdlib.h>

#include "fe-gtk.h"

#include <gtk/gtkbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkentry.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkcellrendererpixbuf.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkliststore.h>
#include <gdk/gdkkeysyms.h>

#include "../common/xchat.h"
#include "../common/util.h"
#include "../common/userlist.h"
#include "../common/modes.h"
#include "../common/notify.h"
#include "../common/xchatc.h"
#include "../common/dcc.h"
#include "gtkutil.h"
#include "palette.h"
#include "maingui.h"
#include "menu.h"
#include "pixmaps.h"
#include "userlistgui.h"


GdkPixbuf *
get_user_icon (server *serv, struct User *user)
{
	char *pre;
	int level;

	if (!user)
		return NULL;

	/* these ones are hardcoded */
	switch (user->prefix[0])
	{
	case 0: return NULL;
	case '@': return pix_op;
	case '%': return pix_hop;
	case '+': return pix_voice;
	}

	/* find out how many levels above Op this user is */
	pre = strchr (serv->nick_prefixes, '@');
	if (pre && pre != serv->nick_prefixes)
	{
		pre--;
		level = 0;
		while (1)
		{
			if (pre[0] == user->prefix[0])
			{
				switch (level)
				{
				case 0: return pix_red;	/* 1 level above op */
				case 1: return pix_purple;	 /* 2 levels above op */
				}
				break;	/* 3+, no icons */
			}
			level++;
			if (pre == serv->nick_prefixes)
				break;
			pre--;
		}
	}

	return NULL;
}

void
fe_userlist_numbers (session *sess)
{
	char tbuf[256];

	if (sess == current_tab || !sess->gui->is_tab)
	{
		if (sess->total)
		{
			sprintf (tbuf, "%d ops, %d total", sess->ops, sess->total);
			gtk_label_set_text (GTK_LABEL (sess->gui->namelistinfo), tbuf);
		} else
		{
			gtk_label_set_text (GTK_LABEL (sess->gui->namelistinfo), NULL);
		}
	}
}

char **
userlist_selection_list (GtkWidget *widget, int *num_ret)
{
	GtkTreeIter iter;
	GtkTreeView *treeview = (GtkTreeView *) widget;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	int i, num_sel;
	char **nicks;

	*num_ret = 0;
	/* first, count the number of selections */
	num_sel = 0;
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			if (gtk_tree_selection_iter_is_selected (selection, &iter))
				num_sel++;
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}

	if (num_sel < 1)
		return NULL;

	nicks = malloc (sizeof (char *) * (num_sel + 1));

	i = 0;
	gtk_tree_model_get_iter_first (model, &iter);
	do
	{
		if (gtk_tree_selection_iter_is_selected (selection, &iter))
		{
			gtk_tree_model_get (model, &iter, 1, &nicks[i], -1);
			i++;
			nicks[i] = NULL;
		}
	}
	while (gtk_tree_model_iter_next (model, &iter));

	*num_ret = i;
	return nicks;
}

static GtkTreeIter *
find_row (GtkTreeView *treeview, GtkTreeModel *model, struct User *user,
			 int *selected)
{
	static GtkTreeIter iter;
	struct User *row_user;

	*selected = FALSE;
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			gtk_tree_model_get (model, &iter, 3, &row_user, -1);
			if (row_user == user)
			{
				if (gtk_tree_view_get_model (treeview) == model)
				{
					if (gtk_tree_selection_iter_is_selected (gtk_tree_view_get_selection (treeview), &iter))
						*selected = TRUE;
				}
				return &iter;
			}
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}

	return NULL;
}

void
userlist_set_value (GtkWidget *treeview, gfloat val)
{
	gtk_adjustment_set_value (
			gtk_tree_view_get_vadjustment (GTK_TREE_VIEW (treeview)), val);
}

gfloat
userlist_get_value (GtkWidget *treeview)
{
	return gtk_tree_view_get_vadjustment (GTK_TREE_VIEW (treeview))->value;
}

int
fe_userlist_remove (session *sess, struct User *user)
{
	GtkTreeIter *iter;
	GtkAdjustment *adj;
	gfloat val, end;
	int sel;

	iter = find_row (GTK_TREE_VIEW (sess->gui->user_tree),
						  sess->res->user_model, user, &sel);
	if (!iter)
		return 0;

	adj = gtk_tree_view_get_vadjustment (GTK_TREE_VIEW (sess->gui->user_tree));
	val = adj->value;

	gtk_list_store_remove (sess->res->user_model, iter);

	/* is it the front-most tab? */
	if (gtk_tree_view_get_model (GTK_TREE_VIEW (sess->gui->user_tree))
		 == sess->res->user_model)
	{
		end = adj->upper - adj->lower - adj->page_size;
		if (val > end)
			val = end;
		gtk_adjustment_set_value (adj, val);
	}

	return sel;
}

void
fe_userlist_insert (session *sess, struct User *newuser, int row, int sel, struct User *after)
{
/*	gfloat val;*/
	GtkTreeModel *model = sess->res->user_model;
	GdkPixbuf *pix;
	GtkTreeIter iter;

/*	val = userlist_get_value (sess->gui->user_tree);*/

	/* TreeView is no faster with this! Why?? */
/*	if (after)
	{
		gtk_list_store_insert_after (GTK_LIST_STORE (model), iter,
											  (GtkTreeIter *)after->gui);
	} else
	{*/
		switch (row)
		{
		case -1:
			gtk_list_store_append (GTK_LIST_STORE (model), &iter);
			break;
		default:
			/* row 0 does an *_prepend() */
			gtk_list_store_insert (GTK_LIST_STORE (model), &iter, row);
		}
	/*}*/

	pix = get_user_icon (sess->server, newuser);

	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
							  0, pix,
							  1, newuser->nick,
							  2, newuser->hostname,
							  3, newuser,
							  -1);

	/* is it me? */
	if (!sess->server->p_cmp (newuser->nick, sess->server->nick) && sess->gui->nick_box)
	{
		sess->res->myself = newuser;
		if (!sess->gui->is_tab || sess == current_tab)
			mg_set_access_icon (sess->gui, pix);
	}

#if 0
	if (prefs.hilitenotify && notify_isnotify (sess, newuser->nick))
	{
		gtk_clist_set_foreground ((GtkCList *) sess->gui->user_clist, row,
										  &colors[prefs.nu_color]);
	}
#endif

	/* is it the front-most tab? */
	if (gtk_tree_view_get_model (GTK_TREE_VIEW (sess->gui->user_tree))
		 == model)
	{
	/*	userlist_set_value (sess->gui->user_tree, val);*/

		if (sel)
			gtk_tree_selection_select_iter (gtk_tree_view_get_selection
										(GTK_TREE_VIEW (sess->gui->user_tree)), &iter);
	}
}

void
fe_userlist_move (session *sess, struct User *user, int new_row)
{
	fe_userlist_insert (sess, user, new_row, fe_userlist_remove (sess, user), NULL);
}

void
fe_userlist_clear (session *sess)
{
	gtk_list_store_clear (sess->res->user_model);
	sess->res->myself = NULL;
}

static void
userlist_dnd_drop (GtkTreeView *widget, GdkDragContext *context,
						 gint x, gint y, GtkSelectionData *selection_data,
						 guint info, guint ttime, gpointer userdata)
{
	struct User *user;
	char tbuf[400];
	char *p, *data, *next;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (!gtk_tree_view_get_path_at_pos (widget, x, y, &path, NULL, NULL, NULL))
		return;

	model = gtk_tree_view_get_model (widget);
	if (!gtk_tree_model_get_iter (model, &iter, path))
		return;
	gtk_tree_model_get (model, &iter, 3, &user, -1);

	p = data = strdup (selection_data->data);
	while (*p)
	{
		next = strchr (p, '\r');
		if (strncasecmp ("file:", p, 5) == 0)
		{
			if (next)
				*next = 0;
			p += 5;
#ifdef WIN32
			if (strncmp (p, "///", 3) == 0)
				p += 3;
#endif
			dcc_send (current_sess, tbuf, user->nick, p, prefs.dcc_max_send_cps);
		}
		if (!next)
			break;
		p = next + 1;
		if (*p == '\n')
			p++;
	}
	free (data);
}

static gboolean
userlist_dnd_motion (GtkTreeView *widget, GdkDragContext *context, gint x,
							gint y, guint ttime)
{
	GtkTreePath *path;
	GtkTreeSelection *sel;

	if (gtk_tree_view_get_path_at_pos (widget, x, y, &path, NULL, NULL, NULL))
	{
		sel = gtk_tree_view_get_selection (widget);
		gtk_tree_selection_unselect_all (sel);
		gtk_tree_selection_select_path (sel, path);
	}

	return TRUE;
}

static gboolean
userlist_dnd_leave (GtkTreeView *widget, GdkDragContext *context, guint ttime)
{
	gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (widget));
	return TRUE;
}

void *
userlist_create_model (void)
{
	return gtk_list_store_new (4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
										G_TYPE_POINTER);
}

static void
userlist_add_columns (GtkTreeView * treeview)
{
	GtkCellRenderer *renderer;

	/* icon column */
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
																-1, NULL, renderer,
																"pixbuf", 0, NULL);
/*	gtk_tree_view_column_set_sizing (gtk_tree_view_get_column (treeview, 0),
												GTK_TREE_VIEW_COLUMN_AUTOSIZE);*/

	/* nick column */
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
																-1, NULL, renderer,
																"text", 1, NULL);
/*	gtk_tree_view_column_set_sizing (gtk_tree_view_get_column (treeview, 1),
												GTK_TREE_VIEW_COLUMN_AUTOSIZE);*/

	if (prefs.showhostname_in_userlist)
	{
		/* hostname column */
		renderer = gtk_cell_renderer_text_new ();
		gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
																	-1, NULL, renderer,
																	"text", 2, NULL);
	}
}

static gint
userlist_click_cb (GtkWidget *widget, GdkEventButton *event, gpointer userdata)
{
	char **nicks;
	int i;
	GtkTreeSelection *sel;
	GtkTreePath *path;

	if (!event)
		return FALSE;

	if (event->type == GDK_2BUTTON_PRESS && prefs.doubleclickuser[0])
	{
		nicks = userlist_selection_list (widget, &i);
		if (nicks)
		{
			nick_command_parse (current_sess, prefs.doubleclickuser, nicks[0],
									  nicks[0]);
			while (i)
			{
				i--;
				g_free (nicks[i]);
			}
			free (nicks);
		}
		return TRUE;
	}

	if (event->button == 3)
	{
		/* do we have a multi-selection? */
		nicks = userlist_selection_list (widget, &i);
		if (nicks && i > 1)
		{
			menu_nickmenu (current_sess, event, nicks[0], i);
			while (i)
			{
				i--;
				g_free (nicks[i]);
			}
			free (nicks);
			return TRUE;
		}
		if (nicks)
			free (nicks);

		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
		if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
			 event->x, event->y, &path, 0, 0, 0))
		{
			gtk_tree_selection_unselect_all (sel);
			gtk_tree_selection_select_path (sel, path);
			gtk_tree_path_free (path);
			nicks = userlist_selection_list (widget, &i);
			if (nicks)
			{
				menu_nickmenu (current_sess, event, nicks[0], i);
				while (i)
				{
					i--;
					g_free (nicks[i]);
				}
				free (nicks);
			}
		} else
		{
			gtk_tree_selection_unselect_all (sel);
		}

		return TRUE;
	}

	return FALSE;
}

static gboolean
userlist_key_cb (GtkWidget *wid, GdkEventKey *evt, gpointer userdata)
{
	if (evt->keyval >= GDK_asterisk && evt->keyval <= GDK_z)
	{
		/* dirty trick to avoid auto-selection */
		GTK_ENTRY (current_sess->gui->input_box)->editable = 0;
		gtk_widget_grab_focus (current_sess->gui->input_box);
		GTK_ENTRY (current_sess->gui->input_box)->editable = 1;
		gtk_widget_event (current_sess->gui->input_box, (GdkEvent *)evt);
		return TRUE;
	}

	return FALSE;
}

GtkWidget *
userlist_create (GtkWidget *box)
{
	GtkWidget *sw, *treeview;
	static const GtkTargetEntry dnd_targets[] =
	{
		{"text/uri-list", 0, 1}
	};

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
													 GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
											  prefs.showhostname_in_userlist ?
												GTK_POLICY_AUTOMATIC :
												GTK_POLICY_NEVER,
											  GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (box), sw, TRUE, TRUE, 0);
	gtk_widget_show (sw);

	treeview = gtk_tree_view_new ();
	gtk_widget_set_name (treeview, "xchat-userlist");
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection
										  (GTK_TREE_VIEW (treeview)),
										  GTK_SELECTION_MULTIPLE);

	/* set up drops */
	gtk_drag_dest_set (treeview, GTK_DEST_DEFAULT_ALL, dnd_targets, 1,
							 GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);

	g_signal_connect (G_OBJECT (treeview), "drag_motion",
							G_CALLBACK (userlist_dnd_motion), 0);
	g_signal_connect (G_OBJECT (treeview), "drag_leave",
							G_CALLBACK (userlist_dnd_leave), 0);
	g_signal_connect (G_OBJECT (treeview), "drag_data_received",
							G_CALLBACK (userlist_dnd_drop), 0);
	g_signal_connect (G_OBJECT (treeview), "button_press_event",
							G_CALLBACK (userlist_click_cb), 0);
	g_signal_connect (G_OBJECT (treeview), "key_press_event",
							G_CALLBACK (userlist_key_cb), 0);

	userlist_add_columns (GTK_TREE_VIEW (treeview));

	gtk_container_add (GTK_CONTAINER (sw), treeview);
	gtk_widget_show (treeview);

	return treeview;
}

void
userlist_show (session *sess)
{
	gtk_tree_view_set_model (GTK_TREE_VIEW (sess->gui->user_tree),
									 sess->res->user_model);
}
