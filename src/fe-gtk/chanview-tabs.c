/* file included in chanview.c */

typedef struct
{
	GtkWidget *outer;	/* outer box */
	GtkWidget *inner;	/* inner box */
	GtkWidget *b1;		/* button1 */
	GtkWidget *b2;		/* button2 */
} tabview;

static void chanview_populate (chanview *cv);

/* ignore "toggled" signal? */
static int ignore_toggle = FALSE;
static int tab_left_is_moving = 0;
static int tab_right_is_moving = 0;

/* userdata for gobjects used here:
 *
 * tab (togglebuttons inside boxes):
 *   "u" userdata passed to tab-focus callback function (sess)
 *   "c" the tab's (chan *)
 *
 * box (family box)
 *   "f" family
 *
 */

/*
 * GtkViewports request at least as much space as their children do.
 * If we don't intervene here, the GtkViewport will be granted its
 * request, even at the expense of resizing the top-level window.
 */
static void
cv_tabs_sizerequest (GtkWidget *viewport, GtkRequisition *requisition, chanview *cv)
{
	if (!cv->vertical)
		requisition->width = 1;
	else
		requisition->height = 1;
}

static void
cv_tabs_sizealloc (GtkWidget *widget, GtkAllocation *allocation, chanview *cv)
{
	GtkAdjustment *adj;
	GtkWidget *inner;
	gint viewport_size;

	inner = ((tabview *)cv)->inner;

	if (cv->vertical)
	{
		adj = gtk_viewport_get_vadjustment (GTK_VIEWPORT (inner->parent));
		gdk_window_get_geometry (inner->parent->window, 0, 0, 0, &viewport_size, 0);
	} else
	{
		adj = gtk_viewport_get_hadjustment (GTK_VIEWPORT (inner->parent));
		gdk_window_get_geometry (inner->parent->window, 0, 0, &viewport_size, 0, 0);
	}

	if (adj->upper <= viewport_size)
	{
		gtk_widget_hide (((tabview *)cv)->b1);
		gtk_widget_hide (((tabview *)cv)->b2);
	} else
	{
		gtk_widget_show (((tabview *)cv)->b1);
		gtk_widget_show (((tabview *)cv)->b2);
	}
}

static gint
tab_search_offset (GtkWidget *inner, gint start_offset,
				   gboolean forward, gboolean vertical)
{
	GList *boxes;
	GList *tabs;
	GtkWidget *box;
	GtkWidget *button;
	gint found;

	boxes = GTK_BOX (inner)->children;
	if (!forward && boxes)
		boxes = g_list_last (boxes);

	while (boxes)
	{
		box = ((GtkBoxChild *)boxes->data)->widget;
		boxes = (forward ? boxes->next : boxes->prev);

		tabs = GTK_BOX (box)->children;
		if (!forward && tabs)
			tabs = g_list_last (tabs);

		while (tabs)
		{
			button = ((GtkBoxChild *)tabs->data)->widget;
			tabs = (forward ? tabs->next : tabs->prev);

			if (!GTK_IS_TOGGLE_BUTTON (button))
				continue;

			found = (vertical ? button->allocation.y : button->allocation.x);
			if ((forward && found > start_offset) ||
				(!forward && found < start_offset))
				return found;
		}
	}

	return 0;
}

static void
tab_scroll_left_up_clicked (GtkWidget *widget, chanview *cv)
{
	GtkAdjustment *adj;
	gint viewport_size;
	gfloat new_value;
	GtkWidget *inner;
	gfloat i;

	inner = ((tabview *)cv)->inner;

	if (cv->vertical)
	{
		adj = gtk_viewport_get_vadjustment (GTK_VIEWPORT (inner->parent));
		gdk_window_get_geometry (inner->parent->window, 0, 0, 0, &viewport_size, 0);
	} else
	{
		adj = gtk_viewport_get_hadjustment (GTK_VIEWPORT (inner->parent));
		gdk_window_get_geometry (inner->parent->window, 0, 0, &viewport_size, 0, 0);
	}

	new_value = tab_search_offset (inner, adj->value, 0, cv->vertical);

	if (new_value + viewport_size > adj->upper)
		new_value = adj->upper - viewport_size;

	if (!tab_left_is_moving)
	{
		tab_left_is_moving = 1;

		for (i = adj->value; ((i > new_value) && (tab_left_is_moving)); i -= 0.1)
		{
			gtk_adjustment_set_value (adj, i);
			while (g_main_pending ())
				g_main_iteration (TRUE);
		}

		gtk_adjustment_set_value (adj, new_value);

		tab_left_is_moving = 0;		/* hSP: set to false in case we didnt get stopped (the normal case) */
	}
	else
	{
		tab_left_is_moving = 0;		/* hSP: jump directly to next element if user is clicking faster than we can scroll.. */
	}
}

