#define GTK_DISABLE_DEPRECATED

#include <string.h>
#include <stdlib.h>

#include <gtk/gtkarrow.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkviewport.h>
#include <gtk/gtkvseparator.h>

/* keep this code generic, don't include xchat.h! */

#include "tabs.h"

#undef ALPHA_SORT

/* ignore "toggled" signal? */
static int ignore_toggle = FALSE;


/* userdata for gobjects used here:
 *
 * group:
 *   "c" tab-focus callback function
 *   "foc" currently focused tab
 *   "i" inner hbox (inside the viewport)
 *   "v" set to 1 if group is vertical type
 *   "b1" first arrow button
 *   "b2" second arrow button
 *
 * family boxes inside group
 *   "f" family
 *
 * tab (togglebuttons inside boxes):
 *   "u" userdata passed to tab-focus callback function (sess)
 *   "f" family
 *   "g" group this tab is apart of
 *
 */

/*
 * GtkViewports request at least as much space as their children do.
 * If we don't intervene here, the GtkViewport will be granted its
 * request, even at the expense of resizing the top-level window.
 */
static void
tab_viewport_size_request (GtkWidget *widget, GtkRequisition *requisition, gpointer user_data)
{
	if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (user_data), "v")) == 0)
		requisition->width = 1;
	else
		requisition->height = 1;
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
tab_scroll_left_up_clicked (GtkWidget *widget, GtkWidget *group)
{
	GtkAdjustment *adj;
	gint viewport_size;
	gfloat new_value;
	GtkWidget *inner;
	gint vertical;
	gfloat i;

	inner = g_object_get_data (G_OBJECT (group), "i");
	vertical = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (group), "v"));

	if (vertical)
	{
		adj = gtk_viewport_get_vadjustment (GTK_VIEWPORT (inner->parent));
		gdk_window_get_geometry (inner->parent->window, 0, 0, 0, &viewport_size, 0);
	} else
	{
		adj = gtk_viewport_get_hadjustment (GTK_VIEWPORT (inner->parent));
		gdk_window_get_geometry (inner->parent->window, 0, 0, &viewport_size, 0, 0);
	}

	new_value = tab_search_offset (inner, adj->value, 0, vertical);

	if (new_value + viewport_size > adj->upper)
		new_value = adj->upper - viewport_size;

	for (i = adj->value; i > new_value; i -= 0.1)
	{
		gtk_adjustment_set_value (adj, i);
		while (g_main_pending ())
			g_main_iteration (TRUE);
	}

	gtk_adjustment_set_value (adj, new_value);
}

static void
tab_scroll_right_down_clicked (GtkWidget *widget, GtkWidget *group)
{
	GtkAdjustment *adj;
	gint viewport_size;
	gfloat new_value;
	GtkWidget *inner;
	gint vertical;
	gfloat i;

	inner = g_object_get_data (G_OBJECT (group), "i");
	vertical = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (group), "v"));

	if (vertical)
	{
		adj = gtk_viewport_get_vadjustment (GTK_VIEWPORT (inner->parent));
		gdk_window_get_geometry (inner->parent->window, 0, 0, 0, &viewport_size, 0);
	} else
	{
		adj = gtk_viewport_get_hadjustment (GTK_VIEWPORT (inner->parent));
		gdk_window_get_geometry (inner->parent->window, 0, 0, &viewport_size, 0, 0);
	}

	new_value = tab_search_offset (inner, adj->value, 1, vertical);

	if (new_value == 0 || new_value + viewport_size > adj->upper)
		new_value = adj->upper - viewport_size;

	for (i = adj->value; i < new_value; i += 0.1)
	{
		gtk_adjustment_set_value (adj, i);
		while (g_main_pending ())
			g_main_iteration (TRUE);
	}

	gtk_adjustment_set_value (adj, new_value);
}

