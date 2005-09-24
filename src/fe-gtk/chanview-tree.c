/* file included in chanview.c */

typedef struct
{
	GtkTreeView *tree;
	GtkWidget *scrollw;	/* scrolledWindow */
} treeview;

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
		cv->cb_focus (cv, ch, ch->family, ch->userdata);
	}
}

static gboolean
cv_tree_click_cb (GtkTreeView *tree, GdkEventButton *event, chanview *cv)
{
	chan *ch = cv->focused;	/* FIXME */
	GtkTreeSelection *sel;
	GtkTreePath *path;
	GtkTreeIter iter;

	if (event->button != 3)
		return FALSE;

	sel = gtk_tree_view_get_selection (tree);
	if (gtk_tree_view_get_path_at_pos (tree, event->x, event->y, &path, 0, 0, 0))
	{
/*		gtk_tree_selection_unselect_all (sel);
		gtk_tree_selection_select_path (sel, path);*/
		if (gtk_tree_model_get_iter (GTK_TREE_MODEL (cv->store), &iter, path))
			gtk_tree_model_get (GTK_TREE_MODEL (cv->store), &iter, COL_CHAN, &ch, -1);
		gtk_tree_path_free (path);
	}

	printf("tree click\n");
	return cv->cb_contextmenu (event, ch, ch->userdata);
}

static void
cv_tree_init (chanview *cv)
{
	GtkWidget *view, *win;
	GtkCellRenderer *renderer;

	win = gtk_scrolled_window_new (0, 0);
	gtk_container_add (GTK_CONTAINER (cv->box), win);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win),
											  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_show (win);

	view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (cv->store));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
	gtk_container_add (GTK_CONTAINER (win), view);

	/* main column */
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT (renderer), 1);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
																-1, NULL, renderer,
									"text", COL_NAME, "attributes", COL_ATTR, NULL);

	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (view))),
							"changed", G_CALLBACK (cv_tree_sel_cb), cv);
	g_signal_connect (G_OBJECT (view), "button_press_event",
							G_CALLBACK (cv_tree_click_cb), cv);
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

/* this thing is overly complicated */

static chan *
cv_tree_find_chan_by_number (chanview *cv, int num)
{
	GtkTreeIter iter, inner;
	chan *ch;
	int i = 0;

	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (cv->store), &iter))
	{
		do
		{
			if (i == num)
			{
				gtk_tree_model_get (GTK_TREE_MODEL (cv->store), &iter, COL_CHAN, &ch, -1);
				return ch;
			}
			i++;

			if (gtk_tree_model_iter_children (GTK_TREE_MODEL (cv->store), &inner, &iter))
			{
				do
				{
					if (i == num)
					{
						gtk_tree_model_get (GTK_TREE_MODEL (cv->store), &inner, COL_CHAN, &ch, -1);
						return ch;
					}
					i++;
				}
				while (gtk_tree_model_iter_next (GTK_TREE_MODEL (cv->store), &inner));
			}
		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (cv->store), &iter));
	}

	return NULL;
}

static void
cv_tree_move_focus (chanview *cv, gboolean relative, int num)
{
	chan *ch;

	if (!relative)
	{
		ch = cv_tree_find_chan_by_number (cv, num);
		if (ch)
			cv_tree_focus (ch);
	}
	else
	{

	}
}

static void
cv_tree_remove (chan *ch)
{
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
}

static void
cv_tree_rename (chan *ch, char *name, int trunc_len)
{
	/* nothing to do, it's already renamed in the store */
}