static void
tab_scroll_right_down_clicked (GtkWidget *widget, chanview *cv)
{
	GtkAdjustment *adj;
	gint viewport_size;
	gfloat new_value;
	GtkWidget *inner;
	gfloat i;

	inner = ((tabview *)cv)->inner;

	if (cv->vertical)
	{
		adj = gtk_viewport_get_vadjustment (GTK_VIEWPORT (inner->parent));
		gdk_window_get_geometry (inner->parent->window, 0, 0, 0, &viewport_size, 0);
	} else
	{
		adj = gtk_viewport_get_hadjustment (GTK_VIEWPORT (inner->parent));
		gdk_window_get_geometry (inner->parent->window, 0, 0, &viewport_size, 0, 0);
	}

	new_value = tab_search_offset (inner, adj->value, 1, cv->vertical);

	if (new_value == 0 || new_value + viewport_size > adj->upper)
		new_value = adj->upper - viewport_size;

	if (!tab_right_is_moving)
	{
		tab_right_is_moving = 1;

		for (i = adj->value; ((i < new_value) && (tab_right_is_moving)); i += 0.1)
		{
			gtk_adjustment_set_value (adj, i);
			while (g_main_pending ())
				g_main_iteration (TRUE);
		}

		gtk_adjustment_set_value (adj, new_value);

		tab_right_is_moving = 0;		/* hSP: set to false in case we didnt get stopped (the normal case) */
	}
	else
	{
		tab_right_is_moving = 0;		/* hSP: jump directly to next element if user is clicking faster than we can scroll.. */
	}
}

static gboolean
tab_scroll_cb (GtkWidget *widget, GdkEventScroll *event, gpointer cv)
{
	/* mouse wheel scrolling */
	if (event->direction == GDK_SCROLL_UP)
		tab_scroll_left_up_clicked (widget, cv);
	else if (event->direction == GDK_SCROLL_DOWN)
		tab_scroll_right_down_clicked (widget, cv);

	return FALSE;
}

static void
cv_tabs_xclick_cb (GtkWidget *button, chanview *cv)
{
	cv->cb_xbutton (cv, cv->focused, cv->focused->tag, cv->focused->userdata);
}

/* make a Scroll (arrow) button */

static GtkWidget *
make_sbutton (GtkArrowType type, void *click_cb, void *userdata)
{
	GtkWidget *button, *arrow;

	button = gtk_button_new ();
	arrow = gtk_arrow_new (type, GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (button), arrow);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT (button), "clicked",
							G_CALLBACK (click_cb), userdata);
	g_signal_connect (G_OBJECT (button), "scroll_event",
							G_CALLBACK (tab_scroll_cb), userdata);
	gtk_widget_show (arrow);

	return button;
}