int
tab_group_resize (GtkWidget *group)
{
	GtkAdjustment *adj;
	GtkWidget *inner;
	gint vertical;
	gint viewport_size;

	vertical = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (group), "v"));
	inner = g_object_get_data (G_OBJECT (group), "i");

	if (vertical)
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
		gtk_widget_hide (g_object_get_data (G_OBJECT (group), "b1"));
		gtk_widget_hide (g_object_get_data (G_OBJECT (group), "b2"));
	} else
	{
		gtk_widget_show (g_object_get_data (G_OBJECT (group), "b1"));
		gtk_widget_show (g_object_get_data (G_OBJECT (group), "b2"));
	}

	return 0;
}

/* called when a tab is clicked (button down) */

static void
tab_pressed_cb (GtkToggleButton *tab, GtkWidget *group)
{
	void (*callback) (GtkWidget *tab, gpointer userdata, gpointer family);
	GtkWidget *old_tab;

	ignore_toggle = TRUE;
	gtk_toggle_button_set_active (tab, TRUE);

	/* de-activate the old tab */
	old_tab = g_object_get_data (G_OBJECT (group), "foc");
	if (old_tab)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (old_tab), FALSE);
	ignore_toggle = FALSE;

	if (tab->active)
	{
		callback = g_object_get_data (G_OBJECT (group), "c");
		callback (GTK_WIDGET (tab), g_object_get_data (G_OBJECT (tab), "u"),
					 g_object_get_data (G_OBJECT (tab), "f"));

		g_object_set_data (G_OBJECT (group), "foc", tab);
	}
}

