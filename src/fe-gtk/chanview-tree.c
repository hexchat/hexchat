/* HexChat
 * Copyright (C) 1998-2010 Peter Zelezny.
 * Copyright (C) 2009-2013 Berke Viktor.
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

/* file included in chanview.c */

typedef struct
{
	GtkTreeView *tree;
	GtkWidget *scrollw;	/* scrolledWindow */
} treeview;

#include <gdk/gdk.h>

static void 	/* row-activated, when a row is double clicked */
cv_tree_activated_cb (GtkTreeView *view, GtkTreePath *path,
							 GtkTreeViewColumn *column, gpointer data)
{
	if (gtk_tree_view_row_expanded (view, path))
		gtk_tree_view_collapse_row (view, path);
	else
		gtk_tree_view_expand_row (view, path, FALSE);
}

static void		/* row selected callback */
cv_tree_sel_cb (GtkTreeSelection *sel, chanview *cv)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	chan *ch;

	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, COL_CHAN, &ch, -1);

		cv->focused = ch;
		cv->cb_focus (cv, ch, ch->tag, ch->userdata);
	}
}

static gboolean
cv_tree_click_cb (GtkTreeView *tree, GdkEventButton *event, chanview *cv)
{
	chan *ch;
	GtkTreePath *path;
	GtkTreeIter iter;
	int ret = FALSE;

	if (gtk_tree_view_get_path_at_pos (tree, event->x, event->y, &path, 0, 0, 0))
	{
		if (gtk_tree_model_get_iter (GTK_TREE_MODEL (cv->store), &iter, path))
		{
			gtk_tree_model_get (GTK_TREE_MODEL (cv->store), &iter, COL_CHAN, &ch, -1);
			ret = cv->cb_contextmenu (cv, ch, ch->tag, ch->userdata, event);
		}
		gtk_tree_path_free (path);
	}
	return ret;
}

static gboolean
cv_tree_scroll_event_cb (GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
	if (prefs.hex_gui_tab_scrollchans)
	{
		if (event->direction == GDK_SCROLL_DOWN)
			mg_switch_page (1, 1);
		else if (event->direction == GDK_SCROLL_UP)
			mg_switch_page (1, -1);

		return TRUE;
	}

	return FALSE;
}