static void
cv_tabs_init (chanview *cv)
{
	GtkWidget *box, *hbox = NULL;
	GtkWidget *viewport;
	GtkWidget *outer;
	GtkWidget *button;

	if (cv->vertical)
		outer = gtk_vbox_new (0, 0);
	else
		outer = gtk_hbox_new (0, 0);
	((tabview *)cv)->outer = outer;
	g_signal_connect (G_OBJECT (outer), "size_allocate",
							G_CALLBACK (cv_tabs_sizealloc), cv);
/*	gtk_container_set_border_width (GTK_CONTAINER (outer), 2);*/
	gtk_widget_show (outer);

	viewport = gtk_viewport_new (0, 0);
	gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);
	g_signal_connect (G_OBJECT (viewport), "size_request",
							G_CALLBACK (cv_tabs_sizerequest), cv);
	g_signal_connect (G_OBJECT (viewport), "scroll_event",
							G_CALLBACK (tab_scroll_cb), cv);
	gtk_box_pack_start (GTK_BOX (outer), viewport, 1, 1, 0);
	gtk_widget_show (viewport);

	if (cv->vertical)
		box = gtk_vbox_new (FALSE, 0);
	else
		box = gtk_hbox_new (FALSE, 0);
	((tabview *)cv)->inner = box;
	gtk_container_add (GTK_CONTAINER (viewport), box);
	gtk_widget_show (box);

	/* if vertical, the buttons can be side by side */
	if (cv->vertical)
	{
		hbox = gtk_hbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (outer), hbox, 0, 0, 0);
		gtk_widget_show (hbox);
	}

	/* make the Scroll buttons */
	((tabview *)cv)->b2 = make_sbutton (cv->vertical ?
													GTK_ARROW_UP : GTK_ARROW_LEFT,
													tab_scroll_left_up_clicked,
													cv);

	((tabview *)cv)->b1 = make_sbutton (cv->vertical ?
													GTK_ARROW_DOWN : GTK_ARROW_RIGHT,
													tab_scroll_right_down_clicked,
													cv);

	if (hbox)
	{
		gtk_container_add (GTK_CONTAINER (hbox), ((tabview *)cv)->b2);
		gtk_container_add (GTK_CONTAINER (hbox), ((tabview *)cv)->b1);
	} else
	{
		gtk_box_pack_start (GTK_BOX (outer), ((tabview *)cv)->b2, 0, 0, 0);
		gtk_box_pack_start (GTK_BOX (outer), ((tabview *)cv)->b1, 0, 0, 0);
	}

	button = gtkutil_button (outer, GTK_STOCK_CLOSE, NULL, cv_tabs_xclick_cb,
									 cv, 0);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);

	gtk_container_add (GTK_CONTAINER (cv->box), outer);
}

static void
cv_tabs_postinit (chanview *cv)
{
}

static void
tab_add_sorted (chanview *cv, GtkWidget *box, GtkWidget *tab, chan *ch)
{
	GList *list;
	GtkBoxChild *child;
	int i = 0;
	void *b;

	if (!cv->sorted)
	{
		gtk_box_pack_start (GTK_BOX (box), tab, 0, 0, 0);
		gtk_widget_show (tab);
		return;
	}

	/* sorting TODO:
    *   - move tab if renamed (dialogs) */

	/* userdata, passed to mg_tabs_compare() */
	b = ch->userdata;

	list = GTK_BOX (box)->children;
	while (list)
	{
		child = list->data;
		if (!GTK_IS_SEPARATOR (child->widget))
		{
			void *a = g_object_get_data (G_OBJECT (child->widget), "u");

			if (ch->tag == 0 && cv->cb_compare (a, b) > 0)
			{
				gtk_box_pack_start (GTK_BOX (box), tab, 0, 0, 0);
				gtk_box_reorder_child (GTK_BOX (box), tab, i);
				gtk_widget_show (tab);
				return;
			}
		}
		i++;
		list = list->next;
	}

	/* append */
	gtk_box_pack_start (GTK_BOX (box), tab, 0, 0, 0);
	gtk_box_reorder_child (GTK_BOX (box), tab, i);
	gtk_widget_show (tab);
}

/* remove empty boxes and separators */

static void
cv_tabs_prune (chanview *cv)
{
	GList *boxes, *children;
	GtkWidget *box, *inner;
	GtkBoxChild *child;
	int empty;

	inner = ((tabview *)cv)->inner;
	boxes = GTK_BOX (inner)->children;
	while (boxes)
	{
		child = boxes->data;
		box = child->widget;
		boxes = boxes->next;

		/* check if the box is empty (except a vseperator) */
		empty = TRUE;
		children = GTK_BOX (box)->children;
		while (children)
		{
			if (!GTK_IS_SEPARATOR (((GtkBoxChild *)children->data)->widget))
			{
				empty = FALSE;
				break;
			}
			children = children->next;
		}

		if (empty)
			gtk_widget_destroy (box);
	}
}

