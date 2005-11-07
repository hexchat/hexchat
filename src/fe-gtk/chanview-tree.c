/* file included in chanview.c */

typedef struct
{
	GtkTreeView *tree;
	GtkWidget *scrollw;	/* scrolledWindow */
} treeview;

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
	GtkTreeSelection *sel;
	GtkTreePath *path;
	GtkTreeIter iter;
	int ret = FALSE;

	if (event->button != 3 && event->state == 0)
		return FALSE;

	sel = gtk_tree_view_get_selection (tree);
	if (gtk_tree_view_get_path_at_pos (tree, event->x, event->y, &path, 0, 0, 0))
	{
		if (event->button == 2)
		{
			gtk_tree_selection_unselect_all (sel);
			gtk_tree_selection_select_path (sel, path);
		}
		if (gtk_tree_model_get_iter (GTK_TREE_MODEL (cv->store), &iter, path))
		{
			gtk_tree_model_get (GTK_TREE_MODEL (cv->store), &iter, COL_CHAN, &ch, -1);
			ret = cv->cb_contextmenu (cv, ch, ch->tag, ch->userdata, event);
		}
		gtk_tree_path_free (path);
	}
	return ret;
}

static void
cv_tree_init (chanview *cv)
{
	GtkWidget *view, *win;
	GtkCellRenderer *renderer;

	win = gtk_scrolled_window_new (0, 0);
	/*gtk_container_set_border_width (GTK_CONTAINER (win), 1);*/
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (win),
													 GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win),
											  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (cv->box), win);
	gtk_widget_show (win);

	view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (cv->store));
	gtk_widget_set_name (view, "xchat-tree");
	if (cv->style)
		gtk_widget_set_style (view, cv->style);
	/*gtk_widget_modify_base (view, GTK_STATE_NORMAL, &colors[COL_BG]);*/
	GTK_WIDGET_UNSET_FLAGS (view, GTK_CAN_FOCUS);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
	gtk_container_add (GTK_CONTAINER (win), view);

	/* icon column */
	if (cv->use_icons)
	{
		renderer = gtk_cell_renderer_pixbuf_new ();
		gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
																	-1, NULL, renderer,
																	"pixbuf", COL_PIXBUF, NULL);
		gtk_tree_view_column_set_sizing (gtk_tree_view_get_column (GTK_TREE_VIEW (view), 0),
													GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	}

	/* main column */
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT (renderer), 1);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
																-1, NULL, renderer,
									"text", COL_NAME, "attributes", COL_ATTR, NULL);
	gtk_tree_view_column_set_sizing (gtk_tree_view_get_column (GTK_TREE_VIEW (view),
												cv->use_icons ? 1 : 0),
												GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (view))),
							"changed", G_CALLBACK (cv_tree_sel_cb), cv);
	g_signal_connect (G_OBJECT (view), "button-press-event",
							G_CALLBACK (cv_tree_click_cb), cv);
	g_signal_connect (G_OBJECT (view), "row-activated",
							G_CALLBACK (cv_tree_activated_cb), NULL);
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

	path = gtk_tree_model_get_path (model, &ch->iter);
	if (path)
	{
		gtk_tree_view_scroll_to_cell (tree, path, NULL, TRUE, 0.5, 0.5);
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
	/* nothing to do, it's already removed in the store */
}

static void
cv_tree_move (chan *ch, int delta)
{
}

static void
cv_tree_move_family (chan *ch, int delta)
{
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