static void
cv_tree_init (chanview *cv)
{
	GtkWidget *view, *win;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;
	int wid1, wid2;
	static const GtkTargetEntry dnd_src_target[] =
	{
		{"HEXCHAT_CHANVIEW", GTK_TARGET_SAME_APP, 75 }
	};
	static const GtkTargetEntry dnd_dest_target[] =
	{
		{"HEXCHAT_USERLIST", GTK_TARGET_SAME_APP, 75 }
	};

	win = gtk_scrolled_window_new (0, 0);
	/*gtk_container_set_border_width (GTK_CONTAINER (win), 1);*/
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (win),
													 GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win),
											  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (cv->box), win);
	gtk_widget_show (win);

	view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (cv->store));
	gtk_widget_set_name (view, "hexchat-tree");
	if (cv->style)
		gtk_widget_set_style (view, cv->style);
	/*gtk_widget_modify_base (view, GTK_STATE_NORMAL, &colors[COL_BG]);*/
	gtk_widget_set_can_focus (view, FALSE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

	if (prefs.hex_gui_tab_dots)
	{
		gtk_tree_view_set_enable_tree_lines (GTK_TREE_VIEW (view), TRUE);
	}
	
	/* Indented channels with no server looks silly, but we still want expanders */
	if (!prefs.hex_gui_tab_server)
	{
		gtk_widget_style_get (view, "expander-size", &wid1, "horizontal-separator", &wid2, NULL);
		gtk_tree_view_set_level_indentation (GTK_TREE_VIEW (view), -wid1 - wid2);
	}


	gtk_container_add (GTK_CONTAINER (win), view);
	col = gtk_tree_view_column_new();

	/* icon column */
	if (cv->use_icons)
	{
		renderer = gtk_cell_renderer_pixbuf_new ();
		if (prefs.hex_gui_compact)
			g_object_set (G_OBJECT (renderer), "ypad", 0, NULL);

		gtk_tree_view_column_pack_start(col, renderer, FALSE);
		gtk_tree_view_column_set_attributes (col, renderer, "pixbuf", COL_PIXBUF, NULL);
	}

	/* main column */
	renderer = gtk_cell_renderer_text_new ();
	if (prefs.hex_gui_compact)
		g_object_set (G_OBJECT (renderer), "ypad", 0, NULL);
	gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT (renderer), 1);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes (col, renderer, "text", COL_NAME, "attributes", COL_ATTR, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);									

	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (view))),
							"changed", G_CALLBACK (cv_tree_sel_cb), cv);
	g_signal_connect (G_OBJECT (view), "button-press-event",
							G_CALLBACK (cv_tree_click_cb), cv);
	g_signal_connect (G_OBJECT (view), "row-activated",
							G_CALLBACK (cv_tree_activated_cb), NULL);
	g_signal_connect (G_OBJECT (view), "scroll_event",
							G_CALLBACK (cv_tree_scroll_event_cb), NULL);

	gtk_drag_dest_set (view, GTK_DEST_DEFAULT_ALL, dnd_dest_target, 1,
							 GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);
	gtk_drag_source_set (view, GDK_BUTTON1_MASK, dnd_src_target, 1, GDK_ACTION_COPY);

	g_signal_connect (G_OBJECT (view), "drag_begin",
							G_CALLBACK (mg_drag_begin_cb), NULL);
	g_signal_connect (G_OBJECT (view), "drag_drop",
							G_CALLBACK (mg_drag_drop_cb), NULL);
	g_signal_connect (G_OBJECT (view), "drag_motion",
							G_CALLBACK (mg_drag_motion_cb), NULL);
	g_signal_connect (G_OBJECT (view), "drag_end",
							G_CALLBACK (mg_drag_end_cb), NULL);

	((treeview *)cv)->tree = GTK_TREE_VIEW (view);
	((treeview *)cv)->scrollw = win;
	gtk_widget_show (view);
}

static void
cv_tree_postinit (chanview *cv)
{
	gtk_tree_view_expand_all (((treeview *)cv)->tree);
}

static void *
cv_tree_add (chanview *cv, chan *ch, char *name, GtkTreeIter *parent)
{
	GtkTreePath *path;

	if (parent)
	{
		/* expand the parent node */
		path = gtk_tree_model_get_path (GTK_TREE_MODEL (cv->store), parent);
		if (path)
		{
			gtk_tree_view_expand_row (((treeview *)cv)->tree, path, FALSE);
			gtk_tree_path_free (path);
		}
	}

	return NULL;
}

static void
cv_tree_change_orientation (chanview *cv)
{
}

static void
cv_tree_focus (chan *ch)
{
	GtkTreeView *tree = ((treeview *)ch->cv)->tree;
	GtkTreeModel *model = gtk_tree_view_get_model (tree);
	GtkTreePath *path;
	GtkTreeIter parent;
	GdkRectangle cell_rect;
	GdkRectangle vis_rect;
	gint dest_y;

	/* expand the parent node */
	if (gtk_tree_model_iter_parent (model, &parent, &ch->iter))
	{
		path = gtk_tree_model_get_path (model, &parent);
		if (path)
		{
			/*if (!gtk_tree_view_row_expanded (tree, path))
			{
				gtk_tree_path_free (path);
				return;
			}*/
			gtk_tree_view_expand_row (tree, path, FALSE);
			gtk_tree_path_free (path);
		}
	}

	path = gtk_tree_model_get_path (model, &ch->iter);
	if (path)
	{
		/* This full section does what
		 * gtk_tree_view_scroll_to_cell (tree, path, NULL, TRUE, 0.5, 0.5);
		 * does, except it only scrolls the window if the provided cell is
		 * not visible. Basic algorithm taken from gtktreeview.c */

		/* obtain information to see if the cell is visible */
		gtk_tree_view_get_background_area (tree, path, NULL, &cell_rect);
		gtk_tree_view_get_visible_rect (tree, &vis_rect);

		/* The cordinates aren't offset correctly */
		gtk_tree_view_convert_widget_to_bin_window_coords ( tree, cell_rect.x, cell_rect.y, NULL, &cell_rect.y );

		/* only need to scroll if out of bounds */
		if (cell_rect.y < vis_rect.y ||
				cell_rect.y + cell_rect.height > vis_rect.y + vis_rect.height)
		{
			dest_y = cell_rect.y - ((vis_rect.height - cell_rect.height) * 0.5);
			if (dest_y < 0)
				dest_y = 0;
			gtk_tree_view_scroll_to_point (tree, -1, dest_y);
		}
		/* theft done, now make it focused like */
		gtk_tree_view_set_cursor (tree, path, NULL, FALSE);
		gtk_tree_path_free (path);
	}
}