static void
tab_add_real (chanview *cv, GtkWidget *tab, chan *ch)
{
	GList *boxes, *children;
	GtkWidget *sep, *box, *inner;
	GtkBoxChild *child;
	int empty;

	inner = ((tabview *)cv)->inner;
	/* see if a family for this tab already exists */
	boxes = GTK_BOX (inner)->children;
	while (boxes)
	{
		child = boxes->data;
		box = child->widget;

		if (g_object_get_data (G_OBJECT (box), "f") == ch->family)
		{
			tab_add_sorted (cv, box, tab, ch);
			gtk_widget_queue_resize (inner->parent);
			return;
		}

		boxes = boxes->next;

		/* check if the box is empty (except a vseperator) */
		empty = TRUE;
		children = GTK_BOX (box)->children;
		while (children)
		{
			if (!GTK_IS_SEPARATOR (((GtkBoxChild *)children->data)->widget))
			{
				empty = FALSE;
				break;
			}
			children = children->next;
		}

		if (empty)
			gtk_widget_destroy (box);
	}

	/* create a new family box */
	if (cv->vertical)
	{
		/* vertical */
		box = gtk_vbox_new (FALSE, 0);
		sep = gtk_hseparator_new ();
	} else
	{
		/* horiz */
		box = gtk_hbox_new (FALSE, 0);
		sep = gtk_vseparator_new ();
	}

	gtk_box_pack_end (GTK_BOX (box), sep, 0, 0, 4);
	gtk_widget_show (sep);
	gtk_box_pack_start (GTK_BOX (inner), box, 0, 0, 0);
	g_object_set_data (G_OBJECT (box), "f", ch->family);
	gtk_box_pack_start (GTK_BOX (box), tab, 0, 0, 0);
	gtk_widget_show (tab);
	gtk_widget_show (box);
	gtk_widget_queue_resize (inner->parent);
}

static gboolean
tab_ignore_cb (GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
	return TRUE;
}

/* called when a tab is clicked (button down) */

static void
tab_pressed_cb (GtkToggleButton *tab, chan *ch)
{
	chan *old_tab;
	int is_switching = TRUE;
	chanview *cv = ch->cv;

	ignore_toggle = TRUE;
	/* de-activate the old tab */
	old_tab = cv->focused;
	if (old_tab && old_tab->impl)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (old_tab->impl), FALSE);
		if (old_tab == ch)
			is_switching = FALSE;
	}
	gtk_toggle_button_set_active (tab, TRUE);
	ignore_toggle = FALSE;
	cv->focused = ch;

	if (/*tab->active*/is_switching)
		/* call the focus callback */
		cv->cb_focus (cv, ch, ch->tag, ch->userdata);
}

/* called for keyboard tab toggles only */
static void
tab_toggled_cb (GtkToggleButton *tab, chan *ch)
{
	if (ignore_toggle)
		return;

	/* activated a tab via keyboard */
	tab_pressed_cb (tab, ch);
}

static gboolean
tab_click_cb (GtkWidget *wid, GdkEventButton *event, chan *ch)
{
	return ch->cv->cb_contextmenu (ch->cv, ch, ch->tag, ch->userdata, event);
}

static void *
cv_tabs_add (chanview *cv, chan *ch, char *name, GtkTreeIter *parent)
{
	GtkWidget *but;

	but = gtk_toggle_button_new_with_label (name);
	gtk_widget_set_name (but, "xchat-tab");
	g_object_set_data (G_OBJECT (but), "c", ch);
	/* used to trap right-clicks */
	g_signal_connect (G_OBJECT (but), "button_press_event",
						 	G_CALLBACK (tab_click_cb), ch);
	/* avoid prelights */
	g_signal_connect (G_OBJECT (but), "enter_notify_event",
						 	G_CALLBACK (tab_ignore_cb), NULL);
	g_signal_connect (G_OBJECT (but), "leave_notify_event",
						 	G_CALLBACK (tab_ignore_cb), NULL);
	g_signal_connect (G_OBJECT (but), "pressed",
							G_CALLBACK (tab_pressed_cb), ch);
	/* for keyboard */
	g_signal_connect (G_OBJECT (but), "toggled",
						 	G_CALLBACK (tab_toggled_cb), ch);
	g_object_set_data (G_OBJECT (but), "u", ch->userdata);

	tab_add_real (cv, but, ch);

	return but;
}

/* traverse all the family boxes of tabs 
 *
 * A "group" is basically:
 * GtkV/HBox
 * `-GtkViewPort
 *   `-GtkV/HBox (inner box)
 *     `- GtkBox (family box)
 *        `- GtkToggleButton
 *        `- GtkToggleButton
 *        `- ...
 *     `- GtkBox
 *        `- GtkToggleButton
 *        `- GtkToggleButton
 *        `- ...
 *     `- ...
 *
 * */

static int
tab_group_for_each_tab (chanview *cv,
								int (*callback) (GtkWidget *tab, int num, int usernum),
								int usernum)
{
	GList *tabs;
	GList *boxes;
	GtkBoxChild *child;
	GtkBox *innerbox;
	int i;

	innerbox = (GtkBox *) ((tabview *)cv)->inner;
	boxes = innerbox->children;
	i = 0;
	while (boxes)
	{
		child = boxes->data;
		tabs = GTK_BOX (child->widget)->children;

		while (tabs)
		{
			child = tabs->data;

			if (!GTK_IS_SEPARATOR (child->widget))
			{
				if (callback (child->widget, i, usernum) != -1)
					return i;
				i++;
			}
			tabs = tabs->next;
		}

		boxes = boxes->next;
	}

	return i;
}