GtkWidget *
tab_group_new (void *callback, gboolean vertical)
{
	GtkWidget *box;
	GtkWidget *viewport;
	GtkWidget *group;
	GtkWidget *button;
	GtkWidget *arrow;

	if (vertical)
	{
		group = gtk_vbox_new (0, 0);
		g_object_set_data (G_OBJECT (group), "v", (gpointer)1);
	} else
		group = gtk_hbox_new (0, 0);
	g_object_set_data (G_OBJECT (group), "c", callback);
	gtk_container_set_border_width (GTK_CONTAINER (group), 2);
	gtk_widget_show (group);

	viewport = gtk_viewport_new (0, 0);
	gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);
	g_signal_connect (G_OBJECT (viewport), "size_request",
							G_CALLBACK (tab_viewport_size_request), group);
	gtk_box_pack_start (GTK_BOX (group), viewport, 1, 1, 0);
	gtk_widget_show (viewport);

	if (vertical)
		box = gtk_vbox_new (FALSE, 0);
	else
		box = gtk_hbox_new (FALSE, 0);
	g_object_set_data (G_OBJECT (group), "i", box);
	gtk_container_add (GTK_CONTAINER (viewport), box);
	gtk_widget_show (box);

	button = gtk_button_new ();
	g_object_set_data (G_OBJECT (group), "b1", button);
	arrow = gtk_arrow_new (vertical ? GTK_ARROW_DOWN : GTK_ARROW_RIGHT,
								  GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (button), arrow);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT (button), "clicked",
							G_CALLBACK (tab_scroll_right_down_clicked), group);
	gtk_box_pack_end (GTK_BOX (group), button, 0, 0, 0);
	gtk_widget_show_all (button);

	button = gtk_button_new ();
	g_object_set_data (G_OBJECT (group), "b2", button);
	arrow = gtk_arrow_new (vertical ? GTK_ARROW_UP : GTK_ARROW_LEFT,
								  GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (button), arrow);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT (button), "clicked",
							G_CALLBACK (tab_scroll_left_up_clicked), group);
	gtk_box_pack_end (GTK_BOX (group), button, 0, 0, 0);
	gtk_widget_show_all (button);

	return group;
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
tab_group_for_each_tab (GtkWidget *group,
								int (*callback) (GtkWidget *tab, int num, int usernum),
								int usernum)
{
	GList *tabs;
	GList *boxes;
	GtkBoxChild *child;
	GtkBox *innerbox;
	int i;

	innerbox = g_object_get_data (G_OBJECT (group), "i");
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
dummy (GtkWidget *tab, int num, int usernum)
{
	return -1;
}

/* returns the number of tabs a tab-group has */

int
tab_group_get_size (GtkWidget *group)
{
	return tab_group_for_each_tab (group, dummy, 0);
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
tab_group_get_cur_page (GtkWidget *group)
{
	return tab_group_for_each_tab (group, tab_check_focus_cb, 0);
}

static int
tab_focus_num_cb (GtkWidget *tab, int num, int want)
{
	if (num == want)
	{
		tab_focus (tab);
		return 1;
	}

	return -1;
}

/* switch to the tab number specified */

void
tab_group_switch (GtkWidget *group, int relative, int num)
{
	int i, max;

	while (g_main_pending ())
		g_main_iteration (TRUE);

	tab_group_resize (group);

	if (relative)
	{
		max = tab_group_get_size (group);
		i = tab_group_get_cur_page (group) + num;
		/* make it wrap around at both ends */
		if (i < 0)
			i = max - 1;
		if (i >= max)
			i = 0;
		tab_group_for_each_tab (group, tab_focus_num_cb, i);
		return;
	}

	tab_group_for_each_tab (group, tab_focus_num_cb, num);
}

void
tab_focus (GtkWidget *tab)
{
	GtkWidget *focus_tab;
	GtkWidget *group;

	group = g_object_get_data (G_OBJECT (tab), "g");
	focus_tab = g_object_get_data (G_OBJECT (group), "foc");

	if (focus_tab == tab)
		return;

	/* focus the new one (tab_pressed_cb defocuses the old one) */
	tab_pressed_cb (GTK_TOGGLE_BUTTON (tab), group);
}

/* remove empty boxes and separators */

void
tab_group_cleanup (GtkWidget *group)
{
	GList *boxes, *children;
	GtkWidget *box, *inner;
	GtkBoxChild *child;
	int empty;

	inner = g_object_get_data (G_OBJECT (group), "i");
	boxes = GTK_BOX (inner)->children;
	while (boxes)
	{
		child = boxes->data;
		box = child->widget;
		boxes = boxes->next;

#ifndef TABS_SPREAD
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
#else
		/* use this chance to destroy empty boxes */
		if (GTK_BOX (box)->children == NULL)
#endif
			gtk_widget_destroy (box);
	}
}

GtkWidget *
tab_group_get_focused (GtkWidget *group)
{
	return g_object_get_data (G_OBJECT (group), "foc");
}

static void
tab_add_sorted (GtkWidget *box, GtkWidget *tab)
{
#ifdef ALPHA_SORT
	GList *list;
	GtkBoxChild *child;
	char *name = GTK_BUTTON (tab)->label_text;
	int i = 0;

	list = GTK_BOX (box)->children;
	while (list)
	{
		child = list->data;
		if (!GTK_IS_SEPARATOR (child->widget))
		{
			if (strcasecmp (GTK_BUTTON (child->widget)->label_text, name) > 0)
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
#endif

	gtk_box_pack_start (GTK_BOX (box), tab, 0, 0, 0);
	gtk_widget_show (tab);
}

static void
tab_add_real (GtkWidget *group, GtkWidget *tab, void *family)
{
	GList *boxes, *children;
	GtkWidget *sep, *box, *inner;
	GtkBoxChild *child;
	int empty;

	inner = g_object_get_data (G_OBJECT (group), "i");
	/* see if a family for this tab already exists */
	boxes = GTK_BOX (inner)->children;
	while (boxes)
	{
		child = boxes->data;
		box = child->widget;

		if (g_object_get_data (G_OBJECT (box), "f") == family)
		{
			tab_add_sorted (box, tab);
			gtk_widget_queue_resize (inner->parent);
			return;
		}

		boxes = boxes->next;

#ifndef TABS_SPREAD
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
#else
		/* use this chance to destroy empty boxes */
		if (GTK_BOX (box)->children == NULL)
			gtk_widget_destroy (box);
#endif
	}

	/* create a new family box */
	if (g_object_get_data (G_OBJECT (group), "v") != NULL)
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
	g_object_set_data (G_OBJECT (box), "f", family);
	gtk_box_pack_start (GTK_BOX (box), tab, 0, 0, 0);
	gtk_widget_show (tab);
	gtk_widget_show (box);
	gtk_widget_queue_resize (inner->parent);
}

static void
tab_release_cb (GtkToggleButton *widget, gpointer user_data)
{
	/* don't let all tabs be OFF at the same time */
	ignore_toggle = TRUE;
	gtk_toggle_button_set_active (widget, TRUE);
	ignore_toggle = FALSE;
}

static gboolean
tab_ignore_cb (GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
	return TRUE;
}

/* called for keyboard tab toggles only */
static void
tab_toggled_cb (GtkToggleButton *tab, gpointer user_data)
{
	if (ignore_toggle)
		return;

	if (tab->active)
	{
		/* activated a tab via keyboard */
		tab_pressed_cb (tab, g_object_get_data (G_OBJECT (tab), "g"));
		return;
	}

	/* activate it */
	tab_release_cb (tab, NULL);
}

static char *
truncate_tab_name (char *name, int max)
{
	char *buf;

	if (max > 2 && g_utf8_strlen (name, -1) > max)
	{
		/* truncate long channel names */
		buf = malloc (strlen (name) + 4);
		strcpy (buf, name);
		g_utf8_offset_to_pointer (buf, max)[0] = 0;
		strcat (buf, "..");
		return buf;
	}

	return NULL;
}


static void
tab_drag_end (GtkWidget *widget, GdkDragContext *drag_context,
				  gpointer user_data)
{
	void (*callback) (GtkWidget *tab, gpointer userdata);

	if (!drag_context->dest_window)
	{
		callback = user_data;
		callback (widget, NULL);
	}
}

GtkWidget *
tab_group_add (GtkWidget *group, char *name, void *family, void *userdata,
			void *click_cb, void *delink_cb, int trunc_len)
{
	GtkWidget *but;
	char *new_name;
	static const GtkTargetEntry targets[] =
	{
		{"application/x-xchat-tab", 0, 1}
	};

	new_name = truncate_tab_name (name, trunc_len);
	if (new_name)
	{
		but = gtk_toggle_button_new_with_label (new_name);
		free (new_name);
	} else
		but = gtk_toggle_button_new_with_label (name);
	gtk_widget_set_name (but, "xchat-tab");
	/* used to trap right-clicks */
	g_signal_connect (G_OBJECT (but), "button-press-event",
						 	G_CALLBACK (click_cb), userdata);
	/* avoid prelights */
	g_signal_connect (G_OBJECT (but), "enter-notify-event",
						 	G_CALLBACK (tab_ignore_cb), NULL);
	g_signal_connect (G_OBJECT (but), "leave-notify-event",
						 	G_CALLBACK (tab_ignore_cb), NULL);
	g_signal_connect (G_OBJECT (but), "pressed",
							G_CALLBACK (tab_pressed_cb), group);
	g_signal_connect (G_OBJECT (but), "released",
						 	G_CALLBACK (tab_release_cb), NULL);
	/* for keyboard */
	g_signal_connect (G_OBJECT (but), "toggled",
						 	G_CALLBACK (tab_toggled_cb), NULL);

	g_object_set_data (G_OBJECT (but), "f", family);
	g_object_set_data (G_OBJECT (but), "g", group);
	g_object_set_data (G_OBJECT (but), "u", userdata);

	tab_add_real (group, but, family);

	/* DND for detaching tabs */
	gtk_drag_source_set (but, GDK_BUTTON1_MASK, targets, 1, GDK_ACTION_MOVE);
	gtk_drag_dest_set (but, GTK_DEST_DEFAULT_ALL, targets, 1, GDK_ACTION_MOVE);
	g_signal_connect (G_OBJECT (but), "drag-end",
							G_CALLBACK (tab_drag_end), delink_cb);

	return but;
}

void
tab_set_attrlist (GtkWidget *tab, PangoAttrList *list)
{
	gtk_label_set_attributes (GTK_LABEL (GTK_BIN (tab)->child), list);
}

void
tab_rename (GtkWidget *tab, char *name, int trunc_len)
{
	PangoAttrList *attr = gtk_label_get_attributes (GTK_LABEL (GTK_BIN (tab)->child));
	char *new_name;

	new_name = truncate_tab_name (name, trunc_len);
	if (new_name)
	{
		gtk_button_set_label (GTK_BUTTON (tab), new_name);
		free (new_name);
	} else
		gtk_button_set_label (GTK_BUTTON (tab), name);
	gtk_widget_queue_resize (tab->parent->parent->parent);

	if (attr)
		gtk_label_set_attributes (GTK_LABEL (GTK_BIN (tab)->child), attr);
}

void
tab_move (GtkWidget *tab, int delta)
{
	int i, pos = 0;
	GList *list;

	i = 0;
	for (list = GTK_BOX (tab->parent)->children; list; list = list->next)
	{
		GtkBoxChild *child_entry;

		child_entry = list->data;
		if (child_entry->widget == tab)
			pos = i;
		i++;
	}

	pos = (pos - delta) % i;
	gtk_box_reorder_child (GTK_BOX (tab->parent), tab, pos);
}

void
tab_family_move (GtkWidget *tab, int delta)
{
	int i, pos = 0;
	GList *list;
	GtkWidget *group, *inner, *box = NULL;
	void *family;

	group = g_object_get_data (G_OBJECT (tab), "g");
	family = g_object_get_data (G_OBJECT (tab), "f");
	if (!group || !family) return;
	inner = g_object_get_data (G_OBJECT (group), "i");
	if (!inner) return;
	
	/* find position of tab's family */
	i = 0;
	for (list = GTK_BOX (inner)->children; list; list = list->next)
	{
		GtkBoxChild *child_entry;
		void *fam;

		child_entry = list->data;
		fam = g_object_get_data (G_OBJECT (child_entry->widget), "f");
		if (fam == family) {
			box = child_entry->widget;
			pos = i;
		}
		i++;
	}

	pos = (pos - delta) % i;
	gtk_box_reorder_child (GTK_BOX (box->parent), box, pos);
}


void
tab_remove (GtkWidget *tab)
{
	GtkWidget *focus_tab;
	GtkWidget *group;

	group = g_object_get_data (G_OBJECT (tab), "g");
	gtk_widget_destroy (tab);

	focus_tab = g_object_get_data (G_OBJECT (group), "foc");
	if (focus_tab == tab)
	{
		g_object_set_data (G_OBJECT (group), "foc", NULL);
		tab_group_switch (group, 0, FALSE);
	}
}

GtkOrientation
tab_group_get_orientation (GtkWidget *group)
{
	int vertical;
	vertical = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (group), "v"));
	return (vertical ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL);
}

GtkWidget *
tab_group_set_orientation (GtkWidget *group, gboolean vertical)
{
	GtkWidget *box;
	GtkWidget *new_group;
	GList *boxes;
	int is_vertical;

	is_vertical = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (group), "v"));
	if ((vertical && is_vertical) || (!vertical && !is_vertical))
		return group;

	new_group = tab_group_new (g_object_get_data (G_OBJECT (group), "c"),
								vertical);
	g_object_set_data (G_OBJECT (new_group), "foc",
						g_object_get_data (G_OBJECT (group), "foc"));
	box = g_object_get_data (G_OBJECT (group), "i");
	boxes = GTK_BOX (box)->children;
	while (boxes)
	{
		GtkWidget *family_box;
		GList *children;

		family_box = ((GtkBoxChild *) boxes->data)->widget;
		children = GTK_BOX (family_box)->children;

		while (children)
		{
			GtkWidget *child;

			child = ((GtkBoxChild *) children->data)->widget;

			if (GTK_IS_TOGGLE_BUTTON (child))
			{
				void *family;

				gtk_widget_ref (child);
				gtk_container_remove (GTK_CONTAINER (family_box), child);
				g_signal_handlers_disconnect_by_func (G_OBJECT (child),
									G_CALLBACK (tab_pressed_cb), group);
				g_signal_connect (G_OBJECT (child), "pressed",
									G_CALLBACK (tab_pressed_cb), new_group);
				family = g_object_get_data (G_OBJECT (child), "f");
				g_object_set_data (G_OBJECT (child), "g", new_group);
				tab_add_real (new_group, child, family);
				gtk_widget_unref (child);
				children = GTK_BOX (family_box)->children;
			} else
				children = children->next;
		}

		boxes = boxes->next;
	}

	return new_group;
}