static void
cv_tree_move_focus (chanview *cv, gboolean relative, int num)
{
	chan *ch;

	if (relative)
	{
		num += cv_find_number_of_chan (cv, cv->focused);
		num %= cv->size;
		/* make it wrap around at both ends */
		if (num < 0)
			num = cv->size - 1;
	}

	ch = cv_find_chan_by_number (cv, num);
	if (ch)
		cv_tree_focus (ch);
}

static void
cv_tree_remove (chan *ch)
{
}

static void
move_row (chan *ch, int delta, GtkTreeIter *parent)
{
	GtkTreeStore *store = ch->cv->store;
	GtkTreeIter *src = &ch->iter;
	GtkTreeIter dest = ch->iter;
	GtkTreePath *dest_path;

	if (delta < 0) /* down */
	{
		if (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &dest))
			gtk_tree_store_swap (store, src, &dest);
		else	/* move to top */
			gtk_tree_store_move_after (store, src, NULL);

	} else
	{
		dest_path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &dest);
		if (gtk_tree_path_prev (dest_path))
		{
			gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &dest, dest_path);
			gtk_tree_store_swap (store, src, &dest);
		} else
		{	/* move to bottom */
			gtk_tree_store_move_before (store, src, NULL);
		}

		gtk_tree_path_free (dest_path);
	}
}

static void
cv_tree_move (chan *ch, int delta)
{
	GtkTreeIter parent;

	/* do nothing if this is a server row */
	if (gtk_tree_model_iter_parent (GTK_TREE_MODEL (ch->cv->store), &parent, &ch->iter))
		move_row (ch, delta, &parent);
}

static void
cv_tree_move_family (chan *ch, int delta)
{
	move_row (ch, delta, NULL);
}

static void
cv_tree_cleanup (chanview *cv)
{
	if (cv->box)
		/* kill the scrolled window */
		gtk_widget_destroy (((treeview *)cv)->scrollw);
}

static void
cv_tree_set_color (chan *ch, PangoAttrList *list)
{
	/* nothing to do, it's already set in the store */
}

static void
cv_tree_rename (chan *ch, char *name)
{
	/* nothing to do, it's already renamed in the store */
}

static chan *
cv_tree_get_parent (chan *ch)
{
	chan *parent_ch = NULL;
	GtkTreeIter parent;

	if (gtk_tree_model_iter_parent (GTK_TREE_MODEL (ch->cv->store), &parent, &ch->iter))
	{
		gtk_tree_model_get (GTK_TREE_MODEL (ch->cv->store), &parent, COL_CHAN, &parent_ch, -1);
	}

	return parent_ch;
}

static gboolean
cv_tree_is_collapsed (chan *ch)
{
	chan *parent = cv_tree_get_parent (ch);
	GtkTreePath *path = NULL;
	gboolean ret;

	if (parent == NULL)
		return FALSE;

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (parent->cv->store),
											  &parent->iter);
	ret = !gtk_tree_view_row_expanded (((treeview *)parent->cv)->tree, path);
	gtk_tree_path_free (path);
	
	return ret;
}