static int
tab_check_focus_cb (GtkWidget *tab, int num, int unused)
{
	if (GTK_TOGGLE_BUTTON (tab)->active)
		return num;

	return -1;
}

/* returns the currently focused tab number */

static int
tab_group_get_cur_page (chanview *cv)
{
	return tab_group_for_each_tab (cv, tab_check_focus_cb, 0);
}

static void
cv_tabs_focus (chan *ch)
{
	if (ch->impl)
	/* focus the new one (tab_pressed_cb defocuses the old one) */
		tab_pressed_cb (GTK_TOGGLE_BUTTON (ch->impl), ch);
}

static int
tab_focus_num_cb (GtkWidget *tab, int num, int want)
{
	if (num == want)
	{
		cv_tabs_focus (g_object_get_data (G_OBJECT (tab), "c"));
		return 1;
	}

	return -1;
}

static void
cv_tabs_change_orientation (chanview *cv)
{
	/* cleanup the old one */
	if (cv->func_cleanup)
		cv->func_cleanup (cv);

	/* now rebuild a new tabbar or tree */
	cv->func_init (cv);
	chanview_populate (cv);
}

/* switch to the tab number specified */

static void
cv_tabs_move_focus (chanview *cv, gboolean relative, int num)
{
	int i, max;

	if (relative)
	{
		max = cv->size;
		i = tab_group_get_cur_page (cv) + num;
		/* make it wrap around at both ends */
		if (i < 0)
			i = max - 1;
		if (i >= max)
			i = 0;
		tab_group_for_each_tab (cv, tab_focus_num_cb, i);
		return;
	}

	tab_group_for_each_tab (cv, tab_focus_num_cb, num);
}

static void
cv_tabs_remove (chan *ch)
{
	gtk_widget_destroy (ch->impl);
	ch->impl = NULL;

	cv_tabs_prune (ch->cv);
}

static void
cv_tabs_move (chan *ch, int delta)
{
	int i, pos = 0;
	GList *list;
	GtkWidget *parent = ((GtkWidget *)ch->impl)->parent;

	i = 0;
	for (list = GTK_BOX (parent)->children; list; list = list->next)
	{
		GtkBoxChild *child_entry;

		child_entry = list->data;
		if (child_entry->widget == ch->impl)
			pos = i;
		i++;
	}

	pos = (pos - delta) % i;
	gtk_box_reorder_child (GTK_BOX (parent), ch->impl, pos);
}

static void
cv_tabs_move_family (chan *ch, int delta)
{
	int i, pos = 0;
	GList *list;
	GtkWidget *box = NULL;

	/* find position of tab's family */
	i = 0;
	for (list = GTK_BOX (((tabview *)ch->cv)->inner)->children; list; list = list->next)
	{
		GtkBoxChild *child_entry;
		void *fam;

		child_entry = list->data;
		fam = g_object_get_data (G_OBJECT (child_entry->widget), "f");
		if (fam == ch->family)
		{
			box = child_entry->widget;
			pos = i;
		}
		i++;
	}

	pos = (pos - delta) % i;
	gtk_box_reorder_child (GTK_BOX (box->parent), box, pos);
}

static void
cv_tabs_cleanup (chanview *cv)
{
	if (cv->box)
		gtk_widget_destroy (((tabview *)cv)->outer);
}

static void
cv_tabs_set_color (chan *ch, PangoAttrList *list)
{
	gtk_label_set_attributes (GTK_LABEL (GTK_BIN (ch->impl)->child), list);
}

static void
cv_tabs_rename (chan *ch, char *name)
{
	PangoAttrList *attr;
	GtkWidget *tab = ch->impl;

	attr = gtk_label_get_attributes (GTK_LABEL (GTK_BIN (tab)->child));
	if (attr)
		pango_attr_list_ref (attr);

	gtk_button_set_label (GTK_BUTTON (tab), name);
	gtk_widget_queue_resize (tab->parent->parent->parent);

	if (attr)
	{
		gtk_label_set_attributes (GTK_LABEL (GTK_BIN (tab)->child), attr);
		pango_attr_list_unref (attr);
	}
}
